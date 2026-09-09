# lab/34-branchless-minmax

`bmin32`/`bmax32`: branchless min and max over the full `int32_t`
domain, selected by the sign of the exact difference. Header-only:
`minmax.h`. Tests: `test_minmax.c`.

- `minmax.h`
  - The selection bit is bit 63 of the 64-bit difference
    `(uint64_t)(int64_t)a - (uint64_t)(int64_t)b`. The 64-bit
    intermediate holds the exact difference for every `int32_t` pair,
    so bit 63 is set exactly when `a < b`; the shift, the negation,
    and the final select are all unsigned/bitwise operations, so
    nothing overflows and no signed-overflow assumption is needed.
  - A 32-bit `mask = (a - b) >> 31` would be wrong near the
    `INT32_MIN`/`INT32_MAX` boundary: `INT32_MIN - INT32_MAX` wraps to
    `+1`, whose sign bit is clear, so the mask would select the wrong
    operand. The wide difference is what makes the mask exact over
    the whole domain. (Verified directly: the directed edge phase
    covers `INT32_MIN`/`INT32_MAX` pairs with 0 mismatches.)
- `test_minmax.c`
  - Exhaustive over all 2^16 x 2^16 = 4,294,967,296 `int16_t` pairs,
    sign-extended into `int32_t`, differential-checked against the
    ternary-operator reference plus the identities
    `min(a,b) <= max(a,b)` and "each result equals one of the inputs".
  - Directed 49-pair edge sweep over
    `{INT32_MIN, INT32_MIN+1, -1, 0, 1, INT32_MAX-1, INT32_MAX}`.
  - 10,000,000 fixed-seed xorshift32 full-32-bit random pairs.
  - Total: 4,304,967,345 differential checks, 0 mismatches.
  - Disassembly check (`gcc -O2`, non-inline wrapper, `objdump -d`):
    both functions compile to `sub`/`sar`/`xor`/`and`/`xor` with no
    conditional jump and no `cmov`; the select is pure mask
    arithmetic, so the instruction count and path are identical for
    every input pair.
  - Benchmark: timed loop of 200,000,000 pairs at `-O2` with an
    LCG-fed input stream and a checksum sink, reported in ns/pair in
    PROOF.md.
