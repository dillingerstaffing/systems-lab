# PROOF.md: lab/125, unsigned division by 7 from the shift-add series

## Derivation

Start from the binary expansion of the reciprocal of 7. The geometric
series with ratio 2^-3:

    S = 2^-3 + 2^-6 + 2^-9 + ...

sums to (2^-3) / (1 - 2^-3) = (1/8) / (7/8) = 1/7. In binary, 2^-3k is a
single 1 bit at the 3k-th place after the point, so 1/7 = 0.001001001...
repeating with period "001". Multiplying through by x:

    x / 7 = sum_{k>=1} x * 2^{-3k}        (exact, real arithmetic)

For a 32-bit x, the integer part of x * 2^{-3k} is (x >> 3k) for 3k < 32,
and terms with 3k >= 32 shift to zero. Define the estimate

    q0 = sum_{k=1}^{10} (x >> 3k) = sum_{k=1}^{10} floor(x / 8^k).

Error bound. Write {t} for the fractional part of t:

    x/7 - q0 = sum_{k=1}^{10} {x / 8^k} + sum_{k=11}^{inf} x / 8^k
             < 10 * 1 + x * (1/8^11) * (1 / (1 - 1/8)).

Since x <= 2^32 - 1 and 8^11 = 2^33:

    tail < (2^32 / 2^33) * (8/7) = 4/7 < 0.58,

so 0 <= x/7 - q0 < 10.58. Because q0 <= x/7 always, the true quotient
floor(x/7) satisfies 0 <= floor(x/7) - q0 <= 10.

Correction. 7*q0 is formed without multiplication as q0 + (q0 << 1) +
(q0 << 2), which is exact. Since q0 <= x/7, we have 7*q0 <= x, so the
unsigned subtraction (x - t) cannot underflow, and t += 7 cannot overflow
(t <= x - 7 at each increment). The loop

    while ((x - t) >= 7) { q += 1; t += 7; }

preserves t = 7q and terminates with 7q <= x < 7q + 7, i.e. q = floor(x/7),
which is exactly what C unsigned `/` computes. At most 10 iterations.

No `*` or `/` operators appear in the implementation. Verified by
preprocessing div7.h (which strips comments) and grepping the lines
belonging to this file:

    gcc -E div7.h | awk '/^# /{f=($0 ~ /div7\.h"/)} f' | grep -v '^#' | grep '[*/]'
    -> no output (only system-header typedefs such as `void *` contain `*`,
       and those are outside this file's code)

## Contract and edge cases

- x = 0: every shift term is 0, q0 = 0, t = 0, loop skipped, returns 0.
- 1 <= x <= 6: all shift terms are 0 (x >> 3 = 0), x - 0 < 7, returns 0.
- x = UINT32_MAX (4294967295): covered explicitly in the edge-case list;
  returns 613566756. Also hit by the random 32-bit stream.
- Inputs straddling 2^24 (exhaustive boundary): 0x00FFFFFE, 0x00FFFFFF,
  0x01000000, 0x01000001 checked explicitly.
- The estimate can never exceed the true quotient (each term is an integer
  part, so the sum is below the real sum x/7); the test asserts qs <= ref
  for every input and would report any violation. Zero hits observed.

## Genuine build log

`make clean && make` printed:

    gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_O0 test_div7.c
    gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_O2 test_div7.c
    gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
        -fno-sanitize-recover=all -o test_asan test_div7.c
    gcc -std=c11 -Wall -Wextra -Werror -O2 -o bench_O2 bench_div7.c
    gcc -std=c11 -Wall -Wextra -Werror -O2 -c -o impl_check.o impl_check.c
    objdump -d impl_check.o > impl_check.dis
    --- div/idiv mnemonics in implementation object ---
    OK: no div/idiv in udiv7_shiftadd codegen

Zero warnings under -std=c11 -Wall -Wextra -Werror on all four binaries.
One build issue found and fixed during development: bench_div7.c needed
`#define _POSIX_C_SOURCE 199309L` for clock_gettime under strict -std=c11.

## Genuine test output

./test_O0, ./test_O2, ./test_asan each printed (exit 0, no sanitizer
reports):

    edge cases done: 22 cases
    exhaustive 24-bit done: 16777216 values
    random 32-bit done: 10000000 values, splitmix64 seed 0x123456789ABCDEF0
    total checks          : 26777238
    mismatches            : 0
    estimate-above-ref hits: 0
    max correction steps  : 10
    FNV-1a of all results : 0x835b51585dc7f434

The FNV-1a checksum 0x835b51585dc7f434 is byte-identical across the -O0,
-O2, and ASan+UBSan builds. The observed maximum of 10 correction steps
matches the proven bound exactly.

## Genuine benchmark output (./bench_O2, -O2, 50M values per rep)

    rep 0: 900458805 ns total, 18.009 ns/value, sink=0xb173081b
    rep 1: 1464648936 ns total, 29.293 ns/value, sink=0x62e61036
    rep 2: 1313268419 ns total, 26.265 ns/value, sink=0x14591851
    rep 3: 830012688 ns total, 16.600 ns/value, sink=0xc5cc206c
    rep 4: 844909613 ns total, 16.898 ns/value, sink=0x773f2887
    best: 16.600 ns/value over 50000000 values

Method: deterministic stream x += 0x9E3779B9 per step (one add of loop
overhead), results checksummed into `sink` so the loop cannot be
optimized away, CLOCK_MONOTONIC timing, best of 5.

## Disassembly check

`make disasm` compiles impl_check.c (a TU containing only the
implementation, so the test's `/` reference cannot pollute the result) at
-O2 and greps the object for div/idiv mnemonics. Result: none found. The
static function was inlined into main; the whole object is div-free.
