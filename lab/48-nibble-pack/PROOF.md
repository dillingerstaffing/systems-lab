# PROOF: lab/48-nibble-pack

Genuine build log and run output for all four build configs, captured on
2026-09-09 (gcc 13.3.0, Ubuntu 24.04, x86-64). Every number below is copied
from the run output; nothing is fabricated.

## make test-o0

```
$ make test-o0
gcc -std=c11 -Wall -Wextra -Werror -O0 -g -o test_o0 test_nibblepack.c
./test_o0
vector all-zero -> 0               : pass
vector all-0xF -> 0xFFFFFFFFFFFFFFFF: pass
vector 0..15 -> 0xFEDCBA9876543210 : pass
vector masked inputs -> 0x00EDCBA9870FB10F: pass
vector unpack(0xFEDCBA9876543210) -> 0..15: pass
vectors: 5/5 passed
differential: 262144 pack checks, 262144 unpack checks, 0 mismatches
round trip: 1000000 random words, unpack(pack(n))==n and pack(unpack(w))==w, 0 mismatches
fnv1a-64 over all packed outputs: 0x55bf0e9a9dad34f2
benchmark: 2000000 pack+unpack pairs in 0.155 s = 77.7 ns/pair (sink=8473312217079461738)
ALL TESTS PASSED
```

## make test-o2

```
$ make test-o2
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_o2 test_nibblepack.c
./test_o2
vector all-zero -> 0               : pass
vector all-0xF -> 0xFFFFFFFFFFFFFFFF: pass
vector 0..15 -> 0xFEDCBA9876543210 : pass
vector masked inputs -> 0x00EDCBA9870FB10F: pass
vector unpack(0xFEDCBA9876543210) -> 0..15: pass
vectors: 5/5 passed
differential: 262144 pack checks, 262144 unpack checks, 0 mismatches
round trip: 1000000 random words, unpack(pack(n))==n and pack(unpack(w))==w, 0 mismatches
fnv1a-64 over all packed outputs: 0x55bf0e9a9dad34f2
benchmark: 2000000 pack+unpack pairs in 0.055 s = 27.7 ns/pair (sink=8473312217079461738)
ALL TESTS PASSED
```

## make test-asan

```
$ make test-asan
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-sanitize-recover=all -o test_asan test_nibblepack.c
./test_asan
vector all-zero -> 0               : pass
vector all-0xF -> 0xFFFFFFFFFFFFFFFF: pass
vector 0..15 -> 0xFEDCBA9876543210 : pass
vector masked inputs -> 0x00EDCBA9870FB10F: pass
vector unpack(0xFEDCBA9876543210) -> 0..15: pass
vectors: 5/5 passed
differential: 262144 pack checks, 262144 unpack checks, 0 mismatches
round trip: 1000000 random words, unpack(pack(n))==n and pack(unpack(w))==w, 0 mismatches
fnv1a-64 over all packed outputs: 0x55bf0e9a9dad34f2
benchmark: 2000000 pack+unpack pairs in 0.180 s = 89.8 ns/pair (sink=8473312217079461738)
ALL TESTS PASSED
```

## make test-ubsan

```
$ make test-ubsan
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined \
	-fno-sanitize-recover=all -o test_ubsan test_nibblepack.c
./test_ubsan
vector all-zero -> 0               : pass
vector all-0xF -> 0xFFFFFFFFFFFFFFFF: pass
vector 0..15 -> 0xFEDCBA9876543210 : pass
vector masked inputs -> 0x00EDCBA9870FB10F: pass
vector unpack(0xFEDCBA9876543210) -> 0..15: pass
vectors: 5/5 passed
differential: 262144 pack checks, 262144 unpack checks, 0 mismatches
round trip: 1000000 random words, unpack(pack(n))==n and pack(unpack(w))==w, 0 mismatches
fnv1a-64 over all packed outputs: 0x55bf0e9a9dad34f2
benchmark: 2000000 pack+unpack pairs in 0.282 s = 140.8 ns/pair (sink=8473312217079461738)
ALL TESTS PASSED
```

## Notes

- Zero warnings with `-std=c11 -Wall -Wextra -Werror` on all four configs.
- Both sanitizer builds use `-fno-sanitize-recover=all`, so any report
  would have been fatal; none fired.
- The FNV-1a fingerprint 0x55bf0e9a9dad34f2 is byte-identical across -O0,
  -O2, ASan+UBSan, and UBSan builds, showing all builds computed the same
  packed outputs.
- Timing figures vary run to run; the -O2 throughput reported in README
  (27.7 ns/pair) is from the run captured above.
