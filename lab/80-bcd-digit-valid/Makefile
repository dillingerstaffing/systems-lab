CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_bcd

test_bcd: test_bcd.c bcd.c bcd.h
	$(CC) $(CFLAGS) -o $@ test_bcd.c bcd.c

test_bcd_O0: test_bcd.c bcd.c bcd.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_bcd.c bcd.c

test_bcd_san: test_bcd.c bcd.c bcd.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_bcd.c bcd.c

bench: test_bcd.c bcd.c bcd.h
	$(CC) $(CFLAGS) -DBENCH -o test_bcd_bench test_bcd.c bcd.c

run: test_bcd test_bcd_O0 test_bcd_san
	./test_bcd
	./test_bcd_O0
	./test_bcd_san

clean:
	rm -f test_bcd test_bcd_O0 test_bcd_san test_bcd_bench
