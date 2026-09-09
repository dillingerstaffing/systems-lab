# lab/46-fletcher16: Fletcher-16 checksum

`fletcher16()` (fletcher16.h) computes the Fletcher-16 checksum directly
from its dual running-sum recurrence: both sums start at 0, and for each
byte `b`, `sum1 = (sum1 + b) % 255`, `sum2 = (sum2 + sum1) % 255`; the
result is `(sum2 << 8) | sum1`. The function is the recurrence written as
a loop, reducing modulo 255 after every byte, exactly as the definition
is written. No tables, no deferred reduction, no library code.

## Files

- `fletcher16.h`: the implementation (one function).
- `test_fletcher16.c`: eight known-answer vectors, a 1M-buffer
  differential test against an independent closed-form reference, and a
  throughput benchmark.
- `Makefile`: `test-o0`, `test-o2`, `test-asan`, `test-ubsan`, `clean`.
- `PROOF.md`: genuine build log and measured numbers.

## Measured

- 8/8 vectors pass: the three published Fletcher-16 test vectors from
  the Fletcher's checksum article (`"abcde"` -> 0xC8F0, `"abcdef"` ->
  0x2057, `"abcdefgh"` -> 0x0627), each also re-checked by hand
  arithmetic; `"123456789"` -> 0x1EDE, verified by hand (sum1 =
  477 % 255 = 222, sum2 = 2325 % 255 = 30); plus empty -> 0x0000,
  `"a"` -> 0x6161, `{0xFF}` -> 0x0000, `{0x01,0xFF}` -> 0x0201.
- Differential: 1,000,000 fixed-seed (0xF1E7C416) random buffers,
  1,388,938,179 bytes total, lengths covering empty, small, mid, large
  (2048..20015), and the 250..260 modulus neighborhood. The reference is
  a closed-form two-loop weighted-sum computation sharing no update
  logic with the implementation. Mismatches: 0.
- FNV-1a fingerprint 0x903aa7957d888495 over all 1M checksums, identical
  across -O0, -O2, ASan+UBSan, and UBSan builds.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` at -O0, -O2, and
  both sanitizer builds; zero warnings, zero sanitizer reports (built
  with `-fno-sanitize-recover=all`, so any report would be fatal).
- Throughput at -O2: 256.0 MiB in 1.165 s = 219.8 MiB/s. See PROOF.md
  for the full build and run logs.
