#ifndef PTE_PHYS_H
#define PTE_PHYS_H

#include <stdint.h>

/*
 * Sv39 physical address formation from a leaf PTE's page number.
 *
 * The fundamental layout (RISC-V privileged spec, section 4.3.1,
 * Sv39 PTE: bits 53:10 hold the 44-bit physical page number PPN;
 * the 12-bit page offset comes from the virtual address bits 11:0):
 *
 *   phys_addr = (PPN << 12) | page_offset
 *
 * The PPN occupies physical-address bits 55:12 and the offset
 * occupies bits 11:0, so the result is always narrower than 2^56.
 * Both inputs are AND-masked to their field widths before use, so
 * only the field-width bits of a caller-supplied argument survive.
 *
 * Contract: ppn is taken as a 44-bit value (masked with
 * 0xFFFFFFFFFFF) and offset as a 12-bit value (masked with 0xFFF).
 * Bits above the contract widths are ignored, never read.
 */
uint64_t phys_addr_from_ppn(uint64_t ppn, uint64_t offset);

#endif /* PTE_PHYS_H */
