CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_div3

test_div3: test_div3.c div3.c div3.h
	$(CC) $(CFLAGS) -o $@ test_div3.c div3.c

test_div3_O0: test_div3.c div3.c div3.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_div3.c div3.c

test_div3_san: test_div3.c div3.c div3.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_div3.c div3.c

bench: test_div3.c div3.c div3.h
	$(CC) $(CFLAGS) -DBENCH -o test_div3_bench test_div3.c div3.c

run: test_div3 test_div3_O0 test_div3_san
	./test_div3
	./test_div3_O0
	./test_div3_san

clean:
	rm -f test_div3 test_div3_O0 test_div3_san test_div3_bench div3_o2.o
