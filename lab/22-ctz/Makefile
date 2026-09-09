CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_ctz_o0 test_ctz_o2 test_ctz_asan

test_ctz_o0: test_ctz.c ctz.c ctz.h
	$(CC) $(CFLAGS) -O0 -o $@ test_ctz.c ctz.c

test_ctz_o2: test_ctz.c ctz.c ctz.h
	$(CC) $(CFLAGS) -O2 -o $@ test_ctz.c ctz.c

# AddressSanitizer + UBSan: proves the multiply/table path contains no
# undefined behavior (no out-of-range table index, no bad shifts).
test_ctz_asan: test_ctz.c ctz.c ctz.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_ctz.c ctz.c

run: all
	./test_ctz_o0
	./test_ctz_o2
	./test_ctz_asan

clean:
	rm -f test_ctz_o0 test_ctz_o2 test_ctz_asan

.PHONY: all run clean
