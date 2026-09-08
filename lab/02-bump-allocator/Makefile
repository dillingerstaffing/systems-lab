CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror

all: test_bump

test_bump: test_bump.c bump.c bump.h
	$(CC) $(CFLAGS) -o $@ test_bump.c bump.c

# AddressSanitizer + UBSan run: proves no OOB access or UB in the
# allocator's pointer arithmetic (free-list nodes live inside the heap).
asan: test_bump.c bump.c bump.h
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra \
		-fno-omit-frame-pointer -o test_bump_asan test_bump.c bump.c

run: test_bump
	./test_bump

run-asan: asan
	./test_bump_asan

clean:
	rm -f test_bump test_bump_asan

.PHONY: all asan run run-asan clean
