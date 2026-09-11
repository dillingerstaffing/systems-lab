CC      = gcc
STD     = -std=c11
WARN    = -Wall -Wextra -Werror
BIN     = test_O0 test_O2 test_asan bench_O2

all: $(BIN) disasm

test_O0: test_bitop.c bitop.h
	$(CC) $(STD) $(WARN) -O0 -o $@ test_bitop.c

test_O2: test_bitop.c bitop.h
	$(CC) $(STD) $(WARN) -O2 -o $@ test_bitop.c

test_asan: test_bitop.c bitop.h
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
	    -fno-sanitize-recover=all -o $@ test_bitop.c

bench_O2: bench_bitop.c bitop.h
	$(CC) $(STD) $(WARN) -O2 -o $@ bench_bitop.c

impl_check.o: impl_check.c bitop.h
	$(CC) $(STD) $(WARN) -O2 -c -o $@ impl_check.c

# Fail the build if the -O2 codegen of the implementation contains any shift
# by a constant count of 64 or more. A shift by 64 on a 64-bit word is
# undefined behavior, so it must never be able to execute.
disasm: impl_check.o
	objdump -d impl_check.o > impl_check.dis
	@echo "--- shift-by-64-or-more immediates in implementation object ---"
	@if grep -E '(shl|shr|sar)[^,]*\$$0x4[0-9a-f]|\$0*(6[4-9]|[7-9][0-9]|[1-9][0-9][0-9])' impl_check.dis; then \
	    echo "FAIL: shift by >= 64 present"; exit 1; \
	else \
	    echo "OK: no shift by 64 or more in bset/bclr/btg/btst codegen"; \
	fi

check: test_O0 test_O2 test_asan
	./test_O0
	./test_O2
	./test_asan

clean:
	rm -f $(BIN) impl_check.o impl_check.dis
