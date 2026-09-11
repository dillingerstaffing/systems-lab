<!-- PROOF-HEADER
Checks: 22
Mismatches: 0
Checksum: 7eedb40a7904b74b
Throughput: 2.151 ns/validate at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md, lab/137-satp-mode-validate

`satp_mode_legal_rv64(mode)` answers one question about the RV64 satp
CSR: does the 4-bit MODE value (bits 63:60) name an
address-translation scheme the privileged architecture defines? It
returns 1 for a defined scheme and 0 otherwise, including values above
15 that the 4-bit field cannot hold.

## The tested rule, exactly

RV64 defines exactly four legal MODE values:

- 0: Bare, no translation or protection.
- 8: Sv39, page-based 39-bit virtual addressing.
- 9: Sv48, page-based 48-bit virtual addressing.
- 10: Sv57, page-based 57-bit virtual addressing.

Values 1-7 and 12-13 are reserved for standard use; value 11 is
reserved for a future page-based 64-bit scheme (Sv64, not yet defined
by this release); values 14-15 are designated for custom use. All of
these are illegal here. The function is a range predicate: value 0, or
values in the interval 8-10, the shape of the spec's SXLEN=64 rows.

## Spec grounding

The RISC-V Instruction Set Manual, Version 20260626: Intermediate
Release, Volume II: Privileged Architecture, section 4.1.11
("Supervisor Address Translation and Protection (satp) Register"),
Table 114, "Encoding of satp MODE field", page 741. The PDF was
downloaded from the official release URL
(https://github.com/riscv/riscv-isa-manual/releases/download/riscv-isa-release-dc8bf2a-2026-06-26/riscv-spec.pdf),
converted with pdftotext, and the SXLEN=64 rows are quoted here
verbatim from the extracted text:

```
Value Name Description
0      Bare   No translation or protection.
1-7      -    Reserved for standard use
8      Sv39 Page-based 39-bit virtual addressing (see Section 4.4).
9      Sv48 Page-based 48-bit virtual addressing (see Section 4.5).
10     Sv57 Page-based 57-bit virtual addressing (see Section 4.6).
11     Sv64 Reserved for page-based 64-bit virtual addressing.
12-13     -    Reserved for standard use
14-15     -    Designated for custom use
```

The surrounding body text (section 4.1.11) confirms the reading:
"When SXLEN=64, three paged virtual-memory schemes are defined: Sv39,
Sv48, and Sv57... One additional scheme, Sv64, will be defined in a
later version of this specification. The remaining MODE settings are
reserved for future use..."

Correction note: the proof-backlog gloss for this item mislabeled
value 9 as "Sv40". The document's row for value 9 reads "Sv48",
"Page-based 48-bit virtual addressing (see Section 4.5)". The
implementation, the oracle, and this PROOF.md follow the document.

The verdict is legality of the encoding in the spec, not support on a
particular hart: the same section states "Implementations are not
required to support all MODE settings, and if satp is written with an
unsupported MODE, the entire write has no effect; no fields in satp
are modified."

## How it was verified

The implementation (satp_mode_validate.c) is a range predicate:
`mode == 0 || (mode >= 8 && mode <= 10)`. The oracle
(test_satp_mode_validate.c) is structurally independent: an
explicitly enumerated 16-entry list, each row hand-transcribed from
Table 114's SXLEN=64 rows with the document's row text quoted in the
comment block. The two sides share no table and no predicate shape.

Test space, 22 checks total, 0 mismatches:

- Exhaustive: all 16 values the 4-bit MODE field can hold (0-15),
  each against its transcribed row.
- 6 out-of-domain rows: 16, 17, 31, 32, 100, and 0xFFFFFFFF; the
  4-bit field cannot hold these, so no table row covers them and the
  function must return 0 for each.

Every verdict byte feeds an FNV-1a 64-bit checksum. The checksum is
identical across the -O2, -O0, and ASan+UBSan builds:

```
checks=22 mismatches=0 checksum=7eedb40a7904b74b
```

Throughput, best of 5 reps at -O2 (50,000,000 validates cycling MODE
0-15):

```
2.151 ns/validate at -O2 (best of 5, N=50000000)
```

## Scope (stated, not assumed)

This module judges only the MODE encoding. It does not extract MODE,
ASID, or PPN from a satp value (that is lab/134-satp-ppn-extract), does
not walk page tables, and says nothing about whether a given hart
supports a legal setting: legality in the spec is the only verdict
here. Sv64 (value 11) is treated as illegal because the pinned release
reserves it for a scheme "to be defined in a later version of this
specification"; a future release that defines Sv64 would change this
row, which is why the document title and version are quoted above.

## Genuine build log and run output

Captured 2026-09-11 by running the Makefile targets in sequence
(full log also saved as full_run.log in this directory):

```
$ make clean && make && make run && make opt0 && make sanitize && make bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_satp_mode_validate test_satp_mode_validate.c satp_mode_validate.c
./test_satp_mode_validate
mode= 0 got=1 want=1
mode= 1 got=0 want=0
mode= 2 got=0 want=0
mode= 3 got=0 want=0
mode= 4 got=0 want=0
mode= 5 got=0 want=0
mode= 6 got=0 want=0
mode= 7 got=0 want=0
mode= 8 got=1 want=1
mode= 9 got=1 want=1
mode=10 got=1 want=1
mode=11 got=0 want=0
mode=12 got=0 want=0
mode=13 got=0 want=0
mode=14 got=0 want=0
mode=15 got=0 want=0
mode=16 got=0 want=0
mode=17 got=0 want=0
mode=31 got=0 want=0
mode=32 got=0 want=0
mode=100 got=0 want=0
mode=4294967295 got=0 want=0
checks=22 mismatches=0 checksum=7eedb40a7904b74b
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_satp_mode_validate_o0 test_satp_mode_validate.c satp_mode_validate.c
./test_satp_mode_validate_o0
checks=22 mismatches=0 checksum=7eedb40a7904b74b
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_satp_mode_validate_asan test_satp_mode_validate.c satp_mode_validate.c
./test_satp_mode_validate_asan
checks=22 mismatches=0 checksum=7eedb40a7904b74b
gcc -std=c11 -O2 -Wall -Wextra -Werror -o bench_satp_mode_validate bench_satp_mode_validate.c satp_mode_validate.c
./bench_satp_mode_validate
2.151 ns/validate at -O2 (best of 5, N=50000000)
```

All three test builds compiled with zero warnings (-Wall -Wextra
-Werror) and agree on every verdict byte (identical checksum); the
sanitizer build reported no errors. The test exits 0 only when
mismatches is 0.
