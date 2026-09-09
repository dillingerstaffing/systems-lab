# PROOF.md: lab/29-djb2-vs-fnv

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make` from a clean
checkout, then the three test binaries were run in turn. Output below
is the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_hash_o0 test_hash.c hash.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_hash_o2 test_hash.c hash.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_hash_asan test_hash.c hash.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
=== -O0 ===
known-answer vectors: checked=10 mismatches=0
differential: checked=2000000 mismatches=0
checksum=14048541656418671858
chi-square (digest byte 0, 1M strings): djb2=269.90 fnv1a=256.87
throughput: djb2 592.9 MB/s (3.537 s, 2000 rounds) sink=0
throughput: fnv1a 434.3 MB/s (4.828 s, 2000 rounds) sink=0
PASS
o0_exit=0
=== -O2 ===
known-answer vectors: checked=10 mismatches=0
differential: checked=2000000 mismatches=0
checksum=14048541656418671858
chi-square (digest byte 0, 1M strings): djb2=269.90 fnv1a=256.87
throughput: djb2 477.4 MB/s (4.392 s, 2000 rounds) sink=0
throughput: fnv1a 458.0 MB/s (4.579 s, 2000 rounds) sink=0
PASS
o2_exit=0
=== asan ===
known-answer vectors: checked=10 mismatches=0
differential: checked=2000000 mismatches=0
checksum=14048541656418671858
chi-square (digest byte 0, 1M strings): djb2=269.90 fnv1a=256.87
throughput: djb2 495.5 MB/s (4.232 s, 2000 rounds) sink=0
throughput: fnv1a 453.7 MB/s (4.622 s, 2000 rounds) sink=0
PASS
asan_exit=0
```

## What each line covers

- Known-answer vectors: empty string, `"a"`, `"hello"`, `"The quick
  brown fox jumps over the lazy dog"`, and a 256-byte buffer of bytes
  0..255, each hashed by both functions. The shipped implementations
  are compared against pinned digests and against independently
  written spec implementations (djb2 via `(h << 5) + h` in a pointer
  loop; FNV-1a via the prime spelled as shifts and adds,
  `t + (t<<1) + (t<<4) + (t<<5) + (t<<7) + (t<<8) + (t<<40)`):
  10 checks, 0 mismatches.
- Differential: 1,000,000 fixed-seed splitmix64 random strings
  (lengths 1..64, seed `0xDEADBEEF12345678`), implementation vs spec
  for both hashes: 2,000,000 checks, 0 mismatches.
- Checksum `14048541656418671858` is FNV-1a over the raw bytes of all
  2,000,000 digests, identical under `-O0`, `-O2`, and ASan+UBSan;
  zero sanitizer reports on the full run.
- Chi-square: digest byte 0 over the 256 bins from the same 1,000,000
  strings, expected 3906.25 per bin, 255 degrees of freedom so a
  uniform spread lands near 255. Measured: djb2 = 269.90,
  fnv1a = 256.87 on this sample. One byte position over one input
  sample does not establish general superiority of either hash.
- Throughput: fixed 1 MiB seeded buffer, 2000 rounds, digest folded
  into a sink so the loop cannot be optimized away. At `-O2`: djb2
  477.4 MB/s, fnv1a 458.0 MB/s on this run. The `-O0` binary on the
  same machine measured 592.9 MB/s and 434.3 MB/s, so these are rough
  single-run figures with visible rerun variance on a shared box.

2,000,010 total checks, 0 mismatches.
