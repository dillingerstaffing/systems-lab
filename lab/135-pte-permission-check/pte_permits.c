#include "pte_permits.h"

int pte_permits(unsigned r, unsigned w, unsigned x, unsigned u,
                pte_access_t access, pte_mode_t mode) {
    r &= 1u;
    w &= 1u;
    x &= 1u;
    u &= 1u;

    /* Rule 1 (spec section 4.3.1, Table 115; section 4.3.2 step 3):
     * W=1 requires R=1. Encodings 010 and 110 fault. */
    if (w && !r)
        return 0;

    /* Rule 2 (spec section 4.3.1; SUM assumed 0, MXR assumed 0):
     * U=0 pages are inaccessible from U-mode; U=1 pages are
     * inaccessible from S-mode when SUM=0. */
    if (!u && mode == PTE_MODE_U)
        return 0;
    if (u && mode == PTE_MODE_S)
        return 0;

    /* Rule 3 (spec section 4.3.1): the access type needs its grant.
     * load needs R, store needs W, exec needs X. */
    switch (access) {
    case PTE_ACCESS_LOAD:
        return (int)r;
    case PTE_ACCESS_STORE:
        return (int)w;
    case PTE_ACCESS_EXEC:
        return (int)x;
    }
    return 0;
}
