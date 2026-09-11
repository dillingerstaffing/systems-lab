CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_decode

test_pte_decode: test_pte_decode.c pte_decode.h pte_decode.c
	$(CC) $(CFLAGS) -o $@ test_pte_decode.c pte_decode.c

run: test_pte_decode
	./test_pte_decode

sanitize: test_pte_decode.c pte_decode.h pte_decode.c
	$(CC) $(SANFLAGS) -o test_pte_decode_asan test_pte_decode.c pte_decode.c
	./test_pte_decode_asan

opt0: test_pte_decode.c pte_decode.h pte_decode.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_decode_o0 test_pte_decode.c pte_decode.c
	./test_pte_decode_o0

bench: test_pte_decode.c pte_decode.h pte_decode.c bench_pte_decode.c
	$(CC) $(CFLAGS) -o bench_pte_decode bench_pte_decode.c pte_decode.c
	./bench_pte_decode

clean:
	rm -f test_pte_decode test_pte_decode_asan test_pte_decode_o0 bench_pte_decode *.o
