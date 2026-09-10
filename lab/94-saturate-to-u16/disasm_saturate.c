#include "saturate_u16.h"

/* Non-inline wrapper so objdump shows the real -O2 codegen of the
 * branchless clamp. Not part of the shipped module. */
__attribute__((noinline)) uint16_t wrap_saturate_u16(int32_t x)
{
    return saturate_u16(x);
}
