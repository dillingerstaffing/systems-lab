#ifndef MEMMOVE30_H
#define MEMMOVE30_H

#include <stddef.h>

/* Byte-level memmove: copy direction chosen by the overlap geometry. */
void *my_memmove(void *dest, const void *src, size_t n);

#endif
