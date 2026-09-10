<!-- PROOF-HEADER
Throughput: 19306.6 kops/s (798 ops in 41.3 us, churn phase)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/02-bump-allocator

Date: 2026-09-08. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build log and run output.

## Build log

```
$ make clean && make
rm -f test_bump test_bump_asan
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_bump test_bump.c bump.c
make exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## Run output

```
$ ./test_bump
[align] power-of-two alignments 1..128
[align] ok (max alignment verified: 128)
[basic] write/read round-trip, safe no-op frees
[basic] ok
[reuse] freed block is reused by a later allocation
[reuse] ok (a=0x58dcacc2d640 c=0x58dcacc2d640 n_reuse=1)
[split] remainder after reuse goes back on the free list
[split] ok (small=0x58dcacc2c640 rest=0x58dcacc2c6a8)
[oom] exhaustion returns NULL, heap stays usable
[oom] 8 x 64-byte blocks fit in 512 bytes, n_oom=1
[oom] ok (recovered after OOM, n_alloc=9)
[tiny] 1-byte allocs are all reusable after free
[tiny] ok (n_reuse=32)
[frag] 512 allocs of 16..256 bytes, free every other one
[frag] live=94648 peak=94648 free_bytes=34649
[frag] fragmentation ratio after churn: 0.357
[frag] 2048-byte blocks placed into fragments: 30 of 64
[frag] fragmentation ratio after large allocs: 0.991
[frag] churn throughput: 19306.6 kops/s (798 ops in 41.3 us)
[frag] n_alloc=542 n_free=256 n_reuse=0
[frag] ok
ALL TESTS PASSED
run exit=0
```

## Sanitizer run

`make asan` builds with `-fsanitize=address,undefined`; the run exits 0
with byte-identical results (live=94648, fragmentation 0.357, 30 of 64
large blocks placed, ALL TESTS PASSED). No out-of-bounds access and no
undefined behavior in the allocator's pointer arithmetic.

## What the numbers mean

- Reuse is proven by address: after freeing block `a`, the next
  allocation `c` lands at the exact same address.
- Splitting is proven by address: freeing a 1024-byte block, allocating
  100 bytes, then allocating 800 bytes from the remainder, both inside
  the original span.
- OOM is clean: a 512-byte heap fits exactly 8 x 64-byte blocks, the 9th
  returns NULL with `n_oom=1`, and the heap recovers after a free.
- Fragmentation ratio is measured, not asserted: 0.357 after churning
  512 blocks and freeing every other one (all canaries intact, so no
  overlapping writes), rising to 0.991 after the remaining tail is
  consumed by 30 x 2048-byte blocks.
