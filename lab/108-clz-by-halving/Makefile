CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench

all: test_clz_half

clz_half_O2.o: clz_half.c clz_half.h
	$(CC) $(CFLAGS) -c -o $@ clz_half.c

clz_half_O0.o: clz_half.c clz_half.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -c -o $@ clz_half.c

clz_half_san.o: clz_half.c clz_half.h
	$(CC) $(CFLAGS) $(SAN) -c -o $@ clz_half.c

test_clz_half: test_clz_half.c clz_half.h clz_half_O2.o
	$(CC) $(CFLAGS) -o $@ test_clz_half.c clz_half_O2.o

test_clz_half_O0: test_clz_half.c clz_half.h clz_half_O0.o
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_clz_half.c clz_half_O0.o

test_clz_half_san: test_clz_half.c clz_half.h clz_half_san.o
	$(CC) $(CFLAGS) $(SAN) -o $@ test_clz_half.c clz_half_san.o

test_clz_half_bench: test_clz_half.c clz_half.h clz_half_O2.o
	$(CC) $(CFLAGS) -DBENCH -o $@ test_clz_half.c clz_half_O2.o

run: test_clz_half test_clz_half_O0 test_clz_half_san
	./test_clz_half
	./test_clz_half_O0
	./test_clz_half_san

bench: test_clz_half_bench
	./test_clz_half_bench

clean:
	rm -f test_clz_half test_clz_half_O0 test_clz_half_san test_clz_half_bench clz_half_O2.o clz_half_O0.o clz_half_san.o
