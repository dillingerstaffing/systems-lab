/*
 * test_printf.c -- differential test for mini_printf.
 *
 * Every case formats the same arguments with the host libc snprintf and
 * with mini_printf (via a putc capture sink), then compares the byte
 * output and the return value exactly. Any divergence is a failure.
 */
#include "mini_printf.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int total = 0;
static int failed = 0;

typedef struct {
    char buf[8192];
    size_t n;
} capture_t;

static void cap_putc(char c, void *ctx)
{
    capture_t *cap = (capture_t *)ctx;

    if (cap->n < sizeof(cap->buf) - 1)
        cap->buf[cap->n++] = c;
}

static int capture_vprintf(capture_t *cap, const char *fmt, ...)
{
    va_list ap;
    int r;

    cap->n = 0;
    va_start(ap, fmt);
    r = mini_vprintf(cap_putc, cap, fmt, ap);
    va_end(ap);
    return r;
}

/* Differential check: mini_printf must match snprintf byte-for-byte,
 * including the return value (works even with embedded NULs). */
#define CHECK(fmt, ...) do { \
    char expect[4096]; \
    int eref = snprintf(expect, sizeof(expect), fmt, ##__VA_ARGS__); \
    capture_t cap; \
    int got = capture_vprintf(&cap, fmt, ##__VA_ARGS__); \
    total++; \
    if (eref < 0 || got != eref || (int)cap.n != eref || \
        memcmp(expect, cap.buf, (size_t)eref) != 0) { \
        failed++; \
        printf("FAIL fmt=[%s] ref_ret=%d got_ret=%d got_n=%zu\n", \
               fmt, eref, got, cap.n); \
    } \
} while (0)

/* Reference snprintf via a noinline wrapper: deliberate truncation in the
 * CHECKN cases would otherwise trip -Wformat-truncation at the call site. */
__attribute__((noinline))
static int ref_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap, fmt);
    r = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return r;
}

/* mini_snprintf truncation check against snprintf. */
#define CHECKN(size, fmt, ...) do { \
    char e1[128], e2[128]; \
    int r1 = ref_snprintf(e1, size, fmt, ##__VA_ARGS__); \
    int r2 = mini_snprintf(e2, size, fmt, ##__VA_ARGS__); \
    size_t n = (size_t)r1 < (size) ? (size_t)r1 + 1 : (size); \
    total++; \
    if (r1 != r2 || ((size) > 0 && memcmp(e1, e2, n) != 0)) { \
        failed++; \
        printf("FAIL snprintf size=%d fmt=[%s] r1=%d r2=%d e1=[%s] e2=[%s]\n", \
               (int)(size), fmt, r1, r2, e1, e2); \
    } \
} while (0)

static const char *nullstr; /* known-NULL at runtime, opaque to -Wformat */

/* Manual check for formats that -Wformat rejects in the reference call
 * (empty format, trailing '%', unknown conversions). */
static void check_manual(const char *label, const char *expected, int exp_len,
                         const char *fmt, ...)
{
    capture_t cap;
    va_list ap;
    int r;

    total++;
    cap.n = 0;
    va_start(ap, fmt);
    r = mini_vprintf(cap_putc, &cap, fmt, ap);
    va_end(ap);
    if (r != exp_len || (int)cap.n != exp_len ||
        memcmp(expected, cap.buf, (size_t)exp_len) != 0) {
        failed++;
        printf("FAIL manual [%s]: ret=%d n=%zu expected_ret=%d\n",
               label, r, cap.n, exp_len);
    }
}

int main(void)
{
    /* signed decimal */
    static const long long svals[] = {
        0, 1, -1, 2, -2, 9, -9, 42, -42, 127, -128,
        12345, -12345, 1000000, -999999999LL,
        2147483647LL, -2147483648LL,
        9223372036854775807LL, -9223372036854775807LL - 1
    };
    static const char *sfmts[] = {
        "%d", "%i", "%5d", "%-5d", "%05d", "%+d", "% d", "%+05d",
        "%.0d", "%.1d", "%.5d", "%8.3d", "%-8.3d", "%3d", "%*d"
    };
    for (size_t i = 0; i < sizeof(svals) / sizeof(svals[0]); i++)
        for (size_t j = 0; j < sizeof(sfmts) / sizeof(sfmts[0]); j++) {
            if (sfmts[j][1] == '*')
                CHECK(sfmts[j], 8, (int)svals[i]);
            else
                CHECK(sfmts[j], (int)svals[i]);
        }

    /* long / long long */
    CHECK("%ld", LONG_MIN);
    CHECK("%ld", LONG_MAX);
    CHECK("%ld", -1L);
    CHECK("%lld", -9223372036854775807LL - 1);
    CHECK("%lld", 9223372036854775807LL);
    CHECK("%+020lld", -9223372036854775807LL - 1);
    CHECK("%llu", 18446744073709551615ULL);
    CHECK("%llu", 0ULL);
    CHECK("%020llu", 18446744073709551615ULL);
    CHECK("%zd", (long)(size_t)123456789012345ULL); /* z reads 64-bit */
    CHECK("%zd", (long)-42);
    CHECK("%zu", (size_t)123456789012345ULL);
    CHECK("%lld", svals[17]);

    /* unsigned */
    static const unsigned uvals[] = {
        0u, 1u, 2u, 42u, 1234567890u, 4294967295u, 4294967294u
    };
    static const char *ufmts[] = {
        "%u", "%5u", "%-5u", "%05u", "%.0u", "%.8u", "%10.5u", "%-10.3u"
    };
    for (size_t i = 0; i < sizeof(uvals) / sizeof(uvals[0]); i++)
        for (size_t j = 0; j < sizeof(ufmts) / sizeof(ufmts[0]); j++)
            CHECK(ufmts[j], uvals[i]);

    /* hex / octal */
    static const unsigned xvals[] = {
        0u, 1u, 15u, 16u, 255u, 256u, 0xdeadbeefu, 0xffffffffu
    };
    static const char *xfmts[] = {
        "%x", "%X", "%#x", "%#X", "%08x", "%-8x", "%.0x",
        "%#08x", "%5.3x", "%o", "%#o", "%08o", "%.0o", "%#.0o",
        "%#08X", "%llx", "%#llx"
    };
    for (size_t i = 0; i < sizeof(xvals) / sizeof(xvals[0]); i++)
        for (size_t j = 0; j < sizeof(xfmts) / sizeof(xfmts[0]); j++) {
            if (xfmts[j][1] == '#' && xfmts[j][2] == 'l')
                CHECK(xfmts[j], (unsigned long long)xvals[i]);
            else if (xfmts[j][1] == 'l')
                CHECK(xfmts[j], (unsigned long long)xvals[i]);
            else
                CHECK(xfmts[j], xvals[i]);
        }
    CHECK("%llx", 18446744073709551615ULL);
    CHECK("%#llx", 18446744073709551615ULL);

    /* strings (local array: nullstr is not a constant initializer) */
    const char *strs[] = {
        "hello", "", "x", "a longer string, with punctuation! 0123", nullstr
    };
    static const char *strfmts[] = {
        "%s", "%10s", "%-10s", "%.0s", "%.1s", "%.3s",
        "%8.3s", "%-8.3s", "%2s", "%*s", "%.*s"
    };
    for (size_t i = 0; i < sizeof(strs) / sizeof(strs[0]); i++)
        for (size_t j = 0; j < sizeof(strfmts) / sizeof(strfmts[0]); j++) {
            if (strfmts[j][1] == '.' && strfmts[j][2] == '*')
                CHECK(strfmts[j], 3, strs[i]); /* "%.*s" only */
            else if (strfmts[j][1] == '*')
                CHECK(strfmts[j], 8, strs[i]); /* "%*s" only */
            else
                CHECK(strfmts[j], strs[i]);
        }

    /* characters (including NUL, compared length-aware) */
    static const char cvals[] = { 'A', 'z', '0', ' ', '~', '\0' };
    static const char *cfmts[] = { "%c", "%3c", "%-3c" };
    for (size_t i = 0; i < sizeof(cvals); i++)
        for (size_t j = 0; j < sizeof(cfmts) / sizeof(cfmts[0]); j++)
            CHECK(cfmts[j], cvals[i]);

    /* pointers */
    int local = 42;
    static void *pvals[6];

    pvals[0] = NULL;
    pvals[1] = (void *)0x1;
    pvals[2] = (void *)0x1234;
    pvals[3] = (void *)&local;
    pvals[4] = (void *)(uintptr_t)0xdeadbeef;
    pvals[5] = (void *)(uintptr_t)0x1000;
    static const char *pfmts[] = { "%p", "%20p", "%-20p" };
    for (size_t i = 0; i < 6; i++)
        for (size_t j = 0; j < sizeof(pfmts) / sizeof(pfmts[0]); j++)
            CHECK(pfmts[j], pvals[i]);

    /* literals, escapes, mixed conversions */
    CHECK("hello world");
    CHECK("100%%");
    CHECK("%%");
    CHECK("%%%%");
    check_manual("empty", "", 0, "");
    check_manual("trailing-%", "abc%", 4, "abc%"); /* prints literally */
    check_manual("unknown-%q", "%q", 2, "%q");     /* prints literally */
    CHECK("%d", 5);
    CHECK("val=%d hex=%x str=%s!", -42, 0xdeadbeefu, "hi");
    CHECK("%p %d %s %c %x", pvals[3], -7, "mix", 'Q', 255u);
    CHECK("%d%d%d", 1, 22, 333);
    CHECK("%s=%d", "answer", 42);
    CHECK("%*.*d", 10, 4, 123);
    CHECK("%*.*s", 10, 3, "abcdef");
    CHECK("%.*d", -3, 42);  /* negative precision via * = absent */
    CHECK("%05d|%-5d|%+d|% d", 7, 7, 7, 7);
    CHECK("%#x %#o", 0u, 0u);
    CHECK("%u %x %o", 0u, 0u, 0u);

    /* mini_snprintf truncation behavior */
    CHECKN(0, "%d-%s-%x", 12345, "hello", 0xabcdu);
    CHECKN(1, "%d-%s-%x", 12345, "hello", 0xabcdu);
    CHECKN(2, "%d-%s-%x", 12345, "hello", 0xabcdu);
    CHECKN(5, "%d-%s-%x", 12345, "hello", 0xabcdu);
    CHECKN(10, "%d-%s-%x", 12345, "hello", 0xabcdu);
    CHECKN(64, "%d-%s-%x", 12345, "hello", 0xabcdu);
    CHECKN(8, "%05d", -42);
    CHECKN(4, "%s", "toolong");

    printf("mini_printf differential test: %d cases, %d failed\n",
           total, failed);
    if (failed == 0)
        printf("ALL TESTS PASSED\n");
    return failed != 0;
}
