/* Stress + unit tests for lab/08-seqlock.
 *
 * Unit (single-threaded):
 *   - write/read round-trip for 1000 generations: every read succeeds on
 *     the first try, the counter matches, and the payload is internally
 *     consistent.
 *   - detector check: a valid payload with one flipped bit (in a word, in
 *     the counter, in the stored checksum) must fail seqlock_verify, and
 *     the untouched copy must pass. This grounds the claim that the
 *     checksum catches a torn read independently of the sequence protocol.
 *
 * Stress (1 writer + 4 readers):
 *   - the writer publishes generations as fast as it can; each reader
 *     takes 2.5M successful snapshots (10M total).
 *   - every successful snapshot is checksum-verified (a failure means a
 *     torn read slipped past the sequence protocol) and checked for
 *     per-reader monotonicity of the generation counter (the single writer
 *     only ever increases it, so a decrease is a consistency bug).
 *   - retries are counted per reader: a retry means the reader observed
 *     the writer inside its critical section or overlapping its copy.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "seqlock.h"

#define NREADERS 4
#define READS_PER_READER 2500000UL /* 10M successful reads total */

static struct seqlock lock;
static atomic_bool stop;
static atomic_bool failed;
static unsigned long long writer_gens;

struct reader_stats {
    unsigned long long reads;   /* successful snapshots */
    unsigned long long retries; /* try_read returned false */
    unsigned long long torn;    /* passed seq check, failed checksum */
    unsigned long long mono;    /* generation counter went backwards */
};

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void *writer_fn(void *arg)
{
    (void)arg;
    uint64_t c = 0;
    while (!atomic_load_explicit(&stop, memory_order_relaxed)) {
        seqlock_write(&lock, ++c);
    }
    writer_gens = c;
    return NULL;
}

static void *reader_fn(void *arg)
{
    struct reader_stats *st = arg;
    struct seqlock_payload p;
    uint64_t last = 0;
    bool first = true;

    memset(st, 0, sizeof *st);
    while (st->reads < READS_PER_READER &&
           !atomic_load_explicit(&stop, memory_order_relaxed)) {
        if (seqlock_try_read(&lock, &p)) {
            if (!seqlock_payload_ok(&p)) {
                st->torn++;
                atomic_store_explicit(&failed, true, memory_order_relaxed);
                atomic_store_explicit(&stop, true, memory_order_relaxed);
                return NULL;
            }
            if (!first && p.counter < last) {
                st->mono++;
                atomic_store_explicit(&failed, true, memory_order_relaxed);
                atomic_store_explicit(&stop, true, memory_order_relaxed);
                return NULL;
            }
            first = false;
            last = p.counter;
            st->reads++;
        } else {
            st->retries++;
        }
    }
    return NULL;
}

static int unit_tests(void)
{
    struct seqlock l;
    struct seqlock_payload p;
    seqlock_init(&l);

    /* Round-trip: with no concurrency every read succeeds first try. */
    for (uint64_t c = 1; c <= 1000; c++) {
        seqlock_write(&l, c);
        if (!seqlock_try_read(&l, &p)) {
            printf("unit: FAIL generation %llu needed a retry with no writer running\n",
                   (unsigned long long)c);
            return 1;
        }
        if (p.counter != c || !seqlock_payload_ok(&p)) {
            printf("unit: FAIL generation %llu inconsistent\n", (unsigned long long)c);
            return 1;
        }
    }
    printf("unit: round-trip 1000 generations ok, 0 retries\n");

    /* Detector: one flipped bit anywhere in a published payload must fail. */
    seqlock_write(&l, 4242);
    if (!seqlock_try_read(&l, &p) || !seqlock_verify(&p)) {
        printf("unit: FAIL clean payload did not verify\n");
        return 1;
    }
    struct seqlock_payload q;

    q = p; q.words[3] ^= 1ULL;
    if (seqlock_verify(&q)) { printf("unit: FAIL flipped word accepted\n"); return 1; }

    q = p; q.counter ^= 1ULL;
    if (seqlock_verify(&q)) { printf("unit: FAIL flipped counter accepted\n"); return 1; }

    q = p; q.check ^= 1ULL;
    if (seqlock_verify(&q)) { printf("unit: FAIL flipped checksum accepted\n"); return 1; }

    printf("unit: checksum detector rejects single-bit corruption in word, counter, and check\n");
    return 0;
}

int main(void)
{
    printf("== lab/08 seqlock: single writer, %d readers ==\n", NREADERS);

    if (unit_tests() != 0)
        return 1;

    seqlock_init(&lock);
    atomic_init(&stop, false);
    atomic_init(&failed, false);

    pthread_t wt;
    pthread_t rt[NREADERS];
    struct reader_stats st[NREADERS];

    if (pthread_create(&wt, NULL, writer_fn, NULL) != 0) {
        perror("pthread_create writer");
        return 1;
    }
    for (int i = 0; i < NREADERS; i++) {
        if (pthread_create(&rt[i], NULL, reader_fn, &st[i]) != 0) {
            perror("pthread_create reader");
            return 1;
        }
    }

    double t0 = now_s();
    for (int i = 0; i < NREADERS; i++)
        pthread_join(rt[i], NULL);
    double t1 = now_s();
    atomic_store_explicit(&stop, true, memory_order_relaxed);
    pthread_join(wt, NULL);

    unsigned long long reads = 0, retries = 0, torn = 0, mono = 0;
    for (int i = 0; i < NREADERS; i++) {
        printf("reader %d: reads=%llu retries=%llu torn=%llu mono_viol=%llu\n",
               i, st[i].reads, st[i].retries, st[i].torn, st[i].mono);
        reads += st[i].reads;
        retries += st[i].retries;
        torn += st[i].torn;
        mono += st[i].mono;
    }

    unsigned long long attempts = reads + retries;
    double retry_rate = attempts ? 100.0 * (double)retries / (double)attempts : 0.0;
    printf("total: reads=%llu attempts=%llu retries=%llu retry_rate=%.3f%% torn=%llu mono_viol=%llu\n",
           reads, attempts, retries, retry_rate, torn, mono);
    printf("writer published %llu generations in %.3f s (%.2f Mreads/s across readers)\n",
           writer_gens, t1 - t0, reads / (t1 - t0) / 1e6);

    if (atomic_load_explicit(&failed, memory_order_relaxed) || torn || mono) {
        printf("FAIL: torn reads or monotonicity violations observed\n");
        return 1;
    }
    if (reads != (unsigned long long)NREADERS * READS_PER_READER) {
        printf("FAIL: short read count\n");
        return 1;
    }
    printf("PASS: %llu reads, 0 torn, 0 monotonicity violations\n", reads);
    return 0;
}
