#include "pte_global.h"

static uint64_t pte_ppn(uint64_t pte)
{
    return (pte >> PTE_PPN_SHIFT) & PTE_PPN_MASK;
}

walk_result sv39_walk(const pt_mem *mem, satp_t satp, uint64_t va)
{
    walk_result r;
    unsigned cur;
    int level;

    r.fault = 0;
    r.pa = 0;
    r.global = 0;
    r.level = 0;

    cur = satp.root;
    if (cur >= PT_NPAGES) {
        r.fault = 1;
        return r;
    }

    for (level = 2; level >= 0; level--) {
        unsigned idx = (unsigned)((va >> (12 + 9 * level)) & 0x1FFu);
        uint64_t pte = mem->pages[cur].e[idx];

        if (!(pte & PTE_V)) {
            r.fault = 1;
            return r;
        }
        if ((pte & (PTE_R | PTE_W | PTE_X)) == 0) {
            /*
             * Non-leaf: descend to the next level. The G bit on a
             * branch PTE is not consulted; only the leaf PTE's G bit
             * decides whether the translation is global.
             */
            cur = (unsigned)pte_ppn(pte);
            if (cur >= PT_NPAGES) {
                r.fault = 1;
                return r;
            }
            continue;
        }
        /* Leaf. */
        if (((pte & PTE_R) == 0) && (pte & PTE_W)) {
            r.fault = 1; /* reserved: W without R */
            return r;
        }
        {
            uint64_t ppn = pte_ppn(pte);
            unsigned lo_bits = (unsigned)(9 * level);
            uint64_t off_mask;

            /* Superpage misalignment: low PPN bits must be zero. */
            if (lo_bits != 0u &&
                (ppn & ((1ULL << lo_bits) - 1ULL)) != 0u) {
                r.fault = 1;
                return r;
            }
            off_mask = (level == 0)
                ? 0xFFFULL
                : (1ULL << (12 + 9 * level)) - 1ULL;
            r.pa = (ppn << 12) | (va & off_mask);
            r.global = (pte & PTE_G) ? 1 : 0;
            r.level = level;
            return r;
        }
    }
    r.fault = 1;
    return r;
}

void tlb_init(tlb_t *t)
{
    unsigned i;

    for (i = 0; i < TLB_NSLOTS; i++)
        t->slot[i].valid = 0;
    t->next = 0;
}

void tlb_insert(tlb_t *t, uint64_t vpn, uint16_t asid, uint64_t pa,
                int global)
{
    tlb_entry *e = &t->slot[t->next];

    e->valid = 1;
    e->vpn = vpn;
    e->asid = asid;
    e->pa = pa;
    e->global = global ? 1 : 0;
    t->next = (t->next + 1u) % TLB_NSLOTS;
}

lookup_result tlb_lookup(const tlb_t *t, uint64_t vpn, uint16_t asid)
{
    lookup_result r;
    unsigned i;

    r.hit = 0;
    r.pa = 0;
    for (i = 0; i < TLB_NSLOTS; i++) {
        const tlb_entry *e = &t->slot[i];

        if (e->valid && e->vpn == vpn && (e->global || e->asid == asid)) {
            r.hit = 1;
            r.pa = e->pa;
            return r;
        }
    }
    return r;
}
