# Bump allocator with free-list reuse

Bare-metal-friendly bump allocator in C11 (`bump.h` / `bump.c`): allocations
come from a bump pointer over a caller-supplied region, freed blocks go on
an intrusive free list and are reused (first fit with splitting, no
coalescing). No libc calls in the allocator itself, so it builds for
`-ffreestanding` targets.

API: `bump_init`, `bump_alloc(heap, size, align)`, `bump_free(heap, ptr,
size)`, plus `bump_live_bytes`, `bump_free_bytes`, `bump_fragmentation`.

Verified by `test_bump.c` against fake heap regions: alignment 1..128,
non-power-of-two rejection, write/read round-trip, free-list reuse proven
by address, block splitting, clean OOM with recovery, 1-byte allocations,
and a 512-block churn test with canary integrity checks and a measured
fragmentation ratio.

Build: `make` (requires `-Wall -Wextra -Werror` clean), `make run`,
`make run-asan` for the sanitizer pass. Genuine build log and output are
in `PROOF.md`.
