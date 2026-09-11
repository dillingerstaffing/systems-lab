# lab/149-pte-permission-and

Sv39 multi-level page-table walk permission check:
`walk_permits(levels, nlevels, access, mode)` decides whether an
access is legal given one decoded PTE per walk level. The spec
decides permission at the single leaf where the walk terminates
(section 4.3.2, steps 3 and 5): non-leaf entries carry no
permissions and must be pure pointers ((R,W,X) == (0,0,0)),
r=0,w=1 faults at any level, and the leaf's R/W/X/U bits decide
per the section 4.3.1 / Table 115 rules (reserved 010 and 110
encodings fault, U-bit vs mode with SUM=0 assumed, load needs R,
store needs W, exec needs X). The backlog's "AND of the leaf PTE
bits across the walk" gloss is a simplification; the module
implements and states the rule as the spec actually describes
it.

Verified by differential test against an independently written
encoding of the same spec rules (no shared helper code) over the
exhaustive 26208 cases (1-, 2-, and 3-level walks, all 16 R/W/X/U
combinations per level, 3 access types, 2 modes) plus 8192
fixed-seed random walks (seed 149) and 2 malformed-input edge
cases, 0 mismatches, with the FNV-1a checksum of every verdict
byte identical across -O2, -O0, and ASan+UBSan builds. Best
observed throughput: 12.058 ns per check at -O2. See PROOF.md
for the full verification record, including the spec section
citations.

## Build and verify

```
make        # build the test binary
make run    # run the differential test
make opt0   # same test compiled -O0
make sanitize  # same test under ASan+UBSan
make bench  # throughput bench at -O2, best of 5 reps
make clean  # remove built binaries
```
