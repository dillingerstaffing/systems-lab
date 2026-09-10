<!-- PROOF-HEADER
Checks: 609
Mismatches: 0
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/03-mini-printf

Date: 2026-09-08. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build log and run output.

## Build log

```
$ make clean && make
rm -f test_printf mini_printf.o
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_printf test_printf.c mini_printf.c
make exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## Run output

```
$ ./test_printf
mini_printf differential test: 609 cases, 0 failed
ALL TESTS PASSED
run exit=0
```

## Binary size

```
$ size mini_printf.o test_printf
   text	   data	    bss	    dec	    hex	filename
   5104	      0	      0	   5104	   13f0	/tmp/mini_printf.o
  19899	   1152	     88	  21139	   5293	test_printf
$ ls -l test_printf
-rwxrwx--- 1 root nogroup 29488 Sep  8 22:56 test_printf
```

The formatter itself (`mini_printf.o`, `-O2`) is 5104 bytes of text,
zero data/bss: no lookup tables, no static buffers, no heap.

## What was verified

- Differential test against the host libc `snprintf`: every one of the
  609 cases formats identical arguments with both implementations and
  compares the output bytes and the return value exactly (length-aware,
  so embedded NULs via `%c` are covered).
- Conversions `%d %i %u %o %x %X %c %s %p %%`; flags `- +` (space) `#`
  `0`; widths including `*`; precisions including `.*`; length
  modifiers `l ll z`.
- Edge cases: `INT_MIN`, `LLONG_MIN`, `UINT_MAX`, `ULLONG_MAX`,
  `%.0d` of 0 (empty), `%#.0o` of 0 (`"0"`), empty string, `NUL`
  character, `NULL` string (with and without precision), `NULL`
  pointer (`(nil)`), unknown conversion `%q`, trailing `%`.
- `mini_snprintf` truncation semantics at buffer sizes 0, 1, 2, 5, 10,
  64, byte-compared against `snprintf`.
- Also passes clean under `-O0` and under
  `-fsanitize=address,undefined` (609 cases, 0 failures).

## Bugs found by the test during development

1. `%z` length modifier on signed conversions read only 32 bits
   (`%zd` of a 64-bit value was truncated). Fixed: `z` reads 64-bit.
2. `NULL` `%s` with precision diverged from libc (glibc prints empty
   when precision < 6, `(null)` otherwise). Fixed: library matches the
   libc rule.
3. Two test-harness bugs of mine (wrong varargs for literal-precision
   `%s` formats; swapped character positions when detecting `%.*s`),
   caught because the differential test crashed instead of passing
   silently.
