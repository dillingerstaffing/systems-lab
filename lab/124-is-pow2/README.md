# lab/124-is-pow2

Branchless power-of-two test for 64-bit words:
`is_pow2(x) = (x != 0) & ((x & (x - 1)) == 0)`.

Verified by differential test against a hand-written per-bit popcount
oracle over directed 2^k / 2^k +/- 1 edges for k = 0..63, an exhaustive
24-bit sweep, and 10M fixed-seed splitmix64 64-bit values, 0 mismatches.
-O2 disassembly shows 0 conditional jumps. See PROOF.md for the full
verification record.
