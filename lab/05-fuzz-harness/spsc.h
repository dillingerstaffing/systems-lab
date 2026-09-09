/* Wait-free single-producer / single-consumer ring buffer (Lamport queue).
 *
 * Rules of use: exactly one thread calls spsc_push (the producer), exactly
 * one thread calls spsc_pop (the consumer). spsc_init runs before either
 * thread starts.
 *
 * How it works:
 * - Bounded buffer; capacity must be a power of two.
 * - head and tail are monotonically increasing unsigned counters, so the
 *   slot index is (counter & mask) and unsigned wrap-around of the counters
 *   themselves is harmless.
 * - push writes the slot first, then release-stores tail. pop acquire-loads
 *   tail, reads the slot, then release-stores head. Synchronization flows
 *   one way (producer -> consumer via tail, consumer -> producer via head);
 *   there are no locks and no compare-and-swap retry loops, so both
 *   operations are wait-free.
 * - full  <=> (tail - head) == capacity.  empty <=> head == tail.
 *   The subtraction is unsigned, so it stays correct across wrap-around.
 *
 * Cache behavior: head and tail live on separate 64-byte cache lines so the
 * producer and consumer do not false-share.
 */

#ifndef SPSC_H
#define SPSC_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct spsc {
    _Alignas(64) atomic_size_t head; /* consumer-owned: next slot to read */
    _Alignas(64) atomic_size_t tail; /* producer-owned: next slot to write */
    size_t mask;                    /* capacity - 1; written by spsc_init */
    uint32_t *buf;                  /* capacity slots; written by spsc_init */
} spsc_t;

/* Set up q over caller-provided storage of `capacity` uint32_t slots.
 * Returns false (and touches nothing) unless capacity is a nonzero power
 * of two. */
static inline bool spsc_init(spsc_t *q, uint32_t *storage, size_t capacity)
{
    if (capacity == 0 || (capacity & (capacity - 1)) != 0)
        return false;
    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);
    q->mask = capacity - 1;
    q->buf = storage;
    return true;
}

static inline size_t spsc_capacity(const spsc_t *q)
{
    return q->mask + 1;
}

/* Approximate element count; exact only if called from the consumer while
 * the producer is quiet (or vice versa). */
static inline size_t spsc_size(const spsc_t *q)
{
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    return tail - head;
}

static inline bool spsc_empty(const spsc_t *q)
{
    return spsc_size(q) == 0;
}

static inline bool spsc_full(const spsc_t *q)
{
    return spsc_size(q) == spsc_capacity(q);
}

/* Producer only. Returns false without touching the queue if it is full. */
static inline bool spsc_push(spsc_t *q, uint32_t v)
{
    size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    /* Acquire pairs with the consumer's release-store of head: we must see
     * every slot the consumer freed before we reuse it. */
    size_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    if (tail - head == spsc_capacity(q))
        return false; /* full */
    q->buf[tail & q->mask] = v;
    /* Release pairs with the consumer's acquire-load of tail: the slot
     * write is visible before the item becomes readable. */
    atomic_store_explicit(&q->tail, tail + 1, memory_order_release);
    return true;
}

/* Consumer only. Returns false (leaving *out untouched) if empty. */
static inline bool spsc_pop(spsc_t *q, uint32_t *out)
{
    size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    /* Acquire pairs with the producer's release-store of tail: we must see
     * the slot write before we read it. */
    size_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    if (head == tail)
        return false; /* empty */
    *out = q->buf[head & q->mask];
    /* Release pairs with the producer's acquire-load of head: the slot is
     * not reused until our read is complete. */
    atomic_store_explicit(&q->head, head + 1, memory_order_release);
    return true;
}

#endif /* SPSC_H */
