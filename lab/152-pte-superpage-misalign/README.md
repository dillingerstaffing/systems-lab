# lab/152-pte-superpage-misalign

Sv39 superpage misalignment check:
`pte_superpage_misaligned(level, ppn)` reports whether a leaf PTE at
the given translation level must raise a page fault because its PPN is
misaligned for a superpage. A level-1 leaf is a 2 MiB superpage and a
level-2 leaf a 1 GiB superpage, so the low 9*level PPN bits must be
zero; any set bit there faults. A level-0 leaf is a 4 KiB page with no
such constraint, and a level outside {0, 1, 2} is out of domain and
reported explicitly as -1 rather than silently accepted. Per the
privileged spec, section 4.3.2 step 5 (the misaligned-superpage check;
section reference cited from knowledge, no local spec PDF was found on
this machine to re-verify the wording against).

Verified by differential test against an independently written oracle
(a top-down per-bit scan that never constructs a mask, structurally
different from the implementation's mask expression) over directed
rows for all three levels (PPN=0, PPN all-ones, every low bit set, low
bits clear with high bits set, the boundary bit exactly at 9*level, a
single-bit sweep across every position that can matter, and level-0
never faulting) plus out-of-domain level rows, plus 10,000,000
fixed-seed splitmix64 random 44-bit PPNs per level: 30000063 checks,
0 mismatches, with the FNV-1a checksum of every verdict byte identical
across -O2, -O0, and ASan+UBSan builds. Best observed throughput:
4.548 ns per predict at -O2. See PROOF.md for the full verification
record, including the spec section citation and its stated limits.

## Build and verify

```
make        # build the test binary
make run    # run the differential test
make opt0   # same test compiled -O0
make sanitize  # same test under ASan+UBSan
make bench  # throughput bench at -O2, best of 5 reps
make clean  # remove built binaries
```
