#include <stdint.h>

#include "signextend.h"

/*
 * Non-inline wrapper so objdump shows the exact instruction sequence
 * the compiler emits for the shift identity.
 */
int64_t sign_extend_wrap(uint64_t x, int w)
{
    return sign_extend64(x, w);
}
