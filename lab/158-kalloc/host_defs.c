// Host implementation of panic() (declared in xv6 defs.h) for the kalloc
// workbench. Instead of killing the machine, it longjmps back to the
// test harness so the guard checks in kfree() can be tested.
#include <setjmp.h>
#include <stdio.h>
#include "types.h"

jmp_buf panic_jmp;
char panic_msg[128];

void
panic(char *s)
{
  snprintf(panic_msg, sizeof(panic_msg), "%s", s);
  longjmp(panic_jmp, 1);
}
