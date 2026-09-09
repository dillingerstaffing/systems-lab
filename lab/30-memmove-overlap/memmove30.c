#include "memmove30.h"

void *my_memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d == s || n == 0)
        return dest;

    /* A forward copy reads s[i] before any write lands on or past it
       only while no destination byte sits at a not-yet-read source
       address. When d < s, every write d[i] hits an address at or below
       s[i], so each source byte is read before the copy can overwrite
       it: forward is correct. When d > s, the write d[i] = s[i] would
       hit s[i + (d - s)] before that higher source byte is read, so
       forward corrupts; instead copy backward from the top down, so
       each read s[i-1] happens before any write can reach that address:
       backward is correct. When the regions are disjoint, both orders
       read every source byte before any write touches it, so forward
       (the d <= s branch) is correct too. */
    if (d < s) {
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--)
            d[i - 1] = s[i - 1];
    }

    return dest;
}
