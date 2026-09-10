CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_halfadder_o2

test_halfadder_o2: test_halfadder.c halfadder.h halfadder.c
	$(CC) $(CFLAGS) -o $@ test_halfadder.c halfadder.c

run: test_halfadder_o2
	./test_halfadder_o2

sanitize: test_halfadder.c halfadder.h halfadder.c
	$(CC) $(SANFLAGS) -o test_halfadder_asan test_halfadder.c halfadder.c
	./test_halfadder_asan

opt0: test_halfadder.c halfadder.h halfadder.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_halfadder_o0 test_halfadder.c halfadder.c
	./test_halfadder_o0

disasm: test_halfadder.c halfadder.h halfadder.c
	$(CC) $(CFLAGS) -c -o halfadder.o halfadder.c
	@echo "--- add_carry_chain_w disassembly ---"
	objdump -d halfadder.o --disassemble=add_carry_chain_w
	@echo "--- add-instruction grep over impl object (expect only loop-counter inc/add, none on operand datapath) ---"
	objdump -d halfadder.o | grep -E '\b(add|adc)\b' || true

clean:
	rm -f test_halfadder_o2 test_halfadder_asan test_halfadder_o0 *.o

.PHONY: all run sanitize opt0 disasm clean
