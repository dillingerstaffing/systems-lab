CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_inc128

test_inc128: test_inc128.c inc128.c inc128.h
	$(CC) $(CFLAGS) -o $@ test_inc128.c inc128.c

test_inc128_O0: test_inc128.c inc128.c inc128.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_inc128.c inc128.c

test_inc128_san: test_inc128.c inc128.c inc128.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_inc128.c inc128.c

bench: test_inc128.c inc128.c inc128.h
	$(CC) $(CFLAGS) -DBENCH -o test_inc128_bench test_inc128.c inc128.c

run: test_inc128 test_inc128_O0 test_inc128_san
	./test_inc128
	./test_inc128_O0
	./test_inc128_san

clean:
	rm -f test_inc128 test_inc128_O0 test_inc128_san test_inc128_bench
