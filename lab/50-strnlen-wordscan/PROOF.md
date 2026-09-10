<!-- PROOF-HEADER
Checks: 20480
Mismatches: 0
Checksum: 0xfe17a11752e1318c
Throughput: 31.6 ns/value at -O2
Environment: Host
Verdict: PASS
-->
# PROOF: lab/50-strnlen-wordscan

Genuine build log and run output for all four build configs, captured on
2026-09-09 (gcc 13.3.0, Ubuntu 24.04, x86-64). Every number below is copied
from the run output; nothing is fabricated.

Note: during development the UBSan build caught a real out-of-bounds write
in the test program's own vector scratch buffer (`vbuf[64]` was too small
for the len=100 hand vectors), before these final logs were captured. The
bug was in the test harness, not in `my_strnlen`; the buffer was enlarged
to 512 bytes and all four configs were re-run clean. This is why the
sanitizer builds exist.

## make test-o0

```
=== make test-o0 ===
gcc -std=c11 -Wall -Wextra -Werror -O0 -g -o test_o0 test_strnlen.c
./test_o0
vector empty string: len=0 m=0 iz=-1 max=10 -> 0: pass
vector abc at m=3: len=3 m=3 iz=-1 max=10 -> 3: pass
vector abc truncated by max=2: len=3 m=3 iz=-1 max=2 -> 2: pass
vector abc max=3 ends at NUL: len=3 m=3 iz=-1 max=3 -> 3: pass
vector max=0 touches nothing: len=3 m=5 iz=-1 max=0 -> 0: pass
vector interior zero at 1: len=3 m=2 iz=1 max=10 -> 1: pass
vector len 7, head and tail only: len=7 m=1 iz=-1 max=100 -> 7: pass
vector one word, zero at byte 0: len=8 m=4 iz=0 max=16 -> 0: pass
vector one word, zero at byte 3: len=8 m=4 iz=3 max=16 -> 3: pass
vector one word, zero at byte 7: len=8 m=4 iz=7 max=16 -> 7: pass
vector len 16, NUL at end: len=16 m=6 iz=-1 max=100 -> 16: pass
vector len 16, max=15 no NUL in range: len=16 m=6 iz=-1 max=15 -> 15: pass
vector len 100, max=50 no NUL in range: len=100 m=0 iz=-1 max=50 -> 50: pass
vector single byte string: len=1 m=7 iz=-1 max=1 -> 1: pass
vector single byte, max past NUL: len=1 m=7 iz=-1 max=10 -> 1: pass
vectors done
differential: 20480 checks, 0 mismatches
guard: 33 cases, 0 faults, 0 wrong answers: pass
fnv1a-64 over all results: 0xfe17a11752e1318c
benchmark: 20000000 calls in 1.263 s = 63.2 ns/value (sink=3930000000)
ALL TESTS PASSED
```

## make test-o2

```
=== make test-o2 ===
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_o2 test_strnlen.c
./test_o2
vector empty string: len=0 m=0 iz=-1 max=10 -> 0: pass
vector abc at m=3: len=3 m=3 iz=-1 max=10 -> 3: pass
vector abc truncated by max=2: len=3 m=3 iz=-1 max=2 -> 2: pass
vector abc max=3 ends at NUL: len=3 m=3 iz=-1 max=3 -> 3: pass
vector max=0 touches nothing: len=3 m=5 iz=-1 max=0 -> 0: pass
vector interior zero at 1: len=3 m=2 iz=1 max=10 -> 1: pass
vector len 7, head and tail only: len=7 m=1 iz=-1 max=100 -> 7: pass
vector one word, zero at byte 0: len=8 m=4 iz=0 max=16 -> 0: pass
vector one word, zero at byte 3: len=8 m=4 iz=3 max=16 -> 3: pass
vector one word, zero at byte 7: len=8 m=4 iz=7 max=16 -> 7: pass
vector len 16, NUL at end: len=16 m=6 iz=-1 max=100 -> 16: pass
vector len 16, max=15 no NUL in range: len=16 m=6 iz=-1 max=15 -> 15: pass
vector len 100, max=50 no NUL in range: len=100 m=0 iz=-1 max=50 -> 50: pass
vector single byte string: len=1 m=7 iz=-1 max=1 -> 1: pass
vector single byte, max past NUL: len=1 m=7 iz=-1 max=10 -> 1: pass
vectors done
differential: 20480 checks, 0 mismatches
guard: 33 cases, 0 faults, 0 wrong answers: pass
fnv1a-64 over all results: 0xfe17a11752e1318c
benchmark: 20000000 calls in 0.632 s = 31.6 ns/value (sink=3930000000)
ALL TESTS PASSED
```

## make test-asan

```
=== make test-asan ===
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-sanitize-recover=all -o test_asan test_strnlen.c
./test_asan
vector empty string: len=0 m=0 iz=-1 max=10 -> 0: pass
vector abc at m=3: len=3 m=3 iz=-1 max=10 -> 3: pass
vector abc truncated by max=2: len=3 m=3 iz=-1 max=2 -> 2: pass
vector abc max=3 ends at NUL: len=3 m=3 iz=-1 max=3 -> 3: pass
vector max=0 touches nothing: len=3 m=5 iz=-1 max=0 -> 0: pass
vector interior zero at 1: len=3 m=2 iz=1 max=10 -> 1: pass
vector len 7, head and tail only: len=7 m=1 iz=-1 max=100 -> 7: pass
vector one word, zero at byte 0: len=8 m=4 iz=0 max=16 -> 0: pass
vector one word, zero at byte 3: len=8 m=4 iz=3 max=16 -> 3: pass
vector one word, zero at byte 7: len=8 m=4 iz=7 max=16 -> 7: pass
vector len 16, NUL at end: len=16 m=6 iz=-1 max=100 -> 16: pass
vector len 16, max=15 no NUL in range: len=16 m=6 iz=-1 max=15 -> 15: pass
vector len 100, max=50 no NUL in range: len=100 m=0 iz=-1 max=50 -> 50: pass
vector single byte string: len=1 m=7 iz=-1 max=1 -> 1: pass
vector single byte, max past NUL: len=1 m=7 iz=-1 max=10 -> 1: pass
vectors done
differential: 20480 checks, 0 mismatches
guard: 33 cases, 0 faults, 0 wrong answers: pass
fnv1a-64 over all results: 0xfe17a11752e1318c
benchmark: 20000000 calls in 1.574 s = 78.7 ns/value (sink=3930000000)
ALL TESTS PASSED
```

## make test-ubsan

```
=== make test-ubsan ===
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined \
	-fno-sanitize-recover=all -o test_ubsan test_strnlen.c
./test_ubsan
vector empty string: len=0 m=0 iz=-1 max=10 -> 0: pass
vector abc at m=3: len=3 m=3 iz=-1 max=10 -> 3: pass
vector abc truncated by max=2: len=3 m=3 iz=-1 max=2 -> 2: pass
vector abc max=3 ends at NUL: len=3 m=3 iz=-1 max=3 -> 3: pass
vector max=0 touches nothing: len=3 m=5 iz=-1 max=0 -> 0: pass
vector interior zero at 1: len=3 m=2 iz=1 max=10 -> 1: pass
vector len 7, head and tail only: len=7 m=1 iz=-1 max=100 -> 7: pass
vector one word, zero at byte 0: len=8 m=4 iz=0 max=16 -> 0: pass
vector one word, zero at byte 3: len=8 m=4 iz=3 max=16 -> 3: pass
vector one word, zero at byte 7: len=8 m=4 iz=7 max=16 -> 7: pass
vector len 16, NUL at end: len=16 m=6 iz=-1 max=100 -> 16: pass
vector len 16, max=15 no NUL in range: len=16 m=6 iz=-1 max=15 -> 15: pass
vector single byte string: len=1 m=7 iz=-1 max=1 -> 1: pass
vector single byte, max past NUL: len=1 m=7 iz=-1 max=10 -> 1: pass
vectors done
differential: 20480 checks, 0 mismatches
guard: 33 cases, 0 faults, 0 wrong answers: pass
fnv1a-64 over all results: 0xfe17a11752e1318c
benchmark: 20000000 calls in 0.821 s = 41.1 ns/value (sink=3930000000)
ALL TESTS PASSED
```
