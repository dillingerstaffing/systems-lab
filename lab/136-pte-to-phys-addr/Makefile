CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_phys

test_pte_phys: test_pte_phys.c pte_phys.h pte_phys.c
	$(CC) $(CFLAGS) -o $@ test_pte_phys.c pte_phys.c

run: test_pte_phys
	./test_pte_phys

sanitize: test_pte_phys.c pte_phys.h pte_phys.c
	$(CC) $(SANFLAGS) -o test_pte_phys_asan test_pte_phys.c pte_phys.c
	./test_pte_phys_asan

opt0: test_pte_phys.c pte_phys.h pte_phys.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_phys_o0 test_pte_phys.c pte_phys.c
	./test_pte_phys_o0

bench: bench_pte_phys.c pte_phys.h pte_phys.c
	$(CC) $(CFLAGS) -o bench_pte_phys bench_pte_phys.c pte_phys.c
	./bench_pte_phys

clean:
	rm -f test_pte_phys test_pte_phys_asan test_pte_phys_o0 bench_pte_phys *.o
