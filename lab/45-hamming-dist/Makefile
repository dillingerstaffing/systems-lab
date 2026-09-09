CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror

all: test_hamming_o0 test_hamming_o2 test_hamming_asan

test_hamming_o0: test_hamming.c hamming.c hamming.h
	$(CC) $(CFLAGS) -O0 -o $@ test_hamming.c hamming.c

test_hamming_o2: test_hamming.c hamming.c hamming.h
	$(CC) $(CFLAGS) -O2 -o $@ test_hamming.c hamming.c

# AddressSanitizer + UBSan: confirms the XOR and the SWAR mask
# arithmetic contain no undefined behavior (all unsigned, but the
# checkers prove it).
test_hamming_asan: test_hamming.c hamming.c hamming.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_hamming.c hamming.c

run: all
	./test_hamming_o0
	./test_hamming_o2
	./test_hamming_asan

clean:
	rm -f test_hamming_o0 test_hamming_o2 test_hamming_asan

.PHONY: all run clean
