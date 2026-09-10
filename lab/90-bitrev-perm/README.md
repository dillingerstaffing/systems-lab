# lab/90-bitrev-perm: bit-reversed index permutation of an 8-element array

`bitrev8_perm(in, out)` writes `out[bitrev(i)] = in[i]` for i = 0..7,
where `bitrev` is the 3-bit index reversal computed only from the
swap identities (exchange bit 0 with bit 2, keep bit 1 fixed); no
lookup table, no library call.

Verified by:
- differential test against an independently written reference that
  reverses each index with an explicit per-bit loop (different code
  path), over 1,000,000 randomized 8-element arrays of distinct values
- bijection check: with 8 distinct inputs, the output contains each
  input value exactly once, on every case
- involution check: applying the permutation twice restores the
  original ordering, on every case
