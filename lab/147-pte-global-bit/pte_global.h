#ifndef PTE_GLOBAL_H
#define PTE_GLOBAL_H

#include <stdint.h>

/*
 * Sv39 page-table walk with G-bit (global mapping) handling, plus a
 * small ASID-tagged translation cache modeling the TLB rule for
 * global mappings.
 *
 * Bit identities used (RISC-V privileged architecture, Sv39):
 *   PTE flag bits: V=0, R=1, W=2, X=3, U=4, G=5, A=6, D=7.
 *   PPN field: bits 53:10 (44 bits).
 *   Sv39 has three levels; VPN[2] = va[38:30], VPN[1] = va[29:21],
 *   VPN[0] = va[20:12]; each level indexes 512 entries.
 *   satp carries MODE in bits 63:60 (8 = Sv39), the ASID in bits
 *   59:44, and the root page-table PPN in bits 43:0.
 *
 * The G-bit rule: a leaf PTE with G set marks its translation global,
 * so the cached translation applies across address spaces without
 * consulting the ASID. A leaf with G clear is ASID-scoped: the cached
 * translation applies only when the current ASID matches the ASID it
 * was cached under.
 *
 * Scope notes (deliberate, see PROOF.md): the walker models validity
 * (V), leaf detection (R/W/X), the reserved W-without-R combination,
 * and superpage misalignment faults. It does not model permission
 * faults or A/D-bit faults. The G bit is read from the leaf PTE only;
 * G set on a non-leaf (branch) PTE does not mark the translation
 * global. Physical memory is a flat array of pages; a page's PPN is
 * its index in the array.
 */

#define PTE_V (1ULL << 0)
#define PTE_R (1ULL << 1)
#define PTE_W (1ULL << 2)
#define PTE_X (1ULL << 3)
#define PTE_U (1ULL << 4)
#define PTE_G (1ULL << 5)
#define PTE_A (1ULL << 6)
#define PTE_D (1ULL << 7)

#define PTE_PPN_SHIFT 10
#define PTE_PPN_MASK ((1ULL << 44) - 1ULL) /* 44-bit PPN field, bits 53:10 */

typedef struct {
    uint64_t e[512];
} pt_page;

#define PT_NPAGES 64

typedef struct {
    pt_page pages[PT_NPAGES];
} pt_mem;

/* Minimal satp: ASID (bits 59:44) and root page index (PPN). */
typedef struct {
    uint16_t asid;
    unsigned root;
} satp_t;

typedef struct {
    int fault;
    uint64_t pa;   /* translated physical address */
    int global;    /* G bit of the leaf PTE */
    int level;     /* 2 = 1 GiB, 1 = 2 MiB, 0 = 4 KiB */
} walk_result;

walk_result sv39_walk(const pt_mem *mem, satp_t satp, uint64_t va);

/*
 * Translation cache. Keyed by 4 KiB VPN; each entry carries the ASID
 * it was cached under and the leaf's G bit. Lookup hits when the VPN
 * matches and (the entry is global, in which case the ASID is not
 * consulted, or the ASIDs match).
 */
#define TLB_NSLOTS 256

typedef struct {
    int valid;
    uint64_t vpn;
    uint16_t asid;
    uint64_t pa;
    int global;
} tlb_entry;

typedef struct {
    tlb_entry slot[TLB_NSLOTS];
    unsigned next; /* FIFO insertion cursor */
} tlb_t;

typedef struct {
    int hit;
    uint64_t pa;
} lookup_result;

void tlb_init(tlb_t *t);
void tlb_insert(tlb_t *t, uint64_t vpn, uint16_t asid, uint64_t pa,
                int global);
lookup_result tlb_lookup(const tlb_t *t, uint64_t vpn, uint16_t asid);

#endif
