# lab/44-bcd-add

Two functions, `bcd_add` and `bcd_sub` (bcd.h), operating on packed BCD
bytes (two decimal digits per byte, high nibble = tens, low nibble =
units). Add: each nibble sums its two digits plus the incoming carry,
and when the sum exceeds 9, adding 6 turns bit 4 into the outgoing carry
while the low nibble becomes sum - 10. Sub: each nibble forms digit
minus digit minus incoming borrow, and when the result is negative,
subtracting 6 makes the low nibble equal difference + 10 with the
borrow propagated.

Contract: operands must be valid packed BCD (each nibble 0..9); bytes
with a nibble above 9 are outside the contract and untested. Out-of-range
results wrap with a flag: 0x99+0x01 returns 0x00 with carry 1,
0x00-0x01 returns 0x99 with borrow 1.

Verification (test_bcd.c): differential test against an independent
nibble-unpack reference over the entire defined domain, all 100x100
valid pairs for add and all 100x100 for sub, 20,000 cases, 0 mismatches;
carry-out fired on exactly 4,950 adds and borrow-out on exactly 4,950
subs, matching the hand-computed counts; FNV-1a checksum ff64fdf0ec2142e5
over all results identical across -O0, -O2, and ASan+UBSan builds.
Compiles clean under -Wall -Wextra -Werror with zero warnings, and the
ASan+UBSan run reported nothing. Measured at -O2: 3.296 ns per add/sub
op over 40,000,000 ops. See PROOF.md for the full build and run logs.
