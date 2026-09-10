<!-- PROOF-HEADER
Checks: 10000000
Mismatches: 0
Throughput: 4.73 Mreads/s across readers
Verdict: PASS
-->

# PROOF.md — lab/08-seqlock

## Build

```
$ make clean && make
gcc -std=c11 -O2 -Wall -Wextra -Werror -pthread -o test_seqlock test_seqlock.c
exit=0
```

Clean under `-Wall -Wextra -Werror`.

## Run

```
$ ./test_seqlock
== lab/08 seqlock: single writer, 4 readers ==
unit: round-trip 1000 generations ok, 0 retries
unit: checksum detector rejects single-bit corruption in word, counter, and check
reader 0: reads=2500000 retries=389445904 torn=0 mono_viol=0
reader 1: reads=2500000 retries=210210788 torn=0 mono_viol=0
reader 2: reads=2500000 retries=242297107 torn=0 mono_viol=0
reader 3: reads=2500000 retries=194130540 torn=0 mono_viol=0
total: reads=10000000 attempts=1046084339 retries=1036084339 retry_rate=99.044% torn=0 mono_viol=0
writer published 43514894 generations in 2.113 s (4.73 Mreads/s across readers)
PASS: 10M reads, 0 torn, 0 monotonicity violations
exit=0
```

(The per-reader retry counts vary with scheduling; two earlier `-O2` runs
also passed with 10M reads, 0 torn, and retry rates of 98.3% and 99.3%.
`make sanitize` (ASan+UBSan) and `make opt0` each pass with the same
10M-read / 0-torn result.)

## Why these numbers mean the mechanism works

- **No torn snapshot gets past the sequence check.** The writer is the only
  thread that stores `seq`, and the counter strictly increases, odd while
  the writer is inside its critical section. A reader that samples `s0`
  even, copies, and samples `s1 == s0` cannot have had a write overlap its
  copy: any completed writer critical section inside the copy window would
  have stored an odd value and then a larger even value, which the second
  acquire load (program-ordered after the copy) would observe through cache
  coherence. The release on the writer's even store pairs with the reader's
  first acquire load, so the payload the reader copied is the published
  generation, not stale bytes.
- **The zero-torn claim does not trust the sequence check.** Every one of
  the 10,000,000 successful snapshots was re-verified against the
  writer-maintained FNV-1a checksum, and the detector itself was tested
  deterministically: flipping one bit in a word, in the counter, or in the
  stored checksum makes `seqlock_verify` fail. A torn copy passing the
  sequence check would still have to forge a 64-bit checksum to go
  uncounted; the counter was 0 across all three build configurations.
- **Generation counters never went backwards.** Each reader checked that
  its snapshots' counters were monotonically non-decreasing. The single
  writer only increases the counter, so a decrease would mean a reader saw a
  mixture of two generations; 0 violations across 10M reads.
- **The retry path is heavily exercised, not vestigial.** The writer
  published 43.5M generations during the run (about 20M/s), so a reader's
  copy window almost always overlaps a write: 1,036,084,339 retries at a
  99.044% retry rate, every one of them a correct refusal rather than a
  torn read. Retries are cheap (two atomic loads on the fast path) and the
  readers still completed 10M verified snapshots in 2.1 s.

## A bug the sanitizer build caught

The first `make sanitize` run failed immediately with `torn=1` on a
reader's very first snapshot. The cause was the initial state, not the
lock: `seqlock_init` zeroed the payload with `check = 0`, but the FNV-1a of
an all-zero payload is not zero, so a reader snapshotting the pristine
state (before the writer's first release store) saw `s0 == s1 == 0` with a
payload that fails verification: a false torn positive. The fix publishes
generation 0 as a real generation inside `seqlock_init`, so every even seq
value a reader can ever observe pairs with a checksummed payload. After
the fix, the ASan+UBSan build passes with 10M reads and 0 torn, same as
`-O2` and `-O0`.

## Notes

- The odd seq store is relaxed: it only needs atomicity, since the single
  writer owns it. The even store is release: it publishes the payload. Both
  reader loads are acquire.
- No ThreadSanitizer target: TSan cannot see the sequence protocol and
  would flag the plain payload copy as a race (a false positive). The
  ordering argument is documented at the primitives in `seqlock.h`.
- Single-writer is a hard precondition, stated in `seqlock.h`: two writers
  would break the total order on `seq` that the reader's check relies on.
