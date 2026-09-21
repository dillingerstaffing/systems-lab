#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

/*
 * sync_file_range demo: fsync syncs the whole file; this syscall syncs a
 * byte range, and the flags split the write into start and wait phases.
 *
 * T1  flags=0 is a documented no-op -> returns 0.
 * T2  an invalid flag bit -> EINVAL (error path is real).
 * T3  a pipe fd -> ESPIPE (only regular files, block devices, dirs).
 * T4  WRITE alone starts writeback of dirty pages and returns (async).
 * T5  WAIT_BEFORE|WRITE|WAIT_AFTER on a middle 4 MiB sub-range: the full
 *     write-for-data-integrity combo, range-scoped, returns 0.
 * T6  timing: range combo vs fsync() on the same dirty file, 3 iterations.
 *
 * Honest boundaries: no cache dropping (unprivileged), shared 2-vCPU VM,
 * so timings are ratios, not a datasheet. Durability itself is not
 * crash-tested; the man page's own warning stands (no metadata sync, no
 * disk-cache flush, no guarantees except pure overwrites).
 */

static long ns_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void die(const char *what)
{
    perror(what);
    exit(1);
}

/* Fill the file with fresh dirty pages each iteration. */
static void dirty(int fd, size_t len)
{
    static char buf[1 << 20];
    memset(buf, 0xA5, sizeof buf);
    if (lseek(fd, 0, SEEK_SET) < 0)
        die("lseek");
    for (size_t off = 0; off < len; off += sizeof buf) {
        ssize_t w = write(fd, buf, sizeof buf);
        if (w != (ssize_t)sizeof buf)
            die("write");
        buf[0] ^= (char)(off & 0xff); /* keep pages distinct */
    }
}

int main(void)
{
    const char *path = "data.bin";
    const size_t LEN = 8 << 20; /* 8 MiB */
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        die("open");

    /* T1: flags 0 is a permitted no-op. */
    errno = 0;
    if (sync_file_range(fd, 0, 0, 0) != 0)
        die("T1 flags=0");
    printf("T1 flags=0 no-op: returned 0\n");

    /* T2: invalid flag bit -> EINVAL. */
    errno = 0;
    if (sync_file_range(fd, 0, 0, 1u << 31) == 0) {
        printf("T2 UNEXPECTED: invalid flag accepted\n");
    } else if (errno == EINVAL) {
        printf("T2 invalid flag bit: EINVAL as documented\n");
    } else {
        printf("T2 UNEXPECTED errno=%d\n", errno);
    }

    /* T3: pipe fd -> ESPIPE. */
    int p[2];
    if (pipe(p) != 0)
        die("pipe");
    errno = 0;
    if (sync_file_range(p[0], 0, 0, SYNC_FILE_RANGE_WRITE) == 0) {
        printf("T3 UNEXPECTED: pipe accepted\n");
    } else if (errno == ESPIPE) {
        printf("T3 pipe fd: ESPIPE as documented\n");
    } else {
        printf("T3 UNEXPECTED errno=%d\n", errno);
    }
    close(p[0]);
    close(p[1]);

    /* T4: WRITE alone = async start, returns without waiting. */
    dirty(fd, LEN);
    long t0 = ns_now();
    if (sync_file_range(fd, 0, 0, SYNC_FILE_RANGE_WRITE) != 0)
        die("T4 WRITE");
    long t1 = ns_now();
    if (sync_file_range(fd, 0, 0, SYNC_FILE_RANGE_WAIT_AFTER) != 0)
        die("T4 WAIT_AFTER");
    long t2 = ns_now();
    printf("T4 WRITE-only (async start): %ld us; WAIT_AFTER (wait): %ld us\n",
           (t1 - t0) / 1000, (t2 - t1) / 1000);

    /* T5: full integrity combo, scoped to the middle 4 MiB. */
    dirty(fd, LEN);
    t0 = ns_now();
    if (sync_file_range(fd, 2 << 20, 4 << 20,
                        SYNC_FILE_RANGE_WAIT_BEFORE |
                        SYNC_FILE_RANGE_WRITE |
                        SYNC_FILE_RANGE_WAIT_AFTER) != 0)
        die("T5 range combo");
    t1 = ns_now();
    printf("T5 WAIT_BEFORE|WRITE|WAIT_AFTER on bytes [2MiB,6MiB): ok, %ld us\n",
           (t1 - t0) / 1000);

    /* T6: range combo vs fsync on the same dirty file. */
    for (int i = 0; i < 3; i++) {
        dirty(fd, LEN);
        t0 = ns_now();
        if (sync_file_range(fd, 0, 0,
                            SYNC_FILE_RANGE_WAIT_BEFORE |
                            SYNC_FILE_RANGE_WRITE |
                            SYNC_FILE_RANGE_WAIT_AFTER) != 0)
            die("T6 range");
        t1 = ns_now();
        dirty(fd, LEN);
        long s0 = ns_now();
        if (fsync(fd) != 0)
            die("T6 fsync");
        long s1 = ns_now();
        printf("T6 iter %d: full-range combo %ld us, fsync %ld us\n",
               i, (t1 - t0) / 1000, (s1 - s0) / 1000);
    }

    close(fd);
    printf("ALL TESTS PASSED\n");
    return 0;
}
