CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_superpage

test_pte_superpage: test_pte_superpage.c pte_superpage.h pte_superpage.c
	$(CC) $(CFLAGS) -o $@ test_pte_superpage.c pte_superpage.c

run: test_pte_superpage
	./test_pte_superpage

sanitize: test_pte_superpage.c pte_superpage.h pte_superpage.c
	$(CC) $(SANFLAGS) -o test_pte_superpage_asan test_pte_superpage.c pte_superpage.c
	./test_pte_superpage_asan

opt0: test_pte_superpage.c pte_superpage.h pte_superpage.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_superpage_o0 test_pte_superpage.c pte_superpage.c
	./test_pte_superpage_o0

bench: bench_pte_superpage.c pte_superpage.h pte_superpage.c
	$(CC) $(CFLAGS) -o bench_pte_superpage bench_pte_superpage.c pte_superpage.c
	./bench_pte_superpage

clean:
	rm -f test_pte_superpage test_pte_superpage_asan test_pte_superpage_o0 bench_pte_superpage *.o
