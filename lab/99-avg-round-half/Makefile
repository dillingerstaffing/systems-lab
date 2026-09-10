CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench

all: test_avg_round_half

test_avg_round_half: test_avg_round_half.c avg_round_half.h
	$(CC) $(CFLAGS) -o $@ test_avg_round_half.c

test_avg_round_half_O0: test_avg_round_half.c avg_round_half.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_avg_round_half.c

test_avg_round_half_san: test_avg_round_half.c avg_round_half.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_avg_round_half.c

test_avg_round_half_bench: test_avg_round_half.c avg_round_half.h
	$(CC) $(CFLAGS) -DBENCH -o $@ test_avg_round_half.c

run: test_avg_round_half test_avg_round_half_O0 test_avg_round_half_san
	./test_avg_round_half
	./test_avg_round_half_O0
	./test_avg_round_half_san

bench: test_avg_round_half_bench
	./test_avg_round_half_bench

clean:
	rm -f test_avg_round_half test_avg_round_half_O0 test_avg_round_half_san test_avg_round_half_bench
