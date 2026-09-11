#include "oracle_step.h"

/* Explode the PTE into bits[0..63] by repeated division. No shifts, no
 * masks, no bitwise operators: the extraction cannot share a masking bug
 * with the implementation. */
static void pte_to_bits(uint64_t pte, unsigned bits[64])
{
    unsigned i;

    for (i = 0; i < 64; i++) {
        bits[i] = (unsigned)(pte % 2u);
        pte /= 2u;
    }
}

/* Rebuild the 44-bit PPN (spec PTE bits 53-10) by positional
 * accumulation, again with no masks or shifts. */
static uint64_t rebuild_ppn(const unsigned bits[64])
{
    unsigned i;
    uint64_t ppn = 0;
    uint64_t place = 1;

    for (i = 10; i <= 53; i++) {
        if (bits[i])
            ppn += place;
        place *= 2u;
    }
    return ppn;
}

oracle_result_t oracle_step(uint64_t pte, int level)
{
    unsigned bits[64];
    unsigned i;
    uint64_t ppn;
    oracle_result_t r;

    r.verdict = ORACLE_FAULT;
    r.next_ppn = 0;
    r.leaf_phys = 0;

    pte_to_bits(pte, bits);

    /* Spec step 3, in spec order. */
    if (bits[0] == 0)
        return r; /* v=0 */
    if (bits[1] == 0 && bits[2] == 1)
        return r; /* r=0, w=1: reserved encoding */
    for (i = 54; i < 64; i++) {
        if (bits[i])
            return r; /* reserved high bit set */
    }

    /* Spec step 4: the leaf test comes first in the spec's phrasing. */
    if (bits[1] == 1 || bits[3] == 1) {
        ppn = rebuild_ppn(bits);
        r.verdict = ORACLE_LEAF;
        r.leaf_phys = ppn * 4096u;
        return r;
    }

    /* Valid pointer. Non-leaf D (bit 7), A (bit 6), U (bit 4) are
     * reserved for future standard use (4.3.1); step 3 faults on them. */
    if (bits[7] || bits[6] || bits[4])
        return r;
    if (level <= 0)
        return r; /* pointer at the last level: nowhere to descend */

    ppn = rebuild_ppn(bits);
    r.verdict = ORACLE_DESCEND;
    r.next_ppn = ppn;
    return r;
}
