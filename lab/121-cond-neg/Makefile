CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench disasm

all: test_cond_neg

test_cond_neg: test_cond_neg.c cond_neg.c cond_neg.h
	$(CC) $(CFLAGS) -o $@ test_cond_neg.c cond_neg.c

test_cond_neg_O0: test_cond_neg.c cond_neg.c cond_neg.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_cond_neg.c cond_neg.c

test_cond_neg_san: test_cond_neg.c cond_neg.c cond_neg.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_cond_neg.c cond_neg.c

test_cond_neg_bench: test_cond_neg.c cond_neg.c cond_neg.h
	$(CC) $(CFLAGS) -DBENCH -o $@ test_cond_neg.c

run: test_cond_neg test_cond_neg_O0 test_cond_neg_san
	./test_cond_neg
	./test_cond_neg_O0
	./test_cond_neg_san

bench: test_cond_neg_bench
	./test_cond_neg_bench

# Programmatic jump scan: the -O2 object code of cond_neg must contain
# zero jump instructions (conditional or otherwise). objdump lines are
# "addr: bytes... <tab> mnemonic args", so a mnemonic beginning with
# 'j' is a jump (all x86 jump mnemonics start with 'j').
disasm:
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -c cond_neg.c -o cond_neg.o
	objdump -d --disassemble=cond_neg cond_neg.o
	@echo '--- jump scan (mnemonic begins with j) ---'
	@objdump -d --disassemble=cond_neg cond_neg.o | \
		grep -cE '^[[:space:]]*[0-9a-f]+:[[:space:]]*(([0-9a-f]{2} )+)[[:space:]]*j[a-z]+([[:space:]]|$$)' \
		| xargs -I{} echo "jump instructions found: {}"

clean:
	rm -f test_cond_neg test_cond_neg_O0 test_cond_neg_san test_cond_neg_bench cond_neg.o
