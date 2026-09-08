CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror

all: test_crc32

test_crc32: test_crc32.c crc32.c crc32.h
	$(CC) $(CFLAGS) -o $@ test_crc32.c crc32.c

run: test_crc32
	./test_crc32

sanitize:
	$(CC) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
		-o test_crc32_asan test_crc32.c crc32.c
	./test_crc32_asan

opt0:
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_crc32_o0 test_crc32.c crc32.c
	./test_crc32_o0

clean:
	rm -f test_crc32 test_crc32_asan test_crc32_o0

.PHONY: all run sanitize opt0 clean
