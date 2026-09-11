#ifndef PTE_STEP_H
#define PTE_STEP_H

#include <stdint.h>

/*
 * Single-step Sv39 page-table-walk classification.
 *
 * pte_step(pte, level) classifies one 64-bit Sv39 PTE at the given walk
 * level (2, 1, or 0) as FAULT, DESCEND, or LEAF, following the privileged
 * spec's walk algorithm exactly (pinned release
 * riscv-isa-release-dc8bf2a-2026-06-26, section 4.3.2 steps 3 and 4; Sv39
 * reuses that algorithm with LEVELS=3 and PTESIZE=8, per section 4.4.1):
 *
 * - Step 3: v=0, or the reserved r=0,w=1 encoding, or any reserved-for-
 *   future-standard-use bit set, faults the walk.
 * - Step 4: a valid PTE with r=1 or x=1 is a leaf; otherwise it is a
 *   pointer to the next level, and the walk continues at pte.ppn*PAGESIZE
 *   (a pointer at level 0 faults, since there is no level below it).
 *
 * Which bits the step reads (derived from the spec text, not assumed):
 *
 * - V (bit 0), R/W/X (bits 1-3): read by steps 3 and 4 for the verdict.
 * - PPN (bits 53-10): read on every descend (it is the next-level table
 *   pointer) and on every leaf (it forms the physical address).
 * - Reserved high bits (bits 63-54: N, PBMT, and 60-54): fault when set
 *   (section 4.4.1; the scope here is Svnapot and Svpbmt not implemented,
 *   so bits 63-61 stay reserved).
 * - D/A/U (bits 7, 6, 4) on a non-leaf PTE: section 4.3.1 says "For
 *   non-leaf PTEs, the D, A, and U bits are reserved for future standard
 *   use", and step 3 faults when any reserved-for-future-standard-use bit
 *   is set within the PTE. A non-leaf PTE with D, A, or U set therefore
 *   faults. (QEMU 8.2.2 implements exactly this on its inner-PTE path.)
 * - G (bit 5): has a defined meaning at non-leaf ("the global setting
 *   implies that all mappings in the subsequent levels of the page table
 *   are global", section 4.3.1) but no walk step reads it for the verdict.
 * - RSW (bits 9-8): "reserved for use by supervisor software; the
 *   implementation shall ignore this field" (section 4.3.1).
 *
 * So the step's verdict is a function of V, R, W, X, D, A, U, the reserved
 * high bits, PPN, and the level, and never of G or RSW.
 *
 * Premise correction (the backlog item this lab implements): the item's
 * gloss claimed the walker ignores a non-leaf PTE's PPN. That is not what
 * the spec says: step 4 reads pte.ppn on every descend ("let
 * a=pte.ppn*PAGESIZE and go to step 2"); the PPN is the next-level pointer
 * and cannot be ignored. The gloss also suggested D/A/U are the ignored
 * bits; they are not (see above: a non-leaf PTE with D, A, or U set
 * faults). The bits the step genuinely never reads for its verdict are G
 * and RSW. This module implements the spec's rule, and the differential
 * test proves the G/RSW invariance over the full cross product below.
 *
 * Scope: walk-step classification only. Leaf permission and attribute
 * checks (steps 5-9: superpage alignment, U/SUM, R/W/X against the access
 * type, A/D update) need the access type and privilege mode and are
 * covered by sibling labs, not here. PMP/PMA checks are out of scope.
 */

typedef enum {
    PTE_STEP_FAULT = 0,   /* walk stops with a page fault */
    PTE_STEP_DESCEND = 1, /* valid pointer: continue at next_ppn*4096 */
    PTE_STEP_LEAF = 2     /* terminal PTE: page base at leaf_phys */
} pte_step_verdict_t;

typedef struct {
    pte_step_verdict_t verdict;
    uint64_t next_ppn;  /* DESCEND: the 44-bit PPN of the next-level table */
    uint64_t leaf_phys; /* LEAF: physical page base, PPN*4096, offset zero */
} pte_step_result_t;

/*
 * Classify one Sv39 PTE at walk level 2, 1, or 0.
 * Contract: level is 0, 1, or 2. Only the field named for the returned
 * verdict is meaningful (next_ppn for DESCEND, leaf_phys for LEAF).
 */
pte_step_result_t pte_step(uint64_t pte, int level);

#endif /* PTE_STEP_H */
