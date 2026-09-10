#ifndef FMIX64_H
#define FMIX64_H

#include <stdint.h>

/*
 * fmix64: the 64-bit finalization mix from MurmurHash3_x64_128.
 * The sequence of operations is published source (Austin Appleby,
 * MurmurHash3.cpp, public domain); this header reimplements it for
 * the test to compare against a separately compiled verbatim copy.
 */
#define FMIX64_C1 UINT64_C(0xff51afd7ed558ccd)
#define FMIX64_C2 UINT64_C(0xc4ceb9fe1a85ec53)

static inline uint64_t fmix64(uint64_t k)
{
    k ^= k >> 33;
    k *= FMIX64_C1;
    k ^= k >> 33;
    k *= FMIX64_C2;
    k ^= k >> 33;
    return k;
}

/*
 * The two multiply constants are odd, so each is a unit modulo 2^64:
 * an odd a has a multiplicative inverse because gcd(a, 2^64) = 1.
 * The inverses below were found by Newton-Raphson iteration
 *     inv <- inv * (2 - c * inv)   (mod 2^64),
 * starting from inv = 1, which is the inverse mod 2.  Each round
 * doubles the number of correct low bits (1 -> 2 -> 4 -> 8 -> 16 ->
 * 32 -> 64), so six rounds give the exact 64-bit inverse, verified
 * in the test by the check inv * c == 1 (mod 2^64).
 */
#define FMIX64_C1_INV UINT64_C(0x4f74430c22a54005)
#define FMIX64_C2_INV UINT64_C(0x9cb4b2f8129337db)

/*
 * Invert the shift-xor step y = x ^ (x >> s) for any s in 1..63.
 * The low s bits of y are the low s bits of x.  Given x correct in
 * its low i bits, folding x ^= x >> i fixes the next block:
 *   x[i .. 2i) after the fold = y[i..2i) ^ x_old[0..i)
 *                             = x_true[i..2i) ^ x_true[0..i) ^ x_true[0..i)
 *                             = x_true[i..2i).
 * Starting with x = y (correct in the low 33 bits for s = 33) and
 * doubling the fold width each round (33, then the loop exits at 66
 * since 66 >= 64), every bit is recovered exactly once.
 */
static inline uint64_t fmix64_xorshift33_inv(uint64_t y)
{
    uint64_t x = y;
    for (unsigned s = 33; s < 64; s *= 2)
        x ^= x >> s;
    return x;
}

/*
 * fmix64_inv: exact inverse of fmix64.  Each of the five steps is a
 * bijection on 64-bit words (shift-xor as shown above; multiplication
 * by an odd constant, inverted by its modular inverse), so applying
 * the step inverses in reverse order gives fmix64_inv(fmix64(x)) == x
 * for every x.  All arithmetic is unsigned 64-bit, hence fully
 * defined (modular).
 */
static inline uint64_t fmix64_inv(uint64_t h)
{
    h = fmix64_xorshift33_inv(h);
    h *= FMIX64_C2_INV;
    h = fmix64_xorshift33_inv(h);
    h *= FMIX64_C1_INV;
    h = fmix64_xorshift33_inv(h);
    return h;
}

#endif /* FMIX64_H */
