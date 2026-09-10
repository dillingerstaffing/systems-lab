#ifndef BCD_DIGIT_VALID_H
#define BCD_DIGIT_VALID_H

#include <stdint.h>

/*
 * bcd_invalid_mask: given a 16-bit word w holding four packed BCD
 * nibbles, returns a 4-bit mask where bit i is 1 iff nibble i
 * (nibble 0 = bits 0..3, the least significant) holds a value in
 * 10..15, i.e. is not a valid BCD digit.
 *
 * Construction. The add-guard identity: for a nibble value n in
 * 0..15, n + 6 >= 16 iff n >= 10. So adding 6 to a nibble carries out
 * of the nibble exactly when the nibble is invalid.
 *
 * The hazard is inter-nibble carry aliasing: in the naive form
 * w + 0x6666 a carry generated at a low invalid nibble propagates
 * through higher valid nibbles and falsely flags them. It is defeated
 * by adding 6 to even and odd nibbles in two separate masked adds:
 *
 *   t_even = (w & 0x0F0F) + 0x0606   (lanes: nibbles 0 and 2)
 *   t_odd  = (w & 0xF0F0) + 0x6060   (lanes: nibbles 1 and 3)
 *
 * In each add, the two active lanes sit 8 bits apart, and every bit
 * between them is 0 in both addends. A binary full adder with both
 * addend bits 0 kills any incoming carry (c_out = 0 whatever c_in),
 * so a generated carry dies at the first bit past its lane and the
 * carry into every lane is provably 0. No carry can ever reach another
 * nibble's lane.
 *
 * Each nibble's generated carry (carry into bit k of a+b) is read with
 * the identity c_k = t_k ^ a_k ^ b_k, which reduces to the single sum
 * bit t_k because the two addend bits at each lane boundary are 0:
 *
 *   mask bit 0 = bit 4  of t_even (nibble 0's carry out)
 *   mask bit 1 = bit 8  of t_odd  (nibble 1's carry out)
 *   mask bit 2 = bit 12 of t_even (nibble 2's carry out)
 *   mask bit 3 = bit 16 of t_odd  (nibble 3's carry out, 32-bit add)
 *
 * The full derivation, with every mask and constant bit position
 * verified by hand, is in PROOF.md.
 */
uint8_t bcd_invalid_mask(uint16_t w);

#endif
