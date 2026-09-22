#include <stdatomic.h>
_Atomic int x, y;
void st_seq_cst(int v){ atomic_store_explicit(&x, v, memory_order_seq_cst); }
int  ld_seq_cst(void){ return atomic_load_explicit(&y, memory_order_seq_cst); }
void st_release(int v){ atomic_store_explicit(&x, v, memory_order_release); }
int  ld_acquire(void){ return atomic_load_explicit(&y, memory_order_acquire); }
