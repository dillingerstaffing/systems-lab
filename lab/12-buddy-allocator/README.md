# lab/12-buddy-allocator

Buddy allocator in C: power-of-two splitting on allocation, xor-buddy
coalescing on free, over a caller-supplied power-of-two heap. 32-byte
minimum block, 16-byte header (block order at offset 0, intrusive free-list
link at offset 8), caller pointer at block + 16, always 16-byte aligned.
No libc calls in `buddy.c`/`buddy.h` (only `<stddef.h>`, `<stdint.h>`), no
static state, every failure is a NULL return.

Verified by `test_buddy.c`:

- 1024 allocations: every pointer 16-byte aligned, every block base
  aligned to its own block size, every block the smallest power of two
  holding the request.
- 512 live blocks: pairwise disjoint (130,816 pairs checked), payload
  canaries intact.
- Free-all in shuffled order collapses a 64 KiB heap back into exactly
  one block (`free_bytes == largest_free == 65536`), and a full-heap
  allocation succeeds afterwards.
- Churn comparison: the same deterministic workload (512 allocations of
  16..256 bytes from a fixed LCG sequence, every other block freed, 256
  left live) runs through this allocator and through lab/02's bump
  allocator, unmodified. Fragmentation ratio, defined identically for
  both as `(free_bytes - largest_free_block) / free_bytes`, measured
  after the churn: buddy 0.789, bump 0.357. Placing 2048-byte blocks into
  the fragments afterwards: buddy 5 of 64, bump 30 of 64.
- Bad inputs rejected safely: non-power-of-two heap size, misaligned
  heap base, oversize request (NULL + `n_oom`), zero-size request,
  NULL free.

The checkerboard free pattern (every other block) is adversarial to
coalescing by construction: each freed block's buddy stays live, so no
merge is possible. That is why the buddy number is higher here; the
workload is identical for both allocators, so the two ratios are directly
comparable.

Build: `make` (`-Wall -Wextra -Werror`), `make run`, `make run-asan`
(AddressSanitizer + UBSan). See `PROOF.md` for the genuine build log and
run output.
