<!-- PROOF-HEADER
Checks: 4200728
Mismatches: 0
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/14-align-arith

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then `make run`. Output below is the genuine build log and the
genuine run output, captured verbatim.

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_align_o0 test_align.c align.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_align_o2 test_align.c align.c
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_align_asan test_align.c align.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
total_cases=4200728 mismatches=0
PASS
total_cases=4200728 mismatches=0
PASS
total_cases=4200728 mismatches=0
PASS
```

The three lines are the `-O0`, `-O2`, and ASan+UBSan binaries in
order, each exiting 0. ASan and UBSan report no issues, so the
shift/or propagation and the mask arithmetic contain no out-of-bounds
access and no undefined behavior.

## What the numbers mean

- 4,200,728 total checks across the four primitives, 0 mismatches
  against the division/loop-based references.
- Boundary sweep: 97 p values (2^k - 1, 2^k, 2^k + 1 for k = 0..31,
  plus UINT32_MAX) crossed with all 32 powers of two a = 1..2^31 for
  align_up and align_down (one check per primitive per pair: 6,208
  checks), and the same 97 p values as x for is_pow2 and round_up_pow2
  (one check per primitive: 194 checks).
- Two random loops of 1,048,576 fixed-seed random 32-bit values
  (xorshift32, seed 0x12345678), each value checked against both
  primitives of the loop: 2,097,152 checks for align_up/align_down and
  2,097,152 for is_pow2/round_up_pow2.
- 22 explicit edge cases: a = 1 (identity), x = 0, x = UINT32_MAX,
  round_up_pow2 overflow returning 0 (including rup2(2^31 + 1) and
  rup2(UINT32_MAX)).

The PRNG is fixed-seed, so the run is reproducible bit for bit:
`make run` on this checkout produces the exact output above.
