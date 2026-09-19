# lab/158-kalloc

xv6's physical page allocator: the allocator that keeps its books in the
empty shelves.

Every allocator faces a bootstrap question: where does it keep its books?
`kalloc.c` (76 lines, verbatim from mit-pdos/xv6-riscv master) answers by
keeping no books at all. The free list is threaded through the free pages
themselves: each free page's first eight bytes hold a `struct run { struct
run *next; }`, the list node stored in the empty shelf. Since nothing else
is stored in a free page, the allocator needs no separate metadata, which
matters when the allocator itself is what provides memory.

This lab compiles the real `kalloc.c` and the real xv6 headers (`types.h`,
`param.h`, `memlayout.h`, `spinlock.h`, `riscv.h`, `defs.h`) unmodified
against a 64 MiB fake arena on the host, and runs it. Two pieces are host
shims, both documented in the README and the source:

- `host_spinlock.c`: xv6's spinlock API over a pthread mutex, with
  acquire/release counters (and aborts on double-acquire or unbalanced
  release, so `kalloc.c`'s lock discipline is verified).
- `host_defs.c`: `panic()` longjmps to the harness instead of killing the
  machine, so `kfree`'s guard checks are testable.

Two build accommodations (not source changes to `kalloc.c`):

- xv6 builds with `-std=gnu99`; this lab uses `-std=gnu11` so `riscv.h`'s
  GNU `asm` CSR helpers compile.
- `kalloc.c` is compiled with `-Wno-builtin-declaration-mismatch`: xv6's
  `defs.h` declares `memset` with a 32-bit length, colliding with host
  libc's `size_t`; harmless here since `PGSIZE` fits.
- xv6's `kfree` guard is `(pa < end || pa >= PHYSTOP)` with
  `PHYSTOP = 0x88000000`, so the 64 MiB fake arena must sit below 2 GiB:
  the `end` symbol is placed at 0x10000000 via `-Wl,--defsym` and the arena
  is mmaped there `MAP_FIXED`; `-no-pie` is required (an earlier PIE build
  resolved the absolute `end` symbol to garbage under ASLR and tripped the
  guard). Page 0 of the arena stands in for the kernel; `freerange` starts
  at `end + PGSIZE`.

What the harness proves firsthand:

- **T1**: `freerange(end + PGSIZE, end + 64 MiB)` then a full drain yields
  exactly 16,383 pages, all distinct, 4096-aligned, inside the arena.
- **T4**: an unaligned `freerange(end + 100, end + 100 + 2*PGSIZE)` frees
  exactly one page (`PGROUNDUP` rounds the start up; the second candidate
  fails the `p + PGSIZE <= pa_end` bound).
- **T2**: the freelist is a stack: free a, b, c, then `kalloc` returns
  c, b, a. Push to head, pop from head.
- **T3**: `kfree` fills the freed page with 0x01, `kalloc` refills with
  0x05 (the "catch dangling refs" junk from the source comments; the
  freelist `next` pointer overwrites the first eight bytes afterward).
- **T5**: `kfree` panics on a misaligned address, an address below `end`,
  and an address at or above `PHYSTOP` (all three caught via longjmp).
- **T6**: four threads doing 3,000 `kalloc`/`kfree` cycles each conserve
  all 16,383 pages; acquire/release counts balance at 89,553 apiece.

Three runs, byte-identical. Clean under `-O2`, `-O0`, and
`-fsanitize=address,undefined`, all 12 assertions passing each way.

## The mechanism, in the source's own words

xv6's own section 3.5 states the design in one sentence: the allocator's
data structure is a free list of physical pages, and each free page's list
node is stored in the free page itself. Allocation is removing the head of
the list; freeing is pushing onto it; one pointer read and one pointer
write, under a single spinlock. At boot, `kinit` calls
`freerange(end, PHYSTOP)`, turning every page between the kernel and the
128 MiB mark into one long list via per-page `kfree` calls: the allocator
starts with nothing and is given everything.

## Why not the obvious alternatives

- A bump allocator cannot free, and every xv6 page (user pages, kernel
  stacks, page-table pages, pipe buffers) must come back.
- A buddy allocator coalesces but costs complexity this teaching kernel
  refuses to pay; xv6 never splits or merges, it only ever hands out whole
  pages.

The honest cost: one global freelist under one spinlock. MIT's own locks
lab exists to fix exactly this, having students split the allocator into
per-CPU freelists with stealing because `kalloctest` contends on
`kmem.lock`.

## Honest boundaries

This exercises the allocator's algorithm (freelist mechanics, guards, junk
fills, locking), not its boot integration: on real xv6, `kinit()` runs
`freerange(end, PHYSTOP)` over the 128 MiB above the kernel at 0x80000000 on
QEMU's virt board (from `memlayout.h`, read not executed). The 0x01/0x05
junk-fill values are from the fetched master. The pthread-mutex spinlock
stands in for RISC-V atomics; the lock discipline (every acquire released,
never double-held) is what is verified.

## Build and run

`make run` builds and runs the harness (3 runs, 36 assertions, 0 mismatches
expected). `make sanitize` and `make opt0` re-verify under
`-fsanitize=address,undefined` and `-O0`. The genuine build log and run
output are in `PROOF.md`.
