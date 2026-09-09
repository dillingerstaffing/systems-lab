# lab/15-ticket-ring: MPMC ring buffer under a ticket lock

A many-producer, single-consumer ring buffer (capacity 1024) where every
push happens inside a ticket lock: `ticket.h` implements the lock on C11
atomics (fetch_add hands out unique increasing tickets, `serving` admits
holders in ticket order), and `ring.h` implements the bounded ring
(index = counter mod capacity, full when head - tail == capacity).
Producers hold the lock for exactly one push; the single consumer needs
no lock of its own.

Verified by `test_ticket_ring.c`:

- edge behavior: empty pop rejected, fill-to-full at 1024, push on full
  rejected, full drain in FIFO order.
- ticket admission order: 8 threads x 8 tickets admitted in exact ticket
  order 0..63.
- stress: 4 producers x 250,000 items each, 1,000,000 items drained.
  Zero lost items, zero duplicates (1M-entry bitmap), per-producer FIFO
  order preserved, checksum matched against the closed-form sum, and the
  worst fairness gap across producers measured (see PROOF.md for the
  numbers).

Build: `make` (needs `-pthread`). Sanitizer run: `make run-asan`.
