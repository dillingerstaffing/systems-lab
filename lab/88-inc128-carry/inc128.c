#include "inc128.h"

inc128_res inc128(u128 x)
{
    /* (x.lo + 1) mod 2^64; zero exactly when x.lo was UINT64_MAX. */
    uint64_t lo2 = x.lo + 1;
    uint64_t carry = (lo2 == 0);

    /* x.hi plus the carry; wraps to zero exactly when carry was 1
     * and x.hi was UINT64_MAX. */
    uint64_t hi2 = x.hi + carry;

    inc128_res r;
    r.hi = hi2;
    r.lo = lo2;
    /* 1 exactly when the input was all ones: lo2 == 0 says
     * x.lo == UINT64_MAX (so carry was 1), and hi2 == 0 then says
     * x.hi == UINT64_MAX. */
    r.carry = (int)(lo2 == 0 && hi2 == 0);
    return r;
}
