CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror

all: test_crc32c

test_crc32c: test_crc32c.c crc32c.c crc32c.h
	$(CC) $(CFLAGS) -o $@ test_crc32c.c crc32c.c

run: test_crc32c
	./test_crc32c

sanitize:
	$(CC) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
		-o test_crc32c_asan test_crc32c.c crc32c.c
	./test_crc32c_asan

opt0:
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_crc32c_o0 test_crc32c.c crc32c.c
	./test_crc32c_o0

clean:
	rm -f test_crc32c test_crc32c_asan test_crc32c_o0

.PHONY: all run sanitize opt0 clean
