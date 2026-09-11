# lab/146-pte-accessed-bit

Sv39 PTE A (accessed) bit rule check: `pte_a_check` takes a PTE flag
byte and an access type (read/write/execute) and reports whether the
access faults and what the A bit is after the access. A permitted
access sets A to 1; a fault leaves the PTE untouched, so A keeps its
input value. Invalid (V = 0) and reserved (W = 1, R = 0) entries fault
every access.

Verified by differential test against a hand-transcribed spec table
over the full space: all 256 flag bytes times 3 access types, 768
cases, 0 mismatches, with the A-bit transition invariant checked on
every case. See PROOF.md for the full verification record.
