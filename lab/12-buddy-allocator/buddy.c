/*
 * buddy.c - Buddy allocator: power-of-two split on alloc, xor-buddy merge
 * on free. See buddy.h for the contract.
 *
 * No libc dependency: only <stddef.h> and <stdint.h> are used, so this
 * compiles for -ffreestanding targets (RISC-V bare metal included).
 */
#include "buddy.h"

struct buddy_node {
    struct buddy_node *next; /* lives at block offset 8, header order at 0 */
};

static int is_pow2(size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

/* Block size for an order: 32 << order, computed in size_t. */
static size_t bsize(unsigned order)
{
    return (size_t)BUDDY_MIN_BLOCK << order;
}

/*
 * Smallest order whose block holds `bytes`. Returns BUDDY_MAX_ORDERS as a
 * sentinel when no supported order is big enough.
 */
static unsigned order_for(size_t bytes)
{
    unsigned o = 0;
    size_t s = BUDDY_MIN_BLOCK;

    while (s < bytes) {
        if (o + 1 >= BUDDY_MAX_ORDERS)
            return BUDDY_MAX_ORDERS;
        s <<= 1;
        o++;
    }
    return o;
}

void buddy_init(buddy_heap_t *h, void *mem, size_t size)
{
    unsigned o;

    h->base = (uint8_t *)mem;
    h->total = 0;
    h->max_order = 0;
    for (o = 0; o < BUDDY_MAX_ORDERS; o++)
        h->free_list[o] = NULL;
    h->live_bytes = 0;
    h->peak_bytes = 0;
    h->n_alloc = 0;
    h->n_free = 0;
    h->n_oom = 0;

    if (size < BUDDY_MIN_BLOCK || !is_pow2(size))
        return; /* unusable heap: allocs will return NULL */
    if (((uintptr_t)mem & (size - 1)) != 0)
        return; /* base must be aligned to the heap size */

    h->total = size;
    h->max_order = order_for(size); /* exact: size is a power of two */
    if (h->max_order >= BUDDY_MAX_ORDERS) {
        h->total = 0; /* larger than the supported orders: unusable */
        return;
    }
    *(size_t *)mem = h->max_order;
    h->free_list[h->max_order] = (struct buddy_node *)
        ((uint8_t *)mem + sizeof(size_t)); /* link field at offset 8 */
    h->free_list[h->max_order]->next = NULL;
}

void *buddy_alloc(buddy_heap_t *h, size_t size)
{
    size_t need;
    unsigned order, o;
    uint8_t *block;
    struct buddy_node *node;

    if (size == 0 || h->total == 0)
        return NULL;
    if (size > (size_t)-1 - BUDDY_HDR_SIZE) {
        h->n_oom++;
        return NULL;
    }
    need = size + BUDDY_HDR_SIZE;
    if (need < BUDDY_MIN_BLOCK)
        need = BUDDY_MIN_BLOCK;

    order = order_for(need);
    if (order > h->max_order) {
        h->n_oom++;
        return NULL;
    }

    o = order;
    while (o <= h->max_order && h->free_list[o] == NULL)
        o++;
    if (o > h->max_order) {
        h->n_oom++;
        return NULL;
    }

    /* Split down to the wanted order; right halves go on the free lists. */
    block = (uint8_t *)h->free_list[o] - sizeof(size_t);
    h->free_list[o] = h->free_list[o]->next;
    while (o > order) {
        uint8_t *right;

        o--;
        right = block + bsize(o);
        *(size_t *)right = o;
        node = (struct buddy_node *)(right + sizeof(size_t));
        node->next = h->free_list[o];
        h->free_list[o] = node;
    }
    *(size_t *)block = order;

    h->n_alloc++;
    h->live_bytes += bsize(order);
    if (h->live_bytes > h->peak_bytes)
        h->peak_bytes = h->live_bytes;
    return block + BUDDY_HDR_SIZE;
}

void buddy_free(buddy_heap_t *h, void *ptr)
{
    uint8_t *b;
    size_t off, blk, orig;
    unsigned o;
    struct buddy_node *node;

    if (ptr == NULL || h->total == 0)
        return;
    b = (uint8_t *)ptr - BUDDY_HDR_SIZE;
    if (b < h->base || b + BUDDY_MIN_BLOCK > h->base + h->total)
        return; /* outside the heap; the header read below is in bounds */
    off = (size_t)(b - h->base);
    if (off % BUDDY_MIN_BLOCK != 0)
        return; /* not a block start */

    o = *(const size_t *)b; /* order recorded by buddy_alloc */
    if (o > h->max_order)
        return;
    blk = bsize(o);
    orig = blk;
    if (off % blk != 0)
        return; /* block start misaligned for its order: not ours */

    /* Merge with the free buddy while one is available. */
    while (o < h->max_order) {
        size_t boff = off ^ blk; /* buddy offset: one xor, the whole trick */
        struct buddy_node **link = &h->free_list[o];
        int found = 0;

        while (*link) {
            if ((uint8_t *)*link - sizeof(size_t) == h->base + boff) {
                *link = (*link)->next;
                found = 1;
                break;
            }
            link = &(*link)->next;
        }
        if (!found)
            break;
        if (boff < off)
            off = boff;
        b = h->base + off;
        o++;
        blk <<= 1;
    }

    *(size_t *)b = o;
    node = (struct buddy_node *)(b + sizeof(size_t));
    node->next = h->free_list[o];
    h->free_list[o] = node;

    h->n_free++;
    /* live_bytes is informational; guard against caller size mismatch. */
    h->live_bytes -= (orig <= h->live_bytes) ? orig : h->live_bytes;
}

size_t buddy_free_bytes(const buddy_heap_t *h)
{
    size_t total = 0;
    unsigned o;

    for (o = 0; o <= h->max_order && o < BUDDY_MAX_ORDERS; o++) {
        struct buddy_node *n = h->free_list[o];

        while (n) {
            total += bsize(o);
            n = n->next;
        }
    }
    return total;
}

size_t buddy_largest_free(const buddy_heap_t *h)
{
    unsigned o;

    /* The largest free block is the highest non-empty order's block. */
    for (o = h->max_order; o < BUDDY_MAX_ORDERS; o--) {
        if (h->free_list[o] != NULL)
            return bsize(o);
        if (o == 0)
            break;
    }
    return 0;
}

double buddy_fragmentation(const buddy_heap_t *h)
{
    size_t total = buddy_free_bytes(h);
    size_t largest = buddy_largest_free(h);

    if (total == 0)
        return 0.0;
    return (double)(total - largest) / (double)total;
}
