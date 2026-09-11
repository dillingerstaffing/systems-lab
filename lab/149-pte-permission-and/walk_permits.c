#include <stddef.h>
#include "walk_permits.h"

/* Leaf verdict: the lab/135 rule set, stated here rather than
 * shared, so this module's verification surface is self-contained. */
static int leaf_permits(unsigned r, unsigned w, unsigned x, unsigned u,
                        walk_access_t access, walk_mode_t mode) {
    /* Spec section 4.3.1, Table 115: W=1 requires R=1; the (X,W,R)
     * encodings 010 and 110 fault. */
    if (w && !r)
        return 0;

    /* Spec section 4.3.1; SUM assumed 0: U=0 pages are not for
     * U-mode, U=1 pages are not for S-mode. */
    if (!u && mode == WALK_MODE_U)
        return 0;
    if (u && mode == WALK_MODE_S)
        return 0;

    /* Spec section 4.3.1: the access type needs its grant. */
    switch (access) {
    case WALK_ACCESS_LOAD:
        return (int)r;
    case WALK_ACCESS_STORE:
        return (int)w;
    case WALK_ACCESS_EXEC:
        return (int)x;
    }
    return 0;
}

int walk_permits(const pte_bits_t *levels, int nlevels,
                 walk_access_t access, walk_mode_t mode) {
    int i;

    if (levels == NULL || nlevels <= 0)
        return 0;

    for (i = 0; i < nlevels; i++) {
        unsigned r = levels[i].r & 1u;
        unsigned w = levels[i].w & 1u;
        unsigned x = levels[i].x & 1u;
        unsigned u = levels[i].u & 1u;

        /* Spec section 4.3.2, step 3: r=0 and w=1 at any level
         * stops the walk and raises a page fault. */
        if (w && !r)
            return 0;

        /* Spec section 4.3.2, step 5: r=1 or x=1 ends the walk at
         * a leaf, and the leaf's bits decide the verdict. A level
         * with r=x=0 and w=0 is a pointer to the next level. */
        if (r || x)
            return leaf_permits(r, w, x, u, access, mode);
    }

    /* The levels ran out before any leaf was reached: there is no
     * next table to descend into, so the access faults. */
    return 0;
}
