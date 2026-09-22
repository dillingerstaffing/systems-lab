<!-- PROOF-HEADER
Checks: 36
Mismatches: 0
Checksum: b84a9a8a800bb03bee2d942b1859c2f432887a7b4cd17fe7a6f6f686d0563af8
Environment: Host (Linux, x86-64; xPack GNU RISC-V Embedded GCC 15.2.0, -O2 -mabi=lp64d)
Verdict: PASS
-->
# PROOF.md, lab/161-ztso-fence-delete

`check.py` proves the Ztso (Total Store Ordering) promise is cashed in by
the compiler as deleted fences. `tso.c` holds four atomics: a
sequentially consistent store, a sequentially consistent load, a release
store, and an acquire load. The script compiles it twice, once for
`-march=rv64gc` and once for `-march=rv64gc_ztso`, and reads the fence
instructions out of each function's disassembly.

36 checks, 0 mismatches: 3 runs x 12 assertions each. Per run: both
builds compile clean (2); the fence sequence per function matches the
expected table (8: base emits `fence rw,w` before a seq_cst store and
`fence r,rw` after an acquire load, Ztso deletes exactly those two
shapes and keeps one `fence rw,rw` per seq_cst op); and the binary
signature (2: the ztso object's `Tag_RISCV_arch` gains `_ztso1p0`,
the base object's does not). The three run logs are byte-identical, so
the checksum is stable with no normalization.

The deleted fences are exactly the ones the spec makes redundant: Ztso
chapter 26 says the RVTSO rules "make redundant any non-I/O fences that
do not have both PW and SR set". `fence rw,w` lacks SR, `fence r,rw`
lacks PW, and both are gone under `-march=rv64gc_ztso`. The surviving
`fence rw,rw` has both, one per seq_cst op, positioned against the one
reorder TSO still permits (a later load passing an earlier store).

Honest boundary, recorded not passed: QEMU 8.2.2 has no `ztso` CPU
property, so the RVWMO-vs-RVTSO behavioral delta (the message-passing
litmus: hart 0 stores x=1 then y=1, hart 1 loads y then x; r1=1/r2=0 is
observable under RVWMO, forbidden under RVTSO) stays a design, not an
executed test. What this lab proves is the compiler side: the promise
goes in, the fences come out.

## Build log (genuine)

```
$ make run
b84a9a8a800bb03bee2d942b1859c2f432887a7b4cd17fe7a6f6f686d0563af8  -
build: clean, no warnings (-Wall -Wextra -Werror), 3 byte-identical runs
```
