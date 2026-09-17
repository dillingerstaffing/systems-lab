#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/pidfd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

/* lab/156-pidfd-getfd: pidfd_getfd(2), firsthand.
 *
 * The parent reaches into the child's file-descriptor table and takes one.
 * The child opens an unlinked 20-byte tmpfile holding "0123456789ABCDEFGHIJ"
 * (its fd 3), seeks to offset 5, and reports the fd number over a pipe. The
 * parent opens a pidfd for the child and calls pidfd_getfd(2) to duplicate
 * the descriptor into its own table. The proof of the shared open file
 * description: the parent reads 15 bytes and gets "56789ABCDEFGHIJ", the
 * child's file at the child's offset, with no seek of its own.
 *
 * Linux 5.6+, no glibc wrapper: both syscalls go through syscall(2). */

static long pidfd_open2(pid_t pid, unsigned flags) {
    return syscall(SYS_pidfd_open, pid, flags);
}
static long pidfd_getfd2(int pidfd, int targetfd, unsigned flags) {
    return syscall(SYS_pidfd_getfd, pidfd, targetfd, flags);
}

int main(void) {
    int p[2];
    if (pipe(p) != 0) { perror("pipe"); return 1; }

    pid_t c = fork();
    if (c < 0) { perror("fork"); return 1; }

    if (c == 0) { /* child */
        close(p[0]);
        char tmpl[] = "/tmp/pgetXXXXXX";
        int tfd = mkstemp(tmpl);
        if (tfd < 0) { perror("child mkstemp"); _exit(1); }
        unlink(tmpl);
        const char *content = "0123456789ABCDEFGHIJ";
        if (write(tfd, content, 20) != 20) { perror("child write tmp"); _exit(1); }
        int fd = tfd;
        if (fd < 0) { perror("child open"); _exit(1); }
        /* leave the offset mid-file: the parent's read must land here */
        if (lseek(fd, 5, SEEK_SET) != 5) { perror("child lseek"); _exit(1); }
        if (write(p[1], &fd, sizeof fd) != sizeof fd) { perror("child write"); _exit(1); }
        /* stay alive so the parent can reach into our table */
        pause();
        _exit(0);
    }

    /* parent */
    close(p[1]);
    int childfd;
    if (read(p[0], &childfd, sizeof childfd) != sizeof childfd) { perror("parent read"); return 1; }
    printf("T1 child reports its tmpfile fd = %d\n", childfd);

    long pfd = pidfd_open2(c, 0);
    printf("T2 pidfd_open(child): %ld errno=%d\n", pfd, errno);
    if (pfd < 0) return 1;

    errno = 0;
    long stolen = pidfd_getfd2((int)pfd, childfd, 0);
    printf("T3 pidfd_getfd(pidfd, %d): %ld errno=%d (%s)\n",
           childfd, stolen, errno, strerror(errno));
    if (stolen < 0) return 1;

    /* the shared-description proof: child left offset at 5; read via stolen fd */
    char buf[64]; memset(buf, 0, sizeof buf);
    ssize_t n = read((int)stolen, buf, sizeof buf - 1);
    printf("T4 read via stolen fd: n=%zd bytes=%s", n, buf);
    int ok = (n == 15 && memcmp(buf, "56789ABCDEFGHIJ", 15) == 0);
        printf("T4 %s\n", ok ? "ok: parent reads the child's file at the child's offset (shared description)" : "UNEXPECTED");

    /* T4b: the man page promises FD_CLOEXEC on the returned fd */
    int gfd = fcntl((int)stolen, F_GETFD);
    printf("T4b fcntl(F_GETFD) on stolen fd: %d (FD_CLOEXEC=%d) %s\n",
           gfd, FD_CLOEXEC, (gfd == FD_CLOEXEC) ? "ok: close-on-exec set" : "UNEXPECTED");

    /* T5: flags must be 0 */
    errno = 0;
    long bad = pidfd_getfd2((int)pfd, childfd, 1);
    printf("T5 pidfd_getfd flags=1: %ld errno=%d (%s)\n", bad, errno, strerror(errno));

    kill(c, SIGKILL);
    waitpid(c, NULL, 0);
    return 0;
}
