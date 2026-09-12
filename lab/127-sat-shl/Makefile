CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_satshl_o0 test_satshl_o2 test_satshl_asan

test_satshl_o0: test_satshl.c satshl.c satshl.h
	$(CC) $(CFLAGS) -O0 -o $@ test_satshl.c satshl.c

test_satshl_o2: test_satshl.c satshl.c satshl.h
	$(CC) $(CFLAGS) -O2 -o $@ test_satshl.c satshl.c

# AddressSanitizer + UBSan: proves the construction contains no
# undefined behavior (no shift by >= 64 anywhere, including the
# (64 - kk) & 63 fold when kk == 0).
test_satshl_asan: test_satshl.c satshl.c satshl.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_satshl.c satshl.c

run: all
	./test_satshl_o0
	./test_satshl_o2
	./test_satshl_asan

# Programmatic branchlessness check: compile sat_shl64 standalone at
# -O2, disassemble it, and fail the build if the function contains any
# conditional jump. Unconditional `jmp` is allowed and excluded; every
# other j<cc> mnemonic is a conditional jump.
disasm: satshl.c satshl.h
	$(CC) $(CFLAGS) -O2 -c -o satshl.o satshl.c
	@echo "--- sat_shl64 disassembly (gcc -O2, x86_64) ---"
	objdump -d satshl.o --disassemble=sat_shl64
	@echo "--- conditional jump count in sat_shl64 ---"
	@test 0 -eq `objdump -d satshl.o --disassemble=sat_shl64 | \
		grep -oE '\bj[a-z]+\b' | grep -vc '^jmp$$'` && \
		echo "conditional_jumps=0 PASS" || \
		(echo "conditional_jumps!=0 FAIL"; exit 1)

clean:
	rm -f test_satshl_o0 test_satshl_o2 test_satshl_asan satshl.o

.PHONY: all run disasm clean
