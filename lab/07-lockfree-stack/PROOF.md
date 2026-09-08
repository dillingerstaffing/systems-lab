# PROOF.md — lab/07-lockfree-stack

## Build

```
$ make clean && make
gcc -std=c11 -O2 -Wall -Wextra -Werror -pthread -o test_treiber test_treiber.c treiber.c
exit=0
```

Clean under `-Wall -Wextra -Werror`. The binary contains 6 `lock cmpxchg16b`
sites (verified with objdump); no 16-byte atomic goes through libatomic.

## Run

```
$ ./test_treiber
== lab/07 lock-free Treiber stack ==
cpuid: CMPXCHG16B present
sanity: LIFO order 3,2,1 ok, empty pop returns NULL
unit: ABA rejection ok (stale tag 1 vs head tag 4, same pointer 0x7ffeb5579c50)
threads=4 nodes=262144
phase1: concurrent push         0.017 s    15.21 Mops/s  cas_fail=24336 (+24336)  aba_caught=0 (+0)
phase2: pop/push churn          0.259 s     6.18 Mops/s  cas_fail=397197 (+372861)  aba_caught=28150 (+28150)
phase3: parallel drain          0.053 s     4.96 Mops/s  cas_fail=397714 (+517)  aba_caught=28150 (+0)
audit: pushed=262144 popped=262144 lost=0 duplicated=0 bad_id=0 bad_canary=0 stack_empty=yes
total: cas_fail=397714 aba_caught=28150
drain distribution per thread: t0=93888 t1=44461 t2=85619 t3=38176
PASS: conservation holds, no loss/duplication/corruption
exit=0
```

(The `cas_fail`/`aba_caught` columns show the cumulative total with the
phase delta in parentheses. The run above is the captured run; two earlier
runs also passed with 5614 and 1459 tag rejections respectively. Contention
counts vary with scheduling on this 2-core machine; conservation held in
every run.)

`make sanitize` (AddressSanitizer + UBSan) and `make opt0` also build and
pass with the same conservation result.

## Why these numbers mean the mechanism works

- **Atomicity.** The head is one 16-byte value and the only writer is
  `lock cmpxchg16b`. The instruction compares and swaps the full 16 bytes
  indivisibly, so a torn head is impossible by construction, not by testing.
- **ABA defense, deterministically.** The unit test recycles node A through
  the head (pop A, push B, push A) while holding the earlier observation
  `(A, tag 1)`, then applies the stale update by hand. The pointers match
  exactly, the tag differs (1 vs 4), and the CAS is rejected; the stack
  then still pops A, B, NULL. A tagless CAS would have succeeded here and
  corrupted the list.
- **ABA defense, statistically.** In the churn phase, 4 threads recycle
  nodes through the head 1.6M times; 28150 CAS attempts failed with the
  pointer unchanged but the tag moved, i.e. 28150 stale observations the
  tag rejected. x86-64 cmpxchg has no spurious failures, so each is a
  genuine rejection.
- **Ordering.** Push's plain stores to the node are sequenced before its
  CAS; pop's plain loads of `node->next` and the payload are sequenced
  after its head read. The locked CAS is a full barrier on both sides, so
  the required release (publish) and acquire (observe) edges hold; the
  release-sequence rules extend them across any number of intervening
  CASes. A failed CAS publishes nothing.
- **Conservation.** 262144 nodes pushed across 4 threads, 262144 popped,
  each id observed exactly once, every canary intact, stack empty at the
  end. Nodes come from a fixed pool and are never freed, so reclamation is
  excluded by construction and the audit isolates the stack algorithm.

## Notes

- libatomic on this toolchain reports 16-byte atomics as not lock-free
  (`__atomic_is_lock_free(16, 0) == 0`) despite the CPU offering
  CMPXCHG16B, so the module issues the instruction directly rather than
  routing the algorithm's core operation through an opaque runtime.
- No ThreadSanitizer target: TSan cannot instrument inline assembly and
  would flag the head as a race (false positive). The atomicity claim
  rests on the ISA instead.
