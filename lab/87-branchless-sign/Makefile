CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean

all: test_sign

test_sign: test_sign.c sign.c sign.h
	$(CC) $(CFLAGS) -o $@ test_sign.c sign.c

test_sign_O0: test_sign.c sign.c sign.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_sign.c sign.c

test_sign_san: test_sign.c sign.c sign.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_sign.c sign.c

bench: test_sign.c sign.c sign.h
	$(CC) $(CFLAGS) -DBENCH -o test_sign_bench test_sign.c sign.c

run: test_sign test_sign_O0 test_sign_san
	./test_sign
	./test_sign_O0
	./test_sign_san

clean:
	rm -f test_sign test_sign_O0 test_sign_san test_sign_bench
