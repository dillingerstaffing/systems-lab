/*
 * Stress test for the Treiber stack in treiber.c.
 *
 * What it proves, and how:
 *
 *   1. LIFO order (single-threaded): push 1,2,3 -> pop 3,2,1, then NULL.
 *   2. Element conservation under contention: 4 threads push 262144 distinct
 *      nodes, churn them (pop then re-push, 200000 rounds per thread), then
 *      4 threads drain the stack in parallel. Every node carries an id and a
 *      canary derived from the id. After the drain, each id must have been
 *      popped exactly once and every canary must be intact. Loss,
 *      duplication, or corruption of a single element fails the test.
 *   3. The ABA counter earns its keep: the churn phase recycles nodes
 *      through the head as fast as 4 threads can, which is exactly the
 *      pop-A-push-A-again pattern the tag defends against. The run reports
 *      how many stale observations the tag rejected (pointer unchanged, tag
 *      moved).
 *
 * Nodes live in one fixed pool for the whole run and are never freed, so
 * memory reclamation cannot mask or cause a bug: the only thing under test
 * is the stack algorithm. Termination arguments are given where they are
 * not obvious.
 */

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "treiber.h"

#define NTHREADS        4
#define PER_THREAD_PUSH 65536
#define NODES           (NTHREADS * PER_THREAD_PUSH) /* 262144 */
#define CHURN_ITERS     200000
/* A thread that cannot pop for this long is stuck on a bug, not on
 * contention: abort loudly instead of hanging the run. */
#define SPIN_TIMEOUT_S  60.0

static tstack_t stack;
static tnode_t *pool;                 /* NODES nodes, never freed */
static pthread_barrier_t barrier;

static _Atomic uint64_t drain_remaining;
static _Atomic uint64_t drain_index;
static tnode_t **drained;             /* NODES slots */
static _Atomic uint64_t drain_per_thread[NTHREADS];
static _Atomic bool phase_failed;

static uint64_t canary_of(uint64_t id)
{
    /* Knuth multiplicative hash plus a constant: deterministic, and a
     * single flipped bit anywhere in id or canary breaks the check. */
    return id * 0x9E3779B97F4A7C15ULL + 0x6C62272E07BB0142ULL;
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static bool cpu_has_cmpxchg16b(void)
{
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    (void)a; (void)b; (void)d;
    return (c >> 13) & 1u; /* CPUID.01H:ECX.CMPXCHG16B[bit 13] */
}

/* Pop, spinning while the stack is transiently empty. Aborts the run if the
 * wait exceeds SPIN_TIMEOUT_S (a bug, not contention, at that point). */
static tnode_t *pop_spin(void)
{
    double start = now_s();
    unsigned spin = 0;
    for (;;) {
        tnode_t *n = tstack_pop(&stack);
        if (n)
            return n;
        if ((++spin & 1023) == 0 && now_s() - start > SPIN_TIMEOUT_S) {
            fprintf(stderr, "FATAL: pop spun %.0f s on a non-draining stack\n",
                    now_s() - start);
            atomic_store_explicit(&phase_failed, true, memory_order_relaxed);
            return NULL;
        }
        sched_yield();
    }
}

typedef struct { int tid; } targ_t;

/* Phase 1: every thread pushes its own slice of the pool. */
static void *push_worker(void *arg)
{
    targ_t *t = arg;
    pthread_barrier_wait(&barrier);
    size_t base = (size_t)t->tid * PER_THREAD_PUSH;
    for (size_t i = 0; i < PER_THREAD_PUSH; i++) {
        tnode_t *n = &pool[base + i];
        n->id = base + i;
        n->canary = canary_of(n->id);
        tstack_push(&stack, n);
    }
    return NULL;
}

/* Phase 2: pop a node, push it straight back. Each round preserves the
 * element count exactly, while recycling nodes through the head as fast as
 * possible -- the pattern that exercises the ABA tag. */
static void *churn_worker(void *arg)
{
    (void)arg;
    pthread_barrier_wait(&barrier);
    for (int i = 0; i < CHURN_ITERS; i++) {
        tnode_t *n = pop_spin();
        if (!n)
            return NULL;
        tstack_push(&stack, n);
    }
    return NULL;
}

/*
 * Phase 3: parallel drain. Termination: drain_remaining starts at NODES and
 * only a successful pop decrements it. Unpopped nodes are always on the
 * stack (no thread holds one across iterations here), so remaining > 0
 * implies the stack is non-empty implies pop succeeds. Exactly NODES pops
 * can succeed, each node at most once, so the loop must end with
 * remaining == 0.
 */
static void *drain_worker(void *arg)
{
    targ_t *t = arg;
    pthread_barrier_wait(&barrier);
    uint64_t mine = 0;
    for (;;) {
        if (atomic_load_explicit(&drain_remaining, memory_order_relaxed) == 0)
            break;
        tnode_t *n = tstack_pop(&stack);
        if (!n) {
            sched_yield();
            continue;
        }
        size_t slot = atomic_fetch_add_explicit(&drain_index, 1,
                                                memory_order_relaxed);
        drained[slot] = n;
        atomic_fetch_sub_explicit(&drain_remaining, 1, memory_order_relaxed);
        mine++;
    }
    atomic_store_explicit(&drain_per_thread[t->tid], mine,
                          memory_order_relaxed);
    return NULL;
}

static uint64_t stat_cas_fail(void)
{
    return atomic_load_explicit(&stack.cas_fail, memory_order_relaxed);
}

static uint64_t stat_aba(void)
{
    return atomic_load_explicit(&stack.aba_caught, memory_order_relaxed);
}

static int run_phase(void *(*fn)(void *), const char *name, uint64_t ops,
                     uint64_t *fail0, uint64_t *aba0)
{
    pthread_t th[NTHREADS];
    targ_t args[NTHREADS];
    pthread_barrier_init(&barrier, NULL, NTHREADS + 1);
    for (int i = 0; i < NTHREADS; i++) {
        args[i].tid = i;
        if (pthread_create(&th[i], NULL, fn, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    pthread_barrier_wait(&barrier); /* release all threads at once */
    double t0 = now_s();
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(th[i], NULL);
    double dt = now_s() - t0;
    pthread_barrier_destroy(&barrier);

    uint64_t f1 = stat_cas_fail(), a1 = stat_aba();
    printf("%-28s %8.3f s  %7.2f Mops/s  cas_fail=%llu (+%llu)  aba_caught=%llu (+%llu)\n",
           name, dt, (ops / 1e6) / dt,
           (unsigned long long)f1, (unsigned long long)(f1 - *fail0),
           (unsigned long long)a1, (unsigned long long)(a1 - *aba0));
    *fail0 = f1;
    *aba0 = a1;
    if (atomic_load_explicit(&phase_failed, memory_order_relaxed)) {
        printf("FAIL: worker abort during %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    printf("== lab/07 lock-free Treiber stack ==\n");
    if (!cpu_has_cmpxchg16b()) {
        printf("SKIP: CPU lacks CMPXCHG16B\n");
        return 2;
    }
    printf("cpuid: CMPXCHG16B present\n");

    /* --- single-threaded LIFO sanity --- */
    tstack_init(&stack);
    if (!tstack_empty(&stack)) { printf("FAIL: new stack not empty\n"); return 1; }
    tnode_t s1 = { NULL, 1, canary_of(1) };
    tnode_t s2 = { NULL, 2, canary_of(2) };
    tnode_t s3 = { NULL, 3, canary_of(3) };
    tstack_push(&stack, &s1);
    tstack_push(&stack, &s2);
    tstack_push(&stack, &s3);
    tnode_t *p;
    uint64_t expect_ids[] = { 3, 2, 1 };
    for (int i = 0; i < 3; i++) {
        p = tstack_pop(&stack);
        if (!p || p->id != expect_ids[i] || p->canary != canary_of(p->id)) {
            printf("FAIL: LIFO order violated at pop %d\n", i);
            return 1;
        }
    }
    if (tstack_pop(&stack) != NULL || !tstack_empty(&stack)) {
        printf("FAIL: empty pop did not return NULL\n");
        return 1;
    }
    printf("sanity: LIFO order 3,2,1 ok, empty pop returns NULL\n");

    /* --- deterministic ABA rejection, white-box on the primitive ---
     * Recycle node A through the head while holding a stale observation
     * of it, then attempt the stale update by hand. The pointers match
     * (a tagless CAS would succeed and corrupt the list); the tag must
     * refuse it. */
    {
        tstack_t s2;
        tstack_init(&s2);
        tnode_t A = { NULL, 100, canary_of(100) };
        tnode_t B = { NULL, 101, canary_of(101) };
        tnode_t D = { NULL, 102, canary_of(102) };
        tstack_push(&s2, &A);              /* head = (A, 1) */
        tagged_t stale = tagged_load(&s2.head); /* observe (A, 1) */
        if (tstack_pop(&s2) != &A) { printf("FAIL: aba setup pop\n"); return 1; }
        tstack_push(&s2, &B);              /* head = (B, 3) */
        tstack_push(&s2, &A);              /* head = (A, 4): A recycled */
        D.next = (tnode_t *)stale.ptr;     /* what push() would have stored */
        tagged_t exp = stale;
        tagged_t des = { &D, stale.tag + 1 };
        bool ok = tagged_cas(&s2.head, &exp, des);
        if (ok) {
            printf("FAIL: stale CAS succeeded, ABA not defeated\n");
            return 1;
        }
        if (exp.ptr != stale.ptr || exp.tag == stale.tag) {
            printf("FAIL: unexpected head after rejected CAS\n");
            return 1;
        }
        /* The stack is undisturbed: popping still yields A then B. */
        if (tstack_pop(&s2) != &A || tstack_pop(&s2) != &B ||
            tstack_pop(&s2) != NULL) {
            printf("FAIL: stack disturbed by rejected CAS\n");
            return 1;
        }
        printf("unit: ABA rejection ok (stale tag %llu vs head tag %llu, "
               "same pointer %p)\n",
               (unsigned long long)stale.tag, (unsigned long long)exp.tag,
               exp.ptr);
    }

    /* --- contention phases --- */
    pool = malloc((size_t)NODES * sizeof *pool);
    drained = malloc((size_t)NODES * sizeof *drained);
    if (!pool || !drained) { printf("FAIL: malloc\n"); return 1; }
    tstack_init(&stack); /* reset stats after the sanity run */

    uint64_t fail0 = 0, aba0 = 0;
    printf("threads=%d nodes=%d\n", NTHREADS, NODES);
    if (run_phase(push_worker, "phase1: concurrent push",
                  NODES, &fail0, &aba0))
        return 1;
    if (run_phase(churn_worker, "phase2: pop/push churn",
                  (uint64_t)NTHREADS * CHURN_ITERS * 2, &fail0, &aba0))
        return 1;

    atomic_init(&drain_remaining, NODES);
    atomic_init(&drain_index, 0);
    atomic_init(&phase_failed, false);
    if (run_phase(drain_worker, "phase3: parallel drain",
                  NODES, &fail0, &aba0))
        return 1;

    /* --- conservation audit --- */
    int rc = 0;
    uint64_t got = atomic_load_explicit(&drain_index, memory_order_relaxed);
    if (got != NODES) {
        printf("FAIL: drained %llu nodes, pushed %d\n",
               (unsigned long long)got, NODES);
        rc = 1;
    }
    uint32_t *counts = calloc(NODES, sizeof *counts);
    if (!counts) { printf("FAIL: malloc\n"); return 1; }
    uint64_t bad_canary = 0, bad_id = 0;
    for (uint64_t i = 0; i < got; i++) {
        tnode_t *n = drained[i];
        if (n->id >= (uint64_t)NODES) { bad_id++; continue; }
        if (n->canary != canary_of(n->id)) bad_canary++;
        counts[n->id]++;
    }
    uint64_t dup = 0, lost = 0;
    for (int i = 0; i < NODES; i++) {
        if (counts[i] == 0) lost++;
        else if (counts[i] > 1) dup++;
    }
    free(counts);
    if (!tstack_empty(&stack)) {
        printf("FAIL: stack not empty after drain\n");
        rc = 1;
    }
    printf("audit: pushed=%d popped=%llu lost=%llu duplicated=%llu "
           "bad_id=%llu bad_canary=%llu stack_empty=%s\n",
           NODES, (unsigned long long)got,
           (unsigned long long)lost, (unsigned long long)dup,
           (unsigned long long)bad_id, (unsigned long long)bad_canary,
           tstack_empty(&stack) ? "yes" : "no");
    if (lost || dup || bad_id || bad_canary)
        rc = 1;

    printf("total: cas_fail=%llu aba_caught=%llu\n",
           (unsigned long long)stat_cas_fail(),
           (unsigned long long)stat_aba());
    printf("drain distribution per thread:");
    for (int i = 0; i < NTHREADS; i++)
        printf(" t%d=%llu", i,
               (unsigned long long)atomic_load_explicit(&drain_per_thread[i],
                                                        memory_order_relaxed));
    printf("\n");
    printf(rc == 0 ? "PASS: conservation holds, no loss/duplication/corruption\n"
                   : "FAIL: conservation violated\n");
    free(pool);
    free(drained);
    return rc;
}
