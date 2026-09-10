# lab/92-branchless-select

Branchless 2-way select over two `uint64` values: `bselect(sel, a, b)`
returns `b` when `sel == 1` and `a` when `sel == 0`, computed from the
mask identity with no comparison and no branch.

The implementation uses one unsigned fact: unsigned negation wraps
modulo 2^64 (C11 6.2.5p9), so it is total for every `uint64_t`. For
`sel` in `{0, 1}` that gives `mask = 0` or `mask = 2^64 - 1`
(all bits set), exactly the two complements needed by
`(a & ~mask) | (b & mask)` to select `a` or `b` bit-disjointly. No
signed arithmetic is performed anywhere, so no overflow or undefined
behavior is possible. The full hand derivation is in `PROOF.md`.

Contract, stated explicitly: `sel` must be 0 or 1. For any other
`sel` the result is the raw arithmetic blend (e.g. `sel = 2` gives
`mask = 0xFFFFFFFFFFFFFFFE`), not a defined selection. The test
program prints four such out-of-contract rows to make the boundary
explicit, and never differential-checks them.

Verified in `test_select.c`:

- Directed rows: 9 in-contract `(sel, a, b)` triples covering 0,
  `UINT64_MAX`, and mixed patterns, each checked against the ternary
  reference `sel ? b : a` and printed in the log.
- 8,589,934,592 exhaustive 16-bit `(sel, a, b)` triples (`sel` in
  `{0, 1}`, `a` and `b` over `[0, 65535]`), differential-checked
  against the ternary reference.
- 10,000,000 fixed-seed `splitmix64` random 64-bit triples (seed
  `0x123456789ABCDEF0`), each differential-checked against the same
  reference.
- 8,599,934,601 total checks, 0 mismatches. FNV-1a 64-bit checksum
  over the result stream: `47f9a66e9c481f08`, identical across `-O0`,
  `-O2`, and ASan+UBSan builds.
- The `-O2` disassembly of `bselect` contains no jump instruction of
  any kind (`endbr64`, `lea`, `neg`, `and`, `and`, `or`, `ret` only);
  the full excerpt is in `PROOF.md`.
- Measured at -O2: 2.68 ns/value (372.5 Mvalues/s over 25M timed
  values, best of 5).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
