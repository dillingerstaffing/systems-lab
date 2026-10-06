// amoadd-refcount: the add that returns the old value, applied as a
// lock-free reference-count drop. M-mode bare metal on the QEMU virt board.
#include "uart.h"

// One word holding the reference count. amoadd.w needs natural alignment.
static volatile unsigned int refcount __attribute__((aligned(4)));

// Drop one reference. Returns the count BEFORE this drop.
// If it returns 1, this drop took the count to zero: the caller frees.
static unsigned int rc_drop(volatile unsigned int *p) {
    unsigned int old;
    __asm__ volatile ("amoadd.w %0, %2, (%1)"
                      : "=r" (old) : "r" (p), "r" (-1) : "memory");
    return old;
}

static unsigned int rc_acquire(volatile unsigned int *p) {
    unsigned int old;
    __asm__ volatile ("amoadd.w %0, %2, (%1)"
                      : "=r" (old) : "r" (p), "r" (1) : "memory");
    return old;
}

// The naive version, for the listing contrast only.
unsigned int naive_drop(volatile unsigned int *p) {
    unsigned int old = *p;
    *p = old - 1;
    return old;
}

static void show(const char *op, unsigned int old, unsigned int now) {
    uart_puts(op);
    uart_puts(": old=");
    uart_put_dec(old);
    uart_puts(" now=");
    uart_put_dec(now);
    uart_puts("\n");
}

int main(void) {
    uart_init();
    refcount = 3;
    uart_puts("count=3\n");
    unsigned int o;
    o = rc_drop(&refcount);    show("drop", o, refcount);
    o = rc_drop(&refcount);    show("drop", o, refcount);
    o = rc_drop(&refcount);    show("drop", o, refcount);
    uart_puts(o == 1 ? "last holder frees\n" : "ERROR\n");
    o = rc_acquire(&refcount); show("acquire", o, refcount);
    uart_puts("DONE\n");
    return 0;
}
