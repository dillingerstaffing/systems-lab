CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_iso

test_iso: test_iso.c iso.h iso.c
	$(CC) $(CFLAGS) -o $@ test_iso.c iso.c

run: test_iso
	./test_iso

sanitize: test_iso.c iso.h iso.c
	$(CC) $(SANFLAGS) -o test_iso_asan test_iso.c iso.c
	./test_iso_asan

opt0: test_iso.c iso.h iso.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_iso_o0 test_iso.c iso.c
	./test_iso_o0

disasm: test_iso.c iso.h iso.c
	$(CC) $(CFLAGS) -c -o iso.o iso.c
	@echo "--- iso_lowest disassembly ---"
	objdump -d iso.o --disassemble=iso_lowest

clean:
	rm -f test_iso test_iso_asan test_iso_o0 *.o
