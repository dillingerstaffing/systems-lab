<!-- PROOF-HEADER
Checks: 1065547
Mismatches: 0
Checksum: 258f283508816c19
Throughput: 2.66 ns/value at -O2 (best of 5)
Environment: Host
Verdict: PASS
-->
# PROOF: lab/101-round-nearest-even

`q32_32_round_even(v)`: round a 64-bit Q32.32 fixed-point value to
the nearest integer with ties to even, from bit-manipulation
identities only. No float, no division in the implementation.

## Renumber note

The backlog item was named lab/94-round-nearest-even, but lab/94
is taken in this repo (lab/94-mulhi-signed), so the module was
built as lab/101, the lowest free lab number >= 100 at the time
(local listing and `gh api repos/dillingerstaffing/systems-lab/
contents/lab` agreed: 101, 102, 105, 109, 110, 111, 112, 113
free; 101 is the lowest).

## What was built

`round_rne.h`, `round_rne.c`, `test_round_rne.c`, `Makefile`,
`README.md`, `PROOF.md`. Plain C11,
`-std=c11 -Wall -Wextra -Werror`. No intrinsics, no builtins in
the implementation. `unsigned __int128` is used only in the test
oracle, never in `round_rne.c`.

## Derivation

Bits: [63:32] integer part, [31:0] fraction part.

    round_bit = fraction bit 31 (value exactly 1/2)
    sticky    = OR of fraction bits 0..30
    lsb       = integer bit 0
    round_up  = round_bit AND (sticky OR lsb)
    result    = integer_part + round_up

Each fraction bit k has value 2^(k-32). Case analysis:

- round_bit = 0: the fraction is below 1/2, because the top
  fraction bit alone is worth 1/2 and bits 0..30 sum to at most
  2^31 - 1 units of 2^-32, which is strictly less than 1/2.
  round_up = 0, result = integer_part. Correct.
- round_bit = 1, sticky = 1: the fraction is 1/2 plus at least
  one lower bit, so strictly above 1/2. round_up = 1, result =
  integer_part + 1. Correct.
- round_bit = 1, sticky = 0: the fraction is exactly 1/2. The
  candidates are integer_part and integer_part + 1; exactly one
  is even, and it is integer_part when lsb = 0, integer_part + 1
  when lsb = 1. So round_up = lsb picks the even neighbor.
  Correct.

The final addition is unsigned 32-bit: integer_part =
0xFFFFFFFF with round_up = 1 yields 0. The overflow contract is
pinned, not left ambiguous: 0xFFFFFFFF.FFFFFFFF rounds up and
wraps to 0, and dedicated rows assert this behavior.

## Build log (genuine output)

```
$ make clean && make test_round_rne test_round_rne_O0 test_round_rne_san test_round_rne_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -c -o round_rne_O2.o round_rne.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_round_rne test_round_rne.c round_rne_O2.o
gcc -std=c11 -Wall -Wextra -Werror -O0 -c -o round_rne_O0.o round_rne.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_round_rne_O0 test_round_rne.c round_rne_O0.o
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -c -o round_rne_san.o round_rne.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_round_rne_san test_round_rne.c round_rne_san.o
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_round_rne_bench test_round_rne.c round_rne_O2.o
```

Zero warnings on all eight compile/link steps (warnings are
errors).

## Run logs (genuine output)

`-O2`:
```
disasm check: no conditional jumps in round_rne_O2.o OK
checks: 1065547
mismatches: 0
checksum: 258f283508816c19
```

`-O0`:
```
disasm check: no conditional jumps in round_rne_O2.o OK
checks: 1065547
mismatches: 0
checksum: 258f283508816c19
```

ASan+UBSan:
```
disasm check: no conditional jumps in round_rne_O2.o OK
checks: 1065547
mismatches: 0
checksum: 258f283508816c19
```

All three builds agree: 1,065,537 checks (65,536 exhaustive low
16 fraction bits + 11 directed rows + 1,000,000 fixed-seed
splitmix64 values, seed 0x123456789ABCDEF0), 0 mismatches, one
checksum. The checksum `258f283508816c19` is the FNV-1a 64-bit
hash of the 32-bit result stream.

Independent manual check on the -O2 object file: the full
disassembly of `q32_32_round_even` is

```
0000000000000000 <q32_32_round_even>:
   0:	endbr64
   4:	mov    %rdi,%rdx
   7:	xor    %eax,%eax
   9:	shr    $0x20,%rdx
   d:	test   $0x7fffffff,%edi
  13:	mov    %edx,%ecx
  15:	setne  %al
  18:	shr    $0x1f,%edi
  1b:	and    $0x1,%ecx
  1e:	or     %ecx,%eax
  20:	and    %edi,%eax
  22:	add    %edx,%eax
  24:	ret
```

`mov`/`xor`/`shr`/`test`/`setne`/`and`/`or`/`add`/`ret` only;
no conditional jump anywhere, which is the branchless claim made
machine-checkable. The test performs the same scan
programmatically on `round_rne_O2.o` (any mnemonic starting with
'j' fails) and passed on every build.

## Throughput (genuine output, -O2)

```
$ ./test_round_rne_bench
rep 0: 2.690 ns/value (sink 2252652425303672)
rep 1: 2.668 ns/value (sink 2252652425303672)
rep 2: 2.856 ns/value (sink 2252652425303672)
rep 3: 2.694 ns/value (sink 2252652425303672)
rep 4: 2.655 ns/value (sink 2252652425303672)
best: 2.655 ns/value
```

Best of 5 over a 1M-value buffer: 2.66 ns/value. The PRNG fill
runs once before timing starts and is not inside the timed loop;
the sink is identical across reps, so every rep rounds the same
1,048,576 values.

## What was verified

- The derivation above proves the single expression correct for
  every input; the differential test re-checked 1,065,537 inputs
  against the independent exact oracle (2*frac vs 2^32 in
  unsigned __int128, an arithmetic route sharing no bit
  identities with the implementation) with 0 mismatches.
- The exhaustive pass covers all 65,536 values with only the
  low 16 fraction bits varying; the 11 directed rows cover every
  midpoint parity combination, sticky set/not set, carry into
  the integer part, and the wraparound contract
  (0xFFFFFFFF.FFFFFFFF -> 0, 0xFFFFFFFF.80000000 -> 0,
  0xFFFFFFFF.80000001 -> 0, 0xFFFFFFFE.80000000 -> 0xFFFFFFFE).
- The -O2 machine code of the implementation contains no
  conditional jump (checked programmatically in the test and
  independently by reading the disassembly).
- -O0, -O2, ASan, and UBSan builds produce identical results.
- The seed `0x123456789ABCDEF0` is fixed, so the random pass is
  reproducible.

## Scope note

This module is distinct from the shipped lab/28-tie-to-even-half.
Lab/28 rounds only `x / 2` (a single halving, where the remainder
bit is the whole decision). This module is the general Q32.32
case: a full 32-bit fraction field, the midpoint as an interior
value (fraction bit 31), and the sticky OR over the lower 30
bits, which lab/28 never needs. Different mechanism, different
contract.
