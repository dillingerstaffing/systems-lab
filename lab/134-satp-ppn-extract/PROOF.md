<!-- PROOF-HEADER
Checks: 60397376
Mismatches: 0
Checksum: a5936745f37e6d57
Throughput: 153.158 ns/check at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/134-satp-ppn-extract

`satp_decode` splits an RV64 `satp` CSR value into its three fields,
`satp_recombine` rebuilds the word from the fields:

```
mode = (satp >> 60) & 0xF          (bits 63:60)
asid = (satp >> 44) & 0xFFFF      (bits 59:44)
ppn  =  satp        & 0xFFFFFFFFFFF (bits 43:0)

recombine = (mode << 60) | (asid << 44) | ppn
```

## Spec grounding

Layout and MODE encodings per RISC-V Instruction Set Manual, Volume II:
Privileged Architecture, v20211203, section 4.1.11 "Supervisor Address
Translation and Protection (satp) Register". Figure 4.14 gives the RV64
packing: MODE in bits 63:60 (4 bits), ASID in bits 59:44 (16 bits),
PPN in bits 43:0 (44 bits). The MODE encoding table in the same
section: 0 = Bare, 8 = Sv39, 9 = Sv48, 10 = Sv57, 11 = Sv64, other
values reserved. The extractor reports the raw 4 MODE bits and does
not interpret them.

## Why the identity holds

The three fields partition the 64-bit word: 4 + 16 + 44 = 64, with
MODE occupying 63:60, ASID 59:44, PPN 43:0. No overlaps, no gaps, no
reserved bits. Shifting right by a field's low bit moves that field
into bit 0, and AND with (1 << width) - 1 clears everything above the
field width, so each decode reads exactly its own field. Recombine
places each field back at its low-bit position; because the fields are
disjoint and together cover the whole word, recombine(decode(satp))
reproduces every input bit, so recombine(decode(satp)) == satp for
every 64-bit value. Recombine also masks each input struct field to
its width, so only field-width bits survive.

## The oracle

The differential oracle is structurally different from the
implementation: it builds each field bit-by-bit from single-bit
extractions at the spec's bit positions (MODE at bit 60+b, ASID at
44+b, PPN at bit b), accumulated with OR. It never shifts a
whole multi-bit field and never uses a composite field mask. The only
thing the oracle shares with the implementation is the field
boundaries, which come from the spec layout, not from the code under
test.

## Test corpus (every case: oracle match plus recombine(decode(s)) == s)

- (a) Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..63 (192 cases).
  The k = 43/44 and k = 59/60 rows straddle the PPN/ASID and
  ASID/MODE field boundaries, exercising carries across them.
- (b) Exhaustive ASID: all 65,536 ASID values placed in bits 59:44,
  MODE and PPN zero.
- (c) Exhaustive 24-bit PPN lanes: every value of a 24-bit window at
  lane offsets 0, 10, 20 inside bits 43:0; 3 x 16,777,216 =
  50,331,648 cases. The three lanes overlap (bits 0-23, 10-33,
  20-43) and together cover the full 44-bit PPN.
- (d) 10,000,000 fixed-seed splitmix64 raw 64-bit values
  (seed 0x123456789ABCDEF0), exercising all three fields together,
  including all 16 MODE values.

Total: 192 + 65,536 + 50,331,648 + 10,000,000 = 60,397,376 checks.

The FNV-1a 64 checksum folds the decoded output of every case (mode
byte, asid low then high byte, then the 6 low bytes of ppn covering
44 bits); identical checksums across -O2, -O0, and ASan+UBSan builds
confirm the decoded outputs are build-independent.

## Build log and run output (genuine)

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_satp_fields test_satp_fields.c satp_fields.c

$ ./test_satp_fields
checks=60397376 mismatches=0 checksum=a5936745f37e6d57
elapsed=9.250 s (153.158 ns/check)
```

``` 
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_satp_fields_o0 test_satp_fields.c satp_fields.c
./test_satp_fields_o0
checks=60397376 mismatches=0 checksum=a5936745f37e6d57
elapsed=16.126 s (267.004 ns/check)

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_satp_fields_asan test_satp_fields.c satp_fields.c
./test_satp_fields_asan
checks=60397376 mismatches=0 checksum=a5936745f37e6d57
elapsed=15.500 s (256.626 ns/check)
```

All three builds: 60,397,376 checks, 0 mismatches, identical checksum,
exit code 0, no ASan/UBSan findings.

## Environment

Host. gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), Linux
7.0.0-26-generic x86_64. Module executed natively on the build
machine; the decoder reads the value passed in, so no actual CSR
hardware access is involved (satp is machine/supervisor privileged
state and cannot be read from user mode).

## Scope statement

Verified: the three field extractions against the spec layout via the
per-bit oracle over the corpus above, and exact word reconstruction
by recombine on every case. Not verified: any hardware behavior (the
module models the CSR bit layout, it does not touch real CSRs); MODE
interpretation beyond reporting the raw bits (the spec's MODE table is
cited for the encoding meanings only).
