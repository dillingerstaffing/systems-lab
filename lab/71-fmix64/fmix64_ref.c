/*
 * fmix64_reference: the published MurmurHash3 64-bit finalization
 * mix, copied verbatim from Austin Appleby's MurmurHash3.cpp
 * (the fmix64 helper of MurmurHash3_x64_128), which is placed in the
 * public domain by its author.  The symbol is renamed so this
 * translation unit can be linked into the same test binary as the
 * lab implementation for a differential comparison.
 *
 * Provenance: https://github.com/aappleby/smhasher
 * (src/MurmurHash3.cpp), the reference implementation published by
 * the algorithm's author.  The constant values were additionally
 * cross-checked against three independent published ports
 * (Apache commons-codec MurmurHash3.java, ns-3 hash-murmur3,
 * ashn-dot-dev/mellifera referencing the smhasher source line).
 */
#include <stdint.h>

uint64_t fmix64_reference(uint64_t k)
{
    k ^= k >> 33;
    k *= UINT64_C(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= UINT64_C(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;

    return k;
}
