# lab/05: deterministic fuzz harness (C11)

A deterministic fuzz harness over two lab modules, built to run millions
of randomized operations against independent models and report exactly
what held. The module sources (`spsc.h`, `bump.h`, `bump.c`) are copied
into this directory so the harness builds standalone.

## What it does

- **PRNG:** xorshift64 with the fixed seed `0x123456789ABCDEF0`. Every
  decision in the run derives from it, so the run is exactly reproducible;
  the output includes a digest line that must match across runs
  (`make determinism` checks this).
- **Ring buffer (lab/01):** 2,000,000 interleaved push/pop operations with
  random payloads against an independent model (a plain array holding the
  expected queue contents). Every push/pop result must agree with the
  model's full/empty state, every popped value must equal the model's
  oldest value, a sentinel verifies pop-on-empty leaves `*out` untouched,
  and `spsc_size` / `spsc_full` / `spsc_empty` are cross-checked against
  the model every 65,536 ops.
- **Bump allocator (lab/02):** 4,000,000 operations in 16 epochs over a
  fresh 256 KiB heap per epoch. Op mix: 40% alloc (size 1..128, alignment
  class 0,1,2,4,8,16,32,64,128), 30% canary touch of a random live block,
  30% free of a random live block. The live set is kept in a 64..192 band
  so all three op types fire at healthy rates. Every returned pointer is
  checked for heap bounds, for the requested alignment, and for overlap
  with every other live block. Each block carries a canary derived from a
  per-allocation tag, verified on every touch and before every free. Each
  epoch ends by verifying `bump_live_bytes` equals the tracked live total,
  then freeing everything and requiring it to return to zero.
- **OOM is a counted outcome, not a failure.** The allocator never
  coalesces and drops the smaller split sliver, so a churned heap
  legitimately exhausts; every NULL return is a clean, verified path.
  Double-free is not exercised: the allocator documents it as an
  undetected caller bug, and the harness never double-frees by
  construction (freed blocks leave the tracking table).

## What was verified (real numbers from the run in PROOF.md)

- 6,000,000 total operations, 0 crashes, 0 invariant violations.
- Ring: 969,687 pushes, 969,685 pops, 29,779 full-hits, 30,849 empty-hits;
  FIFO order held on every pop; 85.3 Mops/s.
- Allocator: 699,163 successful allocs (every one bounds-, alignment-
  and overlap-checked), 668,778 canary touches (all intact), 699,163
  frees (canary verified before each), 664,682 free-list reuses,
  1,933,913 clean OOM NULLs; every epoch drained with `bump_live_bytes`
  returning to exactly 0.
- Observed behavior, measured: under this churn the allocator sustains
  roughly 43,500-43,900 successful allocations per 256 KiB heap before
  fragmentation stops satisfying requests; the harness ends the epoch
  there (sliding 4096-attempt window under 10% success) and starts fresh.
- Reproducibility: two runs print the same digest
  (`0x670F6CCDEA1A651C`).
- Sanitizers: AddressSanitizer clean (zero reports over the full run).
  UBSan flagged 17 misaligned-access reports in `bump.c`: splitting a
  free block can place the intrusive `struct free_node` (needs 8-byte
  alignment) at a sub-8 address when the request size is not a multiple
  of 8 (e.g. `bump_alloc(h, 20, 1)` leaves the tail node at base+20).
  Benign on x86-64, but a real portability hazard for strict-alignment
  targets. The harness's own invariants all held; root cause, minimal
  reproducer, and a proposed contract-preserving fix are in PROOF.md.
  The finding is against lab/02's `bump.c` (copied here faithfully) and
  is left for the lab/02 owner.

## Build and run

```
make            # builds ./fuzz with -O2 -Wall -Wextra -Wpedantic -Werror
make run        # runs the full harness (~100 s on this machine)
make run-asan   # full run under ASan+UBSan
make determinism# two runs, diffs the digest lines
make clean
```

`PROOF.md` holds the genuine build log and run output.
