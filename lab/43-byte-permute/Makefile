CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_permute

test_permute: test_permute.c permute.h
	$(CC) $(CFLAGS) -o $@ test_permute.c

run: test_permute
	./test_permute

sanitize: test_permute.c permute.h
	$(CC) $(SANFLAGS) -o test_permute_asan test_permute.c
	./test_permute_asan

opt0: test_permute.c permute.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_permute_o0 test_permute.c
	./test_permute_o0

clean:
	rm -f test_permute test_permute_asan test_permute_o0

.PHONY: all run sanitize opt0 clean
