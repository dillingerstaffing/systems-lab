# Treiber stack, built on the hardware double-width CAS.
#
# No libatomic: the 16-byte head is driven by inline `lock cmpxchg16b`, the
# one instruction the algorithm's atomicity rests on. x86-64 required.
#
# No ThreadSanitizer target: TSan cannot see inside inline assembly, so it
# would report the head as an uninstrumented race (a false positive). The
# atomicity argument lives in treiber.c and rests on the ISA, not on the
# sanitizer.

CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror -pthread

all: test_treiber

test_treiber: test_treiber.c treiber.c treiber.h
	$(CC) $(CFLAGS) -o $@ test_treiber.c treiber.c

run: test_treiber
	./test_treiber

sanitize:
	$(CC) -std=c11 -O1 -g -Wall -Wextra -Werror -pthread \
		-fsanitize=address,undefined \
		-o test_treiber_asan test_treiber.c treiber.c
	./test_treiber_asan

opt0:
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -pthread \
		-o test_treiber_o0 test_treiber.c treiber.c
	./test_treiber_o0

clean:
	rm -f test_treiber test_treiber_asan test_treiber_o0

.PHONY: all run sanitize opt0 clean
