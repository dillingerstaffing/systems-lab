#include "satshl.h"

/*
 * Saturating 64-bit left shift, built with no data-dependent branches:
 * no if/else, no ternary, no &&/|| short-circuit on the data path. The
 * comparisons below yield 0/1 integers and combine with arithmetic and
 * bitwise ops only; the -O2 disassembly check (see the Makefile `disasm`
 * target) asserts the compiled sat_shl64 contains zero conditional
 * jumps.
 *
 * Construction, for an unsigned shift amount k:
 *
 *   kk = k & 63 keeps every shift amount in 0..63, so no C shift is
 *   ever performed with an amount >= 64 (undefined behavior).
 *   shifted = x << kk is then always a valid shift.
 *
 *   lost = the bits shifted out of the top for k in 1..63, which are
 *   exactly x >> (64 - k). Written as (x >> (63 - kk)) >> 1 so every
 *   shift amount stays in range: for kk == 0 the first shift yields 0
 *   or 1 and the second clears it, so lost is 0 with no branch and no
 *   shift-by-64. Note kk == 0 with k in 1..63 cannot happen.
 *
 *   big = 1 iff k >= 64. For big shifts overflow is decided by
 *   (x != 0) alone: shifting any nonzero value by >= 64 loses every
 *   bit. For k < 64 overflow is decided by (lost != 0). The two cases
 *   are selected with 0/1 masks, not branches:
 *
 *     ovf = (big & (x != 0)) | ((1 - big) & (lost != 0))
 *
 *   The final merge saturates without a branch: where ovf_mask is all
 *   ones, shifted | (MAX ^ shifted) is MAX; where it is zero the
 *   expression is shifted.
 */
uint64_t sat_shl64(uint64_t x, unsigned k)
{
    uint64_t kk = (uint64_t)k & 63u;
    uint64_t shifted = x << kk;

    /* x >> (64-kk) is the bits shifted out; the two-step shift keeps
       every amount in 0..63 and yields 0 when kk == 0. */
    uint64_t lost = (x >> (63u - kk)) >> 1u;

    uint64_t big = (uint64_t)((k >> 6) != 0);      /* 1 iff k >= 64 */
    uint64_t ovf = (big & (uint64_t)(x != 0)) |
                   ((1u - big) & (uint64_t)(lost != 0));

    uint64_t ovf_mask = 0u - ovf;                  /* all ones iff overflow */
    return shifted | (ovf_mask & (UINT64_MAX ^ shifted));
}
