/*
 * SHA-256 implementation -- single-file, no dependencies.
 * Based on RFC 6234 / FIPS 180-4.
 * Public domain.
 */
#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t  buffer[64];
} SHA256_CTX;

void sha256_init(SHA256_CTX* ctx);
void sha256_update(SHA256_CTX* ctx, const uint8_t* data, size_t len);
void sha256_final(SHA256_CTX* ctx, uint8_t hash[32]);

/** Convenience: hash `len` bytes of `data` into `hash[32]`. */
void sha256(const uint8_t* data, size_t len, uint8_t hash[32]);

#ifdef __cplusplus
}
#endif

#endif /* SHA256_H */
