CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_mask

test_mask: test_mask.c mask.h
	$(CC) $(CFLAGS) -o $@ test_mask.c

run: test_mask
	./test_mask

sanitize: test_mask.c mask.h
	$(CC) $(SANFLAGS) -o test_mask_asan test_mask.c
	./test_mask_asan

opt0: test_mask.c mask.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_mask_o0 test_mask.c
	./test_mask_o0

disasm: disasm_mask.c mask.h
	$(CC) $(CFLAGS) -c disasm_mask.c -o disasm_mask.o
	objdump -d disasm_mask.o

clean:
	rm -f test_mask test_mask_asan test_mask_o0 disasm_mask.o

.PHONY: all run sanitize opt0 disasm clean
