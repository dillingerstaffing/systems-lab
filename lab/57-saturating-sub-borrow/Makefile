CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_sat_sub_o0 test_sat_sub_o2 test_sat_sub_asan

test_sat_sub_o0: test_sat_sub.c sat_sub.c sat_sub.h
	$(CC) $(CFLAGS) -O0 -o $@ test_sat_sub.c sat_sub.c

test_sat_sub_o2: test_sat_sub.c sat_sub.c sat_sub.h
	$(CC) $(CFLAGS) -O2 -o $@ test_sat_sub.c sat_sub.c

# AddressSanitizer + UBSan: the implementation and the oracle both run
# under the sanitizers, so any undefined behavior would fail loudly
# instead of silently.
test_sat_sub_asan: test_sat_sub.c sat_sub.c sat_sub.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_sat_sub.c sat_sub.c

# The 2^32 exhaustive sweep runs on the -O2 build only; the -O0 and
# sanitizer builds run the directed edges plus the 10M random pairs,
# whose FNV-1a checksum must match across all three builds.
run: all
	./test_sat_sub_o2 full
	./test_sat_sub_o0
	./test_sat_sub_asan

bench: all
	./test_sat_sub_o2 bench

clean:
	rm -f test_sat_sub_o0 test_sat_sub_o2 test_sat_sub_asan sat_sub.o

.PHONY: all run bench clean
