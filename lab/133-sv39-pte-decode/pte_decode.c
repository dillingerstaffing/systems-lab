#include "pte_decode.h"

pte_fields_t pte_decode(uint64_t pte) {
    pte_fields_t f;
    f.v = (uint8_t)(pte & 1ULL);
    f.r = (uint8_t)((pte >> 1) & 1ULL);
    f.w = (uint8_t)((pte >> 2) & 1ULL);
    f.x = (uint8_t)((pte >> 3) & 1ULL);
    f.u = (uint8_t)((pte >> 4) & 1ULL);
    f.g = (uint8_t)((pte >> 5) & 1ULL);
    f.a = (uint8_t)((pte >> 6) & 1ULL);
    f.d = (uint8_t)((pte >> 7) & 1ULL);
    f.rsw = (uint8_t)((pte >> 8) & 3ULL);
    f.ppn = (pte >> 10) & 0xFFFFFFFFFFFULL; /* 44 bits, bits 53:10 */
    return f;
}

uint64_t pte_encode(pte_fields_t f) {
    return ((f.ppn & 0xFFFFFFFFFFFULL) << 10) |
           ((uint64_t)(f.rsw & 3u) << 8) |
           ((uint64_t)(f.d & 1u) << 7) |
           ((uint64_t)(f.a & 1u) << 6) |
           ((uint64_t)(f.g & 1u) << 5) |
           ((uint64_t)(f.u & 1u) << 4) |
           ((uint64_t)(f.x & 1u) << 3) |
           ((uint64_t)(f.w & 1u) << 2) |
           ((uint64_t)(f.r & 1u) << 1) |
           (uint64_t)(f.v & 1u);
}
