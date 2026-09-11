<!-- PROOF-HEADER
Checks: 34402
Mismatches: 0
Checksum: 6266f98fc9acd43b
Throughput: 12.058 ns/check at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/149-pte-permission-and

`walk_permits(levels, nlevels, access, mode)` decides whether an
access is legal given one decoded PTE per Sv39 walk level. It
returns 1 for legal, 0 for a page fault. The check is pure boolean
logic over the decoded fields, per the privileged spec's
page-table walk rules.

## The tested rule, exactly

The access is legal iff, for the levels consumed by the walk:

1. no level holds the reserved r=0, w=1 encoding (faults at any
   level);
2. the walk ends at the first level with r=1 or x=1 (that entry
   is the leaf);
3. every level before that leaf is a pure pointer,
   (R,W,X) == (0,0,0);
4. the leaf's permission bits permit the access: the (X,W,R)
   encodings 010 and 110 are reserved and fault, U=0 pages are
   inaccessible from U-mode, U=1 pages are inaccessible from
   S-mode with SUM=0, and load needs R, store needs W, exec
   needs X.

If the levels run out before any leaf is reached, the walk is
malformed and the access faults.

The backlog gloss ("effective permission is the AND of the leaf
PTE bits across the walk") is not what the spec says, and this
module does not implement it. The spec decides permission at the
single leaf where the walk terminates; non-leaf entries hold no
permissions, so there is no permission bit to AND at those
levels. The true multi-level conjunction is over the walk's
validity: every level must be either a valid pointer or the
terminating leaf, and the leaf must grant the access. One denied
level (a reserved encoding, a pointer chain that runs out, or a
leaf that denies) denies the access.

## Spec grounding

RISC-V Instruction Set Manual, Volume II: Privileged
Architecture, pinned release riscv-isa-release-dc8bf2a-2026-06-26
(the release cited by the sibling Sv39 labs):

- Section 4.3.2 "Virtual Address Translation Process", step 3:
  "If pte.v=0, or if pte.r=0 and pte.w=1, or if any bits or
  encodings that are reserved for future standard use are set
  within pte, stop and raise a page-fault exception
  corresponding to the original access type." This applies at
  every level of the walk, so a non-leaf entry is a valid
  pointer only when (R,W,X) == (0,0,0).
- Section 4.3.2, step 5: "If pte.r=1 or pte.x=1, go to step 6.
  Otherwise, this PTE is a pointer to the next level of the
  page table... go to step 2." The walk terminates at the first
  leaf; deeper entries are never consulted.
- Section 4.3.1 "Addressing and Memory Protection", Table 115:
  "Writable pages must also be marked readable; the contrary
  combinations are reserved for future use" (010 and 110 are
  reserved). Access-type grants: a load without read permission
  raises a load page fault, a store without write permission a
  store page fault, an instruction fetch without execute
  permission a fetch page fault. The U bit: "U-mode software may
  only access the page when U=1"; with SUM clear, supervisor
  faults on U=1 pages. Section 4.4 (Sv39): bits 9-0 carry the
  same meaning as Sv32, so these rules apply to every Sv39
  level.

## The oracle

The differential oracle is a separately written encoding of the
same spec rules, sharing no helper code with the
implementation: it locates the terminating level in a first
scan, then checks the reserved encodings across the consumed
levels against an explicit two-pattern table instead of the
w-implies-r implication, reads the leaf's grant from a
three-slot table indexed by access type instead of a switch,
and computes the privilege rule as a boolean predicate instead
of early returns. Under SUM=0 the S-mode/U=1 rule already covers
the spec's "supervisor may not execute code on U=1 pages
irrespective of SUM"; the oracle lists that rule separately
anyway, and both encodings agree on all 34402 rows.

## Test corpus

Exhaustive: 1-, 2-, and 3-level walks (Sv39 has three levels),
every level taking all 16 R/W/X/U combinations, times 3 access
types (load, store, exec) times 2 modes (S, U):
96 + 1536 + 24576 = 26208 cases. Fixed-seed random corpus
(seed 149, xorshift32): 8192 further walks of 1 to 3 levels with
uniformly random bit fields. Contract edges: nlevels=0 and a NULL
levels pointer both return the fault verdict (2 fixed cases).
Every case requires implementation == oracle and the verdict
byte in {0,1}.

Checksum: FNV-1a 64 over every case's verdict byte in fixed loop
order, so the run's outputs are pinned, not just the mismatch
count.

Result: checks=34402, mismatches=0, checksum folded over all
34402 verdicts, verdict PASS.

## Throughput (separate bench, same machine)

`bench_walk_permits.c` cycles through the 3-level exhaustive
space (24576 inputs) for 10,000,000 calls, 5 reps, best rep
reported; the accumulator is kept live through a volatile sink
so the loop is not optimized away. Best observed: 12.058 ns per
check at -O2 on this machine.

## Environment

Host. gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64, Linux
7.0.0-26-generic. Built with `gcc -std=c11 -O2 -Wall -Wextra
-Werror`. All builds are warning-free under -Wall -Wextra
-Werror. Test binaries are not committed.

## Build log and run output (genuine)

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_walk_permits test_walk_permits.c walk_permits.c

$ ./test_walk_permits
checks=34402 mismatches=0 checksum=6266f98fc9acd43b
elapsed=0.001 s (27.668 ns/check)
header-checks=34402
header-mismatches=0
header-checksum=6266f98fc9acd43b
```

```
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_walk_permits_o0 test_walk_permits.c walk_permits.c
./test_walk_permits_o0
checks=34402 mismatches=0 checksum=6266f98fc9acd43b
elapsed=0.003 s (87.949 ns/check)
header-checks=34402
header-mismatches=0
header-checksum=6266f98fc9acd43b

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_walk_permits_asan test_walk_permits.c walk_permits.c
./test_walk_permits_asan
checks=34402 mismatches=0 checksum=6266f98fc9acd43b
elapsed=0.008 s (218.627 ns/check)
header-checks=34402
header-mismatches=0
header-checksum=6266f98fc9acd43b
```

```
$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_walk_permits bench_walk_permits.c walk_permits.c
./bench_walk_permits
rep 0: 120579708 ns total, 12.058 ns/check
rep 1: 122664015 ns total, 12.266 ns/check
rep 2: 123176845 ns total, 12.318 ns/check
rep 3: 122046498 ns total, 12.205 ns/check
rep 4: 122618527 ns total, 12.262 ns/check
best=12.058 ns/check
header-throughput=12.058 ns/check
sink=2119985360 (accumulator kept live)
```

All three test builds: 34402 checks, 0 mismatches, identical
checksum, exit code 0, no ASan/UBSan findings.

## Scope statement

Verified: the walk-permission verdict over the full 26208-row
exhaustive space (1- to 3-level walks, all R/W/X/U combinations
per level, all access types, both modes) plus 8192 fixed-seed
random walks and 2 malformed-input edge cases, against an
independently written encoding of the spec's walk rules, with
SUM=0, MXR=0, and V=1 at every level assumed. Not verified:
anything outside the walk permission check. The module does not
model the V bit, A/D bit handling, PMP/PMA checks, superpage
misalignment, MPRV, SUM=1, MXR=1, or any actual hardware
behavior. U bits on non-leaf entries are ignored, matching the
spec's leaf-only permission checks. Each of those is a separate
mechanism with its own verification surface.
