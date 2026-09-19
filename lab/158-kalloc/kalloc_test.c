// Workbench: xv6's physical page allocator (kernel/kalloc.c, verbatim from
// mit-pdos/xv6-riscv master) compiled for the host and exercised against a
// 64 MiB fake arena. The real headers (types.h, param.h, memlayout.h,
// spinlock.h, riscv.h) are used unmodified; only the spinlock body (pthread
// mutex) and panic() (longjmp) are host shims. (defs.h is not includable on
// the host: it redeclares the string functions with xv6's uint.)
//
// xv6's kfree guard is (pa < end || pa >= PHYSTOP) with PHYSTOP = 0x88000000,
// so the fake arena must live below 2 GiB: `end` is placed by the linker at
// 0x10000000 (--defsym) and the arena is mmaped there MAP_FIXED.
#include <pthread.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"

void freerange(void *pa_start, void *pa_end);
void *kalloc(void);
void kfree(void *pa);
void initlock(struct spinlock *lk, char *name);

struct xv6_kmem { struct spinlock lock; void *freelist; };
extern struct xv6_kmem kmem;
#define end ((char *)0x10000000)

extern jmp_buf panic_jmp;
extern char panic_msg[128];
extern long spin_acquires, spin_releases;

#define ARENA (64UL * 1024 * 1024)
#define EXPECTED ((ARENA - PGSIZE) / PGSIZE) // 16383

static int fails = 0;
#define CHECK(cond, label) do { \
  if (cond) { printf("ok: %s\n", label); } \
  else { printf("FAIL: %s\n", label); fails++; } \
  fflush(stdout); \
} while (0)

static int cmpptr(const void *a, const void *b) {
  uintptr_t x = *(uintptr_t *)a, y = *(uintptr_t *)b;
  return (x > y) - (x < y);
}

static void *thread_fn(void *arg) {
  (void)arg;
  for (int i = 0; i < 3000; i++) {
    char *p = kalloc();
    if (p) { p[0] = (char)i; kfree(p); }
  }
  return 0;
}

int
main(void)
{
  void *arena = mmap((void *)end, ARENA, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  if (arena != (void *)end) { printf("FAIL mmap fixed arena\n"); return 1; }
  initlock(&kmem.lock, "kmem"); // what xv6's kinit() does before freerange

  // T1: freerange the arena, drain it, verify every page.
  freerange(end + PGSIZE, end + ARENA);
  static char *held[EXPECTED + 8];
  long n = 0;
  char *p;
  while ((p = kalloc()) != 0) {
    if (n < (long)(EXPECTED + 8)) held[n] = p;
    n++;
  }
  CHECK(n == (long)EXPECTED, "T1 drain count == 16383 pages");
  int bad = 0;
  for (long i = 0; i < n && i < (long)(EXPECTED + 8); i++) {
    uintptr_t a = (uintptr_t)held[i];
    if (a % PGSIZE) bad = 1;
    if (a < (uintptr_t)(end + PGSIZE) || a >= (uintptr_t)(end + ARENA)) bad = 1;
  }
  qsort(held, n, sizeof(char *), cmpptr);
  for (long i = 1; i < n; i++)
    if (held[i] == held[i - 1]) bad = 1;
  CHECK(!bad, "T1 pages distinct, aligned, inside arena");

  // T4: freerange rounds an unaligned range (list is empty; all pages held).
  // Note: end+4096 is held[0] after the qsort, so it must not be freed twice.
  freerange(end + 100, end + 100 + 2 * PGSIZE);
  char *q = kalloc();
  CHECK(q == end + PGSIZE, "T4 PGROUNDUP: only page end+4096 freed from (end+100, end+100+2pg)");
  CHECK(kalloc() == 0, "T4 second kalloc is 0 (exactly one page qualified)");

  // Restore the full freelist (q is held[0]; free it once via q).
  kfree(q);
  for (long i = 1; i < n; i++) kfree(held[i]);

  // T2: the freelist is a stack (kfree pushes head, kalloc pops head).
  char *a = kalloc(), *b = kalloc(), *c = kalloc();
  kfree(a); kfree(b); kfree(c);
  char *p1 = kalloc(), *p2 = kalloc(), *p3 = kalloc();
  int lifo = (p1 == c) && (p2 == b) && (p3 == a);
  CHECK(lifo, "T2 LIFO order: last freed is first allocated");
  kfree(p1); kfree(p2); kfree(p3);

  // T3: junk fills. kfree writes 1, kalloc writes 5 ("catch dangling refs").
  // (kfree's first 8 bytes are then overwritten by the freelist next-pointer,
  // so the 0x01 check reads past it.)
  char *j = kalloc();
  memset(j, 0xAA, PGSIZE);
  kfree(j);
  int freed_junk = ((unsigned char)j[8] == 1) && ((unsigned char)j[PGSIZE - 1] == 1);
  char *j2 = kalloc();
  int alloc_junk = (j2 == j) && ((unsigned char)j2[0] == 5) &&
                   ((unsigned char)j2[PGSIZE - 1] == 5);
  CHECK(freed_junk, "T3 kfree fills page with 0x01");
  CHECK(alloc_junk, "T3 kalloc refills page with 0x05");
  kfree(j2);

  // T5: kfree's three guard checks panic.
  struct { void *pa; const char *label; } guards[] = {
    { end + 1, "T5 misaligned address panics" },
    { (void *)((uintptr_t)end - PGSIZE), "T5 address below end panics" },
    { (void *)PHYSTOP, "T5 address >= PHYSTOP panics" },
  };
  for (int i = 0; i < 3; i++) {
    if (setjmp(panic_jmp) == 0) {
      kfree(guards[i].pa);
      CHECK(0, guards[i].label); // reached only if no panic
    } else {
      CHECK(strcmp(panic_msg, "kfree") == 0, guards[i].label);
    }
  }

  // T6: 4 threads hammer kalloc/kfree; pages are conserved.
  pthread_t th[4];
  for (int i = 0; i < 4; i++) pthread_create(&th[i], 0, thread_fn, 0);
  for (int i = 0; i < 4; i++) pthread_join(th[i], 0);
  long n2 = 0;
  while (kalloc() != 0) n2++;
  CHECK(n2 == (long)EXPECTED, "T6 threaded alloc/free conserves all 16383 pages");

  CHECK(spin_acquires == spin_releases, "lock discipline: acquires == releases");
  printf("acquires=%ld releases=%ld fails=%d\n", spin_acquires, spin_releases, fails);
  return fails != 0;
}
