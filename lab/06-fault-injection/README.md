# lab/06: fault injection over the SPSC ring buffer

A third thread flips bits in live ring-buffer slots mid-transfer, and the
test shows the per-frame integrity check catching every corruption.

Setup: the producer pushes 200,000 frames through the lab/01 SPSC ring
buffer (vendored here as `spsc.h`). Each frame is 16 payload words
followed by one CRC32 word covering the payload (CRC32 from lab/04,
vendored here as `crc32.c`/`crc32.h`). The consumer pops each frame,
recomputes the CRC32 over the popped payload, and compares it with the
popped CRC word. Word 0 of each payload carries the frame sequence
number, so loss or reorder is visible too.

Two modes, selected by `argv[1]`:

- `off`: no injector thread. All 200,000 frames (3,400,000 words) must
  transfer with valid CRCs and intact payloads. Exit 0 on success.
- `on`: the injector flips one bit in a payload word of every 32nd
  frame starting at frame 7 (6,250 injections). The injector
  rendezvouses with the consumer through a gate: the consumer parks
  before reading an injected frame, the injector flips a bit in one of
  that frame's slots, then releases the consumer. This is
  breakpoint-style injection (hold the reader, corrupt the in-flight
  word, resume): every flip is guaranteed to land in a slot the
  producer has written and the consumer has not yet read, and the
  gate's release/acquire atomics order the flip before the consumer's
  read, so there is no data race. Every injected frame must fail its
  CRC check, each detection is attributed to the exact slot and bit
  flipped, and no other frame may fail. Detections are printed and the
  program exits 1: that nonzero exit is the test result, the loud
  signal that the integrity check caught the fault. Exit 2 would mean
  the harness itself disagreed (a missed injection or a false
  positive).

Verification (`PROOF.md` holds the real build log and run output):

- fault-off: 200,000 frames verified clean, 0 errors, exit 0.
- fault-on: 6,250 corruptions injected, 6,250 detected, every detection
  attributed to the exact flipped word and bit, the other 193,750
  frames verified clean, 0 errors, exit 1 as designed.

Build and run:

```
make        # builds test_fault with -O2 -Wall -Wextra -Wpedantic -Werror
make run    # fault-off (must exit 0), then fault-on (must exit 1)
```
