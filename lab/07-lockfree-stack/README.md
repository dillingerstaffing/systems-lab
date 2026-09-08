# lab/07-lockfree-stack

Treiber stack in C11 with an ABA counter, built on the hardware
double-width compare-and-swap.

- `treiber.h` / `treiber.c`
  - Lock-free LIFO. The shared head is one 16-byte `(pointer, tag)` value;
    the only operation that touches it is `lock cmpxchg16b`, issued through
    inline assembly, so the atomicity rests on the instruction the CPU
    actually executes. No libatomic: on this toolchain libatomic reports
    16-byte atomics as not lock-free, so the module drives the ISA
    primitive directly instead of an opaque runtime path.
  - The tag is a 64-bit counter incremented by every successful CAS. A
    thread that reads the head, gets preempted while other threads pop and
    re-push the same node, cannot succeed its CAS on the recycled pointer:
    the tag has moved. x86-64 only (requires CMPXCHG16B; the harness checks
    CPUID before running).
  - Memory ordering is documented at the primitives in `treiber.h`:
    `lock cmpxchg16b` is a full barrier, which covers the release a push
    needs (publishing `node->next` and the payload) and the acquire a pop
    needs (observing them). Failed CASes publish nothing and need no
    ordering.
- `test_treiber.c`
  - Single-threaded LIFO order check and empty-pop check.
  - Deterministic ABA test: a stale `(pointer, tag)` observation is applied
    by hand after the node is recycled; the CAS is rejected while the
    pointer matches, and the stack is undisturbed.
  - 4-thread stress: 262144 distinct nodes pushed concurrently, 200000
    pop/push churn rounds per thread, then a parallel drain. Every node
    carries an id and a canary; the audit requires each id popped exactly
    once and every canary intact. Measured in the captured run below:
    262144 pushed, 262144 popped, 0 lost, 0 duplicated, 0 corrupted, and
    28150 stale observations rejected by the tag.

Build: `make` (`-Wall -Wextra -Werror` clean), run: `make run`,
`make sanitize` (ASan+UBSan), `make opt0`. Genuine build log and output
are in `PROOF.md`.
