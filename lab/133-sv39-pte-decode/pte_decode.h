#ifndef PTE_DECODE_H
#define PTE_DECODE_H

#include <stdint.h>

/*
 * Sv39 leaf page-table-entry decoding.
 *
 * The PTE is a fixed bit layout (RISC-V privileged spec, Sv39):
 *   bits 53:10 -> PPN (44 bits, page number)
 *   bits 9:8   -> RSW (2 bits, reserved for supervisor software)
 *   bit 7      -> D (dirty)
 *   bit 6      -> A (accessed)
 *   bit 5      -> G (global)
 *   bit 4      -> U (user)
 *   bit 3      -> X (execute)
 *   bit 2      -> W (write)
 *   bit 1      -> R (read)
 *   bit 0      -> V (valid)
 *
 * The ten single-bit/field masks are disjoint and cover bits 53:0
 * with no overlap and no gaps, so decoding is pure shift/mask and
 * re-encoding the decoded fields restores the word exactly.
 *
 * Contract on the input: pte_decode reads only bits 53:0 of pte.
 * Bits 63:54 are reserved for future use; they are never read, so
 * any value there leaves the decoded fields unchanged.
 * pte_encode rebuilds bits 53:0 from the fields and always leaves
 * bits 63:54 clear; each input field is masked to its width first,
 * so only the field-width bits of a caller-supplied struct survive.
 */
typedef struct {
    uint64_t ppn; /* bits 53:10, stored right-aligned (44 bits) */
    uint8_t rsw;  /* bits 9:8 */
    uint8_t d;    /* bit 7 */
    uint8_t a;    /* bit 6 */
    uint8_t g;    /* bit 5 */
    uint8_t u;    /* bit 4 */
    uint8_t x;    /* bit 3 */
    uint8_t w;    /* bit 2 */
    uint8_t r;    /* bit 1 */
    uint8_t v;    /* bit 0 */
} pte_fields_t;

pte_fields_t pte_decode(uint64_t pte);
uint64_t pte_encode(pte_fields_t f);

#endif /* PTE_DECODE_H */
