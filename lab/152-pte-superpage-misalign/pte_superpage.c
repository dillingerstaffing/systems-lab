#include "pte_superpage.h"

/* The Sv39 PPN field is 44 bits wide (PTE bits 53:10). */
#define PPN_MASK 0xFFFFFFFFFFFULL

int pte_superpage_misaligned(int level, uint64_t ppn)
{
    uint64_t p;

    /* Sv39 page tables have levels 0, 1, and 2 only. */
    if (level < 0 || level > 2)
        return -1;
    /* A level-0 leaf is a 4 KiB page: every PPN bit is significant,
     * so no PPN value can be misaligned. */
    if (level == 0)
        return 0;

    p = ppn & PPN_MASK;
    /*
     * A level-1 leaf is a 2 MiB superpage and a level-2 leaf a 1 GiB
     * superpage. The low 9*level PPN bits index inside the superpage's
     * span, so a set bit there means the PPN names no aligned
     * superpage and the translation must fault.
     */
    if ((p & ((1ULL << (9 * level)) - 1ULL)) != 0)
        return 1;
    return 0;
}
