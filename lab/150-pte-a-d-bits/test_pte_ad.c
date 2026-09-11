#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "pte_ad.h"

/*
 * Independent oracle: a direct bit-level simulation of the same spec
 * rules, written without sharing any helper code with pte_ad.c. The
 * differences are structural on purpose:
 *
 * - flat uint64_t array (nnodes*512) plus a parallel int child array,
 *   instead of the implementation's pte_node_t struct tree;
 * - a recursive walk instead of the implementation's iterative loop;
 * - every PTE field extracted inline with shifts and masks at each
 *   use site, instead of the implementation's bit() helper and
 *   is_leaf() predicate;
 * - the result struct and the update logic re-derived from the spec
 *   text, not copied from the implementation.
 *
 * The test builders below construct the same scripted trees twice,
 * once in each representation, and compare_build() checks the two
 * trees are bit-identical before any access runs.
 */

/* Test-local PTE constructors (used only by the builders here). */
#define PTE_LEAF(r, w, x, u, a, d, ppn) \
    ((1ULL) | ((uint64_t)(r) << 1) | ((uint64_t)(w) << 2) | \
     ((uint64_t)(x) << 3) | ((uint64_t)(u) << 4) | \
     ((uint64_t)(a) << 6) | ((uint64_t)(d) << 7) | \
     ((uint64_t)(ppn) << 10))
#define PTE_PTR(ppn) ((1ULL) | ((uint64_t)(ppn) << 10))
#define VA(i2, i1, i0) \
    ((((uint64_t)(i2)) << 30) | (((uint64_t)(i1)) << 21) | \
     (((uint64_t)(i0)) << 12))

typedef struct {
    uint64_t *e;   /* nnodes * 512 flat PTE array */
    int *child;    /* nnodes * 512 flat child-node array, -1 when none */
    int nnodes;
} or_tree_t;

typedef struct {
    int fault;
    unsigned level, i2, i1, i0;
    int a_gained, d_gained;
} or_result_t;

static or_result_t or_step(or_tree_t *t, uint64_t va, int access, int mode,
                           int ni, int level,
                           unsigned p2, unsigned p1, unsigned p0)
{
    or_result_t r = {1, 0, 0, 0, 0, 0, 0}; /* default: fault, nothing */
    unsigned idx, q2 = p2, q1 = p1, q0 = p0;
    uint64_t pte, ppn, npte;
    unsigned v, rr, w, x, u, a, d, code;
    int ok, cn;

    if (ni < 0 || ni >= t->nnodes)
        return r;
    idx = (unsigned)((va >> (12 + 9 * level)) & 0x1FFULL);
    if (level == 2)
        q2 = idx;
    else if (level == 1)
        q1 = idx;
    else
        q0 = idx;
    pte = t->e[(size_t)ni * 512 + idx];
    v = (unsigned)(pte & 1ULL);
    rr = (unsigned)((pte >> 1) & 1ULL);
    w = (unsigned)((pte >> 2) & 1ULL);
    x = (unsigned)((pte >> 3) & 1ULL);
    u = (unsigned)((pte >> 4) & 1ULL);
    a = (unsigned)((pte >> 6) & 1ULL);
    d = (unsigned)((pte >> 7) & 1ULL);
    ppn = pte >> 10;

    if (!v || (!rr && w)) /* spec 4.3.2 step 3 */
        return r;
    if (rr || x) { /* leaf: steps 5, 6, 8, then the step-9 update */
        if (level > 0 && (ppn & ((1ULL << (9 * level)) - 1ULL)) != 0ULL)
            return r; /* misaligned superpage */
        if ((mode == 1 && !u) || (mode == 0 && u))
            return r; /* U-bit vs mode, SUM=0 */
        code = (x << 2) | (w << 1) | rr;
        if (code == 2 || code == 6)
            return r; /* reserved (X,W,R) encodings */
        ok = (access == 0) ? (int)rr : (access == 1) ? (int)w : (int)x;
        if (!ok)
            return r;
        r.fault = 0;
        r.level = (unsigned)level;
        r.i2 = q2;
        r.i1 = q1;
        r.i0 = q0;
        npte = pte;
        if (!a) { /* completed access sets A on the leaf */
            npte |= (1ULL << 6);
            r.a_gained = 1;
        }
        if (access == 1 && !d) { /* completed store sets D on the leaf */
            npte |= (1ULL << 7);
            r.d_gained = 1;
        }
        t->e[(size_t)ni * 512 + idx] = npte;
        return r;
    }
    if (level == 0)
        return r; /* pointer chain ran out of levels */
    cn = t->child[(size_t)ni * 512 + idx];
    return or_step(t, va, access, mode, cn, level - 1, q2, q1, q0);
}

/* FNV-1a 64-bit, folded over every verdict so the checksum must match
 * across -O0, -O2, and sanitizer builds. */
static uint64_t fnv = 14695981039346656037ULL;

static void feed(uint64_t v)
{
    fnv ^= v;
    fnv *= 1099511628211ULL;
}

static uint64_t checks;
static uint64_t mismatches;

static void compare_one(ad_result_t got, or_result_t want, const char *tag,
                        int seqno)
{
    checks++;
    if (got.fault != want.fault ||
        got.leaf.level != want.level ||
        got.leaf.i2 != want.i2 ||
        got.leaf.i1 != want.i1 ||
        got.leaf.i0 != want.i0 ||
        got.a_gained != want.a_gained ||
        got.d_gained != want.d_gained) {
        mismatches++;
        printf("MISMATCH %s step %d: impl fault=%d leaf=(%u,%u,%u,%u) "
               "a=%d d=%d | oracle fault=%d leaf=(%u,%u,%u,%u) a=%d d=%d\n",
               tag, seqno,
               got.fault, got.leaf.level, got.leaf.i2, got.leaf.i1,
               got.leaf.i0, got.a_gained, got.d_gained,
               want.fault, want.level, want.i2, want.i1, want.i0,
               want.a_gained, want.d_gained);
    }
    feed((uint64_t)(uint32_t)got.fault);
    feed((uint64_t)got.leaf.level);
    feed((uint64_t)got.leaf.i2);
    feed((uint64_t)got.leaf.i1);
    feed((uint64_t)got.leaf.i0);
    feed((uint64_t)(uint32_t)got.a_gained);
    feed((uint64_t)(uint32_t)got.d_gained);
}

/* Both builders must produce bit-identical trees. */
static void compare_build(pte_tree_t *it, or_tree_t *ot, const char *tag)
{
    int n, i;
    checks++;
    for (n = 0; n < it->nnodes; n++) {
        for (i = 0; i < 512; i++) {
            if (it->nodes[n].e[i] != ot->e[(size_t)n * 512 + i] ||
                it->nodes[n].child[i] != ot->child[(size_t)n * 512 + i]) {
                mismatches++;
                printf("MISMATCH %s build: node %d entry %d differs\n",
                       tag, n, i);
                return;
            }
        }
    }
}

/* After a sequence, the A/D state of every PTE must agree. */
static void compare_state(pte_tree_t *it, or_tree_t *ot, const char *tag)
{
    int n, i;
    checks++;
    for (n = 0; n < it->nnodes; n++) {
        for (i = 0; i < 512; i++) {
            uint64_t ie = it->nodes[n].e[i];
            uint64_t oe = ot->e[(size_t)n * 512 + i];
            unsigned ia = (unsigned)((ie >> 6) & 1ULL);
            unsigned id = (unsigned)((ie >> 7) & 1ULL);
            unsigned oa = (unsigned)((oe >> 6) & 1ULL);
            unsigned od = (unsigned)((oe >> 7) & 1ULL);
            feed((uint64_t)ia);
            feed((uint64_t)id);
            if (ia != oa || id != od) {
                mismatches++;
                printf("MISMATCH %s state: node %d entry %d A/D "
                       "impl=%u/%u oracle=%u/%u\n",
                       tag, n, i, ia, id, oa, od);
                return;
            }
        }
    }
}

/* ---- Tree A: mixed levels, valid mappings with varied flags ---- */

static void buildA_impl(pte_tree_t *t)
{
    int n, i;
    for (n = 0; n < t->nnodes; n++)
        for (i = 0; i < 512; i++) {
            t->nodes[n].e[i] = 0;
            t->nodes[n].child[i] = -1;
        }
    t->nodes[0].e[5] = PTE_PTR(0);
    t->nodes[0].child[5] = 1;
    t->nodes[0].e[7] = PTE_LEAF(1, 1, 0, 1, 0, 0, 0); /* gigapage RW U */
    t->nodes[1].e[3] = PTE_PTR(0);
    t->nodes[1].child[3] = 2;
    t->nodes[1].e[9] = PTE_LEAF(1, 0, 0, 1, 0, 0, 0); /* megapage R U */
    t->nodes[2].e[11] = PTE_LEAF(1, 1, 0, 1, 0, 0, 0); /* 4KiB RW U */
    t->nodes[2].e[12] = PTE_LEAF(1, 0, 0, 1, 0, 0, 0); /* 4KiB R U */
    t->nodes[2].e[13] = PTE_LEAF(1, 1, 1, 1, 1, 1, 0); /* 4KiB RWX U, A=D=1 */
}

static void buildA_or(or_tree_t *t)
{
    int n, i;
    for (n = 0; n < t->nnodes; n++)
        for (i = 0; i < 512; i++) {
            t->e[(size_t)n * 512 + i] = 0;
            t->child[(size_t)n * 512 + i] = -1;
        }
    t->e[5] = PTE_PTR(0);
    t->child[5] = 1;
    t->e[7] = PTE_LEAF(1, 1, 0, 1, 0, 0, 0);
    t->e[512 + 3] = PTE_PTR(0);
    t->child[512 + 3] = 2;
    t->e[512 + 9] = PTE_LEAF(1, 0, 0, 1, 0, 0, 0);
    t->e[1024 + 11] = PTE_LEAF(1, 1, 0, 1, 0, 0, 0);
    t->e[1024 + 12] = PTE_LEAF(1, 0, 0, 1, 0, 0, 0);
    t->e[1024 + 13] = PTE_LEAF(1, 1, 1, 1, 1, 1, 0);
}

/* ---- Tree B: fault-heavy shapes ---- */

static void buildB_impl(pte_tree_t *t)
{
    int n, i;
    for (n = 0; n < t->nnodes; n++)
        for (i = 0; i < 512; i++) {
            t->nodes[n].e[i] = 0;
            t->nodes[n].child[i] = -1;
        }
    t->nodes[0].e[1] = PTE_PTR(0);
    t->nodes[0].child[1] = 1;
    t->nodes[0].e[2] = 1ULL | (1ULL << 2); /* V=1, r=0, w=1: reserved */
    t->nodes[1].e[4] = PTE_LEAF(1, 1, 0, 1, 0, 0, 1); /* megapage, ppn=1: misaligned */
    t->nodes[1].e[5] = PTE_PTR(0); /* dangling: child stays -1 */
    t->nodes[1].e[8] = PTE_PTR(0);
    t->nodes[1].child[8] = 2;
    t->nodes[2].e[8] = PTE_PTR(0); /* valid pointer at level 0: chain runs out */
}

static void buildB_or(or_tree_t *t)
{
    int n, i;
    for (n = 0; n < t->nnodes; n++)
        for (i = 0; i < 512; i++) {
            t->e[(size_t)n * 512 + i] = 0;
            t->child[(size_t)n * 512 + i] = -1;
        }
    t->e[1] = PTE_PTR(0);
    t->child[1] = 1;
    t->e[2] = 1ULL | (1ULL << 2);
    t->e[512 + 4] = PTE_LEAF(1, 1, 0, 1, 0, 0, 1);
    t->e[512 + 5] = PTE_PTR(0);
    t->e[512 + 8] = PTE_PTR(0);
    t->child[512 + 8] = 2;
    t->e[1024 + 8] = PTE_PTR(0);
}

/* ---- Exhaustive sweep: one leaf at a fixed level, all flag combos ---- */

static void buildSweep_impl(pte_tree_t *t, int level,
                            unsigned r, unsigned w, unsigned x,
                            unsigned u, unsigned a, unsigned d)
{
    int n, i;
    for (n = 0; n < t->nnodes; n++)
        for (i = 0; i < 512; i++) {
            t->nodes[n].e[i] = 0;
            t->nodes[n].child[i] = -1;
        }
    if (level == 2) {
        t->nodes[0].e[0] = PTE_LEAF(r, w, x, u, a, d, 0);
    } else if (level == 1) {
        t->nodes[0].e[0] = PTE_PTR(0);
        t->nodes[0].child[0] = 1;
        t->nodes[1].e[0] = PTE_LEAF(r, w, x, u, a, d, 0);
    } else {
        t->nodes[0].e[0] = PTE_PTR(0);
        t->nodes[0].child[0] = 1;
        t->nodes[1].e[0] = PTE_PTR(0);
        t->nodes[1].child[0] = 2;
        t->nodes[2].e[0] = PTE_LEAF(r, w, x, u, a, d, 0);
    }
}

static void buildSweep_or(or_tree_t *t, int level,
                          unsigned r, unsigned w, unsigned x,
                          unsigned u, unsigned a, unsigned d)
{
    int n, i;
    for (n = 0; n < t->nnodes; n++)
        for (i = 0; i < 512; i++) {
            t->e[(size_t)n * 512 + i] = 0;
            t->child[(size_t)n * 512 + i] = -1;
        }
    if (level == 2) {
        t->e[0] = PTE_LEAF(r, w, x, u, a, d, 0);
    } else if (level == 1) {
        t->e[0] = PTE_PTR(0);
        t->child[0] = 1;
        t->e[512] = PTE_LEAF(r, w, x, u, a, d, 0);
    } else {
        t->e[0] = PTE_PTR(0);
        t->child[0] = 1;
        t->e[512] = PTE_PTR(0);
        t->child[512] = 2;
        t->e[1024] = PTE_LEAF(r, w, x, u, a, d, 0);
    }
}

/* ---- Sequence runner ---- */

typedef struct {
    uint64_t va;
    int access;
    int mode;
} step_t;

typedef void (*builder_impl_t)(pte_tree_t *);
typedef void (*builder_or_t)(or_tree_t *);

static void run_sequence(const char *name,
                         builder_impl_t bi, builder_or_t bo,
                         const step_t *steps, int nsteps)
{
    static pte_node_t inodes[3];
    static uint64_t oe[3 * 512];
    static int och[3 * 512];
    pte_tree_t it = {inodes, 3};
    or_tree_t ot = {oe, och, 3};
    int s;

    bi(&it);
    bo(&ot);
    compare_build(&it, &ot, name);
    for (s = 0; s < nsteps; s++) {
        ad_result_t got = ad_predict(&it, steps[s].va,
                                     (ad_access_t)steps[s].access,
                                     (ad_mode_t)steps[s].mode);
        or_result_t want = or_step(&ot, steps[s].va, steps[s].access,
                                   steps[s].mode, 0, 2, 0, 0, 0);
        compare_one(got, want, name, s);
    }
    compare_state(&it, &ot, name);
}

int main(void)
{
    static pte_node_t inodes[3];
    static uint64_t oe[3 * 512];
    static int och[3 * 512];
    pte_tree_t it = {inodes, 3};
    or_tree_t ot = {oe, och, 3};
    int level, bits, access, mode;
    unsigned r, w, x, u, a, d;

    /* Exhaustive: leaf at each level, all 64 R/W/X/U/A/D combos,
     * all 3 access types, both modes. VA is 0 (vpn all zero). */
    for (level = 2; level >= 0; level--) {
        for (bits = 0; bits < 64; bits++) {
            r = (unsigned)(bits & 1);
            w = (unsigned)((bits >> 1) & 1);
            x = (unsigned)((bits >> 2) & 1);
            u = (unsigned)((bits >> 3) & 1);
            a = (unsigned)((bits >> 4) & 1);
            d = (unsigned)((bits >> 5) & 1);
            for (access = 0; access < 3; access++) {
                for (mode = 0; mode < 2; mode++) {
                    ad_result_t got;
                    or_result_t want;
                    buildSweep_impl(&it, level, r, w, x, u, a, d);
                    buildSweep_or(&ot, level, r, w, x, u, a, d);
                    got = ad_predict(&it, 0, (ad_access_t)access,
                                     (ad_mode_t)mode);
                    want = or_step(&ot, 0, access, mode, 0, 2, 0, 0, 0);
                    compare_one(got, want, "sweep", bits);
                }
            }
        }
    }

    /* Invalid (V=0) leaf at each level: every access faults, nothing
     * is touched. */
    for (level = 2; level >= 0; level--) {
        for (access = 0; access < 3; access++) {
            for (mode = 0; mode < 2; mode++) {
                ad_result_t got;
                or_result_t want;
                buildSweep_impl(&it, level, 0, 0, 0, 0, 0, 0);
                buildSweep_or(&ot, level, 0, 0, 0, 0, 0, 0);
                if (level == 2) {
                    inodes[0].e[0] &= ~1ULL;
                    oe[0] &= ~1ULL;
                } else if (level == 1) {
                    inodes[1].e[0] &= ~1ULL;
                    oe[512] &= ~1ULL;
                } else {
                    inodes[2].e[0] &= ~1ULL;
                    oe[1024] &= ~1ULL;
                }
                got = ad_predict(&it, 0, (ad_access_t)access,
                                 (ad_mode_t)mode);
                want = or_step(&ot, 0, access, mode, 0, 2, 0, 0, 0);
                compare_one(got, want, "vzero", access);
            }
        }
    }

    /* Scripted sequences over tree A. */
    {
        static const step_t s1[] = { /* gigapage: A then D */
            {VA(7, 0, 0), 0, 0}, {VA(7, 0, 0), 0, 0},
            {VA(7, 0, 0), 1, 0}, {VA(7, 0, 0), 1, 0}};
        static const step_t s2[] = { /* megapage: read, faulting write */
            {VA(5, 9, 0), 0, 0}, {VA(5, 9, 0), 1, 0},
            {VA(5, 9, 0), 0, 1}};
        static const step_t s3[] = { /* 4KiB RW: A, D, then exec faults */
            {VA(5, 3, 11), 0, 0}, {VA(5, 3, 11), 1, 0},
            {VA(5, 3, 11), 2, 0}};
        static const step_t s4[] = { /* 4KiB R-only: write faults */
            {VA(5, 3, 12), 0, 0}, {VA(5, 3, 12), 1, 0}};
        static const step_t s5[] = { /* 4KiB RWX, A=D=1: nothing to gain */
            {VA(5, 3, 13), 0, 1}, {VA(5, 3, 13), 1, 1},
            {VA(5, 3, 13), 2, 1}};
        static const step_t s6[] = { /* invalid entries, then U-mode read */
            {VA(0, 0, 0), 0, 0}, {VA(6, 0, 0), 0, 0},
            {VA(7, 0, 0), 0, 1}};
        run_sequence("A1", buildA_impl, buildA_or, s1, 4);
        run_sequence("A2", buildA_impl, buildA_or, s2, 3);
        run_sequence("A3", buildA_impl, buildA_or, s3, 3);
        run_sequence("A4", buildA_impl, buildA_or, s4, 2);
        run_sequence("A5", buildA_impl, buildA_or, s5, 3);
        run_sequence("A6", buildA_impl, buildA_or, s6, 3);
    }

    /* Scripted sequences over tree B: every access faults and no PTE
     * may change. */
    {
        static const step_t t1[] = {{VA(2, 0, 0), 0, 0}}; /* reserved r=0,w=1 */
        static const step_t t2[] = {{VA(1, 4, 0), 0, 0}}; /* misaligned megapage */
        static const step_t t3[] = {{VA(1, 5, 0), 1, 0}}; /* dangling pointer */
        static const step_t t4[] = {{VA(1, 6, 0), 2, 1}}; /* v=0 entry */
        static const step_t t5[] = {{VA(1, 8, 8), 0, 0}}; /* chain runs out */
        run_sequence("B1", buildB_impl, buildB_or, t1, 1);
        run_sequence("B2", buildB_impl, buildB_or, t2, 1);
        run_sequence("B3", buildB_impl, buildB_or, t3, 1);
        run_sequence("B4", buildB_impl, buildB_or, t4, 1);
        run_sequence("B5", buildB_impl, buildB_or, t5, 1);
    }

    printf("checks=%" PRIu64 " mismatches=%" PRIu64
           " checksum=%016" PRIx64 "\n",
           checks, mismatches, fnv);
    return mismatches == 0 ? 0 : 1;
}
