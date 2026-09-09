/*
 * onesum.h: one's-complement checksum over a byte buffer, the layout used
 * by IPv4 headers.
 *
 * Words are read as 16-bit big-endian values assembled from bytes, so
 * there is no cast and no alignment assumption. Each word is added into
 * a 16-bit accumulator with end-around carry: a carry out of bit 15 is
 * folded back into bit 0 of the same addition, so the accumulator stays
 * 16 bits after every add. A trailing odd byte is padded with a zero
 * byte. An empty buffer sums to 0x0000 and complements to 0xFFFF.
 */
#ifndef ONESUM_H
#define ONESUM_H

#include <stddef.h>
#include <stdint.h>

static unsigned ones_complement_sum(const uint8_t *buf, size_t len)
{
    uint32_t acc = 0;
    size_t i = 0;

    while (i + 1 < len) {
        uint32_t word = ((uint32_t)buf[i] << 8) | buf[i + 1];
        acc += word;
        if (acc > 0xFFFFu)          /* end-around carry: fold carry-out */
            acc = (acc & 0xFFFFu) + 1u;
        i += 2;
    }
    if (i < len) {
        acc += (uint32_t)buf[i] << 8; /* odd byte padded with zero byte */
        if (acc > 0xFFFFu)
            acc = (acc & 0xFFFFu) + 1u;
    }
    return (unsigned)(~acc & 0xFFFFu);
}

#endif /* ONESUM_H */
