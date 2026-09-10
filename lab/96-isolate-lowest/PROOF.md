<!-- PROOF-HEADER
Checks: 1065536
Mismatches: 0
Checksum: 6c73cad58aa2174f
Throughput: 2.169 ns/value at -O2
Environment: Host
-->
# PROOF.md, lab/96-isolate-lowest

`iso_lowest(x) = x & -x` for 64-bit words, with `-x` computed as
unsigned `(0 - x)` mod 2^64.

## Why the identity holds

For unsigned arithmetic on N bits, `-x` is defined as `2^N - x`
(i.e. `(0 - x)` wraps modulo 2^N). Write x with lowest set bit at
position k: bits below k are 0, bit k is 1. Then `2^N - x` flips all
bits of x and adds 1, which carries through the zeros below k and
stops at bit k. The result has bit k set and bits below k clear,
exactly matching x only at bit k. `x & -x` therefore keeps bit k and
clears every other bit. When x = 0, `(0 - 0)` is 0 and the AND is 0.

## What was verified (real runs, real numbers)

- Differential test: `iso_lowest` against a naive per-bit scanning
  reference over all 65,536 exhaustive 16-bit inputs plus 1,000,000
  fixed-seed splitmix64 64-bit values (seed 0x243F6A8885A308D3,
  increment 0x9E3779B97F4A7C15). Total 1,065,536 cases,
  1,065,535 nonzero, 0 mismatches.
- Invariants checked on every nonzero case, using an independent
  naive bit-loop popcount (no builtins): `popcount(x - iso) ==
  popcount(x) - 1` and `iso & (iso - 1) == 0`. 0 invariant failures.
- The x = 0 contract (returns 0) is asserted explicitly and covered
  by the exhaustive sweep.
- FNV-1a checksum of the full result stream, identical across three
  builds: -O0, -O2, ASan+UBSan (-O1): `6c73cad58aa2174f`.
- Clean under `-std=c11 -Wall -Wextra -Werror`: zero warnings.
- ASan+UBSan: zero reports.
- Timing at -O2: 2,168,603 ns for 1,000,000 values = 2.169 ns/value
  over a pre-generated array. Honest caveat: the measured path
  includes the array load and one XOR accumulate per value, so this
  is a per-iteration cost of the loop as written, not of the AND
  instruction alone.
- Disassembly (gcc 13.3.0, -O2, x86_64): gcc emits exactly the
  stated construction, nothing else:

```
iso_lowest:
    mov    %rdi,%rax
    neg    %rax
    and    %rdi,%rax
    ret
```

## Genuine build and run log

Build (-O2):

```
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_iso test_iso.c iso.c
```

Run (-O2):

```
cases=1065536 nonzero=1065535 mismatches=0 invariant_fails=0
fnv1a_checksum=6c73cad58aa2174f
timed: 1000000 values, 2168603.000 ns total, 2.169 ns/value
```

Build and run (-O0):

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_iso_o0 test_iso.c iso.c
./test_iso_o0
cases=1065536 nonzero=1065535 mismatches=0 invariant_fails=0
fnv1a_checksum=6c73cad58aa2174f
```

Build and run (ASan+UBSan):

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_iso_asan test_iso.c iso.c
./test_iso_asan
cases=1065536 nonzero=1065535 mismatches=0 invariant_fails=0
fnv1a_checksum=6c73cad58aa2174f
```

No sanitizer output appeared, which is the expected clean result.

## Scope

One identity, one function, 65,536 exhaustive inputs plus one
million fixed-seed random inputs, two mathematical invariants plus
the zero contract. Nothing else is claimed.
