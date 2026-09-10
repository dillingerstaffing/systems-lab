/* disasm_median3.c — non-inline wrapper so objdump shows the real
 * instruction sequence for med3 (see PROOF.md).
 */
#include <stdint.h>

#include "median3.h"

int64_t med3_wrap(int64_t a, int64_t b, int64_t c)
{
    return med3(a, b, c);
}
