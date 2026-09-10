# lab/53-double-dabble

`dabble_to_bcd` (dabble.h): converts an 8-bit value (0..255) to a
3-digit packed BCD in a uint16_t, bits 11..8 = hundreds, bits 7..4 =
tens, bits 3..0 = ones; e.g. 255 becomes 0x255.

Method: the shift-and-add-3 identity, implemented from bit
fundamentals only. One 32-bit shift register holds a three-nibble BCD
scratch area (bits 19..8) plus the input bits still to fold in (bits
7..0). For each input bit, most significant first: if a BCD nibble
holds a digit d >= 5, add 3 to that nibble, then shift the register
left by 1. Adding 3 moves d into 8..12, and the shift's low nibble
lands on (2d + 6) mod 16 = 2d - 10 with the shifted-out bit equal to 1
exactly when 2d >= 10, so the add-3 pre-arranges the shift's carry-out
to be the decimal carry. Digits below 5 double in place. The
implementation uses only shifts, masks, adds, and comparisons on
unsigned types; no sprintf, no lookup tables.

Verification (test_dabble.c): exhaustive over all 256 inputs. Each
input contributes 5 checks: three nibble-in-0..9 layout checks, one
identity check (hundreds*100 + tens*10 + ones == input), and one
differential check against snprintf "%03u" as oracle, 1,280 checks
total, 0 mismatches. The throughput accumulator is identical
(0x000001b9) across -O0, -O2, and ASan+UBSan builds. Compiles clean
under -Wall -Wextra -Werror with zero warnings; the ASan+UBSan run
reported nothing. Measured at -O2: 54.468 ns per conversion over
100,000,000 values; the timed loop includes one xorshift32 step per
value so inputs are runtime data the compiler cannot fold. See
PROOF.md for the full build and run logs.
