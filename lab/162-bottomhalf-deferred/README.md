# lab/162-bottomhalf-deferred

Linux defers interrupt work in layers, and this lab reads the mechanism
straight from the Linux v6.5 kernel source, vendored under `src/`. A
hardirq handler runs with interrupts off and no process to its name, so
it does the minimum and hands the rest to the bottom half. The bottom
half is a ladder: the hardirq does the minimum, softirqs (and tasklets
built on them) take the urgent remainder that still cannot sleep, and
the workqueue takes the part that needs to sleep. Each rung exists
because the rung above it is missing exactly one property.

Facts read firsthand from the vendored files, with line numbers
(`check.py` verifies each one):

- `interrupt.h` L550-561: ten softirq types in priority order: HI,
  TIMER, NET_TX, NET_RX, BLOCK, IRQ_POLL, TASKLET, SCHED, HRTIMER, RCU.
  The comment on the last entry: "Preferable RCU should always be the
  last softirq".
- `softirq.c` L461-475: the handler loop's two governors:
  `MAX_SOFTIRQ_TIME = msecs_to_jiffies(2)` and
  `MAX_SOFTIRQ_RESTART = 10`. The comment says these limits were
  established via experimentation, balancing latency against fairness.
- `softirq.c` L566-578: after the handler loop, if pending work remains
  and the 2ms / 10-restart budgets are spent (or `need_resched`),
  `wakeup_softirqd()` hands the work to the per-CPU `ksoftirqd` thread.
- `softirq.c` L622-644 (`__irq_exit_rcu`): softirqs are invoked at
  hardirq exit only when `!in_interrupt()`, never nested.
- `interrupt.h` L687-690: `tasklet_trylock` returns
  `!test_and_set_bit(TASKLET_STATE_RUN, ...)`: the same tasklet can
  never execute on two CPUs at once.
- `interrupt.h` L705-709: `tasklet_schedule` sets
  `TASKLET_STATE_SCHED` with `test_and_set_bit`, so scheduling a
  tasklet twice collapses into a single run.
- `softirq.c` L758-800 (`tasklet_action_common`): the per-CPU tasklet
  list is stolen under `local_irq_disable()`, then each tasklet runs
  with `local_irq_enable()` held; if the trylock fails or `count` is
  nonzero, the tasklet is re-queued and the softirq is re-raised, so
  the work is honored later instead of being lost.

Vendored from `torvalds/linux` at tag `v6.5`:
`kernel/softirq.c` (999 lines) and `include/linux/interrupt.h`
(834 lines). Nothing else was changed; `check.py` pins the quoted
facts to their line numbers so the evidence stays checkable.

The history (two agreeing sources, paraphrased): the old bottom halves
numbered 32, ran globally synchronized, and only one was allowed to run
anywhere in the system at a time. During 2.5 the BH interface was thrown
away; every user moved to softirqs, tasklets, or workqueues.
