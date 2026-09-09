CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_xorshift_period_o0 test_xorshift_period_o2 test_xorshift_period_asan

test_xorshift_period_o0: test_xorshift_period.c xorshift16.c xorshift16.h
	$(CC) $(CFLAGS) -O0 -o $@ test_xorshift_period.c xorshift16.c

test_xorshift_period_o2: test_xorshift_period.c xorshift16.c xorshift16.h
	$(CC) $(CFLAGS) -O2 -o $@ test_xorshift_period.c xorshift16.c

# AddressSanitizer + UBSan: the exhaustive 65536-entry visited array and
# the shifts are all checked at runtime, so any out-of-bounds index or
# undefined shift would fail loudly instead of silently.
test_xorshift_period_asan: test_xorshift_period.c xorshift16.c xorshift16.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_xorshift_period.c xorshift16.c

run: all
	./test_xorshift_period_o0
	./test_xorshift_period_o2
	./test_xorshift_period_asan

clean:
	rm -f test_xorshift_period_o0 test_xorshift_period_o2 test_xorshift_period_asan

.PHONY: all run clean
