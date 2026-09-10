<!-- PROOF-HEADER
Checks: 4304967524
Mismatches: 0
Checksum: 0xfdf651d8a57e9cd1
Throughput: 4.300 ns/pair at -O2
Environment: Host
-->
# PROOF.md, lab/57-saturating-sub-borrow

`sat_sub64(a, b)`: unsigned saturating subtraction, `a - b` when
`a >= b` else 0. Implementation: `sat_sub.c`, `sat_sub.h`. Tests:
`test_sat_sub.c`, `Makefile`.

## Why the construction is exact

For unsigned `a, b`, the subtraction `a - b` underflows exactly when
`a < b`: the borrow-out of `a - b` IS the comparison `a < b`, no
carry chain to reason about. `keep = -(uint64_t)(a >= b)` is
all-ones bits when `a >= b` and zero otherwise, so
`diff & keep` returns `a - b` on no borrow and 0 on borrow. All
operations are unsigned, so the wrap in `a - b` is defined by the C
standard. The branchless claim is verified in the generated assembly:

```
$ gcc -std=c11 -O2 -c sat_sub.c -o sat_sub.o && objdump -d sat_sub.o
0000000000000000 <sat_sub64>:
   0: f3 0f 1e fa        endbr64
   4: 48 89 f8           mov    %rdi,%rax
   7: 31 d2              xor    %edx,%edx
   9: 48 29 f0           sub    %rsi,%rax
   c: 48 39 f7           cmp    %rsi,%rdi
   f: 48 0f 42 c2        cmovb  %rdx,%rax
  13: c3                 ret
```

`sub; cmp; cmovb; ret`: the borrow flag drives a conditional move,
no conditional jumps anywhere in the function. At `-O0` the same
function compiles to `cmp; setae; neg; and`, also branch-free. No
`__int128` appears in the implementation.

## Oracle

The reference is `ref(a, b) = (a >= b) ? (uint64_t)((unsigned
__int128)a - b) : 0`. The subtraction runs in `unsigned __int128`,
an exact integer domain for 64-bit operands, so the oracle cannot
wrap. `__int128` appears ONLY in the oracle. Test phases:

- Phase 1: exhaustive over all 2^32 pairs of `uint16_t` operands
  (ran on the `-O2` build only).
- Phase 2: directed 144-pair cross sweep over
  `{0, 1, 2, 3, 2^32-1, 2^32, 0x5555555555555555, 0xAAAAAAAAAAAAAAAA,
  2^63-1, 2^63, 2^64-2, 2^64-1}`, plus an 84-pair boundary band
  (`b` in `{0, a-2, a-1, a, a+1, a+2, UINT64_MAX}`) around each edge
  value, covering the exact borrow point `b == a` at 64-bit scale.
- Phase 3: 10,000,000 fixed-seed splitmix64 random 64-bit pairs
  (seed 0x243F6A8885A308D3).

An FNV-1a checksum over the phase 2 and 3 outputs must be identical
across the `-O0`, `-O2`, and ASan+UBSan builds.

## Build log

```
$ make all
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_sat_sub_o0 test_sat_sub.c sat_sub.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_sat_sub_o2 test_sat_sub.c sat_sub.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_sat_sub_asan test_sat_sub.c sat_sub.c
```

Zero warnings on all three builds. The sanitizers cover the
implementation and the oracle, so any undefined behavior would fail
loudly.

## Measured results (gcc 13.3.0, x86-64, 2026-09-10 UTC)

```
$ ./test_sat_sub_o2 full
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 325.9 s (75.888 ns/pair)
phase 2 (directed edges + boundary band): 228 pairs, 0 mismatches
phase 3 (splitmix64, seed 0x243F6A8885A308D3): 10000000 pairs, 0 mismatches
total pairs: 4304967524, total mismatches: 0, FNV-1a: 0xfdf651d8a57e9cd1

$ ./test_sat_sub_o0
phase 2 (directed edges + boundary band): 228 pairs, 0 mismatches
phase 3 (splitmix64, seed 0x243F6A8885A308D3): 10000000 pairs, 0 mismatches
total pairs: 10000228, total mismatches: 0, FNV-1a: 0xfb3dd8407f4dd8d1

$ ./test_sat_sub_asan
phase 2 (directed edges + boundary band): 228 pairs, 0 mismatches
phase 3 (splitmix64, seed 0x243F6A8885A308D3): 10000000 pairs, 0 mismatches
total pairs: 10000228, total mismatches: 0, FNV-1a: 0xfb3dd8407f4dd8d1

$ ./test_sat_sub_o2
phase 2 (directed edges + boundary band): 228 pairs, 0 mismatches
phase 3 (splitmix64, seed 0x243F6A8885A308D3): 10000000 pairs, 0 mismatches
total pairs: 10000228, total mismatches: 0, FNV-1a: 0xfb3dd8407f4dd8d1

$ ./test_sat_sub_o2 bench
bench: 100000000 pairs, 0.430 s, 4.300 ns/pair (includes PRNG step, sink=1708381785801299162)
```

Cross-build checksum: 0xfb3dd8407f4dd8d1 on `-O0`, `-O2`, and
ASan+UBSan alike. The 75.888 ns/pair figure for phase 1 includes the
oracle subtraction, the mismatch check, and the FNV-1a accumulation
per pair; the 4.300 ns/pair bench figure includes one splitmix64
draw per operand, so it is a lower bound on the saturating
subtract itself.
