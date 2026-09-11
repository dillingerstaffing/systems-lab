#ifndef MUL32_H
#define MUL32_H

#include <stdint.h>

/*
 * mullo32: low 32 bits of the 64-bit product a * b.
 *
 * The implementation uses only shifts, adds, and bit tests: the three
 * contributing 16x16 partial products are built by shift-add, and no
 * multiply operator appears anywhere in mul32.c. No intrinsics, no
 * builtins, no library calls.
 */
uint32_t mullo32(uint64_t a, uint64_t b);

#endif
