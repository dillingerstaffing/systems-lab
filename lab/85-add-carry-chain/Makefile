CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_add128

test_add128: test_add128.c add128.h add128.c
	$(CC) $(CFLAGS) -o $@ test_add128.c add128.c

run: test_add128
	./test_add128 quick

run-full: test_add128
	./test_add128 full

bench: test_add128
	./test_add128 bench

sanitize: test_add128.c add128.h add128.c
	$(CC) $(SANFLAGS) -o test_add128_asan test_add128.c add128.c
	./test_add128_asan quick

opt0: test_add128.c add128.h add128.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_add128_o0 test_add128.c add128.c
	./test_add128_o0 quick

disasm: add128.c add128.h
	$(CC) $(CFLAGS) -c -o add128.o add128.c
	@echo "--- add128 disassembly ---"
	objdump -d add128.o --disassemble=add128
	@echo "--- __int128 grep over add128 sources (expect no output) ---"
	@grep -n "__int128" add128.c add128.h || true

clean:
	rm -f test_add128 test_add128_asan test_add128_o0 *.o

.PHONY: all run run-full bench sanitize opt0 disasm clean
