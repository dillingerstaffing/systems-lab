/*
 * saturate_u16.h - branchless clamp of a 32-bit signed value into
 * [0, 65535], using only the sign-mask identities.
 *
 * saturate_u16(x): returns 0 for x < 0, 65535 for x > 65535, and x
 * otherwise, with no conditional jump anywhere in the path.
 *
 * The mechanism is one thing: the sign-mask select.  Wherever a
 * choice "pick A when predicate p holds, B otherwise" is needed,
 * the code builds mask = 0xFFFFFFFF exactly when p holds
 * (0x00000000 otherwise) out of bit 31 of an unsigned 32-bit
 * value, then computes a ^ ((a ^ b) & mask).  All arithmetic is on
 * unsigned 32-bit values and every shift count is a compile-time
 * constant below 32, so every operation is fully defined C11; no
 * implementation-defined signed right shift is used anywhere,
 * the sign predicate is read as bit 31 of the unsigned
 * representation instead.
 *
 * Step 1, clamp below at 0.  Let ux = (uint32_t)x.
 * neg = 0u - (ux >> 31) is 0xFFFFFFFF exactly when bit 31 of ux
 * is set (x < 0 in two's complement) and 0 otherwise, so
 * t = ux & ~neg keeps ux when x >= 0 and clears it to 0 when
 * x < 0: t = max(x, 0), and t lies in [0, 2^31 - 1].
 *
 * Step 2, clamp above at 65535 from the sign of (t - 65536).
 * Because t <= 2^31 - 1, the unsigned difference d = t - 65536
 * wraps exactly when t < 65536 and never wraps upward: for
 * t >= 65536, d = t - 65536 lies in [0, 2^31 - 1 - 65536] with
 * bit 31 clear; for t < 65536, d = 2^32 - (65536 - t) lies in
 * [2^32 - 65536, 2^32 - 1] with bit 31 set.  So
 * ge = 1u - (d >> 31) is 1 exactly when t >= 65536, and
 * mask = 0u - ge is 0xFFFFFFFF exactly in that case.
 * r = t ^ ((t ^ 65535u) & mask): with mask all ones,
 * r = t ^ (t ^ 65535) = 65535; with mask zero, r = t.  So
 * r = min(t, 65535) = min(max(x, 0), 65535).
 *
 * The result always lies in [0, 65535], so the narrowing cast to
 * uint16_t is exact.
 *
 * No builtins, no intrinsics, no inline asm, no library calls,
 * no ternary, no if/else, no min/max calls: plain C11 unsigned
 * arithmetic, shifts, and bitwise ops.
 */
#ifndef SATURATE_U16_H
#define SATURATE_U16_H

#include <stdint.h>

static inline uint16_t saturate_u16(int32_t x)
{
    uint32_t ux = (uint32_t)x;

    /* Step 1: clamp below at 0.  neg is 0xFFFFFFFF exactly when
     * bit 31 of ux is set (x < 0), 0 otherwise; t = ux & ~neg is
     * 0 for x < 0 and ux for x >= 0. */
    uint32_t neg = 0u - (ux >> 31);
    uint32_t t = ux & ~neg;

    /* Step 2: clamp above at 65535.  (t - 65536u) >> 31 is 1
     * exactly when t < 65536 (the subtraction wraps downward;
     * it cannot wrap upward because t <= 2^31 - 1), so ge is 1
     * exactly when t >= 65536 and mask selects 65535 in that
     * case, t otherwise. */
    uint32_t ge = 1u - ((t - 65536u) >> 31);
    uint32_t mask = 0u - ge;
    uint32_t r = t ^ ((t ^ 65535u) & mask);

    return (uint16_t)r;
}

#endif /* SATURATE_U16_H */
