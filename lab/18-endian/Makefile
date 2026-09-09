CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_endian_o0 test_endian_o2 test_endian_asan

test_endian_o0: test_endian.c endian.c endian.h
	$(CC) $(CFLAGS) -O0 -o $@ test_endian.c endian.c

test_endian_o2: test_endian.c endian.c endian.h
	$(CC) $(CFLAGS) -O2 -o $@ test_endian.c endian.c

# AddressSanitizer + UBSan: confirms the mask arithmetic and the
# shifts contain no undefined behavior (all unsigned shifts are
# within width, but the checkers prove it).
test_endian_asan: test_endian.c endian.c endian.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_endian.c endian.c

run: all
	./test_endian_o0
	./test_endian_o2
	./test_endian_asan

clean:
	rm -f test_endian_o0 test_endian_o2 test_endian_asan

.PHONY: all run clean
