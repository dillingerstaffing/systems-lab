#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "pte_step.h"
#include "oracle_step.h"

/*
 * Differential test: pte_step (mask-based) vs oracle_step (divmod
 * bit-array, no shared code or constants) over:
 *
 * 1. An exhaustive cross product (2,359,296 PTEs):
 *      level in {0,1,2}                      3
 *      V                                     2
 *      R, W, X (all 8 encodings)             8
 *      D, A, U (all 8 combos)                8
 *      PPN[5:0] (bits 15-10, small width)    64
 *      reserved bits 63-54: clear, 10 singles, all set   12
 *      G, RSW (the ignored set, all 8)       8
 *    For every base (all bits but G/RSW fixed) the 8 ignored-bit
 *    patterns must yield identical verdicts (invariance property).
 * 2. 10,000,000 fixed-seed splitmix64 random 64-bit PTEs (seed
 *    0x9E3779B97F4A7C15, the splitmix64 reference constant), each with
 *    a PRNG-derived level in {0,1,2}.
 *
 * Every check compares the verdict and, on DESCEND, next_ppn, and on
 * LEAF, leaf_phys (the PPN decode sanity check). An FNV-1a checksum over
 * all verdict bytes must be identical across -O2, -O0, ASan+UBSan builds.
 */

/* Fixed seed: the splitmix64 reference constant. Documented, not random. */
static uint64_t rng_state = 0x9E3779B97F4A7C15ULL;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv = 14695981039346656037ULL;

static void fnv_mix(unsigned byte)
{
    fnv ^= (uint64_t)byte;
    fnv *= 1099511628211ULL;
}

static uint64_t checks;
static uint64_t mismatches;
static uint64_t leaf_checks;
static uint64_t descend_checks;
static uint64_t inv_bases;
static uint64_t inv_violations;
static unsigned shown;

/* The ignored set: G (bit 5), RSW (bits 9-8). Pattern k in 0..7. */
static uint64_t ignored_bits(unsigned k)
{
    return ((uint64_t)(k & 1u) << 5) | ((uint64_t)((k >> 1) & 3u) << 8);
}

static void check_one(uint64_t pte, int level, int *vi_out, int *vo_out)
{
    pte_step_result_t ri = pte_step(pte, level);
    oracle_result_t ro = oracle_step(pte, level);
    int ok = 1;

    if ((int)ri.verdict != (int)ro.verdict) {
        ok = 0;
    } else if (ri.verdict == PTE_STEP_DESCEND &&
               ri.next_ppn != ro.next_ppn) {
        ok = 0;
    } else if (ri.verdict == PTE_STEP_LEAF &&
               ri.leaf_phys != ro.leaf_phys) {
        ok = 0;
    }
    if (ri.verdict == PTE_STEP_LEAF)
        leaf_checks++;
    if (ri.verdict == PTE_STEP_DESCEND)
        descend_checks++;
    if (!ok) {
        mismatches++;
        if (shown < 8) {
            shown++;
            printf("MISMATCH pte=%016" PRIx64 " level=%d "
                   "impl=%d/%016" PRIx64 "/%016" PRIx64 " "
                   "oracle=%d/%016" PRIx64 "/%016" PRIx64 "\n",
                   pte, level,
                   (int)ri.verdict, ri.next_ppn, ri.leaf_phys,
                   (int)ro.verdict, ro.next_ppn, ro.leaf_phys);
        }
    }
    fnv_mix((unsigned)ri.verdict);
    checks++;
    *vi_out = (int)ri.verdict;
    *vo_out = (int)ro.verdict;
}

int main(void)
{
    uint64_t rsvd[12];
    unsigned r;
    int level, v, rwx, dau;
    uint64_t ppnlow, k;
    uint64_t i;

    /* Reserved-bit patterns for bits 63-54: clear, each single bit, all. */
    rsvd[0] = 0;
    for (r = 0; r < 10; r++)
        rsvd[1 + r] = 1ULL << (54 + r);
    rsvd[11] = 0xFFC0000000000000ULL;

    /* Exhaustive matrix, grouped by base for the invariance property. */
    for (level = 0; level <= 2; level++) {
        for (v = 0; v <= 1; v++) {
            for (rwx = 0; rwx < 8; rwx++) {
                for (dau = 0; dau < 8; dau++) {
                    for (ppnlow = 0; ppnlow < 64; ppnlow++) {
                        for (r = 0; r < 12; r++) {
                            uint64_t base = 0;
                            int vi[8], vo[8];

                            if (v)
                                base |= 1ULL << 0;
                            base |= (uint64_t)(rwx & 1) << 1;
                            base |= (uint64_t)((rwx >> 1) & 1) << 2;
                            base |= (uint64_t)((rwx >> 2) & 1) << 3;
                            base |= (uint64_t)(dau & 1) << 4;        /* U */
                            base |= (uint64_t)((dau >> 1) & 1) << 6; /* A */
                            base |= (uint64_t)((dau >> 2) & 1) << 7; /* D */
                            base |= ppnlow << 10;
                            base |= rsvd[r];

                            for (k = 0; k < 8; k++)
                                check_one(base | ignored_bits((unsigned)k),
                                          level, &vi[k], &vo[k]);

                            inv_bases++;
                            for (k = 1; k < 8; k++) {
                                if (vi[k] != vi[0] || vo[k] != vo[0])
                                    inv_violations++;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Fixed-seed random sweep over the full 64-bit PTE space. */
    for (i = 0; i < 10000000ULL; i++) {
        uint64_t pte = splitmix64();
        int lv = (int)(splitmix64() % 3u);
        int vi, vo;

        check_one(pte, lv, &vi, &vo);
    }

    printf("checks=%" PRIu64 " mismatches=%" PRIu64
           " checksum=%016" PRIx64 "\n",
           checks, mismatches, fnv);
    printf("exhaustive=2359296 random=10000000\n");
    printf("leaf_phys_cross_checks=%" PRIu64 "\n", leaf_checks);
    printf("descend_ppn_cross_checks=%" PRIu64 "\n", descend_checks);
    printf("ignored_invariance_bases=%" PRIu64 " violations=%" PRIu64 "\n",
           inv_bases, inv_violations);
    return (mismatches == 0 && inv_violations == 0) ? 0 : 1;
}
