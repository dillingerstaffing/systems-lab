CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_pte_step

test_pte_step: test_pte_step.c pte_step.h pte_step.c oracle_step.h oracle_step.c
	$(CC) $(CFLAGS) -o $@ test_pte_step.c pte_step.c oracle_step.c

run: test_pte_step
	./test_pte_step

sanitize: test_pte_step.c pte_step.h pte_step.c oracle_step.h oracle_step.c
	$(CC) $(SANFLAGS) -o test_pte_step_asan test_pte_step.c pte_step.c oracle_step.c
	./test_pte_step_asan

opt0: test_pte_step.c pte_step.h pte_step.c oracle_step.h oracle_step.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_pte_step_o0 test_pte_step.c pte_step.c oracle_step.c
	./test_pte_step_o0

bench: bench_pte_step.c pte_step.h pte_step.c
	$(CC) $(CFLAGS) -o bench_pte_step bench_pte_step.c pte_step.c
	./bench_pte_step

clean:
	rm -f test_pte_step test_pte_step_asan test_pte_step_o0 bench_pte_step *.o
