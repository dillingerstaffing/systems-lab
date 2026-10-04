# lab/163-lui-addi-rounding

What two instructions the RISC-V assembler emits for one 32-bit constant,
and the rounding rule hiding inside the pair.

`li.s` loads five constants (`0x7FF`, `0x800`, `0x12345678`, `0x12345EEF`,
`-1`); `listing.txt` is the firsthand objdump from
`riscv64-unknown-elf-gcc 13.2.0 -march=rv64g`. `naive.s` is the hand-written
pair the assembler refuses, with its error.

Rule under test: `addi`'s 12-bit immediate is signed (-2048..2047), so when
the constant's low 12 bits have bit 11 set (0x800..0xFFF), the assembler
rounds the `lui`'s high 20 bits up by one and writes the low part as the
signed remainder. `PROOF.md` walks each listing line.
