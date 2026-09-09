CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_bit_deposit_o0 test_bit_deposit_o2 test_bit_deposit_asan

test_bit_deposit_o0: test_bit_deposit.c bit_deposit.c bit_deposit.h
	$(CC) $(CFLAGS) -O0 -o $@ test_bit_deposit.c bit_deposit.c

test_bit_deposit_o2: test_bit_deposit.c bit_deposit.c bit_deposit.h
	$(CC) $(CFLAGS) -O2 -o $@ test_bit_deposit.c bit_deposit.c

# AddressSanitizer + UBSan: proves the shift/mask construction contains
# no undefined behavior (the n = 64 mask case never executes
# 1ULL << 64; every other shift stays below 64 bits).
test_bit_deposit_asan: test_bit_deposit.c bit_deposit.c bit_deposit.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_bit_deposit.c bit_deposit.c

run: all
	./test_bit_deposit_o0
	./test_bit_deposit_o2
	./test_bit_deposit_asan

clean:
	rm -f test_bit_deposit_o0 test_bit_deposit_o2 test_bit_deposit_asan

.PHONY: all run clean
