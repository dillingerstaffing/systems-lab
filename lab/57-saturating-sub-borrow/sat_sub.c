/* sat_sub.c - unsigned saturating subtraction built on the borrow-out
 * identity.
 *
 * For unsigned a, b: the subtraction a - b underflows exactly when
 * a < b, so the borrow-out of a - b IS the comparison a < b; there is
 * no carry chain to reason about. Saturating to 0 on borrow therefore
 * reduces to: keep the raw difference where a >= b, force it to 0
 * where a < b.
 *
 * keep = -(uint64_t)(a >= b) is all-ones bits when a >= b and zero
 * otherwise. diff & keep is then the saturated result. Every
 * operation is unsigned, so the wrap in (a - b) is defined by the C
 * standard. No branch, no __int128. */
#include "sat_sub.h"

uint64_t sat_sub64(uint64_t a, uint64_t b)
{
    uint64_t diff = a - b;
    uint64_t keep = -(uint64_t)(a >= b);
    return diff & keep;
}
