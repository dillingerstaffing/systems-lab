CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_clz_byte

test_clz_byte: test_clz_byte.c clz_byte.c clz_byte.h
	$(CC) $(CFLAGS) -o $@ test_clz_byte.c clz_byte.c

run: test_clz_byte
	./test_clz_byte

sanitize: test_clz_byte.c clz_byte.c clz_byte.h
	$(CC) $(SANFLAGS) -o test_clz_byte_asan test_clz_byte.c clz_byte.c
	./test_clz_byte_asan

opt0: test_clz_byte.c clz_byte.c clz_byte.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_clz_byte_o0 test_clz_byte.c clz_byte.c
	./test_clz_byte_o0

clean:
	rm -f test_clz_byte test_clz_byte_asan test_clz_byte_o0

.PHONY: all run sanitize opt0 clean
