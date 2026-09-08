#ifndef TREIBER_H
#define TREIBER_H

/*
 * Treiber stack: a lock-free LIFO built on one shared atomic word.
 *
 * The shared state is a single 16-byte value, (pointer, tag), and the ONLY
 * operation that ever touches it is the x86-64 double-width compare-and-
 * swap, `lock cmpxchg16b`. That instruction is atomic by the architecture:
 * the 16 bytes are compared and, on equality, replaced as one indivisible
 * step, and no other core can observe a halfway state. Everything else in
 * this file is ordinary loads and stores ordered around that primitive.
 *
 * The tag is a 64-bit counter bumped by every successful CAS. It exists for
 * one reason: without it, a thread that reads head = A, gets preempted while
 * other threads pop A and push A again, would succeed its CAS on recycled
 * state and silently corrupt the list (the ABA hazard). With the tag, each
 * successful CAS mints a (pointer, tag) pair that has never existed before
 * and never will again (2^64 values cannot wrap in any real run), so a stale
 * observation can never match the current head. A CAS failure in which the
 * pointer is unchanged but the tag moved is exactly such a rejection, and
 * the stack counts them (see aba_caught).
 *
 * x86-64 only: this module executes `cmpxchg16b` directly and faults without
 * it. The test harness checks CPUID for the feature before running.
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The unit of atomicity: 16-byte aligned so `cmpxchg16b` accepts it
 * (a misaligned operand raises #GP). */
typedef struct __attribute__((aligned(16))) {
    void    *ptr;   /* head node, low 8 bytes */
    uint64_t tag;   /* monotonic ABA counter, high 8 bytes */
} tagged_t;

_Static_assert(_Alignof(tagged_t) == 16, "tagged_t must be 16-byte aligned");
_Static_assert(sizeof(tagged_t) == 16, "tagged_t must be exactly 16 bytes");

/*
 * The two primitives. `lock cmpxchg16b` compares rdx:rax against the 16
 * bytes at the address and, if equal, stores rcx:rbx there, atomically. On
 * failure rdx:rax comes back holding the value actually present. The LOCK
 * prefix makes the instruction a full memory barrier: no load or store,
 * compiler- or CPU-side, migrates across it (the "memory" clobber stops the
 * compiler; the locked bus/cache transaction stops the CPU).
 *
 * Ordering argument, in C11 terms:
 *
 *   push needs RELEASE on a successful head update: the plain stores to the
 *   node (n->next, and the caller's id/canary, sequenced before the push
 *   call) must be visible to any thread that later reads the new head.
 *   pop needs ACQUIRE when it takes the head: before reading n->next or the
 *   payload, it must observe everything sequenced before the push that
 *   published n. A failed CAS publishes nothing, so it needs no ordering.
 *
 *   `lock cmpxchg16b` is strictly stronger than release, acquire, or both
 *   combined, so both requirements are met with margin. The chain is:
 *   push's plain stores ->(sequenced-before)-> push CAS (full barrier)
 *   ->(synchronizes-with, via the release sequence the CAS extends)->
 *   pop's head read/CAS (full barrier) ->(sequenced-before)-> pop's plain
 *   loads. Release sequences (C11 7.17.3): every CAS on the head is a
 *   read-modify-write, so each one extends the release sequence of the push
 *   it observed, and any thread that reads any later head value still
 *   synchronizes with the original push.
 */

/* Full 16-byte compare-and-swap. On failure, *expected is overwritten with
 * the value actually present (as reported by the instruction itself).
 * Operands: %0=ZF result, %1=head memory, %2/%3=rax/rdx comparand,
 * %4/%5=rbx/rcx new value. */
static inline bool tagged_cas(volatile tagged_t *addr,
                              tagged_t *expected,
                              tagged_t desired)
{
    unsigned char zf; /* ZF = 1 iff the exchange happened */
    uint64_t exp_lo = (uint64_t)(uintptr_t)expected->ptr; /* rax */
    uint64_t exp_hi = expected->tag;                      /* rdx */
    __asm__ volatile("lock cmpxchg16b %1"
                     : "=@ccz" (zf),
                       "+m" (*addr),
                       "+a" (exp_lo),
                       "+d" (exp_hi)
                     : "b" ((uint64_t)(uintptr_t)desired.ptr),
                       "c" (desired.tag)
                     : "memory", "cc");
    expected->ptr = (void *)(uintptr_t)exp_lo;
    expected->tag = exp_hi;
    return zf != 0;
}

/*
 * 16-byte load, implemented as CAS((0,0) -> (0,0)). If the head is not
 * (0,0) nothing is stored and the instruction reports the true current
 * value; if it is (0,0) the store is a no-op. Either way the value comes
 * back through the architecturally atomic 16-byte primitive, so the read
 * cannot tear. One locked operation per load is the price of a read that
 * rests on a hardware guarantee rather than on "aligned loads are atomic
 * in practice". x86-64 cmpxchg has no spurious failures, so a reported
 * failure always means the value genuinely differed.
 */
static inline tagged_t tagged_load(volatile tagged_t *addr)
{
    tagged_t cur = { NULL, 0 };
    tagged_cas(addr, &cur, cur);
    return cur;
}

typedef struct tnode {
    struct tnode *next;  /* plain pointer: published by the push CAS,
                            observed after acquiring the head */
    uint64_t      id;    /* caller-owned identity, set before push */
    uint64_t      canary;/* caller-owned integrity word, set before push */
} tnode_t;

typedef struct {
    tagged_t head;              /* offset 0; struct alignment is 16 */
    _Atomic uint64_t cas_fail;  /* CAS failures (contention), stats only */
    _Atomic uint64_t aba_caught;/* failures with pointer unchanged: stale
                                   observations the tag rejected */
} tstack_t;

void     tstack_init(tstack_t *s);
void     tstack_push(tstack_t *s, tnode_t *n);
tnode_t *tstack_pop(tstack_t *s);   /* NULL when the stack is empty */
bool     tstack_empty(tstack_t *s);

/*
 * Contract on the caller (standard for intrusive containers): a node is
 * pushed by one thread at a time and is never pushed while already on the
 * stack. Nodes are never freed while any thread may hold a reference; the
 * harness uses a fixed pool, so reclamation is out of scope by construction
 * and the test isolates the stack algorithm itself.
 */

#endif /* TREIBER_H */
