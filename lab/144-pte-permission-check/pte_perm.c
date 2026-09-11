#include "pte_perm.h"

int pte_perm_ok(uint8_t flags, perm_access_t access, perm_mode_t mode)
{
    unsigned f = (unsigned)flags & 0x1EU; /* keep only U, X, W, R */
    int r = (int)((f >> 1) & 1U);
    int w = (int)((f >> 2) & 1U);
    int x = (int)((f >> 3) & 1U);
    int u = (int)((f >> 4) & 1U);

    if (w && !r) {
        return 0; /* reserved combination, §4.3.2 step 3: always faults */
    }
    if (u != (int)mode) {
        return 0; /* U bit must match privilege mode (§4.1.1, SUM=0) */
    }
    switch (access) {
    case PERM_READ:
        return r;
    case PERM_WRITE:
        return w;
    case PERM_EXEC:
        return x;
    default:
        return 0;
    }
}
