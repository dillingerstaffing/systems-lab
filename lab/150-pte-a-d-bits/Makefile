CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_ad

test_pte_ad: test_pte_ad.c pte_ad.h pte_ad.c
	$(CC) $(CFLAGS) -o $@ test_pte_ad.c pte_ad.c

run: test_pte_ad
	./test_pte_ad

sanitize: test_pte_ad.c pte_ad.h pte_ad.c
	$(CC) $(SANFLAGS) -o test_pte_ad_asan test_pte_ad.c pte_ad.c
	./test_pte_ad_asan

opt0: test_pte_ad.c pte_ad.h pte_ad.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_ad_o0 test_pte_ad.c pte_ad.c
	./test_pte_ad_o0

bench: bench_pte_ad.c pte_ad.h pte_ad.c
	$(CC) $(CFLAGS) -o bench_pte_ad bench_pte_ad.c pte_ad.c
	./bench_pte_ad

clean:
	rm -f test_pte_ad test_pte_ad_asan test_pte_ad_o0 bench_pte_ad *.o
