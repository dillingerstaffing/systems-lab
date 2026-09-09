# lab/41-mask-below

A single function, `mask_below(n)` (mask.h), that returns the low-n-bit mask
for n in 0..64 using the 2^n - 1 identity: `(n == 64) ? ~0ULL : ((1ULL << n) - 1)`.
The n = 64 case returns `~0ULL` directly, so a 64-bit shift never executes.
n > 64 is outside the function's contract and is not handled.

Verification (test_mask.c): differential test against an independent per-bit
reference for every width n = 0..64, 1,000,000 fixed-seed splitmix64 cases
per width, for 65,000,000 total cases with 0 mismatches; the invariant
`(m + 1) & m == 0` held on all 65,000,000 cases; an FNV-1a checksum over all
results came out identical (9899f825c9bda325) across -O0, -O2, and
ASan+UBSan builds; measured throughput at -O2 was 1.578 ns per mask value.
Compiles clean under -Wall -Wextra -Werror with zero warnings, and the
ASan+UBSan run reported nothing. See PROOF.md for the full build and run logs.
