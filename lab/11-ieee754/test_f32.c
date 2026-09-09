/*
 * Differential test: f32_add / f32_mul (integer-only, in f32.c) against the
 * hardware FPU over directed edge cases plus 1M random operand pairs.
 *
 * Comparison rule: bit-exact, except when either result is NaN. NaN
 * payloads are implementation-defined, so for NaN results we compare
 * NaN-ness and the sign bit only. Payload-only differences are counted
 * separately and reported honestly.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fenv.h>

#include "f32.h"

#define N_RANDOM 1000000L
#define MAX_REPORT 8

static uint64_t rng_state = 0x123456789ABCDEFULL;

static uint64_t xs64(void)
{
    uint64_t x = rng_state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}

static uint32_t r32(void)
{
    return (uint32_t)(xs64() >> 32);
}

/* Biased operand generator: uniform patterns plus focused edge classes. */
static uint32_t gen_operand(void)
{
    uint32_t c = r32();

    switch (c % 20) {
    case 0: { /* subnormal, nonzero */
        uint32_t sign = r32() & 0x80000000u;
        uint32_t frac = 1u + (r32() % 0x007FFFFFu);
        return sign | frac;
    }
    case 1: /* signed zero */
        return r32() & 0x80000000u;
    case 2: /* infinity */
        return (r32() & 0x80000000u) | 0x7F800000u;
    case 3: { /* NaN: random sign, quiet or signaling, random payload */
        uint32_t sign = r32() & 0x80000000u;
        uint32_t payload = r32() & 0x007FFFFFu;

        if (payload == 0)
            payload = 1;
        if ((r32() & 1u) == 0)
            payload &= ~0x00400000u; /* clear quiet bit: SNaN */
        return sign | 0x7F800000u | payload;
    }
    case 4: { /* tiny normal exponent (exp field 1..8) */
        uint32_t sign = r32() & 0x80000000u;
        uint32_t exp = 1u + (r32() % 8u);
        return sign | (exp << 23) | (r32() & 0x007FFFFFu);
    }
    case 5: { /* huge normal exponent (exp field 247..254) */
        uint32_t sign = r32() & 0x80000000u;
        uint32_t exp = 247u + (r32() % 8u);
        return sign | (exp << 23) | (r32() & 0x007FFFFFu);
    }
    default: /* uniform over all 2^32 patterns */
        return r32();
    }
}

static uint32_t hw_add(uint32_t a, uint32_t b)
{
    float fa, fb, fr;
    uint32_t r;

    memcpy(&fa, &a, 4);
    memcpy(&fb, &b, 4);
    fr = fa + fb;
    memcpy(&r, &fr, 4);
    return r;
}

static uint32_t hw_mul(uint32_t a, uint32_t b)
{
    float fa, fb, fr;
    uint32_t r;

    memcpy(&fa, &a, 4);
    memcpy(&fb, &b, 4);
    fr = fa * fb;
    memcpy(&r, &fr, 4);
    return r;
}

static int is_nan_u32(uint32_t u)
{
    return (u & 0x7F800000u) == 0x7F800000u && (u & 0x007FFFFFu) != 0;
}

/*
 * 0 = mismatch. NaN rule: both NaN and same sign bit counts as a match;
 * payload-only differences are tallied separately by the caller.
 */
static int equal_rule(uint32_t x, uint32_t y, int *nan_payload_only)
{
    int xn = is_nan_u32(x), yn = is_nan_u32(y);

    *nan_payload_only = 0;
    if (xn || yn) {
        if (!xn || !yn)
            return 0;
        if (((x ^ y) & 0x80000000u) != 0)
            return 0;
        if (x != y)
            *nan_payload_only = 1;
        return 1;
    }
    return x == y;
}

static void print_f32(const char *label, uint32_t u)
{
    float f;

    memcpy(&f, &u, 4);
    printf("  %s: bits=0x%08X value=%g\n", label, u, (double)f);
}

static long reported = 0;

static void report_mismatch(const char *op, uint32_t a, uint32_t b,
                            uint32_t sw, uint32_t hw)
{
    if (reported >= MAX_REPORT)
        return;
    reported++;
    printf("MISMATCH %s #%ld:\n", op, reported);
    print_f32("a ", a);
    print_f32("b ", b);
    print_f32("sw", sw);
    print_f32("hw", hw);
}

/* Directed edge pairs: {a, b}. */
static const uint32_t directed[][2] = {
    {0x7F800000u, 0xFF800000u}, /* inf + -inf -> NaN */
    {0xFF800000u, 0x7F800000u}, /* -inf + inf -> NaN */
    {0x7F800000u, 0x7F800000u}, /* inf + inf */
    {0xFF800000u, 0xFF800000u}, /* -inf + -inf */
    {0x7F800000u, 0x00000000u}, /* inf * +0 -> NaN */
    {0x7F800000u, 0x80000000u}, /* inf * -0 -> NaN */
    {0x00000000u, 0xFF800000u}, /* +0 * -inf -> NaN */
    {0x7F800000u, 0xFF800000u}, /* inf * -inf -> -inf */
    {0x7F7FFFFFu, 0x7F7FFFFFu}, /* FLT_MAX + FLT_MAX -> inf */
    {0x7F7FFFFFu, 0x7F7FFFFFu}, /* FLT_MAX * FLT_MAX -> inf */
    {0x7F7FFFFFu, 0x3F800000u}, /* FLT_MAX * 2 -> inf */
    {0x3F800000u, 0x33800000u}, /* 1 + 2^-24 -> 1 (ties to even) */
    {0x3F800001u, 0x33800000u}, /* (1+2^-23) + 2^-24 -> 1+2^-22 */
    {0x3F800001u, 0x33000000u}, /* (1+2^-23) + 2^-25 -> 1+2^-23 */
    {0xBF800001u, 0x33800000u}, /* -(1+2^-23) + 2^-24 */
    {0x3F800001u, 0x3F800001u}, /* (1+2^-23)^2 -> 1+2^-22 */
    {0x00800000u, 0x00800000u}, /* min normal + min normal */
    {0x00000001u, 0x00000001u}, /* min subnormal + min subnormal */
    {0x007FFFFFu, 0x007FFFFFu}, /* max subnormal + max subnormal */
    {0x00000001u, 0x3F800000u}, /* min subnormal * 1 */
    {0x00000001u, 0x00000001u}, /* min subnormal * min subnormal -> 0 */
    {0x00800000u, 0x80000000u}, /* min normal * -0 -> -0 */
    {0x3F800000u, 0xBF800000u}, /* 1 + -1 -> +0 */
    {0x80000000u, 0x80000000u}, /* -0 + -0 -> -0 */
    {0x80000000u, 0x00000000u}, /* -0 + +0 -> +0 */
    {0x80000000u, 0x3F800000u}, /* -0 * 1 -> -0 */
    {0x7F800001u, 0x3F800000u}, /* SNaN + 1 -> quieted NaN */
    {0xFF800001u, 0x3F800000u}, /* -SNaN + 1 */
    {0x7FC00001u, 0x7F800001u}, /* QNaN + SNaN */
    {0x7F800001u, 0x7FC00001u}, /* SNaN + QNaN */
    {0x7F800001u, 0x7F800001u}, /* SNaN * SNaN */
    {0x4B800000u, 0xCB800000u}, /* 2^24 + -2^24 -> +0 */
    {0x33800000u, 0x33800000u}, /* 2^-24 + 2^-24 */
    {0x00800001u, 0x80800000u}, /* near-cancellation to subnormal */
    {0x3F7FFFFFu, 0x33800000u}, /* (1-2^-24) + 2^-24 -> 1 */
    {0x7F7FFFFFu, 0x00800000u}, /* FLT_MAX + min normal -> FLT_MAX */
    {0x00000002u, 0x7F800000u}, /* subnormal * inf -> inf */
    {0x34000000u, 0x34000000u}, /* 2^-24 * 2^-24 -> subnormal */
};

static int is_subnormal_u32(uint32_t u)
{
    return (u & 0x7F800000u) == 0 && (u & 0x007FFFFFu) != 0;
}

static int is_inf_u32(uint32_t u)
{
    return (u & 0x7FFFFFFFu) == 0x7F800000u;
}

int main(void)
{
    long i, n_directed, add_mm = 0, mul_mm = 0;
    long nan_payload_only_add = 0, nan_payload_only_mul = 0;
    long nan_cases = 0, subnormal_cases = 0, inf_cases = 0;

    /* The hardware oracle is only valid under round-to-nearest with
       subnormals preserved (no FTZ/DAZ). Refuse to run otherwise. */
    if (fegetround() != FE_TONEAREST) {
        printf("ABORT: rounding mode is not FE_TONEAREST\n");
        return 2;
    }
    {
        volatile float tiny = 1e-40f; /* subnormal */

        if (tiny * 1.0f == 0.0f) {
            printf("ABORT: host flushes subnormals to zero\n");
            return 2;
        }
    }
    printf("self-check: FE_TONEAREST, subnormals preserved by hardware\n");

    n_directed = (long)(sizeof(directed) / sizeof(directed[0]));
    for (i = 0; i < n_directed; i++) {
        uint32_t a = directed[i][0], b = directed[i][1];
        uint32_t sa = f32_add(a, b), ha = hw_add(a, b);
        uint32_t sm = f32_mul(a, b), hm = hw_mul(a, b);
        int npo;

        if (!equal_rule(sa, ha, &npo)) {
            report_mismatch("add", a, b, sa, ha);
            add_mm++;
        } else {
            nan_payload_only_add += npo;
        }
        if (!equal_rule(sm, hm, &npo)) {
            report_mismatch("mul", a, b, sm, hm);
            mul_mm++;
        } else {
            nan_payload_only_mul += npo;
        }
    }
    printf("directed: %ld pairs x add/mul, add mismatches %ld, mul mismatches %ld\n",
           n_directed, add_mm, mul_mm);

    for (i = 0; i < N_RANDOM; i++) {
        uint32_t a = gen_operand(), b = gen_operand();
        uint32_t sa = f32_add(a, b), ha = hw_add(a, b);
        uint32_t sm = f32_mul(a, b), hm = hw_mul(a, b);
        int npo;

        if (is_nan_u32(a) || is_nan_u32(b))
            nan_cases++;
        if (is_subnormal_u32(a) || is_subnormal_u32(b))
            subnormal_cases++;
        if (is_inf_u32(a) || is_inf_u32(b))
            inf_cases++;

        if (!equal_rule(sa, ha, &npo)) {
            report_mismatch("add", a, b, sa, ha);
            add_mm++;
        } else {
            nan_payload_only_add += npo;
        }
        if (!equal_rule(sm, hm, &npo)) {
            report_mismatch("mul", a, b, sm, hm);
            mul_mm++;
        } else {
            nan_payload_only_mul += npo;
        }
    }

    printf("random: %ld pairs x add/mul\n", N_RANDOM);
    printf("  add mismatches: %ld\n", add_mm);
    printf("  mul mismatches: %ld\n", mul_mm);
    printf("  NaN operand cases: %ld\n", nan_cases);
    printf("  subnormal operand cases: %ld\n", subnormal_cases);
    printf("  inf operand cases: %ld\n", inf_cases);
    printf("  NaN payload-only differences (sign matched): add %ld, mul %ld\n",
           nan_payload_only_add, nan_payload_only_mul);

    if (add_mm == 0 && mul_mm == 0) {
        printf("ALL CHECKS PASSED\n");
        return 0;
    }
    printf("FAILURES PRESENT\n");
    return 1;
}
