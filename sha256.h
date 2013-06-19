
/* MD5DEEP - sha256.h
 *
 * By Jesse Kornblum
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/* $Id: sha256.h 171 2009-01-15 23:04:12Z jessekornblum $ */

#ifndef _SHA256_H
#define _SHA256_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t total[2];
  uint32_t state[8];
  uint8_t buffer[64];
} context_sha256_t;

typedef context_sha256_t sha256_ctx;

void sha256_starts( context_sha256_t *ctx );

void sha256_update( context_sha256_t *ctx, uint8_t *input, uint32_t length );

void sha256_finish( context_sha256_t *ctx, uint8_t digest[32] );

void sha256( uint8_t *input, uint32_t length, uint8_t *digest );

int hash_init_sha256(void * ctx);
int hash_update_sha256(void * ctx, unsigned char *buf, uint64_t len);
int hash_final_sha256(void * ctx, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif /* sha256.h */
