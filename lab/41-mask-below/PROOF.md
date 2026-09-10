<!-- PROOF-HEADER
Checks: 65000000
Mismatches: 0
Checksum: 9899f825c9bda325
Throughput: 1.578 ns per mask value at -O2 (50,000,000 iteration loop, mask_below(i % 65), results xor-folded into printed accumulator)
Environment: Host
-->

# PROOF.md: lab/41-mask-below

Environment: gcc 13.3.0 (Ubuntu), x86_64.

## Build

First `make` attempt failed: `clock_gettime` was undeclared under
`-std=c11 -Werror` because no POSIX feature-test macro was defined.
Fixed by adding `#define _POSIX_C_SOURCE 200809L` to test_mask.c.
After that fix:

```
$ make clean && make
rm -f test_mask test_mask_asan test_mask_o0 disasm_mask.o
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_mask test_mask.c
build_exit=0
```

Zero warnings under -Wall -Wextra -Werror.

## Runs

### -O2 (`./test_mask`)

```
cases=65000000
mismatches=0
invariant_failures=0
fnv1a=9899f825c9bda325
verification_time=1.297 s
throughput_ns_per_value=1.578
throughput_acc=0001555555555555
run_exit=0
```

### -O0 (`make opt0`)

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_mask_o0 test_mask.c
./test_mask_o0
cases=65000000
mismatches=0
invariant_failures=0
fnv1a=9899f825c9bda325
verification_time=4.145 s
throughput_ns_per_value=5.533
throughput_acc=0001555555555555
opt0_exit=0
```

### ASan+UBSan (`make sanitize`)

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_mask_asan test_mask.c
./test_mask_asan
cases=65000000
mismatches=0
invariant_failures=0
fnv1a=9899f825c9bda325
verification_time=4.499 s
throughput_ns_per_value=2.151
throughput_acc=0001555555555555
sanitize_exit=0
```

The sanitizer run printed no reports: zero ASan/UBSan findings.

## What was measured

- 65,000,000 differential cases (65 widths x 1,000,000 fixed-seed
  splitmix64 draws): `mask_below(n)` vs the independent per-bit reference.
  0 mismatches.
- Invariant `(m + 1) & m == 0` on all 65,000,000 cases: 0 failures.
- FNV-1a checksum over all 65,000,000 results: `9899f825c9bda325`,
  identical across -O0, -O2, and ASan+UBSan builds.
- Throughput at -O2: 1.578 ns per mask value, measured over a 50,000,000
  iteration loop calling `mask_below(i % 65)` with the results xor-folded
  into an accumulator that is printed, so the calls are not optimized away.

## Disassembly of the primitive (`make disasm`, gcc -O2, x86_64)

```
disasm_mask.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <disasm_entry>:
   0:   f3 0f 1e fa          endbr64
   4:   48 c7 c0 ff ff ff ff  mov    $0xffffffffffffffff,%rax
   b:   83 ff 40             cmp    $0x40,%edi
   e:   74 0e                je     1e <disasm_entry+0x1e>
  10:   b8 01 00 00 00       mov    $0x1,%eax
  15:   89 f9                mov    %edi,%ecx
  17:   48 d3 e0             shl    %cl,%rax
  1a:   48 83 e8 01          sub    $0x1,%rax
  1e:   c3                   ret
```

The compiler kept the source-level branch: it pre-loads rax with all ones,
jumps over the shift path when n == 64, and otherwise does
`1 << n` then `- 1`. No 64-bit shift executes for n = 64, matching the
contract stated in mask.h.
