CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_permits

test_pte_permits: test_pte_permits.c pte_permits.h pte_permits.c
	$(CC) $(CFLAGS) -o $@ test_pte_permits.c pte_permits.c

run: test_pte_permits
	./test_pte_permits

sanitize: test_pte_permits.c pte_permits.h pte_permits.c
	$(CC) $(SANFLAGS) -o test_pte_permits_asan test_pte_permits.c pte_permits.c
	./test_pte_permits_asan

opt0: test_pte_permits.c pte_permits.h pte_permits.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_permits_o0 test_pte_permits.c pte_permits.c
	./test_pte_permits_o0

bench: bench_pte_permits.c pte_permits.h pte_permits.c
	$(CC) $(CFLAGS) -o bench_pte_permits bench_pte_permits.c pte_permits.c
	./bench_pte_permits

clean:
	rm -f test_pte_permits test_pte_permits_asan test_pte_permits_o0 bench_pte_permits *.o
