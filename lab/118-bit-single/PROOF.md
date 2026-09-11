<!-- PROOF-HEADER
Checks: 4017408
Mismatches: 0
Checksum: 0x4c11b8482936109c
Throughput: 1.332 ns/value
Environment: Host
Verdict: pass
-->
# PROOF.md: lab/118, single-bit set/clear/toggle/test from the mask identities

## Derivation

Start from the fundamental truth about unsigned integers: a 64-bit word
is the sum of b_i * 2^i for i = 0..63, with each bit b_i in {0, 1}. The
mask m = 1ULL << i has exactly bit i set and all other bits clear, so it
selects bit i and nothing else.

- set = x | m. For j = i, x_i | 1 = 1; for j != i, x_j | 0 = x_j. Bit i is
  forced to 1, every other bit is unchanged.
- clear = x & ~m. ~m has exactly bit i clear. For j = i, x_i & 0 = 0; for
  j != i, x_j & 1 = x_j. Bit i is forced to 0, the rest unchanged.
- toggle = x ^ m. For j = i, x_i ^ 1 = !x_i; for j != i, x_j ^ 0 = x_j.
- test = (x >> i) & 1. Unsigned right shift by i is floor(x / 2^i), which
  moves bit i to position 0; & 1 reads exactly that bit.

Each identity follows directly from the definition of the bitwise
operators on the bit decomposition, no analogy involved.

## Contract and edge cases

- i = 63: bit 63 is the most significant bit of the word, and
  1ULL << 63 = 0x8000000000000000 is well defined because the shift count
  63 is below the word width of 64. The test pins this edge explicitly:
  bset(0, 63), bclr(all-ones, 63), btg(MSB, 63), and btst on the MSB are
  checked against their known truths before the sweep starts.
- i = 0: bset(0, 0) = 1, btst(0, 0) = 0; covered by the directed sweep.
- No shift by 64 can ever execute, in three layers. First, a constant
  shift by 64 cannot compile: -Wall enables -Wshift-count-overflow, and
  -Werror turns it into a build failure. Second, every operation asserts
  i >= 0 && i < 64, and the asserts are live in all three test builds (no
  NDEBUG anywhere in the Makefile). Third, the ASan+UBSan build arms the
  UBSan shift-exponent check with -fno-sanitize-recover=all, so any shift
  by 64 or more at runtime would abort the run; the run completed 4,017,408
  checks with exit 0 and zero sanitizer reports.
- Reference independence: the differential references rebuild each answer
  one bit at a time with plain right shifts of counts 0..63 and never use
  the (1ULL << i) mask identities, so a systematic error in the identities
  cannot hide behind a matching reference.

## Genuine build log

`make clean && make` printed:

    gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_O0 test_bitop.c
    gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_O2 test_bitop.c
    gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
        -fno-sanitize-recover=all -o test_asan test_bitop.c
    gcc -std=c11 -Wall -Wextra -Werror -O2 -o bench_O2 bench_bitop.c
    gcc -std=c11 -Wall -Wextra -Werror -O2 -c -o impl_check.o impl_check.c
    objdump -d impl_check.o > impl_check.dis
    --- shift-by-64-or-more immediates in implementation object ---
    OK: no shift by 64 or more in bset/bclr/btg/btst codegen

Zero warnings under -std=c11 -Wall -Wextra -Werror on all four binaries.

## Genuine test output

./test_O0, ./test_O2, ./test_asan each printed (exit 0, no sanitizer
reports), byte-identical across all three:

    edge i=63 pinned: mask=0x8000000000000000
    directed done: 4352 (word,position) pairs, 17408 checks
    random done: 1000000 (word,position) pairs, splitmix64 seed 0x123456789ABCDEF0
    total checks          : 4017408
    mismatches            : 0
    FNV-1a of all results : 0x4c11b8482936109c

The FNV-1a checksum 0x4c11b8482936109c is byte-identical across the -O0,
-O2, and ASan+UBSan builds. The directed sweep is 68 values (0, all-ones,
all 64 single-bit words, both alternating patterns) crossed with all 64
positions, 4 operations per pair (17,408 checks), plus 1,000,000 random
(word, position) pairs at 4 operations each (4,000,000 checks).

## Genuine benchmark output (./bench_O2, -O2, 50M operations per rep)

    rep 0: 70773200 ns total, 1.415 ns/value, sink=0x1c04d08e70cd141c
    rep 1: 78164406 ns total, 1.563 ns/value, sink=0x3809a11ce19a2838
    rep 2: 66593389 ns total, 1.332 ns/value, sink=0x540e71ab52673c54
    rep 3: 73967731 ns total, 1.479 ns/value, sink=0x70134239c3345070
    rep 4: 66730024 ns total, 1.335 ns/value, sink=0x8c1812c83401648c
    best: 1.332 ns/value over 50000000 operations

Method: 1,048,576 (word, position) pairs pre-generated from fixed-seed
splitmix64 before any timing starts; each iteration applies one operation
(cycling set, clear, toggle, test), results checksummed into `sink` so the
loop cannot be optimized away, CLOCK_MONOTONIC timing, best of 5. The sink
differs between reps because it accumulates across reps, which is expected.

## Disassembly check

`make disasm` compiles impl_check.c (a TU containing only the
implementation, so nothing else can pollute the result) at -O2 and greps
the object for any shift instruction with a constant count of 64 or more.
Result: none found. The variable shifts compile to count-in-%cl forms,
whose hardware count is taken mod 64 and can never exceed 63 anyway, and
the assert guards compiled in keep any out-of-range position from ever
reaching them in the test builds.
