#include "minmax.h"

/* Non-inline wrappers so objdump shows the real -O2 codegen of the
 * branchless select. Not part of the shipped module. */
__attribute__((noinline)) int32_t wrap_bmin(int32_t a, int32_t b)
{
    return bmin32(a, b);
}

__attribute__((noinline)) int32_t wrap_bmax(int32_t a, int32_t b)
{
    return bmax32(a, b);
}
