# PROOF.md, lab/163-lui-addi-rounding

`li` is a pseudo-instruction: the assembler, not the hardware, turns one
constant into one or two real instructions. This lab pins down the exact
split the assembler chooses, firsthand, for five constants.

## Environment

- Toolchain: `riscv64-unknown-elf-gcc 13.2.0` (Ubuntu riscv64-unknown-elf
  13.2.0 / binutils 2.42), `-march=rv64g`, assembled 2026-10-04.
- Source: `li.s` in this directory. Full disassembly: `listing.txt`.

## The listings (from `listing.txt`)

`li a0, 0x7FF` needs no `lui`: 2047 fits a signed 12-bit immediate, so one
instruction does the whole job:

```
0: 7ff00513   addi a0, zero, 2047
```

`li a1, 0x800` is the smallest constant that needs the pair, and the
rounding shows at its barest. The constant's top 20 bits are `0x0`, but the
`lui` carries `0x1`:

```
4: 000015b7   lui   a1, 0x1
8: 8005859b   addiw a1, a1, -2048
```

`li a2, 0x12345678` needs no rounding: its low 12 bits are `0x678`, bit 11
clear, so the pair is the naive split:

```
c: 12345637   lui   a2, 0x12345
10: 6786061b  addiw a2, a2, 1656
```

`li a3, 0x12345EEF` rounds: the low 12 bits are `0xEEF` (bit 11 set), so the
`lui` carries `0x12346`, one more than the constant's `0x12345`, and the low
part is the signed remainder `0xEEF - 0x1000 = -273`:

```
14: 123466b7  lui   a3, 0x12346
18: eef6869b  addiw a3, a3, -273
```

`li a4, -1` is again a single instruction:

```
1c: fff00713  addi a4, zero, -1
```

## Why the rounding is forced

`lui` loads 20 bits into positions 31:12; `addi` adds a 12-bit immediate
into positions 11:0. The immediate is signed: -2048..2047. The constant's
low 12 bits are unsigned: 0..4095. When they reach 0x800 or more, no signed
12-bit field can hold them, so the assembler adds 0x800 to the constant
before splitting (rounding the high 20 bits up exactly when bit 11 of the
low part is set) and emits the low part signed.

Recombination check for the rounded case: `0x12346000 - 273 = 0x12345EEF`.
A reader who treats `-273` as unsigned `0xEEF` computes `0x12346EEF`,
off by exactly `0x1000`.

## The naive pair does not assemble

Hand-writing the unrounded pair for `0x12345EEF`:

```
lui a0, 0x12345
addi a0, a0, 0xEEF
```

`riscv64-unknown-elf-gcc` rejects it (`naive.s` in this directory):

```
naive.s:5: Error: illegal operands `addi a0,a0,0xEEF'
```

`0xEEF` (3823) exceeds the signed 12-bit range, so the assembler never
accepts the pair a hand-writer would guess first. The rounding is not a
choice; the only choice is whether you do it on purpose.

## Verdict

PASS: every listing line matches the stated rule, and the rejection of the
naive pair is reproduced. The rule for the bench: bit 11 of the low 12 bits
set means the `lui` carries one extra and the `addi` goes negative to pay
it back.
