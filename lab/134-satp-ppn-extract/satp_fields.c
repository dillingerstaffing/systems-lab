#include "satp_fields.h"

/*
 * Width masks for the three RV64 satp fields. Each mask is
 * (1 << width) - 1 for the field's width from the spec layout:
 * MODE 4 bits, ASID 16 bits, PPN 44 bits.
 */
#define SATP_MODE_MASK 0xFULL          /* 4 bits */
#define SATP_ASID_MASK 0xFFFFULL      /* 16 bits */
#define SATP_PPN_MASK  0xFFFFFFFFFFFULL /* 44 bits */

satp_fields_t satp_decode(uint64_t satp) {
    satp_fields_t f;
    f.mode = (uint8_t)((satp >> 60) & SATP_MODE_MASK);
    f.asid = (uint16_t)((satp >> 44) & SATP_ASID_MASK);
    f.ppn  = satp & SATP_PPN_MASK;
    return f;
}

uint64_t satp_recombine(satp_fields_t f) {
    return (((uint64_t)(f.mode & (uint8_t)SATP_MODE_MASK)) << 60) |
           (((uint64_t)(f.asid & (uint16_t)SATP_ASID_MASK)) << 44) |
           (f.ppn & SATP_PPN_MASK);
}
