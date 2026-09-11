#ifndef ORACLE_STEP_H
#define ORACLE_STEP_H

#include <stdint.h>

/*
 * Independent oracle for pte_step, written as a deliberately different
 * code path: the PTE is exploded into a 64-element bit array by repeated
 * division (no shifts, no masks, no bitwise operators anywhere in the
 * extraction), the PPN is rebuilt by positional accumulation, and the
 * spec walk steps are applied in spec order with plain integer tests.
 * Shares no code and no constants with pte_step.c; the only common
 * input is the spec text both were written from.
 */

typedef enum {
    ORACLE_FAULT = 0,
    ORACLE_DESCEND = 1,
    ORACLE_LEAF = 2
} oracle_verdict_t;

typedef struct {
    oracle_verdict_t verdict;
    uint64_t next_ppn;
    uint64_t leaf_phys;
} oracle_result_t;

oracle_result_t oracle_step(uint64_t pte, int level);

#endif /* ORACLE_STEP_H */
