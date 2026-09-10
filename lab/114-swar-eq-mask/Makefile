CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_eqmask

test_eqmask: test_eqmask.c eqmask.c eqmask.h
	$(CC) $(CFLAGS) -o $@ test_eqmask.c eqmask.c

test_eqmask_O0: test_eqmask.c eqmask.c eqmask.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_eqmask.c eqmask.c

test_eqmask_san: test_eqmask.c eqmask.c eqmask.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_eqmask.c eqmask.c

bench: test_eqmask.c eqmask.c eqmask.h
	$(CC) $(CFLAGS) -DBENCH -o test_eqmask_bench test_eqmask.c eqmask.c

run: test_eqmask test_eqmask_O0 test_eqmask_san
	./test_eqmask
	./test_eqmask_O0
	./test_eqmask_san

clean:
	rm -f test_eqmask test_eqmask_O0 test_eqmask_san test_eqmask_bench
