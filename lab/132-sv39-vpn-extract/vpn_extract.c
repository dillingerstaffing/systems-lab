#include "vpn_extract.h"

vpn_fields_t vpn_extract(uint64_t va) {
    vpn_fields_t f;
    f.page_offset = (uint16_t)(va & 0xFFFULL);
    f.vpn0 = (uint16_t)((va >> 12) & 0x1FFULL);
    f.vpn1 = (uint16_t)((va >> 21) & 0x1FFULL);
    f.vpn2 = (uint16_t)((va >> 30) & 0x1FFULL);
    return f;
}

uint64_t vpn_recombine(vpn_fields_t f) {
    return ((uint64_t)f.vpn2 << 30) |
           ((uint64_t)f.vpn1 << 21) |
           ((uint64_t)f.vpn0 << 12) |
           (uint64_t)f.page_offset;
}
