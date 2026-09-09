/* lab/08-seqlock: sequence lock for single-writer / multi-reader shared state.
 *
 * One writer, any number of readers. The writer owns a 64-bit sequence
 * counter:
 *
 *   seq odd   -> writer is inside its critical section, readers retry
 *   seq even  -> idle; the payload holds generation seq/2
 *
 * Writer protocol:
 *   1. seq = s+1 (relaxed store; single writer, no contention on seq itself)
 *   2. plain stores to the payload
 *   3. seq = s+2 (release store: publishes the payload)
 *
 * Reader protocol:
 *   1. s0 = acquire-load seq; if odd, retry
 *   2. plain copy of the payload
 *   3. s1 = acquire-load seq; if s0 != s1, retry
 *
 * Why a (s0 == s1, s0 even) observation means the copy is one generation
 * the writer actually published:
 *
 * - The writer's stores to seq form a single total order: it is the only
 *   writer and the counter only increases, odd while writing, even when
 *   idle. A completed writer critical section that overlapped the reader's
 *   copy window would have stored an odd value and then a larger even
 *   value; cache coherence means the reader's second load (program-ordered
 *   after the copy) cannot then still observe s0. So s0 == s1 even implies
 *   no writer critical section overlapped the copy.
 * - Visibility: the reader's first acquire load, when it reads an even
 *   value s0, synchronizes with the writer's release store of s0. The
 *   payload stores are sequenced before that release store, and the
 *   reader's payload loads are sequenced after the acquire load, so the
 *   copy sees the published generation, not stale bytes.
 *
 * Preconditions the caller must hold: exactly one thread ever calls
 * seqlock_write on a given lock; readers never write. Violating either is
 * a data race outside what this header can defend.
 */

#ifndef SEQLOCK_H
#define SEQLOCK_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SEQLOCK_WORDS 8

/* One published generation. `counter` is the generation id, `words` are a
 * deterministic function of it, and `check` is a 64-bit FNV-1a hash over
 * counter+words, stored by the writer. A reader recomputes the hash over
 * its copy: a mismatch means the copy is not a generation the writer ever
 * published, i.e. a torn read, detected independently of the sequence
 * protocol. */
struct seqlock_payload {
    uint64_t counter;
    uint64_t words[SEQLOCK_WORDS];
    uint64_t check;
};

struct seqlock {
    atomic_ullong seq;
    struct seqlock_payload data;
};

/* FNV-1a mixing step, 64-bit. */
static inline uint64_t seqlock_mix(uint64_t h, uint64_t w)
{
    h ^= w;
    h *= 1099511628211ULL; /* FNV prime */
    return h;
}

/* words[i] as a pure function of the generation counter. Both sides agree
 * on this so a reader can check internal consistency, not just the hash. */
static inline uint64_t seqlock_word(uint64_t counter, size_t i)
{
    return counter * 0x9E3779B97F4A7C15ULL + (uint64_t)i * 0xBF58476D1CE4E5B9ULL;
}

static inline uint64_t seqlock_checksum(const struct seqlock_payload *p)
{
    uint64_t h = 14695981039346656037ULL; /* FNV offset basis */
    h = seqlock_mix(h, p->counter);
    for (size_t i = 0; i < SEQLOCK_WORDS; i++)
        h = seqlock_mix(h, p->words[i]);
    return h;
}

/* Recompute the writer's checksum over a copy. False means the copy never
 * existed as a published generation: a torn read. */
static inline bool seqlock_verify(const struct seqlock_payload *p)
{
    return seqlock_checksum(p) == p->check;
}

/* Full internal-consistency check: checksum plus the word function. */
static inline bool seqlock_payload_ok(const struct seqlock_payload *p)
{
    if (!seqlock_verify(p))
        return false;
    for (size_t i = 0; i < SEQLOCK_WORDS; i++)
        if (p->words[i] != seqlock_word(p->counter, i))
            return false;
    return true;
}

static inline void seqlock_init(struct seqlock *l)
{
    /* Publish generation 0 as a real generation before any thread runs.
     * Every even seq value a reader can ever observe must pair with a
     * checksummed payload; a zeroed payload with check = 0 would fail
     * seqlock_verify (the FNV of zeros is not zero), so the initial state
     * has to satisfy the same invariant as every writer-published state. */
    l->data.counter = 0;
    for (size_t i = 0; i < SEQLOCK_WORDS; i++)
        l->data.words[i] = seqlock_word(0, i);
    l->data.check = seqlock_checksum(&l->data);
    atomic_init(&l->seq, 0);
}

/* Publish generation `counter`. Single-writer only. */
static inline void seqlock_write(struct seqlock *l, uint64_t counter)
{
    unsigned long long s = atomic_load_explicit(&l->seq, memory_order_relaxed);
    /* s is even here: only this thread writes, and it always leaves seq even. */
    atomic_store_explicit(&l->seq, s + 1, memory_order_relaxed); /* odd: inside */
    struct seqlock_payload *d = &l->data;
    d->counter = counter;
    for (size_t i = 0; i < SEQLOCK_WORDS; i++)
        d->words[i] = seqlock_word(counter, i);
    d->check = seqlock_checksum(d);
    /* Release: everything above is visible to a reader whose acquire load
     * observes this even value. */
    atomic_store_explicit(&l->seq, s + 2, memory_order_release);
}

/* Try to take a consistent snapshot into *out.
 * Returns true on success; false means the caller must retry (a writer was
 * inside, or a write overlapped the copy). The caller counts retries. */
static inline bool seqlock_try_read(struct seqlock *l, struct seqlock_payload *out)
{
    unsigned long long s0 = atomic_load_explicit(&l->seq, memory_order_acquire);
    if (s0 & 1ULL)
        return false; /* writer inside */
    *out = l->data; /* plain copy, guarded by the sequence check below */
    unsigned long long s1 = atomic_load_explicit(&l->seq, memory_order_acquire);
    return s0 == s1;
}

#endif /* SEQLOCK_H */
