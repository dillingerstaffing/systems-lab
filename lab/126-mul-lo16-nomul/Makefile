CC      = gcc
STD     = -std=c11
WARN    = -Wall -Wextra -Werror
BIN     = test_O0 test_O2 test_asan bench_O2

all: $(BIN) disasm

test_O0: test_mullo16.c mul16.c mul16.h
	$(CC) $(STD) $(WARN) -O0 -o $@ test_mullo16.c mul16.c

test_O2: test_mullo16.c mul16.c mul16.h
	$(CC) $(STD) $(WARN) -O2 -o $@ test_mullo16.c mul16.c

test_asan: test_mullo16.c mul16.c mul16.h
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
	    -fno-sanitize-recover=all -o $@ test_mullo16.c mul16.c

bench_O2: bench_mullo16.c mul16.c mul16.h
	$(CC) $(STD) $(WARN) -O2 -o $@ bench_mullo16.c mul16.c

mul16_impl.o: mul16.c mul16.h
	$(CC) $(STD) $(WARN) -O2 -c -o $@ mul16.c

# Programmatic disassembly check: fail the build if the -O2 codegen of
# mullo16 contains any multiply-class instruction (mul, imul, mulw, madd,
# msub, smull, umull, fmul, ...). The implementation is shifts and adds
# only, so a multiply-class mnemonic in the object is a hard failure.
# This check is programmatic, never eyeballed.
disasm: mul16_impl.o
	objdump -d mul16_impl.o > mul16_impl.dis
	@echo "--- multiply-class instructions in mullo16 codegen (-O2) ---"
	@if grep -E -i '\b(imul|mul|mulw|muld|mull|mulh|mulhu|madd|msub|smull|umull|fmul)\b' mul16_impl.dis; then \
	    echo "FAIL: multiply-class instruction present in mullo16"; exit 1; \
	else \
	    echo "OK: no multiply-class instruction in mullo16 codegen"; \
	fi

check: test_O0 test_O2 test_asan
	./test_O0
	./test_O2
	./test_asan

clean:
	rm -f $(BIN) mul16_impl.o mul16_impl.dis
