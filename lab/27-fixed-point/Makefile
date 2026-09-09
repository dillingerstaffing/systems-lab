CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L
LDLIBS = -lm

all: test_fixed_point_o0 test_fixed_point_o2 test_fixed_point_asan

test_fixed_point_o0: test_fixed_point.c fixed_point.c fixed_point.h
	$(CC) $(CFLAGS) -O0 -o $@ test_fixed_point.c fixed_point.c $(LDLIBS)

test_fixed_point_o2: test_fixed_point.c fixed_point.c fixed_point.h
	$(CC) $(CFLAGS) -O2 -o $@ test_fixed_point.c fixed_point.c $(LDLIBS)

# AddressSanitizer + UBSan: proves the int64_t product, the shifts on
# non-negative values, and the uint32_t narrowing contain no undefined
# behavior, including the wraparound paths.
test_fixed_point_asan: test_fixed_point.c fixed_point.c fixed_point.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_fixed_point.c fixed_point.c $(LDLIBS)

run: all
	./test_fixed_point_o0
	./test_fixed_point_o2
	./test_fixed_point_asan

clean:
	rm -f test_fixed_point_o0 test_fixed_point_o2 test_fixed_point_asan

.PHONY: all run clean
