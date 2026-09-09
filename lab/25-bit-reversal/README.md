# lab/25-bit-reversal

`bit_reverse.c` (`bit_reverse.h` declares it): 64-bit bit reversal,
`uint64_t bit_reverse(uint64_t x)`, bit k of the input moves to
position 63 - k of the output. The implementation is six group-swap
steps with doubling group sizes: swap adjacent bits (mask
`0x5555555555555555`), 2-bit groups (`0x3333333333333333`), nibbles
within bytes (`0x0F0F0F0F0F0F0F0F`), bytes within 16-bit halves
(`0x00FF00FF00FF00FF`), 16-bit halves within 32-bit halves
(`0x0000FFFF0000FFFF`), and the two 32-bit halves. Every step is
`((x >> m) & mask) | ((x & mask) << m)` on unsigned values only; no
compiler builtin and no dedicated instruction appears in the source,
and the byte reordering in the last three steps is written out as
shifts and ORs.

Verified by `test_bit_reverse.c` (fixed-seed splitmix64, seed
`0x123456789ABCDEF0`, fully reproducible):

- 4 explicit boundaries: 0, `UINT64_MAX`, `0x5555555555555555`,
  `0xAAAAAAAAAAAAAAAA`; plus all 64 single-bit values `2^k`.
- Exhaustive: every 16-bit value embedded at four 16-bit lanes of
  the 64-bit word (262,144 cases).
- 10,000,000 random 64-bit values.

Every case is checked two ways: differential against a naive
reference that moves bit k to position 63 - k one bit at a time, and
the involution invariant `bit_reverse(bit_reverse(x)) == x`.
10,262,212 total checks, 0 mismatches, 0 involution failures.
Identical FNV-1a checksum (`8367746376622110355`) under `-O0`,
`-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2` (100,000,000 timed values, each drawn from the PRNG
and XORed into a sink so the loop cannot be optimized away): 4.8-6.1
ns/value across two runs. Honest caveats: the figure includes the
PRNG step, so it is the cost of one generate-and-reverse case, not
one bare `bit_reverse` call, and machine variance on this box is
roughly +-1 ns. The 100M-value sink total was 6543017044187020749.

Disassembly check (gcc 13.3.0, `-O2`, x86_64): the compiler kept
the 1/2/4-bit group swaps as shift/and/or sequences and
strength-reduced the byte-reordering tail (steps 8, 16, 32) into a
single `bswap %rax`, which is the exact byte-reversal those three
steps compute. No hidden rescue of the within-byte logic, which has
no x86-64 instruction and stays as written.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
