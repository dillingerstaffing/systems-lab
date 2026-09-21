<!-- PROOF-HEADER
Checks: 36
Mismatches: 0
Checksum: 0758fc488309e4b4b89164a1dffd2ff31305879252dc486f677dcb978371b504
Environment: Host (Linux 7.0.0-38-generic, x86-64, gcc 13.3.0)
Verdict: PASS
-->
# PROOF.md, lab/160-sync-file-range

`syncfilerange.c` proves the `sync_file_range(2)` contract firsthand: the
syscall syncs a byte range, not a file, and its flags split writeback into
start and wait phases.

36 checks, 0 mismatches: 3 runs x 12 assertions each (flags=0 no-op returns
0; invalid flag bit returns EINVAL; pipe fd returns ESPIPE; WRITE alone
returns after starting writeback while a separate WAIT_AFTER waits;
WAIT_BEFORE|WRITE|WAIT_AFTER on bytes [2MiB,6MiB) returns 0; full-range
integrity combination vs fsync on the same dirty file, 3 iterations, the
range combination consistently faster because fsync also pays metadata and
journal). The checksum is the SHA-256 of the three run logs with every
digit run normalized to `#`, because microsecond timings vary run to run on
a shared VM while every return code, errno, and ordering assertion is
stable. Clean under `-std=c11 -O2 -Wall -Wextra -Werror`.

## Build log (genuine)

```
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o sfr syncfilerange.c
build: clean, no warnings
```

## Run output (genuine, run 1; timings vary run to run)

```
T1 flags=0 no-op: returned 0
T2 invalid flag bit: EINVAL as documented
T3 pipe fd: ESPIPE as documented
T4 WRITE-only (async start): 12849 us; WAIT_AFTER (wait): 22462 us
T5 WAIT_BEFORE|WRITE|WAIT_AFTER on bytes [2MiB,6MiB): ok, 16990 us
T6 iter 0: full-range combo 18504 us, fsync 25417 us
T6 iter 1: full-range combo 12959 us, fsync 22387 us
T6 iter 2: full-range combo 12432 us, fsync 30954 us
ALL TESTS PASSED
```

Honest boundaries: no cache dropping (unprivileged), shared 2-vCPU VM, so
timings are ratios, not a datasheet. Durability was not crash-tested; the
man page's "extremely dangerous" warning (no metadata, no disk-cache flush,
no guarantees except pure overwrites of instantiated blocks) is quoted, not
re-proven. Scratch file `data.bin` (8 MiB) removed after the runs.
