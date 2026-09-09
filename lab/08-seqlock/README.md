# lab/08-seqlock

Sequence lock for single-writer / multi-reader shared state, in C11.
Header-only: `seqlock.h`. Tests: `test_seqlock.c`.

- `seqlock.h`
  - One 64-bit sequence counter plus a fixed payload (`counter`,
    8 words derived from it, and a 64-bit FNV-1a checksum stored by the
    writer). The writer stores seq odd (relaxed), writes the payload with
    plain stores, then stores seq even (release). Readers acquire-load seq,
    copy the payload with plain loads, acquire-load seq again, and retry if
    the first sample was odd or the two samples differ.
  - The release on the writer's even store publishes the payload: the
    payload stores are sequenced before it, and a reader's payload loads are
    sequenced after its first acquire load, so a matching even pair means
    the copy is exactly one generation the writer published. The writer is
    the only thread that ever stores seq and the counter only increases, so
    an overlapped write would force the second sample to differ; cache
    coherence makes the second load observe it.
  - `seqlock_init` publishes generation 0 as a real generation, so the
    initial state satisfies the same invariant as every writer-published
    state (an all-zero payload would fail the checksum check).
  - No locks anywhere; the write path is three atomic stores plus the
    payload writes, the read path is two atomic loads plus the copy.
- `test_seqlock.c`
  - Single-threaded round-trip: 1000 generations written and read back,
    every read succeeding on the first try with matching counter and
    consistent payload.
  - Detector check: a valid payload with one flipped bit in a word, in the
    counter, or in the stored checksum is rejected by `seqlock_verify`,
    while the untouched copy passes. The zero-torn-read claim below rests
    on this detector, independent of the sequence protocol.
  - Stress: 1 writer publishing generations as fast as it can, 4 readers
    each taking 2.5M successful snapshots. Every snapshot is
    checksum-verified (a failure means a torn read slipped past the sequence
    protocol) and checked for per-reader monotonicity of the generation
    counter. Retries are counted per reader. Captured run:
    10,000,000 reads, 1,036,084,339 retries (99.044% retry rate),
    0 torn, 0 monotonicity violations, writer published 43,514,894
    generations in 2.113 s (4.73 Mreads/s across readers).

Build: `make` (`-Wall -Wextra -Werror` clean), run: `make run`,
`make sanitize` (ASan+UBSan, also passes with 10M reads and 0 torn),
`make opt0` (also passes). Genuine build log and output are in `PROOF.md`.
No ThreadSanitizer target: TSan cannot see the sequence protocol, so the
plain payload copy guarded by it would be reported as a race (a false
positive); the ordering argument is documented in `seqlock.h`.
