# lab/93-swar-zero-count

Count the zero bytes in a 64-bit word with parallel byte arithmetic:
`count_zero_bytes(x)` in `zero_count.h` returns 0..8.

The construction starts from the zero-byte detection identity
`(x - 0x0101010101010101) & ~x & 0x8080808080808080`, but that identity
as written is only an existence test: a borrow out of a zero low byte
propagates into the byte above, and a 0x01 byte above a zero byte gets a
phantom high bit (measured: 1001 overcounts in 10,065,536 inputs against
a per-byte reference). This module runs the subtraction in 16-bit lanes
with a 0x7F guard in each lane's high byte, so no borrow ever crosses a
lane boundary and each lane's top bit is set exactly when its byte was
zero. Even and odd bytes are handled in separate lane words and their
counts added.

Verified in `main.c` (see `PROOF.md` for the full log):

- Differential test against a naive per-byte reference: exhaustive all
  65,536 16-bit inputs plus 10,000,000 fixed-seed splitmix64 64-bit
  words (seed `0x123456789ABCDEF0`): 10,065,536 checked, 0 mismatches.
- FNV-1a checksum of all outputs identical across -O0, -O2, and
  ASan+UBSan builds (`ca180bdf75d1ddd9`).
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; zero sanitizer
  reports.
- -O2 disassembly of the hot path: straight-line integer arithmetic,
  no branch, no byte-by-byte loop.
- Throughput at -O2: 2.244 ns/value (best of 5 runs over 26,214,400
  precomputed words).

Build: `make test` compiles all three variants and runs them.
