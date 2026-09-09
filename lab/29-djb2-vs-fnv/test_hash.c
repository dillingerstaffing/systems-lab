#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "hash.h"

/* splitmix64, fixed seed: one reproducible 64-bit stream. */
static uint64_t rng_state;

static void rng_seed(uint64_t seed)
{
    rng_state = seed;
}

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15u);
    z = (z ^ (z >> 30u)) * 0xBF58476D1CE4E5B9u;
    z = (z ^ (z >> 27u)) * 0x94D049BB133111EBu;
    return z ^ (z >> 31u);
}

/* Independent spec of djb2: same recurrence, different construction.
 * 33 == 2^5 + 1, so h * 33 == (h << 5) + h; pointer loop instead of
 * index loop. */
static uint64_t spec_djb2_64(const unsigned char *data, size_t len)
{
    uint64_t h = 5381u;
    const unsigned char *p = data;
    const unsigned char *end = data + len;
    while (p != end) {
        h = ((h << 5) + h) + *p;
        p++;
    }
    return h;
}

/* Independent spec of FNV-1a: same recurrence, the prime multiply
 * spelled out as shifts and adds of the xor result.
 * 1099511628211 = 0x100000001B3 = 2^40 + 2^8 + 2^7 + 2^5 + 2^4
 * + 2^1 + 2^0. */
static uint64_t spec_fnv1a_64(const unsigned char *data, size_t len)
{
    uint64_t h = 14695981039346656037u;
    for (size_t i = 0; i < len; i++) {
        uint64_t t = h ^ data[i];
        h = t + (t << 1) + (t << 4) + (t << 5) + (t << 7) + (t << 8)
            + (t << 40);
    }
    return h;
}

/* Pinned vectors produced by the spec implementations above; the
 * shipped functions must reproduce them exactly. */
struct vector {
    const char *label;
    const unsigned char *bytes;
    size_t len;
    uint64_t djb2_expect;
    uint64_t fnv1a_expect;
};

static uint64_t ns_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

int main(void)
{
    uint64_t mismatches = 0;
    uint64_t checked = 0;
    uint64_t checksum = 14695981039346656037u;
    uint64_t djb2_bins[256] = { 0 };
    uint64_t fnv1a_bins[256] = { 0 };
    static const char fox[] = "The quick brown fox jumps over the lazy dog";
    unsigned char allbytes[256];
    for (int i = 0; i < 256; i++)
        allbytes[i] = (unsigned char)i;

    /* Known-answer vectors. */
    {
        static const struct vector vecs[] = {
            { "empty", (const unsigned char *)"", 0,
              0x0000000000001505u, 0xcbf29ce484222325u },
            { "\"a\"", (const unsigned char *)"a", 1,
              0x000000000002b606u, 0xaf63dc4c8601ec8cu },
            { "\"hello\"", (const unsigned char *)"hello", 5,
              0x000000310f923099u, 0xa430d84680aabd0bu },
            { "fox", (const unsigned char *)fox, sizeof fox - 1,
              0x36d23eef34cc38deu, 0xf3f9b7f5e7e47110u },
        };
        for (size_t i = 0; i < sizeof vecs / sizeof vecs[0]; i++) {
            uint64_t d_impl = djb2_64(vecs[i].bytes, vecs[i].len);
            uint64_t d_spec = spec_djb2_64(vecs[i].bytes, vecs[i].len);
            uint64_t f_impl = fnv1a_64(vecs[i].bytes, vecs[i].len);
            uint64_t f_spec = spec_fnv1a_64(vecs[i].bytes, vecs[i].len);
            if (d_impl != d_spec || d_impl != vecs[i].djb2_expect) {
                printf("VECTOR-MISMATCH djb2 %s impl=%" PRIu64
                       " spec=%" PRIu64 " expect=%" PRIu64 "\n",
                       vecs[i].label, d_impl, d_spec,
                       vecs[i].djb2_expect);
                mismatches++;
            }
            if (f_impl != f_spec || f_impl != vecs[i].fnv1a_expect) {
                printf("VECTOR-MISMATCH fnv1a %s impl=%" PRIu64
                       " spec=%" PRIu64 " expect=%" PRIu64 "\n",
                       vecs[i].label, f_impl, f_spec,
                       vecs[i].fnv1a_expect);
                mismatches++;
            }
            checked += 2;
        }
        /* 256-byte all-bytes buffer: generated at runtime, so the
         * reference digest was computed by the spec implementation. */
        {
            uint64_t d_impl = djb2_64(allbytes, 256);
            uint64_t d_spec = spec_djb2_64(allbytes, 256);
            uint64_t f_impl = fnv1a_64(allbytes, 256);
            uint64_t f_spec = spec_fnv1a_64(allbytes, 256);
            if (d_impl != d_spec || d_impl != 0x21ca86fe9a5b9485u) {
                printf("VECTOR-MISMATCH djb2 all-bytes\n");
                mismatches++;
            }
            if (f_impl != f_spec || f_impl != 0x4242dc5249c33625u) {
                printf("VECTOR-MISMATCH fnv1a all-bytes\n");
                mismatches++;
            }
            checked += 2;
        }
        printf("known-answer vectors: checked=%" PRIu64 " mismatches=%"
               PRIu64 "\n", checked, mismatches);
    }

    /* 1M fixed-seed random strings, lengths 1..64: differential impl
     * vs spec for both hashes, plus chi-square bins of byte 0 (the
     * low byte) of each digest. */
    {
        static unsigned char buf[64];
        const uint64_t N = 1000000u;
        uint64_t diff_checked = 0;
        rng_seed(0xDEADBEEF12345678u);
        for (uint64_t i = 0; i < N; i++) {
            size_t len = 1 + (size_t)(splitmix64() % 64u);
            for (size_t j = 0; j < len; j++)
                buf[j] = (unsigned char)(splitmix64() & 0xFFu);
            uint64_t d_impl = djb2_64(buf, len);
            uint64_t d_spec = spec_djb2_64(buf, len);
            uint64_t f_impl = fnv1a_64(buf, len);
            uint64_t f_spec = spec_fnv1a_64(buf, len);
            if (d_impl != d_spec) {
                if (mismatches < 8)
                    printf("MISMATCH djb2 string %" PRIu64 " len %zu\n",
                           i, len);
                mismatches++;
            }
            if (f_impl != f_spec) {
                if (mismatches < 8)
                    printf("MISMATCH fnv1a string %" PRIu64 " len %zu\n",
                           i, len);
                mismatches++;
            }
            diff_checked += 2;
            djb2_bins[d_impl & 0xFFu]++;
            fnv1a_bins[f_impl & 0xFFu]++;
            /* Fold both digests into the cross-build checksum. */
            for (int b = 0; b < 8; b++) {
                checksum ^= (unsigned char)(d_impl >> (8 * b));
                checksum *= 1099511628211u;
                checksum ^= (unsigned char)(f_impl >> (8 * b));
                checksum *= 1099511628211u;
            }
        }
        printf("differential: checked=%" PRIu64 " mismatches=%" PRIu64 "\n",
               diff_checked, mismatches);
        printf("checksum=%" PRIu64 "\n", checksum);
    }

    /* Chi-square over the 256 bins of digest byte 0 from the 1M
     * random strings above. Expected 1000000/256 = 3906.25 per bin;
     * chi-square = sum((obs - exp)^2 / exp), 255 degrees of freedom,
     * so a uniform spread lands near 255. */
    {
        const double exp = 1000000.0 / 256.0;
        double chi_djb2 = 0.0, chi_fnv1a = 0.0;
        for (int i = 0; i < 256; i++) {
            double dd = (double)djb2_bins[i] - exp;
            double df = (double)fnv1a_bins[i] - exp;
            chi_djb2 += dd * dd / exp;
            chi_fnv1a += df * df / exp;
        }
        printf("chi-square (digest byte 0, 1M strings): djb2=%.2f "
               "fnv1a=%.2f\n", chi_djb2, chi_fnv1a);
    }

    /* Throughput: a fixed 1 MiB buffer, seeded once, hashed 2000
     * times; the digest is XORed into a sink so the loop stays. */
    {
        static unsigned char big[1024 * 1024];
        rng_seed(0x9E3779B97F4A7C15u);
        for (size_t i = 0; i < sizeof big; i++)
            big[i] = (unsigned char)(splitmix64() & 0xFFu);

        const uint64_t ROUNDS = 2000u;
        uint64_t sink = 0;
        uint64_t t0 = ns_now();
        for (uint64_t r = 0; r < ROUNDS; r++)
            sink ^= djb2_64(big, sizeof big) ^ r;
        uint64_t t1 = ns_now();
        double bytes = (double)ROUNDS * (double)sizeof big;
        printf("throughput: djb2 %.1f MB/s (%.3f s, %" PRIu64
               " rounds) sink=%" PRIu64 "\n",
               bytes / (double)(t1 - t0) * 1000.0, (double)(t1 - t0) / 1e9,
               ROUNDS, sink);

        sink = 0;
        t0 = ns_now();
        for (uint64_t r = 0; r < ROUNDS; r++)
            sink ^= fnv1a_64(big, sizeof big) ^ r;
        t1 = ns_now();
        printf("throughput: fnv1a %.1f MB/s (%.3f s, %" PRIu64
               " rounds) sink=%" PRIu64 "\n",
               bytes / (double)(t1 - t0) * 1000.0, (double)(t1 - t0) / 1e9,
               ROUNDS, sink);
    }

    if (mismatches == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("FAIL\n");
    return 1;
}
