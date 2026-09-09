CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror

all: test_align_o0 test_align_o2 test_align_asan

test_align_o0: test_align.c align.c align.h
	$(CC) $(CFLAGS) -O0 -o $@ test_align.c align.c

test_align_o2: test_align.c align.c align.h
	$(CC) $(CFLAGS) -O2 -o $@ test_align.c align.c

# AddressSanitizer + UBSan: confirms no UB in the shift/or propagation
# or the mask arithmetic (all unsigned, but the checkers prove it).
test_align_asan: test_align.c align.c align.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_align.c align.c

run: all
	./test_align_o0
	./test_align_o2
	./test_align_asan

clean:
	rm -f test_align_o0 test_align_o2 test_align_asan

.PHONY: all run clean
