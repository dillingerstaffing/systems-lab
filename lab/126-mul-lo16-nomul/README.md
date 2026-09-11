# lab/126: low 16 bits of a 16x16 multiply, without the multiply operator

`mullo16(uint16_t a, uint16_t b)` in `mul16.c` returns the low 16 bits of
the 32-bit product `a * b`, computed with shifts and adds only. No
multiply operator, no intrinsics, no builtins appear in the implementation.

## Idea

The full product is the sum over all pairs of bit positions of
a_i * b_j * 2^(i+j). Truncating the product to 16 bits discards every term
with i+j >= 16, so the low 16 bits of the product depend only on the low
16 bits of each operand. That makes the shift-add loop legitimate: for
each set bit j of b, the term contributed is a * 2^j, which is exactly
`a << j`. The loop adds each such term into a 32-bit accumulator and the
final truncation to 16 bits is precisely the identity above.

## Verification

- Differential test in `test_mullo16.c`: the oracle is
  `(uint16_t)((uint32_t)a * (uint32_t)b)`, the true low 16 bits of the
  product, over all 2^32 input pairs. The multiply operator appears only
  in the oracle, never in `mullo16`. All outputs fold into one FNV-1a
  checksum, identical across the -O0, -O2, and ASan+UBSan builds.
- Programmatic disassembly check (`make disasm`): the -O2 object is
  scanned for multiply-class instructions and the build fails if any
  appears.
- `bench_mullo16.c` times the -O2 loop over fresh splitmix64 operand
  pairs, best of 5 runs.

Measured numbers are in PROOF.md.
