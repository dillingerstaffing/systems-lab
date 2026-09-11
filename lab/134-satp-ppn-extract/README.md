# lab/134-satp-ppn-extract

RV64 `satp` CSR field extraction: MODE (bits 63:60), ASID (bits 59:44),
PPN (bits 43:0), from pure shift/mask identities, plus a recombine
function that rebuilds the word from the decoded fields.

Verified by differential test against a hand-written per-bit oracle
that builds each field bit-by-bit from single-bit extractions at the
spec's bit positions (structurally different from the shift/mask
implementation, no shared helper code), plus the invariant
recombine(decode(satp)) == satp on every case: directed 2^k / 2^k +/- 1
edges for k = 0..63 (straddling the PPN/ASID and ASID/MODE
boundaries), an exhaustive 16-bit ASID sweep, exhaustive 24-bit PPN
lanes at offsets 0/10/20 covering the full 44-bit PPN, and 10M
fixed-seed splitmix64 raw 64-bit values, 0 mismatches. See PROOF.md
for the full verification record.

## Build and verify

```
make        # build the test binary
make run    # run the differential test
make opt0   # same test compiled -O0
make sanitize  # same test under ASan+UBSan
make clean  # remove built binaries
```
