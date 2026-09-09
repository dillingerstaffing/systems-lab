CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_bsearch

test_bsearch: test_bsearch.c bsearch_branchless.h
	$(CC) $(CFLAGS) -o $@ test_bsearch.c

run: test_bsearch
	./test_bsearch

sanitize: test_bsearch.c bsearch_branchless.h
	$(CC) $(SANFLAGS) -o test_bsearch_asan test_bsearch.c
	./test_bsearch_asan

opt0: test_bsearch.c bsearch_branchless.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_bsearch_o0 test_bsearch.c
	./test_bsearch_o0

clean:
	rm -f test_bsearch test_bsearch_asan test_bsearch_o0

.PHONY: all run sanitize opt0 clean
