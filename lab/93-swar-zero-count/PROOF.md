# PROOF.md: lab/93-swar-zero-count

## What was built

`zero_count.h` counts the zero bytes in a 64-bit word using parallel
byte arithmetic. The starting point is the zero-byte detection identity

    mask = (x - 0x0101010101010101) & ~x & 0x8080808080808080

which detects a zero byte because subtracting 1 from byte 0x00 borrows and
flips the byte's top bit on, while any nonzero byte subtracts cleanly or
already had its top bit set (cleared by the `& ~x` term).

## Measured defect in the identity as written

The identity is only valid as an existence test, not for counting. A
borrow out of a zero low byte propagates into the byte above it; if that
byte is 0x01, the extra subtract turns it into 0xFF and sets its top bit
even though the byte is not zero. Differential testing of the naive form
against a per-byte reference loop found **1001 mismatches in 10,065,536
inputs**. Concrete case: x = 0x0100 (bytes: 00 01 00 00 00 00 00 00) makes
the naive form report 8 zero bytes where 7 exist, because byte 1 (0x01)
receives the borrow from byte 0 (0x00) and produces a phantom high bit.

## The fix, verified

The subtraction runs in 16-bit lanes with a 0x7F guard in each lane's
high byte (even bytes of x in one word's lanes, odd bytes in another's).
A lane holds (0x7F << 8) | v; subtracting 0x0001 per lane borrows at most
1 out of the low byte and at most 1 into the high byte, so the high byte
stays >= 0x7D and no borrow ever crosses a lane boundary. Each lane's low
byte then behaves exactly like an isolated byte subtraction, and its top
bit (after `& ~lanes`) is set exactly when the byte was zero. The four
per-lane indicators are summed with ((v >> 7) * 0x0001000100010001) >> 48;
even and odd counts are added. Returns 0..8.

## Build log (genuine output)

```
cc -std=c11 -Wall -Wextra -Werror -O0 -o test_o0 main.c
cc -std=c11 -Wall -Wextra -Werror -O2 -o test_o2 main.c
cc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -o test_asan main.c
```

Zero warnings under -std=c11 -Wall -Wextra -Werror. Zero sanitizer reports
from the ASan+UBSan binary (exit status 0).

## Differential test results (genuine output)

Each binary ran the same suite: exhaustive all 16-bit inputs (65,536)
plus 10,000,000 fixed-seed splitmix64 64-bit words (seed
0x123456789ABCDEF0), compared against a naive per-byte reference loop,
with an FNV-1a checksum over all outputs.

```
=== O0 ===
checked=10065536 mismatches=0 fnv1a=ca180bdf75d1ddd9
=== O2 ===
checked=10065536 mismatches=0 fnv1a=ca180bdf75d1ddd9
=== ASan+UBSan ===
checked=10065536 mismatches=0 fnv1a=ca180bdf75d1ddd9
```

FNV-1a checksum `ca180bdf75d1ddd9` is identical across -O0, -O2, and
ASan+UBSan builds.

## Disassembly check (-O2)

`cc -O2 -S` on a probe calling `count_zero_bytes`: the hot path inlines to
straight-line integer arithmetic (movabs/and/or/not/add/shr/sal; the
multiply is expanded to shift-add sequences). The full listing contains
no branch, no loop, and no byte-by-byte compare: there is no per-byte
branch anywhere in the function. The constants in the listing decode to
exactly the lane mask 0x00FF00FF00FF00FF, guard 0x7F007F007F007F00, and
per-lane subtract 0x0001000100010001 used in the source.

## Throughput (-O2, genuine output)

Timed over 26,214,400 precomputed fixed-seed words, best of 5 runs:

```
timed=26214400 best=2.244 ns/value checksum=4143000
```

(-O0: 18.707 ns/value; ASan+UBSan: 6.967 ns/value; checksums equal, so the
timed path computed the same values.)

## What was verified

- 10,065,536 inputs (exhaustive 16-bit + 10M fixed-seed 64-bit): zero
  mismatches against the per-byte reference.
- Output checksum identical across -O0, -O2, and ASan+UBSan builds.
- Zero compiler warnings; zero sanitizer reports.
- -O2 hot path has no byte-by-byte branch (verified in disassembly).
- Throughput measured at -O2: 2.244 ns/value (best of 5).
