/* lab/157-tls: one variable, two threads, two addresses.
 *
 * A __thread variable has the same name in every thread but different
 * storage in each. The program prints the raw addresses (for the reader)
 * and then runs four assertions on the behavior (for the harness):
 *   T1 the two threads see different addresses for the same name
 *   T2 both threads read the initializer (7) before any store
 *   T3 the worker's store lands in the worker's copy (42)
 *   T4 the worker's store does not touch main's copy (still 7)
 */
#include <stdio.h>
#include <pthread.h>

__thread int counter = 7;   /* one per thread, same name, different storage */

static int failures = 0;

#define CHECK(name, cond) do {                                        \
    if (cond) { printf("ok: " name "\n"); }                           \
    else { printf("FAIL: " name "\n"); failures++; }                  \
} while (0)

struct obs {
    void *main_addr;
    void *worker_addr;
    int main_before;
    int worker_before;
    int worker_after;
    int main_after;
};

static void *worker(void *arg) {
    struct obs *o = arg;
    o->worker_addr = &counter;
    o->worker_before = counter;
    printf("worker: &counter = %p, counter = %d\n", (void *)&counter, counter);
    counter = 42;
    o->worker_after = counter;
    printf("worker: after store, counter = %d\n", counter);
    return NULL;
}

int main(void) {
    struct obs o = {0};
    pthread_t t;
    o.main_addr = &counter;
    o.main_before = counter;
    printf("main:   &counter = %p, counter = %d\n", (void *)&counter, counter);
    pthread_create(&t, NULL, worker, &o);
    pthread_join(t, NULL);
    o.main_after = counter;
    printf("main:   after join, counter = %d\n", counter);

    CHECK("T1 addrs-differ", o.main_addr != o.worker_addr);
    CHECK("T2 initial-values-7", o.main_before == 7 && o.worker_before == 7);
    CHECK("T3 worker-store-42", o.worker_after == 42);
    CHECK("T4 main-still-7", o.main_after == 7);
    return failures != 0;
}
