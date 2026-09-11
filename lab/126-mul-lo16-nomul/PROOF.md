<!-- PROOF-HEADER
Checks: 4294967296
Mismatches: 0
Checksum: 0x9fad8276d9322325
Throughput: 127.744 ns/value at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/126, low 16 bits of a 16x16 multiply, without the multiply operator

## What was built

`mullo16(uint16_t a, uint16_t b)` in `mul16.c` returns the low 16 bits of
the 32-bit product `a * b`. The implementation is one loop of shifts and
adds: for each set bit j of b it adds `a << j` into a 32-bit accumulator,
then truncates to 16 bits. This is correct because the full product is
the sum of a_i * b_j * 2^(i+j) over all bit pairs, and truncation to 16
bits discards every term with i+j >= 16, so the low 16 bits of the product
depend only on the low 16 bits of each operand. No multiply operator, no
intrinsics, no builtins appear in `mul16.c` or `mul16.h`.

The differential test in `test_mullo16.c` checks the identity
mullo16(a,b) == (uint16_t)((uint32_t)a * (uint32_t)b) for every pair
(a, b) with a and b in 0..65535, i.e. all 4,294,967,296 pairs. The
multiply operator appears only in the oracle, never in the implementation.
All outputs fold into one FNV-1a 64-bit checksum over the whole run.

`make disasm` compiles `mul16.c` at -O2 and programmatically greps the
objdump for multiply-class mnemonics (mul, imul, mulw, madd, msub,
smull, umull, fmul), failing the build if any appears.

## Build log (verbatim)

```
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_O0 test_mullo16.c mul16.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_O2 test_mullo16.c mul16.c
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
    -fno-sanitize-recover=all -o test_asan test_mullo16.c mul16.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o bench_O2 bench_mullo16.c mul16.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -c -o mul16_impl.o mul16.c
objdump -d mul16_impl.o > mul16_impl.dis
--- multiply-class instructions in mullo16 codegen (-O2) ---
OK: no multiply-class instruction in mullo16 codegen
```

Zero warnings under -Wall -Wextra -Werror. The -O2 disassembly of
mullo16 contains only test/je, mov, shl, add, shr, cmp, and ret.

## Run output (verbatim)

`./test_O2` (56s wall):

```
checks=4294967296 mismatches=0 checksum=0x9fad8276d9322325
```

`./test_O0` (113s wall):

```
checks=4294967296 mismatches=0 checksum=0x9fad8276d9322325
```

`./test_asan` (ASan+UBSan, 59s wall):

```
checks=4294967296 mismatches=0 checksum=0x9fad8276d9322325
```

The checksum 0x9fad8276d9322325 is identical across the -O0, -O2, and
ASan+UBSan builds.

`./bench_O2` (fixed seed splitmix64 operand pairs, 2^22 pairs per run,
best of 5; the timed loop honestly includes two splitmix64 draws, the
mullo16 call, and accumulation into a printed sink):

```
run 0: 138.281 ns/value
run 1: 127.744 ns/value
run 2: 146.248 ns/value
run 3: 139.983 ns/value
run 4: 138.574 ns/value
best-of-5: 127.744 ns/value
sink=687253238985
```

## Environment

- Host, gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1)
- x86-64, so the reference instruction class scanned in the disassembly
  check is mul/imul

## Scope of verification

Exhaustive differential proof: all 2^32 input pairs checked against the
plain-multiply oracle with 0 mismatches, plus the checksum agreement
across three differently-optimized builds and a programmatic
multiply-free codegen check. Nothing beyond the 16-bit input domain is
claimed.
