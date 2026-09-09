# lab/22-ctz

`ctz.c` (`ctz.h` declares it): count-trailing-zeros for 32-bit
unsigned integers, `unsigned ctz32(uint32_t x)`, with `ctz32(0)`
defined as 32.

How it works: for nonzero `x`, `x & -x` keeps only the lowest set
bit (two's complement: `-x` flips every bit below the lowest 1 and
keeps that bit, so ANDing with `x` clears the rest), leaving exactly
2^k where k is the trailing-zero count. Multiplying that single set
bit by the constant `0x077CB531` and keeping the top 5 bits maps each
of the 32 possible powers of two to a distinct 5-bit key, and a
32-entry table inverts the key back to k. The test binary asserts
the distinctness for every k, so the table is checked, not trusted.
All arithmetic is unsigned, so no undefined behavior is possible.

Verified by `test_ctz.c` (fixed-seed xorshift64* PRNG, seed
`0x2545F4914F6CDD1D`, fully reproducible):

- de Bruijn identity: all 32 powers of two checked, 32 distinct
  5-bit keys, `ctz32(1u << k) == k` for every k.
- 2,065,536 differential checks, 0 mismatches: exhaustive sweep
  over all 2^16 values 0..65535 against a hand-written naive
  bit-walking reference (returns 32 for 0), plus a `__builtin_ctz`
  cross-check for every nonzero 16-bit value, plus 2,000,000
  full-range 32-bit random values.
- Identical FNV-1a checksum (`7337941371035615009`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2` (100,000,000 timed values, each drawn from the PRNG
and accumulated into a sink so the loop cannot be optimized away):
3.43 ns/value. Honest caveats: the figure includes the PRNG step, so
it is the cost of one generate-and-count case, not one bare `ctz32`
call, and rerun-to-rerun machine variance is roughly +-0.3 ns. The
100M-value sink total was 99978375, an average count of about 1.00,
which matches the expected value for random inputs.

Disassembly check (gcc 13.3.0, `-O2`, x86_64): the compiler kept the
multiply-and-table construction as written (`neg`/`and` to isolate
the lowest bit, `imul` by 0x77cb531, `shr` 27, byte load from the
table); it did not replace it with a hardware `tzcnt`.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
