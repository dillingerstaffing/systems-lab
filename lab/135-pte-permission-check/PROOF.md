<!-- PROOF-HEADER
Checks: 96
Mismatches: 0
Checksum: efd3419afd2b56cb
Throughput: 4.423 ns/check at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/135-pte-permission-check

`pte_permits(r, w, x, u, access, mode)` decides whether an access is
legal given an Sv39 leaf PTE's permission bits and the access type and
privilege mode. It returns 1 for legal, 0 for a page fault. The check
is pure boolean logic over the decoded fields, per the privileged
spec's page permission rules:

```
reserved  = (w == 1 && r == 0)          (encodings 010 and 110)
priv_ok   = !(!u && mode == U) && !(u && mode == S)   (SUM assumed 0)
grant     = load ? r : store ? w : exec ? x
legal     = !reserved && priv_ok && grant
```

## Spec grounding

RISC-V Instruction Set Manual, Volume II: Privileged Architecture,
pinned release riscv-isa-release-dc8bf2a-2026-06-26 (riscv-spec.pdf,
verified against the actual release file):

- Section 4.3.1 "Addressing and Memory Protection": the PTE R/W/X
  fields. "Writable pages must also be marked readable; the contrary
  combinations are reserved for future use" (Table 115 lists 010 and
  110 as reserved). "Attempting to fetch an instruction from a page
  that does not have execute permissions raises a fetch page-fault
  exception. Attempting to execute a load ... whose effective address
  lies within a page without read permissions raises a load
  page-fault exception. Attempting to execute a store ... whose
  effective address lies within a page without write permissions
  raises a store page-fault exception." The U bit: "U-mode software
  may only access the page when U=1. If the SUM bit in the sstatus
  register is set, supervisor mode software may also access pages
  with U=1. However, supervisor code normally operates with the SUM
  bit clear, in which case, supervisor code will fault on accesses to
  user-mode pages. Irrespective of SUM, the supervisor may not
  execute code on pages with U=1."
- Section 4.3.2 "Virtual Address Translation Process", step 3:
  "If pte.v=0, or if pte.r=0 and pte.w=1, or if any bits or encodings
  that are reserved for future standard use are set within pte, stop
  and raise a page-fault exception corresponding to the original
  access type."
- Section 4.4 (Sv39): "Bits 9-0 have the same meaning as for Sv32",
  so the R, W, X, U bit rules above apply to Sv39 leaf PTEs.

## Why the rules compose this way

The three rules are independent filters and all must pass: the
reserved-encoding check is a page fault before any permission logic
runs (section 4.3.2 step 3 runs before the U-bit and R/W/X permission
checks in steps 6-8); the U-bit rule decides whether the privilege
mode may touch the page at all; the grant rule decides whether the
PTE's permission bits cover the access type. An (R,W,X) of 000 is a
pointer to the next level of the page table, not a leaf (section
4.3.1); the grant rules above give every access on such an entry the
illegal verdict, matching the leaf check's conclusion that the entry
grants nothing. The multi-level descent itself is out of scope.

## The oracle

The differential oracle is a separately written encoding of the same
spec rules, sharing no helper code with the implementation: the
reserved encodings are enumerated as the two explicit (r,w,x)
patterns instead of the W-implies-R implication, the access grant is
read from a three-slot table indexed by access type instead of a
switch, and the privilege rule is computed as its own predicate
instead of early returns. Under SUM=0 the S-mode/U=1 rule already
covers the spec's "supervisor may not execute code on U=1 pages
irrespective of SUM"; the oracle lists that rule separately anyway,
and both encodings agree on all 96 rows.

## Test corpus

Exhaustive: all 16 R/W/X/U combinations x 3 access types
(load, store, exec) x 2 modes (S, U) = 96 cases. Every case requires
implementation == oracle and the verdict byte in {0,1}. The
reserved-encoding rows (010, 110 in X,W,R order) are illegal in both
encodings, verified by construction across all u/access/mode rows.

Checksum: FNV-1a 64 over every case's verdict byte in fixed loop
order, so the run's outputs are pinned, not just the mismatch count.

Result: checks=96, mismatches=0, checksum folded over all 96
verdicts, verdict PASS.

## Throughput (separate bench, same machine)

`bench_pte_permits.c` cycles through all 96 input combinations for
10,000,000 calls, 5 reps, best rep reported; the accumulator is kept
live through a volatile sink so the loop is not optimized away.
Best observed: 4.423 ns per check at -O2 on this machine.

## Environment

Host. gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64, Linux
7.0.0-26-generic. Built with `gcc -std=c11 -O2 -Wall -Wextra -Werror`.
All builds are warning-free under -Wall -Wextra -Werror. Test
binaries are not committed.

## Build log and run output (genuine)

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_permits test_pte_permits.c pte_permits.c

$ ./test_pte_permits
checks=96 mismatches=0 checksum=efd3419afd2b56cb
elapsed=0.000 s (30.042 ns/check)
```

```
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_permits_o0 test_pte_permits.c pte_permits.c
./test_pte_permits_o0
checks=96 mismatches=0 checksum=efd3419afd2b56cb
elapsed=0.000 s (85.646 ns/check)

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_pte_permits_asan test_pte_permits.c pte_permits.c
./test_pte_permits_asan
checks=96 mismatches=0 checksum=efd3419afd2b56cb
elapsed=0.000 s (43.510 ns/check)
```

```
$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_pte_permits bench_pte_permits.c pte_permits.c
./bench_pte_permits
rep 0: 47385798 ns total, 4.739 ns/check
rep 1: 45702824 ns total, 4.570 ns/check
rep 2: 46401893 ns total, 4.640 ns/check
rep 3: 44231697 ns total, 4.423 ns/check
rep 4: 46086419 ns total, 4.609 ns/check
best=4.423 ns/check
```

All three test builds: 96 checks, 0 mismatches, identical checksum,
exit code 0, no ASan/UBSan findings.

## Scope statement

Verified: the leaf-PTE permission verdict over the full 96-row input
space against an independently written encoding of the spec rules,
with SUM=0 and MXR=0 assumed. Not verified: anything outside the
leaf permission check. The module does not model the V bit, A/D bit
handling, PMP/PMA checks, superpage misalignment, the multi-level
page-table descent (a 000 R/W/X entry is a pointer, reported here as
granting nothing), MXR=1, SUM=1, MPRV, or any actual hardware
behavior. Each of those is a separate mechanism with its own
verification surface.
