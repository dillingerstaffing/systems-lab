CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_global

test_pte_global: test_pte_global.c pte_global.h pte_global.c
	$(CC) $(CFLAGS) -o $@ test_pte_global.c pte_global.c

run: test_pte_global
	./test_pte_global

sanitize: test_pte_global.c pte_global.h pte_global.c
	$(CC) $(SANFLAGS) -o test_pte_global_asan test_pte_global.c pte_global.c
	./test_pte_global_asan

opt0: test_pte_global.c pte_global.h pte_global.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_global_o0 test_pte_global.c pte_global.c
	./test_pte_global_o0

disasm: test_pte_global.c pte_global.h pte_global.c
	$(CC) $(CFLAGS) -c -o pte_global.o pte_global.c
	@echo "--- sv39_walk disassembly ---"
	objdump -d pte_global.o --disassemble=sv39_walk
	@echo "--- tlb_lookup disassembly ---"
	objdump -d pte_global.o --disassemble=tlb_lookup

bench: test_pte_global.c pte_global.h pte_global.c bench_tlb_lookup.c
	$(CC) $(CFLAGS) -o bench_tlb_lookup bench_tlb_lookup.c pte_global.c
	./bench_tlb_lookup

clean:
	rm -f test_pte_global test_pte_global_asan test_pte_global_o0 bench_tlb_lookup *.o
