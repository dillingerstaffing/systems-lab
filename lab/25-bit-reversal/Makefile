CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_bit_reverse_o0 test_bit_reverse_o2 test_bit_reverse_asan

test_bit_reverse_o0: test_bit_reverse.c bit_reverse.c bit_reverse.h
	$(CC) $(CFLAGS) -O0 -o $@ test_bit_reverse.c bit_reverse.c

test_bit_reverse_o2: test_bit_reverse.c bit_reverse.c bit_reverse.h
	$(CC) $(CFLAGS) -O2 -o $@ test_bit_reverse.c bit_reverse.c

# AddressSanitizer + UBSan: proves the shift/mask group swaps contain
# no undefined behavior (no bad shifts, no signed overflow; all
# arithmetic is unsigned).
test_bit_reverse_asan: test_bit_reverse.c bit_reverse.c bit_reverse.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_bit_reverse.c bit_reverse.c

run: all
	./test_bit_reverse_o0
	./test_bit_reverse_o2
	./test_bit_reverse_asan

clean:
	rm -f test_bit_reverse_o0 test_bit_reverse_o2 test_bit_reverse_asan

.PHONY: all run clean
