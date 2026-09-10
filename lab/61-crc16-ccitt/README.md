# lab/61-crc16-ccitt: CRC-16 (poly 0x1021, reflected)

`crc16_ccitt()` (crc16.h) computes the 16-bit CRC with polynomial
x^16 + x^12 + x^5 + 1 (0x1021) in reflected form: init 0x0000, each byte
folded in least-significant-bit first, one shift/xor step per bit with
the reflected polynomial 0x8408, xorout 0x0000. No table, no library
CRC, no builtin: the shift/xor recurrence is the whole implementation.

## Files

- `crc16.h`: the implementation (one function).
- `test_crc16.c`: five known-answer vectors, a 1M-buffer differential
  test against an independent MSB-first reference, and a throughput
  benchmark.
- `Makefile`: `all`/`run`, `opt0`, `sanitize`, `clean`.
- `PROOF.md`: genuine build log and measured numbers, including two
  vector corrections (the brief's 0xBB3D is CRC-16/ARC, not this
  parameter set; a forum post's 0x72A6 is a byte-swapped 0xA672).

## Measured

- 5/5 vectors pass: `"123456789"` -> 0x2189 (the CRC catalogue's
  CRC-16/KERMIT check value for exactly these parameters), empty ->
  0x0000, `{0x01}` -> 0x1189 (hand arithmetic), XMODEM reference check
  0x7DCC, and `{0x05,0x01,0x00,0x04,0xFC,0xFF}` -> 0xA672.
- Differential: 1,000,000 fixed-seed random buffers (lengths 0..256,
  127,856,024 bytes total) vs an independent MSB-first reference over
  bit-reversed bytes. Mismatches: 0.
- FNV-1a fingerprint 0x8c5660cc04fdd62d over all 1M checksums, identical
  across -O0, -O2, and ASan+UBSan builds.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` at -O0, -O2, and
  ASan+UBSan; zero warnings, zero sanitizer reports.
- Throughput at -O2: 53.9 MiB/s (64 passes over a fixed 1 MiB buffer).
  See PROOF.md for the full build and run logs.
