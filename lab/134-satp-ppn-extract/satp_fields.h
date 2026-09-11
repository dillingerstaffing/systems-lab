#ifndef SATP_FIELDS_H
#define SATP_FIELDS_H

#include <stdint.h>

/*
 * RV64 satp CSR field extraction.
 *
 * The satp register is a fixed bit layout (RISC-V Privileged
 * Architectures, satp register section: RV64 satp packs the fields as
 * shown in Figure 4.14 of v20211203):
 *   bits 63:60 -> MODE (4 bits, address-translation scheme selector)
 *   bits 59:44 -> ASID (16 bits, address-space identifier)
 *   bits 43:0  -> PPN  (44 bits, physical page number of the root
 *                       page table, i.e. its physical address >> 12)
 *
 * The three masks are disjoint and cover all 64 bits with no overlap
 * and no gaps, so decoding is pure shift/mask and re-encoding the
 * decoded fields restores the word exactly:
 * recombine(decode(satp)) == satp for every 64-bit value.
 *
 * MODE encodings named by the spec (same section): 0 = Bare,
 * 8 = Sv39, 9 = Sv48, 10 = Sv57, 11 = Sv64; other values reserved.
 * The extractor does not interpret MODE, it reports the raw 4 bits.
 *
 * satp_recombine masks each input field to its width first, so only
 * the field-width bits of a caller-supplied struct survive; feeding
 * it the output of satp_decode always reproduces the input word.
 */
typedef struct {
    uint8_t mode;  /* bits 63:60, stored right-aligned (4 bits) */
    uint16_t asid; /* bits 59:44, stored right-aligned (16 bits) */
    uint64_t ppn;  /* bits 43:0, stored right-aligned (44 bits) */
} satp_fields_t;

satp_fields_t satp_decode(uint64_t satp);
uint64_t satp_recombine(satp_fields_t f);

#endif /* SATP_FIELDS_H */
