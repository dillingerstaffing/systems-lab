CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_swar_max_o0 test_swar_max_o2 test_swar_max_asan

test_swar_max_o0: test_swar_max.c swar_max.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_swar_max.c

test_swar_max_o2: test_swar_max.c swar_max.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_swar_max.c

# AddressSanitizer + UBSan: the identities use only unsigned 64-bit
# shifts, ORs, ANDs, XORs, NOTs and one wrapping subtraction, plus
# 64-bit multiplies by 0x0001000100010001 on values below 2^16
# (rep16) and the FNV-1a multiply; no undefined behavior is
# possible and the sanitizers confirm it empirically on the full
# case set.  (The naive references shift only by 0..48 and mask
# to 16 bits, also fully defined.  splitmix64 is the standard
# integer-mixing sequence on unsigned 64-bit values.)
test_swar_max_asan: test_swar_max.c swar_max.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_swar_max.c

run: all
	./test_swar_max_o0
	./test_swar_max_o2
	./test_swar_max_asan

clean:
	rm -f test_swar_max_o0 test_swar_max_o2 test_swar_max_asan

.PHONY: all run clean
