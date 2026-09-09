#include <stdint.h>

#include "memcpy10.h"

#define WORD unsigned long
#define WORD_BYTES (sizeof(WORD))
#define WORD_MASK (WORD_BYTES - 1)

void *memcpy10(void *dest, const void *src, size_t n)
{
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;

	/* 1. Head: copy bytes until d sits on a word boundary. */
	while (n > 0 && ((uintptr_t)d & WORD_MASK) != 0) {
		*d++ = *s++;
		n--;
	}

	/* 2. Body: d is word-aligned, so a word store is always aligned.
	 * The source may be unaligned, so never do a multi-byte load
	 * from it directly. Instead assemble each word from single
	 * bytes; that is byte reads only, which are safe at any
	 * alignment on every architecture. */
	if (n >= WORD_BYTES) {
		WORD *dw = (WORD *)d;
		size_t words = n / WORD_BYTES;
		for (size_t i = 0; i < words; i++) {
			WORD w = 0;
			for (size_t b = 0; b < WORD_BYTES; b++)
				w |= (WORD)s[b] << (8 * b);
			dw[i] = w;
			s += WORD_BYTES;
		}
		d = (unsigned char *)(dw + words);
		n -= words * WORD_BYTES;
	}

	/* 3. Tail: fewer than one word left, copy them as bytes. */
	while (n > 0) {
		*d++ = *s++;
		n--;
	}

	return dest;
}
