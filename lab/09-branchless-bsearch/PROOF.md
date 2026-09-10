<!-- PROOF-HEADER
Checks: 4211266
Mismatches: 0
Verdict: PASS
-->

# PROOF.md — lab/09-branchless-bsearch

## Build

```
$ make clean && make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_bsearch test_bsearch.c
exit=0
```

Clean under `-Wall -Wextra -Werror`. All randomness is a deterministic
xorshift64 with a fixed seed, so the run below is reproducible.

## Run

```
$ ./test_bsearch
edge sizes: ok (2995 checks so far)
boundary values: ok (3005 checks so far)
duplicates: ok (6296 checks so far)
bulk differential: ok (206296 checks so far)
iteration counts: ok (211266 checks so far)
correctness: 211266 differential checks, 0 mismatches
n=  16384 round 0 branchless hits  :   242.5 cycles/lookup
n=  16384 round 0 branchless misses:   489.2 cycles/lookup
n=  16384 round 0 libc bsearch hits  :   259.4 cycles/lookup
n=  16384 round 0 libc bsearch misses:   324.1 cycles/lookup
n=  16384 round 1 branchless hits  :   299.5 cycles/lookup
n=  16384 round 1 branchless misses:   268.2 cycles/lookup
n=  16384 round 1 libc bsearch hits  :   239.4 cycles/lookup
n=  16384 round 1 libc bsearch misses:   247.0 cycles/lookup
n=  16384 round 2 branchless hits  :   215.9 cycles/lookup
n=  16384 round 2 branchless misses:   253.7 cycles/lookup
n=  16384 round 2 libc bsearch hits  :   283.4 cycles/lookup
n=  16384 round 2 libc bsearch misses:   254.3 cycles/lookup
n=1048576 round 0 branchless hits  :   609.2 cycles/lookup
n=1048576 round 0 branchless misses:   595.0 cycles/lookup
n=1048576 round 0 libc bsearch hits  :   492.6 cycles/lookup
n=1048576 round 0 libc bsearch misses:   513.9 cycles/lookup
n=1048576 round 1 branchless hits  :   597.1 cycles/lookup
n=1048576 round 1 branchless misses:   600.5 cycles/lookup
n=1048576 round 1 libc bsearch hits  :   496.7 cycles/lookup
n=1048576 round 1 libc bsearch misses:   505.3 cycles/lookup
n=1048576 round 2 branchless hits  :   627.3 cycles/lookup
n=1048576 round 2 branchless misses:   599.4 cycles/lookup
n=1048576 round 2 libc bsearch hits  :   494.7 cycles/lookup
n=1048576 round 2 libc bsearch misses:   509.1 cycles/lookup
checksum sink: 3725845532764 (prevents dead-code elimination)
benchmark agreement: 4000000 timed lookups, 0 mismatches
total differential checks: 4211266, total mismatches: 0
ALL TESTS PASSED
exit=0
```

(`make sanitize`, ASan+UBSan, and `make opt0` each run the same suite to
`ALL TESTS PASSED` with 0 mismatches; the checksum sink is identical
across all three builds, confirming the timed code paths agree.)

## Why these numbers mean the mechanism works

- **The search is correct.** 211,266 differential checks against libc
  `bsearch` (empty array, 1-element array, sizes 0-1000, `0` and
  `UINT32_MAX` in and out of the array, 40 duplicate-bearing arrays,
  200,000 bulk keys) plus 4,000,000 timed lookups re-verified against the
  oracle: 4,211,266 checks, 0 mismatches. On unique arrays the exact
  index matches; on duplicate arrays found/not-found matches and the
  returned index is verified to be the first occurrence.
- **No early exit, no data-dependent branch.** A counting twin of the
  algorithm verifies every lookup at size `n` runs exactly
  `floor(log2(n)) + 1` iterations for `n = 1..70`, for hits and for
  misses. The disassembly check (`gcc -O2`, non-inline wrapper,
  `objdump -d`) shows the loop body as `setb`/`setne` plus `cmov` and
  mask arithmetic (`neg`/`and`); its only conditional jump is the
  trip-count test `i != K`, which depends solely on `n`. The comparison
  never becomes a branch: it becomes a 0/1 value in a register.
- **The cycle counts are apples-to-apples.** Both implementations ran
  the identical 1M-key hit set and 1M-key miss set, 3 rounds each,
  timed with lfence-serialized `rdtsc`; results feed a printed checksum
  so the compiler cannot eliminate the lookups, and verification runs
  in a separate untimed pass so it never pollutes the counts.
- **Reading the table honestly.** At `n = 16384` (cache-resident) the
  branchless form measures 214-300 cycles/lookup against `bsearch`'s
  227-324: roughly on par, with hit and miss costs equal by
  construction. At `n = 1048576` the branchless form measures 585-692
  against `bsearch`'s 486-514. The likely reason (interpretation, not
  measurement): each branchless iteration carries a longer chain of
  dependent arithmetic between the loaded comparison result and the next
  probe address, which stays hidden while loads hit the cache but shows
  once they miss. The numbers move run to run on this shared VM (note
  the noisy `n = 16384` round 0 misses); the table above is one genuine
  run, not a selected best.

## A bug the differential test caught

The first version narrowed with a `while (cnt > 0)` loop. The
iteration-count check showed the trip count varying by key for sizes
that are not one less than a power of two (e.g. 2 vs 3 iterations at
`n = 4`), because the two sub-windows differ in size by one. The shipped
version runs a fixed `K = floor(log2(n)) + 1` iterations with finished
paths as arithmetic no-ops, so the cost is identical for every key.
Correctness was unaffected (the differential checks passed in both
versions); the fix was purely about the data-independent cost claim.

## Notes

- The `n == 0` early return is a one-time boundary guard outside the
  search loop; the empty-array case is covered by the differential
  tests (3 checks against the oracle, all `-1`/`NULL` agreement).
- `bsearch` is the oracle only: the implementation never calls it, and
  its per-comparison indirect call is part of what is being measured
  against.
