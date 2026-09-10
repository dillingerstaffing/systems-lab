CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_fmix64_o0 test_fmix64_o2 test_fmix64_asan

test_fmix64_o0: test_fmix64.c fmix64.h fmix64_ref.c
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_fmix64.c fmix64_ref.c

test_fmix64_o2: test_fmix64.c fmix64.h fmix64_ref.c
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_fmix64.c fmix64_ref.c

# AddressSanitizer + UBSan: fmix64/fmix64_inv use only unsigned 64-bit
# shifts, xors, and multiplies, so no undefined behavior is possible;
# the sanitizers confirm it empirically on the full case set.
test_fmix64_asan: test_fmix64.c fmix64.h fmix64_ref.c
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_fmix64.c fmix64_ref.c

run: all
	./test_fmix64_o0
	./test_fmix64_o2
	./test_fmix64_asan

clean:
	rm -f test_fmix64_o0 test_fmix64_o2 test_fmix64_asan

.PHONY: all run clean
