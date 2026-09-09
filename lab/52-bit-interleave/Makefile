CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_morton

test_morton: test_morton.c morton.h
	$(CC) $(CFLAGS) -o $@ test_morton.c

run: test_morton
	./test_morton

o0: test_morton.c morton.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_morton_o0 test_morton.c
	./test_morton_o0

sanitize: test_morton.c morton.h
	$(CC) $(SANFLAGS) -o test_morton_asan test_morton.c
	./test_morton_asan

clean:
	rm -f test_morton test_morton_o0 test_morton_asan

.PHONY: all run o0 sanitize clean
