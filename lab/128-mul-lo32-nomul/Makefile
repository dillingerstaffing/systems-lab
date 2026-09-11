CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench disasm

all: test_mullo32_o2

test_mullo32_o2: test_mullo32.c mul32.c mul32.h
	$(CC) $(CFLAGS) -O2 -o $@ test_mullo32.c mul32.c

test_mullo32_o0: test_mullo32.c mul32.c mul32.h
	$(CC) $(CFLAGS) -O0 -o $@ test_mullo32.c mul32.c

test_mullo32_asan: test_mullo32.c mul32.c mul32.h
	$(CC) $(CFLAGS) -O2 $(SAN) -o $@ test_mullo32.c mul32.c

bench_mullo32: bench_mullo32.c mul32.c mul32.h
	$(CC) $(CFLAGS) -O2 -o $@ bench_mullo32.c mul32.c

mul32.o: mul32.c mul32.h
	$(CC) $(CFLAGS) -O2 -c mul32.c -o $@

run: test_mullo32_o2 test_mullo32_o0 test_mullo32_asan
	./test_mullo32_o2
	./test_mullo32_o0
	./test_mullo32_asan

bench: bench_mullo32
	./bench_mullo32

# Programmatic multiply scan: the -O2 object code of the implementation
# (mul32.c only) must contain zero multiply-class instructions. objdump
# lines are "addr: bytes... <tab> mnemonic args", so a mnemonic matching
# mul/imul with an optional AT&T size suffix is the multiply class on
# x86-64. The make target fails if the count is not zero.
disasm: mul32.o
	objdump -d mul32.o
	@echo '--- multiply scan (mnemonic mul/imul) ---'
	@hits=$$(objdump -d mul32.o | grep -cE '^[[:space:]]*[0-9a-f]+:[[:space:]]*(([0-9a-f]{2} )+)[[:space:]]+i?mul[a-z]*([[:space:]]|$$)'); \
	echo "multiply instructions found: $$hits"; \
	test "$$hits" -eq 0

clean:
	rm -f test_mullo32_o2 test_mullo32_o0 test_mullo32_asan bench_mullo32 mul32.o
