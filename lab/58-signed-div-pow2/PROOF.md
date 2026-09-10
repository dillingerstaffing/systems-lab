# PROOF: lab/58-signed-div-pow2
Genuine build log and run output for all four build configs, captured on
2026-09-09 (gcc 13.3.0, Ubuntu 24.04, x86-64). Every number below is copied
from the run output; nothing is fabricated.

## make test-o0

```
=== make test-o0 ===
gcc -std=c11 -Wall -Wextra -Werror -O0 -g -o test_o0 test_sdivpow2.c
./test_o0
vector positive odd: x=7 k=1 -> 3: pass
vector negative odd truncates toward zero (shift alone gives -4): x=-7 k=1 -> -3: pass
vector x=-7 k=2 truncates toward zero (shift alone gives -2): x=-7 k=2 -> -1: pass
vector negative exact multiple: x=-8 k=2 -> -2: pass
vector x=-1 k=1 truncates to 0 (shift alone gives -1): x=-1 k=1 -> 0: pass
vector x=-1 k=63: x=-1 k=63 -> 0: pass
vector zero stays zero: x=0 k=5 -> 0: pass
vector INT64_MIN / 2: x=-9223372036854775808 k=1 -> -4611686018427387904: pass
vector INT64_MIN / 2^63 = -1: x=-9223372036854775808 k=63 -> -1: pass
vector INT64_MAX / 2^63 truncates to 0: x=9223372036854775807 k=63 -> 0: pass
vector INT64_MAX / 2: x=9223372036854775807 k=1 -> 4611686018427387903: pass
vector x=-2^62 k=62 exact -1: x=-4611686018427387904 k=62 -> -1: pass
vector x=-2^62-1 k=62 truncates to -1: x=-4611686018427387905 k=62 -> -1: pass
vector k=0 identity, positive: x=5 k=0 -> 5: pass
vector k=0 identity, negative: x=-5 k=0 -> -5: pass
vector k=64 quotient below 1: x=123 k=64 -> 0: pass
vector k=64 INT64_MIN quotient below 1: x=-9223372036854775808 k=64 -> 0: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1000000 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983057 checks, 0 mismatches
fnv1a-64 over all results: 0x3fbd3c3962be4d43
benchmark: 40000000 calls in 0.226 s = 5.7 ns/value (sink=7297694502670326398)
ALL TESTS PASSED
```

## make test-o2

```
=== make test-o2 ===
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_o2 test_sdivpow2.c
./test_o2
vector positive odd: x=7 k=1 -> 3: pass
vector negative odd truncates toward zero (shift alone gives -4): x=-7 k=1 -> -3: pass
vector x=-7 k=2 truncates toward zero (shift alone gives -2): x=-7 k=2 -> -1: pass
vector negative exact multiple: x=-8 k=2 -> -2: pass
vector x=-1 k=1 truncates to 0 (shift alone gives -1): x=-1 k=1 -> 0: pass
vector x=-1 k=63: x=-1 k=63 -> 0: pass
vector zero stays zero: x=0 k=5 -> 0: pass
vector INT64_MIN / 2: x=-9223372036854775808 k=1 -> -4611686018427387904: pass
vector INT64_MIN / 2^63 = -1: x=-9223372036854775808 k=63 -> -1: pass
vector INT64_MAX / 2^63 truncates to 0: x=9223372036854775807 k=63 -> 0: pass
vector INT64_MAX / 2: x=9223372036854775807 k=1 -> 4611686018427387903: pass
vector x=-2^62 k=62 exact -1: x=-4611686018427387904 k=62 -> -1: pass
vector x=-2^62-1 k=62 truncates to -1: x=-4611686018427387905 k=62 -> -1: pass
vector k=0 identity, positive: x=5 k=0 -> 5: pass
vector k=0 identity, negative: x=-5 k=0 -> -5: pass
vector k=64 quotient below 1: x=123 k=64 -> 0: pass
vector k=64 INT64_MIN quotient below 1: x=-9223372036854775808 k=64 -> 0: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1000000 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983057 checks, 0 mismatches
fnv1a-64 over all results: 0x3fbd3c3962be4d43
benchmark: 40000000 calls in 0.092 s = 2.3 ns/value (sink=7297694502670326398)
ALL TESTS PASSED
```

## make test-asan

```
=== make test-asan ===
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-sanitize-recover=all -o test_asan test_sdivpow2.c
./test_asan
vector positive odd: x=7 k=1 -> 3: pass
vector negative odd truncates toward zero (shift alone gives -4): x=-7 k=1 -> -3: pass
vector x=-7 k=2 truncates toward zero (shift alone gives -2): x=-7 k=2 -> -1: pass
vector negative exact multiple: x=-8 k=2 -> -2: pass
vector x=-1 k=1 truncates to 0 (shift alone gives -1): x=-1 k=1 -> 0: pass
vector x=-1 k=63: x=-1 k=63 -> 0: pass
vector zero stays zero: x=0 k=5 -> 0: pass
vector INT64_MIN / 2: x=-9223372036854775808 k=1 -> -4611686018427387904: pass
vector INT64_MIN / 2^63 = -1: x=-9223372036854775808 k=63 -> -1: pass
vector INT64_MAX / 2^63 truncates to 0: x=9223372036854775807 k=63 -> 0: pass
vector INT64_MAX / 2: x=9223372036854775807 k=1 -> 4611686018427387903: pass
vector x=-2^62 k=62 exact -1: x=-4611686018427387904 k=62 -> -1: pass
vector x=-2^62-1 k=62 truncates to -1: x=-4611686018427387905 k=62 -> -1: pass
vector k=0 identity, positive: x=5 k=0 -> 5: pass
vector k=0 identity, negative: x=-5 k=0 -> -5: pass
vector k=64 quotient below 1: x=123 k=64 -> 0: pass
vector k=64 INT64_MIN quotient below 1: x=-9223372036854775808 k=64 -> 0: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1000000 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983057 checks, 0 mismatches
fnv1a-64 over all results: 0x3fbd3c3962be4d43
benchmark: 40000000 calls in 0.115 s = 2.9 ns/value (sink=7297694502670326398)
ALL TESTS PASSED
```

## make test-ubsan

```
=== make test-ubsan ===
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined \
	-fno-sanitize-recover=all -o test_ubsan test_sdivpow2.c
./test_ubsan
vector positive odd: x=7 k=1 -> 3: pass
vector negative odd truncates toward zero (shift alone gives -4): x=-7 k=1 -> -3: pass
vector x=-7 k=2 truncates toward zero (shift alone gives -2): x=-7 k=2 -> -1: pass
vector negative exact multiple: x=-8 k=2 -> -2: pass
vector x=-1 k=1 truncates to 0 (shift alone gives -1): x=-1 k=1 -> 0: pass
vector x=-1 k=63: x=-1 k=63 -> 0: pass
vector zero stays zero: x=0 k=5 -> 0: pass
vector INT64_MIN / 2: x=-9223372036854775808 k=1 -> -4611686018427387904: pass
vector INT64_MIN / 2^63 = -1: x=-9223372036854775808 k=63 -> -1: pass
vector INT64_MAX / 2^63 truncates to 0: x=9223372036854775807 k=63 -> 0: pass
vector INT64_MAX / 2: x=9223372036854775807 k=1 -> 4611686018427387903: pass
vector x=-2^62 k=62 exact -1: x=-4611686018427387904 k=62 -> -1: pass
vector x=-2^62-1 k=62 truncates to -1: x=-4611686018427387905 k=62 -> -1: pass
vector k=0 identity, positive: x=5 k=0 -> 5: pass
vector k=0 identity, negative: x=-5 k=0 -> -5: pass
vector k=64 quotient below 1: x=123 k=64 -> 0: pass
vector k=64 INT64_MIN quotient below 1: x=-9223372036854775808 k=64 -> 0: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1000000 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983057 checks, 0 mismatches
fnv1a-64 over all results: 0x3fbd3c3962be4d43
benchmark: 40000000 calls in 0.115 s = 2.9 ns/value (sink=7297694502670326398)
ALL TESTS PASSED
```

## Notes

- The FNV-1a fingerprint 0x3fbd3c3962be4d43 is identical across all four
  configs, so every build produced bit-identical results on 1,983,057 checks.
- During development the benchmark's accumulator was a signed int64 and
  UBSan flagged its overflow (test-harness arithmetic, not the module).
  The accumulator is now uint64_t; the rerun above is clean with
  -fno-sanitize-recover=all, meaning any sanitizer report would abort.
- The sanitizer builds also confirm the header's no-overflow claim:
  x + bias executed on INT64_MIN and 1M random int64 values with no
  signed-overflow report.
