# PROOF.md, lab/164-srai-division

`x >> 3` is not `x / 8` when `x` can be negative. This lab pins down,
firsthand, the exact instructions gcc 13.2.0 emits for each, and the
runtime answers from QEMU 8.2.2.

## Environment

- Toolchain: `riscv64-unknown-elf-gcc 13.2.0` (Ubuntu riscv64-unknown-elf
  13.2.0 / binutils 2.42), `-O2 -march=rv64g -mabi=lp64d`, compiled
  2026-10-05. Source: `shr.c` in this directory.
- Emulator: QEMU 8.2.2, `qemu-system-riscv64 -machine virt -bios none`,
  bare-metal binary printing over the virt UART at 0x10000000.

## The listings (from `listing.txt`)

`int shr3(int x){ return x >> 3; }` is a single instruction:

```
74: 40355513   srai  a0, a0, 0x3
78: 00008067   ret
```

`int div8(int x){ return x / 8; }` is four:

```
7c: 41f5579b   sraiw a5, a0, 0x1f
80: 01d7d79b   srliw a5, a5, 0x1d
84: 00a7853b   addw  a0, a5, a0
88: 4035551b   sraiw a0, a0, 0x3
8c: 00008067   ret
```

## The mechanism

`srai` is an arithmetic shift: the vacated top bits are filled with
copies of the sign bit, so for negative values the shift rounds toward
negative infinity (floor). C's `/` on integers truncates toward zero
(C99 onward). The compiler therefore cannot use the bare shift for
division; it repairs it with a bias first.

The repair, instruction by instruction:

1. `sraiw a5, a0, 31` smears the sign bit across the register: 0 for
   non-negative x, all ones (-1) for negative x.
2. `srliw a5, a5, 29` turns that into 0 or 7 (2^3 - 1).
3. `addw a0, a5, a0` adds the bias into x. Negative values move up by 7.
4. `sraiw a0, a0, 3` is the same floor shift as before.

Check: x = -7. Bias 7, so -7 + 7 = 0, shift gives 0, which is what
`-7 / 8` must be under truncation toward zero. Without the bias the
shift gives -1, the wrong answer for division. General shape: to turn a
floor shift into a truncating division by 2^k, add 2^k - 1 to negative
values first.

## The run (from `run.txt`)

Thirteen inputs, -19 through 19, on the virt UART under QEMU 8.2.2:

```
-19 >>3=-3  /8=-2
-16 >>3=-2  /8=-2
-9  >>3=-2  /8=-1
-8  >>3=-1  /8=-1
-7  >>3=-1  /8=0
-1  >>3=-1  /8=0
0   >>3=0   /8=0
1   >>3=0   /8=0
7   >>3=0   /8=0
8   >>3=1   /8=1
9   >>3=1   /8=1
16  >>3=2   /8=2
19  >>3=2   /8=2
```

Every non-negative input agrees; every negative non-multiple of 8
disagrees by exactly the rounding difference. The one-instruction path
and the four-instruction path diverge exactly where the two rounding
rules say they must.

## What it buys you at the bench

Reading a disassembly: a shift preceded by a sign-smear and an add is
the compiler dividing by a power of two; a bare `srai` is just a shift,
with floor rounding. Writing C: never hand-rewrite `x / 8` as `x >> 3`
when `x` can be negative, because that changes the rounding. The
compiler already performs the shift optimization itself, bias included.
Use the shift when floor is what you want (common in fixed-point and
bit-index code); write the division when truncation is what you want,
and let the compiler add the bias.
