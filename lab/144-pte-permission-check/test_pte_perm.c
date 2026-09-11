#include <stdint.h>
#include <stdio.h>

#include "pte_perm.h"
#include "oracle.h"

static uint64_t fnv1a = 1469598103934665603ULL;

static void fnv_add(uint64_t v)
{
    fnv1a ^= v;
    fnv1a *= 1099511628211ULL;
}

static const char *access_name(int a)
{
    switch (a) {
    case PERM_READ: return "read";
    case PERM_WRITE: return "write";
    case PERM_EXEC: return "exec";
    default: return "?";
    }
}

int main(void)
{
    long checks = 0;
    long mismatches = 0;

    /* 256 raw flag bytes x 3 access types x 2 privilege modes = 1536 cases.
     * The sweep covers all 16 R/W/X/U combinations (96 distinct
     * R/W/X/U x access x mode cases) and additionally proves that bits
     * outside 4:1 (V, and bits 7:5) are ignored by the implementation. */
    for (unsigned flags = 0; flags < 256; flags++) {
        for (int access = 0; access < 3; access++) {
            for (int mode = 0; mode < 2; mode++) {
                int got = pte_perm_ok((uint8_t)flags,
                                      (perm_access_t)access,
                                      (perm_mode_t)mode);
                int want = oracle_perm_ok((uint8_t)flags, access, mode);
                fnv_add((uint64_t)flags << 32 |
                        (uint64_t)access << 16 |
                        (uint64_t)mode << 8 |
                        (uint64_t)(got & 1));
                checks++;
                if (got != want) {
                    mismatches++;
                    if (mismatches <= 10) {
                        printf("MISMATCH flags=0x%02X access=%s mode=%s got=%d want=%d\n",
                               flags, access_name(access),
                               mode ? "U" : "S", got, want);
                    }
                }
            }
        }
    }

    /* Directed edge rows: every reserved W=1,R=0 row faults, and U mismatch
     * rows fault, independent of access type and mode. */
    const uint8_t reserved_rows[] = { 0x04, 0x0C, 0x14, 0x1C }; /* W=1,R=0, X/U vary */
    long edge_checks = 0;
    for (unsigned i = 0; i < 4; i++) {
        for (int access = 0; access < 3; access++) {
            for (int mode = 0; mode < 2; mode++) {
                int got = pte_perm_ok(reserved_rows[i], (perm_access_t)access, (perm_mode_t)mode);
                edge_checks++;
                if (got != 0) {
                    mismatches++;
                    printf("EDGE-FAIL reserved flags=0x%02X access=%s mode=%s -> %d, want 0\n",
                           reserved_rows[i], access_name(access), mode ? "U" : "S", got);
                }
            }
        }
    }
    /* U=0 page in U-mode must fault; U=1 page in S-mode must fault (SUM=0). */
    const struct { uint8_t flags; int access; int mode; int want; } umode_rows[] = {
        { 0x02, PERM_READ, PERM_UMODE, 0 }, /* R,U=0: U-mode denied */
        { 0x12, PERM_READ, PERM_SMODE, 0 }, /* R,U=1: S-mode denied (SUM=0) */
        { 0x12, PERM_READ, PERM_UMODE, 1 }, /* R,U=1: U-mode allowed */
        { 0x02, PERM_READ, PERM_SMODE, 1 }, /* R,U=0: S-mode allowed */
        { 0x16, PERM_WRITE, PERM_UMODE, 1 }, /* W,R,U=1 in U-mode allowed */
        { 0x16, PERM_WRITE, PERM_SMODE, 0 }, /* W,R,U=1 in S-mode denied (SUM=0) */
        { 0x18, PERM_EXEC, PERM_UMODE, 1 }, /* X,U=1 in U-mode allowed */
        { 0x08, PERM_EXEC, PERM_UMODE, 0 }, /* X,U=0 in U-mode denied */
    };
    for (unsigned i = 0; i < 8; i++) {
        int got = pte_perm_ok(umode_rows[i].flags,
                              (perm_access_t)umode_rows[i].access,
                              (perm_mode_t)umode_rows[i].mode);
        int want = oracle_perm_ok(umode_rows[i].flags, umode_rows[i].access, umode_rows[i].mode);
        edge_checks++;
        if (got != umode_rows[i].want || got != want) {
            mismatches++;
            printf("EDGE-FAIL flags=0x%02X access=%s mode=%s got=%d want=%d\n",
                   umode_rows[i].flags, access_name(umode_rows[i].access),
                   umode_rows[i].mode ? "U" : "S", got, umode_rows[i].want);
        }
    }
    checks += edge_checks;

    printf("CHECKS: %ld\n", checks);
    printf("MISMATCHES: %ld\n", mismatches);
    printf("CHECKSUM: 0x%016llx\n", (unsigned long long)fnv1a);
    printf("VERDICT: %s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
