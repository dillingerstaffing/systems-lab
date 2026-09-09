CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_bgcd_o0 test_bgcd_o2 test_bgcd_asan

test_bgcd_o0: test_bgcd.c bgcd.c bgcd.h
	$(CC) $(CFLAGS) -O0 -o $@ test_bgcd.c bgcd.c

test_bgcd_o2: test_bgcd.c bgcd.c bgcd.h
	$(CC) $(CFLAGS) -O2 -o $@ test_bgcd.c bgcd.c

# AddressSanitizer + UBSan: proves the shift/subtract loop contains no
# undefined behavior (no oversized shifts, no unsigned wrap assumptions).
test_bgcd_asan: test_bgcd.c bgcd.c bgcd.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_bgcd.c bgcd.c

run: all
	./test_bgcd_o0
	./test_bgcd_o2
	./test_bgcd_asan

clean:
	rm -f test_bgcd_o0 test_bgcd_o2 test_bgcd_asan

.PHONY: all run clean
