CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_bit_span_o0 test_bit_span_o2 test_bit_span_asan

test_bit_span_o0: test_bit_span.c bit_span.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_bit_span.c

test_bit_span_o2: test_bit_span.c bit_span.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_bit_span.c

# AddressSanitizer + UBSan: the identities use only unsigned 64-bit
# shifts, ORs, adds, subtracts, multiplies, a NOT, and a table read
# at a 6-bit index, so no undefined behavior is possible; the
# sanitizers confirm it empirically on the full case set.  (The
# naive references shift only by 0..63, also fully defined.  The
# de Bruijn table is derived in the test and checked entry by
# entry against the header's hardcoded copy.)
test_bit_span_asan: test_bit_span.c bit_span.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_bit_span.c

run: all
	./test_bit_span_o0
	./test_bit_span_o2
	./test_bit_span_asan

clean:
	rm -f test_bit_span_o0 test_bit_span_o2 test_bit_span_asan

.PHONY: all run clean
