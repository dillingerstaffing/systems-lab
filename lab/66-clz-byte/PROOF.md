<!-- PROOF-HEADER
Checks: 1065866
Mismatches: 0
Checksum: 0x45C10F152E119ECB
Throughput: 1.767 ns/value at -O2
Environment: Host
-->
# PROOF.md: lab/66-clz-byte

Environment: gcc 13.3.0 (Ubuntu), x86_64.

`clz64_byte()` (clz_byte.h) counts the leading zeros of a `uint64_t`.
Contract: `clz64_byte(0) == 64`.

Construction.  For nonzero `x`, write the word as 8 bytes, byte 7
holding bits [56:64).  Let `B` be the most significant nonzero byte.
Every bit above `B` is zero, that is `8*(7-index(B))` zero bits, and
`B`'s own leading zeros within its 8 bits are `clz8_tab[B]`.  Leading
zeros of the whole word are therefore exactly
`8*(7-index(B)) + clz8_tab[B]`: a bit position is either above `B` (all
zero) or inside `B` (counted by the table), and the two groups are
disjoint and cover the whole word, so the sum is the count, no more
and no less.  The scan finds `B` by testing bytes from the top down.

The table is filled by `clz8_build()` (clz_byte.c) from the bit-by-bit
identity: for each byte value, shift left until a 1 reaches bit 7 and
count the shifts.  A left shift moves every bit one position toward
the most significant bit, so the number of shifts before a 1 appears
at bit 7 is exactly the number of zero bits above the highest set
bit: the definition of a byte's leading-zero count.  For the zero
byte no 1 ever arrives and the loop exhausts at 8, which is the
correct count of leading zeros inside an all-zero byte.

No UB: all shifts are on unsigned values, the largest shift is 56;
table indices come from a `uint8_t` and are in range; the builder's
loop is bounded by `n < 8`; `__builtin_clzll` is never called with 0
(the reference maps 0 to 64 explicitly).

## Hand derivations for the directed known-answer vectors

Single bit `1ULL << k`: the set bit sits in byte `i = k/8` at offset
`k%8` inside the byte.  Bytes above `i` are zero (`8*(7-i)` zeros),
and the byte's own leading zeros are `7-(k%8)`.  Total:
`8*(7-i) + 7-(k%8) = 63-k`.  So bit 0 gives 63, bit 63 gives 0.

- `0x0000000000000000` -> 64 (contract: all 8 bytes zero, scan finds
  no nonzero byte).
- `0x0000000000000001` -> byte 0 = `0x01`, 7 zero bytes above:
  `56 + clz8(0x01)`.  `0x01` needs 7 shifts to reach bit 7, so
  `56 + 7 = 63`.
- `0x8000000000000000` -> byte 7 = `0x80`, no bytes above:
  `0 + clz8(0x80) = 0`.
- `0xFFFFFFFFFFFFFFFF` -> byte 7 = `0xFF`: `0 + 0 = 0`.
- `0x7FFFFFFFFFFFFFFF` -> byte 7 = `0x7F`: `0 + 1 = 1`.
- `0x00F0000000000000` -> byte 6 = `0xF0`, 1 zero byte above:
  `8 + clz8(0xF0) = 8 + 0 = 8`.
- `0x0000000000000080` -> byte 0 = `0x80`: `56 + 0 = 56`.
- `0x0100000000000000` -> byte 6 = `0x01`: `8 + 7 = 15`.
- `0xAAAAAAAAAAAAAAAA` -> byte 7 = `0xAA` (top bit set): `0`.
- `0x5555555555555555` -> byte 7 = `0x55` (top bit clear, next set):
  `1`.

## Build log

All three configurations build with `-std=c11 -Wall -Wextra -Werror`
and zero warnings (a warning would fail the build under `-Werror`):

```
$ make clean && make
rm -f test_clz_byte test_clz_byte_asan test_clz_byte_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_clz_byte test_clz_byte.c clz_byte.c
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_clz_byte_o0 test_clz_byte.c clz_byte.c
$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_clz_byte_asan test_clz_byte.c clz_byte.c
```

## Run output

`-O2` build:

```
table checks:      256
directed checks:   74
exhaustive checks: 65536
random checks:     1000000
mismatches:        0
fnv1a checksum:    0x45C10F152E119ECB
throughput:        1.767 ns/value (30 passes x 1000000 values)
sink:              0x0000000001D8BF84
```

`-O0` build (same full suite):

```
table checks:      256
directed checks:   74
exhaustive checks: 65536
random checks:     1000000
mismatches:        0
fnv1a checksum:    0x45C10F152E119ECB
throughput:        3.986 ns/value (30 passes x 1000000 values)
sink:              0x0000000001D8BF84
```

ASan+UBSan build (same full suite; stderr empty, exit 0, so zero
sanitizer reports):

```
table checks:      256
directed checks:   74
exhaustive checks: 65536
random checks:     1000000
mismatches:        0
fnv1a checksum:    0x45C10F152E119ECB
throughput:        5.544 ns/value (30 passes x 1000000 values)
sink:              0x0000000001D8BF84
```

Totals: 256 + 74 + 65,536 + 1,000,000 = 1,065,866 checks per build,
0 mismatches in every build.  The FNV-1a checksum over every result
byte is `0x45C10F152E119ECB` in all three builds, so all three
computed identical answers.  Throughput at `-O2`: 1.767 ns/value
(about 566 Mops/s); the timed loop folds each result into the printed
sink, so the measured work cannot be discarded by the optimizer.
