# lab/68-swar-sat-add

Saturating add of four packed 16-bit lanes in one 64-bit word, in
`swar_sat_add.h`. Each lane adds independently; a lane that overflows
clamps to `0xFFFF` instead of wrapping.

A plain 64-bit `+` cannot be used: a carry out of one lane would leak
into the next lane's sum. Even lanes (0, 2) and odd lanes (1, 3) are
therefore added as two independent pairs spaced 32 bits apart, so each
lane sum (at most `0x1FFFE`, 17 bits) has its carry-out land on a guard
bit that belongs to no lane. The guard bits are read back as overflow
flags, broadcast to full lane masks by multiplying with `0xFFFF`, and
select between the raw lane sum and `0xFFFF`, branchless. Odd lanes are
shifted down by 16 first so the top lane's carry is not lost off the
word, then shifted back into place.

Verified by `test_swar_sat_add.c`:

- 10 hand-checked known-answer vectors (see PROOF.md for derivations).
- Directed crosstalk sweep: every lane pair drawn from the boundary set
  `{0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF}`, all 36^4 =
  1,679,616 combinations across the four lanes, so every lane saturates
  next to every boundary value in its neighbors: 0 mismatches.
- Exhaustive differential over all 2^32 pairs of 16-bit lane values,
  each packed into all four lanes so every lane position sees every
  possible input pair (4,294,967,296 checks), against an independent
  per-lane scalar saturating reference: 0 mismatches.
- FNV-1a checksum over every result byte: identical across `-O0`,
  `-O2`, and ASan+UBSan builds (the slow builds run the directed sweep
  plus a fixed-seed 2^20 slice instead of the exhaustive loop).
- Throughput at `-O2`: about 4.6 ns per op (218 Mops/s); the timed loop
  includes word packing, the SWAR add, and a checksum fold so the work
  cannot be discarded.

Build: `make run`, `make opt0`, `make sanitize`. Full evidence in
PROOF.md.
