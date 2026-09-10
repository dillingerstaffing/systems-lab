CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_mullo_o0 test_mullo_o2 test_mullo_asan

test_mullo_o0: test_mullo.c mullo_split.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_mullo.c

test_mullo_o2: test_mullo.c mullo_split.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_mullo.c

# AddressSanitizer + UBSan: every operation in mullo64 is unsigned
# (shifts, adds, multiplies, comparisons), so no undefined behavior is
# possible; the sanitizers confirm it empirically on the full case set.
test_mullo_asan: test_mullo.c mullo_split.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_mullo.c

run: all
	./test_mullo_o0
	./test_mullo_o2
	./test_mullo_asan

clean:
	rm -f test_mullo_o0 test_mullo_o2 test_mullo_asan

.PHONY: all run clean
