<!-- PROOF-HEADER
Checks: 30000063
Mismatches: 0
Checksum: 5cb66b0d233f83a1
Throughput: 4.548 ns/predict at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/152-pte-superpage-misalign

`pte_superpage_misaligned(level, ppn)` answers one question about an
Sv39 leaf PTE: given the translation level where the walk terminated
and the 44-bit PPN field, must the translation raise a page fault
because the PPN is misaligned for a superpage? It returns 1 for a
fault, 0 for no fault, and -1 when `level` is outside {0, 1, 2}.

## The tested rule, exactly

- Level 2 leaf: 1 GiB superpage; the low 18 PPN bits must be zero.
  Any set bit in PPN bits 17:0 faults.
- Level 1 leaf: 2 MiB superpage; the low 9 PPN bits must be zero.
  Any set bit in PPN bits 8:0 faults.
- Level 0 leaf: 4 KiB page; all 44 PPN bits are significant, so no
  PPN value is misaligned and the function returns 0.
- Level outside {0, 1, 2}: out of domain, reported explicitly as -1,
  never silently accepted as a translation verdict.
- The boundary bit exactly at position 9*level (bit 9 for level 1,
  bit 18 for level 2) belongs to the superpage's base address, not to
  the in-superpage offset, so setting it alone must not fault.

## Spec grounding

RISC-V Instruction Set Manual, Volume II: Privileged Architecture,
section 4.3.2 (Virtual Address Translation Process), step 5: the
misaligned-superpage check. The rule, as implemented and tested: when
the walk terminates at a leaf PTE at level i > 0, the leaf maps a
superpage, and the low 9*i bits of its PPN must be zero, otherwise the
translation raises a page fault. A level-0 leaf maps a 4 KiB page and
carries no such constraint.

Stated plainly: no local copy of the privileged spec PDF was found on
this machine (the workspace was searched for privileged-spec PDFs and
none exists here), so the section number and the rule's wording above
are cited from knowledge and were not re-verified against a local
document. What was verified exhaustively is the logical rule itself:
the implementation and the independent oracle agree on 30,000,063
(level, PPN) pairs with 0 mismatches.

## How it was verified

The implementation (pte_superpage.c) builds the low-bit mask
`(1ULL << (9*level)) - 1` and tests the masked PPN. The oracle
(test_pte_superpage.c) is structurally different: it scans the 44-bit
PPN field one bit at a time from the top down and reports misalignment
the moment it finds a set bit whose position lies below 9*level. No
mask is ever constructed in the oracle, and it decides by positional
comparison rather than by one bitwise expression.

Test space, 30000063 checks total, 0 mismatches:

- 53 directed rows: for each of the three levels, PPN=0, PPN with all
  44 bits set, every low 9*level bit set, low bits clear with all high
  bits set, the boundary bit exactly at 9*level (plus a high bit,
  must not fault), and a single-bit sweep across every position that
  can matter (bits 0..18 for level 2, bits 0..9 for level 1, bits 0..9
  for level 0, which must never fault).
- 10 out-of-domain rows: levels -1, 3, 4, 100, -100 with PPN=0 and
  PPN all-ones; both sides must return -1.
- 30,000,000 random rows: 10,000,000 fixed-seed splitmix64 44-bit
  PPNs per level (seed 0x123456789ABCDEF0, reseeded per level).

Every verdict byte feeds an FNV-1a 64-bit checksum. The checksum is
identical across the -O2, -O0, and ASan+UBSan builds:

```
checks=30000063 mismatches=0 checksum=5cb66b0d233f83a1
```

Throughput, best of 5 reps at -O2 (50,000,000 predicts, each fed a
fresh splitmix64 44-bit PPN cycling through levels 0..2):

```
4.548 ns/predict at -O2 (best of 5, N=50000000)
```

## Scope (stated, not assumed)

This module implements only the misalignment rule. It does not walk a
page table, check V/R/W/X/U bits, or apply A/D updates; those live in
the sibling labs (135/144 permission, 149 walk-level permission, 150
A/D prediction, 136 address formation, 132 VPN extraction). The `ppn`
argument carries the 44-bit PPN field in its low 44 bits; higher bits
are ignored. Out-of-domain levels return -1, which is a contract
signal, not a translation verdict.

## Genuine build log and run output

Captured 2026-09-11 by running the Makefile targets in sequence
(full log also saved as full_run.log in this directory):

```
$ make clean && make && make run && make opt0 && make sanitize && make bench
$ make clean && make && make run && make opt0 && make sanitize && make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_superpage test_pte_superpage.c pte_superpage.c
./test_pte_superpage
checks=30000063 mismatches=0 checksum=5cb66b0d233f83a1
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_superpage_o0 test_pte_superpage.c pte_superpage.c
./test_pte_superpage_o0
checks=30000063 mismatches=0 checksum=5cb66b0d233f83a1
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_pte_superpage_asan test_pte_superpage.c pte_superpage.c
./test_pte_superpage_asan
checks=30000063 mismatches=0 checksum=5cb66b0d233f83a1
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_pte_superpage bench_pte_superpage.c pte_superpage.c
./bench_pte_superpage
4.548 ns/predict at -O2 (best of 5, N=50000000)
```
