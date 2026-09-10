CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_clmul

test_clmul: test_clmul.c clmul.h
	$(CC) $(CFLAGS) -o $@ test_clmul.c

run: test_clmul
	./test_clmul

sanitize: test_clmul.c clmul.h
	$(CC) $(SANFLAGS) -o test_clmul_asan test_clmul.c
	./test_clmul_asan

opt0: test_clmul.c clmul.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_clmul_o0 test_clmul.c
	./test_clmul_o0

clean:
	rm -f test_clmul test_clmul_asan test_clmul_o0

.PHONY: all run sanitize opt0 clean
