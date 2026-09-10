# PROOF.md - lab/71-fmix64

`fmix64`: the 64-bit finalization mix of MurmurHash3_x64_128, in
`fmix64.h`, with `fmix64_inv`, the exact inverse built step by step
in reverse order.

Why the inverse is exact:

- Shift-xor step `y = x ^ (x >> 33)`: the low 33 bits of `y` are the
  low 33 bits of `x`.  Folding `x ^= x >> s` with `s` doubling fixes
  the next block exactly, because for the newly covered bits
  `i .. 2i`: `x_new[i..2i) = y[i..2i) ^ x_old[0..i)`
  `= x_true[i..2i) ^ x_true[0..i) ^ x_true[0..i) = x_true[i..2i)`.
  With s = 33 the loop runs once (66 >= 64 exits), recovering all
  bits.
- Multiply steps: each constant is odd, so `gcd(c, 2^64) = 1` and a
  modular inverse exists.  The header constants were found by
  Newton-Raphson iteration `inv <- inv * (2 - c * inv)` from inv = 1,
  doubling correct bits per round (1 -> 2 -> 4 -> 8 -> 16 -> 32 ->
  64); the test independently re-derives both inverses from scratch
  and checks `inv * c == 1` (mod 2^64).

Each of the five forward steps is a bijection on 64-bit words, so
the reverse-order composition is the true inverse:
`fmix64_inv(fmix64(x)) == x` for every x.

Reference provenance: `fmix64_ref.c` holds the published
MurmurHash3 fmix64 text (Austin Appleby, MurmurHash3.cpp, public
domain) with the symbol renamed so it links into the same binary.
The constant values were cross-checked against three independent
published ports (Apache commons-codec MurmurHash3.java, ns-3
hash-murmur3, and a mellifera commit quoting the smhasher source
line); all agree on the five operations and both constants.

Scope note: the verification covers every aspect of the module's
claims, differential against the published source and the inverse
round-trip on all inputs exercised.  No timing or avalanche-quality
claims are made; throughput is reported as measured, nothing more.

## Build log (verbatim)

```
$ make clean && make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_fmix64_o0 test_fmix64.c fmix64_ref.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_fmix64_o2 test_fmix64.c fmix64_ref.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_fmix64_asan test_fmix64.c fmix64_ref.c
```

Zero warnings under `-Wall -Wextra -Werror`.

## Run output, build -O2 (verbatim)

```
fmix64 differential + bijection test, build -O2
[1/5] anchor fmix64(0) == 0
  fmix64(0) == 0, as each of the five steps fixes 0
[2/5] multiply-inverse constants
  C1: header inverse 0x4f74430c22a54005 verified, inv * c == 1
  C2: header inverse 0x9cb4b2f8129337db verified, inv * c == 1
[3/5] 10M fixed-seed splitmix64 64-bit values
  differential mismatches: 0
  bijection mismatches:    0
  done: cases=10000000
[4/5] directed edge cases
  done: cases=161 mismatches=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 1.535 ns/value (651.471 M values/s)
  throughput pass 1: 1.594 ns/value (627.237 M values/s)
  throughput pass 2: 1.597 ns/value (626.236 M values/s)
  throughput pass 3: 1.491 ns/value (670.480 M values/s)
  throughput pass 4: 1.643 ns/value (608.700 M values/s)
  throughput best of 5: 1.491 ns/value (670.480 M values/s)
total verification cases: 10000162
total mismatches: 0
FNV-1a checksum of all outputs: 0xa893938fd43cc2de
RESULT: PASS
```

## Run output, build asan+ubsan (verbatim)

```
fmix64 differential + bijection test, build asan+ubsan
[1/5] anchor fmix64(0) == 0
  fmix64(0) == 0, as each of the five steps fixes 0
[2/5] multiply-inverse constants
  C1: header inverse 0x4f74430c22a54005 verified, inv * c == 1
  C2: header inverse 0x9cb4b2f8129337db verified, inv * c == 1
[3/5] 10M fixed-seed splitmix64 64-bit values
  differential mismatches: 0
  bijection mismatches:    0
  done: cases=10000000
[4/5] directed edge cases
  done: cases=161 mismatches=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 2.731 ns/value (366.136 M values/s)
  throughput pass 1: 2.785 ns/value (359.071 M values/s)
  throughput pass 2: 2.900 ns/value (344.884 M values/s)
  throughput pass 3: 6.869 ns/value (145.588 M values/s)
  throughput pass 4: 2.877 ns/value (347.644 M values/s)
  throughput best of 5: 2.731 ns/value (366.136 M values/s)
total verification cases: 10000162
total mismatches: 0
FNV-1a checksum of all outputs: 0xa893938fd43cc2de
RESULT: PASS
```

No AddressSanitizer or UBSan findings on the full suite (no
sanitizer output; the run exited 0).

## Run output, build -O0 (verbatim)

```
fmix64 differential + bijection test, build -O0
[1/5] anchor fmix64(0) == 0
  fmix64(0) == 0, as each of the five steps fixes 0
[2/5] multiply-inverse constants
  C1: header inverse 0x4f74430c22a54005 verified, inv * c == 1
  C2: header inverse 0x9cb4b2f8129337db verified, inv * c == 1
[3/5] 10M fixed-seed splitmix64 64-bit values
  differential mismatches: 0
  bijection mismatches:    0
  done: cases=10000000
[4/5] directed edge cases
  done: cases=161 mismatches=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 4.023 ns/value (248.547 M values/s)
  throughput pass 1: 4.169 ns/value (239.885 M values/s)
  throughput pass 2: 5.475 ns/value (182.635 M values/s)
  throughput pass 3: 4.069 ns/value (245.738 M values/s)
  throughput pass 4: 4.023 ns/value (248.589 M values/s)
  throughput best of 5: 4.023 ns/value (248.589 M values/s)
total verification cases: 10000162
total mismatches: 0
FNV-1a checksum of all outputs: 0xa893938fd43cc2de
RESULT: PASS
```

The FNV-1a checksum over every output word is
`0xa893938fd43cc2de` in all three builds.

## Throughput methodology

The timed loop contains only `fmix64` calls xored into a `volatile`
sink (so the loop cannot be folded away).  The 4M inputs are
generated once with splitmix64 before timing starts, so the PRNG
step is not part of the measured time.  Best of 5 passes is
reported; all 5 passes are listed above.
