/*-
 * HMAC-SHA-224/256/384/512 implementation
 * Last update: 06/15/2005
 * Issue date:  06/15/2005
 *
 * Copyright (C) 2005 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>

#include "hmac_sha2.h"

/* HMAC-SHA-256 functions */

void hmac_sha256_starts(hmac_sha256_ctx *ctx, unsigned char *key,
                       unsigned int key_size)
{
    unsigned int fill;
    unsigned int num;

    unsigned char *key_used;
    unsigned char key_temp[SHA256_DIGEST_SIZE];
    int i;

    if (key_size == SHA256_BLOCK_SIZE) {
        key_used = key;
        num = SHA256_BLOCK_SIZE;
    } else {
        if (key_size > SHA256_BLOCK_SIZE){
            key_used = key_temp;
            num = SHA256_DIGEST_SIZE;
            sha256(key, key_size, key_used);
        } else { /* key_size > SHA256_BLOCK_SIZE */
            key_used = key;
            num = key_size;
        }
        fill = SHA256_BLOCK_SIZE - num;

        memset(ctx->block_ipad + num, 0x36, fill);
        memset(ctx->block_opad + num, 0x5c, fill);
    }

    for (i = 0; i < num; i++) {
        ctx->block_ipad[i] = key_used[i] ^ 0x36;
        ctx->block_opad[i] = key_used[i] ^ 0x5c;
    }

    sha256_starts(&ctx->ctx_inside);
    sha256_update(&ctx->ctx_inside, ctx->block_ipad, SHA256_BLOCK_SIZE);

    sha256_starts(&ctx->ctx_outside);
    sha256_update(&ctx->ctx_outside, ctx->block_opad,
                  SHA256_BLOCK_SIZE);

    /* for hmac_reinit */
    memcpy(&ctx->ctx_inside_reinit, &ctx->ctx_inside,
           sizeof(sha256_ctx));
    memcpy(&ctx->ctx_outside_reinit, &ctx->ctx_outside,
           sizeof(sha256_ctx));
}

void hmac_sha256_reinit(hmac_sha256_ctx *ctx)
{
    memcpy(&ctx->ctx_inside, &ctx->ctx_inside_reinit,
           sizeof(sha256_ctx));
    memcpy(&ctx->ctx_outside, &ctx->ctx_outside_reinit,
           sizeof(sha256_ctx));
}

void hmac_sha256_update(hmac_sha256_ctx *ctx, unsigned char *message,
                        unsigned int message_len)
{
    sha256_update(&ctx->ctx_inside, message, message_len);
}

void hmac_sha256_finish(hmac_sha256_ctx *ctx, unsigned char *mac,
                       unsigned int mac_size)
{
    unsigned char digest_inside[SHA256_DIGEST_SIZE];
    unsigned char mac_temp[SHA256_DIGEST_SIZE];

    sha256_finish(&ctx->ctx_inside, digest_inside);
    sha256_update(&ctx->ctx_outside, digest_inside, SHA256_DIGEST_SIZE);
    sha256_finish(&ctx->ctx_outside, mac_temp);
    memcpy(mac, mac_temp, mac_size);
}

void hmac_sha256(unsigned char *key, unsigned int key_size,
          unsigned char *message, unsigned int message_len,
          unsigned char *mac, unsigned mac_size)
{
    hmac_sha256_ctx ctx;

    hmac_sha256_starts(&ctx, key, key_size);
    hmac_sha256_update(&ctx, message, message_len);
    hmac_sha256_finish(&ctx, mac, mac_size);
}

#ifdef TEST_VECTORS

/* IETF Validation tests */

#include <stdio.h>
#include <stdlib.h>

void test(unsigned char *vector, unsigned char *digest,
          unsigned int digest_size)
{
    unsigned char output[2 * SHA256_DIGEST_SIZE + 1];
    int i;

    output[2 * digest_size] = '\0';

    for (i = 0; i < digest_size ; i++) {
       sprintf((char *) output + 2*i, "%02x", digest[i]);
    }

    printf("H: %s\n", output);
    if (strcmp((char *) vector, (char *) output)) {
        fprintf(stderr, "Test failed.\n");
        exit(1);
    }
}

int main()
{
    static unsigned char *vectors[] =
    {
        /* HMAC-SHA-256 */
        "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7",
        "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",
        "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe",
        "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b",
        "a3b6167473100ee06e0c796c2955552b",
        "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54",
        "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2",
    };

    static unsigned char *messages[] =
    {
        "Hi There",
        "what do ya want for nothing?",
        NULL,
        NULL,
        "Test With Truncation",
        "Test Using Larger Than Block-Size Key - Hash Key First",
        "This is a test using a larger than block-size key "
        "and a larger than block-size data. The key needs"
        " to be hashed before being used by the HMAC algorithm."
    };

    unsigned char mac[SHA256_DIGEST_SIZE];
    unsigned char *keys[7];
    unsigned int keys_len[7] = {20, 4, 20, 25, 20, 131, 131};
    unsigned int messages2and3_len = 50;
    unsigned int mac_256_size;
    int i;

    for (i = 0; i < 7; i++) {
        keys[i] = malloc(keys_len[i]);
        if (keys[i] == NULL) {
            fprintf(stderr, "Can't allocate memory\n");
            return 1;
        }
    }

    memset(keys[0], 0x0b, keys_len[0]);
    strcpy(keys[1], "Jefe");
    memset(keys[2], 0xaa, keys_len[2]);
    for (i = 0; i < keys_len[3]; i++)
        keys[3][i] = (unsigned char) i + 1;
    memset(keys[4], 0x0c, keys_len[4]);
    memset(keys[5], 0xaa, keys_len[5]);
    memset(keys[6], 0xaa, keys_len[6]);

    messages[2] = malloc(messages2and3_len + 1);
    messages[3] = malloc(messages2and3_len + 1);

    if (messages[2] == NULL || messages[3] == NULL) {
        fprintf(stderr, "Can't allocate memory\n");
        return 1;
    }

    messages[2][messages2and3_len] = '\0';
    messages[3][messages2and3_len] = '\0';

    memset(messages[2], 0xdd, messages2and3_len);
    memset(messages[3], 0xcd, messages2and3_len);

    printf("HMAC-SHA-2 IETF Validation tests\n\n");

    for (i = 0; i < 7; i++) {
        if (i != 4) {
            mac_256_size = SHA256_DIGEST_SIZE;
        } else {
            mac_256_size = 128 / 8;
        }

        printf("Test %d:\n", i + 1);

        hmac_sha256(keys[i], keys_len[i], messages[i], strlen(messages[i]),
                    mac, mac_256_size);
        test(vectors[i], mac, mac_256_size);
    }

    printf("All tests passed.\n");

    return 0;
}

#endif /* TEST_VECTORS */

