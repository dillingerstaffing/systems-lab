<!-- PROOF-HEADER
Checks: 4295967301
Mismatches: 0
Checksum: e7c096581363da96
Throughput: 4.85 ns/value at -O2, best of 5
Environment: Host
-->
# PROOF: lab/88-inc128-carry

`inc128(x)`: add 1 to the 128-bit value held as `(hi, lo)` of two
`uint64_t`, returning the new `(hi, lo)` plus `carry`, the carry out
of bit 127.

## What was built

`inc128.h`, `inc128.c`, `test_inc128.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`, no
intrinsics, no builtins. The 128-bit type `unsigned __int128`
appears only in the test oracle; the implementation uses only
`uint64_t` arithmetic. Proof by grep:

```
$ grep -rn "__int128\|__builtin\|__intrinsic" inc128.c inc128.h
(no output, exit 1)
$ grep -c "__int128" test_inc128.c
2
```

(The 2 hits in the test file are the `typedef unsigned __int128
u128ref;` oracle definition and its mention in the comment above
it.)

## Derivation of the carry identity

The implementation is three lines. Each step rests on one fact:
unsigned arithmetic in C wraps modulo 2^N (C11 6.2.5p9).

**Step 1: the carry into the high word.** `lo2 = x.lo + 1` is
`(x.lo + 1) mod 2^64`. `lo2 == 0` holds exactly when
`x.lo + 1 == 2^64`, i.e. exactly when `x.lo == UINT64_MAX`. So
`carry = (lo2 == 0)` is 1 exactly when the low word overflowed,
which is precisely the carry that adding 1 to the 128-bit value
propagates into the high word.

**Step 2: the high word.** `hi2 = x.hi + carry` with
`carry` in {0, 1}. If `carry == 0`, `hi2 = x.hi` and no wrap is
possible. If `carry == 1`, `hi2 == 0` holds exactly when
`x.hi == UINT64_MAX`. Either way `hi2` is `(x.hi + carry) mod 2^64`,
the correct high word of `(x + 1) mod 2^128`.

**Step 3: the final carry out.** The carry out of bit 127 is 1
exactly when `(x + 1) mod 2^128` wrapped, i.e. exactly when the
input `x` was `2^128 - 1`, i.e. `x.hi == UINT64_MAX` and
`x.lo == UINT64_MAX`. The code reports `(lo2 == 0 && hi2 == 0)`:
`lo2 == 0` holds exactly when `x.lo == UINT64_MAX` (step 1, so
`carry == 1`), and then `hi2 == 0` holds exactly when
`x.hi == UINT64_MAX` (step 2). The conjunction therefore holds
exactly when both words were all ones, which is exactly the
full-wrap condition. No other combination of inputs can set it.

The oracle checks the same truth from the other side: with
`x` as `unsigned __int128`, `ref = x + 1` wraps modulo 2^128, and
`carry_ref = (x == (unsigned __int128)-1)`, i.e. 1 exactly when the
input was all ones. The test asserts `new_hi == ref >> 64`,
`new_lo == (uint64_t)ref`, and `carry_out == carry_ref` on every
case.

## Verification plan

1. Directed edge rows exercising the cascade:
   `(0,0)`, `(0,UINT64_MAX)`, `(UINT64_MAX,0)`,
   `(UINT64_MAX,UINT64_MAX)`, `(1,UINT64_MAX)`.
2. Exhaustive differential test over all 2^32 pairs `(hi, lo)` with
   `hi, lo` in `[0, 2^16)`, against the `unsigned __int128` oracle,
   final carry checked on every case.
3. 1,000,000 fixed-seed `splitmix64` random 64-bit pairs (seed
   `0x123456789ABCDEF0`), same per-case oracle check.
4. FNV-1a 64-bit checksum over the full output stream must be
   identical across `-O0`, `-O2`, and ASan+UBSan builds.
5. Throughput at -O2, best of 5 runs of 100M values (inputs from
   `splitmix64` inside the timed loop; stated honestly).
6. Zero warnings under `-Wall -Wextra -Werror`; zero sanitizer
   reports.

## Genuine build log and run output

```
$ make clean && make && make test_inc128_O0 test_inc128_san && make bench
rm -f test_inc128 test_inc128_O0 test_inc128_san test_inc128_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_inc128 test_inc128.c inc128.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_inc128_O0 test_inc128.c inc128.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_inc128_san test_inc128.c inc128.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_inc128_bench test_inc128.c inc128.c
```

(No warnings from any compile line; `-Werror` is on, so the build
would have failed otherwise.)

```
$ ./test_inc128
edge hi=0000000000000000 lo=0000000000000000 -> hi=0000000000000000 lo=0000000000000001 carry=0
edge hi=0000000000000000 lo=ffffffffffffffff -> hi=0000000000000001 lo=0000000000000000 carry=0
edge hi=ffffffffffffffff lo=0000000000000000 -> hi=ffffffffffffffff lo=0000000000000001 carry=0
edge hi=ffffffffffffffff lo=ffffffffffffffff -> hi=0000000000000000 lo=0000000000000000 carry=1
edge hi=0000000000000001 lo=ffffffffffffffff -> hi=0000000000000002 lo=0000000000000000 carry=0
checks=4295967301 mismatches=0 fnv1a=e7c096581363da96
(real 0m28.886s)

$ ./test_inc128_O0
edge hi=0000000000000000 lo=0000000000000000 -> hi=0000000000000000 lo=0000000000000001 carry=0
edge hi=0000000000000000 lo=ffffffffffffffff -> hi=0000000000000001 lo=0000000000000000 carry=0
edge hi=ffffffffffffffff lo=0000000000000000 -> hi=ffffffffffffffff lo=0000000000000001 carry=0
edge hi=ffffffffffffffff lo=ffffffffffffffff -> hi=0000000000000000 lo=0000000000000000 carry=1
edge hi=0000000000000001 lo=ffffffffffffffff -> hi=0000000000000002 lo=0000000000000000 carry=0
checks=4295967301 mismatches=0 fnv1a=e7c096581363da96
(real 1m23.989s)

$ ./test_inc128_san
edge hi=0000000000000000 lo=0000000000000000 -> hi=0000000000000000 lo=0000000000000001 carry=0
edge hi=0000000000000000 lo=ffffffffffffffff -> hi=0000000000000001 lo=0000000000000000 carry=0
edge hi=ffffffffffffffff lo=0000000000000000 -> hi=ffffffffffffffff lo=0000000000000001 carry=0
edge hi=ffffffffffffffff lo=ffffffffffffffff -> hi=0000000000000000 lo=0000000000000000 carry=1
edge hi=0000000000000001 lo=ffffffffffffffff -> hi=0000000000000002 lo=0000000000000000 carry=0
checks=4295967301 mismatches=0 fnv1a=e7c096581363da96
(real 4m33.433s)
```

The sanitizer binary exited 0 with `-fno-sanitize-recover=all`,
so there were no ASan/UBSan reports. The FNV-1a checksum
`e7c096581363da96` is identical across all three builds.

## Throughput

```
$ ./test_inc128_bench
(prints the same 5 edge lines, then:)
checks=4295967301 mismatches=0 fnv1a=e7c096581363da96
bench: 4.85 ns/value (206.1 Mvalues/s over 100M timed values, best of 5)
```

Inputs for the timed loop come from `splitmix64` inside the loop, so
the measured time includes input generation; the compiler cannot
fold the loop because the generator state is carried between
iterations. The sink is `volatile`.

## Exactly what was verified

- 4,295,967,301 checks (5 edge rows + 2^32 exhaustive pairs +
  1,000,000 random pairs), 0 mismatches against the
  `unsigned __int128` oracle; the final carry was checked on every
  single case.
- Checksum `e7c096581363da96` identical across `-O0`, `-O2`, and
  ASan+UBSan; no sanitizer reports.
- Throughput 4.85 ns/value at -O2 (best of 5).
- Build is warning-free under `-Wall -Wextra -Werror`.
- The implementation contains no `__int128`, no intrinsics, no
  builtins (grep above); `__int128` exists only in the test oracle.

Built and tested 2026-09-10. Toolchain: gcc 13.3.0 (Ubuntu) on
x86-64.
