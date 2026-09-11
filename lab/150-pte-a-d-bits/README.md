# lab/150-pte-a-d-bits

Sv39 walk-level accessed/dirty prediction:
`ad_predict(tree, va, access, mode)` walks a scripted three-level
page-table tree for one access and predicts exactly which PTE gains
the A bit and which gains the D bit, mutating the tree the way the
spec's hardware-update scheme would. Per the privileged spec
(pinned release riscv-isa-release-dc8bf2a-2026-06-26, sections 4.3.1,
4.3.2 step 9, 4.4.1), the A/D update applies to the terminating leaf
PTE only: a completed access sets A, a completed store also sets D,
and non-leaf PTEs are never touched (their A/D bits are reserved for
future standard use). The backlog gloss that A is set on every
traversed PTE including non-leaf levels is not what the spec says, so
this module implements and states the rule as the spec actually
describes it. A faulting walk touches no PTE.

Verified by differential test against an independently written oracle
(recursive walk over a flat PTE array with inline bit operations, no
shared helper code) over 1152 exhaustive leaf-flag/access/mode cases
(leaf at level 2, 1, and 0; all 64 R/W/X/U/A/D combinations), 18
invalid-leaf cases, and 23 scripted-sequence accesses over two richer
trees (mixed-level valid mappings; reserved encodings, misaligned
superpage, dangling pointers, exhausted pointer chains), plus
builder-equivalence and post-sequence full-tree state checks: 1215
checks, 0 mismatches, with the FNV-1a checksum of every verdict byte
identical across -O2, -O0, and ASan+UBSan builds. Best observed
throughput: 5.840 ns per predict at -O2. See PROOF.md for the full
verification record, including the spec section citations.

## Build and verify

```
make        # build the test binary
make run    # run the differential test
make opt0   # same test compiled -O0
make sanitize  # same test under ASan+UBSan
make bench  # throughput bench at -O2, best of 5 reps
make clean  # remove built binaries
```
