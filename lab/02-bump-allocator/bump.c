/*
 * bump.c - Bump allocator with free-list reuse. See bump.h for the contract.
 *
 * No libc dependency: only <stddef.h> and <stdint.h> are used, so this
 * compiles for -ffreestanding targets (RISC-V bare metal included).
 */
#include "bump.h"

struct free_node {
    struct free_node *next;
    size_t span; /* total bytes of this free run, starting at the node */
};

#define BUMP_DEFAULT_ALIGN 8u

static int is_pow2(size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

static uintptr_t align_up(uintptr_t p, size_t align)
{
    uintptr_t mask = (uintptr_t)align - 1;
    return (p + mask) & ~mask;
}

void bump_init(bump_heap_t *h, void *mem, size_t size)
{
    h->base = (uint8_t *)mem;
    h->cap = size;
    h->bump = (uintptr_t)mem;
    h->free_list = NULL;
    h->live_bytes = 0;
    h->peak_bytes = 0;
    h->n_alloc = 0;
    h->n_free = 0;
    h->n_reuse = 0;
    h->n_oom = 0;
}

/*
 * Try to satisfy (size, align) from the free list (first fit, with
 * splitting). Returns NULL if nothing fits. The node is unlinked before
 * its memory is handed out, and any usable remainder becomes a new node.
 */
static void *alloc_from_free_list(bump_heap_t *h, size_t size, size_t align)
{
    struct free_node **link = &h->free_list;

    while (*link) {
        struct free_node *node = *link;
        uintptr_t bstart = (uintptr_t)node;
        uintptr_t pstart = align_up(bstart, align);

        /* pstart + size must not overflow and must fit inside the run. */
        if (pstart >= bstart && size <= node->span &&
            pstart - bstart <= node->span - size) {
            uintptr_t pend = pstart + size;
            uintptr_t bend = bstart + node->span;
            size_t lead = (size_t)(pstart - bstart);
            size_t tail = (size_t)(bend - pend);

            *link = node->next; /* unlink before reuse */

            /*
             * Keep the larger of the lead/tail slivers as a free node.
             * (The smaller sliver is lost as internal fragmentation;
             *  both are < sizeof(struct free_node) or the lead contains
             *  the old node bytes, so only one can be kept safely.)
             */
            if (tail >= sizeof(struct free_node) &&
                tail >= lead) {
                struct free_node *rest = (struct free_node *)pend;
                rest->span = tail;
                rest->next = h->free_list;
                h->free_list = rest;
            } else if (lead >= sizeof(struct free_node)) {
                struct free_node *rest = (struct free_node *)bstart;
                rest->span = lead;
                rest->next = h->free_list;
                h->free_list = rest;
            }

            h->n_reuse++;
            return (void *)pstart;
        }
        link = &(*link)->next;
    }
    return NULL;
}

void *bump_alloc(bump_heap_t *h, size_t size, size_t align)
{
    uintptr_t end = (uintptr_t)h->base + h->cap;
    uintptr_t p;
    void *out;

    if (size == 0)
        return NULL;
    if (align == 0)
        align = BUMP_DEFAULT_ALIGN;
    if (!is_pow2(align))
        return NULL;

    /* Every block must be able to hold a free-list node when freed. */
    if (size < sizeof(struct free_node))
        size = sizeof(struct free_node);

    out = alloc_from_free_list(h, size, align);
    if (out == NULL) {
        p = align_up(h->bump, align);
        if (p < h->bump || p >= end || size > end - p) {
            h->n_oom++;
            return NULL;
        }
        h->bump = p + size;
        out = (void *)p;
    }

    h->n_alloc++;
    h->live_bytes += size;
    if (h->live_bytes > h->peak_bytes)
        h->peak_bytes = h->live_bytes;
    return out;
}

void bump_free(bump_heap_t *h, void *ptr, size_t size)
{
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)h->base;
    struct free_node *node;

    if (ptr == NULL)
        return;
    if (p < start || p >= start + h->cap)
        return; /* not ours; ignore */

    if (size < sizeof(struct free_node))
        size = sizeof(struct free_node);

    node = (struct free_node *)ptr;
    node->span = size;
    node->next = h->free_list;
    h->free_list = node;

    h->n_free++;
    /* live_bytes is informational; guard against caller size mismatch. */
    h->live_bytes -= (size <= h->live_bytes) ? size : h->live_bytes;
}

size_t bump_live_bytes(const bump_heap_t *h)
{
    return h->live_bytes;
}

size_t bump_free_bytes(const bump_heap_t *h)
{
    uintptr_t end = (uintptr_t)h->base + h->cap;
    size_t total = (h->bump <= end) ? (size_t)(end - h->bump) : 0;
    struct free_node *n = h->free_list;

    while (n) {
        total += n->span;
        n = n->next;
    }
    return total;
}

double bump_fragmentation(const bump_heap_t *h)
{
    uintptr_t end = (uintptr_t)h->base + h->cap;
    size_t total = 0;
    size_t largest = 0;
    size_t tail = (h->bump <= end) ? (size_t)(end - h->bump) : 0;
    struct free_node *n = h->free_list;

    if (tail > largest)
        largest = tail;
    total += tail;
    while (n) {
        if (n->span > largest)
            largest = n->span;
        total += n->span;
        n = n->next;
    }
    if (total == 0)
        return 0.0;
    return (double)(total - largest) / (double)total;
}
