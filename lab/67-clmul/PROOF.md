# PROOF.md: lab/67-clmul

Environment: gcc 13.3.0 (Ubuntu), x86_64.

`clmul64()` (clmul.h) computes the carryless product of two `uint64_t`
values as a 128-bit `{hi, lo}` result. Each operand is read as a
polynomial over GF(2) (bit i is the coefficient of x^i); result bit j
is the XOR over all i of `bit_i(a) & bit_{j-i}(b)`, so no carries ever
propagate. The implementation uses the shift-xor identity
`clmul(a, b) = XOR over set bits i of a of (b << i)`, branch-free via a
`0 - bit` mask: no tables, no intrinsics, no library calls.

## Hand derivations for the known-answer vectors

Over GF(2), squaring is linear on the exponents (the "freshman's
dream"): `(sum c_i x^i)^2 = sum c_i x^{2i}`, because every cross term
`2*c_i*c_j*x^{i+j}` has coefficient 2 = 0.

- `zero`: no set bits in `a`, XOR of nothing is 0.
- `identity`: only bit 0 of `a` set, result is `b << 0 = b`.
- `2*3`: `x * (x + 1) = x^2 + x`, bits 2 and 1 set, `0b110 = 6`.
- `ff*ff`: `(x^7+...+1)^2 = x^14+x^12+...+x^0`, bits 0,2,...,14 set,
  `0x5555`.
- `32ones^2`: `(sum_{i=0..31} x^i)^2 = sum_{i=0..31} x^{2i}`, bits
  0,2,...,62 set, `0x5555555555555555`, `hi = 0`.
- `x63*x63`: `x^63 * x^63 = x^126`, bit 126 is `hi` bit 62,
  `{0x4000000000000000, 0}`.
- `64ones^2`: coefficient of `x^j` is the number of pairs
  `(i, k)` with `i + k = j` mod 2, which is odd exactly when `j` is
  even (count is `j+1` for `j <= 63`, `127-j` for `j >= 64`); bits
  0,2,...,126 set, `{0x5555555555555555, 0x5555555555555555}`.
- `nibble^2`: `(sum_{i=0..15} x^{4i})^2 = sum_{i=0..15} x^{8i}`, bits
  0,8,...,120 set, i.e. bits 0,8,...,56 in each half,
  `{0x0101010101010101, 0x0101010101010101}`.

Correction note: the first draft of the test expected
`{0, 0x0101010101010101}` for `nibble^2`, forgetting the operand is
64 bits wide so `x^{8i}` reaches bit 120, not bit 56. The
implementation's answer `{0x0101010101010101, 0x0101010101010101}`
matched the corrected derivation on re-check; the expectation was
fixed, not the code. The final runs below use the corrected vector.

## Build

All targets compile with `-std=c11 -Wall -Wextra -Werror`. Zero
warnings (any warning would fail the build).

```
$ make clean && make
rm -f test_clmul test_clmul_asan test_clmul_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_clmul test_clmul.c
build_exit=0
```

`-O0` and sanitizer builds (same flags as the Makefile's `opt0` and
`sanitize` targets):

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_clmul_o0 test_clmul.c
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_clmul_asan test_clmul.c
```

## Runs

PRNG: splitmix64 with fixed seed `0x123456789ABCDEF0` (increment
`0x9E3779B97F4A7C15`); the random stream is identical in every build.

### -O2 (`./test_clmul`)

```
vector zero           got={0000000000000000,0000000000000000} expect={0000000000000000,0000000000000000} OK
vector identity       got={0000000000000000,deadbeefcafebabe} expect={0000000000000000,deadbeefcafebabe} OK
vector 2*3            got={0000000000000000,0000000000000006} expect={0000000000000000,0000000000000006} OK
vector ff*ff          got={0000000000000000,0000000000005555} expect={0000000000000000,0000000000005555} OK
vector 32ones^2       got={0000000000000000,5555555555555555} expect={0000000000000000,5555555555555555} OK
vector x63*x63        got={4000000000000000,0000000000000000} expect={4000000000000000,0000000000000000} OK
vector 64ones^2       got={5555555555555555,5555555555555555} expect={5555555555555555,5555555555555555} OK
vector nibble^2       got={0101010101010101,0101010101010101} expect={0101010101010101,0101010101010101} OK
exhaustive16: 4294967296 pairs, mismatches=0
random64: 1000000 pairs, ref_mismatches=0 conv_mismatches=0
invariants: distributivity_violations=0 commutativity_violations=0 identity_violations=0 annihilator_violations=0
fnv1a=be0ed5a4db5813fc
throughput: 2000000 clmul64 in 0.205 s = 102.53 ns/value (sink=3c9d7ae531361603)
RESULT: PASS
run_exit=0
```

### -O0 (`./test_clmul_o0`)

```
vector zero           got={0000000000000000,0000000000000000} expect={0000000000000000,0000000000000000} OK
vector identity       got={0000000000000000,deadbeefcafebabe} expect={0000000000000000,deadbeefcafebabe} OK
vector 2*3            got={0000000000000000,0000000000000006} expect={0000000000000000,0000000000000006} OK
vector ff*ff          got={0000000000000000,0000000000005555} expect={0000000000000000,0000000000005555} OK
vector 32ones^2       got={0000000000000000,5555555555555555} expect={0000000000000000,5555555555555555} OK
vector x63*x63        got={4000000000000000,0000000000000000} expect={4000000000000000,0000000000000000} OK
vector 64ones^2       got={5555555555555555,5555555555555555} expect={5555555555555555,5555555555555555} OK
vector nibble^2       got={0101010101010101,0101010101010101} expect={0101010101010101,0101010101010101} OK
exhaustive16: 4294967296 pairs, mismatches=0
random64: 1000000 pairs, ref_mismatches=0 conv_mismatches=0
invariants: distributivity_violations=0 commutativity_violations=0 identity_violations=0 annihilator_violations=0
fnv1a=be0ed5a4db5813fc
throughput: 2000000 clmul64 in 0.316 s = 157.95 ns/value (sink=3c9d7ae531361603)
RESULT: PASS
opt0_exit=0
```

### ASan+UBSan (`./test_clmul_asan`)

```
vector zero           got={0000000000000000,0000000000000000} expect={0000000000000000,0000000000000000} OK
vector identity       got={0000000000000000,deadbeefcafebabe} expect={0000000000000000,deadbeefcafebabe} OK
vector 2*3            got={0000000000000000,0000000000000006} expect={0000000000000000,0000000000000006} OK
vector ff*ff          got={0000000000000000,0000000000005555} expect={0000000000000000,0000000000005555} OK
vector 32ones^2       got={0000000000000000,5555555555555555} expect={0000000000000000,5555555555555555} OK
vector x63*x63        got={4000000000000000,0000000000000000} expect={4000000000000000,0000000000000000} OK
vector 64ones^2       got={5555555555555555,5555555555555555} expect={5555555555555555,5555555555555555} OK
vector nibble^2       got={0101010101010101,0101010101010101} expect={0101010101010101,0101010101010101} OK
exhaustive16: 4294967296 pairs, mismatches=0
random64: 1000000 pairs, ref_mismatches=0 conv_mismatches=0
invariants: distributivity_violations=0 commutativity_violations=0 identity_violations=0 annihilator_violations=0
fnv1a=be0ed5a4db5813fc
throughput: 2000000 clmul64 in 0.294 s = 146.82 ns/value (sink=3c9d7ae531361603)
RESULT: PASS
san_exit=0
```

The sanitizer run printed no reports: zero ASan/UBSan findings.

## What was measured

- Known-answer vectors, 8/8 pass, each derived by hand from the GF(2)
  polynomial definition above (the freshman's-dream squaring rule and
  direct coefficient counting).
- Differential: 4,295,967,296 checks, 0 mismatches:
  - all 2^32 pairs of 16-bit inputs: the shift-xor recurrence
    (iterate a's bits, shift b) vs an independent recurrence
    (iterate b's bits, shift a), fused into one unrolled pass;
  - 1,000,000 fixed-seed 64-bit pairs: `clmul64()` vs the
    independent recurrence, 0 mismatches; the first 20,000 of those
    pairs additionally vs a literal per-output-bit convolution
    reference (the polynomial-product definition written out
    directly), 0 mismatches.
- Invariants on all 1,000,000 random triples `(a, b, c)`:
  distributivity `clmul(a, b^c) == clmul(a,b) ^ clmul(a,c)` holds
  because the product is GF(2)-linear in each argument; also
  commutativity, `clmul(a,1) == a`, `clmul(a,0) == 0`. 0 violations.
- FNV-1a checksum over the byte stream of every result:
  `be0ed5a4db5813fc`, identical across -O0, -O2, and ASan+UBSan
  builds.
- Throughput at -O2: 102.53 ns/value over 2,000,000 calls. The input
  arrays are prefilled with splitmix64 output before timing starts,
  so the timed region is pure `clmul64` calls; results are xor-folded
  into a printed sink so the loop is not dead code. Single run, one
  x86_64 host; the number is not portable.

## Limits of verification

- The 2^32 exhaustive pass exercises the recurrence shape on 16-bit
  inputs, not the 64-bit datapath directly. The 64-bit datapath is
  covered by the 1,000,000 random pairs against two independent
  references and by vectors that drive bit 63 (`x63*x63` lands on
  result bit 126; `64ones^2` fills all 128 result bits).
- The two shift-xor recurrences share the same underlying identity; a
  shared misunderstanding of the GF(2) product would survive their
  differential. The convolution reference (the definition, written
  with no shift-xor structure) and the hand-derived vectors are the
  independent anchors against that.
- Distributivity is a theorem of GF(2)-linearity, not an empirical
  discovery; the 1,000,000-case check guards against coding slips in
  the loop, nothing deeper.
- The `nibble^2` expectation was wrong in the first draft (see
  correction note); the final vectors above are the corrected set,
  and the FNV checksum covers the corrected run.
