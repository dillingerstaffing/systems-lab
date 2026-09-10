CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_tie

test_tie: test_tie.c tie.h tie.c
	$(CC) $(CFLAGS) -o $@ test_tie.c tie.c

run: test_tie
	./test_tie

sanitize: test_tie.c tie.h tie.c
	$(CC) $(SANFLAGS) -o test_tie_asan test_tie.c tie.c
	./test_tie_asan

opt0: test_tie.c tie.h tie.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_tie_o0 test_tie.c tie.c
	./test_tie_o0

disasm: test_tie.c tie.h tie.c
	$(CC) $(CFLAGS) -c -o tie.o tie.c
	@echo "--- tie_to_even_half disassembly ---"
	objdump -d tie.o --disassemble=tie_to_even_half
	@echo "--- division-instruction grep (expect no output) ---"
	objdump -d tie.o | grep -Ei 'div|idiv' || true

clean:
	rm -f test_tie test_tie_asan test_tie_o0 *.o

.PHONY: all run sanitize opt0 disasm clean
