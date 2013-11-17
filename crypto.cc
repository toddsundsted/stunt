/******************************************************************************
  Copyright 2013 Todd Sundsted. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "functions.h"
#include "crypto.h"
#include "list.h"
#include "nettle/hmac.h"
#include "nettle/md5.h"
#include "nettle/ripemd160.h"
#include "nettle/sha1.h"
#include "nettle/sha2.h"
#include "random.h"
#include "server.h"
#include "storage.h"
#include "utils.h"

/* supported algorithms */

static int algorithms = 0;

#define MD5    (1<<1)
#define SHA256 (1<<2)
#define SHA512 (1<<3)
#define BCRYPT (1<<4)

#define HAS_MD5    (algorithms | MD5)
#define HAS_SHA256 (algorithms | SHA256)
#define HAS_SHA512 (algorithms | SHA512)
#define HAS_BCRYPT (algorithms | BCRYPT)

extern "C" {

    extern unsigned char _crypt_itoa64[];

    extern char *_crypt_gensalt_md5_rn(const char *, unsigned long,
                                       const char *, int,
                                       char *, int);
    extern char *_crypt_gensalt_sha2_rn(const char *, unsigned long,
                                        const char *, int,
                                        char *, int);
    extern char *_crypt_gensalt_blowfish_rn(const char *, unsigned long,
                                            const char *, int,
                                            char *, int);
    extern char *_crypt_gensalt_traditional_rn(const char *, unsigned long,
                                               const char *, int,
                                               char *, int);
    extern char *_crypt_blowfish_rn(const char *, const char *,
                                    char *, int);
}

/* Parse a salt prefix.  Identify the format, the count (AKA rounds or
 * work factor), and the start of any trailing characters not part of
 * the prefix.  Returns 1 if successful, otherwise 0.
 */
static int
parse_prefix(const char *prefix, size_t prefix_length,
             const char **rest, size_t *rest_length,
             unsigned long *count, int *format)
{
    *count = 0;
    *format = 0;
    *rest = prefix;
    *rest_length = prefix_length;

    char *str_end;

    if (3 <= prefix_length &&
        (('1' == prefix[1] && HAS_MD5) ||
         ('5' == prefix[1] && HAS_SHA256) ||
         ('6' == prefix[1] && HAS_SHA512))) {
	if (!strncmp("$1$", prefix, 3)) {
	    *format = MD5;
	    *rest = prefix + 3;
	    *rest_length = prefix_length - 3;
	    return 1;
	}
	else if (!strncmp("$5$", prefix, 3))
	    *format = SHA256;
	else if (!strncmp("$6$", prefix, 3))
	    *format = SHA512;
	else
	    return 0;
	int i;
	for (i = prefix_length - 1; 3 <= i; i--)
	    if (prefix[i] == '$')
		break;
	if (3 <= i && 11 <= prefix_length && !strncmp("rounds=", prefix + 3, 7)) {
	    errno = 0;
	    if ((*count = strtol(prefix + 10, &str_end, 10), prefix + 10 < str_end) &&
	          prefix + i == str_end &&
	          *count >= 1000 && *count <= 999999999 &&
	          !errno) {
		*rest = str_end + 1;
		*rest_length = prefix_length - (*rest - prefix);
		return 1;
	    }
	    else
		return 0;
	}
	else if (3 <= i)
	    return 0;
	*rest = prefix + 3;
	*rest_length = prefix_length - 3;
	return 1;
    }
    else if (4 <= prefix_length &&
             prefix[0] == '$' && prefix[1] == '2' &&
             (prefix[2] == 'a' || prefix[2] == 'x' || prefix[2] == 'y') &&
             prefix[3] == '$') {
	*format = BCRYPT;
	int i;
	for (i = prefix_length - 1; 4 <= i; i--)
	    if (prefix[i] == '$')
		break;
	if (4 <= i && 7 <= prefix_length) {
	    errno = 0;
	    if ((*count = strtol(prefix + 4, &str_end, 10), prefix + 4 < str_end) &&
	          prefix + i == str_end &&
	          *count >= 4 && *count <= 31 &&
	          !errno) {
		*rest = str_end + 1;
		*rest_length = prefix_length - (*rest - prefix);
		return 1;
	    }
	    else
		return 0;
	}
	else if (4 <= i)
	    return 0;
	*rest = prefix + 4;
	*rest_length = prefix_length - 4;
	return 1;
    }

    return 1;
}

static const char *
md5_hash_bytes(const char *input, int length)
{
    md5_ctx context;
    unsigned char result[16];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("12345678901234567890123456789012");
    const char *answer = hex;

    md5_init(&context);
    md5_update(&context, length, (unsigned char *)input);
    md5_digest(&context, 16, result);
    for (i = 0; i < 16; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
sha1_hash_bytes(const char *input, int length)
{
    sha1_ctx context;
    unsigned char result[20];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("1234567890123456789012345678901234567890");
    const char *answer = hex;

    sha1_init(&context);
    sha1_update(&context, length, (unsigned char *)input);
    sha1_digest(&context, 20, result);
    for (i = 0; i < 20; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
sha224_hash_bytes(const char *input, int length)
{
    sha224_ctx context;
    unsigned char result[28];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("12345678901234567890123456789012345678901234567890123456");
    const char *answer = hex;

    sha224_init(&context);
    sha224_update(&context, length, (unsigned char *)input);
    sha224_digest(&context, 28, result);
    for (i = 0; i < 28; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
sha256_hash_bytes(const char *input, int length)
{
    sha256_ctx context;
    unsigned char result[32];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("1234567890123456789012345678901234567890123456789012345678901234");
    const char *answer = hex;

    sha256_init(&context);
    sha256_update(&context, length, (unsigned char *)input);
    sha256_digest(&context, 32, result);
    for (i = 0; i < 32; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
sha384_hash_bytes(const char *input, int length)
{
    sha384_ctx context;
    unsigned char result[48];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456");
    const char *answer = hex;

    sha384_init(&context);
    sha384_update(&context, length, (unsigned char *)input);
    sha384_digest(&context, 48, result);
    for (i = 0; i < 48; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
sha512_hash_bytes(const char *input, int length)
{
    sha512_ctx context;
    unsigned char result[64];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678");
    const char *answer = hex;

    sha512_init(&context);
    sha512_update(&context, length, (unsigned char *)input);
    sha512_digest(&context, 64, result);
    for (i = 0; i < 64; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
ripemd160_hash_bytes(const char *input, int length)
{
    ripemd160_ctx context;
    unsigned char result[20];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("1234567890123456789012345678901234567890");
    const char *answer = hex;

    ripemd160_init(&context);
    ripemd160_update(&context, length, (unsigned char *)input);
    ripemd160_digest(&context, 20, result);
    for (i = 0; i < 20; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
hmac_sha1_bytes(const char *message, int message_length, const char *key, int key_length)
{
    hmac_sha1_ctx context;
    unsigned char result[20];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("1234567890123456789012345678901234567890");
    const char *answer = hex;

    hmac_sha1_set_key(&context, key_length, (unsigned char *)key);
    hmac_sha1_update(&context, message_length, (unsigned char *)message);
    hmac_sha1_digest(&context, 20, result);
    for (i = 0; i < 20; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

static const char *
hmac_sha256_bytes(const char *message, int message_length, const char *key, int key_length)
{
    hmac_sha256_ctx context;
    unsigned char result[32];
    int i;
    const char digits[] = "0123456789ABCDEF";
    char *hex = str_dup("1234567890123456789012345678901234567890123456789012345678901234");
    const char *answer = hex;

    hmac_sha256_set_key(&context, key_length, (unsigned char *)key);
    hmac_sha256_update(&context, message_length, (unsigned char *)message);
    hmac_sha256_digest(&context, 32, result);
    for (i = 0; i < 32; i++) {
	*hex++ = digits[result[i] >> 4];
	*hex++ = digits[result[i] & 0xF];
    }
    return answer;
}

/**** built in functions ****/

static package
bf_salt(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (prefix, input) */
    Var r;
    package p;

    const char *prefix = arglist.v.list[1].v.str;
    size_t prefix_length = memo_strlen(prefix);
    const char *input = arglist.v.list[2].v.str;
    size_t input_length = memo_strlen(input);

    const char *rest;
    size_t rest_length;
    unsigned long count;
    int format;

    if (!parse_prefix(prefix, prefix_length, &rest, &rest_length, &count, &format)) {
	p = make_raise_pack(E_INVARG, "Invalid prefix", var_ref(arglist.v.list[1]));
	free_var(arglist);
	return p;
    }

    char *(*use)(const char *, unsigned long,
                 const char *, int,
                 char *, int);

    if (MD5 == format)
	use = _crypt_gensalt_md5_rn;
    else if (SHA256 == format || SHA512 == format)
	use = _crypt_gensalt_sha2_rn;
    else if (BCRYPT == format)
	use = _crypt_gensalt_blowfish_rn;
    else if (!prefix[0] ||
             (2 <= prefix_length &&
              memchr(_crypt_itoa64, prefix[0], 64) &&
              memchr(_crypt_itoa64, prefix[1], 64)))
	use = _crypt_gensalt_traditional_rn;
    else {
	p = make_raise_pack(E_INVARG, "Invalid prefix", var_ref(arglist.v.list[1]));
	free_var(arglist);
	return p;
    }

    int random_length;

    const char *random = binary_to_raw_bytes(input, &random_length);

    if (NULL == random) {
	p = make_raise_pack(E_INVARG, "Invalid binary input", var_ref(arglist.v.list[2]));
	free_var(arglist);
	return p;
    }

    errno = 0;

    char output[64];
    char *ret = use(prefix, count, random, random_length, output, sizeof(output));

    if (errno) {
	p = make_error_pack(E_INVARG);
	free_var(arglist);
	return p;
    }

    r.type = TYPE_STR;
    r.v.str = str_dup(ret);

    free_var(arglist);

    return make_var_pack(r);
}

static package
bf_crypt(Var arglist, Byte next, void *vdata, Objid progr)
{				/* (string, [salt]) */
    Var r;
    package p;

    const char *salt;
    int salt_length;

    static char saltstuff[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

    char temp[3];

    if (arglist.v.list[0].v.num == 1 || memo_strlen(arglist.v.list[2].v.str) < 2) {
	/* provide a random 2-letter salt, works with old and new crypts */
	temp[0] = saltstuff[RANDOM() % (int) strlen(saltstuff)];
	temp[1] = saltstuff[RANDOM() % (int) strlen(saltstuff)];
	temp[2] = '\0';
	salt = temp;
	salt_length = 2;
    } else {
	salt = arglist.v.list[2].v.str;
	salt_length = memo_strlen(arglist.v.list[2].v.str);
    }

    const char *rest;
    size_t rest_length;
    unsigned long count;
    int format;

    int success = parse_prefix(salt, salt_length, &rest, &rest_length, &count, &format);

    if (!success) {
	r.type = TYPE_STR;
	r.v.str = str_dup(salt);
	package p = make_raise_pack(E_INVARG, "Invalid salt", r);
	free_var(arglist);
	return p;
    }
    if (!is_wizard(progr) &&
        ((BCRYPT == format && count != 5) ||
         (BCRYPT != format && count))) {
	package p = make_raise_pack(E_PERM, "Cannot specify non-default strength", new_int(count));
	free_var(arglist);
	return p;
    }

    if (BCRYPT == format) {
	errno = 0;

	char output[64];
	char *ret = _crypt_blowfish_rn(arglist.v.list[1].v.str, salt, output, sizeof(output));

	if (errno) {
	    free_var(arglist);
	    return make_error_pack(E_INVARG);
	}

	r.type = TYPE_STR;
	r.v.str = str_dup(ret);
    }
    else {
#if HAVE_CRYPT
	r.type = TYPE_STR;
	r.v.str = str_dup(crypt(arglist.v.list[1].v.str, salt));
#else
	r.type = TYPE_STR;
	r.v.str = str_ref(arglist.v.list[1].v.str);
#endif
    }

    free_var(arglist);

    return make_var_pack(r);
}

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
bf_string_hash(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    int nargs = arglist.v.list[0].v.num;
    const char *str = arglist.v.list[1].v.str;
    const char *algo = 1 < nargs ? arglist.v.list[2].v.str : NULL;

    if (1 == nargs || (1 < nargs && !mystrcasecmp("sha256", algo))) {
	r.type = TYPE_STR;
	r.v.str = sha256_hash_bytes(str, memo_strlen(str));
    }
    else if (1 < nargs && !mystrcasecmp("sha224", algo)) {
	r.type = TYPE_STR;
	r.v.str = sha224_hash_bytes(str, memo_strlen(str));
    }
    else if (1 < nargs && !mystrcasecmp("sha384", algo)) {
	r.type = TYPE_STR;
	r.v.str = sha384_hash_bytes(str, memo_strlen(str));
    }
    else if (1 < nargs && !mystrcasecmp("sha512", algo)) {
	r.type = TYPE_STR;
	r.v.str = sha512_hash_bytes(str, memo_strlen(str));
    }
    else if (1 < nargs && !mystrcasecmp("sha1", algo)) {
	r.type = TYPE_STR;
	r.v.str = sha1_hash_bytes(str, memo_strlen(str));
    }
    else if (1 < nargs && !mystrcasecmp("ripemd160", algo)) {
	r.type = TYPE_STR;
	r.v.str = ripemd160_hash_bytes(str, memo_strlen(str));
    }
    else if (1 < nargs && !mystrcasecmp("md5", algo)) {
	r.type = TYPE_STR;
	r.v.str = md5_hash_bytes(str, memo_strlen(str));
    }
    else {
	free_var(arglist);
	return make_error_pack(E_INVARG);
    }

    free_var(arglist);
    return make_var_pack(r);
}

static package
bf_binary_hash(Var arglist, Byte next, void *vdata, Objid progr)
{
    package p;

    TRY_STREAM;
    try {
	Var r;
	int length;
	int nargs = arglist.v.list[0].v.num;
	const char *bytes = binary_to_raw_bytes(arglist.v.list[1].v.str, &length);
	const char *algo = 1 < nargs ? arglist.v.list[2].v.str : NULL;

	if (!bytes) {
	    p = make_error_pack(E_INVARG);
	}
	else if (1 == nargs || (1 < nargs && !mystrcasecmp("sha256", algo))) {
	    r.type = TYPE_STR;
	    r.v.str = sha256_hash_bytes(bytes, length);
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha224", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha224_hash_bytes(bytes, length);
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha384", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha384_hash_bytes(bytes, length);
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha512", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha512_hash_bytes(bytes, length);
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha1", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha1_hash_bytes(bytes, length);
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("ripemd160", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = ripemd160_hash_bytes(bytes, length);
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("md5", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = md5_hash_bytes(bytes, length);
	    p = make_var_pack(r);
	}
	else {
	  p = make_error_pack(E_INVARG);
	}
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;

    free_var(arglist);
    return p;
}

static package
bf_value_hash(Var arglist, Byte next, void *vdata, Objid progr)
{
    package p;
    Stream *s = new_stream(100);

    alter_ticks_remaining(-1000);

    TRY_STREAM;
    try {
	Var r;
	int nargs = arglist.v.list[0].v.num;
	const char *algo = 1 < nargs ? arglist.v.list[2].v.str : NULL;

	unparse_value(s, arglist.v.list[1]);

	if (1 == nargs || (1 < nargs && !mystrcasecmp("sha256", algo))) {
	    r.type = TYPE_STR;
	    r.v.str = sha256_hash_bytes(stream_contents(s), stream_length(s));
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha224", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha224_hash_bytes(stream_contents(s), stream_length(s));
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha384", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha384_hash_bytes(stream_contents(s), stream_length(s));
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha512", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha512_hash_bytes(stream_contents(s), stream_length(s));
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("sha1", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = sha1_hash_bytes(stream_contents(s), stream_length(s));
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("ripemd160", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = ripemd160_hash_bytes(stream_contents(s), stream_length(s));
	    p = make_var_pack(r);
	}
	else if (1 < nargs && !mystrcasecmp("md5", algo)) {
	    r.type = TYPE_STR;
	    r.v.str = md5_hash_bytes(stream_contents(s), stream_length(s));
	    p = make_var_pack(r);
	}
	else {
	    p = make_error_pack(E_INVARG);
	}
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;

    free_stream(s);
    free_var(arglist);
    return p;
}

static package
bf_string_hmac(Var arglist, Byte next, void *vdata, Objid progr)
{
    package p;
    Stream *s = new_stream(100);

    TRY_STREAM;
    try {
	Var r;

	int nargs = arglist.v.list[0].v.num;
	const char *algo = 2 < nargs ? arglist.v.list[3].v.str : NULL;

	const char *str = arglist.v.list[1].v.str;
	int str_length = memo_strlen(str);

	int key_length;
	const char *key = binary_to_raw_bytes(arglist.v.list[2].v.str, &key_length);

	if (!key) {
	    p = make_error_pack(E_INVARG);
	}
	else {
	    char *key_new = (char *)mymalloc(key_length, M_STRING);
	    memcpy(key_new, key, key_length);
	    key = key_new;

	    if (2 == nargs || (2 < nargs && !mystrcasecmp("sha256", algo))) {
		r.type = TYPE_STR;
		r.v.str = hmac_sha256_bytes(str, str_length, key, key_length);
		p = make_var_pack(r);
	    }
	    else if (2 < nargs && !mystrcasecmp("sha1", algo)) {
		r.type = TYPE_STR;
		r.v.str = hmac_sha1_bytes(str, str_length, key, key_length);
		p = make_var_pack(r);
	    }
	    else {
		p = make_error_pack(E_INVARG);
	    }

	    free_str(key);
	}
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;

    free_stream(s);
    free_var(arglist);
    return p;
}

static package
bf_binary_hmac(Var arglist, Byte next, void *vdata, Objid progr)
{
    package p;
    Stream *s = new_stream(100);

    TRY_STREAM;
    try {
	Var r;

	int nargs = arglist.v.list[0].v.num;
	const char *algo = 2 < nargs ? arglist.v.list[3].v.str : NULL;

	int bytes_length;
	const char *bytes = binary_to_raw_bytes(arglist.v.list[1].v.str, &bytes_length);

	if (!bytes) {
	    p = make_error_pack(E_INVARG);
	}
	else {
	    char *bytes_new = (char *)mymalloc(bytes_length, M_STRING);
	    memcpy(bytes_new, bytes, bytes_length);
	    bytes = bytes_new;

	    int key_length;
	    const char *key = binary_to_raw_bytes(arglist.v.list[2].v.str, &key_length);

	    if (!key) {
		free_str(bytes);
		p = make_error_pack(E_INVARG);
	    }
	    else {
		char *key_new = (char *)mymalloc(key_length, M_STRING);
		memcpy(key_new, key, key_length);
		key = key_new;

		if (2 == nargs || (2 < nargs && !mystrcasecmp("sha256", algo))) {
		    r.type = TYPE_STR;
		    r.v.str = hmac_sha256_bytes(bytes, bytes_length, key, key_length);
		    p = make_var_pack(r);
		}
		else if (2 < nargs && !mystrcasecmp("sha1", algo)) {
		    r.type = TYPE_STR;
		    r.v.str = hmac_sha1_bytes(bytes, bytes_length, key, key_length);
		    p = make_var_pack(r);
		}
		else {
		    p = make_error_pack(E_INVARG);
		}

		free_str(bytes);
		free_str(key);
	    }
	}
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;

    free_stream(s);
    free_var(arglist);
    return p;
}

static package
bf_value_hmac(Var arglist, Byte next, void *vdata, Objid progr)
{
    package p;
    Stream *s = new_stream(100);

    TRY_STREAM;
    try {
	Var r;

	int nargs = arglist.v.list[0].v.num;
	const char *algo = 2 < nargs ? arglist.v.list[3].v.str : NULL;

	unparse_value(s, arglist.v.list[1]);
	const char *lit = str_dup(stream_contents(s));
	int lit_length = stream_length(s);

	int key_length;
	const char *key = binary_to_raw_bytes(arglist.v.list[2].v.str, &key_length);

	if (!key) {
	    free_str(lit);
	    p = make_error_pack(E_INVARG);
	}
	else {
	    char *key_new = (char *)mymalloc(key_length, M_STRING);
	    memcpy(key_new, key, key_length);
	    key = key_new;

	    if (2 == nargs || (2 < nargs && !mystrcasecmp("sha256", algo))) {
		r.type = TYPE_STR;
		r.v.str = hmac_sha256_bytes(lit, lit_length, key, key_length);
		p = make_var_pack(r);
	    }
	    else if (2 < nargs && !mystrcasecmp("sha1", algo)) {
		r.type = TYPE_STR;
		r.v.str = hmac_sha1_bytes(lit, lit_length, key, key_length);
		p = make_var_pack(r);
	    }
	    else {
		p = make_error_pack(E_INVARG);
	    }

	    free_str(lit);
	    free_str(key);
	}
    }
    catch (stream_too_big& exception) {
	p = make_space_pack();
    }
    ENDTRY_STREAM;

    free_stream(s);
    free_var(arglist);
    return p;
}

#undef TRY_STREAM
#undef ENDTRY_STREAM

void
register_crypto(void)
{
    if (!strncmp("$1$", crypt("password", "$1$"), 3))
	algorithms = HAS_MD5;
    if (!strncmp("$5$", crypt("password", "$5$"), 3))
	algorithms = HAS_SHA256;
    if (!strncmp("$6$", crypt("password", "$6$"), 3))
	algorithms = HAS_SHA512;
    algorithms = HAS_BCRYPT;

    register_function("salt", 2, 2, bf_salt, TYPE_STR, TYPE_STR);
    register_function("crypt", 1, 2, bf_crypt, TYPE_STR, TYPE_STR);

    register_function("string_hash", 1, 2, bf_string_hash, TYPE_STR, TYPE_STR);
    register_function("binary_hash", 1, 2, bf_binary_hash, TYPE_STR, TYPE_STR);
    register_function("value_hash", 1, 2, bf_value_hash, TYPE_ANY, TYPE_STR);

    register_function("string_hmac", 2, 3, bf_string_hmac, TYPE_STR, TYPE_STR, TYPE_STR);
    register_function("binary_hmac", 2, 3, bf_binary_hmac, TYPE_STR, TYPE_STR, TYPE_STR);
    register_function("value_hmac", 2, 3, bf_value_hmac, TYPE_ANY, TYPE_STR, TYPE_STR);
}
