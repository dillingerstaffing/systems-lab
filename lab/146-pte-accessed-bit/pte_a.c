#include "pte_a.h"

/*
 * The check is built directly from the bit identities in the header:
 * each flag bit is extracted once, then the invalid and permission
 * rules are evaluated in the order the spec states them (invalid entry
 * first, reserved combination second, permission of the access type
 * last). No branches are elided and no bit's meaning is reused for
 * another; U, G, and D are read from the byte but never consulted.
 */
pte_a_result pte_a_check(uint8_t flags, int access)
{
    pte_a_result r;
    int v = (flags & PTE_V) != 0;
    int rd = (flags & PTE_R) != 0;
    int wr = (flags & PTE_W) != 0;
    int ex = (flags & PTE_X) != 0;
    int a = (flags & PTE_A) != 0;
    int fault;

    if (!v)
        fault = 1;                    /* invalid entry */
    else if (wr && !rd)
        fault = 1;                    /* reserved: W set while R clear */
    else if (access == PTE_ACC_READ)
        fault = !rd;                  /* read needs R */
    else if (access == PTE_ACC_WRITE)
        fault = !wr;                  /* write needs W */
    else
        fault = !ex;                  /* execute needs X */

    r.fault = fault;
    r.a_after = fault ? a : 1;        /* permitted access sets A,
                                         fault leaves the PTE alone */
    return r;
}
