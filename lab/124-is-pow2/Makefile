CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_ispow2

test_ispow2: test_ispow2.c ispow2.h ispow2.c
	$(CC) $(CFLAGS) -o $@ test_ispow2.c ispow2.c

run: test_ispow2
	./test_ispow2

sanitize: test_ispow2.c ispow2.h ispow2.c
	$(CC) $(SANFLAGS) -o test_ispow2_asan test_ispow2.c ispow2.c
	./test_ispow2_asan

opt0: test_ispow2.c ispow2.h ispow2.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_ispow2_o0 test_ispow2.c ispow2.c
	./test_ispow2_o0

disasm: test_ispow2.c ispow2.h ispow2.c
	$(CC) $(CFLAGS) -c -o ispow2.o ispow2.c
	@echo "--- is_pow2 disassembly ---"
	objdump -d ispow2.o --disassemble=is_pow2

bench: test_ispow2.c ispow2.h ispow2.c bench_ispow2.c
	$(CC) $(CFLAGS) -o bench_ispow2 bench_ispow2.c ispow2.c
	./bench_ispow2

clean:
	rm -f test_ispow2 test_ispow2_asan test_ispow2_o0 bench_ispow2 *.o
