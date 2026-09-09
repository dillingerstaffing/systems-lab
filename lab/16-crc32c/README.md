# lab/16-crc32c

Two hand-written implementations of CRC-32C (Castagnoli, generator
polynomial `0x1EDC6F41`, reflected `0x82F63B78`, init `0xFFFFFFFF`,
final xor `0xFFFFFFFF`), with a test that checks them against published
check values and against each other.

- `crc32c.c` / `crc32c.h`
  - `crc32c_bitwise()`: GF(2) polynomial long division, one conditional
    xor per bit, transcribed directly from the division algorithm.
  - `crc32c_table()`: one 256-entry lookup per byte. The table is
    derived at startup from the generator polynomial itself; no
    hardcoded table. `crc32c_table_check()` re-derives all 256 entries
    straight from the polynomial and reports any disagreement.
- `test_crc32c.c`: known-answer vectors (including `\"\"`,
  `\"123456789\"`, the Linux kernel `crypto/testmgr.h` vector
  `\"abcdefg\"`, and iSCSI vectors published in RFC 3720 Appendix B.2 /
  Intel's ipp docs), an exhaustive differential test (all 256 single
  bytes, sizes 0-64, 100000 random buffers up to 4 KiB, 0 mismatches),
  and a throughput measurement of both variants on a 32 MiB buffer.

No library CRC calls; no compiler CRC builtins for the checksum itself.

Build: `make` (also `make opt0` for `-O0`, `make sanitize` for
AddressSanitizer + UBSan), run: `make run`. See PROOF.md for the real
build log, run output, and measured numbers.
