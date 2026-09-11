#ifndef WALK_PERMITS_H
#define WALK_PERMITS_H

/*
 * Sv39 multi-level page-table walk permission check.
 *
 * Given one entry per walk level (levels[0] is the level-2 root
 * entry, levels[nlevels-1] the final entry), decide whether an
 * access is legal under the privileged spec's walk rules. The
 * spec decides permission at the single leaf where the walk
 * terminates; non-leaf entries carry no permissions of their own.
 *
 * Rules encoded (RISC-V Instruction Set Manual, Volume II:
 * Privileged Architecture, pinned release
 * riscv-isa-release-dc8bf2a-2026-06-26, section 4.3.2 "Virtual
 * Address Translation Process"):
 *
 * - Step 3: at any level, r=0 and w=1 faults. (A non-leaf entry is
 *   therefore a valid page-table pointer only when
 *   (R,W,X) == (0,0,0); the r=0,w=1 combination is reserved.)
 * - Step 5: a level with r=1 or x=1 ends the walk at a leaf; the
 *   remaining levels are not consulted. The leaf's R/W/X/U bits
 *   then decide the verdict per section 4.3.1 / Table 115: the
 *   (X,W,R) encodings 010 and 110 are reserved and fault, U=0
 *   pages are inaccessible from U-mode, U=1 pages are
 *   inaccessible from S-mode with SUM=0, and load needs R, store
 *   needs W, exec needs X. Section 4.4 gives Sv39 bits 9-0 the
 *   same meaning as Sv32.
 * - If the levels run out before any leaf is reached, the walk
 *   shape is malformed and the access faults (there is no next
 *   table to descend into).
 *
 * The tested rule, exactly: the access is legal iff every level
 * before the terminating one is a pure pointer ((R,W,X)=(0,0,0)),
 * no level holds the reserved r=0,w=1 encoding, and the leaf's
 * permission bits permit the access type from the current mode.
 * Permission is NOT the bitwise AND of the levels' permission
 * bits: non-leaf entries hold no permissions, so there is nothing
 * to AND at those levels.
 *
 * Scope (stated, not assumed): this function assumes V=1 at every
 * level, SUM=0 and MXR=0, and A/D bit handling outside the check
 * (as lab/135). It does not model MPRV, PMP/PMA checks, superpage
 * misalignment, or actual hardware behavior. U bits on non-leaf
 * entries are ignored, matching the spec's leaf-only permission
 * checks.
 */

typedef struct {
    unsigned r; /* bit 1 */
    unsigned w; /* bit 2 */
    unsigned x; /* bit 3 */
    unsigned u; /* bit 4 */
} pte_bits_t;

typedef enum {
    WALK_ACCESS_LOAD = 0,  /* load or load-reserved */
    WALK_ACCESS_STORE = 1, /* store, store-conditional, or AMO */
    WALK_ACCESS_EXEC = 2   /* instruction fetch */
} walk_access_t;

typedef enum {
    WALK_MODE_S = 0, /* supervisor mode */
    WALK_MODE_U = 1  /* user mode */
} walk_mode_t;

/*
 * Returns 1 when the access is legal, 0 when the walk raises a
 * page fault. Each r/w/x/u field is treated as a single bit
 * (masked to 0/1). A NULL levels pointer or nlevels <= 0 faults.
 */
int walk_permits(const pte_bits_t *levels, int nlevels,
                 walk_access_t access, walk_mode_t mode);

#endif /* WALK_PERMITS_H */
