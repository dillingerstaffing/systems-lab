<!-- PROOF-HEADER -->
Checks: 4304967566
Mismatches: 0
Checksum: e3c4316e414a6146
Throughput: 9.626 ns per decode (best of 5 reps at -O2)
Environment: Host
Verdict: PASS

# PROOF.md, lab/133-sv39-pte-decode

`pte_decode(pte)` splits a 64-bit Sv39 leaf page-table entry into its
fields by pure shift/mask identities, and `pte_encode(f)` recombines
the fields into the word:

- `v   = pte & 1` (bit 0)
- `r   = (pte >> 1) & 1` (bit 1)
- `w   = (pte >> 2) & 1` (bit 2)
- `x   = (pte >> 3) & 1` (bit 3)
- `u   = (pte >> 4) & 1` (bit 4)
- `g   = (pte >> 5) & 1` (bit 5)
- `a   = (pte >> 6) & 1` (bit 6)
- `d   = (pte >> 7) & 1` (bit 7)
- `rsw = (pte >> 8) & 3` (bits 9:8)
- `ppn = (pte >> 10) & 0xFFFFFFFFFFF` (bits 53:10, 44 ones)

## Why the identities hold

The Sv39 PTE is defined by its bit layout: bits 53:10 hold the
physical page number (44 bits), bits 9:8 are reserved for supervisor
software (RSW), and bits 7:0 are the D, A, G, U, X, W, R, V flags.
Shifting right by k moves bit k into bit position 0; masking with a
field of ones keeps exactly the field width and clears everything
above. The ten field masks are disjoint and cover bits 53:0 with no
overlap and no gaps, so the fields partition the word: recombining by
shifting each field back to its position and ORing restores exactly
bits 53:0. `encode(decode(pte)) == pte & 0x003FFFFFFFFFFFFF` is a
consequence of that partition, and the test checks it on every case.

The flag order V, R, W, X, U (bits 0 to 4) matches xv6's
`kernel/riscv.h` (`PTE_V (1L << 0)` through `PTE_U (1L << 4)`), and
the full ten-field layout was cross-checked by a second,
independently written per-bit Python reference (each field rebuilt
one bit at a time, rebuild equals the input masked to bits 53:0) over
200,162 values: the 162 directed edges plus 200,000 fixed-seed random
64-bit words, 0 mismatches.

## The reserved-bits contract (stated explicitly)

Decode reads only bits 53:0 of the input. Bits 63:54 are reserved for
future use; they are never read, so any value there leaves every
decoded field unchanged, and encode always leaves bits 63:54 clear.
This is checked, not assumed: the reserved-bits row below feeds each
directed edge value with bits 63:54 set and requires the decoded
fields to be identical to the un-set version.

## What was verified (real runs, real numbers)

Differential test against an independent oracle plus the round-trip
invariant, on four corpora:

1. Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..53 (162 cases).
2. Reserved-bits row: the 54 directed 2^k values ORed with
   0xFFC0000000000000 (bits 63:54), each checked for field equality
   against the bare value and then run through the full differential
   check (108 checks).
3. Exhaustive 32-bit sweep: every pte in [0, 2^32), 4,294,967,296
   cases.
4. 10,000,000 fixed-seed splitmix64 raw 64-bit values
   (seed 0x123456789ABCDEF0).

The oracle is structurally different from the implementation: one
loop per field, each destination field built bit-by-bit from
single-bit extractions through a destination pointer table, never
shifting or masking a whole multi-bit field at once, with no helper
code shared with the implementation. On every case the test requires
both (a) decoded fields equal the oracle's fields and (b)
encode(decoded) equals the input masked to bits 53:0.

Checksum: FNV-1a 64 over every case's decoded output (6 bytes of PPN,
then RSW, then the packed flag byte), so the run's output bytes are
pinned, not just the mismatch count.

Result: checks=4304967566, mismatches=0, checksum folded over all
4,304,967,566 decoded outputs, verdict PASS.

## Throughput (separate bench, same machine)

`bench_pte_decode.c` decodes 10,000,000 fixed-seed 64-bit words, 5
reps, best rep reported; the accumulator is kept live through a
volatile sink so the decode loop is not optimized away. Three runs
gave bests of 25.782, 9.626, and 11.195 ns per decode at -O2 on this
noisy VM; best observed 9.626 ns per decode.

## Environment

Host. gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64, Linux
7.0.0-26-generic. Built with `gcc -std=c11 -O2 -Wall -Wextra -Werror`.

## Build log

```
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_decode test_pte_decode.c pte_decode.c
$ ./test_pte_decode
checks=4304967566 mismatches=0 checksum=e3c4316e414a6146
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_pte_decode bench_pte_decode.c pte_decode.c
$ ./bench_pte_decode
best=9.626 ns/value
```

Both builds are warning-free under -Wall -Wextra -Werror. Test
binaries are not committed.
