#ifndef VPN_EXTRACT_H
#define VPN_EXTRACT_H

#include <stdint.h>

/*
 * Sv39 virtual-address field extraction.
 *
 * An Sv39 virtual address is 39 address bits plus a 12-bit page offset:
 *   bits 38:30 -> VPN[2] (9 bits)
 *   bits 29:21 -> VPN[1] (9 bits)
 *   bits 20:12 -> VPN[0] (9 bits)
 *   bits 11:0  -> page offset (12 bits)
 *
 * Contract on the input: vpn_extract reads only bits 38:0 of va.
 * A canonical 64-bit Sv39 address has bit 38 sign-extended into
 * bits 63:39, and a raw 39-bit field has those bits clear or absent;
 * both give identical results because extraction never reads bits
 * above 38. vpn_recombine rebuilds the raw 39-bit field.
 */
typedef struct {
    uint16_t vpn0;
    uint16_t vpn1;
    uint16_t vpn2;
    uint16_t page_offset;
} vpn_fields_t;

vpn_fields_t vpn_extract(uint64_t va);
uint64_t vpn_recombine(vpn_fields_t f);

#endif /* VPN_EXTRACT_H */
