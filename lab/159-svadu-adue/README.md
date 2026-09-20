# lab/159-svadu-adue

Svadu: who writes the A and D bits in a page-table entry.

Every page-table entry carries two bookkeeping bits the OS reads when it
chooses eviction victims: Accessed (has this page been touched?) and Dirty
(has it been written, so it must be written back?). Someone has to maintain
them, and RISC-V permits two answers. Without the Svadu extension, the
page-table walker raises a page fault and software sets the bits. With
Svadu, enabled by `menvcfg.ADUE` (bit 61), the walker sets them itself with
an atomic update and no trap fires. Simple implementations may skip the
atomic walker-update logic, so the architecture makes it a runtime bit
instead of a fixed property of the chip; standard supervisor software
assumes either scheme may be in effect.

This lab is M-mode bare metal on QEMU 8.2.2 (`virt`, `-bios none`, Sv39).
M-mode builds the page tables, grants all of physical memory through one
PMP entry (the xv6 `start.c` pattern), delegates load/store page faults to
S-mode with `medeleg`, then drops to S-mode with `mret`. The test runs in
real S-mode against one identity-mapped page at `0x80100000` whose PTE
starts with A and D clear. `menvcfg` is writable only in M-mode, so S-mode
uses `ecall` to ask M-mode to flip ADUE between the two phases.

What the harness proves firsthand (4/4, 3 byte-identical runs):

- **ADUE=0, store**: traps exactly once (`scause` 15). The S-mode handler
  sets A and D, `sfence.vma`, retry succeeds. PTE: `0x200400c7`.
- **ADUE=0, load**: traps exactly once (`scause` 13). The handler sets A
  only. PTE: `0x20040047`.
- **ADUE=1, store**: zero traps. The walker sets A and D itself.
- **ADUE=1, load**: zero traps. The walker sets A only.

Loads need A, stores need A and D, and the trap count tells you who did
the writing.

## Files

- `boot.S`: M-mode entry, 256-byte register save/restore trap entries for
  M-mode (`mret`) and S-mode (`sret`), stack.
- `svadu.c`: page-table build, PMP setup, `medeleg` delegation, the S-mode
  A/D fixup handler, the M-mode ADUE-flip `ecall` handler, the four checks.
- `link.ld`: load at `0x80000000` (boot object first, per the `-kernel`
  load-address rule).
- `run1.txt`, `run2.txt`, `run3.txt`: the three UART transcripts.
- `PROOF.md`: build log, full run output, honest boundaries (QEMU model,
  not silicon; the MPRV dead end; the PMP access-fault lesson).

## Build and run

```
make runs   # builds svadu.elf, runs it 3x under QEMU, checks byte-identical
```

The harness powers the machine off through the SiFive test finisher, so a
clean QEMU exit means 4/4.
