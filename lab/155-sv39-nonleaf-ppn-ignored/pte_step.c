#include "pte_step.h"

/* Sv39 PTE layout (spec section 4.4.1, figure 65). Bits 4-9 are U, G, A,
 * D, RSW; of those, this step reads D/A/U only for the non-leaf reserved
 * check and never reads G or RSW. */

#define PTE_V (1ULL << 0)
#define PTE_R (1ULL << 1)
#define PTE_W (1ULL << 2)
#define PTE_X (1ULL << 3)

/* D (bit 7), A (bit 6), U (bit 4): reserved on a non-leaf PTE (4.3.1). */
#define PTE_DAU_NONLEAF ((1ULL << 7) | (1ULL << 6) | (1ULL << 4))

#define PTE_PPN_MASK 0x003FFFFFFFFFFC00ULL /* bits 53-10, the 44-bit PPN */
#define PTE_PPN_SHIFT 10
#define PTE_RSVD_HI 0xFFC0000000000000ULL /* bits 63-54, reserved */
#define PTE_PAGE_SHIFT 12                 /* PAGESIZE = 4096 */

pte_step_result_t pte_step(uint64_t pte, int level)
{
    pte_step_result_t r = { PTE_STEP_FAULT, 0, 0 };
    uint64_t ppn;

    /* Spec step 3: invalid, reserved R/W encoding, reserved bits set. */
    if (!(pte & PTE_V))
        return r; /* v=0: all other bits are don't-cares */
    if (!(pte & PTE_R) && (pte & PTE_W))
        return r; /* XWR 010 and 110 are reserved for future use */
    if (pte & PTE_RSVD_HI)
        return r; /* N / PBMT / bits 60-54 set */

    ppn = (pte & PTE_PPN_MASK) >> PTE_PPN_SHIFT;

    /* Spec step 4: leaf test first, as the spec phrases it. */
    if (pte & (PTE_R | PTE_X)) {
        r.verdict = PTE_STEP_LEAF;
        r.leaf_phys = ppn << PTE_PAGE_SHIFT;
        return r;
    }

    /* Valid pointer to the next level. Non-leaf D/A/U are reserved for
     * future standard use (4.3.1), and step 3 faults on such bits set. */
    if (pte & PTE_DAU_NONLEAF)
        return r;
    if (level <= 0)
        return r; /* pointer at the last level: nowhere to descend */

    /* The step reads the PPN here: it is the next-level table pointer. */
    r.verdict = PTE_STEP_DESCEND;
    r.next_ppn = ppn;
    return r;
}
