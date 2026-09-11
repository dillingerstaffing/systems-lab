#ifndef PTE_AD_H
#define PTE_AD_H

#include <stdint.h>

/*
 * Sv39 walk-level accessed/dirty prediction.
 *
 * ad_predict(tree, va, access, mode) walks a scripted three-level
 * page-table tree for one access and predicts exactly which PTE gains
 * the A bit and which gains the D bit, mutating the tree the way the
 * hardware update scheme would.
 *
 * Rules encoded (RISC-V Instruction Set Manual, Volume II: Privileged
 * Architecture, pinned release riscv-isa-release-dc8bf2a-2026-06-26):
 *
 * - Section 4.4.1: the Sv39 PTE format; bits 9-0 have the same meaning
 *   as for Sv32; any level of PTE may be a leaf (4 KiB page, 2 MiB
 *   megapage, or 1 GiB gigapage).
 * - Section 4.3.2, steps 2-4: the walk descends while entries are
 *   valid pure pointers ((R,W,X) == (0,0,0)); v=0 or the reserved
 *   r=0,w=1 encoding faults at any level; the first entry with r=1 or
 *   x=1 terminates the walk as the leaf.
 * - Section 4.3.2, steps 5, 6, and 8: a misaligned superpage (level > 0
 *   with nonzero low PPN bits) faults; the leaf's U bit is checked
 *   against the mode (SUM=0 assumed); the leaf's R/W/X bits are
 *   checked against the access type (load needs R, store needs W,
 *   exec needs X); the (X,W,R) encodings 010 and 110 are reserved and
 *   fault. (Step 7's Shadow Stack Memory Protection rules are outside
 *   this module's scope and are not modeled.)
 * - Section 4.3.2, step 9 (hardware-update scheme, Svade not
 *   implemented): when the access completes and the leaf PTE's A bit
 *   is clear, the PTE is updated to set A; when a store completes and
 *   the leaf's D bit is clear, the PTE is updated to set D. The update
 *   targets the terminating leaf PTE only.
 * - Section 4.3.1: "For non-leaf PTEs, the D, A, and U bits are
 *   reserved for future standard use. Until their use is defined by a
 *   standard extension, they must be cleared by software for forward
 *   compatibility." Hardware never sets A or D on a non-leaf PTE, so
 *   the prediction touches only the leaf.
 *
 * The backlog gloss for this item said A is set on every PTE an access
 * traverses, including non-leaf levels. That is not what the pinned
 * spec says (same pattern as lab/149, where the backlog's "AND of the
 * leaf PTE bits" gloss was corrected against the spec): the update in
 * step 9 applies to the terminating leaf PTE, and non-leaf A/D bits
 * are reserved. This module implements and states the rule as the
 * spec actually describes it.
 *
 * Scope (stated, not assumed): SUM=0, MXR=0, no Svade (hardware-update
 * scheme only), no MPRV, no PMP/PMA checks, no speculative A updates,
 * no TLB caching effects, inputs are canonical Sv39 virtual
 * addresses. A faulting walk touches no PTE. A and D bits are never
 * cleared by the update.
 */

typedef enum {
    AD_ACCESS_LOAD = 0,  /* load or load-reserved */
    AD_ACCESS_STORE = 1, /* store, store-conditional, or AMO */
    AD_ACCESS_EXEC = 2   /* instruction fetch */
} ad_access_t;

typedef enum {
    AD_MODE_S = 0, /* supervisor mode */
    AD_MODE_U = 1  /* user mode */
} ad_mode_t;

/* Identity of one PTE inside the scripted tree: the walk level and
 * the vpn indices consumed to reach it. Unused index fields are 0. */
typedef struct {
    unsigned level; /* 2, 1, or 0 */
    unsigned i2;    /* vpn[2] index at level 2 */
    unsigned i1;    /* vpn[1] index at level 1 */
    unsigned i0;    /* vpn[0] index at level 0 */
} pte_id_t;

/* One 512-entry page-table node. child[k] >= 0 names the node index
 * in tree->nodes that entry k points to when entry k is a non-leaf
 * pointer; child[k] == -1 otherwise (leaf or invalid entry). */
typedef struct {
    uint64_t e[512];
    int child[512];
} pte_node_t;

typedef struct {
    pte_node_t *nodes;
    int nnodes;
} pte_tree_t;

typedef struct {
    int fault;      /* 1 when the walk raised a page fault */
    pte_id_t leaf;  /* terminating leaf; meaningful only when fault == 0 */
    int a_gained;   /* leaf A transitioned 0 -> 1 on this access */
    int d_gained;   /* leaf D transitioned 0 -> 1 on this access */
} ad_result_t;

/*
 * Walk tree for one access of type access from mode at va, apply the
 * spec's A/D hardware-update rule to the terminating leaf, and report
 * the outcome. Returns fault=1 (touching no PTE) when any walk check
 * fails; otherwise returns the leaf identity and which of A/D went
 * 0 -> 1 on this access.
 */
ad_result_t ad_predict(pte_tree_t *t, uint64_t va,
                       ad_access_t access, ad_mode_t mode);

#endif /* PTE_AD_H */
