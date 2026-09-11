/*
 * Differential test for satp_mode_legal_rv64().
 *
 * The implementation is a range predicate: value 0, or values in the
 * contiguous interval 8-10 (the shape of Table 114's SXLEN=64 rows).
 * The oracle below is structurally different: it enumerates all 16
 * possible 4-bit MODE values as an explicitly hand-transcribed list,
 * each row quoted from the document's Table 114 (SXLEN=64), with the
 * reserved/custom rows marked illegal. No shared table, no shared
 * predicate: the two sides reach the verdict by different means.
 */
#include <stdio.h>
#include <limits.h>

#include "satp_mode_validate.h"

/* ---------------- independent oracle: hand-transcribed from
 * Table 114 (SXLEN=64 rows), "Encoding of satp MODE field" --------- */

/*
 * Row transcription (document text verbatim):
 *   0      Bare   No translation or protection.
 *   1-7      -    Reserved for standard use
 *   8      Sv39 Page-based 39-bit virtual addressing (see Section 4.4).
 *   9      Sv48 Page-based 48-bit virtual addressing (see Section 4.5).
 *   10     Sv57 Page-based 57-bit virtual addressing (see Section 4.6).
 *   11     Sv64 Reserved for page-based 64-bit virtual addressing.
 *   12-13     -    Reserved for standard use
 *   14-15     -    Designated for custom use
 *
 * A value is legal here when it names a defined translation scheme.
 * Sv64 is only "reserved for", to be defined in a later version of
 * the spec, so it is not a defined scheme. Reserved rows and the
 * custom-use rows are not standard schemes.
 */
static const int expected[16] = {
    1, /* 0  Bare: defined scheme, no translation or protection */
    0, /* 1  reserved for standard use */
    0, /* 2  reserved for standard use */
    0, /* 3  reserved for standard use */
    0, /* 4  reserved for standard use */
    0, /* 5  reserved for standard use */
    0, /* 6  reserved for standard use */
    0, /* 7  reserved for standard use */
    1, /* 8  Sv39: page-based 39-bit virtual addressing, defined */
    1, /* 9  Sv48: page-based 48-bit virtual addressing, defined */
    1, /* 10 Sv57: page-based 57-bit virtual addressing, defined */
    0, /* 11 Sv64: reserved for future 64-bit addressing, not defined */
    0, /* 12 reserved for standard use */
    0, /* 13 reserved for standard use */
    0, /* 14 designated for custom use, not a standard scheme */
    0, /* 15 designated for custom use, not a standard scheme */
};

/* ---------------- FNV-1a checksum over verdict bytes ---------------- */

static unsigned long long fnv = 0xCBF29CE484222325ULL;

static void fnv_feed(unsigned char byte)
{
    fnv ^= byte;
    fnv *= 0x100000001B3ULL;
}

/* ---------------- check harness ---------------- */

static unsigned long long nchecks;
static unsigned long long nmismatches;

static void check(unsigned mode, int want)
{
    int got = satp_mode_legal_rv64(mode);

    nchecks++;
    fnv_feed((unsigned char)got);
    printf("mode=%2u got=%d want=%d\n", mode, got, want);
    if (got != want) {
        nmismatches++;
        printf("MISMATCH mode=%u got=%d want=%d\n", mode, got, want);
    }
}

int main(void)
{
    static const unsigned ood[] = { 16, 17, 31, 32, 100, 0xFFFFFFFFU };
    unsigned mode;
    size_t i;

    /* Exhaustive: every value the 4-bit MODE field can hold. */
    for (mode = 0; mode <= 15; mode++)
        check(mode, expected[mode]);

    /* Out-of-domain: the 4-bit field cannot hold these, so no table
     * row covers them and every one must be illegal. */
    for (i = 0; i < sizeof(ood) / sizeof(ood[0]); i++)
        check(ood[i], 0);

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           nchecks, nmismatches, (unsigned long long)fnv);
    return nmismatches == 0 ? 0 : 1;
}
