CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_minmax

test_minmax: test_minmax.c minmax.h
	$(CC) $(CFLAGS) -o $@ test_minmax.c

run: test_minmax
	./test_minmax

sanitize: test_minmax.c minmax.h
	$(CC) $(SANFLAGS) -o test_minmax_asan test_minmax.c
	./test_minmax_asan

opt0: test_minmax.c minmax.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_minmax_o0 test_minmax.c
	./test_minmax_o0

disasm: disasm_minmax.c minmax.h
	$(CC) $(CFLAGS) -c disasm_minmax.c -o disasm_minmax.o
	objdump -d disasm_minmax.o

clean:
	rm -f test_minmax test_minmax_asan test_minmax_o0 disasm_minmax.o

.PHONY: all run sanitize opt0 disasm clean
