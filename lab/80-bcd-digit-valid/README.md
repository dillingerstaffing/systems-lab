# lab/80-bcd-digit-valid

Packed BCD digit-validity bitmask. `bcd_invalid_mask(w)` takes a
16-bit word holding four packed BCD nibbles and returns a 4-bit mask
where bit i is 1 iff nibble i (nibble 0 = bits 0..3, least significant)
holds a value in 10..15, i.e. is not a valid BCD digit.

The construction rests on the add-guard identity: for a nibble value
n in 0..15, n + 6 >= 16 iff n >= 10, so adding 6 to a nibble carries
out of the nibble exactly when the nibble is invalid.

The hazard is inter-nibble carry aliasing: in the naive form
w + 0x6666, a carry generated at a low invalid nibble propagates
through higher valid nibbles and falsely flags them. It is defeated by
adding 6 to even and odd nibbles in two separate masked adds,
`(w & 0x0F0F) + 0x0606` and `(w & 0xF0F0) + 0x6060`, where the zeroed
nibble lanes between the active lanes absorb the at-most-1 generated
carry (a full adder with both addend bits 0 kills any incoming carry),
so no chain can ever reach another nibble. Each nibble's generated
carry is then read with the carry identity c_k = t_k ^ a_k ^ b_k (carry
into bit k of t = a + b), which reduces to the single sum bit at each
lane boundary because the two addend bits there are 0. The full
bit-by-bit derivation is in `PROOF.md`.

Verified in `test_bcd.c`:

- All 65,536 16-bit inputs, exhaustive, differential-checked against
  an independent per-nibble loop reference: 0 mismatches.
- 18 directed rows chosen to alias under the naive form (0x9A00,
  0x9A9A, 0xA9A9, 0xF900, ...), each checked against the reference;
  the naive w + 0x6666 form misclassifies 4,572 of the 65,536 inputs,
  confirming the hazard is real and defeated.
- 65,554 total checks, 0 mismatches. FNV-1a 64-bit checksum over the
  result stream: `ffd4445043425186`, identical across `-O0`, `-O2`,
  and ASan+UBSan builds.
- The `-O2` disassembly keeps the two masked adds (0x0F0F/0x0606 and
  0xF0F0/0x6060) and the four bit-extractions; no popcount-class
  instruction is emitted.
- Measured at -O2: 4.27 ns/value (234.2 Mvalues/s over 26,214,400
  timed values, best of 5).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
