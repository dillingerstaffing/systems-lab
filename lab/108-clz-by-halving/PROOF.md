<!-- PROOF-HEADER
Checks: 10065537
Mismatches: 0
Checksum: bb15711279c99def
Throughput: 7.56 ns/value at -O2 (best of 5)
Environment: Host
Verdict: PASS
-->
# PROOF: lab/108-clz-by-halving

`clz64_halving(x)`: count leading zeros of a `uint64_t`, computed
from the binary-search halving identity, built only from
shift/compare identities. No clz-class instruction in the
implementation.

## What was built

`clz_half.h`, `clz_half.c`, `test_clz_half.c`, `Makefile`,
`README.md`, this file. Plain C11,
`-std=c11 -Wall -Wextra -Werror`. No intrinsics, no builtins in the
implementation. `__builtin_clzll` is used only as the test oracle,
never in `clz_half.c`.

## Derivation

Definitions: for `x > 0`, let p be the position of the top set bit
(0..63); the answer is `63 - p`.

Each step has a half size k in {32, 16, 8, 4, 2, 1} and does:
`if ((x >> (64 - k)) == 0) { n += k; x <<= k; }`. Let n be the
running count; then n is also the total left shift applied to x so
far, so the top set bit of the shifted x sits at position p + n.
Define `remaining = 63 - p - n`, the number of leading zeros not
yet counted.

Step correctness: the test `(x >> (64 - k)) == 0` says the top k
bits of the shifted x are zero, which holds exactly when
`p + n <= 63 - k`, i.e. exactly when `remaining >= k`.
  - If true, those k bits are leading zeros: `n += k` and
    `x <<= k` preserve the invariant, `remaining` drops by k.
  - If false, `remaining < k`, so the top set bit lies inside the
    top k bits; none of them is a leading zero and x is left
    alone.

Shifting left by k only moves set bits upward; bits shifted out
were counted zeros, so nothing is lost.

Termination: the halves 32+16+8+4+2+1 sum to 63, and each step
subtracts k exactly when `remaining >= k`. This greedy halving
reduces any `remaining` in 0..63 to 0. Since `63 - p <= 63`,
`remaining` ends at 0, so `n = 63 - p` exactly.

Zero: `x = 0` is pinned to 64 by contract before any step.

No clz-class instruction: the test disassembles `clz_half_O2.o`
with objdump (via popen) and fails on any first-token match for
`bsr`, `bsf`, `lzcnt`, or `tzcnt`. The actual -O2 disassembly of
`clz64_halving` is `shr`/`shl`/`test`/`add`/`mov`/`not` only (six
`shr`-test-`shl`-add steps, one per half).

## Build log (genuine output)

```
$ make test_clz_half test_clz_half_O0 test_clz_half_san test_clz_half_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -c -o clz_half_O2.o clz_half.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_clz_half test_clz_half.c clz_half_O2.o
gcc -std=c11 -Wall -Wextra -Werror -O0 -c -o clz_half_O0.o clz_half.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_clz_half_O0 test_clz_half.c clz_half_O0.o
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -c -o clz_half_san.o clz_half.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_clz_half_san test_clz_half.c clz_half_san.o
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_clz_half_bench test_clz_half.c clz_half_O2.o
```

Zero warnings on all seven compile/link steps (warnings are
errors).

## Run logs (genuine output)

`-O2`:
```
disasm check: no bsr/bsf/lzcnt/tzcnt in clz_half_O2.o OK
x=0 contract: clz64_halving(0) = 64 OK
checks: 10065537
mismatches: 0
checksum: bb15711279c99def
```

`-O0`:
```
disasm check: no bsr/bsf/lzcnt/tzcnt in clz_half_O2.o OK
x=0 contract: clz64_halving(0) = 64 OK
checks: 10065537
mismatches: 0
checksum: bb15711279c99def
```

ASan+UBSan:
```
disasm check: no bsr/bsf/lzcnt/tzcnt in clz_half_O2.o OK
x=0 contract: clz64_halving(0) = 64 OK
checks: 10065537
mismatches: 0
checksum: bb15711279c99def
```

All three builds agree: 10,065,537 checks, 0 mismatches, one
checksum. The checksum `bb15711279c99def` is the FNV-1a 64-bit hash
of the result stream; it matches the checksum recorded in
lab/115-clz-fp, which is expected because both labs hash the clz
results over the identical input stream (same exhaustive 16-bit
pass, same seed `0x123456789ABCDEF0`, same order).

Independent manual check on the -O2 object file:
```
$ objdump -d --no-show-raw-insn clz_half_O2.o | grep -cE '\s(bsr|bsf|lzcnt|tzcnt)\s'
0
```

## Throughput (genuine output, -O2)

```
$ ./test_clz_half_bench
rep 0: 7.622 ns/value (sink 0)
rep 1: 7.559 ns/value (sink 783842)
rep 2: 8.248 ns/value (sink 0)
rep 3: 7.559 ns/value (sink 783842)
rep 4: 8.906 ns/value (sink 0)
best: 7.559 ns/value
```

A second run gave best 7.577 ns/value. Best of 5 over a 1M-value
buffer: 7.56 ns/value.

## What was verified

- The invariant proof above covers every input; the differential
  test re-checked 10,065,537 inputs (exhaustive 16-bit + 10M
  fixed-seed 64-bit) against `__builtin_clzll` with 0 mismatches.
- `x = 0` returns 64 by contract.
- The -O2 machine code of the implementation contains no
  clz-class instruction (checked programmatically in the test and
  independently by grep).
- -O0, -O2, ASan, and UBSan builds produce identical results.
- The seed `0x123456789ABCDEF0` is fixed, so the random pass is
  reproducible.
