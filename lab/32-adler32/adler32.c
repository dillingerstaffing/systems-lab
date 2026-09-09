#include "adler32.h"

/* MOD is the largest prime below 65536; the checksum's modulus. */
#define MOD 65521u
/* NMAX: largest block size with no possible 32-bit overflow of the
 * running sums before the per-block reduction. See the header comment
 * for the bound. */
#define NMAX 5552u

uint32_t adler32(const unsigned char *data, size_t len) {
    uint32_t a = 1; /* running sum of bytes, mod 65521 at block ends */
    uint32_t b = 0; /* running sum of a,    mod 65521 at block ends */

    while (len > 0) {
        size_t block = len < NMAX ? len : NMAX;
        len -= block;

        /* Unrolled 16 bytes at a time. Each step applies the recurrence
         * directly: a takes the next byte, b takes the new a. No reduction
         * inside the block; the block bound keeps both sums in 32 bits. */
        while (block >= 16) {
            a += data[0];  b += a;
            a += data[1];  b += a;
            a += data[2];  b += a;
            a += data[3];  b += a;
            a += data[4];  b += a;
            a += data[5];  b += a;
            a += data[6];  b += a;
            a += data[7];  b += a;
            a += data[8];  b += a;
            a += data[9];  b += a;
            a += data[10]; b += a;
            a += data[11]; b += a;
            a += data[12]; b += a;
            a += data[13]; b += a;
            a += data[14]; b += a;
            a += data[15]; b += a;
            data += 16;
            block -= 16;
        }
        while (block > 0) {
            a += *data++;
            b += a;
            block--;
        }

        /* One reduction per block is exact: additions commute with the
         * final modulo, and the block bound guarantees no overflow. */
        a %= MOD;
        b %= MOD;
    }

    return (b << 16) | a;
}
