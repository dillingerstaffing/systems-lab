#ifndef PTE_SUPERPAGE_H
#define PTE_SUPERPAGE_H

#include <stdint.h>

/*
 * Sv39 superpage misalignment check.
 *
 * A leaf PTE found at translation level `level` maps a page whose size
 * depends on the level: level 0 -> 4 KiB, level 1 -> 2 MiB superpage,
 * level 2 -> 1 GiB superpage. For a superpage (level 1 or 2), the low
 * 9*level bits of the 44-bit PPN field must be zero; if any of them is
 * set, the address translation raises a page fault. A level-0 leaf has
 * no such constraint: all 44 PPN bits are significant for a 4 KiB page.
 *
 * pte_superpage_misaligned(level, ppn):
 *   returns  1 if the (level, ppn) pair must raise a page fault
 *              (level is 1 or 2 and a low PPN bit is set),
 *   returns  0 if it must not (an aligned superpage, or any level-0 page),
 *   returns -1 if `level` is outside {0, 1, 2}. Out-of-domain input is
 *              reported explicitly and never silently accepted as a
 *              translation verdict.
 *
 * Only the low 44 bits of `ppn` are the PPN field; higher bits are
 * ignored.
 */
int pte_superpage_misaligned(int level, uint64_t ppn);

#endif /* PTE_SUPERPAGE_H */
