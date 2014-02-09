/******************************************************************************
  Copyright 2010, 2011, 2012, 2013, 2014 Todd Sundsted. All rights reserved.

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

#include "base64.h"
#include "functions.h"
#include "log.h"
#include "storage.h"
#include "streams.h"
#include "utils.h"
#include "server.h"

static const unsigned char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char base64_url_safe_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/**** helpers for catching overly large allocations ****/

#define TRY_STREAM enable_stream_exceptions()
#define ENDTRY_STREAM disable_stream_exceptions()

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
    package pack;

    int safe = arglist.v.list[0].v.num > 1 && is_true(arglist.v.list[2]);

    const unsigned char *table = safe ? base64_url_safe_table : base64_table;

    /* check input */

    int len;
    const char *in;

    in = binary_to_raw_bytes(arglist.v.list[1].v.str, &len);

    if (!in) {
	pack = make_raise_pack(E_INVARG, "Invalid binary string", var_ref(arglist.v.list[1]));
	free_var(arglist);
	return pack;
    }

    /* encode */

    size_t newlen = len * 4 / 3 + 4 + 1;

    if (newlen > stream_alloc_maximum) {
	pack = make_space_pack();
	free_var(arglist);
	return pack;
    }

    char *p, *str;

    p = str = (char *)mymalloc(newlen, M_STRING);

    if (!str) {
	free_var(arglist);
	panic("BF_ENCODE_BASE64: failed to malloc\n");
	return no_var_pack();
    }

    const unsigned char *endp, *inp;

    endp = (unsigned char *)(in + len);
    inp = (unsigned char *)in;

    while (endp - inp >= 3) {
	*p++ = table[inp[0] >> 2];
	*p++ = table[((inp[0] & 0x03) << 4) | (inp[1] >> 4)];
	*p++ = table[((inp[1] & 0x0f) << 2) | (inp[2] >> 6)];
	*p++ = table[inp[2] & 0x3f];
	inp += 3;
    }

    if (endp - inp) {
	*p++ = table[inp[0] >> 2];
	if (endp - inp == 2) {
	    *p++ = table[((inp[0] & 0x03) << 4) | (inp[1] >> 4)];
	    *p++ = table[(inp[1] & 0x0f) << 2];
	} else {
	    *p++ = table[(inp[0] & 0x03) << 4];
	    if (!safe)
		*p++ = '=';
	}
	if (!safe)
	    *p++ = '=';
    }

    *p = '\0';

    /* return */

    Var ret;

    ret.type = TYPE_STR;
    ret.v.str = str;

    free_var(arglist);

    return make_var_pack(ret);
}

static package
bf_decode_base64(Var arglist, Byte next, void *vdata, Objid progr)
{
    package pack;

    int safe = arglist.v.list[0].v.num > 1 && is_true(arglist.v.list[2]);

    const unsigned char *table = safe ? base64_url_safe_table : base64_table;

    /* check input */

    unsigned int len;
    const char *in;

    unsigned char dict[256], tmp;
    size_t i, cnt = 0, pad = 0;

    in = arglist.v.list[1].v.str;
    len = memo_strlen(in);

    memset(dict, 0x80, 256);
    for (i = 0; i < 64; i++)
	dict[table[i]] = i;
    dict['='] = 0;

    for (i = 0; i < len; i++) {
	tmp = (unsigned char)in[i];
	if (dict[tmp] == 0x80) {
	    pack = make_raise_pack(E_INVARG, "Invalid character in encoded data", var_ref(arglist.v.list[1]));
	    free_var(arglist);
	    return pack;
	}
	if (pad && tmp != '=') {
	    pack = make_raise_pack(E_INVARG, "Pad character in encoded data", var_ref(arglist.v.list[1]));
	    free_var(arglist);
	    return pack;
	}
	if (dict[tmp] != 0x80)
	    cnt++;
	if (tmp == '=')
	    pad++;
    }

    if (!safe && cnt % 4) {
	pack = make_raise_pack(E_INVARG, "Invalid length", var_ref(arglist.v.list[1]));
	free_var(arglist);
	return pack;
    }
    if (pad > 2) {
	pack = make_raise_pack(E_INVARG, "Too many pad characters", var_ref(arglist.v.list[1]));
	free_var(arglist);
	return pack;
    }

    /* decode */

    size_t newlen = (cnt + 3) / 4 * 3 + 1;

    if (newlen > stream_alloc_maximum) {
	pack = make_space_pack();
	free_var(arglist);
	return pack;
    }

    char *p, *str;

    p = str = (char *)mymalloc(newlen, M_STRING);
    if (!str) {
	free_var(arglist);
	panic("BF_DECODE_BASE64: failed to malloc\n");
	return no_var_pack();
    }

    unsigned char ar[4], block[4];

    cnt = 0;
    for (i = 0; i < len + len % 4; i++) {
	if (i < len) {
	    tmp = dict[(unsigned char)in[i]];
	    ar[cnt] = in[i];
	} else {
	    tmp = '\0';
	    ar[cnt] = '=';
	}
	block[cnt++] = tmp;
	if (cnt == 4) {
	    *p++ = (block[0] << 2) | (block[1] >> 4);
	    *p++ = (block[1] << 4) | (block[2] >> 2);
	    *p++ = (block[2] << 6) | block[3];
	    cnt = 0;
	}
    }

    *p = '\0';

    if (p > str) {
	if (ar[2] == '=')
	    p -= 2;
	else if (ar[3] == '=')
	    p -= 1;
    }

    /* return */

    static Stream *s = 0;
    if (!s)
	s = new_stream(100);

    TRY_STREAM;
    try {
	stream_add_raw_bytes_to_binary(s, str, (int)(p - str));
	pack = make_var_pack(str_dup_to_var(reset_stream(s)));
    }
    catch (stream_too_big& exception) {
	reset_stream(s);
	pack = make_space_pack();
    }
    ENDTRY_STREAM;

    free_str(str);
    free_var(arglist);

    return pack;
}

void
register_base64(void)
{
    register_function("encode_base64", 1, 2, bf_encode_base64, TYPE_STR, TYPE_ANY);
    register_function("decode_base64", 1, 2, bf_decode_base64, TYPE_STR, TYPE_ANY);
}
