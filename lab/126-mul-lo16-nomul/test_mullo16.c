/*
 * lab/126: differential test of mullo16 against the plain multiply oracle.
 *
 * The oracle is (uint16_t)((uint32_t)a * (uint32_t)b), i.e. the low 16 bits
 * of the true 32-bit product. The test walks the entire 16-bit pair space:
 * every a in 0..65535 crossed with every b in 0..65535, for
 * 65536 * 65536 = 4294967296 checks. This is also exactly the identity
 * property mullo16(a,b) == (uint16_t)((uint32_t)a * (uint32_t)b) under test.
 *
 * The multiply operator appears only in the oracle, never in mullo16.
 * All 2^32 outputs are folded into one FNV-1a 64-bit checksum; the checksum
 * must agree across the -O0, -O2, and ASan+UBSan builds.
 */
#include <stdint.h>
#include <stdio.h>

#include "mul16.h"

static uint64_t fnv1a64(uint64_t h, uint16_t v)
{
    h ^= (uint64_t)v;
    h *= 1099511628211ULL;
    return h;
}

int main(void)
{
    uint64_t checks = 0;
    uint64_t mismatches = 0;
    uint64_t hash = 14695981039346656037ULL; /* FNV-1a 64 offset basis */

    for (uint32_t b = 0; b < 65536u; b++) {
        for (uint32_t a = 0; a < 65536u; a++) {
            uint16_t got = mullo16((uint16_t)a, (uint16_t)b);
            uint16_t want = (uint16_t)((uint32_t)a * (uint32_t)b);
            checks++;
            if (got != want) {
                mismatches++;
                if (mismatches < 8)
                    printf("MISMATCH a=%u b=%u got=%u want=%u\n",
                           (unsigned)a, (unsigned)b,
                           (unsigned)got, (unsigned)want);
            }
            hash = fnv1a64(hash, got);
        }
    }

    printf("checks=%llu mismatches=%llu checksum=0x%016llx\n",
           (unsigned long long)checks,
           (unsigned long long)mismatches,
           (unsigned long long)hash);
    return mismatches == 0 ? 0 : 1;
}
