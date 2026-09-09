CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_memmove_o0 test_memmove_o2 test_memmove_asan

test_memmove_o0: test_memmove.c memmove30.c memmove30.h
	$(CC) $(CFLAGS) -O0 -o $@ test_memmove.c memmove30.c

test_memmove_o2: test_memmove.c memmove30.c memmove30.h
	$(CC) $(CFLAGS) -O2 -o $@ test_memmove.c memmove30.c

# AddressSanitizer + UBSan: proves the byte loops, the pointer
# comparisons, and the size_t arithmetic contain no undefined behavior.
test_memmove_asan: test_memmove.c memmove30.c memmove30.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_memmove.c memmove30.c

run: all
	./test_memmove_o0
	./test_memmove_o2
	./test_memmove_asan

clean:
	rm -f test_memmove_o0 test_memmove_o2 test_memmove_asan

.PHONY: all run clean
