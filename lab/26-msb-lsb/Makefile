CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_msb_lsb_o0 test_msb_lsb_o2 test_msb_lsb_asan

test_msb_lsb_o0: test_msb_lsb.c msb_lsb.c msb_lsb.h
	$(CC) $(CFLAGS) -O0 -o $@ test_msb_lsb.c msb_lsb.c

test_msb_lsb_o2: test_msb_lsb.c msb_lsb.c msb_lsb.h
	$(CC) $(CFLAGS) -O2 -o $@ test_msb_lsb.c msb_lsb.c

# AddressSanitizer + UBSan: proves the multiply/table paths contain no
# undefined behavior (no out-of-range table index, no bad shifts).
test_msb_lsb_asan: test_msb_lsb.c msb_lsb.c msb_lsb.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_msb_lsb.c msb_lsb.c

run: all
	./test_msb_lsb_o0
	./test_msb_lsb_o2
	./test_msb_lsb_asan

clean:
	rm -f test_msb_lsb_o0 test_msb_lsb_o2 test_msb_lsb_asan

.PHONY: all run clean
