CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_floor_log2_o0 test_floor_log2_o2 test_floor_log2_asan

test_floor_log2_o0: test_floor_log2.c floor_log2.c floor_log2.h
	$(CC) $(CFLAGS) -O0 -o $@ test_floor_log2.c floor_log2.c

test_floor_log2_o2: test_floor_log2.c floor_log2.c floor_log2.h
	$(CC) $(CFLAGS) -O2 -o $@ test_floor_log2.c floor_log2.c

# AddressSanitizer + UBSan: proves the shift/OR propagation and SWAR
# popcount contain no undefined behavior (no bad shifts, no signed
# overflow; the 0 - 1 wrap is unsigned arithmetic, not UB).
test_floor_log2_asan: test_floor_log2.c floor_log2.c floor_log2.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_floor_log2.c floor_log2.c

run: all
	./test_floor_log2_o0
	./test_floor_log2_o2
	./test_floor_log2_asan

clean:
	rm -f test_floor_log2_o0 test_floor_log2_o2 test_floor_log2_asan

.PHONY: all run clean
