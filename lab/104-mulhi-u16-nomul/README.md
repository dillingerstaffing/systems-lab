# lab/104-mulhi-u16-nomul

`mulhi_u16(a, b)` in `mulhi_u16.h`: the high 16 bits of the 32-bit
product of two `uint16_t` values, computed with shifts and adds only.
No multiply operator appears anywhere in the implementation, not even
a 32-bit one.

Construction. Write b in binary, b = sum_{i=0..15} bit_i(b) * 2^i.
Then

    a * b = sum_{i=0..15} bit_i(b) * (a << i),

so the product is the sum of the shifted copies of a at exactly the
positions where b has a 1 bit. The loop walks the bits of b low to
high, keeps x = a << i, and adds x into a 32-bit accumulator when bit
i of b is set, returning acc >> 16. The accumulator never exceeds
(2^16 - 1)^2 < 2^32 and x never exceeds a << 16 < 2^32, so every shift
and add is well-defined on unsigned values. The native multiply
`((uint32_t)a * b) >> 16` exists only in the test's oracle
(`ref_high` in `test_mulhi_u16.c`).

Verified by `test_mulhi_u16.c`, differential against the native
32-bit product oracle:

- Exhaustive: all 4,294,967,296 pairs of 16-bit inputs (the complete
  input space), 0 mismatches. Every multiplier bit pattern, hence
  every add/no-add path, is exercised.
- Directed: boundary rows (0, 1, 2, 3, 0x7FFE/0x7FFF/0x8000/0x8001,
  0xFFFD/0xFFFE/0xFFFF) as an 11x11 cross product, plus all 16x16
  power-of-two pairs (377 directed cases, also covered by the
  exhaustive pass).
- 4,294,967,673 checks per build, 0 mismatches, in each of the `-O0`,
  `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every result word is identical across all
  three builds (value recorded in `PROOF.md`).
- Disassembly of the implementation at `-O2` (x86-64, GCC, non-inline
  wrapper compiled from the header alone so the oracle cannot leak
  in): the shift-add loop compiled branchless (`test`/`lea`/`cmovne`,
  `add edi,edi` to double x, `shr si,1` to walk the multiplier bits).
  No `mul`/`imul` instruction appears; the `make disasm` step greps
  the objdump output and fails if one does.
- Throughput at `-O2`: best of 5 passes of 1M pairs (values recorded
  in `PROOF.md`; pairs pre-generated with splitmix64, results xored
  into a volatile sink).
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full case set.
