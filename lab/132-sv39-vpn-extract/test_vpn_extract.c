#include <stdio.h>
#include <stdint.h>
#include "vpn_extract.h"

/* splitmix64, fixed seed. Plain generator code, written out by hand. */
static uint64_t sm_state;

static uint64_t splitmix64(void) {
    uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/*
 * Oracle: structurally different from the shift/mask implementation.
 * Builds each field by iterating bit indices and setting destination
 * bits one at a time; it never uses a multi-bit shift/mask of a
 * whole field.
 */
static vpn_fields_t oracle_fields(uint64_t va) {
    vpn_fields_t f;
    f.vpn0 = 0;
    f.vpn1 = 0;
    f.vpn2 = 0;
    f.page_offset = 0;
    int b, i;
    for (b = 0; b < 12; b++) {
        if ((va >> b) & 1ULL)
            f.page_offset = (uint16_t)(f.page_offset | (uint16_t)(1u << b));
    }
    for (i = 0; i < 3; i++) {
        for (b = 0; b < 9; b++) {
            if ((va >> (12 + 9 * i + b)) & 1ULL) {
                uint16_t bit = (uint16_t)(1u << b);
                if (i == 0)
                    f.vpn0 = (uint16_t)(f.vpn0 | bit);
                else if (i == 1)
                    f.vpn1 = (uint16_t)(f.vpn1 | bit);
                else
                    f.vpn2 = (uint16_t)(f.vpn2 | bit);
            }
        }
    }
    return f;
}

/* FNV-1a 64 over one byte. */
static uint64_t fnv1a_step(uint64_t h, uint8_t b) {
    h ^= b;
    h *= 0x100000001B3ULL;
    return h;
}

/* Fold one 16-bit field into the checksum, low byte then high byte. */
static uint64_t fnv1a_field(uint64_t h, uint16_t v) {
    h = fnv1a_step(h, (uint8_t)(v & 0xFFu));
    return fnv1a_step(h, (uint8_t)(v >> 8));
}

static uint64_t checks = 0, mismatches = 0;
static uint64_t hash = 0xCBF29CE484222325ULL; /* FNV offset basis */

/* One differential case: fields must match the oracle, and
 * recombine(extract(va)) must return exactly bits 38:0 of va. */
static void check_va(uint64_t va) {
    vpn_fields_t got = vpn_extract(va);
    vpn_fields_t want = oracle_fields(va);
    uint64_t want_bits = va & 0x7FFFFFFFFFULL; /* bits 38:0 */
    int ok = (got.vpn0 == want.vpn0) &&
             (got.vpn1 == want.vpn1) &&
             (got.vpn2 == want.vpn2) &&
             (got.page_offset == want.page_offset) &&
             (vpn_recombine(got) == want_bits) &&
             (vpn_recombine(want) == want_bits);
    hash = fnv1a_field(hash, got.vpn0);
    hash = fnv1a_field(hash, got.vpn1);
    hash = fnv1a_field(hash, got.vpn2);
    hash = fnv1a_field(hash, got.page_offset);
    checks++;
    if (!ok) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH va=%llx got=%x,%x,%x,%x want=%x,%x,%x,%x\n",
                   (unsigned long long)va,
                   got.vpn2, got.vpn1, got.vpn0, got.page_offset,
                   want.vpn2, want.vpn1, want.vpn0, want.page_offset);
    }
}

int main(void) {
    int k;
    uint64_t i;

    /* (a) Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..38. */
    for (k = 0; k < 39; k++) {
        uint64_t vals[3] = { 1ULL << k, (1ULL << k) - 1ULL,
                             (1ULL << k) + 1ULL };
        for (i = 0; i < 3; i++)
            check_va(vals[i]);
    }
    /* All-ones and all-zeros 39-bit values. */
    check_va(0x7FFFFFFFFFULL);
    check_va(0ULL);

    /* (b) Canonical-address row: the 2^k directed values with bit 38
     * sign-extended into bits 63:39, as a real Sv39 page walker sees
     * them. Extraction must be identical to the raw 39-bit form. */
    for (k = 0; k < 39; k++) {
        uint64_t raw = 1ULL << k;
        uint64_t canon = (raw & (1ULL << 38))
                         ? raw | 0xFFFFFF8000000000ULL
                         : raw;
        check_va(canon);
    }

    /* (c) Exhaustive 24-bit sweep: all va in [0, 2^24). */
    for (uint32_t va = 0; va < (1U << 24); va++)
        check_va((uint64_t)va);

    /* (d) 10M fixed-seed splitmix64 39-bit values. */
    sm_state = 0x123456789ABCDEF0ULL;
    for (i = 0; i < 10000000ULL; i++)
        check_va(splitmix64() & 0x7FFFFFFFFFULL);

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           (unsigned long long)checks, (unsigned long long)mismatches,
           (unsigned long long)hash);
    return mismatches == 0 ? 0 : 1;
}
