CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_isqrt

test_isqrt: test_isqrt.c isqrt.h
	$(CC) $(CFLAGS) -o $@ test_isqrt.c

run: test_isqrt
	./test_isqrt full

sanitize: test_isqrt.c isqrt.h
	$(CC) $(SANFLAGS) -o test_isqrt_asan test_isqrt.c
	./test_isqrt_asan full

opt0: test_isqrt.c isqrt.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_isqrt_o0 test_isqrt.c
	./test_isqrt_o0 full

clean:
	rm -f test_isqrt test_isqrt_asan test_isqrt_o0

.PHONY: all run sanitize opt0 clean
