CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_signextend

test_signextend: test_signextend.c signextend.h
	$(CC) $(CFLAGS) -o $@ test_signextend.c

o2: test_signextend.c signextend.h
	$(CC) $(CFLAGS) -o test_signextend_o2 test_signextend.c
	./test_signextend_o2

opt0: test_signextend.c signextend.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_signextend_o0 test_signextend.c
	./test_signextend_o0

sanitize: test_signextend.c signextend.h
	$(CC) $(SANFLAGS) -o test_signextend_asan test_signextend.c
	./test_signextend_asan

run: test_signextend
	./test_signextend

disasm: disasm_signextend.c signextend.h
	$(CC) $(CFLAGS) -c disasm_signextend.c -o disasm_signextend.o
	objdump -d disasm_signextend.o

clean:
	rm -f test_signextend test_signextend_o2 test_signextend_o0 test_signextend_asan disasm_signextend.o

.PHONY: all run o2 opt0 sanitize disasm clean
