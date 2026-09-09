/*
 * test_buddy.c - Verification for lab/12-buddy-allocator.
 *
 * 1. Invariants: every returned pointer is 16-byte aligned and sits 16
 *    bytes past a block base that is aligned to its own block size; 512
 *    live blocks never overlap (pairwise check) and keep their canaries;
 *    freeing everything collapses the heap back into exactly one block,
 *    proven by free_bytes == largest_free == heap size and by a
 *    successful full-heap allocation afterwards.
 * 2. Churn: the same deterministic workload (512 allocations of 16..256
 *    bytes from a fixed LCG sequence, free every other block) runs
 *    through the buddy allocator and through lab/02's bump allocator;
 *    both fragmentation ratios are published side by side, using the
 *    same definition: (free - largest_free) / free.
 * 3. Placement: after the churn, 64 attempts to place a 2048-byte block
 *    in each allocator; successes counted.
 *
 * The bump allocator sources are used unmodified from ../02-bump-allocator.
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "buddy.h"
#include "../02-bump-allocator/bump.h"

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            failures++;                                                 \
        }                                                               \
    } while (0)

/* Deterministic PRNG, bit-identical to lab/02's, so the churn size
 * sequence matches lab/02's fragmentation test exactly. */
static uint32_t lcg_state = 0x12345678u;
static uint32_t lcg_next(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return lcg_state >> 8;
}
static void lcg_reset(void)
{
    lcg_state = 0x12345678u;
}

static double now_s(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

/* --- alignment: pointer and block-base alignment ----------------------- */

#define ALIGN_HEAP (1024u * 1024u)
static _Alignas(ALIGN_HEAP) uint8_t align_mem[ALIGN_HEAP];

static void test_alignment(void)
{
    buddy_heap_t h;
    size_t s, n = 0;

    printf("[align] 16-byte pointer alignment, block-base alignment\n");
    buddy_init(&h, align_mem, sizeof(align_mem));
    for (s = 1; s <= 1024; s++) {
        uint8_t *b;
        size_t order, bs;
        void *p = buddy_alloc(&h, s);

        CHECK(p != NULL);
        if (!p)
            break;
        CHECK(((uintptr_t)p & 15u) == 0); /* 16-byte aligned payload */
        b = (uint8_t *)p - BUDDY_HDR_SIZE;
        order = *(const size_t *)b;
        bs = (size_t)BUDDY_MIN_BLOCK << order;
        /* block base aligned to its own size (buddy invariant) */
        CHECK(((size_t)(b - h.base) % bs) == 0);
        /* block is the smallest power of two holding s + 16 */
        CHECK(bs >= s + BUDDY_HDR_SIZE && bs / 2 < s + BUDDY_HDR_SIZE);
        n++;
    }
    printf("[align] ok (%zu allocations checked)\n", n);
}

/* --- overlap: 512 live blocks, pairwise disjoint, canaries intact ------ */

#define OV_N 512
static _Alignas(128u * 1024u) uint8_t ov_mem[128u * 1024u];

static void test_overlap(void)
{
    buddy_heap_t h;
    static void *live[OV_N];
    static size_t req[OV_N];
    size_t i, j, pairs = 0;

    printf("[overlap] 512 blocks pairwise disjoint, canaries intact\n");
    lcg_reset();
    buddy_init(&h, ov_mem, sizeof(ov_mem));
    for (i = 0; i < OV_N; i++) {
        size_t s = 16 + (lcg_next() % 241);

        live[i] = buddy_alloc(&h, s);
        CHECK(live[i] != NULL);
        if (!live[i])
            return;
        req[i] = s;
        memset(live[i], (int)(i & 0xFF), s);
    }

    /* pairwise: block ranges [base, base + blocksize) must not overlap */
    for (i = 0; i < OV_N; i++) {
        uint8_t *bi = (uint8_t *)live[i] - BUDDY_HDR_SIZE;
        size_t bsi = (size_t)BUDDY_MIN_BLOCK << *(const size_t *)bi;

        for (j = i + 1; j < OV_N; j++) {
            uint8_t *bj = (uint8_t *)live[j] - BUDDY_HDR_SIZE;
            size_t bsj = (size_t)BUDDY_MIN_BLOCK << *(const size_t *)bj;

            CHECK(bi + bsi <= bj || bj + bsj <= bi);
            pairs++;
        }
    }

    /* canaries: no block's payload was clobbered by another */
    for (i = 0; i < OV_N; i++) {
        uint8_t *p = live[i];
        size_t s = req[i], k;

        for (k = 0; k < s; k++) {
            if (p[k] != (uint8_t)(i & 0xFF)) {
                printf("FAIL canary corrupted at block %zu byte %zu\n",
                       i, k);
                failures++;
                break;
            }
        }
    }

    /* free in shuffled order (Fisher-Yates on the fixed LCG) */
    for (i = OV_N - 1; i > 0; i--) {
        size_t k = lcg_next() % (i + 1);
        void *t = live[i];

        live[i] = live[k];
        live[k] = t;
    }
    for (i = 0; i < OV_N; i++)
        buddy_free(&h, live[i]);

    printf("[overlap] ok (%zu pairs checked, %lu allocs, %lu frees)\n",
           pairs, h.n_alloc, h.n_free);
}

/* --- coalescing: a fully freed heap is one block again ----------------- */

#define COAL_N 128
#define COAL_HEAP (64u * 1024u)
static _Alignas(COAL_HEAP) uint8_t coal_mem[COAL_HEAP];

static void test_coalescing(void)
{
    buddy_heap_t h;
    static void *live[COAL_N];
    size_t i, n = 0;

    printf("[coalesce] free-all collapses the heap into one block\n");
    lcg_reset();
    buddy_init(&h, coal_mem, sizeof(coal_mem));
    for (i = 0; i < COAL_N; i++) {
        size_t s = 32 + (lcg_next() % 481); /* 32..512 byte requests */

        live[n] = buddy_alloc(&h, s);
        CHECK(live[n] != NULL);
        if (!live[n])
            break;
        n++;
    }
    /* free in shuffled order so merging cannot rely on order */
    for (i = n; i > 1; i--) {
        size_t k = lcg_next() % i;
        void *t = live[i - 1];

        live[i - 1] = live[k];
        live[k] = t;
    }
    for (i = 0; i < n; i++)
        buddy_free(&h, live[i]);

    printf("[coalesce] free_bytes=%zu largest_free=%zu heap=%zu\n",
           buddy_free_bytes(&h), buddy_largest_free(&h), sizeof(coal_mem));
    CHECK(buddy_free_bytes(&h) == sizeof(coal_mem));
    CHECK(buddy_largest_free(&h) == sizeof(coal_mem));
    CHECK(buddy_fragmentation(&h) == 0.0);

    /* ground truth: one block means a full-heap allocation fits */
    {
        void *p = buddy_alloc(&h, sizeof(coal_mem) - BUDDY_HDR_SIZE);

        CHECK(p != NULL);
        printf("[coalesce] full-heap alloc after free-all: %s\n",
               p ? "succeeded" : "FAILED");
        buddy_free(&h, p);
    }
    printf("[coalesce] ok\n");
}

/* --- init validation: bad heaps are unusable, never crash -------------- */

static uint8_t bad_mem[1024];

static void test_init_validation(void)
{
    buddy_heap_t h;

    printf("[init] non-power-of-two size is rejected\n");
    buddy_init(&h, bad_mem, 1000);
    CHECK(buddy_alloc(&h, 16) == NULL);

    printf("[init] misaligned base is rejected\n");
    buddy_init(&h, bad_mem + 1, 512);
    CHECK(buddy_alloc(&h, 16) == NULL);

    printf("[init] oversize request fails cleanly, heap stays usable\n");
    {
        static _Alignas(1024) uint8_t m[1024];
        void *p;

        buddy_init(&h, m, sizeof(m));
        CHECK(buddy_alloc(&h, 1) != NULL);
        p = buddy_alloc(&h, sizeof(m)); /* bigger than the heap */
        CHECK(p == NULL);
        CHECK(h.n_oom == 1);
        CHECK(buddy_alloc(&h, 0) == NULL);
        buddy_free(&h, NULL); /* safe no-op */
    }
    printf("[init] ok\n");
}

/* --- churn: identical workload through buddy and bump ------------------ */

#define CHURN_HEAP (128u * 1024u)
#define CHURN_N 512
static _Alignas(CHURN_HEAP) uint8_t buddy_churn_mem[CHURN_HEAP];
static uint8_t bump_churn_mem[CHURN_HEAP];

struct churn_result {
    double frag;
    double frag_after_big;
    size_t big_placed;
    size_t live;
    size_t peak;
    double kops;
    unsigned long n_alloc, n_free, n_oom;
};

static struct churn_result churn_buddy(void)
{
    buddy_heap_t h;
    static void *live[CHURN_N];
    static size_t sizes[CHURN_N];
    struct churn_result r;
    double t0, t1;
    size_t i, n_got = 0, ops = 0, big_ok = 0;

    lcg_reset();
    buddy_init(&h, buddy_churn_mem, sizeof(buddy_churn_mem));

    t0 = now_s();
    for (i = 0; i < CHURN_N; i++) {
        size_t s = 16 + (lcg_next() % 241);
        void *p = buddy_alloc(&h, s);

        CHECK(p != NULL);
        if (!p)
            break;
        sizes[n_got] = s;
        live[n_got] = p;
        memset(p, (int)(i & 0xFF), s);
        n_got++;
        ops++;
    }
    CHECK(n_got == CHURN_N);
    CHECK(h.n_oom == 0);
    for (i = 0; i < n_got; i += 2) {
        buddy_free(&h, live[i]);
        ops++;
    }
    t1 = now_s();

    r.frag = buddy_fragmentation(&h);

    /* survivors' canaries must be intact */
    for (i = 1; i < n_got; i += 2) {
        uint8_t *p = live[i];
        size_t s = sizes[i], k;

        for (k = 0; k < s; k++) {
            if (p[k] != (uint8_t)(i & 0xFF)) {
                printf("FAIL buddy canary corrupted at block %zu\n", i);
                failures++;
                break;
            }
        }
    }

    /* place 2048-byte blocks into the fragments */
    for (i = 0; i < 64; i++) {
        void *p = buddy_alloc(&h, 2048);

        if (!p)
            break;
        big_ok++;
        ops++;
    }
    r.frag_after_big = buddy_fragmentation(&h);

    r.big_placed = big_ok;
    r.live = h.live_bytes;
    r.peak = h.peak_bytes;
    r.kops = (ops / 1000.0) / ((t1 - t0) > 0 ? (t1 - t0) : 1e-9);
    r.n_alloc = h.n_alloc;
    r.n_free = h.n_free;
    r.n_oom = h.n_oom;
    return r;
}

static struct churn_result churn_bump(void)
{
    bump_heap_t h;
    static void *live[CHURN_N];
    static size_t sizes[CHURN_N];
    struct churn_result r;
    double t0, t1;
    size_t i, n_got = 0, ops = 0, big_ok = 0;

    lcg_reset();
    bump_init(&h, bump_churn_mem, sizeof(bump_churn_mem));

    t0 = now_s();
    for (i = 0; i < CHURN_N; i++) {
        size_t s = 16 + (lcg_next() % 241);
        void *p = bump_alloc(&h, s, 8);

        CHECK(p != NULL);
        if (!p)
            break;
        sizes[n_got] = s;
        live[n_got] = p;
        memset(p, (int)(i & 0xFF), s);
        n_got++;
        ops++;
    }
    CHECK(n_got == CHURN_N);
    CHECK(h.n_oom == 0);
    for (i = 0; i < n_got; i += 2) {
        bump_free(&h, live[i], sizes[i]);
        ops++;
    }
    t1 = now_s();

    r.frag = bump_fragmentation(&h);

    for (i = 1; i < n_got; i += 2) {
        uint8_t *p = live[i];
        size_t s = sizes[i], k;

        for (k = 0; k < s; k++) {
            if (p[k] != (uint8_t)(i & 0xFF)) {
                printf("FAIL bump canary corrupted at block %zu\n", i);
                failures++;
                break;
            }
        }
    }

    for (i = 0; i < 64; i++) {
        void *p = bump_alloc(&h, 2048, 8);

        if (!p)
            break;
        big_ok++;
        ops++;
    }
    r.frag_after_big = bump_fragmentation(&h);

    r.big_placed = big_ok;
    r.live = bump_live_bytes(&h);
    r.peak = h.peak_bytes;
    r.kops = (ops / 1000.0) / ((t1 - t0) > 0 ? (t1 - t0) : 1e-9);
    r.n_alloc = h.n_alloc;
    r.n_free = h.n_free;
    r.n_oom = h.n_oom;
    return r;
}

static void test_churn(void)
{
    struct churn_result buddy, bump;

    printf("[churn] 512 allocs of 16..256 bytes, free every other one\n");
    printf("[churn] identical LCG size sequence into both allocators\n");
    buddy = churn_buddy();
    bump = churn_bump();

    printf("[churn] buddy: frag=%.3f live=%zu peak=%zu\n",
           buddy.frag, buddy.live, buddy.peak);
    printf("[churn] bump : frag=%.3f live=%zu peak=%zu\n",
           bump.frag, bump.live, bump.peak);
    printf("[churn] 2048-byte blocks placed: buddy %zu of 64, bump %zu of 64\n",
           buddy.big_placed, bump.big_placed);
    printf("[churn] frag after large allocs: buddy %.3f, bump %.3f\n",
           buddy.frag_after_big, bump.frag_after_big);
    printf("[churn] throughput: buddy %.1f kops/s, bump %.1f kops/s\n",
           buddy.kops, bump.kops);
    printf("[churn] buddy n_alloc=%lu n_free=%lu n_oom=%lu\n",
           buddy.n_alloc, buddy.n_free, buddy.n_oom);
    printf("[churn] bump  n_alloc=%lu n_free=%lu n_oom=%lu\n",
           bump.n_alloc, bump.n_free, bump.n_oom);
    printf("[churn] ok\n");
}

int main(void)
{
    test_alignment();
    test_overlap();
    test_coalescing();
    test_init_validation();
    test_churn();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d FAILURES\n", failures);
    return 1;
}
