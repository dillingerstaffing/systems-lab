/* ticket.h: ticket lock built on C11 atomics.
 *
 * The mechanism, stated exactly:
 *  - atomic_fetch_add on `next` hands each acquirer a unique,
 *    strictly increasing ticket. Uniqueness comes from the atomicity
 *    of fetch_add; monotonicity comes from addition by one.
 *  - The lock is held by the acquirer whose ticket equals `serving`.
 *    `serving` only advances to ticket+1 when the current holder
 *    releases, so holders are admitted in exactly ticket order: FIFO,
 *    starvation-free.
 *  - Release stores serving = my_ticket + 1 with release ordering, so
 *    every write the holder made inside the critical section is
 *    visible to the next holder.
 */
#ifndef TICKET_H
#define TICKET_H

#include <stdatomic.h>
#include <sched.h>

typedef struct {
    _Atomic unsigned long next;
    _Atomic unsigned long serving;
} ticket_lock_t;

static inline void ticket_lock_init(ticket_lock_t *l)
{
    atomic_init(&l->next, 0);
    atomic_init(&l->serving, 0);
}

/* Returns the caller's ticket. Spins until serving == ticket. */
static inline unsigned long ticket_acquire(ticket_lock_t *l)
{
    unsigned long my =
        atomic_fetch_add_explicit(&l->next, 1, memory_order_relaxed);
    unsigned long spins = 0;
    while (atomic_load_explicit(&l->serving, memory_order_acquire) != my) {
#if defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#endif
        /* Brief pause-spin, then yield: on an oversubscribed machine
         * the ticket holder may be descheduled, and burning whole
         * time slices spinning only delays its return. */
        if (++spins >= 64) {
            sched_yield();
        }
    }
    return my;
}

/* Hand the lock to the next ticket. */
static inline void ticket_release(ticket_lock_t *l, unsigned long my)
{
    atomic_store_explicit(&l->serving, my + 1, memory_order_release);
}

#endif /* TICKET_H */
