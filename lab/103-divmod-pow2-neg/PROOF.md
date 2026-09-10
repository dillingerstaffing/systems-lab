<!-- PROOF-HEADER
Checks: 4129461
Mismatches: 0
Checksum: b9bffe669bde5c90
Throughput: about 35 ns per divmod at -O2 (single timed pass over 4,128,768 values; varies run to run)
-->

# PROOF: lab/103-divmod-pow2-neg

Single-pass signed `divmod` by powers of two, with no `/` or `%` in
the implementation.

## What was built

- `divmod.h` / `divmod.c`: `divmod_pow2(x, k, &q, &r)`.
- `test_divmod.c`: differential harness, 4,129,461 checks per run.
- `Makefile`: `run` (-O2), `opt0` (-O0), `sanitize` (ASan+UBSan),
  `disasm`, `clean`.

## The identities under test

1. `q = (x + ((x >> 63) & (2^k - 1))) >> k` (sign-bias identity;
   arithmetic shift rounds toward negative infinity, the bias
   corrects toward zero to match C `/`).
2. `r = x - (q << k)` (mask identity; the shift is performed in
   unsigned arithmetic so a negative quotient is exact).

## Build log (verbatim, 2026-09-10)

```
$ make clean && make run && make sanitize && make opt0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_divmod test_divmod.c divmod.c
./test_divmod
checked=4129461 mismatches=0 fnv1a=b9bffe669bde5c90
timed_values=4128768 total_ns=146276824 ns_per_value=35.43 fnv1a=4ed683a11b9bd8af
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_divmod_asan test_divmod.c divmod.c
./test_divmod_asan
checked=4129461 mismatches=0 fnv1a=b9bffe669bde5c90
timed_values=4128768 total_ns=146025727 ns_per_value=35.37 fnv1a=4ed683a11b9bd8af
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_divmod_o0 test_divmod.c divmod.c
./test_divmod_o0
checked=4129461 mismatches=0 fnv1a=b9bffe669bde5c90
timed_values=4128768 total_ns=164988702 ns_per_value=39.96 fnv1a=4ed683a11b9bd8af
```

- 4,129,461 = 65,536 exhaustive 16-bit inputs x 63 widths (k = 1..63)
  plus 11 fixed 64-bit edge values x 63 widths. Zero mismatches
  against the `/` and `%` reference on every case.
- The FNV-1a checksum of the full (q, r) stream, `b9bffe669bde5c90`,
  is byte-identical across -O2, -O0, and ASan+UBSan builds. The
  timed-pass checksum `4ed683a11b9bd8af` also agrees across builds.
- Zero compiler warnings under `-Wall -Wextra -Werror`; zero
  ASan/UBSan reports on the final code.
- Throughput at -O2: about 35 ns per divmod on this VM (single
  timed pass over 4,128,768 values; varies run to run).

## Object inspection (verbatim, gcc 13.3, -O2)

```
0000000000000000 <divmod_pow2>:
   0:  f3 0f 1e fa          endbr64
   4:  49 89 d0             mov    %rdx,%r8
   7:  48 c7 c0 ff ff ff ff mov    $0xffffffffffffffff,%rax
   e:  48 89 ca             mov    %rcx,%rdx
  11:  89 f1                mov    %esi,%ecx
  13:  48 d3 e0             shl    %cl,%rax        ; 2^k
  16:  48 89 f9             mov    %rdi,%rcx
  19:  48 c1 f9 3f          sar    $0x3f,%rcx      ; sign extract
  1d:  48 f7 d0             not    %rax            ; 2^k - 1
  20:  48 21 c8             and    %rcx,%rax       ; bias (0 or 2^k - 1)
  23:  89 f1                mov    %esi,%ecx
  25:  48 01 f8             add    %rdi,%rax       ; x + bias
  28:  48 d3 f8             sar    %cl,%rax        ; q
  2b:  49 89 00             mov    %rax,(%r8)
  2e:  48 d3 e0             shl    %cl,%rax        ; q << k
  31:  48 29 c7             sub    %rax,%rdi       ; x - (q << k)
  34:  48 89 3a             mov    %rdi,(%rdx)
  37:  c3                   ret
```

A mnemonic scan of the whole `divmod.o` finds no `div`/`idiv`
instruction. The emitted code is exactly the two identities.

## Development note (what the sanitizer caught)

The first draft computed `r` as `x - (qq << k)` in the signed
domain. UBSan flagged `left shift of negative value` at runtime.
The shift was moved to unsigned arithmetic
(`(uint64_t)qq << k`, subtract mod 2^64, re-read as `int64_t`;
the true remainder is always representable, so this is exact).
The sanitizer build has been clean since, across all 4,129,461
checks.

## What was verified, and what was not

Verified: for every k = 1..63, every 16-bit input and 11 64-bit
edge values, the implementation's q and r equal C `/` and `%`,
the invariants `q * 2^k + r == x`, sign agreement, and `|r| < 2^k`
hold, and the object contains no division instruction.

Not verified: k = 0 and k = 64 (outside the contract by design),
and x = INT64_MIN (excluded by the contract because its magnitude
is not representable). Behavior there is intentionally unspecified.
