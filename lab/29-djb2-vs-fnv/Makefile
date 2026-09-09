CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_hash_o0 test_hash_o2 test_hash_asan

test_hash_o0: test_hash.c hash.c hash.h
	$(CC) $(CFLAGS) -O0 -o $@ test_hash.c hash.c

test_hash_o2: test_hash.c hash.c hash.h
	$(CC) $(CFLAGS) -O2 -o $@ test_hash.c hash.c

# AddressSanitizer + UBSan: proves the recurrence loops contain no
# undefined behavior (no signed overflow; all arithmetic unsigned,
# all buffer accesses bounded by len).
test_hash_asan: test_hash.c hash.c hash.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_hash.c hash.c

run: all
	./test_hash_o0
	./test_hash_o2
	./test_hash_asan

clean:
	rm -f test_hash_o0 test_hash_o2 test_hash_asan

.PHONY: all run clean
