#ifndef PTE_PERMITS_H
#define PTE_PERMITS_H

/*
 * Sv39 leaf-PTE access permission check.
 *
 * Given a decoded PTE's R, W, X, U bits (bits 1, 2, 3, 4 of the PTE),
 * the access type, and the current privilege mode, decide whether the
 * access is legal under the RISC-V privileged specification's page
 * permission rules.
 *
 * Rules encoded (RISC-V Instruction Set Manual, Volume II: Privileged
 * Architecture, pinned release riscv-isa-release-dc8bf2a-2026-06-26,
 * riscv-spec.pdf):
 *
 * 1. Reserved R/W/X encodings: writable pages must also be marked
 *    readable, so W=1 requires R=1; the (X,W,R) encodings 010 and 110
 *    are reserved for future use and raise a page fault on access
 *    (section 4.3.1 "Addressing and Memory Protection", Table 115;
 *    section 4.3.2 "Virtual Address Translation Process", step 3:
 *    "if pte.r=0 and pte.w=1 ... stop and raise a page-fault
 *    exception"). Sv39 bits 9:0 carry the same meaning as Sv32
 *    (section 4.4).
 *
 * 2. U bit vs privilege mode (section 4.3.1): U-mode software may only
 *    access the page when U=1; supervisor software with the SUM bit
 *    clear faults on accesses to U=1 pages. This module assumes SUM=0
 *    and MXR=0. Under that assumption, a U=0 page is inaccessible from
 *    U-mode and a U=1 page is inaccessible from S-mode. (The spec adds
 *    that supervisor may not execute code on U=1 pages irrespective of
 *    SUM; under the SUM=0 assumption that rule is already covered by
 *    the blanket S-mode/U=1 fault.)
 *
 * 3. Access type vs grants (section 4.3.1): a load (or load-reserved)
 *    to a page without read permission raises a load page fault, so
 *    loads need R=1; a store (or store-conditional, AMO) to a page
 *    without write permission raises a store page fault, so stores need
 *    W=1; an instruction fetch from a page without execute permission
 *    raises a fetch page fault, so exec needs X=1.
 *
 * An (R,W,X) of 000 makes the PTE a pointer to the next level of the
 * page table, not a leaf (section 4.3.1). The grant rules above give
 * every access on such an entry the illegal verdict, which matches
 * what the leaf permission check would conclude: the entry grants
 * nothing. Modeling the page-table descent itself is out of scope.
 *
 * Scope (stated, not assumed): this function checks exactly one leaf
 * PTE's permission bits. It does not model MXR, SUM=1, MPRV, the V
 * bit, A/D bit handling, PMP/PMA checks, superpage misalignment, or
 * the multi-level walk. Each of those is a separate mechanism with
 * its own verification surface.
 */

typedef enum {
    PTE_ACCESS_LOAD = 0,  /* load or load-reserved */
    PTE_ACCESS_STORE = 1, /* store, store-conditional, or AMO */
    PTE_ACCESS_EXEC = 2   /* instruction fetch */
} pte_access_t;

typedef enum {
    PTE_MODE_S = 0, /* supervisor mode */
    PTE_MODE_U = 1  /* user mode */
} pte_mode_t;

/*
 * Returns 1 when the access is legal, 0 when it faults. Each of
 * r, w, x, u is treated as a single bit (masked to 0/1).
 */
int pte_permits(unsigned r, unsigned w, unsigned x, unsigned u,
                pte_access_t access, pte_mode_t mode);

#endif /* PTE_PERMITS_H */
