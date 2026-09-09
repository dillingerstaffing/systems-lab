# lab/31-mul-by-constant

`mul_const.c` (`mul_const.h` declares it): `mul_const32(uint32_t x,
uint8_t k)` computes `x * k` using only shifts and adds. The
computation itself never uses the multiply operator; the native
product is used only as the differential oracle in the test.

How it works: write the 8-bit `k` as a sum of set bits,
k = b0 + 2*b1 + ... + 128*b7. Multiplying out gives
x * k = sum over set bits i of (x << i), since multiplying by 2^i is
a left shift by i. The loop tests each bit of `k` in turn, adds the
running shift of `x` when the bit is set, and doubles the running
shift each step. Everything is unsigned, so every shift and add is
defined and the total wraps modulo 2^32, exactly what C specifies for
a 32-bit product.

Verified by `test_mul_const.c` (fixed-seed xorshift64* PRNG, seed
`0x9E3779B97F4A7C15`, fully reproducible):

- 256,000,000 differential checks, 0 mismatches: all 256 constants
  `k` in 0..255, each against 1,000,000 full-range 32-bit `x`
  values, compared against `(uint32_t)((uint64_t)x * k)`. The
  constants cover the edges (`k=0` gives 0, `k=1` gives `x`,
  `k=255` exercises all 8 shift/add steps) and the `x` stream covers
  wrapping near 2^32.
- Identical FNV-1a checksum (`18440810711898140094`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2`: 9.09 ns per multiply (110.0 Mops/s) over 100M timed
multiplies. Honest caveats: the figure includes the PRNG step per
multiply and `k` varies per iteration so the compiler cannot
constant-fold the constant; it is the cost of one
generate-and-multiply case, not one bare call. The `-O2`
disassembly was inspected and shows the compiler kept the
shift-add loop as written (`bt` to test each bit, `lea`/`cmovb` for
the conditional add, `add` doubling the shift), not a hardware
`imul`.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
