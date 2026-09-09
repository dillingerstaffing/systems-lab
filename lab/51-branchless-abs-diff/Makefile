CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_absdiff

test_absdiff: test_absdiff.c absdiff.h
	$(CC) $(CFLAGS) -o $@ test_absdiff.c

run: test_absdiff
	./test_absdiff

sanitize: test_absdiff.c absdiff.h
	$(CC) $(SANFLAGS) -o test_absdiff_asan test_absdiff.c
	./test_absdiff_asan

opt0: test_absdiff.c absdiff.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_absdiff_o0 test_absdiff.c
	./test_absdiff_o0

disasm: disasm_absdiff.c absdiff.h
	$(CC) $(CFLAGS) -c disasm_absdiff.c -o disasm_absdiff.o
	objdump -d disasm_absdiff.o

clean:
	rm -f test_absdiff test_absdiff_asan test_absdiff_o0 disasm_absdiff.o

.PHONY: all run sanitize opt0 disasm clean
