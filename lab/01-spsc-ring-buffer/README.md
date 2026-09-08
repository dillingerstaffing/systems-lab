# lab/01: wait-free SPSC ring buffer (C11)

A single-producer / single-consumer bounded queue in the style of Lamport's
wait-free queue, written in portable C11 (`<stdatomic.h>`).

Design points:

- No locks, no compare-and-swap retry loops. `spsc_push` and `spsc_pop`
  synchronize through one-way release/acquire pairs on `tail` and `head`,
  so both operations are wait-free.
- `head` and `tail` are monotonically increasing unsigned counters; the
  slot index is `counter & mask` (capacity is a power of two), so counter
  wrap-around through zero is handled by ordinary unsigned arithmetic.
- `head` and `tail` sit on separate 64-byte cache lines to avoid
  false-sharing between the producer and consumer cores.

Verification (`PROOF.md` holds the real build log and run output):

- Edge tests: pop on empty, fill to full, push on full rejected, FIFO order,
  drain back to empty; `spsc_init` rejects non-power-of-two capacities.
- 200,000 push/pop cycles on a 4-slot buffer (slot-index wrap-around).
- Head/tail counters preset to `SIZE_MAX - 2` and driven through zero
  mid-test, including a full-then-drain across the wrap (counter wrap-around).
- Stress: 10,000,000 items across two pthreads, order-checked with a
  checksum, plus a measured throughput figure.

Build and run:

```
make        # builds test_spsc with -O2 -Wall -Wextra -Wpedantic
make run    # runs the full suite
make run-tsan  # 1M items under ThreadSanitizer (if supported)
```
