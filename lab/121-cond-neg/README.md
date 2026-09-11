# lab/121-cond-neg

Branchless conditional negation for 64-bit words:
`cond_neg(x, f) = (x ^ -f) + f` with the flag `f` in {0, 1}. The
implementation contains no comparison and no branch on the flag.

The mechanism is the two's-complement negation identity. In unsigned
64-bit arithmetic, `-f` is `0` when `f = 0` and `2^64 - 1` (all ones)
when `f = 1`. XOR with all ones flips every bit, which is bitwise NOT,
and adding 1 after bitwise NOT is exactly two's-complement negation
(`~x + 1 = -x mod 2^64`). So `f = 0` gives `(x ^ 0) + 0 = x` and `f = 1`
gives `(x ^ all-ones) + 1 = ~x + 1 = -x`. The same instruction sequence
handles both cases: gcc 13.3.0 at `-O2` compiles `cond_neg` to
`mov`/`neg`/`xor`/`add`/`ret`, and the programmatic jump scan of the
object code finds 0 jump instructions. Plain C11,
`-std=c11 -Wall -Wextra -Werror`, no intrinsics, no builtins, no
library math in the implementation.

Verified in `test_cond_neg.c`:

- All 65,536 16-bit `x` values x both flag values: 131,072 exhaustive
  checks, differential-tested against the plain if/else reference
  `f ? (0 - x) : x`.
- 10,000,000 fixed-seed `splitmix64` `(x, f)` pairs (seed
  `0x123456789ABCDEF0`, flag from the low bit), differential-tested
  against the same if/else reference.
- 0 mismatches across all 10,131,084 checks.
- Edge rows pinned in the log: `x = 0`, `x = 1`, `x = all-ones`,
  `x = 0x8000000000000000`, `x = 0xFFFF`, `x = 0x123456789ABCDEF0` at
  both flag values.
- FNV-1a checksum `1743a170968927e0` of all results, byte-identical
  across `-O0`, `-O2`, and ASan+UBSan; zero sanitizer reports; build is
  warning-free under `-Wall -Wextra -Werror`.
- Throughput 4.37 ns/value at `-O2` (best of 5 over 25M values).
