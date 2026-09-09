/*
 * Software IEEE-754 binary32 addition and multiplication, decoded straight
 * from the bit layout: 1 sign bit, 8 exponent bits (bias 127), 23 fraction
 * bits. Subnormals, infinities, and NaNs are handled explicitly.
 *
 * Method: the exact sum (or product) is formed as an integer significand in
 * unsigned __int128, wide enough to hold every bit with nothing dropped.
 * That exact value is then rounded to binary32 in a single
 * round-to-nearest-even step. One rounding step means the subnormal range
 * cannot suffer double rounding. NaN payloads propagate from the first NaN
 * operand with the quiet bit set, matching the hardware observed here.
 *
 * Only integer operations are used here. The test harness compares results
 * against the hardware FPU; this file never touches a float.
 */
#include "f32.h"

#define F32_SIGN_MASK  0x80000000u
#define F32_EXP_MASK   0x7F800000u
#define F32_FRAC_MASK  0x007FFFFFu
#define F32_QNAN_BIT   0x00400000u
#define F32_IMPLICIT   0x00800000u

static int f32_is_nan(uint32_t u)
{
    return (u & F32_EXP_MASK) == F32_EXP_MASK && (u & F32_FRAC_MASK) != 0;
}

static int f32_is_inf(uint32_t u)
{
    return (u & ~F32_SIGN_MASK) == F32_EXP_MASK;
}

static int f32_is_zero(uint32_t u)
{
    return (u & ~F32_SIGN_MASK) == 0;
}

/* Set the quiet bit, keeping sign and payload. */
static uint32_t f32_quiet_nan(uint32_t u)
{
    return u | F32_QNAN_BIT;
}

/* Result for invalid operations (inf + -inf, inf * 0); matches x86 SSE. */
static uint32_t f32_invalid(void)
{
    return 0xFFC00000u;
}

struct f32_parts {
    uint32_t sign;  /* 0 or 1 */
    int exp;        /* unbiased exponent; -126 for subnormals */
    uint32_t mant;  /* 24-bit significand with implicit 1, or raw fraction */
};

static struct f32_parts f32_unpack(uint32_t u)
{
    struct f32_parts p;
    uint32_t e = (u >> 23) & 0xFFu;
    uint32_t f = u & F32_FRAC_MASK;

    p.sign = (u >> 31) & 1u;
    if (e == 0) {
        p.exp = -126;
        p.mant = f;
    } else {
        p.exp = (int)e - 127;
        p.mant = f | F32_IMPLICIT;
    }
    return p;
}

/* Bit length of a nonzero unsigned __int128. */
static int u128_bitlen(unsigned __int128 x)
{
    uint64_t hi = (uint64_t)(x >> 64);

    if (hi != 0)
        return 64 + 64 - __builtin_clzll(hi);
    return 64 - __builtin_clzll((uint64_t)x);
}

/* Bit i of x; 0 for out-of-range i. */
static unsigned u128_bit(unsigned __int128 x, int i)
{
    if (i < 0 || i >= 128)
        return 0;
    return (unsigned)((x >> i) & 1);
}

/* Whether any of bits [0, i) of x is set. */
static int u128_any_below(unsigned __int128 x, int i)
{
    unsigned __int128 mask;

    if (i <= 0)
        return 0;
    if (i >= 128)
        return x != 0;
    mask = (((unsigned __int128)1 << i) - 1);
    return (x & mask) != 0;
}

/*
 * Round the exact value T * 2^e_T (T > 0) to binary32 with a single
 * round-to-nearest-even step, then pack the bits. One step keeps the
 * subnormal range free of double rounding.
 */
static uint32_t f32_round_pack(uint32_t sr, unsigned __int128 T, int e_T)
{
    int p = u128_bitlen(T) - 1; /* T in [2^p, 2^(p+1)) */
    int e = e_T + p;            /* unbiased exponent if the result is normal */

    if (e > 127)
        return (sr << 31) | F32_EXP_MASK;

    if (e >= -126) {
        /* Normal range: keep the top 24 bits, round the rest once. */
        uint32_t q;
        unsigned g = 0, r = 0;
        int s = 0;

        if (p >= 23) {
            int drop = p - 23;

            q = (uint32_t)(T >> drop);
            g = u128_bit(T, drop - 1);
            r = u128_bit(T, drop - 2);
            s = u128_any_below(T, drop - 2);
        } else {
            q = (uint32_t)(T << (23 - p));
        }
        if (g && (r || s || (q & 1u))) {
            q += 1;
            if (q == 0x01000000u) {
                q >>= 1;
                e += 1;
                if (e > 127)
                    return (sr << 31) | F32_EXP_MASK;
            }
        }
        return (sr << 31) | ((uint32_t)(e + 127) << 23) | (q & F32_FRAC_MASK);
    }

    /* Subnormal range (or zero): round once to multiples of 2^-149. */
    {
        int shift = -e_T - 149; /* exact multiple is T * 2^-shift */
        uint32_t m;
        unsigned g, r;
        int s;

        if (shift <= 0) {
            /*
             * e_T >= -149 here, so the exact multiple is an integer below
             * 2^23: no rounding can occur (this is the add path, where both
             * inputs are already multiples of 2^-149).
             */
            m = (uint32_t)(T << -shift);
            return (sr << 31) | m;
        }
        m = (shift >= 128) ? 0 : (uint32_t)(T >> shift);
        g = u128_bit(T, shift - 1);
        r = u128_bit(T, shift - 2);
        s = u128_any_below(T, shift - 2);
        if (g && (r || s || (m & 1u))) {
            m += 1;
            if (m == F32_IMPLICIT)
                return (sr << 31) | (1u << 23); /* rounded up to smallest normal */
        }
        return (sr << 31) | m;
    }
}

uint32_t f32_add(uint32_t a, uint32_t b)
{
    uint32_t ua = a, ub = b, sr;
    struct f32_parts pa, pb, t;
    unsigned __int128 T;
    int d;

    /* NaN propagation: the first NaN operand wins, quieted. */
    if (f32_is_nan(a))
        return f32_quiet_nan(a);
    if (f32_is_nan(b))
        return f32_quiet_nan(b);

    /* Infinities. */
    {
        int a_inf = f32_is_inf(a), b_inf = f32_is_inf(b);

        if (a_inf && b_inf)
            return (((a ^ b) >> 31) & 1u) ? f32_invalid() : a;
        if (a_inf)
            return a;
        if (b_inf)
            return b;
    }

    /* Zeros: +0 + -0 is +0 under round-to-nearest; -0 + -0 is -0. */
    {
        int a_zero = f32_is_zero(a), b_zero = f32_is_zero(b);

        if (a_zero && b_zero)
            return (a == b) ? a : 0x00000000u;
        if (a_zero)
            return b;
        if (b_zero)
            return a;
    }

    pa = f32_unpack(a);
    pb = f32_unpack(b);

    /* Larger magnitude first, keeping its bits and sign. */
    if (pb.exp > pa.exp || (pb.exp == pa.exp && pb.mant > pa.mant)) {
        uint32_t tu = ua;

        ua = ub;
        ub = tu;
        t = pa;
        pa = pb;
        pb = t;
    }
    sr = pa.sign;
    d = pa.exp - pb.exp; /* >= 0 */

    /*
     * With the exponents more than 100 apart, the smaller operand is below
     * 2^-77 ulp of the larger, so the sum rounds to the larger operand.
     * (pa is normal here: pb.exp >= -126 and d > 100 give pa.exp > -26.)
     */
    if (d > 100)
        return ua;

    /*
     * Exact magnitude significand; value = T * 2^(pb.exp - 23).
     * pa.mant << d needs at most 24 + 100 = 124 bits.
     */
    T = (unsigned __int128)pa.mant << d;
    if (pa.sign == pb.sign)
        T += pb.mant;
    else
        T -= pb.mant; /* >= 0: |pa| >= |pb| */
    if (T == 0)
        return 0x00000000u; /* equal magnitudes, opposite signs: +0 */

    return f32_round_pack(sr, T, pb.exp - 23);
}

uint32_t f32_mul(uint32_t a, uint32_t b)
{
    struct f32_parts pa, pb;
    unsigned __int128 P;
    uint32_t sr;

    /* NaN propagation: the first NaN operand wins, quieted. */
    if (f32_is_nan(a))
        return f32_quiet_nan(a);
    if (f32_is_nan(b))
        return f32_quiet_nan(b);

    sr = ((a ^ b) >> 31) & 1u;
    {
        int a_inf = f32_is_inf(a), b_inf = f32_is_inf(b);
        int a_zero = f32_is_zero(a), b_zero = f32_is_zero(b);

        if (a_inf || b_inf) {
            if (a_zero || b_zero)
                return f32_invalid(); /* inf * 0 */
            return (sr << 31) | F32_EXP_MASK;
        }
        if (a_zero || b_zero)
            return sr << 31;
    }

    pa = f32_unpack(a);
    pb = f32_unpack(b);

    /* Exact product in 48 bits; value = P * 2^(pa.exp + pb.exp - 46). */
    P = (unsigned __int128)pa.mant * (unsigned __int128)pb.mant;
    return f32_round_pack(sr, P, pa.exp + pb.exp - 46);
}
