CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_onesc_add_o0 test_onesc_add_o2 test_onesc_add_asan

test_onesc_add_o0: test_onesc_add.c onesc_add.c onesc_add.h
	$(CC) $(CFLAGS) -O0 -o $@ test_onesc_add.c onesc_add.c

test_onesc_add_o2: test_onesc_add.c onesc_add.c onesc_add.h
	$(CC) $(CFLAGS) -O2 -o $@ test_onesc_add.c onesc_add.c

# AddressSanitizer + UBSan: proves the comparison-derived carry
# and the folded addition contain no undefined behavior
# (unsigned wraparound is defined, so the sum itself is safe).
test_onesc_add_asan: test_onesc_add.c onesc_add.c onesc_add.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_onesc_add.c onesc_add.c

run: all
	./test_onesc_add_o0
	./test_onesc_add_o2
	./test_onesc_add_asan

clean:
	rm -f test_onesc_add_o0 test_onesc_add_o2 test_onesc_add_asan

.PHONY: all run clean
