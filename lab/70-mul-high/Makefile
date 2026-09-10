CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_mul_high_o0 test_mul_high_o2 test_mul_high_asan

test_mul_high_o0: test_mul_high.c mul_high.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_mul_high.c

test_mul_high_o2: test_mul_high.c mul_high.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_mul_high.c

# AddressSanitizer + UBSan: every operation in mul_high64 is unsigned
# (shifts, adds, multiplies, comparisons), so no undefined behavior is
# possible; the sanitizers confirm it empirically on the full case set.
test_mul_high_asan: test_mul_high.c mul_high.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_mul_high.c

run: all
	./test_mul_high_o0
	./test_mul_high_o2
	./test_mul_high_asan

clean:
	rm -f test_mul_high_o0 test_mul_high_o2 test_mul_high_asan

.PHONY: all run clean
