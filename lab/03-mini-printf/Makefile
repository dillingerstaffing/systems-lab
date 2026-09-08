CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror

all: test_printf

test_printf: test_printf.c mini_printf.c mini_printf.h
	$(CC) $(CFLAGS) -o $@ test_printf.c mini_printf.c

run: test_printf
	./test_printf

clean:
	rm -f test_printf mini_printf.o

.PHONY: all run clean
