CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_popcount

test_popcount: test_popcount.c popcount64.h
	$(CC) $(CFLAGS) -o $@ test_popcount.c

run: test_popcount
	./test_popcount

sanitize: test_popcount.c popcount64.h
	$(CC) $(SANFLAGS) -o test_popcount_asan test_popcount.c
	./test_popcount_asan

opt0: test_popcount.c popcount64.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_popcount_o0 test_popcount.c
	./test_popcount_o0

clean:
	rm -f test_popcount test_popcount_asan test_popcount_o0

.PHONY: all run sanitize opt0 clean
