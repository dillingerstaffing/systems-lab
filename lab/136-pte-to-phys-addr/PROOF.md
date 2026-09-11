<!-- PROOF-HEADER
Checks: 60331824
Mismatches: 0
Checksum: 235e187d99433244
Throughput: 4.275 ns per conversion (best of 5 reps at -O2)
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/136-pte-to-phys-addr

`phys_addr_from_ppn(ppn, offset)` forms the Sv39 physical address
from a leaf PTE's page number:

    phys_addr = ((ppn & 0xFFFFFFFFFFF) << 12) | (offset & 0xFFF)

## Why the identity holds

The Sv39 leaf PTE places its 44-bit physical page number at PTE
bits 53:10 (RISC-V privileged spec, section 4.3.1), and the physical
address is the page number followed by the 12-bit page offset. A
left shift by 12 moves PPN bit k to physical-address bit k + 12,
landing PPN bits 43:0 exactly on physical-address bits 55:12; the
12-bit offset lands exactly on bits 11:0. The two fields are
disjoint, so OR recombines them without carry between the fields.
The result is always narrower than 2^56: shifting a 44-bit value
left by 12 needs 56 bits and no more. Masking both inputs to their
field widths first makes the contract explicit: bits above 44 for
the PPN and above 12 for the offset are never read.

The PPN bit layout was cross-checked against the same pinned spec
section grounding used by lab/133 (PPN = PTE bits 53:10), and the
physical-address formation was verified against the ground-truth
oracle described below rather than against any library routine.

## What was verified (real runs, real numbers)

Differential test against a structurally independent per-bit oracle
on five corpora:

1. Directed edges over the 56-bit physical space: 2^k, 2^k - 1,
   2^k + 1 for k = 0..55, each mapped to its (ppn, offset) pair by
   splitting at bit 12 (168 cases).
2. All-ones / all-zeros rows: (0, 0), (PPN all ones, offset all
   ones), and the two mixed rows (4 cases).
3. Mask-contract rows: full 64-bit inputs with bits above the
   44/12-bit contracts set (bits 63:44 of the PPN, bit 44 set
   alone, offset bit 12 set alone), confirming the high bits are
   never read (4 cases).
4. Exhaustive 24-bit PPN lanes: ppn in [0, 2^24) with offset 0x000,
   ppn in [0, 2^24) with offset 0xFFF, and the top lane
   ppn in [2^44 - 2^24, 2^44) with offset 0x5A5
   (3 x 16,777,216 = 50,331,648 cases).
5. 10,000,000 fixed-seed splitmix64 (ppn, offset) pairs with full
   64-bit inputs, seed 0x123456789ABCDEF0.

The oracle never shifts a whole field: it walks destination bit
indices 0..55 and sets each destination bit individually from one
single-bit extraction of the source input, so its construction
shares no structure with the implementation's
((ppn & mask) << 12) | (offset & mask). No helper code is shared
between oracle and implementation.

Checksum: FNV-1a 64 over every case's 7 output bytes, so the run's
output bytes are pinned, not just the mismatch count. The checksum
235e187d99433244 is identical across the -O2, -O0, and
ASan+UBSan builds, with zero sanitizer findings on the full
60,331,824-case run.

Result: checks=60331824, mismatches=0,
checksum=235e187d99433244, verdict PASS.

## Throughput (separate bench, same machine)

`bench_pte_phys.c` converts 10,000,000 fixed-seed (ppn, offset)
pairs, 5 reps, best rep reported; the accumulator is kept live
through a volatile sink so the conversion loop is not optimized
away. Two runs gave bests of 4.326 and 4.275 ns per conversion at
-O2 on this noisy VM; best observed 4.275 ns per conversion.

## Environment

Host. gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64, Linux
7.0.0-26-generic. Built with `gcc -std=c11 -O2 -Wall -Wextra -Werror`.

## Build log

```
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_phys test_pte_phys.c pte_phys.c
$ ./test_pte_phys
checks=60331824 mismatches=0 checksum=235e187d99433244
header-checks=60331824
header-mismatches=0
header-checksum=235e187d99433244
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_phys_o0 test_pte_phys.c pte_phys.c
$ ./test_pte_phys_o0
checks=60331824 mismatches=0 checksum=235e187d99433244
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_pte_phys_asan test_pte_phys.c pte_phys.c
$ ./test_pte_phys_asan
checks=60331824 mismatches=0 checksum=235e187d99433244
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_pte_phys bench_pte_phys.c pte_phys.c
$ ./bench_pte_phys
rep 0: 42994262 ns total, 4.299 ns/conversion
rep 1: 42749625 ns total, 4.275 ns/conversion
rep 2: 43226791 ns total, 4.323 ns/conversion
rep 3: 43056925 ns total, 4.306 ns/conversion
rep 4: 43481984 ns total, 4.348 ns/conversion
best=4.275 ns/conversion
```

All builds are warning-free under -Wall -Wextra -Werror. The
-O2, -O0, and sanitizer runs print the same checksum, and the
sanitizer runs report zero findings. Test binaries are not
committed.
