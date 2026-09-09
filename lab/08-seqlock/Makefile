# lab/08-seqlock: single-writer / multi-reader sequence lock, header-only.
#
# Writer: seq odd (relaxed), payload stores, seq even (release).
# Reader: acquire seq, plain copy, acquire seq; retry if odd or changed.
# No ThreadSanitizer target: TSan cannot see the sequence protocol, so the
# plain payload copy would be reported as a race (a false positive); the
# ordering argument lives in seqlock.h and rests on release/acquire plus
# the writer's total order on seq.

CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror -pthread

all: test_seqlock

test_seqlock: test_seqlock.c seqlock.h
	$(CC) $(CFLAGS) -o $@ test_seqlock.c

run: test_seqlock
	./test_seqlock

sanitize:
	$(CC) -std=c11 -O1 -g -Wall -Wextra -Werror -pthread \
		-fsanitize=address,undefined \
		-o test_seqlock_asan test_seqlock.c
	./test_seqlock_asan

opt0:
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -pthread \
		-o test_seqlock_o0 test_seqlock.c
	./test_seqlock_o0

clean:
	rm -f test_seqlock test_seqlock_asan test_seqlock_o0

.PHONY: all run sanitize opt0 clean
