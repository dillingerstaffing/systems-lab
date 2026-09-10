# PROOF.md: lab/61-crc16-ccitt

Environment: gcc 13.3.0 (Ubuntu), x86_64.

`crc16_ccitt()` (crc16.h) computes the 16-bit CRC with polynomial
x^16 + x^12 + x^5 + 1 (0x1021) in reflected form: the register starts at
0x0000, each byte is folded in least-significant-bit first, and every bit
shifts the register right one place, xoring 0x8408 (the bit-reversed
0x1021) when the bit shifted out is 1. The shift/xor recurrence is the
whole implementation: no table, no library CRC, no builtin.

Note on known-answer vectors: the brief suggested "123456789" -> 0xBB3D.
Checking that value against the CRC catalogue showed it is not the check
value for these parameters: 0xBB3D is the catalogue's CRC-16/ARC check
(poly 0x8005 reflected). For this module's parameter set (poly 0x1021,
refin true, refout true, init 0x0000, xorout 0x0000) the catalogue entry
is CRC-16/KERMIT, whose check value for "123456789" is 0x2189, which this
implementation reproduces. The 0xBB3D value was not used.

A second correction: a forum post quoting the 6-byte message
{0x05, 0x01, 0x00, 0x04, 0xFC, 0xFF} claimed KERMIT = 0x72A6 alongside
XMODEM = 0x7DCC. The XMODEM value reproduces exactly through the
MSB-first reference, confirming the data; the KERMIT claim does not:
0x72A6 is the byte-swapped form of the value 0xA672 that all three
independent recurrences here (the reflected implementation, the
MSB-first reference over bit-reversed bytes, and a separately written
Python bit-loop) produce for that message with init 0x0000. The poster's
value evidently carries an extra byte swap from their hand procedure and
was not used; 0xA672 is asserted as the module's value for that message.

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
vector empty        expect=0x0000 got=0x0000 ref=0x0000 OK
vector "123456789"  expect=0x2189 got=0x2189 ref=0x2189 OK
vector {0x01}       expect=0x1189 got=0x1189 ref=0x1189 OK
vector xmodem-ref  expect=0x7dcc got=0x7dcc OK
vector kermit-6byte expect=0xa672 got=0xa672 ref=0xa672 OK
differential: 1000000 buffers, 127856024 bytes total, mismatches=0
fnv1a=8c5660cc04fdd62d
verification_time=4.353 s
throughput: 64.0 MiB in 1.187 s = 53.9 MiB/s (acc=0x0000)
RESULT: PASS
run_exit=0
```

### -O0 (`make opt0`)

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_crc16_o0 test_crc16.c
./test_crc16_o0
vector empty        expect=0x0000 got=0x0000 ref=0x0000 OK
vector "123456789"  expect=0x2189 got=0x2189 ref=0x2189 OK
vector {0x01}       expect=0x1189 got=0x1189 ref=0x1189 OK
vector xmodem-ref  expect=0x7dcc got=0x7dcc OK
vector kermit-6byte expect=0xa672 got=0xa672 ref=0xa672 OK
differential: 1000000 buffers, 127856024 bytes total, mismatches=0
fnv1a=8c5660cc04fdd62d
verification_time=27.036 s
throughput: 64.0 MiB in 4.197 s = 15.2 MiB/s (acc=0x0000)
RESULT: PASS
opt0_exit=0
```

### ASan+UBSan (`make sanitize`)

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_crc16_asan test_crc16.c
./test_crc16_asan
vector empty        expect=0x0000 got=0x0000 ref=0x0000 OK
vector "123456789"  expect=0x2189 got=0x2189 ref=0x2189 OK
vector {0x01}       expect=0x1189 got=0x1189 ref=0x1189 OK
vector xmodem-ref  expect=0x7dcc got=0x7dcc OK
vector kermit-6byte expect=0xa672 got=0xa672 ref=0xa672 OK
differential: 1000000 buffers, 127856024 bytes total, mismatches=0
fnv1a=8c5660cc04fdd62d
verification_time=4.323 s
throughput: 64.0 MiB in 1.398 s = 45.8 MiB/s (acc=0x0000)
RESULT: PASS
san_exit=0
```

The sanitizer run printed no reports: zero ASan/UBSan findings.

## What was measured

- Known-answer vectors, 5/5 pass:
  - empty -> 0x0000 (trivial, both implementation and reference).
  - "123456789" -> 0x2189, the CRC catalogue's CRC-16/KERMIT check
    value for exactly these parameters.
  - {0x01} -> 0x1189, verified by hand arithmetic (8 shift/xor steps
    from 0x0001: 0x8408, 0x4204, 0x2102, 0x1081, 0x8C48, 0x4624,
    0x2312, 0x1189) and matching the published crcmod-generated KERMIT
    table entry for byte 0x01.
  - The MSB-first reference reproduces the published XMODEM value
    0x7DCC for {0x05, 0x01, 0x00, 0x04, 0xFC, 0xFF}, tying the reference
    to a published constant; the reflected implementation gives 0xA672
    for the same message, pinned by the implementation, the reference,
    and an independent Python recurrence (the forum's 0x72A6 is the
    byte-swapped artifact, see note above).
- Differential: 1,000,000 fixed-seed (0xC16CC177A16CC177) random
  buffers, lengths 0..256 drawn uniformly (covers empty, single-byte,
  and odd lengths), 127,856,024 bytes total. `crc16_ccitt()` vs the
  independent MSB-first reference (unreflected poly 0x1021 over the
  bit-reversed message, result bit-reversed; different bit order,
  different loop, different polynomial constant). 0 mismatches.
- FNV-1a checksum over all 1,000,000 results: `8c5660cc04fdd62d`,
  identical across -O0, -O2, and ASan+UBSan builds.
- Throughput at -O2: 53.9 MiB/s. The timed loop is 64 full
  `crc16_ccitt()` passes over a fixed 1 MiB splitmix64-filled buffer,
  results xor-folded into a printed accumulator so the loop is not dead
  code. (The accumulator prints 0x0000 because 64 identical CRCs
  xor-cancel; the folding still prevents the compiler from discarding
  the calls.)

## Limits of verification

- The differential reference shares the parameter set (poly 0x1021,
  init 0x0000, xorout 0x0000) with the implementation; agreement between
  them only rules out coding errors, not a wrong parameter
  understanding. The published catalogue check value 0x2189 is what
  ties the parameters to ground truth.
- Throughput was measured on one x86_64 host (gcc 13.3.0); the number
  is not portable. Two -O2 runs gave 53.9 and 43.9 MiB/s; the pasted
  log is the final run.
