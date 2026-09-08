#include "treiber.h"

#include <stddef.h>
#include <stdint.h>

_Static_assert(_Alignof(tstack_t) == 16, "tstack_t must be 16-byte aligned");
_Static_assert(offsetof(tstack_t, head) == 0, "head must be at offset 0");

void tstack_init(tstack_t *s)
{
    s->head.ptr = NULL;
    s->head.tag = 0;
    atomic_init(&s->cas_fail, 0);
    atomic_init(&s->aba_caught, 0);
    /* The head is written before any thread starts (the harness inits
     * before spawning threads), so no atomicity is needed here. */
}

void tstack_push(tstack_t *s, tnode_t *n)
{
    tagged_t cur = tagged_load(&s->head);
    for (;;) {
        n->next = cur.ptr; /* plain store; ordered before the CAS below */
        tagged_t exp = cur;
        tagged_t des = { n, cur.tag + 1 };
        if (tagged_cas(&s->head, &exp, des))
            return; /* linearization point: n became the head here */
        atomic_fetch_add_explicit(&s->cas_fail, 1, memory_order_relaxed);
        if (exp.ptr == cur.ptr) {
            /* Pointer identical, tag moved: this thread's observation was
             * stale. Without the tag the CAS would have succeeded on
             * recycled state. */
            atomic_fetch_add_explicit(&s->aba_caught, 1,
                                      memory_order_relaxed);
        }
        cur = exp;
    }
}

tnode_t *tstack_pop(tstack_t *s)
{
    tagged_t cur = tagged_load(&s->head);
    for (;;) {
        tnode_t *n = (tnode_t *)cur.ptr;
        if (n == NULL)
            return NULL;
        /* Plain load of n->next: safe because our head read (full barrier)
         * synchronizes with the push CAS that published n, and the push's
         * store to n->next was sequenced before that CAS. */
        tnode_t *next = n->next;
        tagged_t exp = cur;
        tagged_t des = { next, cur.tag + 1 };
        if (tagged_cas(&s->head, &exp, des))
            return n; /* linearization point: n left the stack here */
        atomic_fetch_add_explicit(&s->cas_fail, 1, memory_order_relaxed);
        if (exp.ptr == cur.ptr)
            atomic_fetch_add_explicit(&s->aba_caught, 1,
                                      memory_order_relaxed);
        cur = exp;
    }
}

bool tstack_empty(tstack_t *s)
{
    return tagged_load(&s->head).ptr == NULL;
}
