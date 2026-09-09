# lab/33-gray-code: proof log

`make clean && make all` on 2026-09-09 (gcc 13.3.0, Ubuntu 24.04).
Two earlier build attempts failed under `-Werror` because the timing
helpers (`measure_encode`, `measure_decode`, `splitmix64`, `ns_now`) were
defined but unused in the non-MEASURE builds; they were moved under
`#ifdef MEASURE` and the clean log below is the final build.

```
=== BUILD LOG (make all) ===
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_gray_o0 test_gray.c gray.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -DMEASURE -O2 -o test_gray_o2 test_gray.c gray.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_gray_asan test_gray.c gray.c
BUILD_EXIT=0
```

Zero warnings at `-Wall -Wextra -Werror` on all three configs. The
ASan+UBSan binary ran the full exhaustive loop (all 65536 inputs, 262,143
total checks) with no sanitizer reports.

```
=== RUN: ./test_gray_o0 ===
encode differential (impl vs bit-loop ref): 65536 checks, 0 mismatches
decode differential (impl vs bit-loop ref): 65536 checks, 0 mismatches
involution decode(encode(x)) == x: 65536 checks, 0 mismatches
adjacent single-bit change: 65535 checks, 0 mismatches
FNV-1a checksum: 9751602672369123877
EXIT=0
=== RUN: ./test_gray_o2 ===
encode differential (impl vs bit-loop ref): 65536 checks, 0 mismatches
decode differential (impl vs bit-loop ref): 65536 checks, 0 mismatches
involution decode(encode(x)) == x: 65536 checks, 0 mismatches
adjacent single-bit change: 65535 checks, 0 mismatches
FNV-1a checksum: 9751602672369123877
encode throughput: 4.46 ns/value (100M values)
decode throughput: 4.35 ns/value (100M values)
EXIT=0
=== RUN: ./test_gray_asan ===
encode differential (impl vs bit-loop ref): 65536 checks, 0 mismatches
decode differential (impl vs bit-loop ref): 65536 checks, 0 mismatches
involution decode(encode(x)) == x: 65536 checks, 0 mismatches
adjacent single-bit change: 65535 checks, 0 mismatches
FNV-1a checksum: 9751602672369123877
EXIT=0
```

Additional timing runs of the `-O2` binary for stability:

```
encode throughput: 4.32 ns/value (100M values)
decode throughput: 4.33 ns/value (100M values)
encode throughput: 4.35 ns/value (100M values)
decode throughput: 4.31 ns/value (100M values)
```

Both primitives sit at roughly 4.3 ns/value. The timed loop includes one
splitmix64 PRNG step per value, so these numbers are a ceiling on the raw
encode/decode rate; the PRNG cost is included honestly rather than
subtracted.
