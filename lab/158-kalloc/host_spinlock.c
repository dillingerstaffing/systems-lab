// Host implementation of xv6's spinlock API (spinlock.h) for the kalloc
// workbench. The real xv6 spinlock.c uses RISC-V atomics; here a pthread
// mutex stands in so the locking discipline of kalloc.c can be exercised,
// including under threads. Counters verify acquire/release balance.
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "spinlock.h"

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
long spin_acquires = 0;
long spin_releases = 0;

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

void
acquire(struct spinlock *lk)
{
  pthread_mutex_lock(&m);
  if (lk->locked) {
    fprintf(stderr, "acquire: lock %s already held\n", lk->name);
    abort();
  }
  lk->locked = 1;
  spin_acquires++;
  // keep the mutex held for the critical section
}

void
release(struct spinlock *lk)
{
  if (!lk->locked) {
    fprintf(stderr, "release: lock %s not held\n", lk->name);
    abort();
  }
  lk->locked = 0;
  spin_releases++;
  pthread_mutex_unlock(&m);
}
