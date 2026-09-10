CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_xor_swap_o0 test_xor_swap_o2 test_xor_swap_asan

test_xor_swap_o0: test_xor_swap.c xor_swap.c xor_swap.h
	$(CC) $(CFLAGS) -O0 -o $@ test_xor_swap.c xor_swap.c

test_xor_swap_o2: test_xor_swap.c xor_swap.c xor_swap.h
	$(CC) $(CFLAGS) -O2 -o $@ test_xor_swap.c xor_swap.c

# AddressSanitizer + UBSan: proves the xor sequence contains no undefined
# behavior (no unaligned or out-of-bounds access, no bad arithmetic).
test_xor_swap_asan: test_xor_swap.c xor_swap.c xor_swap.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_xor_swap.c xor_swap.c

run: all
	./test_xor_swap_o0
	./test_xor_swap_o2
	./test_xor_swap_asan

clean:
	rm -f test_xor_swap_o0 test_xor_swap_o2 test_xor_swap_asan

.PHONY: all run clean
