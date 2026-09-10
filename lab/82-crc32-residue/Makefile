CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_residue

test_residue: test_residue.c crc32.h crc32.c
	$(CC) $(CFLAGS) -o $@ test_residue.c crc32.c

run: test_residue
	./test_residue

sanitize: test_residue.c crc32.h crc32.c
	$(CC) $(SANFLAGS) -o test_residue_asan test_residue.c crc32.c
	./test_residue_asan

opt0: test_residue.c crc32.h crc32.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_residue_o0 test_residue.c crc32.c
	./test_residue_o0

clean:
	rm -f test_residue test_residue_asan test_residue_o0

.PHONY: all run sanitize opt0 clean
