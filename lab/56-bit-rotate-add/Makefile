CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_rot_add_o0 test_rot_add_o2 test_rot_add_asan

test_rot_add_o0: test_rot_add.c rot_add.c rot_add.h
	$(CC) $(CFLAGS) -O0 -o $@ test_rot_add.c rot_add.c

# The -O2 build also runs the timed throughput loop.
test_rot_add_o2: test_rot_add.c rot_add.c rot_add.h
	$(CC) $(CFLAGS) -DPERF_TEST -O2 -o $@ test_rot_add.c rot_add.c

# AddressSanitizer + UBSan: every shift in rotl64 and the per-bit
# reference runs under the sanitizers, so undefined behavior (a shift
# by 64, a signed overflow, an out-of-range shift count) would fail
# loudly instead of silently.
test_rot_add_asan: test_rot_add.c rot_add.c rot_add.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_rot_add.c rot_add.c

run: all
	./test_rot_add_o0
	./test_rot_add_o2
	./test_rot_add_asan

clean:
	rm -f test_rot_add_o0 test_rot_add_o2 test_rot_add_asan

.PHONY: all run clean
