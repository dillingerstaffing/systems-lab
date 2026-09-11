#ifndef DECIMAL_DIGITS_H
#define DECIMAL_DIGITS_H

#include <stdint.h>

/* Number of decimal digits of x (1 for x == 0), as a straight-line
 * comparison cascade: 1 + (x >= 10) + (x >= 100) + ... + (x >= 10^19).
 * Each comparison is a magnitude truth about x; the implementation uses
 * no loop, no division, no table, and no string formatting.
 * Returns a value in [1, 20]. */
uint8_t digit_count(uint64_t x);

#endif /* DECIMAL_DIGITS_H */
