# lab/29-djb2-vs-fnv

`hash.c` (`hash.h` declares them): two string hash functions, each built
from its recurrence identity and nothing else, no library hash wrapped:

- `uint64_t djb2_64(const unsigned char *data, size_t len)`: the
  64-bit variant, `h = h * 33 + c` per byte, seed 5381.
- `uint64_t fnv1a_64(const unsigned char *data, size_t len)`:
  `h = (h ^ c) * 1099511628211` per byte, offset basis
  14695981039346656037. The xor-before-multiply ordering is what makes
  it FNV-1a rather than FNV-1.

All arithmetic is unsigned 64-bit, so overflow wraps modulo 2^64 and is
well-defined; zero bytes inside the input are hashed as ordinary bytes
because the length is explicit.

Verified by `test_hash.c` (fixed seeds, fully reproducible):

- Known-answer vectors: empty string, `"a"`, `"hello"`,
  `"The quick brown fox jumps over the lazy dog"`, and a 256-byte
  buffer holding bytes 0..255. Both shipped functions are checked
  against pinned digests and against independently written spec
  implementations (djb2: `(h << 5) + h` pointer loop; FNV-1a: the
  prime 1099511628211 as `t + (t<<1) + (t<<4) + (t<<5) + (t<<7) +
  (t<<8) + (t<<40)`): 10 checks, 0 mismatches.
- Differential: 1,000,000 fixed-seed splitmix64 random strings
  (lengths 1..64, seed `0xDEADBEEF12345678`), impl vs spec for both
  hashes: 2,000,000 checks, 0 mismatches.

2,000,010 total checks, 0 mismatches. Identical cross-build checksum
(`14048541656418671858`) under `-O0`, `-O2`, and ASan+UBSan; zero
sanitizer reports.

Spread measurement: chi-square of digest byte 0 over the 256 bins from
the same 1,000,000 strings (expected 3906.25 per bin; uniform lands near
255 with 255 degrees of freedom): djb2 = 269.90, fnv1a = 256.87 on
this sample. Honest caveat: this is one byte position over one input
sample, so it does not establish that either hash spreads better in
general.

Throughput at `-O2` (fixed 1 MiB seeded buffer, 2000 rounds, digest
folded into a sink so the loop cannot be optimized away): djb2 477.4
MB/s, fnv1a 458.0 MB/s on this run. These are rough single-run figures
on a shared box; rerun variance is visible (the `-O0` binary measured
592.9 MB/s and 434.3 MB/s for the same loops).

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
