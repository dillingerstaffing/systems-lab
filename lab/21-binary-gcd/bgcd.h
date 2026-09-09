#ifndef BGCD_H
#define BGCD_H

#include <stdint.h>

/*
 * bgcd32: greatest common divisor of two 32-bit unsigned integers,
 * via Stein's binary GCD. gcd(0, 0) is defined as 0 so the function is
 * total on all inputs. No library gcd is used anywhere in this module.
 */
uint32_t bgcd32(uint32_t u, uint32_t v);

#endif /* BGCD_H */
