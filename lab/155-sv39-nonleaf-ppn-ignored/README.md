# lab/155-sv39-nonleaf-ppn-ignored

Single-step Sv39 page-table-walk classification: `pte_step(pte, level)`
classifies one 64-bit Sv39 PTE at walk level 2, 1, or 0 as FAULT, DESCEND,
or LEAF, following the privileged spec's walk algorithm (pinned release
riscv-isa-release-dc8bf2a-2026-06-26, section 4.3.2 steps 3 and 4; Sv39
reuses it with LEVELS=3, PTESIZE=8 per section 4.4.1). On DESCEND it
returns the next-level table's 44-bit PPN; on LEAF it returns the physical
page base (PPN times 4096).

The backlog item's gloss claimed the walker ignores a non-leaf PTE's PPN.
That is not what the spec says, and this module implements the rule as
the spec actually describes it:

- The non-leaf PPN is the next-level page-table pointer. Step 4 reads it
  on every descend ("let a=pte.ppn*PAGESIZE and go to step 2"); ignoring
  it would break the walk. The differential test cross-checks the decoded
  PPN on every descend (1,069 cases) and the leaf physical address on
  every leaf (64,449 cases).
- The bits the step genuinely never reads for its verdict are G (bit 5)
  and RSW (bits 9-8): G has a defined TLB meaning at non-leaf but no walk
  step consults it, and RSW "shall be ignored by the implementation"
  (section 4.3.1). The test proves this invariance: for each of 294,912
  bases (every combination of level, V, R/W/X, D/A/U, low PPN bits, and
  reserved-bit patterns), all 8 G/RSW patterns yield the identical
  verdict, 0 violations.
- D, A, and U are NOT ignored at non-leaf. Section 4.3.1 reserves them
  for future standard use on non-leaf PTEs, and step 3 faults when any
  reserved-for-future-use bit is set, so a non-leaf PTE with D, A, or U
  set faults (QEMU 8.2.2 implements exactly this on its inner-PTE path).

Verified by differential test against an independently written oracle
(divmod bit-array extraction with no shifts, masks, or bitwise operators,
no shared code or constants with the implementation): 2,359,296
exhaustive cases (level x V x all 8 R/W/X encodings x all 8 D/A/U combos
x 64 low-PPN values x 12 reserved-bit patterns x all 8 G/RSW patterns)
plus 10,000,000 fixed-seed splitmix64 random 64-bit PTEs (seed
0x9E3779B97F4A7C15): 12,359,296 checks, 0 mismatches, with the FNV-1a
checksum of every verdict byte identical across -O2, -O0, and
ASan+UBSan builds. Best observed throughput: 11.026 ns per step at -O2.
See PROOF.md for the full verification record, including the spec
section citations.

## Build and verify

```
make        # build the test binary
make run    # run the differential test
make opt0   # same test compiled -O0
make sanitize  # same test under ASan+UBSan
make bench  # throughput bench at -O2, best of 5 reps
make clean  # remove built binaries
```

## Files

- `pte_step.h`, `pte_step.c`: the classifier and its result type.
- `oracle_step.h`, `oracle_step.c`: the independent per-bit oracle.
- `test_pte_step.c`: exhaustive + random differential test.
- `bench_pte_step.c`: throughput bench.
- `PROOF.md`: the verification record with the spec derivation.
- `full_run.log`: the genuine build log and run output.
