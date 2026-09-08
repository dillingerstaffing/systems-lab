/*
 * mini_printf.h -- a tiny no-libc printf for bare-metal and hosted use.
 *
 * The caller supplies a putc-like output sink plus an opaque context
 * pointer, so this works on a UART, a framebuffer, or a plain buffer
 * with zero dynamic allocation and zero libc I/O dependency.
 *
 * Supported conversions: %d %i %u %o %x %X %c %s %p %%.
 * Flags: - + (space) # 0. Width (including *), precision (including .*).
 * Length modifiers: l ll z (h hh t j accepted and ignored, arguments are
 * promoted anyway). NULL %s prints "(null)", or "" when a precision below
 * 6 is given; NULL %p prints "(nil)": both match the host C library.
 *
 * Returns the number of characters emitted, like printf.
 */
#ifndef MINI_PRINTF_H
#define MINI_PRINTF_H

#include <stdarg.h>
#include <stddef.h>

/* Output sink: any function that consumes one character. */
typedef void (*mini_putc_fn)(char c, void *ctx);

int mini_vprintf(mini_putc_fn putc, void *ctx, const char *fmt, va_list ap);
int mini_printf(mini_putc_fn putc, void *ctx, const char *fmt, ...);

/*
 * Convenience: format into a fixed-size buffer. Returns the number of
 * characters that would have been written (like snprintf); never writes
 * more than size bytes and always NUL-terminates when size > 0.
 */
int mini_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int mini_snprintf(char *buf, size_t size, const char *fmt, ...);

#endif /* MINI_PRINTF_H */
