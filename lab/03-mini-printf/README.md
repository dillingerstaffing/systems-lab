# lab/03-mini-printf: no-libc printf in C

A tiny, freestanding `printf` implementation: no `malloc`, no stdio, no
`string.h`. All output goes through a caller-supplied `putc` callback
plus an opaque context pointer, so it drops straight onto a UART,
a framebuffer, or a fixed buffer on bare metal.

## API (`mini_printf.h`)

```c
typedef void (*mini_putc_fn)(char c, void *ctx);

int mini_printf(mini_putc_fn putc, void *ctx, const char *fmt, ...);
int mini_vprintf(mini_putc_fn putc, void *ctx, const char *fmt, va_list ap);
int mini_snprintf(char *buf, size_t size, const char *fmt, ...);
int mini_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
```

Returns the character count, like the real thing. `mini_snprintf`
never writes more than `size` bytes and always NUL-terminates when
`size > 0`.

## Supported format spec

- Conversions: `%d %i %u %o %x %X %c %s %p %%`
- Flags: `-` `+` (space) `#` `0`
- Width, including `*` (negative width via `*` means `-`); precision,
  including `.*`, truncating `%s`
- Length modifiers: `l` `ll` `z` (`h` `hh` `t` `j` accepted; arguments
  are promoted anyway). `z` reads a 64-bit value.
- `NULL` `%s` prints `(null)`, or empty when a precision below 6 is
  given; `NULL` `%p` prints `(nil)`: both match the host C library.
- Unknown conversions (e.g. `%q`) and a trailing `%` print literally.

Only freestanding headers are used (`<stdarg.h>`, `<stddef.h>`,
`<stdint.h>`, `<limits.h>`).

## Verification

`test_printf.c` is a differential test: every case formats the same
arguments with the host libc `snprintf` and with `mini_printf` (via a
capture sink), then compares the bytes and the return value exactly.
609 cases, covering `%d/%i/%u/%o/%x/%X/%c/%s/%p`, flags, widths,
precisions, `*`/`.*`, `l`/`ll`/`z` modifiers, `INT_MIN`, `LLONG_MIN`,
`UINT_MAX`, `ULLONG_MAX`, empty strings, `NUL` characters, `NULL`
strings and pointers, plus `mini_snprintf` truncation checks
(sizes 0, 1, 2, 5, 10, 64). See `PROOF.md` for the genuine build log
and run output.

```sh
cd lab/03-mini-printf
make run
```
