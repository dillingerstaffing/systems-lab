CC      = gcc
STD     = -std=c11
WARN    = -Wall -Wextra -Werror
BIN     = test_O0 test_O2 test_asan bench_O2

all: $(BIN) disasm

test_O0: test_div7.c div7.h
	$(CC) $(STD) $(WARN) -O0 -o $@ test_div7.c

test_O2: test_div7.c div7.h
	$(CC) $(STD) $(WARN) -O2 -o $@ test_div7.c

test_asan: test_div7.c div7.h
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
	    -fno-sanitize-recover=all -o $@ test_div7.c

bench_O2: bench_div7.c div7.h
	$(CC) $(STD) $(WARN) -O2 -o $@ bench_div7.c

impl_check.o: impl_check.c div7.h
	$(CC) $(STD) $(WARN) -O2 -c -o $@ impl_check.c

# Fail the build if the -O2 codegen of the implementation uses div/idiv.
disasm: impl_check.o
	objdump -d impl_check.o > impl_check.dis
	@echo "--- div/idiv mnemonics in implementation object ---"
	@if grep -wE 'i?div[ql]?' impl_check.dis; then \
	    echo "FAIL: division instruction present"; exit 1; \
	else \
	    echo "OK: no div/idiv in udiv7_shiftadd codegen"; \
	fi

check: test_O0 test_O2 test_asan
	./test_O0
	./test_O2
	./test_asan

clean:
	rm -f $(BIN) impl_check.o impl_check.dis
