#include "pte_ad.h"

/* Bit positions in an Sv39 PTE, per the pinned privileged spec. */
#define PTE_V 0
#define PTE_R 1
#define PTE_W 2
#define PTE_X 3
#define PTE_U 4
#define PTE_A 6
#define PTE_D 7

static unsigned bit(uint64_t pte, int b)
{
    return (unsigned)((pte >> b) & 1ULL);
}

/* True when the entry terminates the walk as a leaf. */
static int is_leaf(uint64_t pte)
{
    return bit(pte, PTE_V) && (bit(pte, PTE_R) || bit(pte, PTE_X));
}

static unsigned vpn_index(uint64_t va, int level)
{
    return (unsigned)((va >> (12 + 9 * level)) & 0x1FFULL);
}

ad_result_t ad_predict(pte_tree_t *t, uint64_t va,
                       ad_access_t access, ad_mode_t mode)
{
    ad_result_t r = {0, {0, 0, 0, 0}, 0, 0};
    unsigned path[3] = {0, 0, 0}; /* vpn indices consumed, by level */
    int ni = 0;                   /* current node index */
    int level = 2;

    for (;;) {
        if (ni < 0 || ni >= t->nnodes) {
            r.fault = 1;
            return r;
        }
        unsigned idx = vpn_index(va, level);
        path[level] = idx;
        uint64_t pte = t->nodes[ni].e[idx];

        /* Step 3: v=0, or reserved r=0,w=1, faults at any level. */
        if (!bit(pte, PTE_V) || (!bit(pte, PTE_R) && bit(pte, PTE_W))) {
            r.fault = 1;
            return r;
        }

        /* Step 4: a leaf ends the walk; a pure pointer descends. */
        if (is_leaf(pte)) {
            uint64_t ppn = pte >> 10;

            /* Step 5: misaligned superpage faults. */
            if (level > 0 && (ppn & ((1ULL << (9 * level)) - 1ULL)) != 0) {
                r.fault = 1;
                return r;
            }

            /* Step 6: U-bit check with SUM=0. */
            unsigned u = bit(pte, PTE_U);
            if ((mode == AD_MODE_U && !u) || (mode == AD_MODE_S && u)) {
                r.fault = 1;
                return r;
            }

            /* Step 8: reserved (X,W,R) encodings and R/W/X checks
             * (step 7's Shadow Stack rules are out of scope). */
            unsigned rwxxwr = (bit(pte, PTE_X) << 2) |
                              (bit(pte, PTE_W) << 1) | bit(pte, PTE_R);
            if (rwxxwr == 2 || rwxxwr == 6) { /* 010, 110 reserved */
                r.fault = 1;
                return r;
            }
            int ok = 0;
            if (access == AD_ACCESS_LOAD)
                ok = bit(pte, PTE_R);
            else if (access == AD_ACCESS_STORE)
                ok = bit(pte, PTE_W);
            else
                ok = bit(pte, PTE_X);
            if (!ok) {
                r.fault = 1;
                return r;
            }

            /* Step 9, hardware-update scheme: the completed access
             * sets A on the terminating leaf PTE, and a completed
             * store also sets D on it. Non-leaf PTEs are never
             * touched (their A/D bits are reserved). */
            r.leaf.level = (unsigned)level;
            r.leaf.i2 = path[2];
            r.leaf.i1 = path[1];
            r.leaf.i0 = path[0];
            if (!bit(pte, PTE_A)) {
                t->nodes[ni].e[idx] = pte | (1ULL << PTE_A);
                pte |= (1ULL << PTE_A);
                r.a_gained = 1;
            }
            if (access == AD_ACCESS_STORE && !bit(pte, PTE_D)) {
                t->nodes[ni].e[idx] = pte | (1ULL << PTE_D);
                r.d_gained = 1;
            }
            return r;
        }

        /* The step-3 filter above removed v=0 and the reserved
         * r=0,w=1 encoding, so a valid entry that is not a leaf is
         * necessarily a pure pointer ((R,W,X) == (0,0,0)): descend. */
        if (level == 0) {
            r.fault = 1; /* pointer chain ran out of levels */
            return r;
        }
        ni = t->nodes[ni].child[idx];
        level--;
    }
}
