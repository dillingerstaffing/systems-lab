#ifndef BIT_DEPOSIT_H
#define BIT_DEPOSIT_H

#include <stdint.h>

/*
 * bit_extract(x, off, w): the w bits [off, off+w) of x, right-justified
 * (bit off of x becomes bit 0 of the result). Computed from the
 * shift/mask identities as (x >> off) & mask_w, where
 * mask_w = (1ULL << w) - 1 (w = 64 is the special case, yielding all
 * 64 bits set, since 1ULL << 64 is undefined).
 *
 * bit_insert(x, off, w, v): x with bits [off, off+w) replaced by the
 * low w bits of v. Computed as
 * (x & ~(mask_w << off)) | ((v & mask_w) << off).
 *
 * Domain: off + w <= 64 (so for w = 64, off must be 0). w = 0 is a
 * defined no-op: extract returns 0, insert returns x unchanged.
 */
uint64_t bit_extract(uint64_t x, unsigned off, unsigned w);
uint64_t bit_insert(uint64_t x, unsigned off, unsigned w, uint64_t v);

#endif
