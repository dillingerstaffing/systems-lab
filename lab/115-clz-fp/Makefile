CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench

all: test_clz_fp

test_clz_fp: test_clz_fp.c clz_fp.c clz_fp.h
	$(CC) $(CFLAGS) -o $@ test_clz_fp.c clz_fp.c

test_clz_fp_O0: test_clz_fp.c clz_fp.c clz_fp.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_clz_fp.c clz_fp.c

test_clz_fp_san: test_clz_fp.c clz_fp.c clz_fp.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_clz_fp.c clz_fp.c

test_clz_fp_bench: test_clz_fp.c clz_fp.c clz_fp.h
	$(CC) $(CFLAGS) -DBENCH -o $@ test_clz_fp.c clz_fp.c

run: test_clz_fp test_clz_fp_O0 test_clz_fp_san
	./test_clz_fp
	./test_clz_fp_O0
	./test_clz_fp_san

bench: test_clz_fp_bench
	./test_clz_fp_bench

clean:
	rm -f test_clz_fp test_clz_fp_O0 test_clz_fp_san test_clz_fp_bench
