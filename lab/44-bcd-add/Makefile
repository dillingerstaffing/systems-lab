CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_bcd

test_bcd: test_bcd.c bcd.h
	$(CC) $(CFLAGS) -o $@ test_bcd.c

run: test_bcd
	./test_bcd

sanitize: test_bcd.c bcd.h
	$(CC) $(SANFLAGS) -o test_bcd_asan test_bcd.c
	./test_bcd_asan

opt0: test_bcd.c bcd.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_bcd_o0 test_bcd.c
	./test_bcd_o0

clean:
	rm -f test_bcd test_bcd_asan test_bcd_o0

.PHONY: all run sanitize opt0 clean
