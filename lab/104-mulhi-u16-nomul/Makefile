CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_mulhi_u16_o0 test_mulhi_u16_o2 test_mulhi_u16_asan disasm_mulhi_u16.txt

test_mulhi_u16_o0: test_mulhi_u16.c mulhi_u16.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_mulhi_u16.c

test_mulhi_u16_o2: test_mulhi_u16.c mulhi_u16.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_mulhi_u16.c

# AddressSanitizer + UBSan: the implementation is pure unsigned
# arithmetic (tests of low bits, 32-bit adds, 32-bit left shifts that
# never overflow the 32-bit width, and right shifts). No signed
# overflow and no out-of-range shift is possible by construction, so
# the sanitizers are expected to stay silent on the full case set.
test_mulhi_u16_asan: test_mulhi_u16.c mulhi_u16.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_mulhi_u16.c

# Disassembly of the noinline wrapper, compiled from the header alone
# (the test's oracle multiply cannot leak in). The grep is the
# requirement: no mul/imul instruction may appear; the target fails
# loudly (exit 1) if one does.
disasm_mulhi_u16.txt: disasm_mulhi_u16.c mulhi_u16.h
	$(CC) $(CFLAGS) -O2 -c -o disasm_mulhi_u16.o disasm_mulhi_u16.c
	objdump -d -M intel disasm_mulhi_u16.o > $@
	! grep -Ei '\b(mul|imul)\b' $@
	rm -f disasm_mulhi_u16.o

run: all
	./test_mulhi_u16_o0
	./test_mulhi_u16_o2
	./test_mulhi_u16_asan

clean:
	rm -f test_mulhi_u16_o0 test_mulhi_u16_o2 test_mulhi_u16_asan disasm_mulhi_u16.txt

.PHONY: all run clean
