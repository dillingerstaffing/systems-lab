CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_a

test_pte_a: test_pte_a.c pte_a.h pte_a.c
	$(CC) $(CFLAGS) -o $@ test_pte_a.c pte_a.c

run: test_pte_a
	./test_pte_a

sanitize: test_pte_a.c pte_a.h pte_a.c
	$(CC) $(SANFLAGS) -o test_pte_a_asan test_pte_a.c pte_a.c
	./test_pte_a_asan

opt0: test_pte_a.c pte_a.h pte_a.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_a_o0 test_pte_a.c pte_a.c
	./test_pte_a_o0

disasm: test_pte_a.c pte_a.h pte_a.c
	$(CC) $(CFLAGS) -c -o pte_a.o pte_a.c
	@echo "--- pte_a disassembly ---"
	objdump -d pte_a.o --disassemble=pte_a_check

bench: test_pte_a.c pte_a.h pte_a.c bench_pte_a.c
	$(CC) $(CFLAGS) -o bench_pte_a bench_pte_a.c pte_a.c
	./bench_pte_a

clean:
	rm -f test_pte_a test_pte_a_asan test_pte_a_o0 bench_pte_a *.o
