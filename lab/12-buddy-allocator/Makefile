CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror

BUMP_SRC = ../02-bump-allocator/bump.c

all: test_buddy

test_buddy: test_buddy.c buddy.c buddy.h $(BUMP_SRC) ../02-bump-allocator/bump.h
	$(CC) $(CFLAGS) -o $@ test_buddy.c buddy.c $(BUMP_SRC)

# AddressSanitizer + UBSan run: proves no OOB access or UB in the
# allocator's pointer arithmetic (free-list nodes live inside the heap).
asan: test_buddy.c buddy.c buddy.h $(BUMP_SRC)
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra \
		-fno-omit-frame-pointer -o test_buddy_asan test_buddy.c buddy.c $(BUMP_SRC)

run: test_buddy
	./test_buddy

run-asan: asan
	./test_buddy_asan

clean:
	rm -f test_buddy test_buddy_asan

.PHONY: all asan run run-asan clean
