# lab/156-pidfd-getfd

`pidfd_getfd(2)`: the syscall that reaches into another process's
file-descriptor table and takes one.

The polite way to hand a descriptor to another process is `SCM_RIGHTS`
over a Unix domain socket, and it demands two things: an established
connection and the cooperation of the process whose descriptor is being
copied. `pidfd_getfd` demands neither. It needs only a pidfd for the
target process (from `pidfd_open`) and one permission check,
`PTRACE_MODE_ATTACH_REALCREDS`, the same ptrace gate that guards
non-dumpable processes. Without that permission it answers `EPERM`.

This program proves the man page's central claim firsthand: the
duplicate refers to the *same open file description*, so the two
descriptors share file status flags and file offset.

## The experiment

1. The child creates an unlinked 20-byte tmpfile holding
   `"0123456789ABCDEFGHIJ"` (its fd 3), seeks to offset 5, and reports
   the fd number to the parent over a pipe. It then stays alive so the
   parent can reach into its table.
2. The parent calls `pidfd_open(child, 0)` (fd 4), then
   `pidfd_getfd(pidfd, 3, 0)` (fd 5).
3. The parent reads 15 bytes through fd 5 with no seek of its own and
   gets `"56789ABCDEFGHIJ"`: the child's file, at the child's offset.
   Three runs, byte-identical.

Two more man-page promises, verified in the same run:

- The returned descriptor arrives with `FD_CLOEXEC` already set
  (`fcntl(F_GETFD) == 1`).
- A `flags` argument of 1 is refused with `EINVAL`: the flags are
  reserved for a future that has not arrived yet.

## Why it matters

A file descriptor names an entry in your process's table, but the open
file description it points to belongs to no process at all.
`pidfd_getfd` is the kernel admitting it: the supervisor's primitive for
taking a descriptor, no socket, no handshake, no cooperation, just the
right to look, and the descriptor is yours, sharing the living offset
with its twin across the process boundary.

## Build and run

Linux 5.6 or newer. No glibc wrapper exists for either syscall, so both
go through `syscall(2)` directly. Run `make run` in this directory; the
genuine build log and run output are in `PROOF.md`.
