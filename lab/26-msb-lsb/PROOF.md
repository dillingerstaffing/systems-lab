# PROOF: lab/26-msb-lsb

## What was built

`ffs64` (1-based lowest-set-bit index, 0 for input 0) and `fls64`
(0-based highest-set-bit index, -1 for input 0) for 64-bit values.
`ffs64` isolates the low bit with `x & -x`; `fls64` smears the high bit
down with the shift/OR identity to an all-ones prefix; each of the 64
possible results is mapped by the de Bruijn multiply `* 0x03f79d71b4cb0a89`
(keeping the top 6 bits) through a 64-entry table back to the bit index.
No bit-scan builtin or library call is used in the implementation.

## What was verified

- The de Bruijn hash property itself: all 64 single-bit hashes and all 64
  all-ones-prefix hashes are pairwise distinct (asserted by the test
  binary before any result comparison).
- Differential test against `__builtin_ffsll` and `63 - __builtin_clzll`:
  all 65536 16-bit inputs plus 133 directed edge cases (0, all-ones,
  0xAA..A, 0x55..5, every 2^k, every 2^(k+1)-1).
- Identical FNV-1a checksum across `-O0`, `-O2`, and ASan+UBSan builds.
- `-Wall -Wextra -Werror` with zero warnings; clean ASan/UBSan run.
- `-O2` disassembly of `ffs64`/`fls64` contains no `tzcnt`, `lzcnt`,
  `bsr`, `bsf`, or `popcnt`: the multiply-and-table construction is what
  executes.

## Build log and run output (genuine, from `make run`, 2026-09-09)

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_msb_lsb_o0 test_msb_lsb.c msb_lsb.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_msb_lsb_o2 test_msb_lsb.c msb_lsb.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_msb_lsb_asan test_msb_lsb.c msb_lsb.c
./test_msb_lsb_o0
checks=131335 mismatches=0 checksum=7928615795610640929
throughput ffs64: 5.62 ns/value over 100000000 timed values
throughput fls64: 8.20 ns/value over 100000000 timed values
./test_msb_lsb_o2
checks=131335 mismatches=0 checksum=7928615795610640929
throughput ffs64: 3.74 ns/value over 100000000 timed values
throughput fls64: 4.35 ns/value over 100000000 timed values
./test_msb_lsb_asan
checks=131335 mismatches=0 checksum=7928615795610640929
throughput ffs64: 5.17 ns/value over 100000000 timed values
throughput fls64: 6.82 ns/value over 100000000 timed values
```

Disassembly sample (`ffs64` at `-O2`, showing the multiply-and-table path):

```
00000000000016a0 <ffs64>:
    16b8:  48 ba 89 0a cb b4 71 9d f7 03   movabs $0x3f79d71b4cb0a89,%rdx
    16c5:  48 f7 d8                        neg    %rax
    16c8:  4c 21 d8                        and    %r11,%rax
    16cb:  48 0f af c2                     imul   %rdx,%rax
    16d6:  48 c1 e8 3a                     shr    $0x3a,%rax
    16da:  0f b6 04 02                     movzbl (%rdx,%rax,1),%eax
    16de:  83 c0 01                        add    $0x1,%eax
```
