# lab/132-sv39-vpn-extract

Sv39 virtual-address field extraction: VPN[2], VPN[1], VPN[0] (9 bits
each) and the 12-bit page offset, from `vpn[i] = (va >> (12 + 9*i)) &
0x1FF` and `va & 0xFFF`.

Verified by differential test against a hand-written per-bit oracle
that builds each field by iterating bit indices (structurally
different from the shift/mask implementation), plus the invariant
recombine(extract(va)) == va on every case: directed 2^k / 2^k +/- 1
edges for k = 0..38, a canonical-address row with bit 38
sign-extended into bits 63:39, an exhaustive 24-bit sweep, and 10M
fixed-seed splitmix64 39-bit values, 0 mismatches. See PROOF.md for
the full verification record.
