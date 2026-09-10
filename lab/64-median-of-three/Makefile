CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_median3_o2

test_median3_o2: test_median3.c median3.h
	$(CC) $(CFLAGS) -o $@ test_median3.c

run: test_median3_o2
	./test_median3_o2

sanitize: test_median3.c median3.h
	$(CC) $(SANFLAGS) -o test_median3_asan test_median3.c
	./test_median3_asan

opt0: test_median3.c median3.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_median3_o0 test_median3.c
	./test_median3_o0

disasm: disasm_median3.c median3.h
	$(CC) $(CFLAGS) -c disasm_median3.c -o disasm_median3.o
	objdump -d disasm_median3.o
	@echo "conditional jumps in med3_wrap:"
	@objdump -d disasm_median3.o | grep -E '\tj[a-z]+\s' | grep -v '\tjmp\s' | wc -l

clean:
	rm -f test_median3_o2 test_median3_asan test_median3_o0 disasm_median3.o

.PHONY: all run sanitize opt0 disasm clean
