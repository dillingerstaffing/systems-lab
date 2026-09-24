<!-- PROOF-HEADER
Checks: 24
Mismatches: 0
Environment: Host (Linux, x86-64; python3 fact verification over vendored source)
Verdict: PASS
-->
# PROOF.md, lab/162-bottomhalf-deferred

`check.py` verifies the Linux bottom-half facts quoted in the README
firsthand against the vendored v6.5 kernel sources (`src/softirq.c`,
`src/interrupt.h`), pinning each fact to its cited line number.

24 checks, 0 mismatches: the ten softirq types in priority order (10:
HI through RCU, RCU last, with the "always be the last softirq"
comment); the two governors (2: `MAX_SOFTIRQ_TIME` is
`msecs_to_jiffies(2)`, `MAX_SOFTIRQ_RESTART` is 10, with the
experimentation comment balancing latency against fairness); the
`ksoftirqd` handoff (1: `wakeup_softirqd()` after the handler loop);
the never-nested hardirq-exit dispatch (2: `__irq_exit_rcu` defined,
dispatch only when `!in_interrupt()`); the tasklet contract (2:
`tasklet_trylock` is the RUN bit test-and-set, `tasklet_schedule` is
the SCHED bit test-and-set with schedule-once semantics); and
`tasklet_action_common` (4: defined, list stolen under
`local_irq_disable()`, tasklets run with `local_irq_enable()`,
re-queue plus `__raise_softirq_irqoff` on contention).

## Check output (genuine)
```
ok   softirq[0] == HI_SOFTIRQ
ok   softirq[1] == TIMER_SOFTIRQ
ok   softirq[2] == NET_TX_SOFTIRQ
ok   softirq[3] == NET_RX_SOFTIRQ
ok   softirq[4] == BLOCK_SOFTIRQ
ok   softirq[5] == IRQ_POLL_SOFTIRQ
ok   softirq[6] == TASKLET_SOFTIRQ
ok   softirq[7] == SCHED_SOFTIRQ
ok   softirq[8] == HRTIMER_SOFTIRQ
ok   softirq[9] == RCU_SOFTIRQ
ok   RCU is last softirq
ok   RCU last comment
ok   MAX_SOFTIRQ_TIME is 2ms
ok   MAX_SOFTIRQ_RESTART is 10
ok   experimentation comment
ok   wakeup_softirqd handoff
ok   __irq_exit_rcu defined
ok   dispatch only when !in_interrupt()
ok   tasklet_trylock is RUN test-and-set
ok   tasklet_schedule is SCHED test-and-set
ok   tasklet_action_common defined
ok   list stolen under local_irq_disable()
ok   tasklet runs with local_irq_enable()
ok   re-queue and re-raise on contention

24 checks, 0 mismatches
```
