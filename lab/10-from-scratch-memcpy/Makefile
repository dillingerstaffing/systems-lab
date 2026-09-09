CC = cc
CFLAGS = -Wall -Wextra -Werror -O2 -std=c11

all: test_memcpy test_memcpy_asan

test_memcpy: memcpy10.c test_memcpy.c memcpy10.h
	$(CC) $(CFLAGS) -o $@ memcpy10.c test_memcpy.c

test_memcpy_asan: memcpy10.c test_memcpy.c memcpy10.h
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -o $@ memcpy10.c test_memcpy.c

check: test_memcpy
	./test_memcpy

check-asan: test_memcpy_asan
	./test_memcpy_asan

clean:
	rm -f test_memcpy test_memcpy_asan
