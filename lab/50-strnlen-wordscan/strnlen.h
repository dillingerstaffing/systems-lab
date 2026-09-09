#ifndef STRNLEN_H
#define STRNLEN_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* my_strnlen(s, max): number of bytes from s up to the first zero byte,
 * or max bytes, whichever comes first.
 *
 * Scan order; every read stays inside [s, s + max):
 *   1. Byte head: compare single bytes while s is not word aligned and
 *      bytes remain. Each compare reads exactly one byte.
 *   2. Word body: while at least one whole word fits inside the bound,
 *      load it with memcpy into a local (gcc emits a single aligned
 *      load) and apply the zero-byte identity below. Each load reads
 *      exactly sizeof(size_t) bytes, all inside the range.
 *   3. Byte tail: compare the remaining (fewer than one word) bytes
 *      one at a time.
 * If max is 0 the function returns 0 without touching s at all.
 *
 * Zero-byte identity: with ONES = 0x0101..01 and HIGH = 0x8080..80,
 *   haszero(w) = ((w - ONES) & ~w) & HIGH.
 * A zero byte b_i = 0 always sets its own 0x80 bit: the subtraction
 * computes byte i as 0 - 1 - borrow_in (borrow_in is 0 or 1), which is
 * 0xFF or 0xFE, both with the high bit set; ~w has byte i = 0xFF, so
 * the AND keeps 0x80. A borrow caused by a lower zero byte can also
 * set higher bytes' bits, so a nonzero result only means "re-scan this
 * word byte by byte": the exact position comes from the byte scan in
 * step 2, never from the word test. Therefore the word test is zero
 * exactly when the word holds no zero byte, which is all the loop
 * needs. The subtraction is unsigned, so it is well defined modulo
 * 2^wordbits.
 */
static inline size_t my_strnlen(const char *s, size_t max)
{
    enum { WS = (int)sizeof(size_t) };
    const size_t ONES = ~(size_t)0 / (size_t)0xFF; /* 0x0101..01 */
    const size_t HIGH = ONES << 7;                /* 0x8080..80 */
    const unsigned char *p = (const unsigned char *)s;
    size_t n = 0;

    _Static_assert(WS > 0 && (WS & (WS - 1)) == 0,
                   "word size must be a power of two");

    if (max == 0) {
        return 0;
    }

    /* Byte head: stop at word alignment or when max is spent. */
    while (max > 0 && ((uintptr_t)p & (size_t)(WS - 1)) != 0) {
        if (*p == 0) {
            return n;
        }
        p++;
        n++;
        max--;
    }

    /* Word body: each iteration consumes a full word inside the bound. */
    while (max >= (size_t)WS) {
        size_t w;
        memcpy(&w, p, WS);
        if ((((w - ONES) & ~w) & HIGH) != 0) {
            /* A zero byte is somewhere in this word; find it exactly. */
            for (int i = 0; i < WS; i++) {
                if (p[i] == 0) {
                    return n + (size_t)i;
                }
            }
        }
        p += WS;
        n += (size_t)WS;
        max -= (size_t)WS;
    }

    /* Byte tail: fewer than one word remains. */
    while (max > 0) {
        if (*p == 0) {
            return n;
        }
        p++;
        n++;
        max--;
    }

    return n;
}

#endif /* STRNLEN_H */
