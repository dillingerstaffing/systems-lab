<!-- PROOF-HEADER
Checks: 1000000
Mismatches: 0
Checksum: 0x277d10c8b7ea96ec
Throughput: 207.7 MiB/s at -O2
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/82-crc32-residue

Date: 2026-09-10. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build logs and run outputs.

`crc32.c`/`crc32.h` are the lab/04 sources compiled in unchanged; the
residue test is in `test_residue.c`.

## Build log and run (-O2)

```
$ make clean && make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_residue test_residue.c crc32.c
make exit=0

$ ./test_residue
ok   crc32("123456789") table: 0xcbf43926
ok   crc32("123456789") bitwise == table: 0xcbf43926
ok   residue over "123456789"+crc (LE): 0x2144df1c
ok   wrong byte order (BE) gives 0x3cc9742c, not the residue
ok   crc32(empty): 0x00000000
ok   residue over empty+4 zero bytes: 0x2144df1c
cases: 1000000 (length-0 cases: 3868)
residue mismatches: 0
oracle disagreements (table vs bitwise): 0
fnv64 over per-case results: 0x277d10c8b7ea96ec
throughput (two-pass: checksum, append, re-checksum over 1000000 buffers, best of 3):
  bytes checksummed: pass1=127936236 pass2=131936236
  207.7 MiB/s (0.20 GiB/s)
RESULT: ALL TESTS PASSED
run exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## -O0 build and run

```
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_residue_o0 test_residue.c crc32.c
./test_residue_o0
ok   crc32("123456789") table: 0xcbf43926
ok   crc32("123456789") bitwise == table: 0xcbf43926
ok   residue over "123456789"+crc (LE): 0x2144df1c
ok   wrong byte order (BE) gives 0x3cc9742c, not the residue
ok   crc32(empty): 0x00000000
ok   residue over empty+4 zero bytes: 0x2144df1c
cases: 1000000 (length-0 cases: 3868)
residue mismatches: 0
oracle disagreements (table vs bitwise): 0
fnv64 over per-case results: 0x277d10c8b7ea96ec
throughput (two-pass: checksum, append, re-checksum over 1000000 buffers, best of 3):
  bytes checksummed: pass1=127936236 pass2=131936236
  184.4 MiB/s (0.18 GiB/s)
RESULT: ALL TESTS PASSED
opt0 exit=0
```

## AddressSanitizer + UBSan build and run

```
$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_residue_asan test_residue.c crc32.c
./test_residue_asan
ok   crc32("123456789") table: 0xcbf43926
ok   crc32("123456789") bitwise == table: 0xcbf43926
ok   residue over "123456789"+crc (LE): 0x2144df1c
ok   wrong byte order (BE) gives 0x3cc9742c, not the residue
ok   crc32(empty): 0x00000000
ok   residue over empty+4 zero bytes: 0x2144df1c
cases: 1000000 (length-0 cases: 3868)
residue mismatches: 0
oracle disagreements (table vs bitwise): 0
fnv64 over per-case results: 0x277d10c8b7ea96ec
throughput (two-pass: checksum, append, re-checksum over 1000000 buffers, best of 3):
  bytes checksummed: pass1=127936236 pass2=131936236
  175.1 MiB/s (0.17 GiB/s)
RESULT: ALL TESTS PASSED
sanitize exit=0
```

The sanitizer run produced no ASan or UBSan report: zero sanitizer
findings across the full 1,000,000-case set. The FNV-1a checksum over
the per-case results, `0x277d10c8b7ea96ec`, is byte-identical across
all three builds.
