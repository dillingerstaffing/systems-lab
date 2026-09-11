#ifndef PTE_A_H
#define PTE_A_H

#include <stdint.h>

/*
 * Sv39 PTE A (accessed) bit rule check.
 *
 * Input: the 8 flag bits of a leaf page table entry (bit 0 = V, 1 = R,
 * 2 = W, 3 = X, 4 = U, 5 = G, 6 = A, 7 = D) and an access type
 * (PTE_ACC_READ, PTE_ACC_WRITE, PTE_ACC_EXEC).
 *
 * Output: whether the access faults, and the A bit's value after the
 * access. The modeled rule (RISC-V Privileged ISA Specification,
 * Sv39 addressing and memory protection):
 *
 *   - V = 0: the PTE is invalid, the access faults.
 *   - W = 1 and R = 0: reserved combination, the PTE is invalid,
 *     the access faults.
 *   - Otherwise the access is permitted when its bit is set
 *     (read needs R, write needs W, execute needs X); a denied
 *     access faults.
 *   - On a permitted access the hardware sets A = 1. On a fault the
 *     translation fails and the PTE is left untouched, so A keeps its
 *     input value.
 *
 * U, G, and D are inert inputs for this check: D is set by hardware on
 * writes but never gates an access, and U is a privilege-mode bit the
 * check does not model. The D transition on stores is not part of this
 * module's output; only A is.
 *
 * access must be 0, 1, or 2. Any other value is outside the contract
 * and the result is unspecified.
 */

#define PTE_V (1u << 0)
#define PTE_R (1u << 1)
#define PTE_W (1u << 2)
#define PTE_X (1u << 3)
#define PTE_U (1u << 4)
#define PTE_G (1u << 5)
#define PTE_A (1u << 6)
#define PTE_D (1u << 7)

#define PTE_ACC_READ  0
#define PTE_ACC_WRITE 1
#define PTE_ACC_EXEC  2

typedef struct {
    int fault;   /* 1 if the access faults, 0 if it proceeds */
    int a_after; /* A bit value after the access (0 or 1) */
} pte_a_result;

pte_a_result pte_a_check(uint8_t flags, int access);

#endif
