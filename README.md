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

## Building

Each module is self-contained:

```
cd lab/01-spsc-ring-buffer
make run
```
