<!-- PROOF-HEADER
Checks: 4524321
Mismatches: 0
Checksum: f62cbb51d7116999
Throughput: 39.464 ns/value at -O2 (best of 5)
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/110, 8-bit bit permutation and its explicit inverse

## Mapping derivation

P sends input bit i to output bit P[i], P = [2,5,0,7,1,6,3,4].
Each of 0..7 appears exactly once as a P[i], so P is a permutation.

The inverse Q is the exact index reversal of P: Q[j] is the i with
P[i] = j.

    P[0]=2 -> Q[2]=0    P[4]=1 -> Q[1]=4
    P[1]=5 -> Q[5]=1    P[5]=6 -> Q[6]=5
    P[2]=0 -> Q[0]=2    P[6]=3 -> Q[3]=6
    P[3]=7 -> Q[7]=3    P[7]=4 -> Q[4]=7

Q = [2,4,0,6,7,1,5,3]. Check: Q[P[0]]=Q[2]=0, Q[P[1]]=Q[5]=1,
Q[P[2]]=Q[0]=2, Q[P[3]]=Q[7]=3, Q[P[4]]=Q[1]=4, Q[P[5]]=Q[6]=5,
Q[P[6]]=Q[3]=6, Q[P[7]]=Q[4]=7. Hence Q[P[i]] = i for every i, and
symmetrically P[Q[i]] = i, so Q is the two-sided inverse of P.

Both directions are implemented as the unrolled shift/mask sum
out = OR_i (((b >> i) & 1) << P[i]) (resp. Q[i]), naming every output
bit with a literal constant.

## Build log

First build attempt (failed under -Werror, kept as evidence the gate
bites):

```
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_O0 test_bitperm.c
test_bitperm.c: In function 'main':
test_bitperm.c:115:53: error: expected expression before ')' token
  115 |             printf("MISMATCH " fmt "\n", __VA_ARGS__); \
      |                                                     ^
test_bitperm.c: At top level:
test_bitperm.c:37:16: error: 'invperm8_ref' defined but not used [-Werror=unused-function]
   37 | static uint8_t invperm8_ref(uint8_t b)
      |                ^~~~~~~~~~~~
test_bitperm.c:27:16: error: 'perm8_ref' defined but not used [-Werror=unused-function]
   27 | static uint8_t perm8_ref(uint8_t b)
      |                ^~~~~~~~~
cc1: all warnings being treated as errors
make: *** [Makefile:9: test_O0] Error 1
```

Fixed: the CHECK macro now uses `##__VA_ARGS__` so the no-argument form
compiles, and the 8-bit loop references are used for per-byte
differential cross-checks in the 16-bit loop. Clean rebuild:

```
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_O0 test_bitperm.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_O2 test_bitperm.c
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
    -fno-sanitize-recover=all -o test_asan test_bitperm.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o bench_O2 bench_bitperm.c
```

## Test runs

```
===== ./test_O0 =====
mapping self-check done: 33 checks
exhaustive 16-bit done: 65536 values
random 64-bit done: 1000000 values, splitmix64 seed 0x123456789ABCDEF0
total checks            : 4524321
mismatches              : 0
FNV-1a of all results   : 0xf62cbb51d7116999
exit=0
===== ./test_O2 =====
mapping self-check done: 33 checks
exhaustive 16-bit done: 65536 values
random 64-bit done: 1000000 values, splitmix64 seed 0x123456789ABCDEF0
total checks            : 4524321
mismatches              : 0
FNV-1a of all results   : 0xf62cbb51d7116999
exit=0
===== ./test_asan =====
mapping self-check done: 33 checks
exhaustive 16-bit done: 65536 values
random 64-bit done: 1000000 values, splitmix64 seed 0x123456789ABCDEF0
total checks            : 4524321
mismatches              : 0
FNV-1a of all results   : 0xf62cbb51d7116999
exit=0
```

## Benchmark

```
rep 0: 1074786693 ns total, 53.739 ns/value, sink=0x56a0815ccdef3380
rep 1: 818659629 ns total, 40.933 ns/value, sink=0xad4102b99bde6700
rep 2: 918685805 ns total, 45.934 ns/value, sink=0x03e1841669cd9a80
rep 3: 864128771 ns total, 43.206 ns/value, sink=0x5a82057337bcce00
rep 4: 789271717 ns total, 39.464 ns/value, sink=0xb12286d005ac0180
best: 39.464 ns/value over 20000000 values
```

## What was verified

- 33 mapping checks: every single-bit input lands on the bit named by P
  and by Q; each of 0..7 occurs exactly once among the P[i]; Q[P[i]] = i
  and P[Q[i]] = i for all i.
- All 65,536 16-bit inputs, 8 assertions each: perm vs the naive per-bit
  loop reference, inv vs its loop reference, per-byte cross-checks of both
  bytes against the 8-bit loop references, inv(perm(x)) == x and
  perm(inv(x)) == x.
- 1,000,000 fixed-seed splitmix64 64-bit values (seed 0x123456789ABCDEF0),
  permuted byte by byte, 4 assertions each: perm vs 64-bit loop reference,
  inv vs 64-bit loop reference, inv(perm(x)) == x, perm(inv(x)) == x.
- 4,524,321 total checks, 0 mismatches. FNV-1a checksum
  0xf62cbb51d7116999 identical across -O0, -O2, and ASan+UBSan.
- Environment: Host (gcc on the build machine). No QEMU involvement; the
  module is pure integer bit manipulation with no platform-specific
  behavior.
