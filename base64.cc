/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */
/******************************************************************************
  Copyright 2010 Todd Sundsted. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY TODD SUNDSTED ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
  EVENT SHALL TODD SUNDSTED OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  The views and conclusions contained in the software and documentation are
  those of the authors and should not be interpreted as representing official
  policies, either expressed or implied, of Todd Sundsted.
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "base64.h"
#include "exceptions.h"
#include "functions.h"
#include "storage.h"
#include "streams.h"
#include "utils.h"
#include "server.h"

static const unsigned char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable; %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned
 * buffer is null terminated to make it easier to use as a C
 * string. The null terminator is not included in out_len.
 */
char *
base64_encode(const char *src, size_t len, size_t *out_len)
{
    char *out, *pos;
    const unsigned char *end, *in;
    size_t olen;

    olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen++;                 /* nul termination */
    out = mymalloc(olen, M_STRING);
    if (out == NULL)
	return NULL;

    end = (unsigned char *)(src + len);
    in = (unsigned char *)src;
    pos = out;
    while (end - in >= 3) {
	*pos++ = base64_table[in[0] >> 2];
	*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
	*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
	*pos++ = base64_table[in[2] & 0x3f];
	in += 3;
    }

    if (end - in) {
	*pos++ = base64_table[in[0] >> 2];
	if (end - in == 1) {
	    *pos++ = base64_table[(in[0] & 0x03) << 4];
	    *pos++ = '=';
	} else {
	    *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
	    *pos++ = base64_table[(in[1] & 0x0f) << 2];
	}
	*pos++ = '=';
    }

    *pos = '\0';
    if (out_len)
	*out_len = pos - out;
    return out;
}

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
char *
base64_decode(const char *src, size_t len, size_t *out_len)
{
    char *out, *pos;
    unsigned char dtable[256], in[4], block[4], tmp;
    size_t i, count, olen;

    memset(dtable, 0x80, 256);
    for (i = 0; i < sizeof(base64_table); i++)
	dtable[base64_table[i]] = i;
    dtable['='] = 0;

    count = 0;
    for (i = 0; i < len; i++) {
	if (dtable[(unsigned char)src[i]] != 0x80)
	    count++;
    }

    if (count % 4)
	return NULL;

    olen = count / 4 * 3;
    pos = out = mymalloc(olen + 1, M_STRING);
    if (out == NULL)
	return NULL;

    count = 0;
    for (i = 0; i < len; i++) {
	tmp = dtable[(unsigned char)src[i]];
	if (tmp == 0x80)
	    continue;

	in[count] = src[i];
	block[count] = tmp;
	count++;
	if (count == 4) {
	    *pos++ = (block[0] << 2) | (block[1] >> 4);
	    *pos++ = (block[1] << 4) | (block[2] >> 2);
	    *pos++ = (block[2] << 6) | block[3];
	    count = 0;
	}
    }

    if (pos > out) {
	if (in[2] == '=')
	    pos -= 2;
	else if (in[3] == '=')
	    pos--;
    }

    *out_len = pos - out;
    return out;
}

/**** built in functions ****/

/**** helpers for catching overly large allocations ****/

#define TRY_STREAM     { enable_stream_exceptions(); TRY
#define ENDTRY_STREAM  ENDTRY  disable_stream_exceptions(); }

static package
make_space_pack()
{
    if (server_flag_option_cached(SVO_MAX_CONCAT_CATCHABLE))
	return make_error_pack(E_QUOTA);
    else
	return make_abort_pack(ABORT_SECONDS);
}

static package
bf_encode_base64(Var arglist, Byte next, void *vdata, Objid progr)
{
    int len;
    size_t length;
    const char *in = binary_to_raw_bytes(arglist.v.list[1].v.str, &len);
    if (NULL == in) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    char *out = base64_encode(in, (size_t)len, &length);
    if (NULL == out) { /* only happens if the encoder can't malloc */
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    static Stream *s = 0;
    if (!s)
	s = new_stream(100);
    TRY_STREAM {
	stream_add_raw_bytes_to_binary(s, out, (int)length);
    }
    EXCEPT (stream_too_big) {
	free_str(out);
	free_var(arglist);
	return make_space_pack();
    }
    ENDTRY_STREAM;
    Var ret;
    ret.type = TYPE_STR;
    ret.v.str = str_dup(reset_stream(s));
    free_str(out);
    free_var(arglist);
    return make_var_pack(ret);
}

static package
bf_decode_base64(Var arglist, Byte next, void *vdata, Objid progr)
{
    int len;
    size_t length;
    const char *in = binary_to_raw_bytes(arglist.v.list[1].v.str, &len);
    if (NULL == in) {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    char *out = base64_decode(in, (size_t)len, &length);
    if (NULL == out) { /* there are problems with the input string or the decoder can't malloc */
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }
    static Stream *s = 0;
    if (!s)
	s = new_stream(100);
    TRY_STREAM {
	stream_add_raw_bytes_to_binary(s, out, (int)length);
    }
    EXCEPT (stream_too_big) {
	free_str(out);
	free_var(arglist);
	return make_space_pack();
    }
    ENDTRY_STREAM;
    Var ret;
    ret.type = TYPE_STR;
    ret.v.str = str_dup(reset_stream(s));
    free_str(out);
    free_var(arglist);
    return make_var_pack(ret);
}

#undef TRY_STREAM
#undef ENDTRY_STREAM

void
register_base64(void)
{
    register_function("encode_base64", 1, 1, bf_encode_base64, TYPE_STR);
    register_function("decode_base64", 1, 1, bf_decode_base64, TYPE_STR);
}
