CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_mulhi_signed_o0 test_mulhi_signed_o2 test_mulhi_signed_asan

test_mulhi_signed_o0: test_mulhi_signed.c mulhi_signed.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_mulhi_signed.c

test_mulhi_signed_o2: test_mulhi_signed.c mulhi_signed.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_mulhi_signed.c

# AddressSanitizer + UBSan: the implementation is pure 64-bit arithmetic
# (unsigned shifts, adds, multiplies, comparisons, and two conditional
# subtracts; the int64_t -> uint64_t conversions are exact mod 2^64).
# The test's signed __int128 oracle uses implementation-defined
# arithmetic right shift on negative values, which UBSan does not
# flag (it is not UB, only implementation-defined); the sanitizers
# confirm no actual undefined behavior on the full case set.
test_mulhi_signed_asan: test_mulhi_signed.c mulhi_signed.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_mulhi_signed.c

run: all
	./test_mulhi_signed_o0
	./test_mulhi_signed_o2
	./test_mulhi_signed_asan

clean:
	rm -f test_mulhi_signed_o0 test_mulhi_signed_o2 test_mulhi_signed_asan

.PHONY: all run clean
