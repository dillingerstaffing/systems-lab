CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_hsum

test_hsum: test_hsum.c hsum.c hsum.h
	$(CC) $(CFLAGS) -o $@ test_hsum.c hsum.c

test_hsum_O0: test_hsum.c hsum.c hsum.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_hsum.c hsum.c

test_hsum_san: test_hsum.c hsum.c hsum.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_hsum.c hsum.c

bench: test_hsum.c hsum.c hsum.h
	$(CC) $(CFLAGS) -DBENCH -o test_hsum_bench test_hsum.c hsum.c

run: test_hsum test_hsum_O0 test_hsum_san
	./test_hsum
	./test_hsum_O0
	./test_hsum_san

clean:
	rm -f test_hsum test_hsum_O0 test_hsum_san test_hsum_bench
