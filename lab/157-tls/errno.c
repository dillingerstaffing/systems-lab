/* lab/157-tls: errno is thread-local too.
 *
 * The most famous __thread variable in C is one you never declared:
 * errno. Each thread gets its own, so one thread's EIO does not leak
 * into another thread's clean slate.
 */
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

static int failures = 0;

#define CHECK(name, cond) do {                                        \
    if (cond) { printf("ok: " name "\n"); }                           \
    else { printf("FAIL: " name "\n"); failures++; }                  \
} while (0)

struct obs {
    void *main_addr;
    void *worker_addr;
    int worker_errno;
    int main_errno;
};

static void *worker(void *arg) {
    struct obs *o = arg;
    errno = 5; /* EIO, in this thread only */
    o->worker_addr = &errno;
    o->worker_errno = errno;
    printf("worker: &errno = %p, errno = %d\n", (void *)&errno, errno);
    return NULL;
}

int main(void) {
    struct obs o = {0};
    pthread_t t;
    errno = 0;
    o.main_addr = &errno;
    printf("main:   &errno = %p\n", (void *)&errno);
    pthread_create(&t, NULL, worker, &o);
    pthread_join(t, NULL);
    o.main_errno = errno;
    printf("main:   after join, errno = %d (untouched by the worker)\n", errno);

    CHECK("T1 errno-addrs-differ", o.main_addr != o.worker_addr);
    CHECK("T2 worker-errno-5", o.worker_errno == 5);
    CHECK("T3 main-errno-0", o.main_errno == 0);
    return failures != 0;
}
