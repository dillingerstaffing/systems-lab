<!-- PROOF-HEADER
Throughput: 190.4 Mops/s (push+pop counted)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/01-spsc-ring-buffer

Date: 2026-09-08. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build log and run output.

## Build log

```
$ make clean && make
rm -f test_spsc test_spsc_tsan
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -pthread -o test_spsc test_spsc.c
make exit=0
```

Zero warnings under `-Wall -Wextra -Wpedantic`.

## Run output

```
$ ./test_spsc
[edge] empty/full behavior, capacity 4
[edge] ok
[edge] init capacity validation
[edge] ok
[wrap] 200000 push/pop cycles on capacity 4
[wrap] ok
[wrap] head/tail counters wrapping past SIZE_MAX
[wrap] ok
[stress] 10000000 items, producer/consumer threads, cap 4096
[stress] transferred 10000000 items in 0.105 s
[stress] throughput: 190.4 Mops/s (push+pop counted)
[stress] checksum ok (49999995000000)
[stress] ok
ALL TESTS PASSED
run exit=0
```

## What was verified

- Empty pop rejected without touching `*out`; fill-to-full; push on full
  rejected; FIFO order preserved; drain back to empty.
- `spsc_init` rejects capacities 0, 3, 5, 6, 7, 12, 100 and accepts 1, 2,
  8, 16.
- 200,000 sequential push/pop cycles on a 4-slot buffer (slot-index
  wrap-around via the mask).
- Head/tail counters preset to `SIZE_MAX - 2`, driven through zero: 8 items
  pushed/popped in order across the wrap, full detected correctly at the
  wrap point, then 100 more post-wrap cycles.
- Stress: 10,000,000 items, one producer thread and one consumer thread,
  every item order-checked, checksum 49999995000000 matches the expected
  sum 0 + 1 + ... + 9999999. Throughput 190.4 Mops/s (each item counted as
  one push + one pop).

## Attempted but not claimable

ThreadSanitizer (`make run-tsan`) was attempted and could not run in this
container: the TSan runtime aborts at startup with
`FATAL: ThreadSanitizer: unexpected memory mapping`, a known incompatibility
between this TSan build and the kernel's address-space layout. It is an
environment failure, not a finding about this code, and no race-freedom
claim is made on its basis. The Makefile keeps the `tsan` target for
machines where TSan works.
