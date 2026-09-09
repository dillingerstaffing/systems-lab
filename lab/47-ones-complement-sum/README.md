# lab/47-ones-complement-sum: one's-complement checksum

`ones_complement_sum()` (onesum.h) computes the 16-bit one's-complement
checksum over a byte buffer, the layout used by IPv4 headers. Words are
read as 16-bit big-endian values assembled from bytes, so there are no
casts and no alignment assumptions. Each word is added into a 16-bit
accumulator with end-around carry: a carry out of bit 15 is folded back
into bit 0 of the same addition. An odd trailing byte is padded with a
zero byte, and an empty buffer sums to 0x0000 which complements to
0xFFFF. No library checksum is used.

## Files

- `onesum.h`: the implementation (one function).
- `test_onesum.c`: nine hand-computed known-answer vectors (arithmetic
  shown in comments), a 1M-buffer differential test against an
  independent reference that accumulates words with no folding and folds
  carries only once at the end, and a throughput benchmark.
- `Makefile`: `test-o0`, `test-o2`, `test-asan`, `test-ubsan`, `clean`.
- `PROOF.md`: genuine build log and measured numbers.

## Measured

- 9/9 vectors pass: empty -> 0xFFFF, `{0x00}` -> 0xFFFF, `{0xFF}` ->
  0x00FF, four zero bytes -> 0xFFFF, four 0xFF bytes -> 0x0000
  (end-around carry: 0xFFFF + 0xFFFF = 0x1FFFE folds to 0xFFFF),
  `{0x00,0x01,0xF2,0x03}` -> 0x0DFB, `{0xFF,0xFF,0x00,0x01}` -> 0xFFFE
  (carry out of bit 15 folded into bit 0), `{0x01,0x02,0x03}` -> 0xFBFD
  and `{0x12,0x34,0x56}` -> 0x97CB (odd-length zero padding).
- Differential: 1,000,000 fixed-seed (0x123456789ABCDEF0) random
  buffers, 172,733,565 bytes total, lengths mixing empty, tiny
  (0..63), general (1..511), and 64..511. The reference shares no
  update logic with the implementation. Mismatches: 0.
- FNV-1a fingerprint 0x7ae87b2b2fb7e3e1 over all 1M checksums, identical
  across -O0, -O2, ASan+UBSan, and UBSan builds.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` at -O0, -O2, and
  both sanitizer builds; zero warnings, zero sanitizer reports (built
  with `-fno-sanitize-recover=all`, so any report would be fatal).
- Throughput at -O2: 256.0 MiB in 0.299 s = 857.4 MiB/s. See PROOF.md
  for the full build and run logs.
