#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "pte_a.h"

/*
 * Oracle: a spec table transcribed by hand from the Sv39 PTE rules in
 * the RISC-V Privileged ISA Specification (addressing and memory
 * protection), not computed from the implementation's code.
 *
 * Row i is indexed by the low four flag bits (i = flags & 0x0F, bits
 * X W R V). The three columns are read, write, execute; 1 = faults,
 * 0 = proceeds. Each row was derived by applying the rules in order:
 *
 *   V = 0          -> invalid entry, faults on every access.
 *   W = 1, R = 0   -> reserved combination, faults on every access.
 *   otherwise      -> read faults iff R = 0, write faults iff W = 0,
 *                     execute faults iff X = 0.
 *
 * Row-by-row derivation (i, bits XWRV, R/W/X fault triple):
 *   0: 0000 V=0              -> 1,1,1
 *   1: 0001 R=W=X=0          -> 1,1,1
 *   2: 0010 V=0              -> 1,1,1
 *   3: 0011 R=1              -> 0,1,1
 *   4: 0100 V=0              -> 1,1,1
 *   5: 0101 W=1,R=0 reserved -> 1,1,1
 *   6: 0110 V=0              -> 1,1,1
 *   7: 0111 R=W=1            -> 0,0,1
 *   8: 1000 V=0              -> 1,1,1
 *   9: 1001 X=1              -> 1,1,0
 *  10: 1010 V=0              -> 1,1,1
 *  11: 1011 R=X=1            -> 0,1,0
 *  12: 1100 V=0              -> 1,1,1
 *  13: 1101 W=1,R=0 reserved -> 1,1,1
 *  14: 1110 V=0              -> 1,1,1
 *  15: 1111 R=W=X=1          -> 0,0,0
 *
 * The A-bit half of the oracle follows the hardware-update rule: on a
 * fault the PTE is untouched (A keeps its input value); on a
 * permitted access the hardware sets A = 1. Bits U, G, D are inert
 * inputs and never index this table.
 */
static const uint8_t ORACLE_FAULT[3][16] = {
    /* read */
    {1,1,1,0, 1,1,1,0, 1,1,1,0, 1,1,1,0},
    /* write */
    {1,1,1,1, 1,1,1,0, 1,1,1,1, 1,1,1,0},
    /* execute */
    {1,1,1,1, 1,1,1,1, 1,0,1,0, 1,1,1,0}
};

static int oracle_fault(uint8_t flags, int access)
{
    return ORACLE_FAULT[access][flags & 0x0F];
}

static int oracle_a_after(uint8_t flags, int access)
{
    int fault = oracle_fault(flags, access);
    return fault ? ((flags & PTE_A) != 0) : 1;
}

/* FNV-1a 64-bit, folded over every (fault, a_after) pair so the
 * checksum must match across -O0, -O2, and sanitizer builds. */
static uint64_t fnv = 14695981039346656037ULL;

static void feed(int r)
{
    fnv ^= (uint64_t)(uint32_t)r;
    fnv *= 1099511628211ULL;
}

static uint64_t checks;
static uint64_t mismatches;

static void check_case(uint8_t flags, int access)
{
    pte_a_result got = pte_a_check(flags, access);
    int want_fault = oracle_fault(flags, access);
    int want_a = oracle_a_after(flags, access);
    int a_in = (flags & PTE_A) != 0;

    /* Assertion 1: the fault verdict matches the spec table. */
    checks++;
    if (got.fault != want_fault) {
        mismatches++;
        printf("MISMATCH fault: flags=0x%02x access=%d got=%d want=%d\n",
               flags, access, got.fault, want_fault);
    }
    feed(got.fault);

    /* Assertion 2: the A value after the access matches the table. */
    checks++;
    if (got.a_after != want_a) {
        mismatches++;
        printf("MISMATCH a_after: flags=0x%02x access=%d got=%d want=%d\n",
               flags, access, got.a_after, want_a);
    }
    feed(got.a_after);

    /* Assertion 3: the A-bit transition invariant, the fundamental
     * truth of this module. A permitted access sets A to 1
     * (0 -> 1 or 1 -> 1); a fault leaves the PTE untouched, so A keeps
     * its input value. This is checked against the invariant itself,
     * not against the oracle's A column. */
    {
        int transition_ok = got.fault ? (got.a_after == a_in)
                                      : (got.a_after == 1);
        checks++;
        if (!transition_ok) {
            mismatches++;
            printf("MISMATCH transition: flags=0x%02x access=%d "
                   "fault=%d a_in=%d a_after=%d\n",
                   flags, access, got.fault, a_in, got.a_after);
        }
        feed(transition_ok);
    }
}

int main(void)
{
    /* Full space: all 256 flag-byte combinations times the 3 access
     * types. U, G, D vary across the 256 bytes and are inert in both
     * the implementation and the oracle, so their irrelevance is
     * exercised, not assumed. */
    for (unsigned f = 0; f < 256; f++)
        for (int a = PTE_ACC_READ; a <= PTE_ACC_EXEC; a++)
            check_case((uint8_t)f, a);

    /* Machine-readable result block, copied verbatim into PROOF.md. */
    printf("HEADER-BEGIN\n");
    printf("Checks: %" PRIu64 "\n", checks);
    printf("Mismatches: %" PRIu64 "\n", mismatches);
    printf("Checksum: %016" PRIx64 "\n", fnv);
    printf("HEADER-END\n");
    printf("cases: 768 (256 flag bytes x 3 access types, exhaustive)\n");
    printf("verdict: %s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
