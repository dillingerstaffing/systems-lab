#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include <stdint.h>

/* djb2, 64-bit variant: h = h * 33 + c for each byte c, seed 5381.
 * All arithmetic is unsigned 64-bit, wrapping modulo 2^64.
 * Processes `len` bytes; embedded zero bytes are hashed as bytes. */
uint64_t djb2_64(const unsigned char *data, size_t len);

/* FNV-1a, 64-bit: h = (h ^ c) * 1099511628211 for each byte c,
 * offset basis 14695981039346656037. All arithmetic unsigned 64-bit. */
uint64_t fnv1a_64(const unsigned char *data, size_t len);

#endif /* HASH_H */
