#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "pte_global.h"

/* Fixed-seed splitmix64, the same constants every run, every build. */
static uint64_t rng_state;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a 64-bit, folded over every compared result so the checksum
 * must match across -O0, -O2, and sanitizer builds. */
static uint64_t fnv = 14695981039346656037ULL;

static void feed(int v)
{
    fnv ^= (uint64_t)(uint32_t)v;
    fnv *= 1099511628211ULL;
}

static void feed64(uint64_t v)
{
    feed((int)(uint32_t)v);
    feed((int)(uint32_t)(v >> 32));
}

static uint64_t checks;
static uint64_t mismatches;

/* ------------------------------------------------------------------ */
/* Scenario: three address spaces sharing global leaves.               */
/*                                                                      */
/* Roots: page 2 = ASID 1, page 3 = ASID 2, page 4 = ASID 9.            */
/*                                                                      */
/* Mappings (VPN indices hard-coded as literals; a wrong literal fails */
/* the walk loudly, so the literals themselves are checked):           */
/*   vpn 0x12345 (VPN2=0, VPN1=145, VPN0=325): global 4K leaf, PPN      */
/*     0xABCDE, shared by all three roots.                             */
/*   vpn 0x23456 (VPN2=0, VPN1=282, VPN0=86): non-global 4K leaf,      */
/*     PPN 0x11111 under ASID 1, PPN 0x22222 under ASID 2.             */
/*   vpn 0x34567 (VPN2=0, VPN1=418, VPN0=359): non-global 4K leaf,     */
/*     PPN 0x33333, present only under ASID 1.                         */
/*   vpn 0x25600 (VPN2=0, VPN1=299): global 2 MiB megapage, PPN        */
/*     0xF0000, shared by ASID 1 and ASID 2.                           */
/*   vpn 0x25800 (VPN1=300): megapage with misaligned PPN -> fault.    */
/*   vpn 0x25A00 (VPN1=301): leaf with W set but R clear -> fault.     */
/*   vpn 0x25C00 (VPN1=302): V clear -> fault.                         */
/*   vpn 0x25E00 (VPN2=0, VPN1=303, VPN0=0): branch PTE has G set,     */
/*     leaf has G clear -> translation must not be global.             */
/*   va 0x40000000 (VPN2=1): global 1 GiB gigapage, PPN 0x40000,      */
/*     shared by ASID 1 and ASID 2.                                    */
/* ------------------------------------------------------------------ */

static pt_mem mem;

static uint64_t mkpte(uint64_t ppn, uint64_t flags)
{
    return ((ppn & PTE_PPN_MASK) << PTE_PPN_SHIFT) | (flags & 0x3FFULL);
}

static void build_tables(void)
{
    unsigned i, j;

    for (i = 0; i < PT_NPAGES; i++)
        for (j = 0; j < 512; j++)
            mem.pages[i].e[j] = 0;

    /* Shared global 4K leaf, vpn 0x12345. */
    mem.pages[2].e[0] = mkpte(10, PTE_V);
    mem.pages[10].e[145] = mkpte(11, PTE_V);
    mem.pages[11].e[325] = mkpte(0xABCDE, PTE_V | PTE_R | PTE_G);

    mem.pages[3].e[0] = mkpte(12, PTE_V);
    mem.pages[12].e[145] = mkpte(13, PTE_V);
    mem.pages[13].e[325] = mkpte(0xABCDE, PTE_V | PTE_R | PTE_G);

    mem.pages[4].e[0] = mkpte(14, PTE_V);
    mem.pages[14].e[145] = mkpte(15, PTE_V);
    mem.pages[15].e[325] = mkpte(0xABCDE, PTE_V | PTE_R | PTE_G);

    /* Non-global per-ASID 4K leaf, vpn 0x23456. */
    mem.pages[10].e[282] = mkpte(20, PTE_V);
    mem.pages[20].e[86] = mkpte(0x11111, PTE_V | PTE_R);

    mem.pages[12].e[282] = mkpte(21, PTE_V);
    mem.pages[21].e[86] = mkpte(0x22222, PTE_V | PTE_R);

    /* Non-global leaf only under ASID 1, vpn 0x34567. */
    mem.pages[10].e[418] = mkpte(22, PTE_V);
    mem.pages[22].e[359] = mkpte(0x33333, PTE_V | PTE_R);

    /* Shared global 2 MiB megapage, vpn 0x25600. */
    mem.pages[10].e[299] = mkpte(0xF0000, PTE_V | PTE_R | PTE_X | PTE_G);
    mem.pages[12].e[299] = mkpte(0xF0000, PTE_V | PTE_R | PTE_X | PTE_G);

    /* Misaligned megapage, vpn 0x25800. */
    mem.pages[10].e[300] = mkpte(0xF0001, PTE_V | PTE_R | PTE_X | PTE_G);

    /* W without R, vpn 0x25A00. */
    mem.pages[10].e[301] = mkpte(0x55000, PTE_V | PTE_W);

    /* vpn 0x25C00: entry left zero, V clear. */

    /* Branch PTE with G set, leaf with G clear, vpn 0x25E00. */
    mem.pages[10].e[303] = mkpte(24, PTE_V | PTE_G);
    mem.pages[24].e[0] = mkpte(0x77777, PTE_V | PTE_R);

    /* Shared global 1 GiB gigapage at va 0x40000000. */
    mem.pages[2].e[1] = mkpte(0x40000, PTE_V | PTE_R | PTE_X | PTE_G);
    mem.pages[3].e[1] = mkpte(0x40000, PTE_V | PTE_R | PTE_X | PTE_G);
}

static const satp_t SATP_A = { 1, 2 };
static const satp_t SATP_B = { 2, 3 };
static const satp_t SATP_C = { 9, 4 };

static void check_walk(const char *tag, walk_result got, int want_fault,
                       uint64_t want_pa, int want_global, int want_level)
{
    int bad = 0;

    checks++;
    feed(got.fault);
    feed64(got.pa);
    feed(got.global);
    feed(got.level);
    if (got.fault != want_fault) {
        printf("MISMATCH %s: fault got=%d want=%d\n", tag, got.fault,
               want_fault);
        mismatches++;
        return;
    }
    if (want_fault)
        return;
    checks += 3;
    if (got.pa != want_pa) {
        printf("MISMATCH %s: pa got=0x%016" PRIx64 " want=0x%016" PRIx64
               "\n", tag, got.pa, want_pa);
        bad = 1;
    }
    if (got.global != want_global) {
        printf("MISMATCH %s: global got=%d want=%d\n", tag, got.global,
               want_global);
        bad = 1;
    }
    if (got.level != want_level) {
        printf("MISMATCH %s: level got=%d want=%d\n", tag, got.level,
               want_level);
        bad = 1;
    }
    mismatches += (uint64_t)bad;
}

static void walk_tests(void)
{
    uint64_t off = 0x678ULL;

    /* The shared global leaf: three ASIDs, one physical page. */
    check_walk("g1/A", sv39_walk(&mem, SATP_A, (0x12345ULL << 12) | off),
               0, (0xABCDEULL << 12) | off, 1, 0);
    check_walk("g1/B", sv39_walk(&mem, SATP_B, (0x12345ULL << 12) | off),
               0, (0xABCDEULL << 12) | off, 1, 0);
    check_walk("g1/C", sv39_walk(&mem, SATP_C, (0x12345ULL << 12) | off),
               0, (0xABCDEULL << 12) | off, 1, 0);

    /* Non-global leaves: each ASID gets its own physical page. */
    check_walk("n1/A", sv39_walk(&mem, SATP_A, (0x23456ULL << 12) | off),
               0, (0x11111ULL << 12) | off, 0, 0);
    check_walk("n1/B", sv39_walk(&mem, SATP_B, (0x23456ULL << 12) | off),
               0, (0x22222ULL << 12) | off, 0, 0);

    /* Non-global leaf under ASID 1 only: ASID 2 must fault, not reuse
     * ASID 1's translation. */
    check_walk("n2/A", sv39_walk(&mem, SATP_A, (0x34567ULL << 12) | off),
               0, (0x33333ULL << 12) | off, 0, 0);
    check_walk("n2/B", sv39_walk(&mem, SATP_B, (0x34567ULL << 12) | off),
               1, 0, 0, 0);

    /* Shared global megapage under two ASIDs. */
    {
        uint64_t va = (0x25600ULL << 12) | 0x12345ULL;
        uint64_t pa = (0xF0000ULL << 12) | 0x12345ULL;

        check_walk("m1/A", sv39_walk(&mem, SATP_A, va), 0, pa, 1, 1);
        check_walk("m1/B", sv39_walk(&mem, SATP_B, va), 0, pa, 1, 1);
    }

    /* Fault cases. */
    check_walk("m2/misaligned", sv39_walk(&mem, SATP_A, 0x25800ULL << 12),
               1, 0, 0, 0);
    check_walk("w1/w-without-r", sv39_walk(&mem, SATP_A, 0x25A00ULL << 12),
               1, 0, 0, 0);
    check_walk("z1/v-clear", sv39_walk(&mem, SATP_A, 0x25C00ULL << 12),
               1, 0, 0, 0);

    /* Branch PTE has G set, leaf has G clear: not global. */
    check_walk("b1/branch-g", sv39_walk(&mem, SATP_A, (0x25E00ULL << 12) | off),
               0, (0x77777ULL << 12) | off, 0, 0);

    /* Shared global gigapage under two ASIDs. */
    {
        uint64_t va = 0x40000000ULL | 0x1234ULL;
        uint64_t pa = (0x40000ULL << 12) | 0x1234ULL;

        check_walk("g2g/A", sv39_walk(&mem, SATP_A, va), 0, pa, 1, 2);
        check_walk("g2g/B", sv39_walk(&mem, SATP_B, va), 0, pa, 1, 2);
    }
}

/* ------------------------------------------------------------------ */
/* Independent oracle: a separately written spec table transcribing   */
/* the G-bit rule. The rule: a cached global translation applies to   */
/* every ASID (the ASID is not consulted); a cached non-global        */
/* translation applies only when the lookup ASID matches the ASID it  */
/* was cached under. Encoded here as an if/else ladder over a plain   */
/* entry list, sharing no code with tlb_lookup's scan. Insertion      */
/* order (FIFO eviction) is shared harness mechanics; the match rule   */
/* under test is encoded independently.                               */
/* ------------------------------------------------------------------ */

typedef struct {
    int live;
    uint64_t vpn;
    uint16_t asid;
    uint64_t pa;
    int global;
} spec_entry;

static spec_entry spec_tab[TLB_NSLOTS];
static unsigned spec_next;

static void spec_init(void)
{
    unsigned i;

    for (i = 0; i < TLB_NSLOTS; i++)
        spec_tab[i].live = 0;
    spec_next = 0;
}

static void spec_insert(uint64_t vpn, uint16_t asid, uint64_t pa, int global)
{
    spec_entry *e = &spec_tab[spec_next];

    e->live = 1;
    e->vpn = vpn;
    e->asid = asid;
    e->pa = pa;
    e->global = global ? 1 : 0;
    spec_next = (spec_next + 1u) % TLB_NSLOTS;
}

static lookup_result spec_lookup(uint64_t vpn, uint16_t asid)
{
    lookup_result r;
    unsigned i;

    r.hit = 0;
    r.pa = 0;
    for (i = 0; i < TLB_NSLOTS; i++) {
        const spec_entry *e = &spec_tab[i];

        if (!e->live)
            continue;
        if (e->vpn != vpn)
            continue;
        if (e->global) {
            /* Global mapping: the ASID is not consulted. */
            r.hit = 1;
            r.pa = e->pa;
            return r;
        }
        if (e->asid == asid) {
            r.hit = 1;
            r.pa = e->pa;
            return r;
        }
    }
    return r;
}

static tlb_t tlb;

static void both_insert(uint64_t vpn, uint16_t asid, uint64_t pa, int global)
{
    tlb_insert(&tlb, vpn, asid, pa, global);
    spec_insert(vpn, asid, pa, global);
}

static void check_lookup(const char *tag, uint64_t vpn, uint16_t asid)
{
    lookup_result got = tlb_lookup(&tlb, vpn, asid);
    lookup_result want = spec_lookup(vpn, asid);
    int bad = 0;

    checks += 2;
    feed(got.hit);
    feed64(got.pa);
    if (got.hit != want.hit) {
        printf("MISMATCH %s: vpn=0x%016" PRIx64 " asid=%u hit got=%d want=%d\n",
               tag, vpn, (unsigned)asid, got.hit, want.hit);
        bad = 1;
    }
    if (got.pa != want.pa) {
        printf("MISMATCH %s: vpn=0x%016" PRIx64 " asid=%u pa got=0x%016" PRIx64
               " want=0x%016" PRIx64 "\n",
               tag, vpn, (unsigned)asid, got.pa, want.pa);
        bad = 1;
    }
    mismatches += (uint64_t)bad;
}

/* Walk results cached in the TLB: the G-bit sharing and ASID-scoping
 * behavior end to end. */
static void tlb_integration_tests(void)
{
    uint64_t off = 0x678ULL;
    uint64_t pa_g1 = (0xABCDEULL << 12) | off;
    uint64_t pa_n1a = (0x11111ULL << 12) | off;
    uint64_t pa_n1b = (0x22222ULL << 12) | off;

    tlb_init(&tlb);
    spec_init();

    /* Global translation cached under ASID 1 is visible under ASID 2. */
    both_insert(0x12345ULL, 1, pa_g1, 1);
    check_lookup("global-cross-asid", 0x12345ULL, 2);

    /* Non-global cached under ASID 1 is invisible under ASID 2. */
    both_insert(0x23456ULL, 1, pa_n1a, 0);
    check_lookup("nonglobal-other-asid-miss", 0x23456ULL, 2);

    /* Each ASID keeps its own non-global entry for the same VPN. */
    both_insert(0x23456ULL, 2, pa_n1b, 0);
    check_lookup("nonglobal-own-asid-2", 0x23456ULL, 2);
    check_lookup("nonglobal-own-asid-1", 0x23456ULL, 1);

    /* A third ASID sees neither non-global entry. */
    check_lookup("nonglobal-third-asid-miss", 0x23456ULL, 3);
}

/* Directed G set/clear x ASID match/mismatch matrix. */
static void tlb_matrix_tests(void)
{
    static const uint16_t owners[2] = { 1, 2 };
    static const uint16_t lookups[3] = { 1, 2, 3 };
    int g, o, a;

    tlb_init(&tlb);
    spec_init();

    for (g = 0; g < 2; g++)
        for (o = 0; o < 2; o++)
            both_insert(0x6000ULL | ((uint64_t)g << 4) | owners[o],
                        owners[o],
                        0x90000ULL + (uint64_t)(g * 2 + o) * 0x1000ULL,
                        g);

    for (g = 0; g < 2; g++)
        for (o = 0; o < 2; o++)
            for (a = 0; a < 3; a++)
                check_lookup("matrix",
                             0x6000ULL | ((uint64_t)g << 4) | owners[o],
                             lookups[a]);

    check_lookup("matrix-miss", 0x7000ULL, 1);
    check_lookup("matrix-miss", 0x7000ULL, 2);
}

/* Bulk differential sweep: random inserts and lookups against the
 * oracle, exercising eviction and stale-entry behavior. */
static void tlb_random_sweep(void)
{
    uint64_t k;
    const uint64_t N = 200000ULL;

    tlb_init(&tlb);
    spec_init();

    for (k = 0; k < N; k++) {
        if (splitmix64() & 1ULL) {
            uint64_t vpn = splitmix64() & 0x3FFFFULL;
            uint16_t asid = (uint16_t)(1 + (splitmix64() & 3ULL));
            uint64_t pa = ((splitmix64() & 0xFFFFFULL) << 12)
                        | (splitmix64() & 0xFFFULL);
            int global = (int)(splitmix64() & 1ULL);

            both_insert(vpn, asid, pa, global);
        } else {
            uint64_t vpn = splitmix64() & 0x3FFFFULL;
            uint16_t asid = (uint16_t)(1 + (splitmix64() & 3ULL));

            check_lookup("sweep", vpn, asid);
        }
    }
}

/* Final slot-by-slot consistency between the two tables. */
static void tlb_consistency_check(void)
{
    unsigned i;

    for (i = 0; i < TLB_NSLOTS; i++) {
        const tlb_entry *e = &tlb.slot[i];
        const spec_entry *s = &spec_tab[i];
        int bad = 0;

        checks += 5;
        feed(e->valid);
        feed64(e->vpn);
        feed((int)e->asid);
        feed64(e->pa);
        feed(e->global);
        if (!!e->valid != !!s->live)
            bad = 1;
        if (e->vpn != s->vpn)
            bad = 1;
        if (e->asid != s->asid)
            bad = 1;
        if (e->pa != s->pa)
            bad = 1;
        if (!!e->global != !!s->global)
            bad = 1;
        if (bad) {
            mismatches++;
            printf("MISMATCH slot %u diverged\n", i);
        }
    }
}

int main(void)
{
    rng_state = 0x123456789ABCDEF0ULL;

    build_tables();
    walk_tests();
    tlb_integration_tests();
    tlb_matrix_tests();
    tlb_random_sweep();
    tlb_consistency_check();

    /* Machine-readable result block, copied verbatim into PROOF.md. */
    printf("HEADER-BEGIN\n");
    printf("Checks: %" PRIu64 "\n", checks);
    printf("Mismatches: %" PRIu64 "\n", mismatches);
    printf("Checksum: %016" PRIx64 "\n", fnv);
    printf("HEADER-END\n");
    printf("verdict: %s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
