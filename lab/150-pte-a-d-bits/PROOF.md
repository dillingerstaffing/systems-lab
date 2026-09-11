<!-- PROOF-HEADER
Checks: 1215
Mismatches: 0
Checksum: 15d19d25bf280794
Throughput: 5.840 ns/predict at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/150-pte-a-d-bits

`ad_predict(tree, va, access, mode)` walks a scripted Sv39
three-level page-table tree for one access and predicts exactly which
PTE gains the A bit and which gains the D bit, mutating the tree the
way the spec's hardware-update scheme would. It returns whether the
walk faulted, the identity of the terminating leaf PTE, and which of
A/D transitioned 0 to 1 on that access.

## The tested rule, exactly

For each access:

1. The walk descends from level 2 while entries are valid pure
   pointers. At any level, v=0 or the reserved r=0, w=1 encoding
   faults the walk; the first entry with r=1 or x=1 terminates the
   walk as the leaf.
2. At the leaf: a misaligned superpage (level > 0 with nonzero low
   PPN bits) faults; the U bit is checked against the mode (SUM=0);
   the (X,W,R) encodings 010 and 110 are reserved and fault; load
   needs R, store needs W, exec needs X.
3. If every check passes, the terminating leaf PTE gains A=1 (when it
   was clear), and a completed store additionally gains D=1 (when it
   was clear). Non-leaf PTEs are never modified: their A/D bits are
   reserved for future standard use and hardware does not set them.
4. A faulting walk touches no PTE. A and D bits are never cleared.

## A correction against the backlog gloss

The backlog item for this lab said the A bit is "set on any access
traversing the PTE, including non-leaf levels". That is not what the
pinned spec says, so this module does not implement it. Section 4.3.2
step 9 performs the A/D update on the terminating leaf PTE only, and
section 4.3.1 states: "For non-leaf PTEs, the D, A, and U bits are
reserved for future standard use. Until their use is defined by a
standard extension, they must be cleared by software for forward
compatibility." The module implements and states the rule as the spec
actually describes it (the same pattern as lab/149, where the
backlog's "AND of the leaf PTE bits" gloss was corrected against the
spec).

## Spec grounding

RISC-V Instruction Set Manual, Volume II: Privileged Architecture,
pinned release riscv-isa-release-dc8bf2a-2026-06-26 (the release cited
by the sibling Sv39 labs), verified against the release PDF
downloaded from the official riscv-isa-manual GitHub release page:

- Section 4.4.1: the Sv39 PTE format; bits 9-0 have the same meaning
  as for Sv32; any level of PTE may be a leaf (4 KiB page, 2 MiB
  megapage, 1 GiB gigapage).
- Section 4.3.2, steps 2-4: the walk loop; step 3 faults on v=0 or
  r=0,w=1 at any level; step 4 ends the walk at the first entry with
  r=1 or x=1.
- Section 4.3.2, steps 5, 6, 8: misaligned superpage check, U-bit
  check, and R/W/X permission check with the reserved 010/110
  encodings.
- Section 4.3.2, step 9 (hardware-update scheme, Svade absent): "If
  the values match, set pte.a to 1 and, if the original memory access
  is a store, also set pte.d to 1. Then store pte to the PTE at
  address a+va.vpn[i]xPTESIZE." The address is the terminating leaf
  PTE at level i.
- Section 4.3.1: "When a virtual page is accessed and the A bit is
  clear, the PTE is updated to set the A bit. When the virtual page
  is written and the D bit is clear, the PTE is updated to set the D
  bit." And: "For non-leaf PTEs, the D, A, and U bits are reserved
  for future standard use."

## How it was verified

The implementation (pte_ad.c: iterative walk over a struct tree, field
extraction via a bit() helper) was differential-tested against an
independently written oracle (test_pte_ad.c: recursive walk over a
flat uint64_t array with a parallel child array, every field extracted
inline with shifts and masks, no shared helper code). Each side builds
its own copy of every scripted tree, and compare_build() checks the
two trees bit-identical before any access runs.

Test space, 1215 checks total, 0 mismatches:

- 1152 exhaustive sweep cases: the leaf placed at level 2, 1, and 0
  in turn; all 64 R/W/X/U/A/D flag combinations; all 3 access types
  (load, store, exec); both modes (S, U). For each, the predicted
  fault flag, leaf identity, and A/D gains were compared, plus the
  resulting leaf PTE value.
- 18 invalid-leaf cases: a V=0 leaf at each level, all access types
  and modes; every walk must fault and touch nothing.
- 23 scripted-sequence accesses over two richer trees: tree A (mixed
  levels: a level-2 gigapage leaf, a level-1 megapage leaf, three
  level-0 leaves with varied flags including one with A=D=1 preset)
  exercised A-then-D gain ordering, repeated accesses gaining
  nothing, faulting writes to read-only leaves, and mode mismatches;
  tree B (reserved r=0,w=1 entry, misaligned megapage, dangling
  pointer, V=0 entry, pointer chain running out at level 0) exercised
  faulting walks that must leave every PTE untouched.
- 11 builder-equivalence checks (the two tree representations
  bit-identical) and 11 post-sequence full-tree A/D state checks.

Every verdict byte feeds an FNV-1a 64-bit checksum. The checksum is
identical across the -O2, -O0, and ASan+UBSan builds:

```
checks=1215 mismatches=0 checksum=15d19d25bf280794
```

Throughput, best of 5 reps at -O2 (2,000,000 predictions, each doing
the full walk plus the A/D update on a gigapage leaf):

```
5.840 ns/predict at -O2 (best of 5, N=2000000)
```

## Scope (stated, not assumed)

SUM=0, MXR=0, Svade not implemented (hardware-update scheme only), no
MPRV, no PMP/PMA checks, no Shadow Stack rules, no speculative A
updates, no TLB caching effects, inputs are canonical Sv39 virtual
addresses. The model predicts updates for architecturally completed
accesses only.

## Genuine build log and run output

Captured 2026-09-11 by running the Makefile targets in sequence
(full log also saved as full_run.log in this directory):

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_ad test_pte_ad.c pte_ad.c
$ make run
./test_pte_ad
checks=1215 mismatches=0 checksum=15d19d25bf280794
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_ad_o0 test_pte_ad.c pte_ad.c
./test_pte_ad_o0
checks=1215 mismatches=0 checksum=15d19d25bf280794
$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_pte_ad_asan test_pte_ad.c pte_ad.c
./test_pte_ad_asan
checks=1215 mismatches=0 checksum=15d19d25bf280794
$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_pte_ad bench_pte_ad.c pte_ad.c
./bench_pte_ad
5.840 ns/predict at -O2 (best of 5, N=2000000)
```
