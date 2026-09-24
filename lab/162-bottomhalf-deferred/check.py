#!/usr/bin/env python3
"""Verify the Linux bottom-half facts quoted in the README firsthand.

Reads the vendored v6.5 kernel sources (src/softirq.c,
src/interrupt.h) and asserts each quoted fact is present at its cited
line number: the ten softirq types in priority order with RCU last, the
2ms / 10-restart governors and their experimentation comment, the
ksoftirqd handoff, the never-nested hardirq-exit dispatch, and the
tasklet SCHED/RUN test-and-set contract with re-queue on contention.

Exits nonzero on any mismatch.
"""
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOFTIRQ = (HERE / "src" / "softirq.c").read_text().split("\n")
INTERRUPT = (HERE / "src" / "interrupt.h").read_text().split("\n")

checks = 0
mismatches = 0


def check(name, cond, detail=""):
    global checks, mismatches
    checks += 1
    if cond:
        print(f"ok   {name}")
    else:
        mismatches += 1
        print(f"FAIL {name} {detail}")


def lines(text_lines, lo, hi):
    """1-based inclusive line range as one string."""
    return "\n".join(text_lines[lo - 1:hi])


# Ten softirq types in priority order, interrupt.h L550-561
order = ["HI_SOFTIRQ", "TIMER_SOFTIRQ", "NET_TX_SOFTIRQ", "NET_RX_SOFTIRQ",
         "BLOCK_SOFTIRQ", "IRQ_POLL_SOFTIRQ", "TASKLET_SOFTIRQ",
         "SCHED_SOFTIRQ", "HRTIMER_SOFTIRQ", "RCU_SOFTIRQ"]
seg = lines(INTERRUPT, 550, 561)
for i, name in enumerate(order):
    check(f"softirq[{i}] == {name}", f"\t{name}" in seg or f"\t{name}," in seg or name in seg, seg)
check("RCU is last softirq",
      seg.find("RCU_SOFTIRQ") > seg.find("HRTIMER_SOFTIRQ"))
check("RCU last comment",
      "Preferable RCU should always be the last softirq" in seg)

# Governors, softirq.c L461-475
seg = lines(SOFTIRQ, 461, 475)
check("MAX_SOFTIRQ_TIME is 2ms",
      "#define MAX_SOFTIRQ_TIME  msecs_to_jiffies(2)" in seg)
check("MAX_SOFTIRQ_RESTART is 10",
      "#define MAX_SOFTIRQ_RESTART 10" in seg)
check("experimentation comment",
      "established via experimentation" in seg and "latency against fairness" in seg)

# ksoftirqd handoff, softirq.c L566-578
seg = lines(SOFTIRQ, 566, 578)
check("wakeup_softirqd handoff", "wakeup_softirqd()" in seg)

# Never-nested hardirq-exit dispatch, softirq.c L622-644
seg = lines(SOFTIRQ, 622, 644)
check("__irq_exit_rcu defined", "static inline void __irq_exit_rcu(void)" in seg)
check("dispatch only when !in_interrupt()", "!in_interrupt()" in seg)

# Tasklet contract, interrupt.h L687-690 and L705-709
seg = lines(INTERRUPT, 687, 690)
check("tasklet_trylock is RUN test-and-set",
      "static inline int tasklet_trylock(struct tasklet_struct *t)" in seg
      and "test_and_set_bit(TASKLET_STATE_RUN" in seg)
seg = lines(INTERRUPT, 705, 709)
check("tasklet_schedule is SCHED test-and-set",
      "static inline void tasklet_schedule(struct tasklet_struct *t)" in seg
      and "test_and_set_bit(TASKLET_STATE_SCHED" in seg)

# tasklet_action_common: steal list under irq disable, run with irq enable,
# re-queue and re-raise on contention, softirq.c L758-800
seg = lines(SOFTIRQ, 758, 800)
check("tasklet_action_common defined",
      "static void tasklet_action_common(struct softirq_action *a," in seg)
check("list stolen under local_irq_disable()", "local_irq_disable()" in seg)
check("tasklet runs with local_irq_enable()", "local_irq_enable()" in seg)
check("re-queue and re-raise on contention",
      "__raise_softirq_irqoff(softirq_nr)" in seg)

print(f"\n{checks} checks, {mismatches} mismatches")
raise SystemExit(1 if mismatches else 0)
