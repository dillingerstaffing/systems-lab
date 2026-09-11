CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2
SAN = -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all run clean bench disasm

all: test_decimal_digits

test_decimal_digits: test_decimal_digits.c decimal_digits.c decimal_digits.h
	$(CC) $(CFLAGS) -o $@ test_decimal_digits.c decimal_digits.c

test_decimal_digits_O0: test_decimal_digits.c decimal_digits.c decimal_digits.h
	$(CC) -std=c11 -Wall -Wextra -Werror -O0 -o $@ test_decimal_digits.c decimal_digits.c

test_decimal_digits_san: test_decimal_digits.c decimal_digits.c decimal_digits.h
	$(CC) $(CFLAGS) $(SAN) -o $@ test_decimal_digits.c decimal_digits.c

test_decimal_digits_bench: test_decimal_digits.c decimal_digits.c decimal_digits.h
	$(CC) $(CFLAGS) -DBENCH -o $@ test_decimal_digits.c

run: test_decimal_digits test_decimal_digits_O0 test_decimal_digits_san
	./test_decimal_digits
	./test_decimal_digits_O0
	./test_decimal_digits_san

bench: test_decimal_digits_bench
	./test_decimal_digits_bench

# Programmatic scans of the -O2 object code of digit_count:
# - jump scan: no conditional/unconditional jump instructions
#   (all x86 jump mnemonics start with 'j');
# - div scan: no division instructions (div/idiv).
disasm:
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -c decimal_digits.c -o decimal_digits.o
	objdump -d --disassemble=digit_count decimal_digits.o
	@echo '--- jump scan (mnemonic begins with j) ---'
	@objdump -d --disassemble=digit_count decimal_digits.o | \
		grep -cE '^[[:space:]]*[0-9a-f]+:[[:space:]]*(([0-9a-f]{2} )+)[[:space:]]*j[a-z]+([[:space:]]|$$)' \
		| xargs -I{} echo "jump instructions found: {}"
	@echo '--- div scan (div/idiv mnemonics) ---'
	@objdump -d --disassemble=digit_count decimal_digits.o | \
		grep -cE '[[:space:]](i?div)[a-z]*([[:space:]]|$$)' \
		| xargs -I{} echo "division instructions found: {}"

clean:
	rm -f test_decimal_digits test_decimal_digits_O0 test_decimal_digits_san test_decimal_digits_bench decimal_digits.o
