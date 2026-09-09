/*
 * buddy.h - Buddy allocator over a caller-supplied, power-of-two heap.
 *
 * Design: the heap is a power of two in size, split recursively into
 * power-of-two blocks. The smallest block is 32 bytes. Each block carries
 * a 16-byte header: the block's order (log2(size/32)) at offset 0, and,
 * while the block sits on a free list, the intrusive list link at offset
 * 8. The caller-visible pointer is block + 16, so it is always 16-byte
 * aligned, and every block base is aligned to its own size, which is what
 * makes the buddy of a block computable with a single xor of the offset.
 *
 * Freeing a block merges it with its free buddy repeatedly, so a fully
 * freed heap collapses back into one block. No libc calls at all (only
 * <stddef.h> and <stdint.h>), no static state, the caller owns the backing
 * memory, and every failure mode is a NULL return, never a trap.
 *
 * Contract on the backing region: `size` must be a power of two (>= 32)
 * and `mem` must be aligned to `size`. If either is violated the heap is
 * marked unusable and every allocation returns NULL.
 *
 * Threading: NOT thread-safe. One heap per hart/core, or guard with a
 * spinlock at the call site. Double-free is a caller bug and is not
 * detected (same documented limitation as lab/02).
 */
#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>
#include <stdint.h>

/* Smallest block, in bytes (includes the 16-byte header). A power of two. */
#define BUDDY_MIN_BLOCK 32u
/* Bytes between the block base and the caller-visible pointer. */
#define BUDDY_HDR_SIZE 16u
/* Number of orders supported; valid orders are 0 .. BUDDY_MAX_ORDERS - 1,
 * so the largest heap is 32 bytes << 23 = 256 MiB. */
#define BUDDY_MAX_ORDERS 24u

typedef struct buddy_heap buddy_heap_t;

struct buddy_heap {
    uint8_t *base;      /* start of the backing region (caller-owned) */
    size_t   total;     /* total bytes; 0 means the heap is unusable */
    unsigned max_order; /* total == BUDDY_MIN_BLOCK << max_order */

    struct buddy_node *free_list[BUDDY_MAX_ORDERS]; /* one list per order */

    /* Statistics, updated on every call. */
    size_t live_bytes;      /* sum of block sizes currently handed out */
    size_t peak_bytes;      /* high-water mark of live_bytes */
    unsigned long n_alloc;  /* successful buddy_alloc calls */
    unsigned long n_free;   /* accepted buddy_free calls */
    unsigned long n_oom;    /* allocations that failed for lack of memory */
};

/* Initialise a heap over `size` bytes starting at `mem`. */
void buddy_init(buddy_heap_t *h, void *mem, size_t size);

/*
 * Allocate `size` payload bytes, 16-byte aligned. The block handed out is
 * the smallest power of two >= size + 16 (minimum 32 bytes). Returns NULL
 * when size == 0, when the request does not fit, or when the heap is
 * unusable/exhausted (n_oom is incremented on exhaustion).
 */
void *buddy_alloc(buddy_heap_t *h, size_t size);

/*
 * Return a block previously obtained from buddy_alloc. The block's order
 * is read back from its header, so no size argument is needed. Passing
 * NULL, a pointer outside the heap, or a pointer that is not exactly a
 * block start + 16 is ignored.
 */
void buddy_free(buddy_heap_t *h, void *ptr);

/* Sum of the sizes of all blocks currently on the free lists. */
size_t buddy_free_bytes(const buddy_heap_t *h);

/* Size of the single largest free block. */
size_t buddy_largest_free(const buddy_heap_t *h);

/*
 * Fragmentation ratio in [0, 1]: the fraction of free space that is NOT
 * in the single largest free block. 0.0 means all free space is one
 * contiguous block; near 1.0 means it is badly scattered. Same definition
 * as lab/02's bump_fragmentation, so the two numbers are comparable.
 */
double buddy_fragmentation(const buddy_heap_t *h);

#endif /* BUDDY_H */
