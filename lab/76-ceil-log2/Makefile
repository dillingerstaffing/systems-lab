CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_ceil_log2_o0 test_ceil_log2_o2 test_ceil_log2_asan

test_ceil_log2_o0: test_ceil_log2.c ceil_log2.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_ceil_log2.c

test_ceil_log2_o2: test_ceil_log2.c ceil_log2.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_ceil_log2.c

# AddressSanitizer + UBSan: ceil_log2_64 uses only unsigned 64-bit
# shifts, ORs, adds, subtracts, and multiplies, so no undefined
# behavior is possible; the sanitizers confirm it empirically on
# the full case set.  (The loop reference shifts nothing and its
# doubling step is widened before it can overflow, also fully
# defined.)
test_ceil_log2_asan: test_ceil_log2.c ceil_log2.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_ceil_log2.c

run: all
	./test_ceil_log2_o0
	./test_ceil_log2_o2
	./test_ceil_log2_asan

clean:
	rm -f test_ceil_log2_o0 test_ceil_log2_o2 test_ceil_log2_asan

.PHONY: all run clean
