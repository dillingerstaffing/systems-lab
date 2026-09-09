# lab/10: memcpy, rebuilt from alignment fundamentals

`memcpy10()`: a drop-in `memcpy` replacement built around one mechanism,
word-at-a-time copying gated by destination alignment.

## Mechanism

A multi-byte load or store is only safe when the pointer is aligned to
its size, so the copy proceeds in three phases:

1. **Head.** Copy single bytes until the destination pointer reaches a
   machine-word boundary (or the length runs out). Source and
   destination are byte pointers here, safe at any alignment.
2. **Body.** The destination is now word-aligned, so one word can be
   stored per iteration. The source may still be unaligned, and C has no
   portable unaligned multi-byte load, so each source word is assembled
   from single bytes with shifts and ORs. Every memory access in the
   body is then provably aligned (word store) or byte-sized (reads), on
   any architecture.
3. **Tail.** Fewer than one word remains; copy them as bytes.

No libc `memcpy`/`memmove` appears anywhere in the implementation;
libc `memcpy` is used only as the test oracle. Overlapping regions are
out of scope: like the standard `memcpy`, behavior is undefined when
source and destination overlap (use `memmove` for that).

## Files

- `memcpy10.h` / `memcpy10.c`: the implementation (about 40 lines).
- `test_memcpy.c`: differential test plus throughput benchmark.
- `Makefile`: builds `test_memcpy` and `test_memcpy_asan`
  (`-fsanitize=address,undefined`). Build flags: `-Wall -Wextra -Werror`.
- `PROOF.md`: the genuine build log and full run output.

## Verification (measured 2026-09-08, x86-64, gcc 13.3.0 -O2)

- **Differential:** 1,004,160 cases against libc `memcpy`, zero
  mismatches. Coverage: sizes 0..64 crossed with destination
  misalignment 0..7 and source misalignment 0..7 (exhaustive), plus
  1,000,000 random cases with sizes biased small (0..64), medium
  (up to 4096), and 65536, random misalignments 0..7 on both ends,
  deterministic PRNG seed so failures reproduce.
- **Sanitizers:** ASan + UBSan run over the same 1,004,160 cases:
  clean, no reports, exit 0.
- **Throughput:** 64 MiB copies x 20 reps: memcpy10 measured
  1338.8-1398.8 MiB/s across runs; libc `memcpy` measured
  20085.1-21635.4 MiB/s on the same machine. libc is roughly 15x
  faster (it uses wider vector moves); this implementation trades
  speed for a verifiable, fully portable construction.
- Return value equals `dest` in every case.
