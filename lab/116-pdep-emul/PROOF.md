<!-- PROOF-HEADER
Checks: 1002060
Mismatches: 0
Checksum: 5780cdf6a1013eb2
Throughput: 76.59 ns/value at -O2, best of 5
Environment: Host
Verdict: PASS
-->
# PROOF: lab/116-pdep-emul

`pdep64(src, mask)`: software parallel-bit-deposit for 64-bit words.
Scatters the low `popcount(mask)` bits of `src` into the set bit
positions of `mask`, lowest src bit into the lowest set mask position.

## What was built

`pdep.h`, `pdep.c`, `test_pdep.c`, `Makefile`, `README.md`, this file.
Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no builtins,
no library math in the implementation. The oracle in the test is an
independent naive per-bit loop (walks bit positions 0..63 in order), a
different algorithm from the implementation's while-loop over set bits;
the `__builtin_popcountll` conservation check lives in the test only.

## The construction

From `pdep.c`, the loop-scatter identity:

```c
uint64_t pdep64(uint64_t src, uint64_t mask)
{
    uint64_t result = 0;
    uint64_t m = mask;

    while (m != 0) {
        uint64_t lowbit = m & (~m + 1u);   /* lowest set bit of m */

        if ((src & 1u) != 0)
            result |= lowbit;

        src >>= 1;
        m ^= lowbit;
    }

    return result;
}
```

Each iteration deposits the lowest remaining `src` bit into the lowest
remaining set mask position, which is exactly the PDEP scatter. The
`-O2` disassembly of `pdep64` (gcc 13.3.0, x86-64) is the inner loop
compiled to a 15-instruction sequence with no branches in the deposit
path (`neg`/`and` for the lowest-bit isolation, `neg`/`and` for the
conditional deposit), terminating only on the loop-exit `jne`.

## Exactly what was verified

- 1,002,060 differential checks, 0 mismatches against the independent
  per-bit-loop oracle:
  - 1,000,000 fixed-seed `splitmix64` `(src, mask)` pairs
    (seed `0x123456789ABCDEF0`),
  - 2,048 pairs: all 256 8-bit mask values x 8 directed `src` values,
    including `mask=0`,
  - 12 edge-row pairs: `mask=0` and `mask=all-ones` against 6 `src`
    values, with `mask=all-ones` acting as the identity (result == src),
    printed in the log below.
- Conservation invariant on every pair:
  `popcount(result) == popcount(src & lowmask)` where `lowmask` has the
  low `popcount(mask)` bits set (the 64-bit edge written without a
  64-bit shift). Any dropped, duplicated, or invented bit breaks it.
- FNV-1a checksum `5780cdf6a1013eb2` of all results, identical across
  `-O0`, `-O2`, and ASan+UBSan; no sanitizer reports.
- Throughput 76.59 ns/value at `-O2` (best of 5 over 25M timed values).
- Build is warning-free under `-Wall -Wextra -Werror`.

## Build and run logs (actual output)

```
$ make run
./test_pdep
phase1 random: pairs=1000000, mismatches=0
phase2 exhaustive-8bit: pairs=2048, mismatches=0
phase3 edge rows (src, mask, result):
  src=0000000000000000 mask=0000000000000000 -> 0000000000000000
  src=0000000000000000 mask=ffffffffffffffff -> 0000000000000000
  src=deadbeefcafebabe mask=0000000000000000 -> 0000000000000000
  src=deadbeefcafebabe mask=ffffffffffffffff -> deadbeefcafebabe
  src=ffffffffffffffff mask=0000000000000000 -> 0000000000000000
  src=ffffffffffffffff mask=ffffffffffffffff -> ffffffffffffffff
  src=aaaaaaaaaaaaaaaa mask=0000000000000000 -> 0000000000000000
  src=aaaaaaaaaaaaaaaa mask=ffffffffffffffff -> aaaaaaaaaaaaaaaa
  src=5555555555555555 mask=0000000000000000 -> 0000000000000000
  src=5555555555555555 mask=ffffffffffffffff -> 5555555555555555
  src=8000000000000001 mask=0000000000000000 -> 0000000000000000
  src=8000000000000001 mask=ffffffffffffffff -> 8000000000000001
total pairs: 1002060
mismatches: 0
checksum: 5780cdf6a1013eb2
PASS
./test_pdep_O0
phase1 random: pairs=1000000, mismatches=0
phase2 exhaustive-8bit: pairs=2048, mismatches=0
phase3 edge rows (src, mask, result):
  src=0000000000000000 mask=0000000000000000 -> 0000000000000000
  src=0000000000000000 mask=ffffffffffffffff -> 0000000000000000
  src=deadbeefcafebabe mask=0000000000000000 -> 0000000000000000
  src=deadbeefcafebabe mask=ffffffffffffffff -> deadbeefcafebabe
  src=ffffffffffffffff mask=0000000000000000 -> 0000000000000000
  src=ffffffffffffffff mask=ffffffffffffffff -> ffffffffffffffff
  src=aaaaaaaaaaaaaaaa mask=0000000000000000 -> 0000000000000000
  src=aaaaaaaaaaaaaaaa mask=ffffffffffffffff -> aaaaaaaaaaaaaaaa
  src=5555555555555555 mask=0000000000000000 -> 0000000000000000
  src=5555555555555555 mask=ffffffffffffffff -> 5555555555555555
  src=8000000000000001 mask=0000000000000000 -> 0000000000000000
  src=8000000000000001 mask=ffffffffffffffff -> 8000000000000001
total pairs: 1002060
mismatches: 0
checksum: 5780cdf6a1013eb2
PASS
./test_pdep_san
phase1 random: pairs=1000000, mismatches=0
phase2 exhaustive-8bit: pairs=2048, mismatches=0
phase3 edge rows (src, mask, result):
  src=0000000000000000 mask=0000000000000000 -> 0000000000000000
  src=0000000000000000 mask=ffffffffffffffff -> 0000000000000000
  src=deadbeefcafebabe mask=0000000000000000 -> 0000000000000000
  src=deadbeefcafebabe mask=ffffffffffffffff -> deadbeefcafebabe
  src=ffffffffffffffff mask=0000000000000000 -> 0000000000000000
  src=ffffffffffffffff mask=ffffffffffffffff -> ffffffffffffffff
  src=aaaaaaaaaaaaaaaa mask=0000000000000000 -> 0000000000000000
  src=aaaaaaaaaaaaaaaa mask=ffffffffffffffff -> aaaaaaaaaaaaaaaa
  src=5555555555555555 mask=0000000000000000 -> 0000000000000000
  src=5555555555555555 mask=ffffffffffffffff -> 5555555555555555
  src=8000000000000001 mask=0000000000000000 -> 0000000000000000
  src=8000000000000001 mask=ffffffffffffffff -> 8000000000000001
total pairs: 1002060
mismatches: 0
checksum: 5780cdf6a1013eb2
PASS
```

```
$ make bench
./test_pdep_bench
bench: 76.59 ns/value (13.1 Mvalues/s over 25M timed values, best of 5)
```

Built and tested 2026-09-10. Toolchain: gcc 13.3.0 (Ubuntu) on
x86-64.
