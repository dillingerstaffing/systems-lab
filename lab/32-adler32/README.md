# lab/32-adler32: Adler-32 checksum

Adler-32 (RFC 1950, section 8.2) computed directly from its rolling-sum
recurrence: `A = 1 + sum(data[i]) mod 65521`, `B = sum of running A mod
65521`, result `(B << 16) | A`. The loop applies the two updates per byte;
reduction modulo 65521 is deferred to the end of each 5552-byte block,
which is the largest block size that keeps 32-bit accumulators from
overflowing (see `adler32.h` for the bound). No library checksum code is
wrapped.

## Files

- `adler32.h`, `adler32.c`: the implementation (one function, 16-byte
  unrolled inner loop, per-block reduction).
- `test_adler32.c`: six known-answer vectors, a 1M-buffer differential test
  against a per-byte naive reference, and a throughput benchmark.
- `Makefile`: `test-o0`, `test-o2`, `test-asan`, `test-ubsan`, `clean`.
- `PROOF.md`: genuine build log and measured numbers.

## Measured

- 6/6 RFC 1950 vectors pass, including empty, "a", "abc", "message digest",
  "abcdefghijklmnopqrstuvwxyz", and 0x00..0xFF repeated 100 times.
- Differential: 1,000,000 fixed-seed (0xAD1E4321) random buffers,
  3,256,296,802 bytes total, lengths covering empty, odd sizes, and the
  5552-byte block boundary (5544..5560, 11096..11112, 0..20000).
  Mismatches: 0.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` under -O0, -O2,
  ASan+UBSan, and UBSan; all four runs PASS.
- Throughput over 256 MiB at -O2: 1627.8 MiB/s (a second run measured
  1787.9 MiB/s; run-to-run machine variance). -O0: 949.9 MiB/s.
