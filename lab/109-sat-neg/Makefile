CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench

all: test_sat_neg

sat_neg_O2.o: sat_neg.c sat_neg.h
	$(CC) $(CFLAGS) -c -o $@ sat_neg.c

sat_neg_O0.o: sat_neg.c sat_neg.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -c -o $@ sat_neg.c

sat_neg_san.o: sat_neg.c sat_neg.h
	$(CC) $(CFLAGS) $(SAN) -c -o $@ sat_neg.c

test_sat_neg: test_sat_neg.c sat_neg.h sat_neg_O2.o
	$(CC) $(CFLAGS) -o $@ test_sat_neg.c sat_neg_O2.o

test_sat_neg_O0: test_sat_neg.c sat_neg.h sat_neg_O0.o
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_sat_neg.c sat_neg_O0.o

test_sat_neg_san: test_sat_neg.c sat_neg.h sat_neg_san.o
	$(CC) $(CFLAGS) $(SAN) -o $@ test_sat_neg.c sat_neg_san.o

test_sat_neg_bench: test_sat_neg.c sat_neg.h sat_neg_O2.o
	$(CC) $(CFLAGS) -DBENCH -o $@ test_sat_neg.c sat_neg_O2.o

run: test_sat_neg test_sat_neg_O0 test_sat_neg_san
	./test_sat_neg
	./test_sat_neg_O0
	./test_sat_neg_san

bench: test_sat_neg_bench
	./test_sat_neg_bench

clean:
	rm -f test_sat_neg test_sat_neg_O0 test_sat_neg_san test_sat_neg_bench sat_neg_O2.o sat_neg_O0.o sat_neg_san.o
