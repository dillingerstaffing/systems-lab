/*
 * fuzz.c - Deterministic fuzz harness over two lab modules:
 *   lab/01 spsc.h  (wait-free single-producer/single-consumer ring buffer)
 *   lab/02 bump.h  (bump allocator with intrusive free-list reuse)
 *
 * The module sources are copied into this directory so the harness builds
 * standalone.
 *
 * How it works:
 * - A xorshift64 generator (Marsaglia's shift-xor sequence, see rng_next)
 *   drives every decision. The seed is fixed (FUZZ_SEED), so a run is bit
 *   for bit reproducible except for its wall-clock timing line.
 * - Ring buffer: interleaved push/pop of random payloads against an
 *   independent model, a plain array holding the expected queue contents.
 *   Every push result must agree with the model's full/empty state, every
 *   pop result with the model's state, and every popped value must equal
 *   the model's oldest value (FIFO order). A sentinel checks that pop on
 *   an empty queue leaves *out untouched. Periodically spsc_size,
 *   spsc_full and spsc_empty are cross-checked against the model.
 * - Allocator: random allocs (size 1..128, alignment class
 *   0,1,2,4,8,16,32,64,128), random canary touch of live blocks, random
 *   frees, in epochs over a fresh 256 KiB heap. The live set is kept in
 *   a band (64..192 blocks) so alloc, touch and free all fire at their
 *   nominal rates instead of the workload collapsing to one op type.
 *   Every returned pointer is checked for heap bounds and alignment, and
 *   for overlap with every other live block. Each live block carries a
 *   canary pattern derived from a per-allocation tag; canaries are
 *   verified on every touch and before every free. OOM is a legitimate
 *   outcome (the free list never coalesces and splits drop the smaller
 *   sliver, so a churned heap genuinely exhausts): every NULL return is
 *   counted and the run continues. A sliding window ends the epoch if
 *   the success rate collapses. At each epoch end, bump_live_bytes must
 *   equal the sum of the tracked live blocks, then every block is freed
 *   and live_bytes must return to zero.
 * - Double-free is deliberately NOT exercised: the allocator documents it
 *   as a caller bug it does not detect, and this harness must not invent
 *   behavior the module does not have. Freed blocks leave the tracking
 *   table, so a double free cannot occur by construction.
 *
 * Any violation prints to stderr and exits nonzero. Reaching the final
 * PASS line means the run completed with no crash and no violation.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "spsc.h"
#include "bump.h"

/* Fixed seed: the same run always produces the same operation sequence
 * and the same digest. Documented here and echoed in the run output. */
#define FUZZ_SEED 0x123456789ABCDEF0ULL

/* xorshift64: x ^= x<<13; x ^= x>>7; x ^= x<<17. State must stay nonzero,
 * which it does: a nonzero seed can never reach the all-zero fixed point. */
static uint64_t rng_state = FUZZ_SEED;

static uint64_t rng_next(void)
{
    uint64_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}

static void die(const char *what, unsigned long op)
{
    fprintf(stderr, "INVARIANT VIOLATION at op %lu: %s\n", op, what);
    exit(1);
}

/* ---------------- ring buffer fuzz ---------------- */

#define RING_CAP 32u
#define RING_OPS 2000000u
#define POP_SENTINEL 0xDEADBEEFu

static uint64_t fuzz_ring(void)
{
    static uint32_t storage[RING_CAP];
    static uint32_t model[RING_CAP]; /* expected queue contents */
    spsc_t q;
    uint64_t m_head = 0, m_tail = 0; /* m_tail - m_head == model count */
    uint64_t xor_sum = 0;
    unsigned long pushes = 0, pops = 0, full_hits = 0, empty_hits = 0;
    unsigned long i;

    if (!spsc_init(&q, storage, RING_CAP)) {
        fprintf(stderr, "spsc_init rejected capacity %u\n", RING_CAP);
        exit(1);
    }

    for (i = 0; i < RING_OPS; i++) {
        if ((rng_next() & 1u) == 0u) {
            /* push a random payload */
            uint32_t v = (uint32_t)rng_next();
            bool ok = spsc_push(&q, v);
            bool model_full = (m_tail - m_head) == RING_CAP;
            if (ok == model_full)
                die("push result disagrees with model full state", i);
            if (ok) {
                model[m_tail & (RING_CAP - 1u)] = v;
                m_tail++;
                pushes++;
                xor_sum ^= (uint64_t)v + pushes; /* order-sensitive */
            } else {
                full_hits++;
            }
        } else {
            /* pop */
            uint32_t v = POP_SENTINEL;
            bool ok = spsc_pop(&q, &v);
            bool model_empty = (m_head == m_tail);
            if (ok == model_empty)
                die("pop result disagrees with model empty state", i);
            if (ok) {
                uint32_t want = model[m_head & (RING_CAP - 1u)];
                if (v != want)
                    die("popped value differs from model (order error)", i);
                m_head++;
                pops++;
            } else {
                empty_hits++;
                if (v != POP_SENTINEL)
                    die("pop on empty touched *out", i);
            }
        }
        if ((i & 0xFFFFu) == 0u) {
            uint64_t n = m_tail - m_head;
            if (spsc_size(&q) != (size_t)n)
                die("spsc_size disagrees with model count", i);
            if (spsc_full(&q) != (n == RING_CAP))
                die("spsc_full disagrees with model", i);
            if (spsc_empty(&q) != (n == 0u))
                die("spsc_empty disagrees with model", i);
        }
    }

    printf("ring: %u ops, %lu pushes, %lu pops, %lu full-hits, %lu empty-hits\n",
           RING_OPS, pushes, pops, full_hits, empty_hits);
    return xor_sum ^ ((uint64_t)pushes << 32) ^ (uint64_t)pops;
}

/* ---------------- bump allocator fuzz ---------------- */

#define HEAP_BYTES (256u * 1024u)
#define MAX_LIVE 256u
#define LIVE_LO 64u   /* live-set band: below this, force alloc */
#define LIVE_HI 192u  /* live-set band: above this, force free */
#define EPOCHS 16u
#define OPS_PER_EPOCH 250000u
#define MAX_SIZE 128u
/* Sliding-window OOM guard: every 4096 alloc attempts, if fewer than 10%
 * succeeded, the heap is exhausted (no coalescing, dropped split slivers)
 * and the epoch ends. In a productive heap a full window under 10% is
 * essentially impossible, so this only cuts the unproductive tail. */
#define OOM_WINDOW 4096u

struct live_block {
    uint8_t *p;
    size_t size;  /* requested size */
    size_t align; /* requested alignment class */
    uint64_t tag; /* per-allocation canary seed */
};

/* Blocks smaller than a free-list node are rounded up to 16 bytes by the
 * allocator; live_bytes accounting follows the same rule. */
static size_t eff_size(size_t size)
{
    return size < 16u ? 16u : size;
}

static void canary_write(const struct live_block *b)
{
    size_t i;
    for (i = 0; i < b->size; i++)
        b->p[i] = (uint8_t)(b->tag >> (8u * (i & 7u))) ^ 0xA5u;
}

static bool canary_ok(const struct live_block *b)
{
    size_t i;
    for (i = 0; i < b->size; i++)
        if (b->p[i] != (uint8_t)((b->tag >> (8u * (i & 7u))) ^ 0xA5u))
            return false;
    return true;
}

static const size_t ALIGN_CLASSES[] = { 0, 1, 2, 4, 8, 16, 32, 64, 128 };
#define N_ALIGN_CLASSES (sizeof(ALIGN_CLASSES) / sizeof(ALIGN_CLASSES[0]))

static uint64_t fuzz_alloc(unsigned long *ops_done)
{
    static uint8_t heap[HEAP_BYTES];
    static struct live_block live[MAX_LIVE];
    uintptr_t base = (uintptr_t)heap;
    unsigned long allocs = 0, touches = 0, frees = 0, ooms = 0, reuses = 0;
    uint64_t tag_xor = 0;
    unsigned epoch;

    *ops_done = 0;

    for (epoch = 0; epoch < EPOCHS; epoch++) {
        bump_heap_t h;
        size_t n_live = 0;
        unsigned long op;
        unsigned long epoch_allocs = allocs;
        unsigned long win_attempts = 0, win_success = 0;

        bump_init(&h, heap, HEAP_BYTES);

        for (op = 0; op < OPS_PER_EPOCH; op++) {
            unsigned sel = (unsigned)(rng_next() % 10u);
            unsigned long seq = (unsigned long)epoch * OPS_PER_EPOCH + op;

            /* Keep the live set in its band so every op type fires at a
             * healthy rate for the whole epoch. */
            if (n_live < LIVE_LO)
                sel = 0;            /* fill up */
            else if (n_live >= LIVE_HI)
                sel = 9;            /* drain down */
            else if (n_live == MAX_LIVE)
                sel = 9;            /* backstop: table full */

            if (sel <= 3u) {
                /* allocate */
                size_t size = 1u + (size_t)(rng_next() % MAX_SIZE);
                size_t align = ALIGN_CLASSES[rng_next() % N_ALIGN_CLASSES];
                size_t eff_align = (align == 0u) ? 8u : align;
                uint64_t tag = rng_next();
                void *p = bump_alloc(&h, size, align);
                uintptr_t up;
                size_t k;

                win_attempts++;
                if (p == NULL) {
                    /* Clean OOM: counted, never a failure. */
                    ooms++;
                } else {
                    allocs++;
                    win_success++;
                    up = (uintptr_t)p;
                    /* bounds: [p, p+size) inside the heap region */
                    if (up < base || up - base + size > HEAP_BYTES)
                        die("returned pointer outside heap region", seq);
                    /* alignment: requested class (0 means default 8) */
                    if (up % eff_align != 0u)
                        die("returned pointer violates alignment class", seq);
                    /* overlap: no two live blocks may share bytes */
                    for (k = 0; k < n_live; k++) {
                        uintptr_t a = (uintptr_t)live[k].p;
                        uintptr_t b = a + live[k].size;
                        if (up < b && a < up + size)
                            die("returned block overlaps a live block", seq);
                    }
                    live[n_live].p = (uint8_t *)p;
                    live[n_live].size = size;
                    live[n_live].align = align;
                    live[n_live].tag = tag;
                    canary_write(&live[n_live]);
                    n_live++;
                    tag_xor ^= tag;
                }
                /* Sliding-window exhaustion check. */
                if (win_attempts >= OOM_WINDOW) {
                    if (win_success * 10u < win_attempts)
                        break;
                    win_attempts = 0;
                    win_success = 0;
                }
            } else if (sel <= 6u) {
                /* touch a random live block: verify canaries */
                size_t k = (size_t)(rng_next() % n_live);
                if (!canary_ok(&live[k]))
                    die("canary corrupted in live block", seq);
                touches++;
            } else {
                /* free a random live block: verify canaries first, then
                 * remove it from the table so it can never be freed twice */
                size_t k = (size_t)(rng_next() % n_live);
                if (!canary_ok(&live[k]))
                    die("canary corrupted before free", seq);
                bump_free(&h, live[k].p, live[k].size);
                live[k] = live[n_live - 1];
                n_live--;
                frees++;
            }
        }

        /* End-of-epoch invariants: every surviving block intact, the
         * allocator's live_bytes accounting matches the tracking table,
         * and live + free never exceeds the heap. Then drain the epoch:
         * free everything and require live_bytes to return to zero. */
        {
            size_t accounted = 0, k;
            for (k = 0; k < n_live; k++) {
                if (!canary_ok(&live[k]))
                    die("canary corrupted in surviving block", EPOCHS);
                accounted += eff_size(live[k].size);
            }
            if (bump_live_bytes(&h) != accounted)
                die("bump_live_bytes != sum of tracked live blocks", EPOCHS);
            if (bump_live_bytes(&h) + bump_free_bytes(&h) > HEAP_BYTES)
                die("live + free bytes exceed heap size", EPOCHS);
            reuses += h.n_reuse;
            while (n_live > 0) {
                n_live--;
                bump_free(&h, live[n_live].p, live[n_live].size);
                frees++;
            }
            if (bump_live_bytes(&h) != 0u)
                die("live_bytes nonzero after draining epoch", EPOCHS);
        }
        printf("epoch %u: %lu ops, %lu allocs sustained, drained clean\n",
               epoch, op, allocs - epoch_allocs);
        *ops_done += op;
    }

    printf("alloc: %lu ops, %lu allocs, %lu touches, %lu frees, %lu oom, "
           "%lu reuse\n",
           *ops_done, allocs, touches, frees, ooms, reuses);
    return tag_xor ^ ((uint64_t)allocs << 32) ^ (uint64_t)frees;
}

int main(void)
{
    clock_t t0, t1;
    double ring_s, alloc_s;
    uint64_t ring_digest, alloc_digest, digest;
    unsigned long alloc_ops = 0, total_ops;

    printf("fuzz harness: seed 0x%016llX\n", (unsigned long long)FUZZ_SEED);

    t0 = clock();
    ring_digest = fuzz_ring();
    t1 = clock();
    ring_s = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;

    t0 = clock();
    alloc_digest = fuzz_alloc(&alloc_ops);
    t1 = clock();
    alloc_s = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;

    total_ops = (unsigned long)RING_OPS + alloc_ops;
    digest = ring_digest ^ (alloc_digest * 0x9E3779B97F4A7C15ULL);
    printf("digest: 0x%016llX\n", (unsigned long long)digest);
    printf("timing: ring %.2f s (%.0f ops/s), alloc %.2f s (%.0f ops/s)\n",
           ring_s, (double)RING_OPS / ring_s,
           alloc_s, (double)alloc_ops / alloc_s);
    printf("PASS: %lu operations, 0 crashes, 0 invariant violations\n",
           total_ops);
    return 0;
}
