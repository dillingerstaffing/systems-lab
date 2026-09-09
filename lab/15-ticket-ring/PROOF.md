# PROOF.md: lab/15-ticket-ring

Date: 2026-09-09. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build log and run output.

## Build log

```
$ make clean && make
rm -f test_ticket_ring test_ticket_ring_asan
gcc -std=c11 -O2 -Wall -Wextra -Werror -pthread -o test_ticket_ring test_ticket_ring.c
make exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## Run output

```
$ ./test_ticket_ring
[edge] empty pop on fresh ring
[edge] ok
[edge] fill to capacity 1024, one more must fail
[edge] ok
[edge] drain order is FIFO
[edge] ok
[order] 8 threads x 8 tickets, admission must equal ticket order
[order] ok: 64 admissions in exact ticket order
[stress] 4 producers x 250000 items, ring capacity 1024, 1 consumer
[stress] drained 1000000 items in 7.371 s
[stress] throughput: 0.3 Mops/s (push+pop counted)
[stress] lost items: 0
[stress] duplicate items: 0 (bitmap check)
[stress] per-producer FIFO order: preserved (seq == last+1 held)
[stress] checksum: 499999500000, expected 499999500000
[stress] producer 0 max fairness gap: 4 items
[stress] producer 1 max fairness gap: 4 items
[stress] producer 2 max fairness gap: 4 items
[stress] producer 3 max fairness gap: 4 items
[stress] worst fairness gap across all producers: 4 (ideal 3)
[stress] ok
ALL TESTS PASSED
run exit=0
```

## Sanitizer run

```
$ make run-asan
gcc -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Wall -Wextra -Werror -pthread -o test_ticket_ring_asan test_ticket_ring.c
./test_ticket_ring_asan
```

Same full 1,000,000-item workload under AddressSanitizer and
UndefinedBehaviorSanitizer: zero reports, ALL TESTS PASSED, same measured
numbers (0 lost, 0 duplicates, checksum 499999500000, worst fairness gap
4). asan exit=0.

## What was verified

- Empty pop rejected without touching `*out`; fill-to-full at 1024;
  push on full rejected; full drain returned items in FIFO order.
- Ticket admission order: 8 threads taking 8 tickets each were admitted
  in exact ticket order 0..63, confirming FIFO admission with no
  starvation.
- Stress: 1,000,000 items through the ring, 4 producers x 250,000.
  - Loss: a 1,000,000-entry bitmap recorded every item id; 0 lost.
  - Duplication: the same bitmap flagged any repeat; 0 duplicates.
  - Per-producer FIFO: each producer's items arrived seq 0..249999 in
    order; the seq == last+1 check held for all 1,000,000 items.
  - Checksum: sum of all item ids = 499999500000, equal to the
    closed-form expected sum.
  - Fairness: worst gap between consecutive items of the same producer
    was 4 other-producer items, against an ideal of 3. The gap is
    measured from the served order, not assumed: the ticket lock
    guarantees FIFO admission of lock requests, but a producer
    descheduled between releasing the lock and re-acquiring can miss
    its turn in the round-robin, letting another producer serve twice
    in a row. No producer was starved; every producer's 250,000 items
    all arrived.

## Notes

- Each producer holds the ticket lock for exactly one push, then
  releases; on a full ring it releases, sleeps 1 us, and retries with
  a fresh ticket. The consumer is lock-free.
- The acquire spin loop pauses briefly, then yields after 64 spins, so
  the test completes on an oversubscribed 2-core machine instead of
  burning whole time slices. This changes no ordering guarantee: ticket
  order still determines admission order.
- Throughput (0.3 Mops/s) is reported as measured on this loaded VM;
  it is a property of this run's environment, not of the algorithm.
