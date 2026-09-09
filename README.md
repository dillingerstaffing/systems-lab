# systems-lab

Small systems-programming modules, each one real, each one verified.
Every module under `lab/` ships with its source, a test suite, and a
`PROOF.md` containing the genuine build log and run output. Nothing here is
a mock or a placeholder: if it does not compile and pass, it is not
committed.

## Modules

- `lab/01-spsc-ring-buffer`: wait-free single-producer/single-consumer ring
  buffer in C11. 10M-item two-thread stress test with order checking and
  checksum, measured at 190.4 Mops/s, plus empty/full edge tests and
  head/tail counter wrap-around tests.
- `lab/02-bump-allocator`: bump allocator with alignment through 128 bytes,
  free-list reuse, and OOM behavior verified in a 512-byte heap. 512 churn
  allocations measured a 0.357 fragmentation ratio at 19,306.6 kops/s.
  Clean under ASan and UBSan.
- `lab/03-mini-printf`: printf replacement with no libc, supporting
  `%d %u %x %s %c %p`. 609 differential cases against host `snprintf`,
  zero failures, 5104 bytes of text. Clean under `-O0`, ASan, and UBSan.
- `lab/04-crc32`: CRC32 from the generator polynomial 0xEDB88320, both
  bitwise and table-driven, the table derived at startup. 7 known-answer
  inputs, 2,088 randomized cross-checks, all 256 table entries verified:
  2,358 checks, zero failures. 56.1 MB/s bitwise vs 231.3 MB/s
  table-driven on 32 MiB, a 4.1x speedup.
- `lab/07-lockfree-stack`: Treiber stack in C11 on 16-byte compare-and-swap
  with a 64-bit ABA tag. Four threads churned 1.6M operations: 262,144
  pushed, 262,144 popped, 0 lost, 0 duplicated, 28,150 stale observations
  rejected by the tag check.
- `lab/05-fuzz-harness`: deterministic fuzzer over the ring buffer and
  bump allocator. Same seed, same digest on every run: 6M operations under
  ASan and UBSan, zero violations.
- `lab/08-seqlock`: sequence lock for single-writer, multi-reader shared
  state. One writer, 4 readers: 10M reads, 0 torn reads, 0 monotonicity
  violations.
- `lab/06-fault-injection`: single-bit flips injected into ring-buffer
  frames mid-transfer, caught by the CRC32 check from lab/04. 200,000
  frames (3.4M words), 6,250 injected corruptions: 6,250 detected, 0
  missed, 0 false positives.
- `lab/09-branchless-bsearch`: binary search over sorted `uint32_t` with
  no data-dependent branch in the loop: the comparison becomes a 0/1
  integer and the window narrows by arithmetic, exactly
  floor(log2(n))+1 iterations for every key. 211,266 differential checks
  against libc `bsearch` plus 4M timed lookups, 0 mismatches; hit and
  miss cost the same by construction. Measured with `rdtsc` on identical
  key sets: at n=16384 (cache-resident) 214-300 cycles/lookup vs libc
  `bsearch` 227-324; at n=1048576 branchless 585-692 vs `bsearch`
  486-514. Clean under `-O0`, ASan, and UBSan.
- `lab/10-from-scratch-memcpy`: `memcpy` rebuilt from alignment
  fundamentals: byte head until the destination is word-aligned, a word
  body where each source word is assembled from bytes (provably
  aligned-safe at any source alignment), and a byte tail. 1,004,160
  differential cases against libc `memcpy` (exhaustive sizes 0-64 at all
  64 misalignment combinations, plus 1M random sizes to 64 KiB),
  0 mismatches. Clean under ASan and UBSan. Measured 1338.8-1398.8
  MiB/s vs libc 20085.1-21635.4 on the same host, the gap stated
  honestly in PROOF.md.

- `lab/11-ieee754`: software IEEE-754 binary32 add and multiply built
  from the bit layout (sign, exponent, fraction) with integer arithmetic
  only, zero float operations in the implementation; exact significand in
  `unsigned __int128`, rounded once round-to-nearest-even. 4,000,152
  differential comparisons against the hardware FPU (38 directed edge
  pairs: infinities, signed zeros, subnormal boundaries, SNaN/QNaN,
  rounding ties, plus 1,000,000 biased random pairs), 0 mismatches.
- `lab/12-buddy-allocator`: buddy allocator with power-of-two splitting
  and xor-buddy coalescing, validated against the lab/02 bump allocator
  on an identical churn workload. Checkerboard pattern fragmentation:
  buddy 0.789 vs bump 0.357; large-block success 5/64 vs 30/64, the
  trade-off stated honestly in PROOF.md. Free-all coalesces the 64 KiB
  heap back to one block.

- `lab/14-align-arith`: alignment and power-of-two rounding primitives
  (`align_up`, `align_down`, `is_pow2`, `round_up_pow2`) built from bit
  identities, differential-checked against division/loop references:
  4,200,728 total checks across the four primitives, 0 mismatches:
  97-input boundary sweep over all 32 power-of-two alignments (6,208
  checks on align_up/align_down, 194 on is_pow2/round_up_pow2), two
  loops of 1,048,576 fixed-seed random 32-bit values checked against
  both primitives per loop (4,194,304 checks), and 22 explicit edge
  cases (identity, zero, UINT32_MAX, round-up overflow). Zero warnings
  at
  `-Wall -Wextra -Werror` under `-O0`, `-O2`, ASan, and UBSan.
- `lab/17-bitcount`: 64-bit population count via the SWAR
  parallel-add masks (0x5555..., 0x3333..., 0x0F0F...) with a
  multiply-shift fold, differential-checked against
  `__builtin_popcountll`: 10,000,133 total checks (133 directed edge
  cases: 0, all-ones, every single-bit position 0..63, alternating
  patterns, 2^k - 1 for k = 0..64; plus 10,000,000 fixed-seed random
  64-bit values), 0 mismatches, identical checksums under `-O0`,
  `-O2`, ASan, and UBSan. Throughput measured at 3.98 ns/value
  (251.4 Mvalues/s) at `-O2` on the actual SWAR instruction sequence
  (verified by disassembly, no POPCNT emitted; timed loop includes
  the PRNG step, so this is a ceiling).
- `lab/18-endian`: 16/32/64-bit byte-swap built only from shifts,
  ORs, and masks, differential-checked against
  `__builtin_bswap16/32/64`: 12,000,304 total checks, 0 mismatches
  (2,000,024 + 2,000,046 + 2,000,082 differential comparisons per
  width, 152 directed edge cases plus 2,000,000 fixed-seed splitmix64
  random values each; every value also verified against the
  involution invariant `swap(swap(x)) == x`). Checksums identical
  across `-O0`, `-O2`, and ASan+UBSan builds; zero warnings under
  `-Wall -Wextra -Werror`. Throughput measured at 2.74 ns/value
  (365.6 Mvalues/s) at `-O2` on u64_swap (timed loop includes the
  PRNG step, so this is a ceiling); disassembly confirms gcc
  recognizes the idiom and emits a single `bswap` per width.

- `lab/15-ticket-ring`: ticket lock on C11 atomics (`fetch_add`
  tickets, serving admits in ticket order) guarding a bounded MPMC ring
  buffer. Verified with 4 producers x 250k items: 1M drained, 0 lost,
  0 duplicates, per-producer FIFO preserved, checksum 499999500000
  matches; worst fairness gap 4 against ideal 3. Clean under
  `-Wall -Wextra -Werror`, ASan, and UBSan.

- `lab/16-crc32c`: CRC-32C (Castagnoli) with the lookup table derived
  from the generator polynomial 0x82F63B78 (all 256 entries match a
  direct computation) and differential-checked against a bitwise
  reference: 7 known-answer vectors (including RFC 3720 vectors) x 2
  implementations all match published values, and 100,321 comparisons
  (256 exhaustive 1-byte inputs, 65 sizes, 100,000 random buffers) with
  0 mismatches. Measured throughput: bitwise 35.5 MiB/s vs table
  139.9 MiB/s.

- `lab/19-saturating-arith`: saturating 32-bit add/sub built from the
  sign-bit overflow identities, differential-tested against a 64-bit
  reference: 10,000,338 total checks (338 directed edge cases plus
  10,000,000 random pairs), 0 mismatches, identical checksum
  1390331946882605843 across `-O0`, `-O2`, and ASan+UBSan builds. Clean
  under `-Wall -Wextra -Werror`. Measured at `-O2`: 18.2 ns/add,
  17.7 ns/sub.

- `lab/20-xorshift-period`: xorshift16 PRNG, full period verified by
  exhaustive state-space traversal: seeds 0x0001 and 0xbeef each visit
  all 65,535 nonzero states exactly once before returning to the seed
  (zero confirmed as the only fixed point), matching the theoretical
  2^16 - 1 period. Clean under `-Wall -Wextra -Werror`, `-O0`, `-O2`,
  ASan, and UBSan. Measured at `-O2`: about 4.9 ns/step.

- `lab/21-binary-gcd`: Stein's binary GCD for 32-bit integers, built
  from the shift/subtract identities, differential-tested against a
  naive Euclid reference: 2,048,576 total checks (1,048,576 exhaustive
  0..1023 pairs plus 1,000,000 fixed-seed random 32-bit pairs), 0
  mismatches, identical FNV-1a checksum 11576187220487214191 across
  `-O0`, `-O2`, and ASan+UBSan builds. Clean under
  `-Wall -Wextra -Werror`. Measured at `-O2`: about 528 ns per
  fully-checked pair (includes the PRNG step, oracle comparison, and
  checksum).

## Building

Each module is self-contained:

```
cd lab/01-spsc-ring-buffer
make run
```
