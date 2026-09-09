/* lab/46-fletcher16: Fletcher-16 checksum.
 *
 * The checksum is defined by two running sums over the input bytes.
 * Both sums start at 0; for each byte b:
 *
 *     sum1 = (sum1 + b) % 255
 *     sum2 = (sum2 + sum1) % 255
 *
 * The result is (sum2 << 8) | sum1. That recurrence is the entire
 * definition, so fletcher16() below is the recurrence written as a
 * loop, nothing more. Reduction modulo 255 happens after every byte,
 * exactly as written above; no deferred-reduction trick, no table,
 * no library code.
 *
 * Contract: data may be NULL when len is 0. Otherwise data points to
 * len readable bytes.
 */
#ifndef LAB46_FLETCHER16_H
#define LAB46_FLETCHER16_H

#include <stddef.h>
#include <stdint.h>

static inline uint16_t fletcher16(const unsigned char *data, size_t len)
{
    /* unsigned is 32 bits here: sum1 + b can reach 254 + 255 = 509 and
     * sum2 + sum1 can reach 254 + 254 = 508, so neither add can
     * overflow before the % 255. */
    unsigned sum1 = 0;
    unsigned sum2 = 0;
    for (size_t i = 0; i < len; i++) {
        sum1 = (sum1 + data[i]) % 255u;
        sum2 = (sum2 + sum1) % 255u;
    }
    return (uint16_t)((sum2 << 8) | sum1);
}

#endif /* LAB46_FLETCHER16_H */
