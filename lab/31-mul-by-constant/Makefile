CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_mul_const_o0 test_mul_const_o2 test_mul_const_asan

test_mul_const_o0: test_mul_const.c mul_const.c mul_const.h
	$(CC) $(CFLAGS) -O0 -o $@ test_mul_const.c mul_const.c

test_mul_const_o2: test_mul_const.c mul_const.c mul_const.h
	$(CC) $(CFLAGS) -O2 -o $@ test_mul_const.c mul_const.c

# AddressSanitizer + UBSan: all arithmetic is unsigned (shifts and
# adds), so no undefined behavior is possible; the sanitizers confirm
# it empirically on the full 256M-case sweep.
test_mul_const_asan: test_mul_const.c mul_const.c mul_const.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_mul_const.c mul_const.c

run: all
	./test_mul_const_o0
	./test_mul_const_o2
	./test_mul_const_asan

clean:
	rm -f test_mul_const_o0 test_mul_const_o2 test_mul_const_asan

.PHONY: all run clean
