<!-- PROOF-HEADER
Checks: 26777408
Mismatches: 0
Checksum: 296ab7282627f4b7
Throughput: 2.813 ns/value at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/124-is-pow2

`is_pow2(x) = (x != 0) & ((x & (x - 1)) == 0)` for uint64, written branchless:
two setcc results combined with bitwise AND, no conditional jumps.

## Why the identity holds

Subtracting 1 from x borrows through every trailing zero bit and stops at
the lowest set bit, flipping exactly those bits. So `x - 1` differs from x
in precisely the lowest set bit and all bits below it. `x & (x - 1)` clears
the lowest set bit of x and leaves everything else unchanged. If x has
exactly one bit set (a power of two), the AND is zero; otherwise at least
one bit survives. For x = 0, unsigned subtraction wraps mod 2^64:
`(0 - 1)` is all-ones, `0 & all-ones` is 0, and `0 == 0` is true, so the
`(x != 0)` guard is what keeps 0 from reporting true. The guard and the
equality are both evaluated without branches.

## What was verified (real runs, real numbers)

- Differential test of `is_pow2` against a per-bit popcount-loop oracle
  (written by hand, no builtins), 26,777,408 cases total, 0 mismatches:
  - Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..63 (192 cases,
    including 2^63 and 2^63 + 1; 2^0 - 1 = 0 is also covered here).
  - Exhaustive 24-bit sweep: all x in [0, 2^24), 16,777,216 cases.
  - 10,000,000 fixed-seed splitmix64 64-bit values, seed
    0x123456789ABCDEF0, increment 0x9E3779B97F4A7C15.
  - The full 2^32 exhaustive loop was deliberately NOT run: 4.29B cases
    against a per-bit oracle could not complete in one run, so this module
    ships the largest fully-verified slice above instead. The slice above
    is exactly what was checked, nothing more.
- The x = 0 contract (must return 0) is asserted explicitly, and 0 is
  covered by both the directed edges and the exhaustive sweep.
- FNV-1a 64-bit checksum of the full result stream, identical across
  three builds: -O0, -O2, ASan+UBSan (-O1): `296ab7282627f4b7`.
- Clean under `-std=c11 -Wall -Wextra -Werror`: zero warnings, all builds.
- ASan+UBSan (-O1, -g): zero reports, exit 0.
- Timing at -O2 over a pre-generated array of 10,000,000 values,
  best of 5 reps: 2.813 ns/value. Honest caveat: the measured path
  includes the array load, the call, and one XOR accumulate per value,
  so this is the per-iteration cost of the loop as written, not of the
  function body alone.
- Disassembly (gcc 13.3.0, -O2, x86_64) of `is_pow2` contains 0
  conditional jumps. gcc emits the stated construction, nothing else:

```
is_pow2:
    lea    -0x1(%rdi),%rax   ; x - 1
    test   %rdi,%rax
    sete   %dl               ; (x & (x - 1)) == 0
    xor    %eax,%eax
    test   %rdi,%rdi
    setne  %al               ; x != 0
    and    %edx,%eax         ; bitwise AND, branchless
    ret
```

## Build log and run output (real, from this machine)

```
$ make run
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_ispow2 test_ispow2.c ispow2.c
./test_ispow2
checks=26777408 mismatches=0 checksum=296ab7282627f4b7

$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_ispow2_o0 test_ispow2.c ispow2.c
./test_ispow2_o0
checks=26777408 mismatches=0 checksum=296ab7282627f4b7

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -o test_ispow2_asan test_ispow2.c ispow2.c
./test_ispow2_asan
checks=26777408 mismatches=0 checksum=296ab7282627f4b7

$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_ispow2 bench_ispow2.c ispow2.c
./bench_ispow2
rep 0: 78787659 ns total, 7.879 ns/value
rep 1: 83977414 ns total, 8.398 ns/value
rep 2: 53498096 ns total, 5.350 ns/value
rep 3: 39368622 ns total, 3.937 ns/value
rep 4: 28134772 ns total, 2.813 ns/value
best=2.813 ns/value
```

Verification environment: Host (module executed natively on the build
machine). Toolchain: gcc 13.3.0 on Ubuntu 24.04, x86_64.
