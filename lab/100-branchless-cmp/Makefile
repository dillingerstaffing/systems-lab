CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_cmp

test_cmp: test_cmp.c cmp.c cmp.h
	$(CC) $(CFLAGS) -o $@ test_cmp.c cmp.c

test_cmp_O0: test_cmp.c cmp.c cmp.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_cmp.c cmp.c

test_cmp_san: test_cmp.c cmp.c cmp.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_cmp.c cmp.c

bench: test_cmp.c cmp.c cmp.h
	$(CC) $(CFLAGS) -DBENCH -o test_cmp_bench test_cmp.c cmp.c

run: test_cmp test_cmp_O0 test_cmp_san
	./test_cmp
	./test_cmp_O0
	./test_cmp_san

clean:
	rm -f test_cmp test_cmp_O0 test_cmp_san test_cmp_bench
