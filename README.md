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

- `lab/22-ctz`: count-trailing-zeros via the de Bruijn multiply
  identity with a 32-entry table, differential-checked against
  `__builtin_ctz` (all nonzero 16-bit values) and a naive bit-loop
  reference (includes 0): 2,065,536 total checks, 0 mismatches,
  identical FNV-1a checksum 7337941371035615009 across `-O0`, `-O2`,
  and ASan+UBSan builds. Clean under `-Wall -Wextra -Werror`.
  Disassembly confirms the multiply-and-table construction survives
  compilation unchanged (no hardware `tzcnt` substituted). Measured
  at `-O2`: 3.4-5.1 ns/value over 100M timed values (includes the
  PRNG step).

- `lab/30-memmove-overlap`: byte-level `my_memmove` built from the
  overlap copy-direction identity (copy forward when dest < src,
  backward when dest > src), differential-tested against libc
  `memmove`: 8,385 cases (sizes 0..64 x dest offsets -64..+64), 0
  mismatches, identical checksum 13318231107937211255 across `-O0`,
  `-O2`, and ASan+UBSan builds. Clean under `-Wall -Wextra -Werror`.
  A directed case shows a naive always-forward copy corrupting
  backward overlaps while `my_memmove` stays byte-exact. Measured at
  `-O2`: ~1.2 GiB/s vs libc ~43 GiB/s, the gap honestly documented
  as the cost of the byte-at-a-time design.
- `lab/45-hamming-dist`: Hamming distance from the XOR identity
  (`d(a,b) = popcount(a^b)`) with popcount rebuilt from the SWAR
  parallel-add identities from lab/17, differential-tested against a
  naive bit-loop reference: 10,000,073 checks (73 directed edge cases
  plus 10,000,000 fixed-seed random 64-bit pairs), 0 mismatches,
  identical FNV-1a checksum `b80215e1eb7bac94` across `-O0`, `-O2`,
  and ASan+UBSan builds. Disassembly confirms no `popcnt`
  instruction in the binary. Clean under `-Wall -Wextra -Werror`.
  Measured at `-O2`: 5.69 ns/pair (175.6 Mpairs/s) over 100M timed
  pairs (loop includes the PRNG step, so a ceiling).

- `lab/45-hamming-dist`: Hamming distance of two 64-bit words as
  `d(a, b) = popcount(a ^ b)`, with the popcount rebuilt from the SWAR
  parallel-add bit identities (no library popcount wrapped),
  differential-tested against a naive bit-loop reference: 10,000,073
  total checks (73 directed edge cases plus 10,000,000 fixed-seed
  splitmix64 pairs), 0 mismatches, identical FNV-1a checksum
  b80215e1eb7bac94 across `-O0`, `-O2`, and ASan+UBSan builds.
  Disassembly confirms the raw SWAR sequence survives compilation
  with zero `popcnt` instructions in the object. Clean under
  `-Wall -Wextra -Werror`, no sanitizer reports. Measured at `-O2`:
  5.69 ns/pair, 175.6 Mpairs/s over 100M timed pairs (timed loop
  includes two PRNG steps per pair, so this is a ceiling on the raw
  rate).

- `lab/23-floor-log2`: floor(log2) of a 64-bit word from the shift/OR
  bit-propagation identity (`x |= x >> 1/2/4/8/16/32`) plus a from-scratch
  SWAR popcount minus 1, no hardware clz anywhere in the implementation.
  Differential-tested against `63 - __builtin_clzll`: 10,000,193 checks
  (193 boundary cases including 2^k, 2^k +/- 1, 2^k - 1, UINT64_MAX, plus
  10,000,000 fixed-seed splitmix64 values), 0 mismatches, identical
  FNV-1a checksum 16451078516552681775 across `-O0`, `-O2`, and
  ASan+UBSan builds. Disassembly confirms the shift/OR sequence survives
  compilation with no bsr/lzcnt/tzcnt emitted. Measured at `-O2`:
  7.7-7.9 ns/value over 100M timed values (loop includes the PRNG step).

- `lab/24-bit-deposit`: bitfield `extract`/`insert` built only from the
  shift/mask identities, with the n = 64 mask special case handled so no
  `1ULL << 64` is ever executed (UBSan confirms it never fires on a
  shift). Differential-tested against a naive per-bit loop reference:
  79,777,216 total checks (16,777,216 exhaustive: widths 1..8 x offsets
  0..15 x all 65536 16-bit inputs for both primitives; plus 63,000,000
  round-trip cases, widths 1..63 x 1,000,000 fixed-seed splitmix64
  values, checking `insert(0, off, w, extract(x, off, w)) ==
  x & (mask_w << off)`), 0 mismatches, identical FNV-1a checksum
  1148145629208527209 across `-O0`, `-O2`, and ASan+UBSan builds. Clean
  under `-Wall -Wextra -Werror`, no sanitizer reports. Measured at `-O2`:
  ~5.1 ns/case over 100M timed cases (each case includes one PRNG step
  plus one extract and one insert).

- `lab/25-bit-reversal`: 64-bit bit reversal from the SWAR group-swap
  identities (swap adjacent bits, then 2-bit groups, then nibbles, then
  bytes via shift/OR; no bit-reverse builtin anywhere in the source).
  Differential-tested against a naive bit-loop reference that moves bit
  k to position 63 - k, plus the `reverse(reverse(x)) == x` involution
  on every case: 10,262,212 total checks (68 directed values, 262144
  exhaustive 16-bit lanes, 10,000,000 fixed-seed splitmix64 values), 0
  mismatches, identical FNV-1a checksum 8367746376622110355 across
  `-O0`, `-O2`, and ASan+UBSan builds. Disassembly at `-O2` shows the
  size-1/2/4 group swaps surviving as shift/and/or sequences; gcc folds
  the three byte-swap shift/OR stages into one semantics-preserving
  `bswap`. Clean under `-Wall -Wextra -Werror`, no sanitizer reports.
  Measured at `-O2`: ~6.1 ns/value over 100M timed values (loop includes
  the PRNG step, +-1 ns machine variance observed).

- `lab/26-msb-lsb`: `ffs64` (1-based lowest-set-bit index) from the `x & -x`
  low-bit isolation identity and `fls64` (0-based highest-set-bit index)
  from the shift/OR smear identity, each mapped through a de Bruijn
  multiply (`* 0x03f79d71b4cb0a89`, top 6 bits) and a 64-entry table; the
  constant's hash distinctness is asserted by the test rather than trusted,
  and no bit-scan builtin appears in the implementation.
  Differential-tested against `__builtin_ffsll` and
  `63 - __builtin_clzll`: 131,335 checks (all 65536 16-bit inputs plus 133
  directed edges: 0, all-ones, 2^k, 2^(k+1)-1), 0 mismatches, identical
  FNV-1a checksum 7928615795610640929 across `-O0`, `-O2`, and
  ASan+UBSan builds. Clean under `-Wall -Wextra -Werror`. Disassembly at
  `-O2` shows the multiply-and-table construction with no `tzcnt`,
  `lzcnt`, `bsr`, `bsf`, or `popcnt`. Measured at `-O2`: 3.74 ns/value
  (ffs64) and 4.35 ns/value (fls64) over 100M timed values each (loop
  includes the PRNG step).

- `lab/33-gray-code`: 16-bit Gray code `encode` from `n ^ (n >> 1)` and
  `decode` from the xor-fold, differential-tested against a naive
  bit-loop reference over all 65536 16-bit inputs: 262,143 total checks
  (65,536 encode + 65,536 decode + 65,536 `decode(encode(x)) == x`
  involution + 65,535 single-bit-adjacency pairs), 0 mismatches,
  identical FNV-1a checksum 9751602672369123877 across `-O0`, `-O2`,
  and ASan+UBSan builds. Clean under `-Wall -Wextra -Werror`, no
  sanitizer reports. Measured at `-O2`: about 4.3 ns/value for both
  encode and decode over 100M timed values (loop includes the PRNG
  step, so a ceiling).

- `lab/31-mul-by-constant`: `mul_const32(x, k)` for 8-bit `k` built
  from the distributive law (sum over set bits of `k` of `x << i`),
  using only shifts and adds: 256,000,000 differential checks
  against the native product (all 256 constants, 1M fixed-seed
  random 32-bit values each), 0 mismatches, identical checksum
  18440810711898140094 across `-O0`, `-O2`, and ASan+UBSan builds.
  Zero warnings at `-Wall -Wextra -Werror`, no sanitizer reports,
  and the `-O2` disassembly was inspected to confirm the shift-add
  loop survived (no `imul`). Measured at `-O2`: 9.09 ns/multiply
  (110.0 Mops/s).

- `lab/34-branchless-minmax`: branchless `bmin32`/`bmax32` over the full
  `int32_t` domain, the operand selected by the sign bit of the exact
  64-bit difference (a 32-bit `(a - b) >> 31` mask would pick the wrong
  operand near `INT32_MIN`/`INT32_MAX`; the wide difference needs no
  signed-overflow assumption). 4,304,967,345 differential checks
  against the ternary-operator reference (all 2^32 `int16_t` pairs
  exhaustive, 49 directed `INT32_MIN`/`INT32_MAX` edge pairs, 10M
  fixed-seed random 32-bit pairs), 0 mismatches under `-O2`, `-O0`,
  and ASan+UBSan. `-O2` disassembly shows `sub`/`sar`/`xor`/`and`/`xor`
  with no branch and no `cmov`. Measured at `-O2`: 3.650 ns/pair over
  200M timed pairs.

- `lab/27-fixed-point`: Q16.16 fixed-point `add`/`mul` built from 64-bit
  intermediate identities with round-to-nearest, differential-tested
  against a `double` reference: 10,001,352 total cases (1,352 directed
  plus 5M random multiply pairs and 5M random add pairs), 0 mismatches
  within 1 ulp, identical checksum 9619856039744915734 across `-O0`,
  `-O2`, and ASan+UBSan builds. Zero warnings at `-Wall -Wextra
  -Werror`. Measured at `-O2`: 38.78 ns/op (mul) and 29.59 ns/op (add).

- `lab/29-djb2-vs-fnv`: djb2 and FNV-1a built straight from their
  recurrence identities (djb2 via `(h << 5) + h`, FNV-1a via the prime
  spelled as shifts and adds), differential-tested against
  independently written spec implementations plus 10 pinned
  known-answer vectors: 2,000,010 checks, 0 mismatches, identical
  checksum 14048541656418671858 across `-O0`, `-O2`, and ASan+UBSan
  builds. Avalanche measured on 1M random strings (chi-square on
  digest byte 0): djb2 269.90, FNV-1a 256.87, so FNV-1a spreads better.
  Measured at `-O2`: djb2 477.4 MB/s, FNV-1a 458.0 MB/s.

- `lab/32-adler32`: Adler-32 built directly from the rolling-sum
  recurrence in RFC 1950 section 8.2 (`A = 1 + sum of bytes`,
  `B = running sum of A`, both mod 65521), with deferred reduction in
  5552-byte blocks, the largest block size that cannot overflow 32
  bits. 6/6 RFC 1950 known-answer vectors pass. Differential-tested
  against a naive per-byte-modulo reference: 1,000,000 fixed-seed
  random buffers, 3,256,296,802 bytes total, 0 mismatches, with length
  coverage across the 5552 block boundary. Clean under `-O0`, `-O2`,
  ASan+UBSan. Measured at `-O2`: 1627.8 to 1787.9 MiB/s.
- `lab/40-sign-extend`: sign extension of an arbitrary width `w`
  (1..64) to 64 bits via the arithmetic-shift identity (shift left so
  bit `w-1` lands on bit 63, then arithmetic right shift to replicate
  it). Differential-tested against an independent bit-test reference:
  4,194,311 checks (widths 1..63 x all 65536 16-bit inputs, width 64 x
  all 65536 16-bit inputs as an identity check, plus 7 directed edge
  cases including w=64 with INT64_MIN), 0 mismatches, identical FNV-1a
  checksum `261071a94624789d` across `-O0`, `-O2`, and ASan+UBSan
  builds. Disassembly of the `-O2` primitive is just `shl`/`sar` with
  the same count register. Measured at `-O2`: 1.08 ns/value.

- `lab/41-mask-below`: low-n-bit mask built from the `2^n - 1` identity
  (`(1u << n) - 1`, with the n=64 shift-count edge handled without
  undefined behavior). Differential-tested against a loop-built
  reference: all 65 widths times 1,000,000 fixed-seed random 64-bit
  values, 65,000,000 cases, 0 mismatches, invariants `popcount(mask) ==
  n` and `mask & ~mask == 0` holding on every case. FNV-1a checksum
  9899f825c9bda325 identical across runs. Measured at `-O2`: 1.58
  ns/value.


- `lab/44-bcd-add`: packed BCD add/sub built from the +/-6 nibble
  correction identities (add 6 when a nibble exceeds 9 after the
  binary add; subtract 6 on borrow). Differential-tested against a
  decimal-digit naive reference: 20,000 checks, 0 mismatches,
  FNV-1a checksum `ff64fdf0ec2142e5` identical across `-O0`, `-O2`,
  and ASan+UBSan builds. Measured at `-O2`: 3.296 ns/op.

- `lab/39-rotr`: 64-bit rotate right/left built from the shift/OR
  identities (`(x >> r) | (x << ((64 - r) & 63))`, no narrowing
  shift), differential-tested against a naive bit-loop reference and
  the `rotr(rotl(x, r), r) == x` round-trip invariant over all 65,536
  16-bit inputs times all 32 rotation amounts: 4,194,304 total
  checks, 0 mismatches, identical FNV-1a checksum
  501688248194884901 across `-O0`, `-O2`, and ASan+UBSan builds.
  Clean under `-Wall -Wextra -Werror`, no sanitizer reports.
  Measured at `-O2` over 100M timed values: rotr 2.65 to 2.77
  ns/value, rotl 2.65 to 2.66 ns/value (timed loop includes the
  PRNG step, so a ceiling).

- `lab/35-parity-fold`: `parity64` built only from the xor-fold
  reduction identity (`x ^= x >> 32; 16; 8; 4; 2; 1; return x & 1`,
  folding preserves the bit-XOR). Differential-tested against a naive
  per-bit loop: 2,065,536 total checks (65,536 exhaustive 16-bit
  inputs, 1M fixed-seed random 64-bit values, plus the homomorphism
  `parity(a^b) == parity(a)^parity(b)` on 1M pairs), 0 mismatches,
  identical FNV-1a checksum d42f33eaf01cd639 across `-O0`, `-O2`, and
  ASan+UBSan builds. Clean under `-Wall -Wextra -Werror`, no
  sanitizer reports. Measured at `-O2`: 4.26 ns/value (234.6
  Mvalues/s) over 25M timed values.

- `lab/43-byte-permute`: `permute64` implementing the fixed non-identity
  byte permutation P = (2,5,0,7,1,6,3,4) and `inv_permute64`
  implementing its exact inverse INV = (2,4,0,6,7,1,5,3), both as
  explicit shift/mask composition with hardcoded indices, no tables.
  Differential-tested against an independent table-driven byte loop:
  1,065,536 cases (65,536 exhaustive 16-bit inputs plus 1M fixed-seed
  random 64-bit values), 0 mismatches, and the invariants
  `inv(permute(x)) == x` and `permute(inv(x)) == x` held on every
  case. Identical FNV-1a checksum f75c74855e39dfd5 across `-O0`,
  `-O2`, and ASan+UBSan builds. Clean under `-Wall -Wextra
  -Werror`, no sanitizer reports. Measured at `-O2`: 4.424 ns per
  permute/inverse call over 2M calls.

- `lab/46-fletcher16`: Fletcher-16 checksum implemented only from its
  dual running-sum recurrence (`sum1 = (sum1 + byte) % 255`,
  `sum2 = (sum2 + sum1) % 255`). Differential-tested against an
  independent closed-form two-loop weighted-sum reference: 8/8
  published and hand-checked vectors pass ("abcde" -> 0xC8F0,
  "abcdef" -> 0x2057, "abcdefgh" -> 0x0627), plus 1,000,000
  fixed-seed random buffers (1,388,938,179 bytes), 0 mismatches.
  FNV-1a checksum `0x903aa7957d888495` identical across `-O0`,
  `-O2`, ASan+UBSan, and UBSan builds; zero warnings under
  `-std=c11 -Wall -Wextra -Werror`, zero sanitizer reports.
  Measured at `-O2`: 219.8 MiB/s.

- `lab/47-ones-complement-sum`: IPv4-style one's-complement checksum
  with per-addition end-around carry, 16-bit words assembled from
  bytes with no casts. 9/9 hand-computed vectors pass (empty ->
  0xFFFF, carry folding of 0xFFFF+0xFFFF, odd-length padding),
  1,000,000 fixed-seed random buffers (172,733,565 bytes, mixed
  empty/odd/even lengths) differential-tested against a 64-bit
  fold-at-end naive reference, 0 mismatches. FNV-1a checksum
  `0x7ae87b2b2fb7e3e1` identical across `-O0`, `-O2`, ASan+UBSan,
  and UBSan builds; zero warnings under `-std=c11 -Wall -Wextra
  -Werror`, zero sanitizer reports. Measured at `-O2`: 857.4 MiB/s.

- `lab/48-nibble-pack`: pack 16 4-bit nibbles into one 64-bit word
  (and unpack) from shift/OR/mask identities only, no tables.
  262,144 pack plus 262,144 unpack checks (all 65,536 16-bit values
  times 4 nibble lanes) differential-tested against an independent
  bit-loop reference, 0 mismatches; the round-trip invariants
  `unpack(pack(n)) == n` and `pack(unpack(w)) == w` hold on
  1,000,000 fixed-seed random words, 0 mismatches. FNV-1a checksum
  `0x55bf0e9a9dad34f2` identical across `-O0`, `-O2`, ASan+UBSan,
  and UBSan builds; zero warnings under `-std=c11 -Wall -Wextra
  -Werror`, zero sanitizer reports. Measured at `-O2`: 27.7 ns
  per pack+unpack pair.

- `lab/49-div-round-pow2`: round-to-nearest division of a u32 by 2^k
  from the shift/add-half identities `((x + 2^(k-1)) >> k)` with ties
  rounding up, computed in a 64-bit intermediate so the identity stays
  exact over the whole u32 domain (the pure 32-bit form wraps, e.g.
  x=0xFFFFFFFF, k=31 gives 0 instead of 2). 17/17 hand-checked vectors
  pass (ties at k=1/7/32, k=0 identity, k=32 bounds, 32-bit-wrap case),
  differential-tested against the independent rational reference
  `(2x + 2^k) / 2^(k+1)`: 983,040 exhaustive checks (all 16-bit x,
  k=1..15) plus 1,000,000 fixed-seed splitmix64 random full-u32 checks
  (k=1..31), 1,983,040 total, 0 mismatches. FNV-1a checksum
  `0xee4a225fd47d1345` identical across `-O0`, `-O2`, ASan+UBSan, and
  UBSan builds; zero warnings under `-std=c11 -Wall -Wextra -Werror`,
  zero sanitizer reports. Measured at `-O2`: 4.9 ns/value (timed loop
  with a PRNG step, stated as a ceiling).

- `lab/50-strnlen-wordscan`: bounded `my_strnlen(s, max)` from the
  word-at-a-time zero-byte detection identity
  `((w - 0x0101..01) & ~w) & 0x8080..80`: byte head to word alignment,
  word body loading only words fully inside `[s, s+max)` (never reads
  past `max`), byte tail. The zero lemma (a zero byte always sets its
  own 0x80 bit) is proven in PROOF.md. 15/15 hand-checked vectors
  pass; differential-tested against libc `strnlen` over lengths 0..256
  at every misalignment 0..7 with five `max` values: 20,480 checks,
  0 mismatches. Guard-page check (mmaped pages, PROT_NONE after):
  33 terminator-at-boundary and max-at-guard-edge cases, 0 faults,
  under ASan and UBSan. FNV-1a fingerprint `0xfe17a11752e1318c`
  identical across `-O0`, `-O2`, ASan+UBSan, and UBSan builds; zero
  warnings, zero sanitizer reports. Measured at `-O2`: 31.6 ns/value
  (200-byte string, stated as a ceiling).
- `lab/52-bit-interleave`: Morton (Z-order) codes for a 16-bit coordinate
  pair, `morton_interleave`/`morton_deinterleave` from the bit-spreading
  shift/mask identities only (no tables, no builtins). Differential-checked
  against a naive per-bit-loop reference: exhaustive over all 65,536 8-bit
  pairs plus 10,000,000 fixed-seed splitmix64 random 32-bit values on the
  round-trip invariant: 20,196,608 checks, 0 mismatches. FNV-1a fingerprint
  `0x57707ad2ff081ccb` identical across `-O0`, `-O2`, and ASan+UBSan builds;
  zero warnings, zero sanitizer reports. Measured at `-O2`: 8.705 ns/pair
  (round-trip loop, stated as a ceiling since it includes the splitmix64 step).
- `lab/58-signed-div-pow2`: truncated signed division of int64 by 2^k from
  the sign-bias identity `(x + ((x >> 63) & (2^k - 1))) >> k`, matching C's
  `/` (truncation toward zero, not floor). 17/17 hand-checked vectors pass,
  including `x=-7,k=1 -> -3` and `x=-1,k=1 -> 0` where a bare shift rounds
  the wrong way; differential-tested against C division over all 16-bit
  inputs at k=1..15 plus 1,000,000 fixed-seed random 64-bit pairs at
  k=1..63: 1,983,057 checks, 0 mismatches. FNV-1a fingerprint
  `0x3fbd3c3962be4d43` identical across `-O0`, `-O2`, ASan+UBSan, and UBSan
  builds; zero warnings, zero sanitizer reports. Measured at `-O2`:
  2.3 ns/value (stated as a ceiling).

## Building

Each module is self-contained:

```
cd lab/01-spsc-ring-buffer
make run
```

- lab/51-branchless-abs-diff: header-only absdiff64 via sign-mask on wrapped difference, 4.295B checks (4.29e9 16-bit exhaustive + 1M random) with 0 mismatches vs llabs, contract excludes d==INT64_MIN only, verified at -O2/-O0/ASan

- **lab/56**: 64-bit rotate-left composed with wrapping add from the shift modulo identity; 64,000,000 differential checks (64 amounts x 1M values), 0 mismatches, 10.51 ns/value
- **lab/57**: branch-free unsigned saturating subtract from the borrow-out identity; 4,304,967,296 checks (2^16 x 2^16 exhaustive + 10M random), 0 mismatches, 4.300 ns/pair
- **lab/58**: signed truncating division by 2^k via sign-bias formula; 1,983,057 checks, 0 mismatches, 2.3 ns/value
- **lab/59**: uint64 average without overflow via (a&b)+((a^b)>>1); 4,304,967,440 checks, 0 mismatches, 2.059 ns/pair
- **lab/53**: 8-bit to 3-digit packed BCD via shift-and-add-3; 1280 differential checks over all 256 inputs, 0 mismatches vs snprintf, 54.468 ns/value
- **lab/55**: longest consecutive zero-bit run via shift/AND cascade; 1,065,669 differential checks, 0 mismatches, 31.47 ns/value
- **lab/60**: high-n-bit mask via `~0ULL << (64 - n)` (n=0 returns 0, n=64 shifts by 0, no shift by 64 ever executes); 65,000,000 differential checks over all 65 widths x 1M fixed-seed values, 0 mismatches, popcount/contiguity invariants hold on every case, FNV-1a 0xcf623e770a179ce5 identical across -O0/-O2/ASan+UBSan, 3.379 ns/value
- **lab/61**: CRC-16/CCITT in reflected form (poly 0x8408, init 0x0000, no table); 1,000,000 fixed-seed differential buffers (127,856,024 bytes), 0 mismatches vs independent MSB-first reference, 5/5 known-answer vectors pass including the catalogue KERMIT check value 0x2189, FNV-1a 0x8c5660cc04fdd62d identical across -O0/-O2/ASan+UBSan, 53.9 MiB/s at -O2
- **lab/67**: carryless (GF(2)) 64x64 -> 128-bit multiply via the shift-xor identity, no carries; 4,295,967,296 differential checks (2^32 exhaustive 16-bit pairs + 1,000,000 fixed-seed 64-bit pairs), 0 mismatches vs independent recurrence and literal convolution reference, distributivity holds on all 1,000,000 random triples, 8/8 hand-derived vectors pass, FNV-1a 0xbe0ed5a4db5813fc identical across -O0/-O2/ASan+UBSan, 102.53 ns/value at -O2
- **lab/68**: saturating add of four packed 16-bit lanes in one 64-bit word via the SWAR mask/subtract lane-guard identity (even/odd lane pairs with guard bits, branchless clamp to 0xFFFF); 4,296,646,922 differential checks (2^32 exhaustive lane pairs + 1,679,616 directed crosstalk vectors + 10 hand-derived vectors), 0 mismatches vs independent per-lane scalar reference, FNV-1a 0x8de5b49f4d62f90d (-O2 exhaustive) and 0xa2f2b90587bcb5b1 identical across -O0/-O2/ASan+UBSan quick slice, 4.59 ns/op at -O2
- **lab/69**: 64-bit integer square root via the bit-by-bit restoring identity, no floating point and no division; 1,065,536 differential checks, 0 mismatches vs naive loop reference, checksum 6c88bc16097c16f0 identical across runs, 457.67 ns/value at -O2
- **lab/70**: high 64 bits of the 128-bit product via the 32-bit splitting identity (no __int128 in the implementation); 4,304,971,681 differential checks, 0 mismatches vs unsigned __int128 reference, checksum 0x309d79ae453ace68 identical across -O0/-O2/ASan+UBSan, 2.8 ns/pair at -O2
- **lab/71**: MurmurHash3 64-bit finalizer built from the shift-xor-multiply avalanche identities, with a verified exact inverse (shift-xor folding plus Newton-Raphson modular inverses of both odd multiply constants); 10,000,162 total verification cases (10M fixed-seed random values, 161 directed edges), 0 differential and 0 bijection mismatches vs the published Appleby reference, FNV-1a checksum 0xa893938fd43cc2de, 1.491 ns/value (670.5 M values/s) best of 5 at -O2, zero warnings under -Wall -Wextra -Werror, zero sanitizer findings under ASan+UBSan
- **lab/72**: 8x8 bit-matrix transpose of a 64-bit word via three SWAR delta-swap stages at distances 7, 14, 28, with index-bit-swap proof and masks generated by an independent Python script (not copied from any published constant); 10,065,622 total verification cases (exhaustive 16-bit inputs, 10M fixed-seed random words, directed edges), 0 mismatches vs naive per-bit reference, transpose(transpose(x)) == x on every case, FNV-1a checksum 0xdb2cc52bd54d6a06, 3.031 ns/value (329.9 M values/s) best of 5 at -O2, zero warnings under -Wall -Wextra -Werror, zero sanitizer findings under ASan+UBSan
- **lab/65**: popcount of a 64-bit word via a 256-entry byte table built from the one-bit identity (table[i] = (i & 1) + table[i >> 1], no hard-coded popcounts), word result from the byte-decomposition sum identity; 1,065,792 differential checks (256 table entries + 65,536 exhaustive 16-bit inputs + 1,000,000 fixed-seed 64-bit values), 0 mismatches vs __builtin_popcountll, FNV-1a 0xe1fc995406ad5592 identical across -O0/-O2/ASan+UBSan, 7.405 ns/value at -O2
- **lab/66**: count leading zeros of a 64-bit word from a 256-entry 8-bit table plus the byte-scan identity (most significant nonzero byte sets the lane, zero input defined as 64); 1,065,866 differential checks (256 table re-derivations + 74 directed vectors + 65,536 exhaustive 16-bit inputs + 1,000,000 fixed-seed 64-bit values), 0 mismatches vs __builtin_clzll, FNV-1a 0x45c10f152e119ecb identical across -O0/-O2/ASan+UBSan, 1.767 ns/value at -O2
- **lab/73**: low 64 bits of the 64x64 product via the 32-bit splitting identity (cross terms accumulated mod 2^64, no __int128 anywhere in module, test, or implementation, verified by grep); 4,304,971,681 differential checks (2^32 exhaustive 16-bit pairs + 10M fixed-seed splitmix64 64-bit pairs + directed carry-state edges), 0 mismatches vs native unsigned 64-bit product reference, FNV-1a checksum 0x9c7c2747bf0beb25 identical across -O0/-O2/ASan+UBSan, 1.645 ns/pair best of 5 at -O2 (607.9 M pairs/s), zero warnings under -Wall -Wextra -Werror, zero sanitizer findings
- **lab/74**: exact (a*b+c) mod 2^64 via the 32-bit splitting identities (four 32x32->64 partial products accumulated with explicit wrap detection, high word accumulated for exactness accounting then discarded, +c by unsigned wraparound, no __int128 in the implementation); 34,369,738,593 differential checks (2^32 16-bit (a,b) pairs x 8 c values + 10M fixed-seed splitmix64 64-bit triples + 218 directed carry/sweep edges + 7 anchors), 0 mismatches vs unsigned __int128 oracle, FNV-1a checksum 0xee898af0ca125a96 identical across -O0/-O2/ASan+UBSan, 2.144 ns/triple best of 5 at -O2 (466.3 M triples/s), zero warnings under -Wall -Wextra -Werror, zero sanitizer findings
- **lab/75**: count leading ones of a 64-bit word from the invert-then-scan identity `clo(x) = clz(~x)`, with clz built from the shift/OR fill propagation cascade and a SWAR popcount, no clz-class instruction anywhere (verified in the -O2 disassembly: no bsr, lzcnt, or popcnt); zero input of the scan (all-ones word) defined by contract as 64; 10,065,619 differential checks (65,536 exhaustive 16-bit inputs + 10M fixed-seed splitmix64 64-bit words + 73 directed run-length/pattern edges + 10 anchors), 0 mismatches vs naive bit-scan reference, FNV-1a checksum 0x89c5b240295bdfcc identical across -O0/-O2/ASan+UBSan, 5.483 ns/value best of 5 at -O2 (182.4 M values/s, 8.267 ns/value with the PRNG in the timed loop), zero warnings under -Wall -Wextra -Werror, zero sanitizer findings
