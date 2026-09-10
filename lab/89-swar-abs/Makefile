CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_swar_abs_o0 test_swar_abs_o2 test_swar_abs_asan

test_swar_abs_o0: test_swar_abs.c swar_abs.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_swar_abs.c

test_swar_abs_o2: test_swar_abs.c swar_abs.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_swar_abs.c

# AddressSanitizer + UBSan: the implementation uses only unsigned
# 64-bit shifts by compile-time constants below 64, ORs, ANDs,
# XORs, NOTs and one wrapping subtraction, plus the FNV-1a
# multiply; no undefined behavior is possible and the sanitizers
# confirm it empirically on the full case set.  (The naive
# reference shifts only by 0..48 and masks to 16 bits, also fully
# defined.  splitmix64 is the standard integer-mixing sequence on
# unsigned 64-bit values.)
test_swar_abs_asan: test_swar_abs.c swar_abs.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_swar_abs.c

run: all
	./test_swar_abs_o0
	./test_swar_abs_o2
	./test_swar_abs_asan

clean:
	rm -f test_swar_abs_o0 test_swar_abs_o2 test_swar_abs_asan

.PHONY: all run clean
