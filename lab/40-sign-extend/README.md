# lab/40-sign-extend

Sign extension of an arbitrary bit width `w` (1..64) to 64 bits via the
arithmetic-shift identity: shift left so bit `w-1` lands on bit 63, then
shift arithmetically right by the same amount, letting the sign bit
replicate across the upper positions. The shift amount `64 - w` stays in
`[0, 63]`, so no 64-bit shift is ever executed; `w == 0` is outside the
contract.

Measured: differential verification against an independent bit-test
reference over widths 1..63 x all 65536 16-bit inputs, width 64 x all
65536 16-bit inputs (identity check), plus directed edge cases (w=1 with
x=0,1; w=63 with the high bit set and clear; w=64 with INT64_MIN), for a
total of 4,194,311 checks with zero mismatches. An FNV-1a checksum folded
over every result is identical across -O0, -O2, and ASan+UBSan builds.
Throughput at -O2 is measured in the timed loop below (ns/value).
