# lab/20-xorshift-period

A 16-bit xorshift generator with its period proved by exhaustive
state-space traversal, not by argument.

The recurrence uses the (7, 9, 8) shift triple for 16-bit xorshift
(documented by Marsaglia as a maximal-period triple; the test
re-verifies that independently). One step costs three shifts, three
XORs, no memory access, no multiplication.

The test seeds the generator, walks it until the state returns to
the seed, and requires:

- exactly 65,535 steps before first return (2^16 - 1),
- every visited state unique before return (a 65,536-byte visited
  array detects any premature repeat),
- all 65,535 nonzero states visited exactly once,
- the state 0 never visited from a nonzero seed, and
  `xorshift16_step(0) == 0` (the fixed point that bounds the period
  at 2^16 - 1 instead of 2^16).

Two unrelated seeds (0x0001, 0xBEEF) are walked, showing each
nonzero seed rides the same single full cycle. See PROOF.md for the
build log, run output, and measured numbers.
