# PROOF.md, lab/165-amoadd-refcount

`amoadd.w rd, rs2, (rs1)` adds `rs2` into the word at `(rs1)` and writes
the value that word held BEFORE the add into `rd`, as one indivisible
operation. This lab pins down, firsthand, that exact contract, and shows
the applied trick it enables: a lock-free reference-count drop where the
returned old value tells the caller whether it was the last holder.

## Environment

- Toolchain: `riscv64-unknown-elf-gcc 13.2.0` (Ubuntu riscv64-unknown-elf
  13.2.0 / binutils 2.42), `-O2 -march=rv64imac_zicsr -mabi=lp64
  -mcmodel=medany -ffreestanding -nostdlib -nostartfiles`, compiled
  2026-10-06. Source: `main.c` in this directory.
- Emulator: QEMU 8.2.2, `qemu-system-riscv64 -machine virt -bios none`,
  bare-metal binary printing over the virt UART at 0x10000000.
- Related prior work: `riscv-baremetal-demo/src/amo-add-atomicity`
  (10000-iteration single-hart read-modify-write contract check). This lab
  is the applied follow-up: the old-value return used as a refcount drop.

## The listings (from `listing.txt`)

Each `rc_drop` / `rc_acquire` inlines to a single instruction:

```
80000248: 009425af  amoadd.w  a1,s1,(s0)   # drop: s1 holds -1
8000025c: 009425af  amoadd.w  a1,s1,(s0)   # drop
80000270: 009424af  amoadd.w  s1,s1,(s0)   # drop
800002a2: 00b425af  amoadd.w  a1,a1,(s0)   # acquire: a1 holds +1
```

The naive drop, for contrast, is three instructions with the read and
the write split apart:

```
80000218: 4108      lw     a0,0(a0)
8000021a: fff5071b  addiw  a4,a0,-1
8000021e: c398      sw     a4,0(a5)
```

Between that `lw` and `sw` another hart can run its own `lw` and both
write back the same decremented value: one reference is lost. The
`amoadd.w` form has no such window; the ISA defines the read, add, and
write as one atomic operation.

## The mechanism

`amoadd.w` computes `old = mem[rs1]`, writes `mem[rs1] = old + rs2`, and
puts `old` in `rd`. For a reference-count drop, `rs2` is -1. The caller
needs exactly one fact: did this drop take the count to zero? That fact
is the old value. If `old == 1`, the count is now 0 and no other holder
remains, so the caller holding that return value frees the object. No
second read is needed, and a second read would be wrong: it is a
separate, later observation, while the old value in `rd` is the
observation that belongs to this update.

## The run (from `run.txt`)

Count starts at 3, three drops, then one acquire, on the virt UART
under QEMU 8.2.2:

```
count=3
drop: old=3 now=2
drop: old=2 now=1
drop: old=1 now=0
last holder frees
acquire: old=0 now=1
DONE
```

Every `rd` value is the pre-add value, every memory value is the exact
post-add value, and the third drop, the one that observed `old == 1`,
is the one that frees. The acquire from zero returns `old == 0` with the
word now 1.

Scope note: one hart issuing back-to-back `amoadd.w` cannot observe
contention, so this run verifies the instruction's single-hart
read-modify-write contract (old value returned each time, exact final
values). Atomicity under multi-hart contention is the architecture's
guarantee for the A extension, stated in the RISC-V unprivileged ISA
manual, not something a single-hart run can demonstrate.
