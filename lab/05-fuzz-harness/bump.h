/*
 * bump.h - Bare-metal-friendly bump allocator with a free list.
 *
 * Design: allocations normally come from a monotonically increasing bump
 * pointer over a caller-supplied memory region. Freed blocks go onto an
 * intrusive free list (first bytes of the freed block hold the list node)
 * and are reused by later allocations; larger blocks are split, the
 * remainder going back on the free list. No coalescing.
 *
 * Bare-metal properties: no libc calls at all (this header and bump.c use
 * only <stddef.h> and <stdint.h>), no static state, the caller owns the
 * backing memory, and every failure mode is a NULL return, never a trap.
 * The free-list node needs 16 bytes, so every allocation is rounded up to
 * at least 16 bytes internally; this guarantees any handed-out block can
 * later be freed and reused.
 *
 * Threading: NOT thread-safe. One heap per hart/core, or guard with a
 * spinlock at the call site.
 */
#ifndef BUMP_H
#define BUMP_H

#include <stddef.h>
#include <stdint.h>

typedef struct bump_heap bump_heap_t;

struct bump_heap {
    uint8_t *base;   /* start of the backing region (caller-owned) */
    size_t   cap;    /* total bytes in the region */
    uintptr_t bump;  /* absolute address of the next fresh byte */
    struct free_node *free_list;

    /* Statistics, updated on every call. */
    size_t live_bytes;      /* bytes currently handed out */
    size_t peak_bytes;      /* high-water mark of live_bytes */
    unsigned long n_alloc;  /* successful bump_alloc calls */
    unsigned long n_free;   /* accepted bump_free calls */
    unsigned long n_reuse;  /* allocations satisfied from the free list */
    unsigned long n_oom;    /* allocations that failed for lack of memory */
};

/* Initialise a heap over `size` bytes starting at `mem`. `mem` may be any
 * alignment; the allocator aligns internally. */
void bump_init(bump_heap_t *h, void *mem, size_t size);

/*
 * Allocate `size` bytes aligned to `align` (must be a power of two;
 * 0 means the default alignment of 8). Returns NULL when:
 *   - size == 0,
 *   - align is not a power of two,
 *   - the heap has no block that fits (n_oom is incremented).
 * The free list is searched first (first fit); only then is fresh
 * memory bumped.
 */
void *bump_alloc(bump_heap_t *h, size_t size, size_t align);

/*
 * Return a block previously obtained from bump_alloc. `size` must be the
 * same size passed to bump_alloc for that block. Passing NULL, or a
 * pointer outside the heap's region, is ignored. Double-free is a caller
 * bug and is not detected (documented limitation).
 */
void bump_free(bump_heap_t *h, void *ptr, size_t size);

/* Bytes currently handed out. */
size_t bump_live_bytes(const bump_heap_t *h);

/* Bytes free: free-list contents plus the untouched tail of the region. */
size_t bump_free_bytes(const bump_heap_t *h);

/*
 * Fragmentation ratio in [0, 1]: the fraction of free space that is NOT
 * in the single largest contiguous free run. 0.0 means all free space is
 * one contiguous run; a value near 1.0 means the free space is badly
 * scattered. The untouched tail counts as one run.
 */
double bump_fragmentation(const bump_heap_t *h);

#endif /* BUMP_H */
