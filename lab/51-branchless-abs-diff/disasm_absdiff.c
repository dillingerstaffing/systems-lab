/* disasm_absdiff.c — non-inline wrapper so objdump shows the real
 * instruction sequence for badiff64 (see PROOF.md).
 */
#include <stdint.h>

#include "absdiff.h"

uint64_t badiff64_wrap(int64_t a, int64_t b)
{
    return badiff64(a, b);
}
