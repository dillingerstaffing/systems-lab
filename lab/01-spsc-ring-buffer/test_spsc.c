/* Tests for the wait-free SPSC ring buffer (spsc.h).
 *
 * 1. Edge cases: empty pop, fill-to-full, overfill rejection, order, empty
 *    again; init rejects non-power-of-two capacities.
 * 2. Index wrap-around: many push/pop cycles on a tiny buffer so the slot
 *    index wraps via the mask over and over.
 * 3. Counter wrap-around: head/tail preset just below SIZE_MAX so the
 *    unsigned counters themselves wrap through zero mid-test.
 * 4. Stress: STRESS_N items pushed by one thread and popped in order by
 *    another, with a checksum, plus a throughput number.
 *
 * Exit status 0 on success, 1 on the first failure.
 */

#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "spsc.h"

#ifndef STRESS_N
#define STRESS_N 10000000u
#endif

static int failures = 0;

#define CHECK(cond, ...)                                           \
    do {                                                           \
        if (!(cond)) {                                             \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);             \
            printf(__VA_ARGS__);                                    \
            putchar('\n');                                          \
            failures++;                                            \
            return;                                                \
        }                                                          \
    } while (0)

/* ------------------------------------------------------------------ */
/* 1. Edge cases                                                       */
/* ------------------------------------------------------------------ */

static void test_empty_full_edges(void)
{
    printf("[edge] empty/full behavior, capacity 4\n");
    uint32_t storage[4];
    spsc_t q;

    CHECK(spsc_init(&q, storage, 4), "init(cap 4) rejected");
    CHECK(spsc_empty(&q), "fresh queue not empty");
    CHECK(!spsc_full(&q), "fresh queue reports full");

    uint32_t out = 0xDEAD;
    CHECK(!spsc_pop(&q, &out), "pop on empty succeeded");
    CHECK(out == 0xDEAD, "pop on empty touched *out");

    for (uint32_t i = 0; i < 4; i++)
        CHECK(spsc_push(&q, 100 + i), "push %u failed", i);
    CHECK(spsc_full(&q), "full queue not reported full");
    CHECK(!spsc_empty(&q), "full queue reported empty");
    CHECK(!spsc_push(&q, 999), "push on full succeeded");

    for (uint32_t i = 0; i < 4; i++) {
        CHECK(spsc_pop(&q, &out), "pop %u failed", i);
        CHECK(out == 100 + i, "pop %u: got %u, want %u", i, out, 100 + i);
    }
    CHECK(spsc_empty(&q), "drained queue not empty");
    CHECK(!spsc_pop(&q, &out), "pop after drain succeeded");
    printf("[edge] ok\n");
}

static void test_init_validation(void)
{
    printf("[edge] init capacity validation\n");
    uint32_t storage[16];
    spsc_t q;

    size_t bad[] = {0, 3, 5, 6, 7, 12, 100};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++)
        CHECK(!spsc_init(&q, storage, bad[i]),
              "init accepted bad capacity %zu", bad[i]);

    size_t good[] = {1, 2, 8, 16};
    for (size_t i = 0; i < sizeof good / sizeof good[0]; i++) {
        CHECK(spsc_init(&q, storage, good[i]),
              "init rejected good capacity %zu", good[i]);
        CHECK(spsc_capacity(&q) == good[i], "capacity mismatch");
    }
    printf("[edge] ok\n");
}

/* ------------------------------------------------------------------ */
/* 2. Slot-index wrap-around (mask reuse)                              */
/* ------------------------------------------------------------------ */

static void test_index_wraparound(void)
{
    printf("[wrap] 200000 push/pop cycles on capacity 4\n");
    uint32_t storage[4];
    spsc_t q;
    uint32_t out;

    CHECK(spsc_init(&q, storage, 4), "init failed");
    for (uint32_t i = 0; i < 200000; i++) {
        CHECK(spsc_push(&q, i), "push failed at cycle %u", i);
        CHECK(spsc_pop(&q, &out), "pop failed at cycle %u", i);
        CHECK(out == i, "cycle %u: got %u", i, out);
    }
    CHECK(spsc_empty(&q), "queue not empty after cycles");
    printf("[wrap] ok\n");
}

/* ------------------------------------------------------------------ */
/* 3. Counter wrap-around: head/tail pass through zero mid-test        */
/* ------------------------------------------------------------------ */

static void test_counter_wraparound(void)
{
    printf("[wrap] head/tail counters wrapping past SIZE_MAX\n");
    uint32_t storage[8];
    spsc_t q;

    CHECK(spsc_init(&q, storage, 8), "init failed");
    /* Start both counters 3 below the maximum so the test drives them
     * through zero. This is white-box but legitimate: the header promises
     * unsigned counters, and this is exactly the path that must be right. */
    atomic_store_explicit(&q.head, (size_t)-3, memory_order_relaxed);
    atomic_store_explicit(&q.tail, (size_t)-3, memory_order_relaxed);

    /* Push 3: tail goes (SIZE_MAX-3) -> SIZE_MAX. */
    for (uint32_t i = 0; i < 3; i++)
        CHECK(spsc_push(&q, 10 + i), "pre-wrap push %u failed", i);
    CHECK(spsc_size(&q) == 3, "size %zu, want 3", spsc_size(&q));

    /* Push 5 more: tail wraps through 0 to 5; queue becomes full. */
    for (uint32_t i = 3; i < 8; i++)
        CHECK(spsc_push(&q, 10 + i), "wrap push %u failed", i);
    CHECK(spsc_full(&q), "queue not full after wrap");
    CHECK(!spsc_push(&q, 999), "push on wrapped-full queue succeeded");

    /* Drain: order must survive the wrap. */
    uint32_t out;
    for (uint32_t i = 0; i < 8; i++) {
        CHECK(spsc_pop(&q, &out), "wrap pop %u failed", i);
        CHECK(out == 10 + i, "wrap pop %u: got %u, want %u", i, out, 10 + i);
    }
    CHECK(spsc_empty(&q), "queue not empty after wrapped drain");

    /* Keep going past the wrap to prove steady state is sane. */
    for (uint32_t i = 0; i < 100; i++) {
        CHECK(spsc_push(&q, i), "post-wrap push %u failed", i);
        CHECK(spsc_pop(&q, &out), "post-wrap pop %u failed", i);
        CHECK(out == i, "post-wrap cycle %u: got %u", i, out);
    }
    printf("[wrap] ok\n");
}

/* ------------------------------------------------------------------ */
/* 4. Stress: STRESS_N items across two threads, order + checksum      */
/* ------------------------------------------------------------------ */

typedef struct {
    spsc_t *q;
    uint32_t n;
} worker_arg_t;

static void *producer(void *arg)
{
    worker_arg_t *a = arg;
    for (uint32_t i = 0; i < a->n; i++)
        while (!spsc_push(a->q, i)) {
            /* full: spin until the consumer makes room */
        }
    return NULL;
}

static void *consumer(void *arg)
{
    worker_arg_t *a = arg;
    uint64_t *sum = malloc(sizeof *sum);
    if (!sum)
        return NULL;
    *sum = 0;
    uint32_t expect = 0, v;
    while (expect < a->n) {
        if (spsc_pop(a->q, &v)) {
            if (v != expect) {
                /* Encode the mismatch in the sum slot: top bit set, and
                 * stash expected/got so the main thread can report it. */
                *sum = UINT64_C(0x8000000000000000) |
                       ((uint64_t)expect << 32) | v;
                return sum;
            }
            *sum += v;
            expect++;
        }
    }
    return sum;
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void test_stress(void)
{
    const uint32_t n = STRESS_N;
    printf("[stress] %" PRIu32 " items, producer/consumer threads, cap 4096\n",
           n);

    /* 4096 slots x 4 bytes = 16 KiB, comfortably inside L1. */
    static uint32_t storage[4096];
    spsc_t q;
    CHECK(spsc_init(&q, storage, 4096), "init failed");

    worker_arg_t arg = {&q, n};
    pthread_t prod, cons;

    double t0 = now_s();
    CHECK(pthread_create(&prod, NULL, producer, &arg) == 0,
          "pthread_create producer failed");
    CHECK(pthread_create(&cons, NULL, consumer, &arg) == 0,
          "pthread_create consumer failed");

    void *ret = NULL;
    CHECK(pthread_join(prod, NULL) == 0, "pthread_join producer failed");
    CHECK(pthread_join(cons, &ret) == 0, "pthread_join consumer failed");
    double dt = now_s() - t0;

    if (!ret) {
        printf("FAIL consumer thread returned NULL (malloc failed?)\n");
        failures++;
        return;
    }
    uint64_t sum = *(uint64_t *)ret;
    free(ret);

    if (sum & UINT64_C(0x8000000000000000)) {
        uint32_t expect = (uint32_t)(sum >> 32);
        uint32_t got = (uint32_t)sum;
        printf("FAIL order violation: expected %u, got %u\n", expect, got);
        failures++;
        return;
    }
    uint64_t want = (uint64_t)n * (n - 1) / 2;
    CHECK(sum == want, "checksum %" PRIu64 ", want %" PRIu64, sum, want);

    double ops = 2.0 * n / dt; /* every item is one push + one pop */
    printf("[stress] transferred %" PRIu32 " items in %.3f s\n", n, dt);
    printf("[stress] throughput: %.1f Mops/s (push+pop counted)\n", ops / 1e6);
    printf("[stress] checksum ok (%" PRIu64 ")\n", sum);
    printf("[stress] ok\n");
}

int main(void)
{
    test_empty_full_edges();
    test_init_validation();
    test_index_wraparound();
    test_counter_wraparound();
    test_stress();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d FAILURES\n", failures);
    return 1;
}
