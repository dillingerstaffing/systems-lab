# lab/67-clmul

Carryless (GF(2)) 64x64 -> 128-bit multiply in `clmul.h`. Each operand
is a polynomial over GF(2); the product bit j is the XOR over all i of
`bit_i(a) & bit_{j-i}(b)`. Implemented with the shift-xor identity
`clmul(a,b) = XOR over set bits i of a of (b << i)`: no carries, no
tables, no intrinsics.

Verified by `test_clmul.c`:

- 8 hand-checked known-answer vectors (see PROOF.md for derivations).
- Exhaustive differential over all 2^32 pairs of 16-bit inputs against
  an independent recurrence (iterates b's bits, shifts a): 0 mismatches.
- 1,000,000 fixed-seed 64-bit random pairs against the independent
  recurrence, plus the first 20,000 against a literal per-output-bit
  convolution reference: 0 mismatches.
- Invariants on all 1,000,000 random triples: distributivity
  `clmul(a, b^c) == clmul(a,b) ^ clmul(a,c)`, commutativity,
  `clmul(a,1) == a`, `clmul(a,0) == 0`: 0 violations.
- FNV-1a checksum over every result byte: identical across `-O0`,
  `-O2`, and ASan+UBSan builds.

Build: `make run`, `make opt0`, `make sanitize`. Full evidence in
PROOF.md.
