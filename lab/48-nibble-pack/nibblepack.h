#ifndef NIBBLEPACK_H
#define NIBBLEPACK_H

#include <stdint.h>

/* pack_nibbles16: nibble i (0..15) of n occupies bits [4*i, 4*i+3] of the
 * returned word. Each input is reduced to its low 4 bits before placement,
 * so out-of-range inputs are packed as (input & 0xF). */
static inline uint64_t pack_nibbles16(const uint8_t n[16]) {
    uint64_t w = 0;
    for (int i = 0; i < 16; i++) {
        w |= (uint64_t)(n[i] & 0xFu) << (4 * i);
    }
    return w;
}

/* unpack_nibbles16: inverse of pack_nibbles16.
 * out[i] is the 4-bit value sitting at bits [4*i, 4*i+3] of w. */
static inline void unpack_nibbles16(uint64_t w, uint8_t out[16]) {
    for (int i = 0; i < 16; i++) {
        out[i] = (uint8_t)((w >> (4 * i)) & 0xFu);
    }
}

#endif /* NIBBLEPACK_H */
