CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_bitrev

test_bitrev: test_bitrev.c bitrev.h bitrev.c
	$(CC) $(CFLAGS) -o $@ test_bitrev.c bitrev.c

run: test_bitrev
	./test_bitrev

sanitize: test_bitrev.c bitrev.h bitrev.c
	$(CC) $(SANFLAGS) -o test_bitrev_asan test_bitrev.c bitrev.c
	./test_bitrev_asan

opt0: test_bitrev.c bitrev.h bitrev.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_bitrev_o0 test_bitrev.c bitrev.c
	./test_bitrev_o0

disasm: test_bitrev.c bitrev.h bitrev.c
	$(CC) $(CFLAGS) -c -o bitrev.o bitrev.c
	@echo "--- bitrev8_perm disassembly ---"
	objdump -d bitrev.o --disassemble=bitrev8_perm
	@echo "--- table / library reversal grep (expect no output) ---"
	objdump -d bitrev.o | grep -Ei 'rbit|reverse|table' || true

clean:
	rm -f test_bitrev test_bitrev_asan test_bitrev_o0 *.o

.PHONY: all run sanitize opt0 disasm clean
