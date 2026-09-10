# lab/64-median-of-three

`med3`: branchless median of three `int64_t` values, computed as a
3-element sorting network: three compare-swap steps, each built from a
comparison mask plus bitwise selection, with no conditional jumps.
Header-only: `median3.h`. Tests: `test_median3.c`.

- Comparison mask: signed order maps to unsigned order by flipping the
  sign bit; the unsigned `<` is decided on 32-bit halves, each
  comparison `-(((xh - yh) >> 63))` exact because the halves are under
  2^32 and can never wrap; combined as
  `hlo | (~(hlo | hhi) & llo)`. Exact on the whole `int64_t` range.
- Compare-swap selects inputs bitwise
  (`b ^ ((a ^ b) & m)` / `b ^ ((a ^ b) & ~m)`), so the inputs are never
  added or subtracted and no overflow is possible.
- Median via the network `(a,b)`, `(max_ab,c)`, `(min_ab, min_maxc)`:
  `med = max(min(a,b), min(max(a,b),c))`, the value left in the middle
  position after the three swaps.
- Contract: defined for every `int64_t` triple, no excluded inputs
  (INT64_MIN and INT64_MAX included).
- `test_median3.c`
  - Exhaustive over all 256^3 = 16,777,216 `int8_t` triples,
    sign-extended into `int64_t`, differential-checked against a
    comparison-based 3-sort reference.
  - Directed 9^3 = 729 edge triples over
    `{INT64_MIN, INT64_MIN+1, -2^60, -1, 0, 1, 2^60, INT64_MAX-1,
    INT64_MAX}`.
  - 5,000,000 fixed-seed splitmix64 random triples (seed
    `0x123456789ABCDEF0`), full `int64_t` range on every input.
  - Total: 21,777,945 differential checks, 0 mismatches, at `-O0`,
    `-O2`, and under ASan+UBSan; identical FNV-1a checksum of all
    outputs (`1512015986339184498`) across the three builds.
  - Disassembly check (`gcc -O2`, non-inline wrapper, `objdump -d`):
    the body is `sub`/`sar`/`shr`/`xor`/`and`/`or`/`not`/`mov` with
    zero conditional jumps in the object.
  - Benchmark: timed loop of 100,000,000 triples at `-O2`, reported in
    ns/triple in PROOF.md (10.379 ns/triple).
