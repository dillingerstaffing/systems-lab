#ifndef MULLO_SPLIT_H
#define MULLO_SPLIT_H

#include <stdint.h>

/*
 * mullo64: the low 64 bits of the product a * b.
 *
 * Write a = ah * 2^32 + al and b = bh * 2^32 + bl, each half below 2^32.
 * Then
 *     a * b = ah*bh * 2^64 + (al*bh + ah*bl) * 2^32 + al*bl.
 *
 * The ah*bh * 2^64 term contributes nothing below bit 64, so the low
 * word is
 *     (al*bl + ((al*bh + ah*bl) << 32)) mod 2^64.
 *
 * This is exact in 64-bit unsigned arithmetic, which wraps modulo
 * 2^64 by the C standard, so every intermediate sum may be formed in
 * a plain uint64_t:
 *   - al*bl, al*bh, ah*bl are each below 2^64 (each factor is below
 *     2^32), hence exact;
 *   - (al*bh + ah*bl) can reach 2^65, but only its value modulo 2^64
 *     matters, because the following shift left by 32 discards the
 *     top 32 bits anyway: ((x + 2^64*w) << 32) is congruent to
 *     (x << 32) modulo 2^64 for any w;
 *   - adding al*bl to the shifted cross term wraps modulo 2^64,
 *     which is exactly the low word of a*b.
 *
 * No 128-bit type, no intrinsics, no builtins: only 64-bit unsigned
 * shifts, adds, and multiplies, all well-defined.  The three partial
 * products and the split shift/add sequence survive -O2 codegen
 * (verified by disassembly, see PROOF.md).
 */
static inline uint64_t mullo64(uint64_t a, uint64_t b)
{
    uint64_t al = (uint32_t)a;  /* low 32 bits of a */
    uint64_t ah = a >> 32;      /* high 32 bits of a */
    uint64_t bl = (uint32_t)b;
    uint64_t bh = b >> 32;

    uint64_t p0 = al * bl;      /* each partial product < 2^64 */
    uint64_t p1 = al * bh;
    uint64_t p2 = ah * bl;
    /* ah*bh is dropped: it contributes nothing below bit 64. */

    uint64_t cross = p1 + p2;   /* wraps mod 2^64; harmless, see above */
    return p0 + (cross << 32);
}

#endif /* MULLO_SPLIT_H */
