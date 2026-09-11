#define _POSIX_C_SOURCE 199309L /* clock_gettime */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "walk_permits.h"

/*
 * Independent reference for the differential test. It encodes the
 * same spec rules (section 4.3.2 steps 3 and 5; section 4.3.1 /
 * Table 115 for the leaf; SUM=0 and MXR=0 assumed) but is
 * structured differently from walk_permits.c: instead of
 * descending with early returns, it locates the terminating level
 * in a first scan, then checks the reserved encodings across the
 * consumed levels from an explicit two-pattern table, then reads
 * the grant from a table indexed by access type and computes the
 * privilege rule as a boolean predicate. No helper code is shared
 * with walk_permits.c.
 */
static int oracle_walk(const unsigned *r, const unsigned *w,
                       const unsigned *x, const unsigned *u,
                       int nlevels, unsigned access, unsigned mode) {
    int term = -1;
    int i;

    /* First scan: the first level with R or X set ends the walk
     * (spec section 4.3.2, step 5). */
    for (i = 0; i < nlevels; i++) {
        if ((r[i] & 1u) || (x[i] & 1u)) {
            term = i;
            break;
        }
    }
    if (term < 0)
        return 0; /* levels exhausted before any leaf: fault */

    /* Reserved r=0,w=1 encoding across the consumed levels, named
     * as the two explicit (r,w,x) patterns (spec step 3). */
    for (i = 0; i <= term; i++) {
        unsigned ri = r[i] & 1u, wi = w[i] & 1u, xi = x[i] & 1u;
        if (ri == 0 && wi == 1 && xi == 0)
            return 0;
        if (ri == 0 && wi == 1 && xi == 1)
            return 0;
    }

    {
        unsigned lr = r[term] & 1u;
        unsigned lw = w[term] & 1u;
        unsigned lx = x[term] & 1u;
        unsigned lu = u[term] & 1u;

        /* Grant table: load reads R, store reads W, exec reads X. */
        int grant[3];
        grant[WALK_ACCESS_LOAD] = (int)lr;
        grant[WALK_ACCESS_STORE] = (int)lw;
        grant[WALK_ACCESS_EXEC] = (int)lx;

        /* Privilege predicate. U=0 is not for U-mode; U=1 is not
         * for S-mode while SUM=0; S-mode exec on a U=1 page is
         * illegal irrespective of SUM (subsumed here by the
         * SUM=0 rule). */
        int priv_ok = 1;
        if (lu == 0 && mode == WALK_MODE_U)
            priv_ok = 0;
        if (lu == 1 && mode == WALK_MODE_S)
            priv_ok = 0;
        if (access == (unsigned)WALK_ACCESS_EXEC && lu == 1 &&
            mode == (unsigned)WALK_MODE_S)
            priv_ok = 0;

        return grant[access] && priv_ok;
    }
}

/* FNV-1a 64 over one byte. */
static uint64_t fnv1a_step(uint64_t h, uint8_t b) {
    h ^= b;
    h *= 0x100000001B3ULL;
    return h;
}

static uint64_t checks = 0, mismatches = 0;
static uint64_t hash = 0xCBF29CE484222325ULL; /* FNV offset basis */

/* xorshift32 for the fixed-seed random corpus. Seed stated in
 * PROOF.md so the corpus is reproducible. */
static uint32_t rng_state = 149u;
static uint32_t xorshift32(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

/*
 * One differential case: implementation and oracle must agree, and
 * the checksum folds the verdict byte so the run's outputs are
 * pinned, not just the mismatch count.
 */
static void check_case(const pte_bits_t *levels, int nlevels,
                       walk_access_t access, walk_mode_t mode) {
    unsigned r[3], w[3], x[3], u[3];
    int i;
    int got, want, ok;

    for (i = 0; i < nlevels; i++) {
        r[i] = levels[i].r;
        w[i] = levels[i].w;
        x[i] = levels[i].x;
        u[i] = levels[i].u;
    }

    got = walk_permits(levels, nlevels, access, mode);
    want = oracle_walk(r, w, x, u, nlevels, (unsigned)access,
                       (unsigned)mode);
    ok = (got == want) && (got == 0 || got == 1);

    hash = fnv1a_step(hash, (uint8_t)got);

    checks++;
    if (!ok) {
        mismatches++;
        if (mismatches < 20) {
            printf("MISMATCH nlevels=%d access=%d mode=%d got=%d want=%d\n",
                   nlevels, (int)access, (int)mode, got, want);
            for (i = 0; i < nlevels; i++)
                printf("  level %d: r=%u w=%u x=%u u=%u\n", i, r[i], w[i],
                       x[i], u[i]);
        }
    }
}

static void exhaustive(void) {
    unsigned r, w, x, u;
    pte_bits_t L[3];
    walk_access_t a;
    walk_mode_t m;

    /* 1-, 2-, and 3-level walks: every level takes all 16
     * R/W/X/U combinations. 96 + 1536 + 24576 = 26208 cases. */
    for (r = 0; r < 2; r++)
        for (w = 0; w < 2; w++)
            for (x = 0; x < 2; x++)
                for (u = 0; u < 2; u++)
                    for (a = WALK_ACCESS_LOAD; a <= WALK_ACCESS_EXEC; a++)
                        for (m = WALK_MODE_S; m <= WALK_MODE_U; m++) {
                            L[0].r = r; L[0].w = w;
                            L[0].x = x; L[0].u = u;
                            check_case(L, 1, a, m);

                            {
                                unsigned r2, w2, x2, u2;
                                for (r2 = 0; r2 < 2; r2++)
                                    for (w2 = 0; w2 < 2; w2++)
                                        for (x2 = 0; x2 < 2; x2++)
                                            for (u2 = 0; u2 < 2; u2++) {
                                                L[1].r = r2; L[1].w = w2;
                                                L[1].x = x2; L[1].u = u2;
                                                check_case(L, 2, a, m);

                                                {
                                                    unsigned r3, w3, x3, u3;
                                                    for (r3 = 0; r3 < 2; r3++)
                                                        for (w3 = 0; w3 < 2; w3++)
                                                            for (x3 = 0; x3 < 2; x3++)
                                                                for (u3 = 0; u3 < 2; u3++) {
                                                                    L[2].r = r3; L[2].w = w3;
                                                                    L[2].x = x3; L[2].u = u3;
                                                                    check_case(L, 3, a, m);
                                                                }
                                                }
                                            }
                            }
                        }
}

/* Fixed-seed random corpus: 8192 walks of 1 to 3 levels with
 * uniformly random bit fields, differential-tested like the
 * exhaustive rows. */
static void random_corpus(void) {
    unsigned i;
    for (i = 0; i < 8192u; i++) {
        pte_bits_t L[3];
        int nlevels = 1 + (int)(xorshift32() % 3u);
        int j;
        walk_access_t a = (walk_access_t)(xorshift32() % 3u);
        walk_mode_t m = (walk_mode_t)(xorshift32() % 2u);
        for (j = 0; j < nlevels; j++) {
            L[j].r = xorshift32() & 1u;
            L[j].w = xorshift32() & 1u;
            L[j].x = xorshift32() & 1u;
            L[j].u = xorshift32() & 1u;
        }
        check_case(L, nlevels, a, m);
    }
}

/* Contract edges that are not differential cases: the function
 * must fault on malformed input. */
static void edge_cases(void) {
    pte_bits_t L[1];
    int got, ok;

    L[0].r = 1; L[0].w = 1; L[0].x = 1; L[0].u = 1;

    got = walk_permits(L, 0, WALK_ACCESS_LOAD, WALK_MODE_S);
    ok = (got == 0);
    hash = fnv1a_step(hash, (uint8_t)got);
    checks++;
    if (!ok) {
        mismatches++;
        printf("MISMATCH nlevels=0: got=%d want=0\n", got);
    }

    got = walk_permits(NULL, 3, WALK_ACCESS_LOAD, WALK_MODE_S);
    ok = (got == 0);
    hash = fnv1a_step(hash, (uint8_t)got);
    checks++;
    if (!ok) {
        mismatches++;
        printf("MISMATCH levels=NULL: got=%d want=0\n", got);
    }
}

int main(void) {
    struct timespec t0, t1;
    double elapsed_ns;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    exhaustive();
    random_corpus();
    edge_cases();

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
