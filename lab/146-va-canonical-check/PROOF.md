<!-- PROOF-HEADER
Checks: 30000171
Mismatches: 0
Checksum: e08a833fc82c218c
Throughput: 2.138 ns/value at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/146-va-canonical-check

`canonical_va(va)` decides whether a 64-bit value is a canonical Sv39
virtual address. In Sv39 the usable address is 39 bits wide; the address
is canonical exactly when bits 63:39 are all equal to bit 38 (the sign
extension of the 39-bit address into the full register). The
implementation is one shift and two equality comparisons, no branches:

```c
int canonical_va(uint64_t va)
{
    uint64_t top = va >> 38;
    return (top == 0) || (top == 0x3FFFFFFULL);
}
```

The top 26 bits must be all zero (bit 38 clear) or all one (bit 38 set,
sign-extended). `0x3FFFFFFULL` is the 26-bit all-ones mask. gcc 13.3.0
at -O2 emits this as one shift, two compares, and setcc/OR, with no
conditional jumps (see disassembly below).

## What was verified (real runs, real numbers)

- Differential test of `canonical_va` against a structurally
  independent per-bit oracle, 10,000,057 cases total, 0 mismatches:
  - Oracle: bit 38 is the sign bit; each of bits 63..39 is compared
    against it individually (25 single-bit comparisons), written
    without any whole-field shift, so the oracle shares no structure
    with the implementation's `va >> 38` field comparison.
  - Directed rows (57 unique cases after dedupe): 0, 2^38 - 1, 2^38,
    2^38 + 1, 2^39 - 1, 2^39, each single high bit 39..63 set with
    bit 38 = 0 and with bit 38 = 1, all-ones, 0x0000007FFFFFFFFF,
    0xFFFFFF8000000000, 2^64 - 1.
  - 10,000,000 fixed-seed splitmix64 64-bit values, seed
    0x123456789ABCDEF0, increment 0x9E3779B97F4A7C15.
- Invariant 1, on every case: the verdict is decided by bits 63:38
  only. Replacing bits 37:0 with a random 38-bit value never changes
  the verdict. (Bit 38 is intentionally not randomized: it is the bit
  the top 25 must match, so it is part of the verdict's input. An
  earlier draft of this test randomized bits 38:0 and caught the
  mistake: case 0xffffff8000000000 flipped verdict when bit 38 changed,
  which is correct behavior, so the invariant was tightened to
  bits 37:0 and now holds.)
- Invariant 2, on every case: sign-extension round trip. For a
  canonical va, sign-extending the 39-bit address reproduces va
  exactly; for a non-canonical va it cannot:
  `(int64_t)va == ((int64_t)(va << 25)) >> 25`. The right shift is the
  arithmetic shift of the signed value (the cast precedes the shift).
- Checks counts all three assertions per case: 10,000,057 x 3 =
  30,000,171 checks, 0 mismatches.
- FNV-1a 64-bit checksum of the full result stream, identical across
  three builds: -O0, -O2, ASan+UBSan (-O1, -g): `e08a833fc82c218c`.
- Clean under `-std=c11 -Wall -Wextra -Werror`: zero warnings, all builds.
- ASan+UBSan (-O1, -g): zero reports, exit 0.
- Timing at -O2 over a pre-generated array of 10,000,000 values,
  best of 5 reps: 2.138 ns/value. Honest caveat: the measured path
  includes the array load, the call, and one XOR accumulate per value,
  so this is the per-iteration cost of the loop as written, not of the
  function body alone.
- Disassembly (gcc 13.3.0, -O2, x86_64) of `canonical_va` contains 0
  conditional jumps:

```
canonical_va:
    endbr64
    shr    $0x26,%rdi          ; va >> 38
    sete   %al                ; top == 0
    cmp    $0x3ffffff,%rdi     ; top == all-ones
    sete   %dl
    or     %edx,%eax
    movzbl %al,%eax
    ret
```

## Genuine build log

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_canonical_va test_canonical_va.c canonical_va.c

$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_canonical_va_o0 test_canonical_va.c canonical_va.c

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_canonical_va_asan test_canonical_va.c canonical_va.c

$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_canonical_va bench_canonical_va.c canonical_va.c
```

## Genuine run output (-O2)

```
HEADER-BEGIN
Checks: 30000171
Mismatches: 0
Checksum: e08a833fc82c218c
HEADER-END
directed_cases: 57
verdict: PASS
```

The -O0 and ASan+UBSan runs printed the identical block
(Checks: 30000171, Mismatches: 0, Checksum: e08a833fc82c218c), both
exiting 0. Bench output:

```
acc: 0 (accumulate sink, prevents loop elimination)
best of 5 reps, 10000000 values: 2.138 ns/value
HEADER-LINE
Throughput: 2.138 ns/value at -O2
```

## Case mix, exactly as run

1. 57 unique directed values: 0, 2^38 - 1, 2^38, 2^38 + 1, 2^39 - 1,
   2^39, bits 39..63 set individually with bit 38 both 0 and 1,
   0x0000007FFFFFFFFF, 0xFFFFFF8000000000, 2^64 - 1.
2. 10,000,000 splitmix64 64-bit values, seed 0x123456789ABCDEF0.
3. On each of the 10,000,057 cases, in order: the oracle comparison,
   the low-38-bit replacement invariant, the sign-extension round-trip
   invariant. Each invariant consumes one further splitmix64 draw, so
   the full stream is deterministic and identical across builds.

## Limits of verification

This module models the Sv39 canonicality rule in user-space C on the
host machine; no privileged hardware was involved and no address was
actually translated by an MMU. What was checked is the decision
procedure: that the implementation agrees with the per-bit definition
of the rule on 10,000,057 values and that the two invariants hold on
every one of them. The oracle and the implementation are two different
encodings of the same rule (per-bit definition vs field comparison),
so a misreading of the rule itself would pass this test. The rule was
taken directly from the canonical-address definition (bits 63:39 must
equal bit 38 in Sv39); no emulation of page-table walks or fault
behavior is included.
