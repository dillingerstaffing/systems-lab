#ifndef ADLER32_H
#define ADLER32_H

#include <stddef.h>
#include <stdint.h>

/*
 * adler32: Adler-32 rolling checksum over `len` bytes at `data`.
 *
 * The value computed is exactly the recurrence from RFC 1950, section 8.2:
 *   A = 1 + sum(data[i])              mod 65521
 *   B = sum of the running A values   mod 65521
 *   result = (B << 16) | A
 *
 * The implementation adds bytes into the two running sums in blocks of at
 * most NMAX = 5552 bytes and reduces modulo 65521 only at block ends.
 * 5552 is the largest block size for which 32-bit accumulators cannot
 * overflow: after n bytes, B <= 65520 + n*65520 + 255*n*(n+1)/2, which for
 * n = 5552 equals 4294769700 < 2^32. Reducing after pure addition gives the
 * same value as reducing per byte, so the result matches the recurrence.
 */
uint32_t adler32(const unsigned char *data, size_t len);

#endif
