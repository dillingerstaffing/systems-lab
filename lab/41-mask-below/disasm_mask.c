#include <stdint.h>

#include "mask.h"

/* Single call site so objdump shows exactly what mask_below compiles to. */
uint64_t disasm_entry(int n)
{
    return mask_below(n);
}
