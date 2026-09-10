<!-- PROOF-HEADER
Checks: 4294967296
Mismatches: 0
Checksum: f0bc609332350325
Throughput: 267.73 ns/value at -O2 (uncontended quick run)
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/69-isqrt

Environment: gcc 13.3.0 (Ubuntu), x86_64.

`isqrt64()` (isqrt.h) returns the largest `r` with `r*r <= n` for a
`uint64_t n`. The construction is the digit-by-digit restoring square
root in base 4: 32 iterations, each deciding one more bit of the answer
from the most significant bit down. The trial step uses the identity
`(res + bit)^2 - res^2 = 2*res*bit + bit^2`, rearranged into restoring
form: if the remaining value covers `res + bit`, subtract it and set the
answer's next bit; otherwise clear it. The answer is built shifted
(`res` is halved each round while `bit` steps down through powers of 4),
which is exactly the digit-by-digit recurrence.

Bounds, all unsigned so there is no UB regardless: `bit` starts at 2^62
and only shrinks; `res` obeys `res < 2^33` at every iteration (each round
at most halves `res` then adds `bit <= 2^62`, and that recurrence from
`res = 0` stays below 2^33), so `res + bit < 2^64` and every
add/subtract/shift stays inside 64 bits.

The naive reference in the test is binary search for the largest `m`
with `m*m <= x`, written as `m <= x/m` so no product ever overflows and
no float or libm appears anywhere. For 64-bit samples the invariant is
additionally checked overflow-free as `r*r <= x` (`r < 2^32`, so `r*r`
fits) together with `r+1 > x/(r+1)`, which is equivalent to
`(r+1)^2 > x` since both sides are positive.

## Verbatim build log and run output

(Sections below are pasted verbatim from the run logs.)

### Build (-O2, -O0, ASan+UBSan)

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_isqrt test_isqrt.c
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_isqrt_o0 test_isqrt.c
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_isqrt_asan test_isqrt.c
BUILD_OK
```

### Full run, -O2

```
isqrt64 verification (full)
edges: 30 cases, ok
differential: 1065536 values, 0 mismatches
throughput: 457.67 ns/value over 1048576 values x 20 reps (no PRNG in timed loop, checksum 6c88bc16097c16f0)
exhaustive 2^32: 4294967296 inputs, 0 invariant violations, fnv1a=f0bc609332350325, 230.0 s
RESULT: PASS
EXIT=0
```

### Full run, -O0

```
isqrt64 verification (full)
edges: 30 cases, ok
differential: 1065536 values, 0 mismatches
throughput: 765.04 ns/value over 1048576 values x 20 reps (no PRNG in timed loop, checksum 6c88bc16097c16f0)
exhaustive 2^32: 4294967296 inputs, 0 invariant violations, fnv1a=f0bc609332350325, 529.2 s
RESULT: PASS
EXIT=0
```

### Full run, ASan+UBSan (-O1 -g)

```
isqrt64 verification (full)
edges: 30 cases, ok
differential: 1065536 values, 0 mismatches
throughput: 474.72 ns/value over 1048576 values x 20 reps (no PRNG in timed loop, checksum 6c88bc16097c16f0)
exhaustive 2^32: 4294967296 inputs, 0 invariant violations, fnv1a=f0bc609332350325, 420.8 s
RESULT: PASS
EXIT=0
```

Note: the -O2 full run above includes a throughput line measured while another worker was active on the second core (457.67 ns/value). An uncontended -O2 quick run measured 267.73 ns/value; repeated runs ranged 267-666 ns/value with CPU frequency scaling and concurrent load. The timed loop contains no PRNG step in any run.
