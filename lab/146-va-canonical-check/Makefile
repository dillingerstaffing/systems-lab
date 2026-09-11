CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_canonical_va

test_canonical_va: test_canonical_va.c canonical_va.h canonical_va.c
	$(CC) $(CFLAGS) -o $@ test_canonical_va.c canonical_va.c

run: test_canonical_va
	./test_canonical_va

sanitize: test_canonical_va.c canonical_va.h canonical_va.c
	$(CC) $(SANFLAGS) -o test_canonical_va_asan test_canonical_va.c canonical_va.c
	./test_canonical_va_asan

opt0: test_canonical_va.c canonical_va.h canonical_va.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_canonical_va_o0 test_canonical_va.c canonical_va.c
	./test_canonical_va_o0

disasm: test_canonical_va.c canonical_va.h canonical_va.c
	$(CC) $(CFLAGS) -c -o canonical_va.o canonical_va.c
	@echo "--- canonical_va disassembly ---"
	objdump -d canonical_va.o --disassemble=canonical_va

bench: test_canonical_va.c canonical_va.h canonical_va.c bench_canonical_va.c
	$(CC) $(CFLAGS) -o bench_canonical_va bench_canonical_va.c canonical_va.c
	./bench_canonical_va

clean:
	rm -f test_canonical_va test_canonical_va_asan test_canonical_va_o0 bench_canonical_va *.o
