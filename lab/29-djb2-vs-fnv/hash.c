#include "hash.h"

/* djb2: the recurrence h = h * 33 + c, seed 5381, 64-bit unsigned.
 * 33 is Daniel Bernstein's multiplier; the recurrence itself is the
 * whole algorithm. */
uint64_t djb2_64(const unsigned char *data, size_t len)
{
    uint64_t h = 5381u;

    for (size_t i = 0; i < len; i++)
        h = h * 33u + data[i];

    return h;
}

/* FNV-1a: the recurrence h = (h ^ c) * 1099511628211, offset basis
 * 14695981039346656037. The xor-before-multiply ordering is what
 * distinguishes FNV-1a from FNV-1. */
uint64_t fnv1a_64(const unsigned char *data, size_t len)
{
    uint64_t h = 14695981039346656037u;

    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= 1099511628211u;
    }

    return h;
}
