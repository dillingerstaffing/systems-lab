CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_saturate_u16_o0 test_saturate_u16_o2 test_saturate_u16_asan

test_saturate_u16_o0: test_saturate_u16.c saturate_u16.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_saturate_u16.c

test_saturate_u16_o2: test_saturate_u16.c saturate_u16.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_saturate_u16.c

# AddressSanitizer + UBSan: the implementation uses only unsigned
# 32-bit arithmetic, shifts by compile-time constants below 32,
# and bitwise ops; no undefined behavior is possible and the
# sanitizers confirm it empirically on the full case set.  (The
# if/else reference operates on in-range int32 values only, also
# fully defined.  splitmix64 is the standard integer-mixing
# sequence on unsigned 64-bit values.)
test_saturate_u16_asan: test_saturate_u16.c saturate_u16.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_saturate_u16.c

# Disassembly target: compile the non-inline wrapper at -O2 and
# show its codegen.  Not part of the default build.
disasm: disasm_saturate.c saturate_u16.h
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -c disasm_saturate.c -o disasm_saturate.o
	objdump -d disasm_saturate.o

run: all
	./test_saturate_u16_o0
	./test_saturate_u16_o2
	./test_saturate_u16_asan

clean:
	rm -f test_saturate_u16_o0 test_saturate_u16_o2 test_saturate_u16_asan disasm_saturate.o

.PHONY: all run clean disasm
