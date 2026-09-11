<!-- PROOF-HEADER
Checks: 2304
Mismatches: 0
Checksum: 75d52de33bd90675
Throughput: 3.443 ns/value at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/146-pte-accessed-bit

`pte_a_check(flags, access)` decides whether a memory access against an
Sv39 leaf PTE faults, and what the A (accessed) bit is afterwards. The
rule it encodes comes from the RISC-V Privileged ISA Specification,
Sv39 addressing and memory protection:

- V = 0: the PTE is invalid, the access faults.
- W = 1 and R = 0: reserved combination, the PTE is invalid, the
  access faults.
- Otherwise the access is permitted when its own bit is set: read
  needs R, write needs W, execute needs X. A denied access faults.
- On a permitted access the hardware sets A = 1 (a 0 becomes 1, a 1
  stays 1). On a fault the translation fails and the PTE is untouched,
  so A keeps its input value.

The implementation extracts each flag bit once and evaluates the rules
in the order above: invalid entry, reserved combination, then the
permission of the access type. U, G, and D bits are present in the
input byte but never consulted: D is set by hardware on writes but
never gates an access, and U is a privilege-mode bit this check does
not model. The D transition on stores is outside this module's output;
only A is reported.

## What was verified (real runs, real numbers)

- Differential test of `pte_a_check` against a hand-transcribed spec
  table, over the FULL space: all 256 flag-byte values times the 3
  access types, 768 cases, 0 mismatches.
  - Oracle: a 3 x 16 table indexed by the low four flag bits
    (X W R V), one fault verdict per access type per row, each row
    derived by applying the invalid/reserved/permission rules in
    order. The table is a literal transcription in the test source
    with the row-by-row derivation in comments; it shares no code
    with the implementation's conditional chain. A on fault comes
    from the input byte, A on success is 1.
  - U, G, D vary across the 256 flag bytes and are inert in both the
    implementation and the oracle, so their irrelevance is
    exercised by the 768 cases, not assumed.
- Three assertions per case, 768 x 3 = 2304 checks, 0 mismatches:
  1. the fault verdict matches the spec table;
  2. the A value after the access matches the spec table;
  3. the A-bit transition invariant, checked against the invariant
     itself rather than the oracle: a permitted access yields A = 1,
     a fault yields A equal to the input A.
- FNV-1a 64-bit checksum of the full (fault, a_after) result stream,
  identical across three builds: -O0, -O2, ASan+UBSan (-O1, -g):
  `75d52de33bd90675`.
- Clean under `-std=c11 -Wall -Wextra -Werror`: zero warnings, all
  builds.
- ASan+UBSan (-O1, -g): zero reports, exit 0.
- Timing at -O2 over the 768-case table repeated 200,000 times
  (153,600,000 values), best of 5 reps: 3.443 ns/value. Honest caveat:
  the measured path includes the table index, the call, and one XOR
  accumulate per value, so this is the per-iteration cost of the loop
  as written, not of the function body alone.

## Genuine build log

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_a test_pte_a.c pte_a.c

$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_a_o0 test_pte_a.c pte_a.c

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_pte_a_asan test_pte_a.c pte_a.c

$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_pte_a bench_pte_a.c pte_a.c
```

## Genuine run output (-O2)

```
HEADER-BEGIN
Checks: 2304
Mismatches: 0
Checksum: 75d52de33bd90675
HEADER-END
cases: 768 (256 flag bytes x 3 access types, exhaustive)
verdict: PASS
```

The -O0 and ASan+UBSan runs printed the identical block
(Checks: 2304, Mismatches: 0, Checksum: 75d52de33bd90675), both
exiting 0. Bench output:

```
acc: 0 (accumulate sink, prevents loop elimination)
best of 5 reps, 153600000 values: 3.443 ns/value
HEADER-LINE
Throughput: 3.443 ns/value at -O2
```

## Case mix, exactly as run

1. Flag bytes 0x00 through 0xFF in order (256 values).
2. Access types read (0), write (1), execute (2) for each flag byte.
3. On each of the 768 cases, in order: the fault-verdict comparison,
   the a_after comparison, the A-transition invariant check.

## Limits of verification

This module models the Sv39 PTE permission and A-bit rule in
user-space C on the host machine; no privileged hardware was involved
and no address was actually translated by an MMU. The model assumes
the hardware-A-update behavior: a permitted access sets A in the PTE.
Implementations that instead raise a page fault when A is clear (the
spec permits either) are outside this module's scope. What was checked
is the decision procedure: that the implementation agrees with the
transcribed rule table on every one of the 768 possible inputs and
that the A-transition invariant holds on every one of them. The oracle
and the implementation are two different encodings of the same rule
reading (literal table vs conditional chain), so a misreading of the
spec's rules would pass this test. The rules were taken from the
privileged spec's Sv39 PTE description (V invalid, W-without-R
reserved, R/W/X permission bits, hardware A update on access, PTE
untouched on fault); no page-table walk, no privilege-mode check, and
no D-bit transition are modeled.
