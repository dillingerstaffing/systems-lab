CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_select

test_select: test_select.c select.c select.h
	$(CC) $(CFLAGS) -o $@ test_select.c select.c

test_select_O0: test_select.c select.c select.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_select.c select.c

test_select_san: test_select.c select.c select.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_select.c select.c

bench: test_select.c select.c select.h
	$(CC) $(CFLAGS) -DBENCH -o test_select_bench test_select.c select.c

run: test_select test_select_O0 test_select_san
	./test_select
	./test_select_O0
	./test_select_san

clean:
	rm -f test_select test_select_O0 test_select_san test_select_bench
