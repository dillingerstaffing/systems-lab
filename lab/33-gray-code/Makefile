CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_gray_o0 test_gray_o2 test_gray_asan

test_gray_o0: test_gray.c gray.c gray.h
	$(CC) $(CFLAGS) -O0 -o $@ test_gray.c gray.c

# -DMEASURE enables the throughput measurement in the test binary.
test_gray_o2: test_gray.c gray.c gray.h
	$(CC) $(CFLAGS) -DMEASURE -O2 -o $@ test_gray.c gray.c

# AddressSanitizer + UBSan: the exhaustive loop over all 16-bit inputs
# also exercises the sanitizers on every possible input.
test_gray_asan: test_gray.c gray.c gray.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_gray.c gray.c

run: all
	./test_gray_o0
	./test_gray_o2
	./test_gray_asan

clean:
	rm -f test_gray_o0 test_gray_o2 test_gray_asan

.PHONY: all run clean
