<!-- PROOF-HEADER
Checks: 1000000
Mismatches: 0
Checksum: 0x7ae87b2b2fb7e3e1
Throughput: 857.4 MiB/s at -O2 (256.0 MiB in 0.299 s)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/47-ones-complement-sum

Environment: gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64.

All four targets compile with `-std=c11 -Wall -Wextra -Werror`.
Zero warnings (any warning would fail the build). Sanitizer builds
use `-fno-sanitize-recover=all`, so any report would be fatal; none
appeared.

The differential reference (test_onesum.c, `ref_onesum`) accumulates
every 16-bit word into a 64-bit total with no per-addition folding and
folds carries only once at the end, sharing no update logic with the
per-addition end-around carry in onesum.h.

## Runs

### -O2 (`make test-o2`)

```
$ make test-o2
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_o2 test_onesum.c
./test_o2
vector empty                  expect=0xffff got=0xffff ref=0xffff OK
vector {0x00}                 expect=0xffff got=0xffff ref=0xffff OK
vector {0xFF}                 expect=0x00ff got=0x00ff ref=0x00ff OK
vector zeros4                 expect=0xffff got=0xffff ref=0xffff OK
vector ones4                  expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x00,0x01,0xF2,0x03}  expect=0x0dfb got=0x0dfb ref=0x0dfb OK
vector {0xFF,0xFF,0x00,0x01}  expect=0xfffe got=0xfffe ref=0xfffe OK
vector {0x01,0x02,0x03}       expect=0xfbfd got=0xfbfd ref=0xfbfd OK
vector {0x12,0x34,0x56}       expect=0x97cb got=0x97cb ref=0x97cb OK
differential: 1000000 buffers, 172733565 bytes total, seed=0x123456789ABCDEF0, mismatches=0
fnv1a=0x7ae87b2b2fb7e3e1
throughput: 256.0 MiB in 0.299 s = 857.4 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```

### -O0 (`make test-o0`)

```
$ gcc -std=c11 -Wall -Wextra -Werror -O0 -g -o test_o0 test_onesum.c
./test_o0
vector empty                  expect=0xffff got=0xffff ref=0xffff OK
vector {0x00}                 expect=0xffff got=0xffff ref=0xffff OK
vector {0xFF}                 expect=0x00ff got=0x00ff ref=0x00ff OK
vector zeros4                 expect=0xffff got=0xffff ref=0xffff OK
vector ones4                  expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x00,0x01,0xF2,0x03}  expect=0x0dfb got=0x0dfb ref=0x0dfb OK
vector {0xFF,0xFF,0x00,0x01}  expect=0xfffe got=0xfffe ref=0xfffe OK
vector {0x01,0x02,0x03}       expect=0xfbfd got=0xfbfd ref=0xfbfd OK
vector {0x12,0x34,0x56}       expect=0x97cb got=0x97cb ref=0x97cb OK
differential: 1000000 buffers, 172733565 bytes total, seed=0x123456789ABCDEF0, mismatches=0
fnv1a=0x7ae87b2b2fb7e3e1
throughput: 256.0 MiB in 1.496 s = 171.1 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```

### ASan+UBSan (`make test-asan`)

```
$ gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-sanitize-recover=all -o test_asan test_onesum.c
./test_asan
vector empty                  expect=0xffff got=0xffff ref=0xffff OK
vector {0x00}                 expect=0xffff got=0xffff ref=0xffff OK
vector {0xFF}                 expect=0x00ff got=0x00ff ref=0x00ff OK
vector zeros4                 expect=0xffff got=0xffff ref=0xffff OK
vector ones4                  expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x00,0x01,0xF2,0x03}  expect=0x0dfb got=0x0dfb ref=0x0dfb OK
vector {0xFF,0xFF,0x00,0x01}  expect=0xfffe got=0xfffe ref=0xfffe OK
vector {0x01,0x02,0x03}       expect=0xfbfd got=0xfbfd ref=0xfbfd OK
vector {0x12,0x34,0x56}       expect=0x97cb got=0x97cb ref=0x97cb OK
differential: 1000000 buffers, 172733565 bytes total, seed=0x123456789ABCDEF0, mismatches=0
fnv1a=0x7ae87b2b2fb7e3e1
throughput: 256.0 MiB in 0.478 s = 535.9 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```

### UBSan (`make test-ubsan`)

```
$ gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined \
	-fno-sanitize-recover=all -o test_ubsan test_onesum.c
./test_ubsan
vector empty                  expect=0xffff got=0xffff ref=0xffff OK
vector {0x00}                 expect=0xffff got=0xffff ref=0xffff OK
vector {0xFF}                 expect=0x00ff got=0x00ff ref=0x00ff OK
vector zeros4                 expect=0xffff got=0xffff ref=0xffff OK
vector ones4                  expect=0x0000 got=0x0000 ref=0x0000 OK
vector {0x00,0x01,0xF2,0x03}  expect=0x0dfb got=0x0dfb ref=0x0dfb OK
vector {0xFF,0xFF,0x00,0x01}  expect=0xfffe got=0xfffe ref=0xfffe OK
vector {0x01,0x02,0x03}       expect=0xfbfd got=0xfbfd ref=0xfbfd OK
vector {0x12,0x34,0x56}       expect=0x97cb got=0x97cb ref=0x97cb OK
differential: 1000000 buffers, 172733565 bytes total, seed=0x123456789ABCDEF0, mismatches=0
fnv1a=0x7ae87b2b2fb7e3e1
throughput: 256.0 MiB in 0.331 s = 773.8 MiB/s (checksum folded: 0x00000000, printed so the loop is not dead code)
RESULT: PASS
```
