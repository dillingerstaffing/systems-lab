<!-- PROOF-HEADER
Checks: 2065536
Mismatches: 0
Checksum: d42f33eaf01cd639
Throughput: 4.26 ns/value (234.6 Mvalues/s) at -O2 over 25,000,000 values
Environment: Host
Verdict: PASS
-->

# PROOF: lab/35-parity-fold

`parity64(uint64_t x)` in `parity.c`, built only from the xor-fold
reduction: the XOR of all 64 bits equals parity(x), and folding the
high half into the low half (`x ^= x >> k`) preserves that XOR because
for x = hi:lo, parity(hi ^ lo) = parity(hi) ^ parity(lo) = parity(x).
Folding k = 32, 16, 8, 4, 2, 1 leaves the XOR of all 64 original bits
in bit 0; masking isolates it. No library popcount or parity builtin
is used in the implementation. 2,065,536 checks against a naive
per-bit-loop reference, 0 mismatches; the homomorphism
parity(a ^ b) == parity(a) ^ parity(b) verified on every pair.
FNV-1a 64 checksum over the result stream identical across -O0, -O2,
and ASan+UBSan builds.

## Build log (verbatim, `make`)

```
$ make
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_parity_o0 test_parity.c parity.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_parity_o2 test_parity.c parity.c
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_parity_asan test_parity.c parity.c
BUILD_EXIT=0
```

Zero warnings under `-Wall -Wextra -Werror` on all three targets.

## Run output (verbatim)

-O0 binary:
```
$ ./test_parity_o0
total_checks=2065536 mismatches=0 fnv1a=d42f33eaf01cd639
timed_values=25000000 ns_total=210513760 ns_per_value=8.42 Mvalues_per_sec=118.8
sink=12499150
PASS
```

-O2 binary:
```
$ ./test_parity_o2
total_checks=2065536 mismatches=0 fnv1a=d42f33eaf01cd639
timed_values=25000000 ns_total=106574926 ns_per_value=4.26 Mvalues_per_sec=234.6
sink=12499150
PASS
```

ASan+UBSan binary (-O1 -g, sanitizers on):
```
$ ./test_parity_asan
total_checks=2065536 mismatches=0 fnv1a=d42f33eaf01cd639
timed_values=25000000 ns_total=126569658 ns_per_value=5.06 Mvalues_per_sec=197.5
sink=12499150
PASS
```

No sanitizer report on any binary (ASan/UBSan print to stderr; stderr
was empty in all three runs).

## What the numbers mean

- `total_checks=2065536`: all 65,536 16-bit inputs exhausted, each
  differential-checked against the naive per-bit loop, plus 1,000,000
  fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked against the same
  reference, plus the homomorphism `parity(a ^ b) ==
  parity(a) ^ parity(b)` checked on 1,000,000 pairs drawn from a
  distinct fixed seed (`0xDEADBEEFCAFEBABE`).
- `mismatches=0`: the fold agreed with the reference on every one of
  the 1,065,536 differential cases and satisfied the homomorphism on
  every one of the 1,000,000 pairs.
- `fnv1a=d42f33eaf01cd639` byte-identical on all three builds: the same
  deterministic input streams produced the same result stream and the
  same hash under -O0, -O2, and sanitizers, so the optimizer and the
  sanitizer instrumentation changed no answer.
- `timed_values=25000000` at -O2: 4.26 ns/value, 234.6 Mvalues/s. The
  timed loop only replays the stored 1M values through `parity64` into
  an accumulated sink, so this is the measured fold cost at -O2 on this
  machine (gcc 13.3.0, x86-64).
- `sink=12499150`: identical in all three runs, so the optimizer did
  not drop or alter the timed computation.
