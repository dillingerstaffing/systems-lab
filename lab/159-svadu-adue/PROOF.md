<!-- PROOF-HEADER
Checks: 12
Mismatches: 0
Checksum: b824bcbe0f532bcd161b2e5bfdb40aabafaef28fc49869b57ee538a6f87fe935
Environment: QEMU 8.2.2 virt, -bios none, Sv39, bare metal (ubuntu-rv64 gcc 13.2.0, -O2 -Wall -Wextra -Werror)
Verdict: PASS
-->
# PROOF.md, lab/159-svadu-adue

`svadu.c` + `boot.S` prove the central claim of the Svadu extension
firsthand: the Accessed and Dirty bits in a page-table entry are written by
whoever `menvcfg.ADUE` (bit 61) says writes them, hardware or software, and
the trap count tells you which one it was.

12 checks, 0 mismatches: 3 runs x 4 assertions each (store with ADUE=0
traps exactly once with `scause` 15 and software sets A and D, PTE reads
`0x200400c7`; load with ADUE=0 traps exactly once with `scause` 13 and
software sets A only, PTE reads `0x20040047`; store with ADUE=1 traps zero
times and the walker sets A and D itself; load with ADUE=1 traps zero times
and the walker sets A only). The checksum is the SHA-256 of the 12 PASS
lines across the three runs. Clean under `-O2 -Wall -Wextra -Werror`; the
three UART transcripts are byte-identical.

## Build log (genuine)

```
$ make svadu.elf
riscv64-unknown-elf-gcc -Wall -Wextra -Werror -O2 -ffreestanding -nostdlib \
  -nostartfiles -no-pie -fno-pie -fno-pic -march=rv64imac_zicsr -mabi=lp64 \
  -mcmodel=medany -T link.ld -o svadu.elf boot.S svadu.c
build: clean, no warnings
```

## Run output (genuine, all three runs byte-identical)

```
SVADU up
menvcfg=0x2000000000000000 ADUE=1
s-mode up
ph0 store: traps=1 pte=0x00000000200400c7
PASS S1 store with ADUE=0 traps once (scause 15), software sets A|D
ph0 load:  traps=1 pte=0x0000000020040047
PASS S2 load with ADUE=0 traps once (scause 13), software sets A
m-mode: ADUE=1 menvcfg=0x2000000000000000
ph1 store: traps=0 pte=0x00000000200400c7
PASS S3 store with ADUE=1: zero traps, walker set A and D
ph1 load:  traps=0 pte=0x0000000020040047
PASS S4 load with ADUE=1: zero traps, walker set A only
result 4/4 pass
```

## What the harness proves firsthand

- **S1**: with ADUE=0, a store to the test page (VA `0x80100000`, identity
  mapped, PTE starting A=D=0) traps exactly once (`scause` 15, store page
  fault). The S-mode handler sets A and D in the PTE, issues `sfence.vma`,
  and the retried store succeeds. The PTE reads `0x200400c7`.
- **S2**: with ADUE=0, a load traps exactly once (`scause` 13, load page
  fault). The handler sets A only. The PTE reads `0x20040047`. Loads need
  A; stores need A and D, and software respects exactly that split.
- **S3**: with ADUE=1 (flipped by M-mode via `ecall`, since `menvcfg` is
  M-mode-only), the same store traps zero times and the page-table walker
  sets A and D itself with an atomic update. QEMU 8.2.2 implements exactly
  this switch: `adue = menvcfg & MENVCFG_ADUE` gates a `cmpxchg` PTE update,
  else the translate fails into a page fault.
- **S4**: with ADUE=1, the same load traps zero times and the walker sets
  A only, D clear.

## Honest boundaries

- This verifies QEMU 8.2.2's Svadu model, not silicon. The behavior matches
  the published Svadu specification text (ADUE bit 61 WARL; ADUE=0 falls
  back to fault-to-software; ADUE=1 the walker updates atomically), and
  standard supervisor software is written to assume either scheme may be in
  effect.
- One hart, one page, `-bios none`. The `menvcfg` probe at boot refuses to
  run if the hart reports ADUE unwritable.
- The PMP entry granting all of physical memory follows the xv6 `start.c`
  pattern; with MPRV-style S-effective accesses, default PMP denies S-mode
  everything and the first attempts died as access faults (causes 5 and 7)
  before the entry was added. That failure is kept in the source comments,
  not hidden.
- The first version of this rig stayed in M-mode and set MPRV with MPP=S so
  its data accesses would walk the S-mode tables. It wedged: `mret`
  consumes MPP as the return privilege, so the first trap return dropped
  the hart to S-mode and the retried store faulted forever in the wrong
  mode. QEMU's `mret` helper also clears MPRV on a return to a
  less-privileged mode. The honest structure is the one a real kernel uses:
  delegate the page faults to S-mode with `medeleg` and run the test in
  real S-mode.
