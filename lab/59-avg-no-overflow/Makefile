CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_avg_o0 test_avg_o2 test_avg_asan

test_avg_o0: test_avg.c avg.h
	$(CC) $(CFLAGS) -O0 -o $@ test_avg.c

test_avg_o2: test_avg.c avg.h
	$(CC) $(CFLAGS) -O2 -o $@ test_avg.c

# AddressSanitizer + UBSan: every shift and addition in avg_u64 and the
# oracle runs under the sanitizers, so undefined behavior would fail
# loudly instead of silently.
test_avg_asan: test_avg.c avg.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_avg.c

# The 2^32 exhaustive sweep runs on the -O2 build; the -O0 and sanitizer
# builds run the directed edges plus the 10M random pairs.
run: all
	./test_avg_o2 full
	./test_avg_o0
	./test_avg_asan

clean:
	rm -f test_avg_o0 test_avg_o2 test_avg_asan

.PHONY: all run clean
