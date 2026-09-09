CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_saturating_o0 test_saturating_o2 test_saturating_asan

test_saturating_o0: test_saturating.c saturating.c saturating.h
	$(CC) $(CFLAGS) -O0 -o $@ test_saturating.c saturating.c

test_saturating_o2: test_saturating.c saturating.c saturating.h
	$(CC) $(CFLAGS) -O2 -o $@ test_saturating.c saturating.c

# AddressSanitizer + UBSan: proves the sign-bit arithmetic and the
# uint32_t <-> int32_t conversions contain no undefined behavior.
test_saturating_asan: test_saturating.c saturating.c saturating.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_saturating.c saturating.c

run: all
	./test_saturating_o0
	./test_saturating_o2
	./test_saturating_asan

clean:
	rm -f test_saturating_o0 test_saturating_o2 test_saturating_asan

.PHONY: all run clean
