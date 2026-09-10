CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_roundup

test_roundup: test_roundup.c roundup.h roundup.c
	$(CC) $(CFLAGS) -o $@ test_roundup.c roundup.c

run: test_roundup
	./test_roundup

sanitize: test_roundup.c roundup.h roundup.c
	$(CC) $(SANFLAGS) -o test_roundup_asan test_roundup.c roundup.c
	./test_roundup_asan

opt0: test_roundup.c roundup.h roundup.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_roundup_o0 test_roundup.c roundup.c
	./test_roundup_o0

disasm: test_roundup.c roundup.h roundup.c
	$(CC) $(CFLAGS) -c -o roundup.o roundup.c
	@echo "--- round_up_pow2_64 disassembly ---"
	objdump -d roundup.o --disassemble=round_up_pow2_64
	@echo "--- builtin-instruction grep (expect no output) ---"
	objdump -d roundup.o | grep -Ei 'tzcnt|bsf|popcnt|l zcnt' || true

clean:
	rm -f test_roundup test_roundup_asan test_roundup_o0 *.o

.PHONY: all run sanitize opt0 disasm clean
