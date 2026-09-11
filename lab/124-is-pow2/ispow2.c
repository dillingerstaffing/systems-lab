#include "ispow2.h"

int is_pow2(uint64_t x) {
    return (x != 0) & ((x & (x - 1)) == 0);
}
