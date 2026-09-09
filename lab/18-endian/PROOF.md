# PROOF.md: lab/18-endian

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then `make run`. Output below is the genuine build log and the
genuine run output, captured verbatim.

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_endian_o0 test_endian.c endian.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_endian_o2 test_endian.c endian.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_endian_asan test_endian.c endian.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
./test_endian_o0
directed16=48 directed32=92 directed64=164
total_cases=12000304 mismatches=0 checksum=9210020399109146351
timed_values=100000000 ns_total=537293096 ns_per_value=5.37 Mvalues_per_sec=186.1
checksum=1133990380442512835
PASS
./test_endian_o2
directed16=48 directed32=92 directed64=164
total_cases=12000304 mismatches=0 checksum=9210020399109146351
timed_values=100000000 ns_total=273546698 ns_per_value=2.74 Mvalues_per_sec=365.6
checksum=1133990380442512835
PASS
./test_endian_asan
directed16=48 directed32=92 directed64=164
total_cases=12000304 mismatches=0 checksum=9210020399109146351
timed_values=100000000 ns_total=437921058 ns_per_value=4.38 Mvalues_per_sec=228.4
checksum=1133990380442512835
PASS
```

All three binaries exit 0. ASan and UBSan report no issues, so the
mask arithmetic and the shifts contain no out-of-bounds access and no
undefined behavior.

## What the numbers mean

- 12,000,304 total checks against `__builtin_bswap16/32/64`, 0
  mismatches. Every check is actually two checks per value: the
  differential comparison and the involution invariant
  `swap(swap(x)) == x`.
- Per width: 2,000,024 differential comparisons for 16-bit, 2,000,046
  for 32-bit, 2,000,082 for 64-bit (152 directed values plus
  2,000,000 fixed-seed splitmix64 random values each, seed
  0x123456789ABCDEF0). The 304 directed checks cover: 0, all-ones,
  each byte position set (0xCD/0xAB/0xEF << 8k), each single bit
  0..15/31/63, `0x01020304` / `0x0102030405060708` patterns and their
  mirrors, `0xDEADBEEF` / `0xDEADBEEFCAFEBABE`, high/low halves, and
  alternating masks `0x00FF00FF...` / `0xFF00FF00...`.
- The checksum 9210020399109146351 is identical across `-O0`, `-O2`,
  and the ASan+UBSan binary, so the PRNG stream and the results are
  bit-for-bit reproducible. The timing checksum 1133990380442512835 is
  likewise identical, so all three binaries timed the same value
  stream.
- Throughput: 100,000,000 fresh fixed-seed values through `u64_swap`,
  timed with CLOCK_MONOTONIC. Honest caveats: the timed loop includes
  the splitmix64 PRNG step per value and accumulates into a checksum so
  the compiler cannot delete the loop, so ns/value is a ceiling for
  the combined loop, not a pure swap measurement. Verified by
  disassembly that at `-O2` gcc recognizes the shift/OR idiom and
  emits a single `bswap` instruction for `u64_swap` (and the analogous
  16/32-bit forms); the source still uses only shifts, ORs, and masks,
  and the differential test proves the semantics match the oracle.
