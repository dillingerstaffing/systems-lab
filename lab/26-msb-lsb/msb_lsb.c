#include "msb_lsb.h"

/*
 * Two bit-level facts carry both functions:
 *
 *   1. For nonzero unsigned x, `x & -x` (with -x = ~x + 1 in two's
 *      complement) clears every bit except the lowest set one, so the
 *      result is exactly 2^k where k is the index of that bit.
 *      Proof sketch: -x flips every bit below the lowest 1 of x and
 *      keeps the lowest 1, so the AND with x keeps only that bit.
 *
 *   2. The shift/OR smear `x |= x>>1; x |= x>>2; ... x |= x>>32`
 *      propagates the highest set bit downward until every bit at or
 *      below it is 1, so the result is exactly 2^(k+1) - 1 where k is
 *      the index of the highest set bit (the all-ones prefix 2^64 - 1
 *      when k = 63). Six doublings cover 64 bits: after the 2^i shift,
 *      the run of 1s extends 2^(i+1) bits downward.
 *
 * Each of the 64 possible isolated bits (fact 1) and each of the 64
 * possible smeared prefixes (fact 2) hashes to a distinct 6-bit value
 * when multiplied by 0x03f79d71b4cb0a89 and shifted right by 58. That
 * is a property of this particular constant; the test binary asserts
 * all 64 hashes are distinct for both tables, so the tables are
 * checked, not trusted. Each table inverts its hash back to the bit
 * index k, which is then converted to the documented return value.
 *
 * All arithmetic is unsigned and every shift amount is in range, so
 * there is no undefined behavior. Neither function calls any bit-scan
 * builtin or library routine.
 */
#define DEBRUIJN64 UINT64_C(0x03f79d71b4cb0a89)

static unsigned char ffs_tab[64];
static unsigned char fls_tab[64];
static int tabs_ready = 0;

static void build_tables(void)
{
    unsigned k;

    for (k = 0u; k < 64u; k++) {
        uint64_t lowbit = UINT64_C(1) << k;
        uint64_t prefix = ~UINT64_C(0) >> (63u - k); /* 2^(k+1)-1, bits k..0 */
        uint64_t h1 = (lowbit * DEBRUIJN64) >> 58;
        uint64_t h2 = (prefix * DEBRUIJN64) >> 58;

        ffs_tab[h1] = (unsigned char)k;
        fls_tab[h2] = (unsigned char)k;
    }
    tabs_ready = 1;
}

int ffs64(uint64_t x)
{
    uint64_t lowbit;

    if (!tabs_ready)
        build_tables();
    if (x == 0u)
        return 0;
    lowbit = x & (0u - x);
    return (int)ffs_tab[(lowbit * DEBRUIJN64) >> 58] + 1;
}

int fls64(uint64_t x)
{
    uint64_t s;

    if (!tabs_ready)
        build_tables();
    if (x == 0u)
        return -1;
    s = x;
    s |= s >> 1;
    s |= s >> 2;
    s |= s >> 4;
    s |= s >> 8;
    s |= s >> 16;
    s |= s >> 32;
    return (int)fls_tab[(s * DEBRUIJN64) >> 58];
}
