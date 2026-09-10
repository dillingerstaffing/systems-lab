# lab/13-xor-swap

`xor_swap.c` (`xor_swap.h` declares it): in-place swap of two 64-bit
words, `void xor_swap(uint64_t *a, uint64_t *b)`, using only the three
xor group identities

    *a ^= *b;  *b ^= *a;  *a ^= *b;

with no temporary variable and no builtins.

How it works: xor is associative, `x ^ x == 0`, and `x ^ 0 == x` for
every 64-bit word. Tracing the old values `(a0, b0)` through the three
steps: `a1 = a0 ^ b0`, then `b1 = a1 ^ a0 = b0`, then
`a2 = a1 ^ b1 = a0`. The two identities do all the work; there is no
other mechanism.

Aliasing contract: when `a == b` (the same address), the three steps
collapse onto one word, `v ^= v` zeroes it, and the word stays 0. So
`xor_swap(&x, &x)` zeroes `x`; callers needing identity on aliasing
must guard before calling. Six dedicated rows (0, 1,
0x8000000000000000, all-ones, and two nonzero patterns) prove the
zeroing.

Verified by `test_xor_swap.c` (fixed-seed splitmix64, seed
`0x123456789ABCDEF0`, the same seed used by sibling labs, fully
reproducible):

- 4,295,967,296 differential checks, 0 mismatches: every ordered pair
  of 16-bit values held as `uint64_t` (all 2^32 of them), plus
  1,000,000 full-range random 64-bit pairs, each compared against a
  temp-variable reference swap (`ref_swap` in the test file), requiring
  `new_a == old_b` and `new_b == old_a` on every case.
- Identical FNV-1a checksum (`afa682b2de8a9162`) over every output word under
  `-O0`, `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2` (best of 5 runs, 100,000,000 pairs each, called through
a volatile function pointer so the call cannot be inlined, results
accumulated into a volatile sink so the loop cannot be optimized away):
2.689 ns/pair. Honest caveats: this is the cost of one full
indirect call (dispatch included) plus operand setup and sink
accumulation, not the bare three-xor body; the bare inlined sequence
is a few memory-operand xors. Rerun-to-rerun machine variance is on
the order of +-1 ns.

Disassembly check (gcc 13.3.0, `-O2`, x86_64, `objdump -d` of
`xor_swap.o`, full listing in `PROOF.md`): the three xor operations
survive as written. The compiler kept `xor (%rsi),%rax` for the first
two C statements and folded the third into `xor %rax,(%rdi)`,
carrying the intermediate in `%rax` instead of reloading; it did not
rewrite the idiom into a temp-register `mov` swap or `xchg`.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
