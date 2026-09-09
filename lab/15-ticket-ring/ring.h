/* ring.h: bounded ring buffer for many producers, one consumer.
 *
 * Slot identity: the producer commit counter `head` counts every item
 * ever pushed, so slot index = head & (CAP-1) (CAP is a power of two).
 * The consumer counter `tail` counts every item ever popped. The queue
 * holds exactly head - tail items; full means head - tail == CAP.
 *
 * Concurrency contract: every push runs inside the ticket lock, so
 * producers are mutually exclusive and `head` is only touched under
 * the lock. The single consumer owns `tail`. The only cross-thread
 * handoff is producer -> consumer via `head` (release store on push,
 * acquire load on pop) and consumer -> producer via `tail` (the
 * producer's fullness check loads it with acquire). No other
 * synchronization is needed; the ticket lock orders all producers.
 */
#ifndef RING_H
#define RING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#define RING_CAP 1024u

typedef struct {
    unsigned int data[RING_CAP];
    _Atomic unsigned long head; /* items committed by producers */
    _Atomic unsigned long tail; /* items drained by the consumer */
} mpmc_ring_t;

static inline void ring_init(mpmc_ring_t *r)
{
    atomic_init(&r->head, 0);
    atomic_init(&r->tail, 0);
}

/* Push one item. Call with the ticket lock held. Returns false if full. */
static inline bool ring_push_locked(mpmc_ring_t *r, unsigned int v)
{
    unsigned long h = atomic_load_explicit(&r->head, memory_order_relaxed);
    unsigned long t = atomic_load_explicit(&r->tail, memory_order_acquire);
    if (h - t >= RING_CAP)
        return false;
    r->data[h & (RING_CAP - 1)] = v;
    atomic_store_explicit(&r->head, h + 1, memory_order_release);
    return true;
}

/* Pop one item. Call only from the single consumer. False if empty. */
static inline bool ring_pop(mpmc_ring_t *r, unsigned int *out)
{
    unsigned long t = atomic_load_explicit(&r->tail, memory_order_relaxed);
    unsigned long h = atomic_load_explicit(&r->head, memory_order_acquire);
    if (t == h)
        return false;
    *out = r->data[t & (RING_CAP - 1)];
    atomic_store_explicit(&r->tail, t + 1, memory_order_release);
    return true;
}

#endif /* RING_H */
