# PROOF.md: lab/04-crc32

Date: 2026-09-08. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build log and run output.

## Build log (-O2)

```
$ make clean && make
rm -f test_crc32 test_crc32_asan test_crc32_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_crc32 test_crc32.c crc32.c
make exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## Run output (-O2)

```
$ ./test_crc32
crc32 test: two implementations, known-answer + cross-check + timing
known-answer vectors: 7 inputs x 2 implementations, all match
cross-check: 2088 random buffers, bitwise == table on all
table derivation: all 256 single-byte CRCs match the bitwise result
ALL CORRECTNESS TESTS PASSED

timing: 32 MiB buffer, 5 runs each, sink=0x00000000
  bitwise: 56.1 MB/s (0.45 Gbps)
  table:   231.3 MB/s (1.85 Gbps)
  speedup: 4.1x
ALL TESTS PASSED
run exit=0
```

(`sink=0x00000000` is the xor of the bitwise result and the table result
over the 32 MiB buffer, i.e. the two variants produced the identical
32-bit CRC on the full timing buffer.)

## -O0 build and run

```
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_crc32_o0 test_crc32.c crc32.c
./test_crc32_o0
crc32 test: two implementations, known-answer + cross-check + timing
known-answer vectors: 7 inputs x 2 implementations, all match
cross-check: 2088 random buffers, bitwise == table on all
table derivation: all 256 single-byte CRCs match the bitwise result
ALL CORRECTNESS TESTS PASSED

timing: 32 MiB buffer, 5 runs each, sink=0x00000000
  bitwise: 12.2 MB/s (0.10 Gbps)
  table:   232.0 MB/s (1.86 Gbps)
  speedup: 19.0x
ALL TESTS PASSED
opt0 exit=0
```

## AddressSanitizer + UBSan build and run

```
$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
	-o test_crc32_asan test_crc32.c crc32.c
./test_crc32_asan
crc32 test: two implementations, known-answer + cross-check + timing
known-answer vectors: 7 inputs x 2 implementations, all match
cross-check: 2088 random buffers, bitwise == table on all
table derivation: all 256 single-byte CRCs match the bitwise result
ALL CORRECTNESS TESTS PASSED

timing: 32 MiB buffer, 5 runs each, sink=0x00000000
  bitwise: 74.4 MB/s (0.60 Gbps)
  table:   230.8 MB/s (1.85 Gbps)
  speedup: 3.1x
ALL TESTS PASSED
sanitize exit=0
```

Zero sanitizer reports across 2088 randomized cross-check buffers and
the full 32 MiB timing buffer.

## What was verified

- Known-answer vectors against the published CRC32/IEEE 802.3 check
  values, each checked by BOTH implementations:
  - `""` -> 0x00000000
  - `"a"` -> 0xE8B7BE43
  - `"abc"` -> 0x352441C2
  - `"message digest"` -> 0x20159D7F
  - `"abcdefghijklmnopqrstuvwxyz"` -> 0x4C2750BD
  - 62-char alphanumerics -> 0x1FC2E6D2
  - `"123456789"` -> 0xCBF43926 (the canonical check value)
- Cross-check: 2088 randomized buffers (lengths 0..256 x 8 seeds, plus
  1/4/16/64 KiB blocks) produced bit-identical CRCs from the bitwise
  and table variants. Buffers come from a deterministic xorshift32 PRNG,
  so the run is reproducible.
- Table derivation: the startup-generated 256-entry table was validated
  entry by entry: the single-byte CRC computed by the bitwise division
  routine equals the table-driven result for all 256 byte values. The
  table is derived from the generator polynomial (0xEDB88320, the
  reflected form of 0x04C11DB7), not copied from a reference table.
- Throughput measured with CLOCK_MONOTONIC on a 32 MiB random buffer,
  5 runs each, mean reported:
  - -O2: bitwise 56.1 MB/s (0.45 Gbps), table 231.3 MB/s (1.85 Gbps),
    4.1x speedup.
  - -O0: bitwise 12.2 MB/s (0.10 Gbps), table 232.0 MB/s (1.86 Gbps),
    19.0x speedup. (The table path is dominated by the same
    load-dependent chain at any optimization level; the bitwise path is
    8 conditional shift/xor steps per byte and scales with the
    optimizer.)
