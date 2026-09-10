CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench

all: test_pdep

test_pdep: test_pdep.c pdep.c pdep.h
	$(CC) $(CFLAGS) -o $@ test_pdep.c pdep.c

test_pdep_O0: test_pdep.c pdep.c pdep.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_pdep.c pdep.c

test_pdep_san: test_pdep.c pdep.c pdep.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_pdep.c pdep.c

test_pdep_bench: test_pdep.c pdep.c pdep.h
	$(CC) $(CFLAGS) -DBENCH -o $@ test_pdep.c

run: test_pdep test_pdep_O0 test_pdep_san
	./test_pdep
	./test_pdep_O0
	./test_pdep_san

bench: test_pdep_bench
	./test_pdep_bench

clean:
	rm -f test_pdep test_pdep_O0 test_pdep_san test_pdep_bench
