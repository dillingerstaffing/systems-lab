/*
 * test_bump.c - Host tests for the bump allocator, run against a fake
 * heap region (plain static arrays). Every number printed is measured
 * from the actual run; nothing is fabricated.
 *
 * Compile: gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_bump \
 *              test_bump.c bump.c
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime under -std=c11 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "bump.h"

static int failures = 0;

#define CHECK(cond) do {                                                \
        if (!(cond)) {                                                  \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            failures++;                                                 \
        }                                                               \
    } while (0)

/* Deterministic PRNG so the fragmentation test is reproducible. */
static uint32_t lcg_state = 0x12345678u;
static uint32_t lcg_next(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return lcg_state >> 8;
}

/* --- alignment: every power-of-two alignment must hold ---------------- */
/* 8 alignments x 9 sizes x ~150 avg bytes: needs headroom */
static uint8_t heap1[32768];

static void test_alignment(void)
{
    static const size_t aligns[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    bump_heap_t h;
    size_t i, worst = 0;

    printf("[align] power-of-two alignments 1..128\n");
    bump_init(&h, heap1, sizeof(heap1));
    for (i = 0; i < sizeof(aligns) / sizeof(aligns[0]); i++) {
        size_t a = aligns[i];
        /* try a few sizes and heap states per alignment */
        for (size_t s = 1; s <= 300; s += 37) {
            void *p = bump_alloc(&h, s, a);
            CHECK(p != NULL);
            if (p) {
                uintptr_t up = (uintptr_t)p;
                CHECK((up % a) == 0);
                if (a > worst)
                    worst = a;
                memset(p, 0xA5, s); /* must be safe to write */
            }
        }
    }
    /* invalid alignment is rejected, not silently misaligned */
    CHECK(bump_alloc(&h, 16, 3) == NULL);
    CHECK(bump_alloc(&h, 16, 24) == NULL);
    /* zero size is rejected */
    CHECK(bump_alloc(&h, 0, 8) == NULL);
    printf("[align] ok (max alignment verified: %zu)\n", worst);
}

/* --- basic write/read + free(NULL)/bad-free safety --------------------- */
static uint8_t heap2[2048];

static void test_basic_and_safety(void)
{
    bump_heap_t h;
    uint8_t *p;
    size_t i;

    printf("[basic] write/read round-trip, safe no-op frees\n");
    bump_init(&h, heap2, sizeof(heap2));
    p = bump_alloc(&h, 100, 8);
    CHECK(p != NULL);
    for (i = 0; i < 100; i++)
        p[i] = (uint8_t)(i * 3 + 1);
    for (i = 0; i < 100; i++)
        CHECK(p[i] == (uint8_t)(i * 3 + 1));

    bump_free(&h, NULL, 0);              /* must not crash */
    bump_free(&h, (void *)0x1, 16);      /* outside heap: ignored */
    {   /* pointer just past the heap end, built so the compiler cannot */
        /* constant-fold it into an array subscript */
        uintptr_t bad = (uintptr_t)heap2 + sizeof(heap2) + 64;
        bump_free(&h, (void *)bad, 16);   /* outside: ignored */
    }
    CHECK(h.n_free == 0);                /* none of those counted */

    bump_free(&h, p, 100);
    CHECK(h.n_free == 1);
    printf("[basic] ok\n");
}

/* --- free-list reuse: the freed block must serve the next alloc -------- */
static uint8_t heap3[2048];

static void test_free_reuse(void)
{
    bump_heap_t h;
    void *a, *b, *c;

    printf("[reuse] freed block is reused by a later allocation\n");
    bump_init(&h, heap3, sizeof(heap3));
    a = bump_alloc(&h, 200, 8);
    b = bump_alloc(&h, 300, 8);
    CHECK(a != NULL && b != NULL);
    (void)b;

    bump_free(&h, a, 200);
    c = bump_alloc(&h, 100, 8);
    CHECK(c != NULL);
    /* c must land inside the span that a occupied: genuine reuse */
    CHECK((uintptr_t)c >= (uintptr_t)a &&
          (uintptr_t)c + 100 <= (uintptr_t)a + 200);
    CHECK(h.n_reuse >= 1);
    printf("[reuse] ok (a=%p c=%p n_reuse=%lu)\n",
           a, c, h.n_reuse);
}

/* --- splitting: remainder of a reused block stays usable ---------------- */
static uint8_t heap4[4096];

static void test_split(void)
{
    bump_heap_t h;
    void *big, *small, *rest;

    printf("[split] remainder after reuse goes back on the free list\n");
    bump_init(&h, heap4, sizeof(heap4));
    big = bump_alloc(&h, 1024, 8);
    CHECK(big != NULL);
    bump_free(&h, big, 1024);

    small = bump_alloc(&h, 100, 8);   /* reuses big, leaves ~924 bytes */
    CHECK(small != NULL);
    CHECK((uintptr_t)small >= (uintptr_t)big);

    rest = bump_alloc(&h, 800, 8);    /* must come from the remainder */
    CHECK(rest != NULL);
    CHECK((uintptr_t)rest >= (uintptr_t)big &&
          (uintptr_t)rest + 800 <= (uintptr_t)big + 1024);
    printf("[split] ok (small=%p rest=%p)\n", small, rest);
}

/* --- OOM: fill a small heap, verify failure is clean and recoverable --- */
static uint8_t heap5[512];

static void test_oom(void)
{
    bump_heap_t h;
    void *ptrs[64];
    size_t n = 0;
    void *p;

    printf("[oom] exhaustion returns NULL, heap stays usable\n");
    bump_init(&h, heap5, sizeof(heap5));
    while (n < sizeof(ptrs) / sizeof(ptrs[0])) {
        p = bump_alloc(&h, 64, 8);
        if (!p)
            break;
        ptrs[n++] = p;
    }
    CHECK(n > 0);
    CHECK(p == NULL);
    CHECK(h.n_oom >= 1);
    printf("[oom] %zu x 64-byte blocks fit in 512 bytes, n_oom=%lu\n",
           n, h.n_oom);

    /* heap must still work after OOM: free one block, allocate again */
    bump_free(&h, ptrs[0], 64);
    p = bump_alloc(&h, 64, 8);
    CHECK(p != NULL);
    CHECK(h.n_oom >= 1); /* counter is monotonic, no reset tricks */
    printf("[oom] ok (recovered after OOM, n_alloc=%lu)\n", h.n_alloc);
}

/* --- fragmentation: churn, then measure -------------------------------- */
#define FRAG_HEAP (128u * 1024u)
#define FRAG_N 512
static uint8_t heap6[FRAG_HEAP];

static void test_fragmentation(void)
{
    bump_heap_t h;
    void *live[FRAG_N];
    size_t sizes[FRAG_N];
    uint8_t canary[FRAG_N];
    size_t i, n_got = 0, big_ok = 0;
    double frag_before, frag_after;
    struct timespec t0, t1;
    double secs;
    unsigned long ops = 0;

    printf("[frag] %d allocs of 16..256 bytes, free every other one\n", FRAG_N);
    bump_init(&h, heap6, sizeof(heap6));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < FRAG_N; i++) {
        size_t s = 16 + (lcg_next() % 241);
        void *p = bump_alloc(&h, s, 8);
        CHECK(p != NULL);
        if (!p)
            break;
        sizes[n_got] = s;
        live[n_got] = p;
        canary[n_got] = (uint8_t)(i & 0xFF);
        n_got++;
        memset(p, canary[n_got - 1], s); /* canary: detect overlaps */
        ops++;
    }
    for (i = 0; i < n_got; i += 2) {
        bump_free(&h, live[i], sizes[i]);
        ops++;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    frag_before = bump_fragmentation(&h);

    /* integrity: surviving blocks must still hold their canaries */
    for (i = 1; i < n_got; i += 2) {
        uint8_t *p = live[i];
        size_t s = sizes[i], j;
        for (j = 0; j < s; j++) {
            if (p[j] != canary[i]) {
                printf("FAIL canary corrupted at block %zu byte %zu\n",
                       i, j);
                failures++;
                break;
            }
        }
    }

    /* now try to place large blocks into the Swiss cheese */
    for (i = 0; i < 64; i++) {
        void *p = bump_alloc(&h, 2048, 8);
        if (!p)
            break;
        big_ok++;
        ops++;
    }
    frag_after = bump_fragmentation(&h);

    secs = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("[frag] live=%zu peak=%zu free_bytes=%zu\n",
           bump_live_bytes(&h), h.peak_bytes, bump_free_bytes(&h));
    printf("[frag] fragmentation ratio after churn: %.3f\n", frag_before);
    printf("[frag] 2048-byte blocks placed into fragments: %zu of 64\n",
           big_ok);
    printf("[frag] fragmentation ratio after large allocs: %.3f\n",
           frag_after);
    printf("[frag] churn throughput: %.1f kops/s (%lu ops in %.1f us)\n",
           (ops / 1000.0) / (secs > 0 ? secs : 1e-9), ops, secs * 1e6);
    printf("[frag] n_alloc=%lu n_free=%lu n_reuse=%lu\n",
           h.n_alloc, h.n_free, h.n_reuse);
    printf("[frag] ok\n");
}

/* --- tiny blocks: minimum granularity keeps every block reusable -------- */
static uint8_t heap7[1024];

static void test_tiny(void)
{
    bump_heap_t h;
    void *ps[32];
    size_t i;

    printf("[tiny] 1-byte allocs are all reusable after free\n");
    bump_init(&h, heap7, sizeof(heap7));
    for (i = 0; i < 32; i++) {
        ps[i] = bump_alloc(&h, 1, 8);
        CHECK(ps[i] != NULL);
    }
    for (i = 0; i < 32; i++)
        bump_free(&h, ps[i], 1);
    /* every freed 1-byte block must be reusable (rounded to node size) */
    for (i = 0; i < 32; i++) {
        ps[i] = bump_alloc(&h, 1, 8);
        CHECK(ps[i] != NULL);
    }
    CHECK(h.n_reuse >= 32);
    printf("[tiny] ok (n_reuse=%lu)\n", h.n_reuse);
}

int main(void)
{
    test_alignment();
    test_basic_and_safety();
    test_free_reuse();
    test_split();
    test_oom();
    test_tiny();
    test_fragmentation();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d FAILURES\n", failures);
    return 1;
}
