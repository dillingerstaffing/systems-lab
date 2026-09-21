# lab/160-sync-file-range

`sync_file_range(2)`: the sync that names a byte range.

`fsync()` makes a promise about a whole file: every dirty page, plus the
metadata and the journal. `sync_file_range(fd, offset, nbytes, flags)` makes
a promise about a byte range instead, and its flags split the promise into
"start" and "done." The range is page-grained (the offset rounds down, the
end rounds up), and `nbytes = 0` means offset through end of file.

The three flags compose into documented operations:

- `SYNC_FILE_RANGE_WRITE` alone starts writeback of the range's dirty pages
  and returns immediately. It is explicitly not for data integrity.
- `WAIT_BEFORE | WRITE` waits for already-submitted writeback, then starts
  the range's write. This is the start-for-integrity operation.
- All three flags together (`WAIT_BEFORE | WRITE | WAIT_AFTER`) is the
  range's write-for-data-integrity operation.

This lab proves the contract firsthand in `syncfilerange.c`:

- T1: `flags = 0` is a documented no-op and returns 0.
- T2: an invalid flag bit returns `EINVAL`.
- T3: a pipe file descriptor returns `ESPIPE` (regular files, block
  devices, and directories only).
- T4: on 8 MiB of dirty pages, `WRITE` alone returns after only starting
  writeback (async), and a separate `WAIT_AFTER` then waits for it. Start
  and done are visibly two calls.
- T5: the full `WAIT_BEFORE|WRITE|WAIT_AFTER` combination, scoped to the
  middle 4 MiB (bytes `[2MiB, 6MiB)`), returns 0. Integrity for a
  sub-range, not the file.
- T6: the full-range combination against `fsync()` on the same dirty file,
  3 iterations. The range combination is consistently faster because
  `fsync` also pays for metadata and the journal, which the range call
  never touches.

Why it exists: a database overwriting a hot 4 MiB region of a 40 GiB file
does not want to drag every cold page along or pay the metadata tax, and
splitting start from wait lets writeback be pipelined (start it, do other
work, wait later). The alternatives do not fit: `O_SYNC` makes every write
synchronous with no batching, `msync` is for memory mappings, and `sync()`
is the sledgehammer that syncs the whole system.

Honest boundaries: the man page calls this call "extremely dangerous" and
the warning stands as written. It writes no metadata, gives no guarantees
unless you are purely overwriting already-instantiated blocks (which you
cannot verify from userspace, and which is impossible on copy-on-write
filesystems), and it does not flush disk write caches. Durability was not
crash-tested here. Timings were taken on a shared 2-vCPU VM without cache
dropping (unprivileged), so they are ratios, not a datasheet; the return
codes and errnos were identical across all runs.

Build: `gcc -std=c11 -O2 -Wall -Wextra -Werror`, clean. Mechanism verified
against man-pages 6.19 (`man7.org`), `sync_file_range(2)`.
