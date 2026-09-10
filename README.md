# systems-lab

Small systems-programming modules, each one real, each one verified.
Every module under `lab/` ships with its source, a test suite, and a
`PROOF.md` containing the genuine build log and run output. Nothing here is
a mock or a placeholder: if it does not compile and pass, it is not
committed.

## Modules

- `lab/01-spsc-ring-buffer`: wait-free single-producer/single-consumer ring buffer in C11; correctness rests on the head/tail counter protocol and the empty/full edge behavior.
- `lab/02-bump-allocator`: bump allocator with alignment through 128 bytes and free-list reuse, with OOM behavior pinned against a fixed heap.
- `lab/03-mini-printf`: printf replacement with no libc, supporting %d %u %x %s %c %p; differential-tested against host snprintf.
- `lab/04-crc32`: CRC32 from the generator polynomial 0xEDB88320, both bitwise and table-driven, the table derived at startup from the polynomial.
- `lab/07-lockfree-stack`: Treiber stack in C11 on 16-byte compare-and-swap with a 64-bit ABA tag; the tag check rejects stale observations.
- `lab/05-fuzz-harness`: deterministic fuzzer over the ring buffer and bump allocator; same seed, same digest on every run, clean under the sanitizers.
- `lab/08-seqlock`: sequence lock for single-writer, multi-reader shared state; readers detect torn reads via the sequence counter and retry.
- `lab/06-fault-injection`: single-bit flips injected into ring-buffer frames mid-transfer, caught by the CRC32 check from lab/04; detected with no false positives.
- `lab/09-branchless-bsearch`: binary search over sorted uint32_t with no data-dependent branch in the loop: the comparison becomes a 0/1 integer and the window narrows by arithmetic in exactly floor(log2(n))+1 iterations per key, so hit and miss cost the same by construction.
- `lab/10-from-scratch-memcpy`: memcpy rebuilt from alignment fundamentals: a byte head until the destination is word-aligned, a word body where each source word is assembled from bytes (provably aligned-safe at any source alignment), and a byte tail.

- `lab/11-ieee754`: software IEEE-754 binary32 add and multiply built from the bit layout (sign, exponent, fraction) with integer arithmetic only; exact significand in unsigned __int128, rounded once round-to-nearest-even.
- `lab/12-buddy-allocator`: buddy allocator with power-of-two splitting and xor-buddy coalescing, validated against the lab/02 bump allocator on an identical churn workload; the fragmentation trade-off is stated honestly in PROOF.md.

- `lab/14-align-arith`: alignment and power-of-two rounding primitives (align_up, align_down, is_pow2, round_up_pow2) built from bit identities, differential-checked against division/loop references.
- `lab/17-bitcount`: 64-bit population count via the SWAR parallel-add masks with a multiply-shift fold; disassembly confirms the actual SWAR instruction sequence with no POPCNT emitted.
- `lab/18-endian`: 16/32/64-bit byte-swap built only from shifts, ORs, and masks, plus the involution invariant swap(swap(x)) == x; gcc recognizes the idiom and emits a single bswap per width.

- `lab/15-ticket-ring`: ticket lock on C11 atomics (fetch_add tickets, serving admits in ticket order) guarding a bounded MPMC ring buffer; per-producer FIFO preserved.

- `lab/16-crc32c`: CRC-32C (Castagnoli) with the lookup table derived from the generator polynomial, differential-checked against a bitwise reference.

- `lab/19-saturating-arith`: saturating 32-bit add/sub built from the sign-bit overflow identities.

- `lab/20-xorshift-period`: xorshift16 PRNG; full period verified by exhaustive state-space traversal, zero the only fixed point, matching the theoretical 2^16 - 1 period.

- `lab/21-binary-gcd`: Stein's binary GCD for 32-bit integers, built from the shift/subtract identities.

- `lab/22-ctz`: count-trailing-zeros via the de Bruijn multiply identity with a 32-entry table; disassembly confirms the multiply-and-table construction survives compilation unchanged (no hardware tzcnt substituted).

- `lab/30-memmove-overlap`: byte-level my_memmove built from the overlap copy-direction identity (copy forward when dest < src, backward when dest > src); a directed case shows a naive always-forward copy corrupting backward overlaps while my_memmove stays byte-exact.
- `lab/45-hamming-dist`: Hamming distance of two 64-bit words as d(a, b) = popcount(a ^ b), with the popcount rebuilt from the SWAR parallel-add bit identities (no library popcount wrapped); disassembly confirms the raw SWAR sequence survives compilation with zero popcnt instructions in the object.


- `lab/23-floor-log2`: floor(log2) of a 64-bit word from the shift/OR bit-propagation identity (x |= x >> 1/2/4/8/16/32) plus a from-scratch SWAR popcount minus 1; no hardware clz anywhere in the implementation.

- `lab/24-bit-deposit`: bitfield extract/insert built only from the shift/mask identities, with the n = 64 mask special case handled so no 1ULL << 64 is ever executed (UBSan confirms it never fires); the round-trip invariant insert(0, off, w, extract(x, off, w)) == x & (mask_w << off).

- `lab/25-bit-reversal`: 64-bit bit reversal from the SWAR group-swap identities (swap adjacent bits, then 2-bit groups, then nibbles, then bytes; no bit-reverse builtin anywhere in the source), plus the reverse(reverse(x)) == x involution.

- `lab/26-msb-lsb`: ffs64 (1-based lowest-set-bit index) from the x & -x low-bit isolation identity and fls64 (0-based highest-set-bit index) from the shift/OR smear identity, each mapped through a de Bruijn multiply (* 0x03f79d71b4cb0a89, top 6 bits) and a 64-entry table; the constant's hash distinctness is asserted by the test rather than trusted.

- `lab/33-gray-code`: 16-bit Gray code encode from n ^ (n >> 1) and decode from the xor-fold, plus the decode(encode(x)) == x involution and the single-bit-adjacency property.

- `lab/31-mul-by-constant`: mul_const32(x, k) for 8-bit k built from the distributive law (sum over set bits of k of x << i), using only shifts and adds; the -O2 disassembly was inspected to confirm the shift-add loop survived (no imul).

- `lab/34-branchless-minmax`: branchless bmin32/bmax32 over the full int32_t domain, the operand selected by the sign bit of the exact 64-bit difference (a 32-bit (a - b) >> 31 mask would pick the wrong operand near INT32_MIN/INT32_MAX; the wide difference needs no signed-overflow assumption); -O2 disassembly shows sub/sar/xor/and/xor with no branch and no cmov.

- `lab/27-fixed-point`: Q16.16 fixed-point add/mul built from 64-bit intermediate identities with round-to-nearest.

- `lab/29-djb2-vs-fnv`: djb2 and FNV-1a built straight from their recurrence identities (djb2 via (h << 5) + h, FNV-1a via the prime spelled as shifts and adds); the avalanche comparison between the two is the honest differentiator.

- `lab/32-adler32`: Adler-32 built directly from the rolling-sum recurrence in RFC 1950 section 8.2, with deferred reduction in 5552-byte blocks, the largest block size that cannot overflow 32 bits.
- `lab/40-sign-extend`: sign extension of an arbitrary width w (1..64) to 64 bits via the arithmetic-shift identity (shift left so bit w-1 lands on bit 63, then arithmetic right shift to replicate it); the -O2 disassembly is just shl/sar with the same count register.

- `lab/41-mask-below`: low-n-bit mask built from the 2^n - 1 identity ((1u << n) - 1, with the n=64 shift-count edge handled without undefined behavior); invariants popcount(mask) == n and mask & ~mask == 0 hold on every case.


- `lab/44-bcd-add`: packed BCD add/sub built from the +/-6 nibble correction identities (add 6 when a nibble exceeds 9 after the binary add; subtract 6 on borrow).

- `lab/39-rotr`: 64-bit rotate right/left built from the shift/OR identities ((x >> r) | (x << ((64 - r) & 63)), no narrowing shift), plus the rotr(rotl(x, r), r) == x round-trip invariant.

- `lab/35-parity-fold`: parity64 built only from the xor-fold reduction identity (x ^= x >> 32; 16; 8; 4; 2; 1; return x & 1, folding preserves the bit-XOR), plus the homomorphism parity(a^b) == parity(a)^parity(b).

- `lab/43-byte-permute`: permute64 implementing the fixed non-identity byte permutation P = (2,5,0,7,1,6,3,4) and inv_permute64 implementing its exact inverse INV = (2,4,0,6,7,1,5,3), both as explicit shift/mask composition with hardcoded indices, no tables; inv(permute(x)) == x and permute(inv(x)) == x on every case.

- `lab/46-fletcher16`: Fletcher-16 checksum implemented only from its dual running-sum recurrence (sum1 = (sum1 + byte) % 255, sum2 = (sum2 + sum1) % 255).

- `lab/47-ones-complement-sum`: IPv4-style one's-complement checksum with per-addition end-around carry, 16-bit words assembled from bytes with no casts.

- `lab/48-nibble-pack`: pack 16 4-bit nibbles into one 64-bit word (and unpack) from shift/OR/mask identities only, no tables; the round-trip invariants unpack(pack(n)) == n and pack(unpack(w)) == w.

- `lab/49-div-round-pow2`: round-to-nearest division of a u32 by 2^k from the shift/add-half identities ((x + 2^(k-1)) >> k) with ties rounding up, computed in a 64-bit intermediate so the identity stays exact over the whole u32 domain (the pure 32-bit form wraps, e.g. x=0xFFFFFFFF, k=31 gives 0 instead of 2).

- `lab/50-strnlen-wordscan`: bounded my_strnlen(s, max) from the word-at-a-time zero-byte detection identity ((w - 0x0101..01) & ~w) & 0x8080..80: byte head to word alignment, word body loading only words fully inside [s, s+max) (never reads past max), byte tail; the zero lemma (a zero byte always sets its own 0x80 bit) is proven in PROOF.md, plus a guard-page check with PROT_NONE after.
- `lab/52-bit-interleave`: Morton (Z-order) codes for a 16-bit coordinate pair, morton_interleave/morton_deinterleave from the bit-spreading shift/mask identities only (no tables, no builtins).
- `lab/58-signed-div-pow2`: truncated signed division of int64 by 2^k from the sign-bias identity (x + ((x >> 63) & (2^k - 1))) >> k, matching C's / (truncation toward zero, not floor); the hand-checked vectors include x=-7,k=1 -> -3 and x=-1,k=1 -> 0 where a bare shift rounds the wrong way.

## Building

Each module is self-contained:

```
cd lab/01-spsc-ring-buffer
make run
```

- lab/51-branchless-abs-diff: header-only absdiff64 via sign-mask on wrapped difference; the contract excludes d == INT64_MIN only.

- **lab/56**: 64-bit rotate-left composed with wrapping add from the shift modulo identity.
- **lab/57**: branch-free unsigned saturating subtract from the borrow-out identity.
- **lab/59**: uint64 average without overflow via (a&b) + ((a^b) >> 1).
- **lab/53**: 8-bit to 3-digit packed BCD via shift-and-add-3.
- **lab/55**: longest consecutive zero-bit run via shift/AND cascade.
- **lab/60**: high-n-bit mask via ~0ULL << (64 - n) (n=0 returns 0, n=64 shifts by 0, so no shift by 64 ever executes); popcount and contiguity invariants hold on every case.
- **lab/61**: CRC-16/CCITT in reflected form (poly 0x8408, init 0x0000) from the shift-register recurrence, no table; the catalogue KERMIT check value is pinned.
- **lab/67**: carryless (GF(2)) 64x64 -> 128-bit multiply via the shift-xor identity, no carries; distributivity holds.
- **lab/68**: saturating add of four packed 16-bit lanes in one 64-bit word via the SWAR mask/subtract lane-guard identity (even/odd lane pairs with guard bits, branchless clamp to 0xFFFF).
- **lab/69**: 64-bit integer square root via the bit-by-bit restoring identity, no floating point and no division.
- **lab/70**: high 64 bits of the 128-bit product via the 32-bit splitting identity (no __int128 in the implementation).
- **lab/71**: MurmurHash3 64-bit finalizer built from the shift-xor-multiply avalanche identities, with a verified exact inverse (shift-xor folding plus Newton-Raphson modular inverses of both odd multiply constants); the bijection is checked against the published Appleby reference.
- **lab/72**: 8x8 bit-matrix transpose of a 64-bit word via three SWAR delta-swap stages at distances 7, 14, 28, with an index-bit-swap proof and masks generated by an independent Python script (not copied from any published constant); transpose(transpose(x)) == x on every case.
- **lab/64**: branchless median of three int64s as a 3-element sorting network, each compare-swap from a comparison mask plus bitwise selection (sign-flip with 32-bit-half comparison so the difference can never wrap, exact over the full int64 range); no conditional jumps in the -O2 object.
- **lab/65**: popcount of a 64-bit word via a 256-entry byte table built from the one-bit identity (table[i] = (i & 1) + table[i >> 1], no hard-coded popcounts), word result from the byte-decomposition sum identity.
- **lab/66**: count leading zeros of a 64-bit word from a 256-entry 8-bit table plus the byte-scan identity (most significant nonzero byte sets the lane, zero input defined as 64).
- **lab/73**: low 64 bits of the 64x64 product via the 32-bit splitting identity (cross terms accumulated mod 2^64, no __int128 anywhere in module, test, or implementation, verified by grep).
- **lab/74**: exact (a*b+c) mod 2^64 via the 32-bit splitting identities (four 32x32->64 partial products accumulated with explicit wrap detection, high word accumulated for exactness accounting then discarded, +c by unsigned wraparound, no __int128 in the implementation).
- **lab/75**: count leading ones of a 64-bit word from the invert-then-scan identity clo(x) = clz(~x), with clz built from the shift/OR fill propagation cascade and a SWAR popcount, no clz-class instruction anywhere (verified in the -O2 disassembly: no bsr, lzcnt, or popcnt); the all-ones word is defined by contract as 64.
- **lab/76**: ceiling log2 of a 64-bit word from the fill-cascade and power-of-two identities (no builtin, no floating point).
- **lab/77**: bit span (position of highest set bit minus lowest set bit) from the fill-cascade floor and de Bruijn ctz identities.
- **lab/78**: branchless per-lane unsigned 16-bit max of two packed words in one 64-bit word from the guarded-subtraction comparison identity (one guard bit per lane, minuend lane bounds prove no borrow ever leaves a lane, zero crosstalk).
- **lab/79**: CRC-16/CCITT-FALSE (poly 0x1021 MSB-first, init 0xFFFF) from the shift-register recurrence, with a second table-driven implementation over a 256-entry table derived at startup from the polynomial (table proven by an independent re-derivation); "123456789" matches the CRC catalogue check value and the empty message yields the init value.
- **lab/81**: two's-complement 64-bit negation built only from the ~x + 1 identity (no unary minus in the implementation); INT64_MIN wraps to itself, pinned by contract; invariants (x + neg(x)) == 0 mod 2^64 and neg(neg(x)) == x on every case; the -O2 disassembly is exactly mov/neg/and/ret.
- **lab/82**: CRC-32 residue invariant from the GF(2) polynomial-division identity: crc32(M) appended in little-endian wire order then re-checksummed always equals the residue, using the lab/04 implementation as oracle (big-endian append correctly yields a different residue, documented as contract).
- **lab/83**: count-trailing-zeros via popcount((x ^ (x-1)) >> 1) with hand-rolled SWAR popcount, no builtin ctz wrappers; disassembly contains no tzcnt/bsf/popcnt.
- **lab/84**: round uint64 up to the next power of two from the shift/OR fill cascade on (x-1) then +1, unsigned arithmetic, no builtins; the -O2 disassembly shows the intended construction (sub, five shr/or pairs, add, ret) with no tzcnt/bsf/popcnt/lzcnt.
- **lab/96**: isolate the lowest set bit of a 64-bit word via the two's-complement identity iso(x) = x & -x with the negation as unsigned (0 - x) mod 2^64, no builtins, no intrinsics, no tables; x = 0 returns 0 per contract; the -O2 disassembly is exactly mov/neg/and/ret.
- **lab/103**: single-pass signed divmod by 2^k from the sign-bias identity (bias = (x >> 63) & ((1ULL << k) - 1), quotient = (x + bias) >> k, remainder = x - (quotient << k)); invariants q*2^k+r==x, sign agreement, and |r|<2^k on every case; the -O2 objdump contains no div/idiv.
- **lab/13**: in-place swap of two 64-bit words via three xor-assignments, no temporary; the a==b aliasing case zeroes the word, pinned by contract.
- **lab/97**: 32-bit parity via the 0x6996 nibble-parity constant (fold x to 4 bits with XOR shifts, index the constant); the constant's bit layout is derived by hand in PROOF.md, and the parity homomorphism is checked.
- **lab/106**: two-word (128-bit) unsigned subtraction with borrow propagation, each stage from the borrow-out identity where the (t < b) term is the wrap detector for the b == UINT64_MAX edge; no __int128 in the implementation; the -O2 disassembly is exactly add/setb/and/or/sub/ret with no wide arithmetic.
- **lab/87**: branchless sign of signed 64-bit (-1, 0, +1) via the sign-shift identity ((x >> 63) | ((uint64_t)(-x) >> 63)); branch-free confirmed in the -O2 disassembly.
- **lab/88**: two-word 128-bit increment with carry cascade from the carry identity (carry_out = (lo + 1 == 0)).
- **lab/89**: branchless per-lane absolute value of four packed signed 16-bit lanes in one 64-bit word from the sign-mask identity (x ^ m) - m, with one guard bit per lane (minuend/subtrahend lane bounds prove no borrow ever leaves a lane, zero crosstalk); abs(-32768) = -32768 pinned by contract; abs(abs(x)) == abs(x) and per-lane nonnegativity on every case; branch-free and cmov-free in the -O2 disassembly.
- **lab/91**: unsigned 32-bit division by 3 via the modular-inverse identity q = (x * 0xAAAAAAAB) >> 33, no division operator in the implementation (M = 0xAAAAAAAB = (2^33 + 1) / 3; exactness derived in PROOF.md from x = 3a + r, with an independent Python cross-check); the -O2 disassembly is mov/imul/shr/ret with no div/idiv.
- **lab/90**: bit-reversed index permutation of an 8-element array (out[bitrev(i)] = in[i]), the 3-bit reversal built only from the swap identities, no lookup table, no library call; bijection (every value appears exactly once) and involution both hold on every case.
- **lab/93**: count of zero bytes in a 64-bit word via the zero-byte detection identity; the naive form overcounts because a borrow out of a zero low byte can set a phantom 0x80 bit in the byte above, so the module runs the subtraction in 16-bit lanes with a 0x7F guard per lane and no borrow ever crosses a lane boundary; the -O2 hot path is straight-line integer arithmetic, no byte-by-byte loop.
- **lab/94**: high 64 bits of the signed 128-bit product: from a = ua - sa*2^64, b = ub - sb*2^64, the high word is exactly mulhi_u64(ua,ub) - sa*ub - sb*ua (mod 2^64), the unsigned high word from the 32-bit splitting identity and the sign corrections as conditional subtracts; no __int128 and no full-width signed multiply in the implementation (grep-verified), signed __int128 appears only in the test oracle; the -O2 disassembly is four 32-bit half-width imuls with carry folding and branchless sign corrections, no 64x64 multiply of the original operands.
- **lab/92**: branchless select(sel, a, b) of two uint64 values from the mask identity (mask = -(uint64_t)sel, out = (a & ~mask) | (b & mask)); the -O2 object is verified jump-free.
- **lab/107 (saturate-to-u16)**: branchless clamp of a signed 32-bit value into [0, 65535] from the sign-mask identities; monotonicity and idempotence hold on every case; no conditional jumps in the -O2 disassembly.
- **lab/95**: horizontal sum of four packed unsigned 16-bit lanes in one 64-bit word from the pairwise-add cascade identity (stage 1 sums even/odd lanes with the 0x0000FFFF0000FFFF guard mask into 32-bit fields, stage 2 folds the pair); lane-crosstalk absence proved from the guard bounds (each pair sum at most 0x1FFFE, below 2^32).
- **lab/28**: round-half-to-even u32 halving tie_to_even_half(x) from the bit identity q + ((x & 1) & (q & 1)) with q = x >> 1 (no floats, no division); ties to even (1 -> 0, 3 -> 2, 5 -> 2, 7 -> 4); the -O2 disassembly is exactly shr/and/and/add/ret with no div/idiv.
- **lab/85**: two-word (128-bit) addition with carry propagation, each stage from the carry-out identity carry = (sum < a) | (cin & (sum == a)), where the (cin & (sum == a)) term is the wrap detector for the b == UINT64_MAX edge; no __int128 in the implementation (oracle only); final carry-out checked on every case.
- **lab/86**: saturating 32-bit addition from the overflow-flag identity (result = a + b if no overflow, else INT32_MAX/MIN).
- **lab/100**: branchless three-way unsigned 64-bit compare bcmp64(a, b) returning -1/0/+1 from the borrow-out identity (lt = (a - b) > a, eq = (a == b), result = gt - lt); the -O2 object is verified jump-free.
- **lab/104**: high 16 bits of the 32-bit product of two uint16_t values, mulhi_u16(a, b), from the schoolbook shift-add partial-product identity; no multiply operator anywhere in the implementation (the make disasm step fails the build if one appears).
- **lab/114**: bytewise equality mask of two 64-bit words swar_eqmask64(x, y) from the zero-byte detection identity run in 16-bit lanes with borrow suppression (the naive form false-positives on 0x01-with-borrow); 4,304,967,312 differential checks, 0 mismatches, FNV-1a 731e0818deb17926 identical across -O0/-O2/ASan+UBSan.
