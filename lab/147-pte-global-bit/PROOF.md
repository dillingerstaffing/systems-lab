<!-- PROOF-HEADER
Checks: 201850
Mismatches: 0
Checksum: fa4a8dbd46381f62
Throughput: 196.216 ns/lookup at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/147-pte-global-bit

A from-scratch Sv39 page-table walker plus an ASID-tagged translation
cache, verifying the PTE G-bit rule: a leaf PTE with G set marks its
translation global, so the cached translation applies across address
spaces without consulting the ASID; a leaf with G clear is ASID-scoped.

The bit identities the module is built from (RISC-V privileged
architecture, Sv39): PTE flag bits V=0, R=1, W=2, X=3, U=4, G=5, A=6,
D=7; PPN field bits 53:10; three walk levels with VPN[2] = va[38:30],
VPN[1] = va[29:21], VPN[0] = va[20:12]; satp carries the ASID in bits
59:44 and the root PPN in bits 43:0. Physical memory is a flat array of
64 pages; a page's PPN is its array index.

## What was verified (real runs, real numbers)

Walk tests, 15 walks against hard-coded expected outcomes (48 checks):
- The shared global 4K leaf (vpn 0x12345, PPN 0xABCDE, G set): walks
  under satp ASID 1, ASID 2, and ASID 9 all translate to the same
  physical address `(0xABCDE << 12) | 0x678`, global=1, level=0. Three
  address spaces, one physical page.
- The non-global contrast (vpn 0x23456, G clear): ASID 1 walks to
  `(0x11111 << 12) | 0x678`, ASID 2 walks to `(0x22222 << 12) | 0x678`,
  global=0. Same VPN, different physical pages per address space.
- A non-global leaf present only under ASID 1 (vpn 0x34567): the ASID 1
  walk succeeds; the ASID 2 walk faults instead of reusing ASID 1's
  translation.
- The shared global 2 MiB megapage (vpn 0x25600, PPN 0xF0000, G set):
  both ASIDs translate to `(0xF0000 << 12) | 0x12345`, global=1,
  level=1.
- The shared global 1 GiB gigapage (va 0x40000000, PPN 0x40000, G set):
  both ASIDs translate to `(0x40000 << 12) | 0x1234`, global=1,
  level=2.
- Fault cases, all fault as required: misaligned megapage PPN (low 9
  PPN bits nonzero), leaf with W set but R clear (reserved
  combination), V-clear entry.
- Branch PTE with G set but leaf with G clear: the walk reports
  global=0. Only the leaf PTE's G bit decides; G on a non-leaf entry
  is not consulted.

TLB differential test, implementation `tlb_lookup` against an
independently written spec table (if/else ladder over a plain entry
list, sharing no code with the implementation's scan; the transcribed
rule: a cached global entry applies to every ASID without consulting
it, a cached non-global entry applies only on ASID match):
- Integration (10 checks): a global translation cached under ASID 1 is
  found by a lookup under ASID 2; a non-global translation cached under
  ASID 1 is missed by a lookup under ASID 2; two non-global entries for
  the same VPN under ASID 1 and ASID 2 coexist and each lookup returns
  its own ASID's physical address; a third ASID sees neither.
- Directed G set/clear x ASID match/mismatch matrix (28 checks): 4
  inserted entries (G in {0,1}, owner in {1,2}) x 3 lookup ASIDs
  {1,2,3}, plus 2 miss probes on an unpopulated VPN.
- Bulk sweep (200,484 checks): 200,000 fixed-seed splitmix64 operations
  (seed 0x123456789ABCDEF0), alternating random inserts (18-bit VPN,
  ASID 1..4, random PA, random G) and random lookups compared against
  the oracle, exercising FIFO eviction and stale entries.
- Final slot-by-slot consistency between the implementation table and
  the spec table, all 256 slots x 5 fields (1,280 checks).

Totals: 201,850 checks, 0 mismatches.
- FNV-1a 64-bit checksum of the full compared-result stream, identical
  across three builds: -O0, -O2, ASan+UBSan (-O1, -g):
  `fa4a8dbd46381f62`.
- Clean under `-std=c11 -Wall -Wextra -Werror`: zero warnings, all builds.
- ASan+UBSan (-O1, -g): zero reports, exit 0.
- Timing at -O2 over 2,000,000 pre-generated random lookups against a
  full 256-entry table (mixed global/non-global, ASIDs 1..4), best of 5
  reps: 196.216 ns/lookup. Honest caveat: this measures the lookup as
  written, a linear scan of up to 256 slots, so most probes scan the
  whole table; it is the per-lookup cost of this model, not of a
  hardware TLB.

## Genuine build log

```
$ make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_pte_global test_pte_global.c pte_global.c

$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_global_o0 test_pte_global.c pte_global.c

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_pte_global_asan test_pte_global.c pte_global.c

$ make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_tlb_lookup bench_tlb_lookup.c pte_global.c
```

## Genuine run output (-O2)

```
HEADER-BEGIN
Checks: 201850
Mismatches: 0
Checksum: fa4a8dbd46381f62
HEADER-END
verdict: PASS
```

The -O0 and ASan+UBSan runs printed the identical block
(Checks: 201850, Mismatches: 0, Checksum: fa4a8dbd46381f62), both
exiting 0. Bench output:

```
acc: 2984259584 (accumulate sink, prevents loop elimination)
best of 5 reps, 2000000 lookups: 196.216 ns/lookup
HEADER-LINE
Throughput: 196.216 ns/lookup at -O2
```

## Case mix, exactly as run

1. 15 directed walks (48 checks): the shared-global 4K leaf under 3
   ASIDs, the non-global per-ASID leaves, the ASID-1-only leaf faulting
   under ASID 2, the shared global megapage under 2 ASIDs, the shared
   global gigapage under 2 ASIDs, 3 fault cases (misaligned megapage,
   W-without-R, V-clear), and the branch-G-set/leaf-G-clear case.
2. 5 TLB integration lookups (10 checks) chaining walk results through
   the cache across ASIDs.
3. 14 directed matrix lookups (28 checks): G in {0,1} x owner in
   {1,2} x lookup ASID in {1,2,3}, plus 2 miss probes.
4. 200,000 fixed-seed splitmix64 insert/lookup operations
   (100,242 lookups x 2 fields = 200,484 checks).
5. 256-slot x 5-field table consistency scan (1,280 checks).
6. Every compared value is folded into the FNV-1a checksum, so the
   checksum equality across builds covers the whole stream.

## Limits of verification

This module is a user-space model of the Sv39 walk and the G-bit TLB
rule on the host machine; no privileged hardware was involved and no
address was actually translated by an MMU. What was checked is the
decision procedure: the walker produces the specified physical
address, global flag, and fault behavior on the scenario tables, and
the cache's ASID/global matching agrees with the independently written
spec table on 100,261 lookups. Deliberate scope cuts: permission faults
and A/D-bit handling are not modeled; G is read from the leaf PTE
only (the model's stated rule; a different reading of G on branch
entries would need its own test); insertion order and FIFO eviction are
shared harness mechanics between the implementation and the oracle,
while the match rule under test is encoded independently; as in any
differential test against a transcribed rule, a misreading of the
architecture's rule itself would pass. The rule text used is the
privileged specification's Sv39 page-table-entry description of the G
bit (global mappings are not distinguished by ASID in the TLB).
