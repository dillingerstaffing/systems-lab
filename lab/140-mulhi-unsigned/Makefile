CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_mulhi_unsigned_o0 test_mulhi_unsigned_o2 test_mulhi_unsigned_asan

test_mulhi_unsigned_o0: test_mulhi_unsigned.c mulhi_unsigned.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_mulhi_unsigned.c

test_mulhi_unsigned_o2: test_mulhi_unsigned.c mulhi_unsigned.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_mulhi_unsigned.c

# AddressSanitizer + UBSan: the implementation is pure 64-bit arithmetic
# (unsigned shifts, adds, multiplies, comparisons; unsigned wrap is
# well-defined), so the sanitizers confirm no actual undefined behavior
# on the full case set.
test_mulhi_unsigned_asan: test_mulhi_unsigned.c mulhi_unsigned.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_mulhi_unsigned.c

run: all
	./test_mulhi_unsigned_o0
	./test_mulhi_unsigned_o2
	./test_mulhi_unsigned_asan

clean:
	rm -f test_mulhi_unsigned_o0 test_mulhi_unsigned_o2 test_mulhi_unsigned_asan

.PHONY: all run clean
