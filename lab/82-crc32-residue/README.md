# lab/82-crc32-residue

The CRC-32 residue invariant. For CRC-32 (polynomial 0x04C11DB7,
register init 0xFFFFFFFF, final xor 0xFFFFFFFF, reflected
input/output), checksumming a message `M`, appending the four bytes
of `crc32(M)` in little-endian (wire) order, and checksumming the
extended message always produces the same value: `0x2144DF1C`,
regardless of what `M` is. The `crc32_*` functions are the lab/04
implementation compiled into this module unchanged; this lab only
orchestrates the two-pass flow and checks the constant.

## What was measured

- `crc32("123456789")` is `0xCBF43926` (both the table-driven and
  the bitwise implementation agree); appending those four bytes in
  little-endian order and re-checksumming gives `0x2144DF1C`.
- Appending the same four bytes in big-endian order gives
  `0x3CC9742C`, not the residue: the byte order is part of the
  contract, checked as a negative control.
- Empty message: `crc32("")` is `0x00000000` (init and xorout
  cancel), and checksumming the four appended zero bytes gives the
  residue `0x2144DF1C` too.
- 1,000,000 fixed-seed splitmix64 buffers (seed
  `0x123456789ABCDEF0`, lengths 0..256, 3,868 length-0 cases
  included): every buffer checksummed, CRC appended, re-checksummed,
  all 1,000,000 equal `0x2144DF1C`. Zero residue mismatches, zero
  table-vs-bitwise disagreements.
- The FNV-1a checksum over the per-case results (length, crc,
  residue of every case) is `0x277d10c8b7ea96ec`, identical across
  the `-O0`, `-O2`, and ASan+UBSan builds.
- Throughput of the two-pass flow (checksum, append, re-checksum)
  over the 1,000,000 buffers at `-O2`: 207.7 MiB/s (best of 3).
  Exact logs are in PROOF.md.

`-std=c11 -Wall -Wextra -Werror` clean; zero ASan/UBSan reports
across the full case set. Exact build logs and run output are in
PROOF.md.
