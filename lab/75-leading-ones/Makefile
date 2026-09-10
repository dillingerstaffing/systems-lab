CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_leading_ones_o0 test_leading_ones_o2 test_leading_ones_asan

test_leading_ones_o0: test_leading_ones.c leading_ones.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_leading_ones.c

test_leading_ones_o2: test_leading_ones.c leading_ones.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_leading_ones.c

# AddressSanitizer + UBSan: clo64 uses only unsigned 64-bit shifts,
# ORs, adds, subtracts, multiplies, and a NOT, so no undefined
# behavior is possible; the sanitizers confirm it empirically on the
# full case set.  (The naive reference shifts only by 0..63, also
# fully defined.)
test_leading_ones_asan: test_leading_ones.c leading_ones.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_leading_ones.c

run: all
	./test_leading_ones_o0
	./test_leading_ones_o2
	./test_leading_ones_asan

clean:
	rm -f test_leading_ones_o0 test_leading_ones_o2 test_leading_ones_asan

.PHONY: all run clean
