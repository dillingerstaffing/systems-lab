<!-- PROOF-HEADER
Checks: 12359296
Mismatches: 0
Checksum: 642afcdb30902952
Throughput: 11.026 ns/step at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/155-sv39-nonleaf-ppn-ignored

`pte_step(pte, level)` classifies one 64-bit Sv39 page table entry at walk
level 2, 1, or 0 as FAULT, DESCEND, or LEAF. On DESCEND it reports the
44-bit PPN of the next-level page table; on LEAF it reports the physical
page base (PPN times 4096). The implementation is mask-based; the oracle
is an independently written per-bit decoder that explodes the PTE by
repeated division (no shifts, no masks, no bitwise operators, no shared
constants).

## The tested rule, exactly

For one PTE at level 2, 1, or 0 (pinned spec
riscv-isa-release-dc8bf2a-2026-06-26, section 4.3.2 steps 3-4; Sv39
reuses this algorithm with LEVELS=3, PTESIZE=8 per section 4.4.1):

1. v=0 faults. The reserved r=0,w=1 encoding (XWR 010 and 110) faults.
   Any reserved high bit set (bits 63-54: N, PBMT, 60-54; Svnapot and
   Svpbmt not implemented in this module's scope) faults.
2. Otherwise the PTE is valid. If r=1 or x=1 it is a leaf: the physical
   page base is PPN*4096.
3. Otherwise it is a pointer to the next level. A non-leaf PTE with D, A,
   or U set faults (section 4.3.1 reserves those bits for future
   standard use on non-leaf PTEs; step 3 faults on reserved-for-future-
   use bits set). A pointer at level 0 faults. Otherwise the walk
   descends to pte.ppn*PAGESIZE.
4. G (bit 5) and RSW (bits 9-8) are never read for the verdict: G has a
   defined TLB meaning at non-leaf but no walk step consults it, and the
   spec says the implementation shall ignore RSW (section 4.3.1).

## A correction against the backlog gloss

The backlog item's stated fundamental truth was that "the Sv39 walker
ignores a non-leaf PTE's PPN field." I checked this against the pinned
spec before building, and it is wrong in two ways:

1. The non-leaf PPN is not ignored. Step 4 reads it on every descend:
   "let a=pte.ppn*PAGESIZE and go to step 2." The PPN is the next-level
   page-table pointer; a walker that ignored it could not descend. This
   module reads the PPN on every descend and every leaf, and the test
   cross-checks the decoded value against the independent oracle on all
   1,069 descends and 64,449 leaves.
2. The gloss's suggested ignored set (D/A/U) is also wrong. D, A, and U
   on a non-leaf PTE are "reserved for future standard use" (section
   4.3.1), and step 3 stops the walk with a page fault when "any bits or
   encodings that are reserved for future standard use are set within
   pte." So a non-leaf PTE with D, A, or U set faults; those bits flip
   the verdict and are not ignored. QEMU 8.2.2 implements exactly this on
   its inner-PTE path (`if (pte & (PTE_D | PTE_A | PTE_U | PTE_ATTR))
   return TRANSLATE_FAIL;` in `get_physical_address`,
   target/riscv/cpu_helper.c at tag v8.2.2).

The bits the step genuinely never consults for its verdict are G (bit 5)
and RSW (bits 9-8). The module name keeps the backlog item's
`lab/155-sv39-nonleaf-ppn-ignored` label; the rule implemented and proved
is the spec's rule, stated above.

## Spec grounding

- Section 4.3.1 (Sv32 PTE format; Sv39 section 4.4.1 states bits 9-0
  have the same meaning): "When all three [R, W, X] are zero, the PTE is
  a pointer to the next level of the page table; otherwise, it is a leaf
  PTE." "For non-leaf PTEs, the D, A, and U bits are reserved for future
  standard use." "The RSW field is reserved for use by supervisor
  software; the implementation shall ignore this field." "For non-leaf
  PTEs, the global setting implies that all mappings in the subsequent
  levels of the page table are global."
- Section 4.3.2, step 3: "If pte.v=0, or if pte.r=0 and pte.w=1, or if
  any bits or encodings that are reserved for future standard use are set
  within pte, stop and raise a page-fault exception."
- Section 4.3.2, step 4: "If pte.r=1 or pte.x=1, go to step 5. Otherwise,
  this PTE is a pointer to the next level of the page table. Let i=i-1.
  If i<0, stop and raise a page-fault exception. Otherwise, let
  a=pte.ppn*PAGESIZE and go to step 2."
- Section 4.4.1: Sv39 PTE figure 65 (PPN in bits 53-10); bits 63, 62-61,
  60-54 reserved, fault when set (Svnapot/Svpbmt unimplemented scope);
  "The algorithm for virtual-to-physical address translation is the same
  as in Section 4.3.2, except LEVELS equals 3 and PTESIZE equals 8."

## How it was verified

Differential test of `pte_step` against `oracle_step`:

- Exhaustive cross product, 2,359,296 PTEs: level in {0,1,2} (3) x V (2)
  x all 8 R/W/X encodings x all 8 D/A/U combinations x PPN bits 15-10
  (64 values, small-width exhaustive) x reserved bits 63-54 as clear,
  each of the 10 single bits set, and all set (12 patterns) x all 8
  G/RSW patterns. Every check compares the verdict; on DESCEND it also
  compares next_ppn, on LEAF leaf_phys (64,449 leaf physical-address
  cross-checks and 1,069 descend PPN cross-checks total, 0 mismatches).
- 10,000,000 fixed-seed splitmix64 random 64-bit PTEs (seed
  0x9E3779B97F4A7C15, the splitmix64 reference constant), each with a
  PRNG-derived level in {0,1,2}.
- Ignored-bit invariance, checked explicitly: the exhaustive matrix is
  grouped into 294,912 bases (every combination of level, V, R/W/X,
  D/A/U, low PPN, reserved pattern with G=RSW=0); for each base all 8
  G/RSW patterns must yield the identical verdict in both the
  implementation and the oracle. Violations: 0.
- Total: 12,359,296 impl-vs-oracle checks, 0 mismatches. FNV-1a checksum
  over all verdict bytes: 642afcdb30902952, identical across -O2, -O0,
  and ASan+UBSan builds.
- Throughput bench at -O2 (best of 5, N=2,000,000 fixed-seed PTEs):
  11.026 ns per step.

## Scope (stated, not assumed)

Walk-step classification only: the module answers FAULT / DESCEND /
LEAF for one PTE at a known level. Leaf permission and attribute checks
(steps 5-9: superpage alignment, U/SUM, R/W/X against the access type,
A/D update policy) need the access type and privilege mode and are
covered by sibling labs, not here. Svnapot and Svpbmt are treated as not
implemented, so bits 63-61 fault when set. PMP/PMA checks are out of
scope.

## Genuine build log and run output

Captured 2026-09-11 by running the Makefile targets in sequence
(full log also saved as full_run.log in this directory):

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_step test_pte_step.c pte_step.c oracle_step.c
$ make run
./test_pte_step
checks=12359296 mismatches=0 checksum=642afcdb30902952
exhaustive=2359296 random=10000000
leaf_phys_cross_checks=64449
descend_ppn_cross_checks=1069
ignored_invariance_bases=294912 violations=0
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_step_o0 test_pte_step.c pte_step.c oracle_step.c
./test_pte_step_o0
checks=12359296 mismatches=0 checksum=642afcdb30902952
exhaustive=2359296 random=10000000
leaf_phys_cross_checks=64449
descend_ppn_cross_checks=1069
ignored_invariance_bases=294912 violations=0
$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_pte_step_asan test_pte_step.c pte_step.c oracle_step.c
./test_pte_step_asan
checks=12359296 mismatches=0 checksum=642afcdb30902952
exhaustive=2359296 random=10000000
leaf_phys_cross_checks=64449
descend_ppn_cross_checks=1069
ignored_invariance_bases=294912 violations=0
$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_pte_step bench_pte_step.c pte_step.c
./bench_pte_step
11.026 ns/step at -O2 (best of 5, N=2000000, sink=5885)
```
