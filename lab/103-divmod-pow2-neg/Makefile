CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_divmod

test_divmod: test_divmod.c divmod.h divmod.c
	$(CC) $(CFLAGS) -o $@ test_divmod.c divmod.c

run: test_divmod
	./test_divmod

sanitize: test_divmod.c divmod.h divmod.c
	$(CC) $(SANFLAGS) -o test_divmod_asan test_divmod.c divmod.c
	./test_divmod_asan

opt0: test_divmod.c divmod.h divmod.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_divmod_o0 test_divmod.c divmod.c
	./test_divmod_o0

disasm: test_divmod.c divmod.h divmod.c
	$(CC) $(CFLAGS) -c -o divmod.o divmod.c
	@echo "--- divmod_pow2 disassembly ---"
	objdump -d divmod.o --disassemble=divmod_pow2

clean:
	rm -f test_divmod test_divmod_asan test_divmod_o0 *.o
