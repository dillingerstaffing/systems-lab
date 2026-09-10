# PROOF.md: lab/79-crc16-table

Environment: gcc 13.3.0 (Ubuntu), x86_64.

`crc16_bitwise()` (crc16.h) computes the 16-bit CRC with polynomial
x^16 + x^12 + x^5 + 1 (0x1021), most-significant-bit first: the register
starts at 0xFFFF, each byte is folded into the top 8 bits, and every bit
shifts the register left one place, xoring 0x1021 when the bit shifted out
of the top is 1. The shift/xor recurrence is the whole implementation: no
table, no library CRC, no builtin.

`crc16_table()` computes the same value byte-at-a-time through a 256-entry
table that `crc16_table_init()` derives from the polynomial at startup:
each `table[i]` is the shift/xor recurrence applied to the single byte i
starting from a zero register with the byte in the top 8 bits. There is no
hard-coded table anywhere in the module. The per-byte update is
`crc = (crc << 8) ^ table[((crc >> 8) ^ byte) & 0xFF]`.

Parameters: init 0xFFFF, no reflection, xorout 0x0000. This matches the CRC
catalogue's CRC-16/CCITT-FALSE entry (poly 0x1021, refin false, refout
false, init 0xFFFF, xorout 0x0000), whose check value for "123456789" is
0x29B1.

## Build

All targets compile with `-std=c11 -Wall -Wextra -Werror`. Zero warnings
(any warning would fail the build).

```
$ make clean && make
rm -f test_crc16 test_crc16_asan test_crc16_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_crc16 test_crc16.c
build_exit=0
```

## Runs

### -O2 (`./test_crc16`)

```
table derivation: 256/256 entries match independent re-derivation
vector "123456789"    expect=0x29b1 bitwise=0x29b1 table=0x29b1 OK
vector empty          expect=0xffff bitwise=0xffff table=0xffff OK
differential: 1000256 messages, 128062115 bytes total, mismatches=0
fnv1a=a2efc13368379e59
verification_time=2.597 s
throughput bitwise: 64.0 MiB in 0.904 s = 70.8 MiB/s (acc=0x0000)
throughput table:   64.0 MiB in 0.323 s = 197.9 MiB/s (acc=0x0000)
RESULT: PASS
run_exit=0
```

### -O0 (`make opt0`)

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_crc16_o0 test_crc16.c
./test_crc16_o0
table derivation: 256/256 entries match independent re-derivation
vector "123456789"    expect=0x29b1 bitwise=0x29b1 table=0x29b1 OK
vector empty          expect=0xffff bitwise=0xffff table=0xffff OK
differential: 1000256 messages, 128062115 bytes total, mismatches=0
fnv1a=a2efc13368379e59
verification_time=14.890 s
throughput bitwise: 64.0 MiB in 7.584 s = 8.4 MiB/s (acc=0x0000)
throughput table:   64.0 MiB in 0.629 s = 101.7 MiB/s (acc=0x0000)
RESULT: PASS
opt0_exit=0
```

### ASan+UBSan (`make sanitize`)

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_crc16_asan test_crc16.c
./test_crc16_asan
table derivation: 256/256 entries match independent re-derivation
vector "123456789"    expect=0x29b1 bitwise=0x29b1 table=0x29b1 OK
vector empty          expect=0xffff bitwise=0xffff table=0xffff OK
differential: 1000256 messages, 128062115 bytes total, mismatches=0
fnv1a=a2efc13368379e59
verification_time=4.145 s
throughput bitwise: 64.0 MiB in 1.035 s = 61.8 MiB/s (acc=0x0000)
throughput table:   64.0 MiB in 0.329 s = 194.3 MiB/s (acc=0x0000)
RESULT: PASS
san_exit=0
```

The sanitizer run printed no reports: zero ASan/UBSan findings.

## Independent cross-checks

The test harness re-derives every table entry with its own serial divider
(the 24-bit dividend: byte bits MSB first, then 16 zero bits, through the
same polynomial one bit at a time), sharing no code with crc16.h's
derivation loop: 256/256 match.

A third recurrence, a Python bit-loop implementing the same serial divider,
produced all 256 entries and `diff` confirmed the C program's
`--dump-table` output is identical line for line: 0 differences.

Python's `binascii.crc_hqx` (an independent library implementation of
poly 0x1021, MSB-first, caller-supplied init) returns 0x29b1 for
"123456789" with init 0xFFFF and 0xffff for the empty message, matching the
vectors both C implementations reproduce.

## What was measured

- Table derivation: 256/256 entries from `crc16_table_init()` match the
  independently re-derived values; a separately written Python recurrence
  matches all 256 too. No table bytes are hard-coded anywhere.
- Known-answer vectors, 2/2 pass through both implementations:
  - "123456789" -> 0x29B1, the CRC catalogue's CRC-16/CCITT-FALSE check
    value for exactly these parameters, also confirmed by
    `binascii.crc_hqx`.
  - empty -> 0xFFFF (the init value, no bits processed).
- Differential: all 256 single-byte messages plus 1,000,000 fixed-seed
  (splitmix64, seed 0x9E3779B97F4A7C15) random buffers, lengths 0..256 drawn
  uniformly (covers empty, single-byte, and odd lengths): 1,000,256
  messages, 128,062,115 bytes total, bitwise vs table-driven.
  0 mismatches.
- FNV-1a checksum over the 256 table entries and every one of the
  2,000,512 per-buffer results: `a2efc13368379e59`, identical across -O0,
  -O2, and ASan+UBSan builds.
- Throughput at -O2: bitwise 70.8 MiB/s, table-driven 197.9 MiB/s. The
  timed loop is 64 pure CRC passes over a fixed 1 MiB splitmix64-filled
  buffer (the buffer is filled once before timing, so the timed region
  contains no PRNG step); results xor-fold into a printed accumulator so
  the calls are not dead code. The accumulator prints 0x0000 because 64
  identical CRCs xor-cancel; the folding still prevents the compiler from
  discarding the calls.

## Limits of verification

- Correctness is pinned to the polynomial division and the catalogue check
  value; the differential test proves the two implementations agree, not
  that either is correct in isolation (the 0x29B1 vector and the Python
  cross-checks do that).
- Throughput is measured on one machine (x86_64, gcc 13.3.0); it is a
  property of this build on this hardware, not a portable claim.
- The table derivation check and the differential test share the fixed
  polynomial constant 0x1021 by necessity; the division structure itself is
  what was written independently three times.
