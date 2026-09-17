<!-- PROOF-HEADER
Checks: 18
Mismatches: 0
Checksum: d1d9fed7b9bb5f208efb3850f20b0ce8c58998e2648e84959bc88e85d508164c
Environment: Host (Linux 7.0.0-26-generic, x86-64, gcc 13.3.0, glibc 2.39)
Verdict: PASS
-->
# PROOF.md, lab/156-pidfd-getfd

`pget.c` proves the man page's central claim about `pidfd_getfd(2)`
firsthand: the duplicate refers to the *same open file description*,
so parent and child share file status flags and file offset.

18 checks, 0 mismatches: 3 runs x 6 assertions each (fd numbers land
where expected, 15 bytes read, bytes equal `"56789ABCDEFGHIJ"`,
`FD_CLOEXEC` set on the returned descriptor, `flags=1` refused with
`EINVAL`, clean exit 0). The checksum is the SHA-256 of three
consecutive program outputs; all three runs are byte-identical.

Clean under `-std=c11 -O2 -Wall -Wextra -Werror`, `-O0`, and
`-fsanitize=address,undefined`, all with zero reports.

## Build log (genuine)

```
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o pget pget.c
build: clean, no warnings
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o pget_asan pget.c
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o pget_o0 pget.c
```

## Run output, 3 runs at -O2 (genuine, byte-identical)

```
T1 child reports its tmpfile fd = 3
T2 pidfd_open(child): 4 errno=0
T3 pidfd_getfd(pidfd, 3): 5 errno=0 (Success)
T4 read via stolen fd: n=15 bytes=56789ABCDEFGHIJT4 ok: parent reads the child's file at the child's offset (shared description)
T4b fcntl(F_GETFD) on stolen fd: 1 (FD_CLOEXEC=1) ok: close-on-exec set
T5 pidfd_getfd flags=1: -1 errno=22 (Invalid argument)
```

Each of the three runs printed exactly the lines above and exited 0.
The ASan+UBSan build and the -O0 build printed the same T1-T5 lines
with the same verdicts and exited 0.

## Honest boundaries

- `PTRACE_MODE_ATTACH_REALCREDS` was not exercised as a failure case:
  parent and child here share a uid, so the gate passes silently. The
  `EPERM` half is stated from the man page (man-pages 6.19), not
  re-tested.
- This verifies Linux's `pidfd_getfd` implementation on this kernel
  (7.0.0-26-generic), not the abstract contract on every kernel.
- `SYS_pidfd_open` / `SYS_pidfd_getfd` have no glibc wrappers; both go
  through `syscall(2)` directly.
