# lab/11-ieee754: software binary32 add and multiply

`f32.c` implements IEEE-754 single-precision addition and multiplication
directly from the bit layout (sign, exponent, fraction), using integer
arithmetic only. No float operations appear in the implementation.

Method: the exact sum (or product) is formed as an integer significand in
`unsigned __int128`, wide enough to hold every bit, then rounded once,
round-to-nearest-even, in `f32_round_pack`. A single rounding step keeps
the subnormal range free of double rounding. Subnormals, infinities,
signed zeros, and NaNs are handled explicitly; NaN results propagate the
first NaN operand with the quiet bit set.

`test_f32.c` differential-tests `f32_add`/`f32_mul` against the hardware
FPU: 38 directed edge pairs (infinities, signed zeros, subnormal
boundaries, SNaN/QNaN, rounding ties) plus 1,000,000 random operand pairs
with biased edge classes (subnormals, infinities, NaNs with random
payloads and signaling bits, tiny/huge exponents).

Comparison is bit-exact, except that NaN payloads are
implementation-defined: when both results are NaN, NaN-ness and the sign
bit must match (payload-only differences are counted separately).

See PROOF.md for the genuine build log, run output, and measured numbers.
