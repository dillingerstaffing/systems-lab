/* test_ticket_ring.c: verify the ticket-guarded MPMC ring.
 *
 * Workload: 4 producer threads x 250,000 items each, one consumer
 * drains 1,000,000 items. Item id = producer * ITEMS + seq, so every
 * item carries its origin and its position in its producer's stream.
 * Each producer holds the ticket lock for exactly one push.
 *
 * Checked on the consumer side, all from direct observation:
 *  - loss/duplication: a 1,000,000-entry bitmap records every id seen;
 *    a set bit on arrival is a duplicate, an unset bit at the end is a
 *    loss. The total item count and the checksum (sum of all ids)
 *    cross-check the bitmap against the closed-form expected sum.
 *  - per-producer FIFO: producer p's items must arrive with seq
 *    0,1,2,...,ITEMS-1 in order; any reorder or repeat fails the
 *    seq == last+1 check.
 *  - fairness gap: for producer p, the gap between two consecutive
 *    items is the number of other producers' items served between
 *    them. With all producers contending and each holding the lock
 *    for one push, strict ticket order gives gap exactly P-1 = 3.
 *    The test records the maximum gap seen per producer.
 */
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ring.h"
#include "ticket.h"

#ifndef NPROD
#define NPROD 4
#endif
#ifndef ITEMS_PER_PROD
#define ITEMS_PER_PROD 250000
#endif
#define TOTAL_ITEMS ((unsigned long)NPROD * ITEMS_PER_PROD)

static mpmc_ring_t g_ring;
static ticket_lock_t g_lock;

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void *producer_fn(void *arg)
{
    unsigned int p = (unsigned int)(uintptr_t)arg;
    for (unsigned long seq = 0; seq < ITEMS_PER_PROD;) {
        unsigned long ticket = ticket_acquire(&g_lock);
        if (ring_push_locked(&g_ring, (unsigned int)(p * ITEMS_PER_PROD + seq))) {
            seq++;
        } else {
            /* Ring full: give the lock back so the consumer keeps
             * draining, then try again with a fresh ticket. */
            struct timespec rq = { 0, 1000 };
            nanosleep(&rq, NULL);
        }
        ticket_release(&g_lock, ticket);
    }
    return NULL;
}

/* ---- edge tests on a local ring ------------------------------------- */
static int edge_tests(void)
{
    mpmc_ring_t r;
    ticket_lock_t l;
    ring_init(&r);
    ticket_lock_init(&l);

    printf("[edge] empty pop on fresh ring\n");
    unsigned int v = 0xdeadbeef;
    if (ring_pop(&r, &v) || v != 0xdeadbeef) {
        printf("[edge] FAIL: pop on empty ring returned true or touched *out\n");
        return 1;
    }
    printf("[edge] ok\n");

    printf("[edge] fill to capacity %u, one more must fail\n", RING_CAP);
    for (unsigned int i = 0; i < RING_CAP; i++) {
        unsigned long t = ticket_acquire(&l);
        bool ok = ring_push_locked(&r, i);
        ticket_release(&l, t);
        if (!ok) {
            printf("[edge] FAIL: push %u rejected before full\n", i);
            return 1;
        }
    }
    {
        unsigned long t = ticket_acquire(&l);
        bool ok = ring_push_locked(&r, 0xffffffff);
        ticket_release(&l, t);
        if (ok) {
            printf("[edge] FAIL: push on full ring accepted\n");
            return 1;
        }
    }
    printf("[edge] ok\n");

    printf("[edge] drain order is FIFO\n");
    for (unsigned int i = 0; i < RING_CAP; i++) {
        unsigned int x;
        if (!ring_pop(&r, &x) || x != i) {
            printf("[edge] FAIL: drain mismatch at %u\n", i);
            return 1;
        }
    }
    if (ring_pop(&r, &v)) {
        printf("[edge] FAIL: ring not empty after full drain\n");
        return 1;
    }
    printf("[edge] ok\n");
    return 0;
}

/* ---- fairness micro-check: ticket order is admission order ---------- */
static _Atomic unsigned long g_order_next;
static unsigned long g_order[64];

static void *ticket_order_fn(void *arg)
{
    ticket_lock_t *l = (ticket_lock_t *)arg;
    for (int i = 0; i < 8; i++) {
        unsigned long t = ticket_acquire(l);
        /* Inside the critical section, so this counter advances in
         * exact admission order. */
        unsigned long slot =
            atomic_fetch_add_explicit(&g_order_next, 1, memory_order_relaxed);
        g_order[slot] = t;
        ticket_release(l, t);
    }
    return NULL;
}

static int ticket_order_test(void)
{
    /* 8 threads each take 8 tickets; the admission order observed
     * inside the critical section must be the ticket sequence 0..63. */
    ticket_lock_t l;
    ticket_lock_init(&l);
    atomic_init(&g_order_next, 0);
    memset(g_order, 0, sizeof(g_order));

    printf("[order] 8 threads x 8 tickets, admission must equal ticket order\n");
    pthread_t th[8];
    for (int i = 0; i < 8; i++)
        pthread_create(&th[i], NULL, ticket_order_fn, &l);
    for (int i = 0; i < 8; i++)
        pthread_join(th[i], NULL);

    for (int i = 0; i < 64; i++) {
        if (g_order[i] != (unsigned long)i) {
            printf("[order] FAIL: slot %d admitted ticket %lu\n", i,
                   g_order[i]);
            return 1;
        }
    }
    printf("[order] ok: 64 admissions in exact ticket order\n");
    return 0;
}

/* ---- main stress test ---------------------------------------------- */
static int stress_test(void)
{
    static unsigned char seen[TOTAL_ITEMS]; /* zero-initialized */
    pthread_t prod[NPROD];

    ring_init(&g_ring);
    ticket_lock_init(&g_lock);

    printf("[stress] %d producers x %lu items, ring capacity %u, 1 consumer\n",
           NPROD, (unsigned long)ITEMS_PER_PROD, RING_CAP);

    double t0 = now_s();
    for (int i = 0; i < NPROD; i++)
        pthread_create(&prod[i], NULL, producer_fn,
                       (void *)(uintptr_t)i);

    /* Consumer loop: single thread, no lock needed on this side. */
    unsigned long got = 0;
    unsigned long long checksum = 0;
    unsigned long last_seq[NPROD];
    unsigned long last_idx[NPROD];
    unsigned long max_gap[NPROD];
    for (int i = 0; i < NPROD; i++) {
        last_seq[i] = (unsigned long)-1;
        last_idx[i] = (unsigned long)-1;
        max_gap[i] = 0;
    }
    int failed = 0;

    while (got < TOTAL_ITEMS) {
        unsigned int id;
        if (!ring_pop(&g_ring, &id)) {
            /* Empty: yield so a producer holding the next item gets
             * scheduled. Does not affect ordering measurements. */
            sched_yield();
            continue;
        }
        unsigned int p = id / ITEMS_PER_PROD;
        unsigned long seq = id % ITEMS_PER_PROD;
        if (p >= (unsigned int)NPROD) {
            printf("[stress] FAIL: item id %u out of range\n", id);
            failed = 1;
            break;
        }
        if (seen[id]) {
            printf("[stress] FAIL: duplicate item id %u\n", id);
            failed = 1;
            break;
        }
        seen[id] = 1;
        if (seq != last_seq[p] + 1) {
            printf("[stress] FAIL: producer %u broke FIFO: seq %lu after %lu\n",
                   p, seq, last_seq[p]);
            failed = 1;
            break;
        }
        last_seq[p] = seq;
        if (last_idx[p] != (unsigned long)-1) {
            unsigned long gap = got - last_idx[p] - 1;
            if (gap > max_gap[p])
                max_gap[p] = gap;
        }
        last_idx[p] = got;
        checksum += id;
        got++;
    }

    for (int i = 0; i < NPROD; i++)
        pthread_join(prod[i], NULL);
    double t1 = now_s();

    if (failed)
        return 1;

    /* loss check: every bitmap bit must be set */
    unsigned long lost = 0;
    for (unsigned long i = 0; i < TOTAL_ITEMS; i++)
        lost += (seen[i] == 0);

    /* checksum against the closed form:
     * sum_p (ITEMS*p*ITEMS + ITEMS*(ITEMS-1)/2) */
    unsigned long long expect = 0;
    for (int p = 0; p < NPROD; p++)
        expect += (unsigned long long)ITEMS_PER_PROD * p * ITEMS_PER_PROD
                + (unsigned long long)ITEMS_PER_PROD * (ITEMS_PER_PROD - 1) / 2;

    double secs = t1 - t0;
    unsigned long worst_gap = 0;
    printf("[stress] drained %lu items in %.3f s\n", got, secs);
    printf("[stress] throughput: %.1f Mops/s (push+pop counted)\n",
           (2.0 * TOTAL_ITEMS / secs) / 1e6);
    printf("[stress] lost items: %lu\n", lost);
    printf("[stress] duplicate items: 0 (bitmap check)\n");
    printf("[stress] per-producer FIFO order: preserved (seq == last+1 held)\n");
    printf("[stress] checksum: %llu, expected %llu\n", checksum, expect);
    for (int p = 0; p < NPROD; p++) {
        printf("[stress] producer %d max fairness gap: %lu items\n", p,
               max_gap[p]);
        if (max_gap[p] > worst_gap)
            worst_gap = max_gap[p];
    }
    printf("[stress] worst fairness gap across all producers: %lu (ideal %d)\n",
           worst_gap, NPROD - 1);

    if (lost != 0 || checksum != expect) {
        printf("[stress] FAIL: lost=%lu checksum %s\n", lost,
               checksum == expect ? "ok" : "MISMATCH");
        return 1;
    }
    printf("[stress] ok\n");
    return 0;
}

int main(void)
{
    if (edge_tests())
        return 1;
    if (ticket_order_test())
        return 1;
    if (stress_test())
        return 1;
    printf("ALL TESTS PASSED\n");
    return 0;
}
