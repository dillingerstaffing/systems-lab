<!-- PROOF-HEADER
Checks: 36
Mismatches: 0
Checksum: bfe44debb6942916f4b4e520f8729df3d070607b9dc533c0097ae9aa23c9b5d8
Environment: Host (Linux 7.0.0-38-generic, x86-64, gcc 13.3.0, glibc 2.39)
Verdict: PASS
-->
# PROOF.md, lab/158-kalloc

`kalloc_test.c` proves the central claim of xv6's physical page allocator
firsthand: the free list is threaded through the free pages themselves, so
the allocator's books live in the empty shelves. `kalloc.c` is verbatim
from mit-pdos/xv6-riscv master (fetched 2026-09-18); the real xv6 headers
are used unmodified; only the spinlock body (pthread mutex) and `panic()`
(longjmp) are host shims, both documented in the README.

36 checks, 0 mismatches: 3 runs x 12 assertions each (`freerange` over the
64 MiB arena drains exactly 16,383 distinct aligned pages; an unaligned
`freerange` frees exactly one page via `PGROUNDUP`; the freelist is a
stack, free a,b,c then kalloc returns c,b,a; `kfree` fills with 0x01 and
`kalloc` refills with 0x05, the "catch dangling refs" junk; `kfree` panics
on a misaligned address, an address below `end`, and an address >=
`PHYSTOP`; 4 threads x 3,000 alloc/free cycles conserve all 16,383 pages
with acquire/release counts balanced at 89,553 apiece). The checksum is the
SHA-256 of the 36 `ok:`/`FAIL:` assertion lines across the three runs; all
three runs are byte-identical. Clean under `-std=gnu11 -O2 -Wall -Wextra
-Werror`, `-O0`, and `-fsanitize=address,undefined`, all with zero reports.

## Build log (genuine)

```
$ gcc -std=gnu11 -O2 -Wall -Wextra -Werror -no-pie -Wno-builtin-declaration-mismatch -Wl,--defsym,end=0x10000000 -o kalloc_test kalloc_test.c kalloc.c host_spinlock.c host_defs.c -pthread
build: clean, no warnings
$ gcc -std=gnu11 -O1 -g -Wall -Wextra -Werror -no-pie -Wno-builtin-declaration-mismatch -Wl,--defsym,end=0x10000000 -fsanitize=address,undefined -o kalloc_test_asan kalloc_test.c kalloc.c host_spinlock.c host_defs.c -pthread
build: clean, no warnings
$ gcc -std=gnu11 -O0 -Wall -Wextra -Werror -no-pie -Wno-builtin-declaration-mismatch -Wl,--defsym,end=0x10000000 -o kalloc_test_o0 kalloc_test.c kalloc.c host_spinlock.c host_defs.c -pthread
build: clean, no warnings
```

`-no-pie` plus `-Wl,--defsym,end=0x10000000`: xv6's `kfree` guard is
`(pa < end || pa >= PHYSTOP)` with `PHYSTOP = 0x88000000`, so the 64 MiB
fake arena must sit below 2 GiB; `end` is placed at 0x10000000 and the arena
is mmaped there `MAP_FIXED`. A PIE build resolved the absolute `end` symbol
to garbage under ASLR and tripped the guard on the first free.
`-Wno-builtin-declaration-mismatch`: xv6's `defs.h` declares `memset` with a
32-bit length, colliding with host libc's `size_t`; harmless since `PGSIZE`
fits. `-std=gnu11` matches xv6's own `-std=gnu99` build so `riscv.h`'s GNU
`asm` CSR helpers compile.

## Run output (genuine, one run; all three runs byte-identical)

```
ok: T1 drain count == 16383 pages
ok: T1 pages distinct, aligned, inside arena
ok: T4 PGROUNDUP: only page end+4096 freed from (end+100, end+100+2pg)
ok: T4 second kalloc is 0 (exactly one page qualified)
ok: T2 LIFO order: last freed is first allocated
ok: T3 kfree fills page with 0x01
ok: T3 kalloc refills page with 0x05
ok: T5 misaligned address panics
ok: T5 address below end panics
ok: T5 address >= PHYSTOP panics
ok: T6 threaded alloc/free conserves all 16383 pages
ok: lock discipline: acquires == releases
acquires=89553 releases=89553 fails=0
```

## Source evidence (genuine, the verified file)

The freelist node lives in the free page itself (`kalloc.c`, verbatim):

```c
struct run {
  struct run *next;
};
```

`kfree` pushes the page onto the list by writing the link into the page:

```c
  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
```

`kalloc` pops the head:

```c
  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);
```

One pointer read and one pointer write per operation, under a single
spinlock. The allocator starts with nothing and `kinit` gives it everything:
`freerange(end, (void *)PHYSTOP)` turns every page between the kernel and
the 128 MiB mark into one long list via per-page `kfree` calls.
