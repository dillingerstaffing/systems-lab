# lab/137-satp-mode-validate

RV64 satp.MODE legality check:
`satp_mode_legal_rv64(mode)` reports whether a 4-bit satp MODE value
names an address-translation scheme the privileged architecture
defines. Only 0 (Bare), 8 (Sv39), 9 (Sv48), and 10 (Sv57) are defined
by Table 114 of section 4.1.11 in the pinned spec release (The RISC-V
Instruction Set Manual, Version 20260626: Intermediate Release);
values 1-7 and 12-13 are reserved for standard use, 11 is reserved for
a future 64-bit scheme (Sv64, not yet defined), 14-15 are designated
for custom use, and values above 15 cannot be held by the 4-bit
field. The implementation is a range predicate (value 0 or values
8-10), checked against an independently hand-transcribed 16-row
oracle, each row quoted from the document's Table 114.

Verified by differential test over the exhaustive 4-bit value space
(0-15) plus 6 out-of-domain rows (16, 17, 31, 32, 100,
0xFFFFFFFF): 22 checks, 0 mismatches, with the FNV-1a checksum of
every verdict byte identical across -O2, -O0, and ASan+UBSan builds.
Best observed throughput: 2.151 ns per validate at -O2. See PROOF.md
for the full verification record, including the verbatim spec table
rows and a correction to the backlog gloss (value 9 is Sv48, not
Sv40).

## Build and verify

```
make        # build the test binary
make run    # run the differential test
make opt0   # same test compiled -O0
make sanitize  # same test under ASan+UBSan
make bench  # throughput bench at -O2, best of 5 reps
make clean  # remove built binaries
```
