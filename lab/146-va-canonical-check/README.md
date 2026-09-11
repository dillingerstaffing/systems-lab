# lab/146-va-canonical-check

Sv39 canonical virtual address test: `canonical_va` reports whether a
64-bit value is a canonical Sv39 address, meaning bits 63:39 all equal
bit 38 (the top 26 bits are all-equal). One shift and two equality
comparisons, no branches, no intrinsics, no builtins.

Verified by differential test against a structurally independent
per-bit oracle over directed sign-boundary rows plus 10M fixed-seed
splitmix64 64-bit values, with a low-39-bit independence invariant and a
sign-extension round-trip invariant on every case, 0 mismatches. See
PROOF.md for the full verification record.
