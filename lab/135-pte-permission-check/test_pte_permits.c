#define _POSIX_C_SOURCE 199309L /* clock_gettime */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "pte_permits.h"

/*
 * Independent reference for the differential test. It encodes the
 * same spec rules (section 4.3.1, Table 115; section 4.3.2 step 3;
 * SUM=0 and MXR=0 assumed) but is written as a separately structured
 * encoding: the reserved encodings are listed explicitly instead of
 * via the W-implies-R implication, the access grant is read from a
 * table indexed by access type instead of a switch, and the privilege
 * rule is computed as its own predicate instead of early returns. No
 * helper code is shared with pte_permits.c.
 */
static int oracle_permits(unsigned r, unsigned w, unsigned x, unsigned u,
                          unsigned access, unsigned mode) {
    /* The two forbidden (X,W,R) patterns, named directly. */
    int reserved = (r == 0 && w == 1 && x == 0) ||
                   (r == 0 && w == 1 && x == 1);

    /* Grant table: load reads R, store reads W, exec reads X. */
    int grant_table[3];
    grant_table[PTE_ACCESS_LOAD] = (int)(r & 1u);
    grant_table[PTE_ACCESS_STORE] = (int)(w & 1u);
    grant_table[PTE_ACCESS_EXEC] = (int)(x & 1u);
    int grant = grant_table[access];

    /* Privilege predicate. U=0 is not for U-mode; U=1 is not for
     * S-mode while SUM=0; S-mode exec on a U=1 page is illegal
     * irrespective of SUM (subsumed here by the SUM=0 rule). */
    int priv_ok = 1;
    if (u == 0 && mode == PTE_MODE_U)
        priv_ok = 0;
    if (u == 1 && mode == PTE_MODE_S)
        priv_ok = 0;
    if (access == (unsigned)PTE_ACCESS_EXEC && u == 1 &&
        mode == (unsigned)PTE_MODE_S)
        priv_ok = 0;

    return !reserved && grant && priv_ok;
}

/* FNV-1a 64 over one byte. */
static uint64_t fnv1a_step(uint64_t h, uint8_t b) {
    h ^= b;
    h *= 0x100000001B3ULL;
    return h;
}

static uint64_t checks = 0, mismatches = 0;
static uint64_t hash = 0xCBF29CE484222325ULL; /* FNV offset basis */

/*
 * One differential case: implementation and oracle must agree, and
 * the checksum folds the verdict byte so the run's outputs are
 * pinned, not just the mismatch count.
 */
static void check_case(unsigned r, unsigned w, unsigned x, unsigned u,
                       pte_access_t access, pte_mode_t mode) {
    int got = pte_permits(r, w, x, u, access, mode);
    int want = oracle_permits(r, w, x, u, (unsigned)access, (unsigned)mode);
    int ok = (got == want) && (got == 0 || got == 1);

    hash = fnv1a_step(hash, (uint8_t)got);

    checks++;
    if (!ok) {
        mismatches++;
        if (mismatches < 20)
            printf("MISMATCH r=%u w=%u x=%u u=%u access=%d mode=%d got=%d want=%d\n",
                   r, w, x, u, (int)access, (int)mode, got, want);
    }
}

int main(void) {
    unsigned r, w, x, u;
    pte_access_t a;
    pte_mode_t m;
    struct timespec t0, t1;
    double elapsed_ns;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* Exhaustive: all 16 R/W/X/U combinations x 3 access types x
     * 2 modes = 96 cases. */
    for (r = 0; r < 2; r++)
        for (w = 0; w < 2; w++)
            for (x = 0; x < 2; x++)
                for (u = 0; u < 2; u++)
                    for (a = PTE_ACCESS_LOAD; a <= PTE_ACCESS_EXEC; a++)
                        for (m = PTE_MODE_S; m <= PTE_MODE_U; m++)
                            check_case(r, w, x, u, a, m);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    elapsed_ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                 (double)(t1.tv_nsec - t0.tv_nsec);

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           (unsigned long long)checks, (unsigned long long)mismatches,
           (unsigned long long)hash);
    printf("elapsed=%.3f s (%.3f ns/check)\n",
           elapsed_ns / 1e9, elapsed_ns / (double)checks);

    /* Machine-readable header lines: the PROOF-HEADER block in
     * PROOF.md is stamped from these values at run time. */
    printf("header-checks=%llu\n", (unsigned long long)checks);
    printf("header-mismatches=%llu\n", (unsigned long long)mismatches);
    printf("header-checksum=%016llx\n", (unsigned long long)hash);

    return mismatches == 0 ? 0 : 1;
}
