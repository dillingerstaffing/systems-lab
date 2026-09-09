# lab/30-memmove-overlap

`memmove30.c` (`memmove30.h` declares it): `my_memmove`, a byte-level
memory copy that handles overlapping regions by choosing the copy
direction from the pointer geometry:

- `dest < src` (or disjoint): copy forward, byte 0 up. Every write
  lands at or below the address being read, so no source byte is
  overwritten before it is read.
- `dest > src`: copy backward, last byte down. Each write lands above
  the address currently being read, so every source byte is read before
  the copy reaches it. This is the only case a naive forward loop gets
  wrong: copying `"ABCDEFGH"` forward into a destination 4 bytes above
  the source smears the early bytes into the later ones and yields
  `"ABCDABCD"`.

That direction choice is the entire mechanism; the file is 20 lines.
No word-aligned fast path is included, by design, so the copy is
deliberately byte-at-a-time (see the measured gap below, stated
honestly).

Verified by `test_memmove.c` (fixed-seed xorshift64* fill, fully
reproducible), differential against libc `memmove` as the oracle:

- 8,385 exhaustive cases, 0 mismatches, identical checksums under
  `-O0`, `-O2`, and ASan+UBSan (see PROOF.md): every size 0..64
  crossed with every destination offset -64..+64 relative to the source
  anchor in a 256-byte arena. This covers disjoint copies, exact
  overlaps, single-byte overlaps, and every overlap depth in between.
- 2 directed cases: the backward overlap above, where a naive
  always-forward copy corrupts (`ABCDABCD`) and `my_memmove` is
  byte-exact; and a forward overlap where `my_memmove` matches libc.
- ASan+UBSan clean: the byte loops, pointer comparisons, and `size_t`
  arithmetic contain no undefined behavior. The differential `memcmp`
  compares two distinct arrays, so no overlap UB is involved.

Throughput, backward overlap, 4 MiB per copy, 200 copies, `-O2` (see
PROOF.md; numbers vary run to run with machine load): `my_memmove`
roughly 1.0-1.7 GiB/s versus libc `memmove` roughly 37-53 GiB/s. The
gap is expected and honest: libc uses vectorized wide loads and stores
while this copy moves one byte at a time, since the goal here was the
direction-selection mechanism, verified exhaustively, not the fast
path.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), `make run`. See `PROOF.md` for the genuine build log
and run output.
