# lab/09-branchless-bsearch

Binary search over a sorted array of `uint32_t` whose hot loop contains no
data-dependent branch. Header-only: `bsearch_branchless.h`. Tests:
`test_bsearch.c`.

- `bsearch_branchless.h`
  - The search window is `[lo, lo + len)`. Each iteration compares
    `a[mid] < key`, which yields a 0/1 integer, and narrows the window by
    arithmetic alone: `lo += lt * (half + 1)`,
    `len = half + lt * (len - 2 * half - 1)`. The loop runs exactly
    `floor(log2(n)) + 1` iterations for every key at a given `n`; paths
    that empty the window early spend the remaining trips as no-ops
    (`live = 0` forces `lt = 0` and reads `a[0]` instead of `a[mid]`).
    No early exit, so a hit costs exactly what a miss costs.
  - Returns the index of the first element equal to `key` (lower-bound
    semantics, so duplicates resolve to the lowest index), or `-1` on a
    miss. The final select is arithmetic (`hit * lo - (1 - hit)`); the
    only branch in the function is the one-time `n == 0` guard.
  - Never calls libc `bsearch`; `bsearch` is used only by the test
    program as the differential oracle.
  - Disassembly check (`gcc -O2`, non-inline wrapper, `objdump -d`):
    the loop body compiles to `setb`/`setne` plus `cmov` and mask
    arithmetic; its single conditional jump is the trip-count test
    `i != K`, which depends only on `n`, never on the key or the array
    contents.
- `test_bsearch.c`
  - Differential test against libc `bsearch`: edge sizes 0 through 1000
    (every hit, below-min/above-max misses, gap keys, `0` and
    `UINT32_MAX` keys), arrays containing `0` and `UINT32_MAX`, 40
    duplicate-bearing arrays (found/not-found must agree; on hit the
    index must hold `key` and be the first occurrence), and 200,000 bulk
    keys against a 100,000-element array. Unique arrays compare exact
    index; `bsearch` makes no order guarantee for duplicates, so those
    compare found/not-found plus first-occurrence validity.
  - Iteration-count check: a counting twin of the algorithm confirms
    every lookup runs exactly `floor(log2(n)) + 1` iterations for
    `n = 1..70`, hits and misses alike.
  - Benchmark: `rdtsc` (lfence-serialized) cycles per lookup over 1M
    lookups x 3 rounds, identical key sets for both implementations,
    hits and misses separately, at `n = 16384` (cache-resident) and
    `n = 1048576`. Timing loops are pure; agreement with the oracle is
    re-checked in an untimed pass (4M extra differential cases).
  - Captured run: 211,266 differential checks plus 4,000,000 timed
    lookups, 0 mismatches. `n = 16384`: branchless 214-300
    cycles/lookup vs libc `bsearch` 227-324 (hit and miss costs equal by
    construction). `n = 1048576`: branchless 585-692 vs `bsearch`
    486-514; once lookups miss the caches, `bsearch`'s shorter
    per-iteration dependency chain wins. Numbers vary run to run on the
    shared VM; the full per-round table is in `PROOF.md`.

Build: `make` (`-Wall -Wextra -Werror` clean), run: `make run`,
`make sanitize` (ASan+UBSan, full suite passes), `make opt0` (passes).
Genuine build log and output are in `PROOF.md`.
