# lab/135-pte-permission-check

Sv39 leaf-PTE access permission check: `pte_permits(r, w, x, u,
access, mode)` decides whether an access is legal under the
privileged spec's page permission rules (reserved R/W/X encodings
010 and 110 fault, U-bit vs mode with SUM=0 assumed, and load needs
R, store needs W, exec needs X).

Verified by differential test against an independently written
encoding of the same spec rules (no shared helper code) over the
exhaustive 16 R/W/X/U combinations x 3 access types x 2 modes =
96 cases, 0 mismatches, with the FNV-1a checksum of every verdict
byte identical across -O2, -O0, and ASan+UBSan builds. Best observed
throughput: 4.423 ns per check at -O2. See PROOF.md for the full
verification record, including the spec section citations.

## Build and verify

```
make        # build the test binary
make run    # run the differential test
make opt0   # same test compiled -O0
make sanitize  # same test under ASan+UBSan
make bench  # throughput bench at -O2, best of 5 reps
make clean  # remove built binaries
```
