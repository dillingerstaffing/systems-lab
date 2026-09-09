# lab/50-strnlen-wordscan: bounded string length via word-at-a-time zero-byte scan

`my_strnlen(s, max)` (strnlen.h) returns the number of bytes from `s` up
to the first zero byte, or `max` bytes, whichever comes first.

Scanning order; every read stays inside `[s, s + max)`:

1. Byte head: compare single bytes while `s` is not word aligned and
   bytes remain.
2. Word body: while at least one whole word fits inside the bound, load
   the word with `memcpy` into a local (gcc emits a single aligned
   `movq` at -O2, verified in the generated assembly, with no `memcpy`
   call remaining) and test the zero-byte identity
   `((w - ONES) & ~w) & HIGH` with `ONES = 0x0101..01`,
   `HIGH = 0x8080..80`. A zero byte always sets its own 0x80 bit: the
   subtraction computes byte i as `0 - 1 - borrow_in` (borrow_in is 0 or
   1), which is 0xFF or 0xFE, both with the high bit set, and `~w` keeps
   it. A borrow from a lower zero byte can also set higher bytes' bits,
   so a nonzero result only means "re-scan this word byte by byte": the
   exact position comes from the byte scan, never from the word test.
   The word test is therefore zero exactly when the word holds no zero
   byte, which is all the loop needs.
3. Byte tail: the remaining (fewer than one word) bytes, one at a time.
   A word is never loaded unless it lies entirely within the bound, so
   no read ever passes `max`.

## Files

- `strnlen.h`: the implementation (one function).
- `test_strnlen.c`: 15 hand-checked vectors (empty, truncation by max,
  max=0, interior zero, head/tail-only length 7, one-word zero at byte
  0/3/7, max ending at and past the NUL), a differential test against
  libc `strnlen` over lengths 0..256 at every misalignment 0..7 with two
  content variants (terminator only, interior zero) and five max values,
  a guard-page over-read check, an FNV-1a checksum over all results, and
  a throughput benchmark.
- `Makefile`: `test-o0`, `test-o2`, `test-asan`, `test-ubsan`, `clean`.
- `PROOF.md`: genuine build log and measured numbers.

## Measured

- 15/15 vectors pass, including `max=0` returning 0 without touching
  memory and the one-word cases finding the zero at byte 0, 3, and 7.
- Differential: 20,480 checks against libc `strnlen`, 0 mismatches
  (lengths 0..256, misalignments 0..7, terminator-only and
  interior-zero content, max in {0, len, len+1, len/2, len-1}).
- Guard page: two `mmap`ed pages with the second `PROT_NONE`; 16
  terminator-at-boundary cases, 16 no-NUL cases with `max` ending
  exactly at the guard edge (every head/tail residue 0..15), and a
  `max=0` call with `s` inside the guard page itself: 33 cases, 0
  faults, 0 wrong answers, under ASan and UBSan with
  `-fno-sanitize-recover=all` (any report would be fatal).
- FNV-1a fingerprint 0xfe17a11752e1318c over all results, identical
  across -O0, -O2, ASan+UBSan, and UBSan builds.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` at -O0, -O2, and
  both sanitizer builds; zero warnings, zero sanitizer reports. (The
  UBSan build caught a real out-of-bounds write in the test's own
  vector scratch buffer during development; it was fixed before these
  runs.)
- Throughput at -O2: 20,000,000 calls in 0.632 s = 31.6 ns/value for a
  200-byte string (offsets 0..7 varied per call). The timed loop is
  `sink += my_strnlen(tbuf + (i % 8), 256)`; this is a ceiling on the
  true per-call cost, not a floor. See PROOF.md for the full build and
  run logs.
