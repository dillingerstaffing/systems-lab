CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_vpn_extract

test_vpn_extract: test_vpn_extract.c vpn_extract.h vpn_extract.c
	$(CC) $(CFLAGS) -o $@ test_vpn_extract.c vpn_extract.c

run: test_vpn_extract
	./test_vpn_extract

sanitize: test_vpn_extract.c vpn_extract.h vpn_extract.c
	$(CC) $(SANFLAGS) -o test_vpn_extract_asan test_vpn_extract.c vpn_extract.c
	./test_vpn_extract_asan

opt0: test_vpn_extract.c vpn_extract.h vpn_extract.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_vpn_extract_o0 test_vpn_extract.c vpn_extract.c
	./test_vpn_extract_o0

bench: test_vpn_extract.c vpn_extract.h vpn_extract.c bench_vpn_extract.c
	$(CC) $(CFLAGS) -o bench_vpn_extract bench_vpn_extract.c vpn_extract.c
	./bench_vpn_extract

clean:
	rm -f test_vpn_extract test_vpn_extract_asan test_vpn_extract_o0 bench_vpn_extract *.o
