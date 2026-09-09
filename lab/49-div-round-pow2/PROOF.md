# PROOF: lab/49-div-round-pow2

Genuine build log and run output for all four build configs, captured on
2026-09-09 (gcc 13.3.0, Ubuntu 24.04, x86-64). Every number below is copied
from the run output; nothing is fabricated.

## make test-o0

```
make test-o0
gcc -std=c11 -Wall -Wextra -Werror -O0 -g -o test_o0 test_divround.c
vector 1.5 rounds up: x=3 k=1 -> 2: pass
vector exact 1: x=2 k=1 -> 1: pass
vector 1.5 rounds up at k=2: x=6 k=2 -> 2: pass
vector 1.25 rounds down: x=5 k=2 -> 1: pass
vector 1.75 rounds up: x=7 k=2 -> 2: pass
vector zero stays zero: x=0 k=7 -> 0: pass
vector tie x=2^(k-1) rounds up: x=64 k=7 -> 1: pass
vector just below tie rounds down: x=63 k=7 -> 0: pass
vector 0.5 tie at k=1 rounds up: x=1 k=1 -> 1: pass
vector k=0 identity: x=3735928559 k=0 -> 3735928559: pass
vector k=0 zero: x=0 k=0 -> 0: pass
vector k=32 tie x=2^31 rounds up: x=2147483648 k=32 -> 1: pass
vector k=32 just below tie: x=2147483647 k=32 -> 0: pass
vector k=32 top of range: x=4294967295 k=32 -> 1: pass
vector k=32 zero: x=0 k=32 -> 0: pass
vector k=31 full-range, needs 64-bit add: x=4294967295 k=31 -> 2: pass
vector k=1 max x (2^32-1)/2 rounds up: x=4294967295 k=1 -> 2147483648: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1983040 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983040 checks, 0 mismatches
fnv1a-64 over all results: 0xee4a225fd47d1345
benchmark: 40000000 calls in 0.394 s = 9.8 ns/value (sink=2286162894)
ALL TESTS PASSED
```

## make test-o2

```
=== make test-o2 ===
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_o2 test_divround.c
./test_o2
vector 1.5 rounds up: x=3 k=1 -> 2: pass
vector exact 1: x=2 k=1 -> 1: pass
vector 1.5 rounds up at k=2: x=6 k=2 -> 2: pass
vector 1.25 rounds down: x=5 k=2 -> 1: pass
vector 1.75 rounds up: x=7 k=2 -> 2: pass
vector zero stays zero: x=0 k=7 -> 0: pass
vector tie x=2^(k-1) rounds up: x=64 k=7 -> 1: pass
vector just below tie rounds down: x=63 k=7 -> 0: pass
vector 0.5 tie at k=1 rounds up: x=1 k=1 -> 1: pass
vector k=0 identity: x=3735928559 k=0 -> 3735928559: pass
vector k=0 zero: x=0 k=0 -> 0: pass
vector k=32 tie x=2^31 rounds up: x=2147483648 k=32 -> 1: pass
vector k=32 just below tie: x=2147483647 k=32 -> 0: pass
vector k=32 top of range: x=4294967295 k=32 -> 1: pass
vector k=32 zero: x=0 k=32 -> 0: pass
vector k=31 full-range, needs 64-bit add: x=4294967295 k=31 -> 2: pass
vector k=1 max x (2^32-1)/2 rounds up: x=4294967295 k=1 -> 2147483648: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1983040 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983040 checks, 0 mismatches
fnv1a-64 over all results: 0xee4a225fd47d1345
benchmark: 40000000 calls in 0.195 s = 4.9 ns/value (sink=2286162894)
ALL TESTS PASSED
```

## make test-asan

```
=== make test-asan ===
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-sanitize-recover=all -o test_asan test_divround.c
./test_asan
vector 1.5 rounds up: x=3 k=1 -> 2: pass
vector exact 1: x=2 k=1 -> 1: pass
vector 1.5 rounds up at k=2: x=6 k=2 -> 2: pass
vector 1.25 rounds down: x=5 k=2 -> 1: pass
vector 1.75 rounds up: x=7 k=2 -> 2: pass
vector zero stays zero: x=0 k=7 -> 0: pass
vector tie x=2^(k-1) rounds up: x=64 k=7 -> 1: pass
vector just below tie rounds down: x=63 k=7 -> 0: pass
vector 0.5 tie at k=1 rounds up: x=1 k=1 -> 1: pass
vector k=0 identity: x=3735928559 k=0 -> 3735928559: pass
vector k=0 zero: x=0 k=0 -> 0: pass
vector k=32 tie x=2^31 rounds up: x=2147483648 k=32 -> 1: pass
vector k=32 just below tie: x=2147483647 k=32 -> 0: pass
vector k=32 top of range: x=4294967295 k=32 -> 1: pass
vector k=32 zero: x=0 k=32 -> 0: pass
vector k=31 full-range, needs 64-bit add: x=4294967295 k=31 -> 2: pass
vector k=1 max x (2^32-1)/2 rounds up: x=4294967295 k=1 -> 2147483648: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1983040 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983040 checks, 0 mismatches
fnv1a-64 over all results: 0xee4a225fd47d1345
benchmark: 40000000 calls in 0.094 s = 2.4 ns/value (sink=2286162894)
ALL TESTS PASSED
```

## make test-ubsan

```
=== make test-ubsan ===
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined \
	-fno-sanitize-recover=all -o test_ubsan test_divround.c
./test_ubsan
vector 1.5 rounds up: x=3 k=1 -> 2: pass
vector exact 1: x=2 k=1 -> 1: pass
vector 1.5 rounds up at k=2: x=6 k=2 -> 2: pass
vector 1.25 rounds down: x=5 k=2 -> 1: pass
vector 1.75 rounds up: x=7 k=2 -> 2: pass
vector zero stays zero: x=0 k=7 -> 0: pass
vector tie x=2^(k-1) rounds up: x=64 k=7 -> 1: pass
vector just below tie rounds down: x=63 k=7 -> 0: pass
vector 0.5 tie at k=1 rounds up: x=1 k=1 -> 1: pass
vector k=0 identity: x=3735928559 k=0 -> 3735928559: pass
vector k=0 zero: x=0 k=0 -> 0: pass
vector k=32 tie x=2^31 rounds up: x=2147483648 k=32 -> 1: pass
vector k=32 just below tie: x=2147483647 k=32 -> 0: pass
vector k=32 top of range: x=4294967295 k=32 -> 1: pass
vector k=32 zero: x=0 k=32 -> 0: pass
vector k=31 full-range, needs 64-bit add: x=4294967295 k=31 -> 2: pass
vector k=1 max x (2^32-1)/2 rounds up: x=4294967295 k=1 -> 2147483648: pass
vectors: 17/17 passed
exhaustive: 983040 checks, 0 mismatches
random: 1983040 checks, 0 mismatches (seed 0x243F6A8885A308D3)
total: 1983040 checks, 0 mismatches
fnv1a-64 over all results: 0xee4a225fd47d1345
benchmark: 40000000 calls in 0.283 s = 7.1 ns/value (sink=2286162894)
ALL TESTS PASSED
```
