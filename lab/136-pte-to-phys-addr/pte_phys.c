#include "pte_phys.h"

uint64_t phys_addr_from_ppn(uint64_t ppn, uint64_t offset) {
    return ((ppn & 0xFFFFFFFFFFFULL) << 12) | (offset & 0xFFFULL);
}
