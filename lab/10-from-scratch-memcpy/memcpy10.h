#ifndef MEMCPY10_H
#define MEMCPY10_H

#include <stddef.h>

/*
 * memcpy10: copy n bytes from src to dest, like the standard memcpy.
 *
 * Mechanism, in order:
 *   1. Head: copy single bytes until the destination pointer is aligned
 *      to a machine-word boundary, or n runs out.
 *   2. Body: with the destination word-aligned, copy one machine word
 *      per iteration. The source may still be unaligned, and unaligned
 *      multi-byte loads are not portable C, so each source word is
 *      assembled from single bytes with shifts and ORs. A word-sized
 *      store then moves the bytes in one operation. (On the x86-64 host
 *      the assembled byte order matches memory order, little-endian; the
 *      differential test against libc memcpy below would catch any
 *      ordering error byte for byte.)
 *   3. Tail: copy the remaining (< one word) bytes one at a time.
 *
 * Scope: exactly like libc memcpy, overlapping source and destination
 * regions are undefined behavior and are NOT handled. Use memmove when
 * the regions may overlap.
 *
 * Returns dest.
 */
void *memcpy10(void *dest, const void *src, size_t n);

#endif
