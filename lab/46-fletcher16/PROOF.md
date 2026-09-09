# PROOF.md: lab/46-fletcher16

Environment: gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64.

Note on the "123456789" vector: the brief suggested "123456789" ->
0xBB3D as a published Fletcher-16 vector. Checking it against the
published standard (the Fletcher's checksum article, which documents
Fletcher's original paper and gives a straightforward implementation
matching this lab's recurrence) showed it is not: the article's
published Fletcher-16 vectors are "abcde" -> 0xC8F0, "abcdef" -> 0x2057,
"abcdefgh" -> 0x0627, all of which this implementation reproduces, and
hand arithmetic gives fletcher16("123456789") = 0x1EDE (sum1 = 477 % 255
= 222, sum2 = 2325 % 255 = 30), matching both the implementation and the
independent reference. The 0xBB3D value was not used; see test_fletcher16.c
for the vector sources.

## Build

All four targets compile with `-std=c11 -Wall -Wextra -Werror`.
Zero warnings (any warning would fail the build).

## Runs

### -O2 (`make test-o2`)

```
$ make test-o2
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_o2 test_fletcher16.c
./test_o2
vector empty          expect=0x0000 got=0x0000 ref=0x0000 OK
vector "abcde"        expect=0xc8f0 got=0xc8f0 ref=0xc8f0 OK
vector "abcdef"       expect=0x2057 got=0x2057 ref=0x2057 OK
vector "abcdefgh"     expect=0x0627 got=0x0627 ref=0x0627 OK
vector "123456789"    expect=0x1ede got=0x1ede ref=0x1ede OK
vector "a"            expect=0x6161 got=0x6161 ref=0x6161 OK
vector {0xFF}         expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x01,0xFF}    expect=0x0201 got=0x0201 ref=0x0201 OK
differential: 1000000 buffers, 1388938179 bytes total, seed=0xF1E7C416, mismatches=0
fnv1a=0x903aa7957d888495
throughput: 256.0 MiB in 1.165 s = 219.8 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```

### -O0 (`make test-o0`)

```
$ make test-o0
gcc -std=c11 -Wall -Wextra -Werror -O0 -g -o test_o0 test_fletcher16.c
./test_o0
vector empty          expect=0x0000 got=0x0000 ref=0x0000 OK
vector "abcde"        expect=0xc8f0 got=0xc8f0 ref=0xc8f0 OK
vector "abcdef"       expect=0x2057 got=0x2057 ref=0x2057 OK
vector "abcdefgh"     expect=0x0627 got=0x0627 ref=0x0627 OK
vector "123456789"    expect=0x1ede got=0x1ede ref=0x1ede OK
vector "a"            expect=0x6161 got=0x6161 ref=0x6161 OK
vector {0xFF}         expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x01,0xFF}    expect=0x0201 got=0x0201 ref=0x0201 OK
differential: 1000000 buffers, 1388938179 bytes total, seed=0xF1E7C416, mismatches=0
fnv1a=0x903aa7957d888495
throughput: 256.0 MiB in 1.304 s = 196.4 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```

### ASan+UBSan (`make test-asan`)

```
$ make test-asan
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -o test_asan test_fletcher16.c
./test_asan
vector empty          expect=0x0000 got=0x0000 ref=0x0000 OK
vector "abcde"        expect=0xc8f0 got=0xc8f0 ref=0xc8f0 OK
vector "abcdef"       expect=0x2057 got=0x2057 ref=0x2057 OK
vector "abcdefgh"     expect=0x0627 got=0x0627 ref=0x0627 OK
vector "123456789"    expect=0x1ede got=0x1ede ref=0x1ede OK
vector "a"            expect=0x6161 got=0x6161 ref=0x6161 OK
vector {0xFF}         expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x01,0xFF}    expect=0x0201 got=0x0201 ref=0x0201 OK
differential: 1000000 buffers, 1388938179 bytes total, seed=0xF1E7C416, mismatches=0
fnv1a=0x903aa7957d888495
throughput: 256.0 MiB in 1.157 s = 221.3 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```

### UBSan (`make test-ubsan`)

```
$ make test-ubsan
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined -fno-sanitize-recover=all -o test_ubsan test_fletcher16.c
./test_ubsan
vector empty          expect=0x0000 got=0x0000 ref=0x0000 OK
vector "abcde"        expect=0xc8f0 got=0xc8f0 ref=0xc8f0 OK
vector "abcdef"       expect=0x2057 got=0x2057 ref=0x2057 OK
vector "abcdefgh"     expect=0x0627 got=0x0627 ref=0x0627 OK
vector "123456789"    expect=0x1ede got=0x1ede ref=0x1ede OK
vector "a"            expect=0x6161 got=0x6161 ref=0x6161 OK
vector {0xFF}         expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x01,0xFF}    expect=0x0201 got=0x0201 ref=0x0201 OK
differential: 1000000 buffers, 1388938179 bytes total, seed=0xF1E7C416, mismatches=0
fnv1a=0x903aa7957d888495
throughput: 256.0 MiB in 1.171 s = 218.7 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```

## Cross-build comparison

| check | -O2 | -O0 | ASan+UBSan | UBSan |
|---|---|---|---|---|
| vector failures | 0 | 0 | 0 | 0 |
| differential mismatches | 0 | 0 | 0 | 0 |
| fnv1a over all checksums | 0x903aa7957d888495 | 0x903aa7957d888495 | 0x903aa7957d888495 | 0x903aa7957d888495 |
| sanitizer reports | n/a | n/a | 0 | 0 |
| compiler warnings | 0 | 0 | 0 | 0 |
