CC = gcc
STD = -std=c11
WARN = -Wall -Wextra -Werror
SRC = pte_perm.c oracle.c

all: oracle_table.h test_pte_perm_o0 test_pte_perm_o2 test_pte_perm_asan

oracle_table.h: scripts/gen_oracle.py
	python3 scripts/gen_oracle.py

test_pte_perm_o0: oracle_table.h $(SRC) test_pte_perm.c pte_perm.h oracle.h
	$(CC) $(STD) $(WARN) -O0 -g -o $@ $(SRC) test_pte_perm.c

test_pte_perm_o2: oracle_table.h $(SRC) test_pte_perm.c pte_perm.h oracle.h
	$(CC) $(STD) $(WARN) -O2 -o $@ $(SRC) test_pte_perm.c

test_pte_perm_asan: oracle_table.h $(SRC) test_pte_perm.c pte_perm.h oracle.h
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -o $@ $(SRC) test_pte_perm.c

bench_pte_perm: oracle_table.h pte_perm.c bench_pte_perm.c pte_perm.h
	$(CC) $(STD) $(WARN) -O2 -o $@ pte_perm.c bench_pte_perm.c

run: all
	./test_pte_perm_o0
	./test_pte_perm_o2
	./test_pte_perm_asan

bench: bench_pte_perm
	./bench_pte_perm

clean:
	rm -f test_pte_perm_o0 test_pte_perm_o2 test_pte_perm_asan bench_pte_perm oracle_table.h

.PHONY: all run bench clean
