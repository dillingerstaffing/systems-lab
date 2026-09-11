#ifndef PTE_PERM_H
#define PTE_PERM_H

#include <stdint.h>

typedef enum {
    PERM_READ = 0,
    PERM_WRITE = 1,
    PERM_EXEC = 2
} perm_access_t;

typedef enum {
    PERM_SMODE = 0,
    PERM_UMODE = 1
} perm_mode_t;

/*
 * pte_perm_ok: decides whether a leaf Sv39 PTE grants a memory access.
 *
 * flags: raw PTE flag bits in their true bit positions: V=bit0, R=bit1,
 *        W=bit2, X=bit3, U=bit4. Only bits 4:1 are read; every other bit,
 *        including V, is ignored.
 *
 * access: PERM_READ / PERM_WRITE / PERM_EXEC. mode: PERM_SMODE / PERM_UMODE.
 *
 * Returns 1 if the access is permitted, 0 if it faults.
 *
 * Rules applied (RISC-V Privileged Architecture v20211203, SUM=0, MXR=0):
 *   - W=1 and R=0 is reserved and always faults (§4.3.2 step 3).
 *   - read needs R=1, write needs W=1, execute needs X=1 (§4.3.2 step 5).
 *   - the U bit must equal the privilege mode: U=1 pages are accessible
 *     only from U-mode, U=0 pages only from S-mode (§4.1.1, SUM=0).
 *
 * MXR and SUM are out of scope; see PROOF.md.
 */
int pte_perm_ok(uint8_t flags, perm_access_t access, perm_mode_t mode);

#endif
