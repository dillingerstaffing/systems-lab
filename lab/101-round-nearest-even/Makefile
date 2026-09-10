CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench

all: test_round_rne

round_rne_O2.o: round_rne.c round_rne.h
	$(CC) $(CFLAGS) -c -o $@ round_rne.c

round_rne_O0.o: round_rne.c round_rne.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -c -o $@ round_rne.c

round_rne_san.o: round_rne.c round_rne.h
	$(CC) $(CFLAGS) $(SAN) -c -o $@ round_rne.c

test_round_rne: test_round_rne.c round_rne.h round_rne_O2.o
	$(CC) $(CFLAGS) -o $@ test_round_rne.c round_rne_O2.o

test_round_rne_O0: test_round_rne.c round_rne.h round_rne_O0.o
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_round_rne.c round_rne_O0.o

test_round_rne_san: test_round_rne.c round_rne.h round_rne_san.o
	$(CC) $(CFLAGS) $(SAN) -o $@ test_round_rne.c round_rne_san.o

test_round_rne_bench: test_round_rne.c round_rne.h round_rne_O2.o
	$(CC) $(CFLAGS) -DBENCH -o $@ test_round_rne.c round_rne_O2.o

run: test_round_rne test_round_rne_O0 test_round_rne_san
	./test_round_rne
	./test_round_rne_O0
	./test_round_rne_san

bench: test_round_rne_bench
	./test_round_rne_bench

clean:
	rm -f test_round_rne test_round_rne_O0 test_round_rne_san test_round_rne_bench round_rne_O2.o round_rne_O0.o round_rne_san.o
