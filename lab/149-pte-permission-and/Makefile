CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_walk_permits

test_walk_permits: test_walk_permits.c walk_permits.h walk_permits.c
	$(CC) $(CFLAGS) -o $@ test_walk_permits.c walk_permits.c

run: test_walk_permits
	./test_walk_permits

sanitize: test_walk_permits.c walk_permits.h walk_permits.c
	$(CC) $(SANFLAGS) -o test_walk_permits_asan test_walk_permits.c walk_permits.c
	./test_walk_permits_asan

opt0: test_walk_permits.c walk_permits.h walk_permits.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_walk_permits_o0 test_walk_permits.c walk_permits.c
	./test_walk_permits_o0

bench: bench_walk_permits.c walk_permits.h walk_permits.c
	$(CC) $(CFLAGS) -o bench_walk_permits bench_walk_permits.c walk_permits.c
	./bench_walk_permits

clean:
	rm -f test_walk_permits test_walk_permits_asan test_walk_permits_o0 bench_walk_permits *.o
