# lab/81-wrapping-neg

`wrapneg_u64(x)`: two's-complement negation of a 64-bit word built
only from the `~x + 1` identity.  No unary minus anywhere in the
implementation; all arithmetic is unsigned 64-bit.

## The mechanism

Two's complement negation is the additive inverse in the ring of
integers modulo 2^64: the unique value `y` with `x + y == 0`
(mod 2^64).  Bitwise, `x + ~x` is all ones, which is
`2^64 - 1`; adding 1 wraps to zero, so `~x + 1` is that unique
inverse.  There is no signed arithmetic and therefore no
undefined case: every input, including `0x8000000000000000`,
is just 64 bits being inverted and incremented.

**Contract on the INT64_MIN edge.** `wrapneg(0x8000000000000000)`
returns `0x8000000000000000`: the value negates to itself.  Both
invariants hold there, since `0x8000000000000000 +
0x8000000000000000` wraps to zero and negating again returns the
same word.  (Signed unary minus on `INT64_MIN` is undefined in C,
so the differential check against unary minus skips this one
input; the edge is pinned as a dedicated test row instead, and
the invariants are checked on it like every other case.)

## Verification

Differential-tested against unary minus `(uint64_t)(-(int64_t)x)`
and against two invariants checked on every case:

- `(x + wrapneg(x)) == 0` (unsigned 64-bit, i.e. zero mod 2^64)
- `wrapneg(wrapneg(x)) == x`

Case sets:

- All 65,536 16-bit values, zero-extended to 64 bits,
  exhaustively (65,536 cases).
- 1,000,000 fixed-seed splitmix64 64-bit words
  (seed `0x123456789ABCDEF0`, the proof-engine convention),
  covering all 64-bit positions, bit flips, and carry chains
  through the `+ 1` (1,000,000 cases).
- `INT64_MIN` as a dedicated row with the explicit contract
  (1 case).

Result: 1,065,537 cases, 0 mismatches, 0 invariant violations.
The FNV-1a checksum of every output is `0x6f25fc9c8181575a`,
identical across the `-O0`, `-O2`, ASan, and UBSan builds.
`-std=c11 -Wall -Wextra -Werror` clean; zero sanitizer reports
across the full case set on all four builds.  Throughput at
`-O2`, best of 5 over 100,000,000 timed values with the
splitmix64 PRNG pre-generated into a 2^20-entry buffer and
excluded from the timed loop: 0.594 ns/value (1683.042 M
values/s); with the PRNG inside the timed loop the cost is
1.607 ns/value, a ceiling that includes the generator cost.
Exact logs are in PROOF.md.

No builtins, no intrinsics, no inline asm, no library calls in
the implementation.
