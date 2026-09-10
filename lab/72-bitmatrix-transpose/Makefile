CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_bitmatrix_transpose_o0 test_bitmatrix_transpose_o2 test_bitmatrix_transpose_asan

test_bitmatrix_transpose_o0: test_bitmatrix_transpose.c bitmatrix_transpose.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_bitmatrix_transpose.c

test_bitmatrix_transpose_o2: test_bitmatrix_transpose.c bitmatrix_transpose.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_bitmatrix_transpose.c

# AddressSanitizer + UBSan: every operation in bitmatrix_transpose64 is an
# unsigned 64-bit shift, xor, or and with all shift counts below 64, so no
# undefined behavior is possible; the sanitizers confirm it empirically on
# the full case set.
test_bitmatrix_transpose_asan: test_bitmatrix_transpose.c bitmatrix_transpose.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_bitmatrix_transpose.c

run: all
	./test_bitmatrix_transpose_o0
	./test_bitmatrix_transpose_o2
	./test_bitmatrix_transpose_asan

clean:
	rm -f test_bitmatrix_transpose_o0 test_bitmatrix_transpose_o2 test_bitmatrix_transpose_asan

.PHONY: all run clean
