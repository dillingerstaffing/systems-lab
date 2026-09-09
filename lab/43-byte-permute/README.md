# lab/43-byte-permute

`permute64` (permute.h) applies one fixed non-identity byte permutation
to a 64-bit word: output byte i comes from input byte
`(2,5,0,7,1,6,3,4)[i]`. It is written as pure shift/mask composition:
each output byte is extracted with `(x >> (8*src)) & 0xFF` and placed
with `<< (8*dst)` and `|`, with every source index hardcoded. There are
no lookup tables in the implementation. `inv_permute64` applies the
exact inverse permutation `(2,4,0,6,7,1,5,3)`, derived by reading the
forward table backwards (inv[P[i]] = i), built the same way.

Verification (test_permute.c): differential test against an independent
table-driven byte-loop oracle for all 65,536 exhaustive 16-bit inputs
and 1,000,000 fixed-seed splitmix64 64-bit values (1,065,536 cases total,
0 mismatches); the round-trip invariant in both directions
(`inv(permute(x)) == x` and `permute(inv(x)) == x`) held on all
1,065,536 cases with 0 failures; an FNV-1a checksum over all results
came out identical (f75c74855e39dfd5) across -O0, -O2, and ASan+UBSan
builds; measured throughput at -O2 was 4.424 ns per permute/inverse call.
Compiles clean under -Wall -Wextra -Werror with zero warnings, and the
ASan+UBSan run reported nothing. See PROOF.md for the full build and run logs.
