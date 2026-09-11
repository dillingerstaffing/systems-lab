<!-- PROOF-HEADER
Checks: 26777374
Mismatches: 0
Checksum: 262f6db6f6303175
Throughput: 3.460 ns/value at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/132-sv39-vpn-extract

`vpn_extract(va)` splits an Sv39 virtual address into its three 9-bit
VPN fields and its 12-bit page offset:
`vpn[i] = (va >> (12 + 9*i)) & 0x1FF`, `page_offset = va & 0xFFF`.

## Why the identity holds

Sv39 defines the virtual address by construction as three 9-bit page
number fields laid end to end above a 12-bit page offset: bits 38:30
are VPN[2], bits 29:21 are VPN[1], bits 20:12 are VPN[0], bits 11:0 are
the offset. Shifting right by (12 + 9*i) moves field i into bit
positions 8:0, and masking with 0x1FF (nine ones) keeps exactly those
bits while clearing everything above. The four masks 0xFFF,
0x1FF<<12, 0x1FF<<21, 0x1FF<<30 are disjoint and cover bits 38:0 with
no overlap and no gaps, so recombining by shifting the fields back
and ORing them restores the address exactly: recombine(extract(va))
== va follows from the disjointness of the masks.

## The sign-extension contract (stated explicitly)

Extraction reads only bits 38:0 of the input. A canonical 64-bit
Sv39 address has bit 38 sign-extended into bits 63:39; a raw 39-bit
field does not. Since extraction never reads bits above 38, both
forms give identical fields, and `vpn_recombine` returns the raw
39-bit field (bits 38:0). The input contract is therefore: any
uint64_t whose bits 38:0 hold the address; high bits are ignored,
never read. This is checked, not assumed: the canonical-address row
below feeds full 64-bit sign-extended addresses into vpn_extract.

## What was verified (real runs, real numbers)

- Differential test of `vpn_extract` against a hand-written per-bit
  oracle (structurally different: it iterates bit indices 0..38 and
  sets each destination bit individually, never shifting a whole
  field), plus the invariant recombine(extract(va)) == va and
  recombine(oracle(va)) == va on every case: 26,777,374 cases total,
  0 mismatches:
  - Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..38 (117 cases).
  - All-ones and all-zeros 39-bit values (2 cases).
  - Canonical-address row: each 2^k value with bit 38 sign-extended
    into bits 63:39, passed to vpn_extract unmasked; fields must be
    identical to the raw 39-bit form (39 cases).
  - Exhaustive 24-bit sweep: all va in [0, 2^24), 16,777,216 cases.
  - 10,000,000 fixed-seed splitmix64 values masked to 39 bits, seed
    0x123456789ABCDEF0, increment 0x9E3779B97F4A7C15.
- FNV-1a 64-bit checksum of the full extracted-field stream
  (vpn0, vpn1, vpn2, page_offset, low byte then high byte per field),
  identical across three builds: -O0, -O2, ASan+UBSan (-O1):
  `262f6db6f6303175`.
- Clean under `-std=c11 -Wall -Wextra -Werror`: zero warnings, all
  builds.
- ASan+UBSan (-O1, -g): zero reports, exit 0.
- Timing at -O2 over a pre-generated array of 10,000,000 39-bit
  values, best of 5 reps: 3.460 ns/value. Honest caveat: the measured
  path includes the array load, the call, and one XOR accumulate of
  all four fields per value, so this is the per-iteration cost of the
  loop as written, not of the function body alone.

## Build log and run output (real, from this machine)

```
$ make run
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_vpn_extract test_vpn_extract.c vpn_extract.c
./test_vpn_extract
checks=26777374 mismatches=0 checksum=262f6db6f6303175

$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_vpn_extract_o0 test_vpn_extract.c vpn_extract.c
./test_vpn_extract_o0
checks=26777374 mismatches=0 checksum=262f6db6f6303175

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -o test_vpn_extract_asan test_vpn_extract.c vpn_extract.c
./test_vpn_extract_asan
checks=26777374 mismatches=0 checksum=262f6db6f6303175

$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_vpn_extract bench_vpn_extract.c vpn_extract.c
./bench_vpn_extract
rep 0: 35616450 ns total, 3.562 ns/value
rep 1: 37635950 ns total, 3.764 ns/value
rep 2: 36527898 ns total, 3.653 ns/value
rep 3: 34595127 ns total, 3.460 ns/value
rep 4: 34884301 ns total, 3.488 ns/value
best=3.460 ns/value
sink=3791 (accumulator kept live)
```

Verification environment: Host (module executed natively on the build
machine). Toolchain: gcc 13.3.0 on Ubuntu 24.04, x86_64.
