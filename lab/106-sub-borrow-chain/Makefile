CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_subborrow

test_subborrow: test_subborrow.c subborrow.h subborrow.c
	$(CC) $(CFLAGS) -o $@ test_subborrow.c subborrow.c

run: test_subborrow
	./test_subborrow

sanitize: test_subborrow.c subborrow.h subborrow.c
	$(CC) $(SANFLAGS) -o test_subborrow_asan test_subborrow.c subborrow.c
	./test_subborrow_asan

opt0: test_subborrow.c subborrow.h subborrow.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_subborrow_o0 test_subborrow.c subborrow.c
	./test_subborrow_o0

disasm: test_subborrow.c subborrow.h subborrow.c
	$(CC) $(CFLAGS) -c -o subborrow.o subborrow.c
	@echo "--- sub_borrow_chain disassembly ---"
	objdump -d subborrow.o --disassemble=sub_borrow_chain
	@echo "--- __int128 grep (expect no output) ---"
	objdump -d subborrow.o | grep -i '__int128' || true

clean:
	rm -f test_subborrow test_subborrow_asan test_subborrow_o0 *.o

.PHONY: all run sanitize opt0 disasm clean
