CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror

all: test_bitcount_o0 test_bitcount_o2 test_bitcount_asan

test_bitcount_o0: test_bitcount.c bitcount.c bitcount.h
	$(CC) $(CFLAGS) -O0 -o $@ test_bitcount.c bitcount.c

test_bitcount_o2: test_bitcount.c bitcount.c bitcount.h
	$(CC) $(CFLAGS) -O2 -o $@ test_bitcount.c bitcount.c

# AddressSanitizer + UBSan: confirms the mask arithmetic and the
# multiply-shift fold contain no undefined behavior (all unsigned,
# but the checkers prove it).
test_bitcount_asan: test_bitcount.c bitcount.c bitcount.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_bitcount.c bitcount.c

run: all
	./test_bitcount_o0
	./test_bitcount_o2
	./test_bitcount_asan

clean:
	rm -f test_bitcount_o0 test_bitcount_o2 test_bitcount_asan

.PHONY: all run clean
