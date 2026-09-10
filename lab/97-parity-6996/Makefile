CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_parity

test_parity: test_parity.c parity.c parity.h
	$(CC) $(CFLAGS) -o $@ test_parity.c parity.c

test_parity_O0: test_parity.c parity.c parity.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_parity.c parity.c

test_parity_san: test_parity.c parity.c parity.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_parity.c parity.c

bench: test_parity.c parity.c parity.h
	$(CC) $(CFLAGS) -DBENCH -o test_parity_bench test_parity.c parity.c

run: test_parity test_parity_O0 test_parity_san
	./test_parity
	./test_parity_O0
	./test_parity_san

clean:
	rm -f test_parity test_parity_O0 test_parity_san test_parity_bench
