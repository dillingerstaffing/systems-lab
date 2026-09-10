CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_wrapneg_o0 test_wrapneg_o2 test_wrapneg_asan test_wrapneg_ubsan

test_wrapneg_o0: test_wrapneg.c wrapneg.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_wrapneg.c

test_wrapneg_o2: test_wrapneg.c wrapneg.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_wrapneg.c

# AddressSanitizer: wrapneg uses only unsigned NOT, add, and
# shifts-free 64-bit arithmetic; the splitmix64 sequence and the
# FNV-1a multiply run on unsigned 64-bit values only.  No
# undefined behavior is possible; ASan confirms memory safety
# empirically on the full case set.
test_wrapneg_asan: test_wrapneg.c wrapneg.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan"' -o $@ test_wrapneg.c

# UndefinedBehaviorSanitizer: confirms empirically that the
# unsigned (~x + 1) construction, the unary-minus reference
# (skipped for INT64_MIN), the invariant additions, and the
# reference/edge-case arithmetic contain no undefined behavior.
test_wrapneg_ubsan: test_wrapneg.c wrapneg.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"ubsan"' -o $@ test_wrapneg.c

run: all
	./test_wrapneg_o0
	./test_wrapneg_o2
	./test_wrapneg_asan
	./test_wrapneg_ubsan

clean:
	rm -f test_wrapneg_o0 test_wrapneg_o2 test_wrapneg_asan test_wrapneg_ubsan

.PHONY: all run clean
