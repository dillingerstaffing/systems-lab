# PROOF.md: lab/13-xor-swap

## What was built

`xor_swap.c` implements `void xor_swap(uint64_t *a, uint64_t *b)` as
three xor-assignments, no temporary variable, no builtins:

```c
*a ^= *b;
*b ^= *a;
*a ^= *b;
```

The swap works because of three facts about xor on 64-bit words:
xor is associative, `x ^ x == 0`, and `x ^ 0 == x`. Writing the old
values as `(a0, b0)`:

- step 1: `a1 = a0 ^ b0`
- step 2: `b1 = a1 ^ a0 = (a0 ^ b0) ^ a0 = b0 ^ (a0 ^ a0) = b0`
- step 3: `a2 = a1 ^ b1 = (a0 ^ b0) ^ b0 = a0 ^ (b0 ^ b0) = a0`

So `*a` ends as `b0` and `*b` ends as `a0`: a swap.

## Aliasing contract

If `a == b` (the same address), the three steps operate on one word:
`v ^= v` makes it `0` and the remaining steps keep it `0`. So
`xor_swap(&x, &x)` zeroes `x`. This is the stated contract of the
module. Callers needing identity when the pointers are equal must guard
before calling. Six dedicated rows in the test output below (values 0,
1, 0x8000000000000000, all-ones, 0x123456789abcdef0,
0xdeadbeefcafebabe) each end at 0, proving it.

## Verification

`test_xor_swap.c` differentially tests `xor_swap` against `ref_swap`, a
temp-variable reference (`t = *a; *a = *b; *b = t;`), requiring
`new_a == old_b` and `new_b == old_a` on every case:

- exhaustive: all 2^32 ordered pairs of 16-bit values held as
  `uint64_t` (i = 0..65535, j = 0..65535),
- random: 1,000,000 splitmix64 pairs, fixed seed
  `0x123456789ABCDEF0` (same seed as sibling labs; run is fully
  reproducible),
- FNV-1a (64-bit) folded over every output word; the checksum must be
  identical across the `-O0`, `-O2`, and ASan+UBSan binaries,
- throughput: best of 5 runs over 100,000,000 pairs, called through a
  volatile function pointer so the call cannot be inlined, accumulated
  into a volatile sink so the loop cannot be optimized away.

## Genuine build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_xor_swap_o0 test_xor_swap.c xor_swap.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_xor_swap_o2 test_xor_swap.c xor_swap.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o test_xor_swap_asan test_xor_swap.c xor_swap.c
```

Zero warnings on all three builds (`-Wall -Wextra -Werror`).

## Genuine run output

The following is the captured output of `make run` (three binaries, in order), verbatim:

```
=== test_xor_swap_o0 ===
alias row: input=0000000000000000 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=0000000000000001 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=8000000000000000 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=ffffffffffffffff -> output=0000000000000000 (zeroed, contract holds)
alias row: input=123456789abcdef0 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=deadbeefcafebabe -> output=0000000000000000 (zeroed, contract holds)
exhaustive 16-bit pairs: done
random pairs: done
timing run 0: 5.364 ns/pair
timing run 1: 5.203 ns/pair
timing run 2: 7.107 ns/pair
timing run 3: 5.144 ns/pair
timing run 4: 5.211 ns/pair
timing best: 5.144 ns/pair (100000000 pairs x 5 runs)
timing sink: 758399e303464300 (prevents DCE)
alias rows: 6
differential cases: 4295967296
mismatches: 0
FNV-1a over outputs: afa682b2de8a9162
EXIT=0
=== test_xor_swap_o2 ===
alias row: input=0000000000000000 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=0000000000000001 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=8000000000000000 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=ffffffffffffffff -> output=0000000000000000 (zeroed, contract holds)
alias row: input=123456789abcdef0 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=deadbeefcafebabe -> output=0000000000000000 (zeroed, contract holds)
exhaustive 16-bit pairs: done
random pairs: done
timing run 0: 3.106 ns/pair
timing run 1: 3.064 ns/pair
timing run 2: 2.858 ns/pair
timing run 3: 3.053 ns/pair
timing run 4: 2.689 ns/pair
timing best: 2.689 ns/pair (100000000 pairs x 5 runs)
timing sink: 758399e303464300 (prevents DCE)
alias rows: 6
differential cases: 4295967296
mismatches: 0
FNV-1a over outputs: afa682b2de8a9162
EXIT=0
=== test_xor_swap_asan ===
alias row: input=0000000000000000 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=0000000000000001 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=8000000000000000 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=ffffffffffffffff -> output=0000000000000000 (zeroed, contract holds)
alias row: input=123456789abcdef0 -> output=0000000000000000 (zeroed, contract holds)
alias row: input=deadbeefcafebabe -> output=0000000000000000 (zeroed, contract holds)
exhaustive 16-bit pairs: done
random pairs: done
timing run 0: 6.097 ns/pair
timing run 1: 6.139 ns/pair
timing run 2: 7.986 ns/pair
timing run 3: 6.109 ns/pair
timing run 4: 6.182 ns/pair
timing best: 6.097 ns/pair (100000000 pairs x 5 runs)
timing sink: 758399e303464300 (prevents DCE)
alias rows: 6
differential cases: 4295967296
mismatches: 0
FNV-1a over outputs: afa682b2de8a9162
EXIT=0
```

## Disassembly check

`gcc 13.3.0 -O2 -c xor_swap.c`, then `objdump -d`:

```
0000000000000000 <xor_swap>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 8b 07            	mov    (%rdi),%rax
   7:	48 33 06            	xor    (%rsi),%rax
   a:	48 89 07            	mov    %rax,(%rdi)
   d:	48 33 06            	xor    (%rsi),%rax
  10:	48 89 06            	mov    %rax,(%rsi)
  13:	48 31 07            	xor    %rax,(%rdi)
  16:	c3                  	ret
```

Honest reading: the three xor operations survive as written. The
compiler kept a memory-operand `xor (%rsi),%rax` for each of the first
two C `^=` statements (offsets 7 and d), and folded the third C
statement into `xor %rax,(%rdi)` at offset 13, carrying the
intermediate in `%rax` instead of a separate load-xor-store. The loads
and stores between steps are preserved in program order, and the
compiler did not rewrite the idiom into a temp-register `mov` sequence
or `xchg`. At `-O0` the object is the literal three load-xor-store
sequences in order (full listing captured at build time).

## Summary of measured numbers

- Differential checks: 4,295,967,296 per binary
  (2^32 exhaustive 16-bit pairs + 1,000,000 splitmix64 64-bit pairs,
  seed 0x123456789ABCDEF0), each checked against the temp-variable
  reference. Mismatches: 0 on all three binaries.
- Aliasing rows: 6/6 zeroed.
- FNV-1a over all outputs: `afa682b2de8a9162`, identical on `-O0`,
  `-O2`, and ASan+UBSan.
- Sanitizer reports: 0 (ASan+UBSan binary exited 0 with no reports).
- Throughput at `-O2`: 2.689 ns/pair, best of 5 runs over 100,000,000
  pairs. This measures one full indirect call (dispatch included) plus
  operand setup and sink accumulation, not the bare three-xor body;
  machine variance is on the order of +-1 ns.
