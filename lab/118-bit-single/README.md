# lab/118: single-bit set/clear/toggle/test from the mask identities

`bset`, `bclr`, `btg`, `btst` in `bitop.h` manipulate one bit of a 64-bit
word using the mask `1ULL << i`: set is `x | m`, clear is `x & ~m`,
toggle is `x ^ m`, test is `(x >> i) & 1`.

## Idea

A 64-bit unsigned integer is the sum of b_i * 2^i for i = 0..63, with each
b_i in {0, 1}. The mask `1ULL << i` has exactly bit i set. OR-ing it in
forces bit i to 1; AND-ing with its complement forces bit i to 0; XOR-ing
flips it; a right shift by i moves bit i to position 0 where `& 1` reads
it. Each operation leaves the other 63 bits untouched, by construction.

The position must lie in 0..63. Bit 63 is the most significant bit:
`1ULL << 63` is `0x8000000000000000`, well defined because the shift count
63 is below the word width of 64. A shift by 64 is undefined behavior, so
every operation asserts `i >= 0 && i < 64`, the asserts are live in all
test builds, and the sanitizer build arms the UBSan shift-exponent check
as a second tripwire.

## Verification

Differential test against naive per-bit loop references (rebuilt one bit
at a time, never using the mask identities):

- the i = 63 edge pinned against known truths
  (`mask(63) = 0x8000000000000000`, set/clear/toggle/test on the MSB)
- directed: 68 values (0, all-ones, every single-bit word, both
  alternating patterns) crossed with all 64 positions
- 1,000,000 fixed-seed splitmix64 (word, position) pairs
  (seed 0x123456789ABCDEF0)

Total 4,017,408 checks, 0 mismatches, across three builds (`-O0`, `-O2`,
ASan+UBSan), all producing the identical FNV-1a result checksum
`0x4c11b8482936109c`. `make disasm` fails the build if the implementation
object contains any shift by a constant count of 64 or more. Throughput
at `-O2`: 1.332 ns/value (best of 5, 50M operations over pre-generated
inputs).

## Files

- `bitop.h`      the implementation (mask identities only)
- `test_bitop.c` differential test vs per-bit references, edge pin, checksum
- `bench_bitop.c` throughput benchmark over pre-generated inputs
- `impl_check.c` minimal TU used by the no-shift-by-64 disassembly check
- `Makefile`     builds everything; `make check` runs all three test builds
- `PROOF.md`     derivation, genuine build/test logs, edge-case contract
