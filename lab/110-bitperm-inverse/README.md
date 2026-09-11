# lab/110: an 8-bit bit permutation and its explicit inverse

`perm8` / `invperm8` in `bitperm.h` permute the bits of a byte by a fixed
mapping P = [2, 5, 0, 7, 1, 6, 3, 4] and un-permute them with the explicit
inverse Q = [2, 4, 0, 6, 7, 1, 5, 3]. `perm16` / `perm64` (and the inverse
forms) apply the byte permutation to every byte of a 16-bit or 64-bit
word.

## Idea

A bit permutation is just a renaming of positions: output bit P[i] takes
input bit i. Written out, that is one shift/mask identity per bit:

    out = OR over i of (((b >> i) & 1) << P[i])

The implementation hardcodes that sum, unrolled, so each output bit is
named by a literal constant. No table, no loop, no branch carries the
mapping at run time; the only data are the eight shift amounts.

The inverse is derived, not discovered: Q is the exact index reversal of
P, Q[j] = the i with P[i] = j. From P = [2,5,0,7,1,6,3,4]:

    P[0]=2 -> Q[2]=0,  P[1]=5 -> Q[5]=1,  P[2]=0 -> Q[0]=2,  P[3]=7 -> Q[7]=3,
    P[4]=1 -> Q[1]=4,  P[5]=6 -> Q[6]=5,  P[6]=3 -> Q[3]=6,  P[7]=4 -> Q[4]=7,

giving Q = [2,4,0,6,7,1,5,3]. Since P is a permutation of 0..7 (each
target appears exactly once, checked numerically), Q[P[i]] = i for every
i, so applying Q after P moves each bit back to where it started.
Round-trip success on all 256 byte values is therefore a proof that P is
bijective and Q is its true two-sided inverse, not just a test result.

## Verification

Differential test against naive per-bit loop references (`perm8_ref`,
`invperm8_ref`, `perm16_ref`, `invperm16_ref`, `perm64_ref`,
`invperm64_ref`, each a plain loop over the same P/Q tables):

- mapping self-check: 33 checks (each single-bit input lands on the bit
  named by P and Q; each of 0..7 appears exactly once as a P[i];
  Q[P[i]] = i and P[Q[i]] = i for all i)
- exhaustive: all 65,536 16-bit inputs, 8 checks each (perm vs loop
  reference, inv vs loop reference, perm8/invperm8 byte-level cross-check
  on both bytes, inv(perm(x)) == x, perm(inv(x)) == x)
- 1,000,000 fixed-seed splitmix64 64-bit values (seed 0x123456789ABCDEF0,
  permuted byte by byte), 4 checks each

Total 4,524,321 checks, 0 mismatches, across three builds (`-O0`, `-O2`,
ASan+UBSan), all producing the identical FNV-1a result checksum
`0xf62cbb51d7116999`. Throughput at `-O2`: 39.464 ns/value (best of 5,
20M 64-bit values, one perm64 application per value).

## Files

- `bitperm.h`      the permutation and its inverse, byte/16/64-bit forms
- `test_bitperm.c` mapping self-check, differential test, checksum
- `bench_bitperm.c` throughput benchmark
- `Makefile`       builds everything; `make check` runs all three test builds
- `PROOF.md`       mapping derivation, genuine build/test logs
