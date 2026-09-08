/*
 * mini_printf.c -- freestanding printf implementation.
 *
 * Only freestanding headers are used (<stdarg.h>, <stddef.h>,
 * <stdint.h>, <limits.h>): no malloc, no stdio, no string.h.
 * All output goes through the caller's putc callback.
 */
#include "mini_printf.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    mini_putc_fn putc;
    void *ctx;
    int count;
} sink_t;

static void emit(sink_t *s, char c)
{
    s->putc(c, s->ctx);
    s->count++;
}

static void emit_n(sink_t *s, char c, int n)
{
    while (n-- > 0)
        emit(s, c);
}

/* Convert v to reversed digits in out. Returns the digit count. */
static int to_rev(unsigned long long v, unsigned base, char *out, int upper)
{
    static const char lo[] = "0123456789abcdef";
    static const char hi[] = "0123456789ABCDEF";
    const char *d = upper ? hi : lo;
    int n = 0;

    do {
        out[n++] = d[v % base];
        v /= base;
    } while (v != 0);
    return n;
}

/* String conversion with optional precision truncation and padding. */
static void fmt_str(sink_t *s, const char *str, int len,
                    int prec, int width, int left)
{
    int n = len;

    if (prec >= 0 && n > prec)
        n = prec;
    int pad = width - n;
    if (pad < 0)
        pad = 0;
    if (!left)
        emit_n(s, ' ', pad);
    for (int i = 0; i < n; i++)
        emit(s, str[i]);
    if (left)
        emit_n(s, ' ', pad);
}

static int c_strlen(const char *s)
{
    int n = 0;

    while (s[n] != '\0')
        n++;
    return n;
}

/* Integer conversion. mag is the absolute value, neg the sign. */
static void fmt_int(sink_t *s, unsigned long long mag, int neg, unsigned base,
                    int upper, int alt, int plus, int space,
                    int width, int prec, int left, int zero)
{
    char buf[24]; /* worst case: 22 octal digits for 64 bits */
    int ndig = to_rev(mag, base, buf, upper);

    if (prec == 0 && mag == 0)
        ndig = 0; /* "%.0d" of 0 prints nothing */
    if (base == 8 && alt && ndig == 0) {
        /* "%#.0o" of 0 still prints a single 0, like libc */
        buf[0] = '0';
        ndig = 1;
    }

    const char *prefix = "";
    int prelen = 0;

    if (alt && mag != 0) {
        if (base == 16) {
            prefix = upper ? "0X" : "0x";
            prelen = 2;
        } else if (base == 8) {
            prefix = "0";
            prelen = 1;
        }
    }

    char sign = 0;

    if (neg)
        sign = '-';
    else if (plus)
        sign = '+';
    else if (space)
        sign = ' ';

    if (left || prec >= 0)
        zero = 0; /* '0' ignored with '-' or precision */

    int zdigits = ndig > prec ? ndig : prec; /* precision zero-padding */
    int total = (sign ? 1 : 0) + prelen + zdigits;
    int pad = width - total;

    if (pad < 0)
        pad = 0;
    if (!left && !zero)
        emit_n(s, ' ', pad);
    if (sign)
        emit(s, sign);
    for (int i = 0; i < prelen; i++)
        emit(s, prefix[i]);
    if (zero)
        emit_n(s, '0', pad);
    emit_n(s, '0', zdigits - ndig);
    for (int i = ndig - 1; i >= 0; i--)
        emit(s, buf[i]);
    if (left)
        emit_n(s, ' ', pad);
}

static void fmt_char(sink_t *s, char c, int width, int left)
{
    int pad = width - 1;

    if (pad < 0)
        pad = 0;
    if (!left)
        emit_n(s, ' ', pad);
    emit(s, c);
    if (left)
        emit_n(s, ' ', pad);
}

static void fmt_ptr(sink_t *s, void *p, int width, int left)
{
    if (p == NULL) {
        fmt_str(s, "(nil)", 5, -1, width, left);
        return;
    }
    char rev[16];
    char buf[2 + 16];
    int n = to_rev((uintptr_t)p, 16, rev, 0);

    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < n; i++)
        buf[2 + i] = rev[n - 1 - i];
    fmt_str(s, buf, 2 + n, -1, width, left);
}

int mini_vprintf(mini_putc_fn putc, void *ctx, const char *fmt, va_list ap)
{
    sink_t s = { putc, ctx, 0 };

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            emit(&s, *p);
            continue;
        }
        p++;

        /* flags */
        int left = 0, plus = 0, space = 0, alt = 0, zero = 0;
        int scanning = 1;

        while (scanning) {
            switch (*p) {
            case '-': left = 1; p++; break;
            case '+': plus = 1; p++; break;
            case ' ': space = 1; p++; break;
            case '#': alt = 1; p++; break;
            case '0': zero = 1; p++; break;
            default: scanning = 0; break;
            }
        }

        /* width */
        int width = 0;

        if (*p == '*') {
            int w = va_arg(ap, int);

            p++;
            if (w < 0) {
                left = 1;
                w = -w;
            }
            width = w;
        } else {
            while (*p >= '0' && *p <= '9') {
                width = width * 10 + (*p - '0');
                p++;
            }
        }

        /* precision */
        int prec = -1;

        if (*p == '.') {
            p++;
            if (*p == '*') {
                int q = va_arg(ap, int);

                p++;
                prec = q < 0 ? -1 : q;
            } else {
                prec = 0;
                while (*p >= '0' && *p <= '9') {
                    prec = prec * 10 + (*p - '0');
                    p++;
                }
            }
        }

        /* length: 0 = default, 1 = l, 2 = ll, 3 = z.
         * h/hh/t/j are accepted (arguments are promoted anyway). */
        int len = 0;

        if (*p == 'l') {
            p++;
            if (*p == 'l') {
                p++;
                len = 2;
            } else {
                len = 1;
            }
        } else if (*p == 'z') {
            p++;
            len = 3;
        } else if (*p == 'h') {
            p++;
            if (*p == 'h')
                p++;
        } else if (*p == 't' || *p == 'j') {
            p++;
        }

        switch (*p) {
        case 'd':
        case 'i': {
            long long v;

            if (len == 2)
                v = va_arg(ap, long long);
            else if (len == 1)
                v = va_arg(ap, long);
            else if (len == 3)
                v = va_arg(ap, long long); /* z: 64-bit ssize_t */
            else
                v = va_arg(ap, int);
            /* -(v+1)+1 avoids overflow on LLONG_MIN */
            int neg = v < 0;
            unsigned long long mag =
                neg ? (unsigned long long)(-(v + 1)) + 1ULL
                    : (unsigned long long)v;

            fmt_int(&s, mag, neg, 10, 0, 0, plus, space,
                    width, prec, left, zero);
            break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            unsigned long long v;

            if (len == 2)
                v = va_arg(ap, unsigned long long);
            else if (len == 1)
                v = va_arg(ap, unsigned long);
            else if (len == 3)
                v = va_arg(ap, size_t);
            else
                v = va_arg(ap, unsigned int);
            unsigned base = *p == 'o' ? 8 : (*p == 'u' ? 10 : 16);
            int upper = *p == 'X';

            fmt_int(&s, v, 0, base, upper, alt, 0, 0,
                    width, prec, left, zero);
            break;
        }
        case 'c':
            fmt_char(&s, (char)va_arg(ap, int), width, left);
            break;
        case 's': {
            const char *str = va_arg(ap, const char *);

            if (str == NULL) {
                /* match libc: a precision shorter than "(null)"
                 * yields empty output */
                str = (prec >= 0 && prec < 6) ? "" : "(null)";
            }
            fmt_str(&s, str, c_strlen(str), prec, width, left);
            break;
        }
        case 'p':
            fmt_ptr(&s, va_arg(ap, void *), width, left);
            break;
        case '%':
            emit(&s, '%');
            break;
        case '\0':
            /* trailing '%': print it literally, then the loop ends */
            emit(&s, '%');
            p--;
            break;
        default:
            /* unknown conversion: print it literally, like "%q" -> "%q" */
            emit(&s, '%');
            emit(&s, *p);
            break;
        }
    }
    return s.count;
}

int mini_printf(mini_putc_fn putc, void *ctx, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = mini_vprintf(putc, ctx, fmt, ap);
    va_end(ap);
    return n;
}

typedef struct {
    char *buf;
    size_t size;
    size_t pos; /* characters emitted so far */
} bufctx_t;

static void buf_putc(char c, void *ctx)
{
    bufctx_t *b = (bufctx_t *)ctx;

    if (b->size > 0 && b->pos < b->size - 1)
        b->buf[b->pos] = c;
    b->pos++;
}

int mini_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    bufctx_t b = { buf, size, 0 };
    int n = mini_vprintf(buf_putc, &b, fmt, ap);

    if (size > 0)
        buf[b.pos < size ? b.pos : size - 1] = '\0';
    return n;
}

int mini_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = mini_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}
