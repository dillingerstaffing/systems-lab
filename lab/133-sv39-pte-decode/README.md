# lab/133-sv39-pte-decode

Sv39 leaf page-table-entry decoding: PPN (bits 53:10), RSW (bits 9:8),
and the D/A/G/U/X/W/R/V flag bits, from pure shift/mask identities,
plus an encoder that recombines the decoded fields into the word.

Verified by differential test against a hand-written per-field oracle
that builds each destination field bit-by-bit through a destination
pointer table (structurally different from the shift/mask
implementation, no shared helper code), plus the invariant
encode(decode(pte)) == pte on every case: directed 2^k / 2^k +/- 1
edges for k = 0..53, a reserved-bits row confirming bits 63:54 are
never read, an exhaustive 32-bit sweep (all 4,294,967,296 values),
and 10M fixed-seed splitmix64 raw 64-bit values, 0 mismatches. See
PROOF.md for the full verification record.

Build and verify:

    make            # builds test_pte_decode
    make run        # runs the full differential test (several minutes)
    make bench      # throughput bench (10M decodes, best of 5 reps)
    make clean      # removes built binaries (never committed)
