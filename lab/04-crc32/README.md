# lab/04-crc32

Two from-scratch implementations of CRC32 (IEEE 802.3, polynomial
`0x04C11DB7`, register init `0xFFFFFFFF`, final xor `0xFFFFFFFF`,
reflected input/output), with a test that checks them against the
published check values and against each other.

- `crc32.c` / `crc32.h`
  - `crc32_bitwise()`: polynomial long division over GF(2), one bit per
    step, a direct transcription of the division algorithm.
  - `crc32_table()`: 8 bits per step. The 256-entry table is derived at
    startup from the generator polynomial itself; there is no
    hardcoded table.
- `test_crc32.c`: known-answer vectors (including `""`, `"a"`,
  `"abc"`, and the canonical `"123456789"` check value
  `0xCBF43926`), a 2088-buffer randomized cross-check proving the two
  variants agree bit-for-bit, a per-entry table-derivation check, and a
  throughput measurement of both variants on a 32 MiB buffer.

Build: `make` (also `make opt0` for `-O0`, `make sanitize` for
AddressSanitizer + UBSan), run: `make run`. See PROOF.md for the real
build log, run output, and measured numbers.
