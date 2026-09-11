# lab/128-mul-lo32-nomul

Low 32 bits of a 64-bit product, computed without any multiply operator.

`mullo32(a, b)` splits each operand's low 32 bits into 16-bit halves and
evaluates the schoolbook expansion, keeping only the partial products
that can reach the low 32 bits of the result: `a0*b0` in full, plus the
low 16 bits of `a0*b1` and `a1*b0` shifted left by 16. The `a1*b1` term
carries a factor of 2^32 and vanishes modulo 2^32, and any partial
product involving bits 32..63 of either input does the same, which is
why the low 32 bits of the product depend only on the low 32 bits of
the operands. Each 16x16 partial product is exact in 32 bits and is
itself computed by shift-add (`mul16`): for each set bit `i` of one
half, add the other half shifted left by `i`.

The implementation (`mul32.c`) contains no multiply operator, no
intrinsics, no builtins, and no library calls; that is checked
programmatically by `make disasm`, which scans the `-O2` object code
for multiply-class instructions and fails the build if any appear.

Verified in `test_mullo32.c` by differential test against the native
`(uint32_t)(a * b)`: every 24-bit value of `a` paired with a
deterministic avalanche of itself (16,777,216 pairs), plus 1,000,000
fixed-seed `splitmix64` 64-bit pairs, with 0 mismatches. All outputs
fold into an FNV-1a checksum that is identical across the `-O0`, `-O2`,
and ASan+UBSan builds. Plain C11, `-std=c11 -Wall -Wextra -Werror`,
warning-free.
