CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_rotr_o0 test_rotr_o2 test_rotr_asan

test_rotr_o0: test_rotr.c rotr.c rotr.h
	$(CC) $(CFLAGS) -O0 -o $@ test_rotr.c rotr.c

# -DMEASURE enables the throughput measurement in the test binary.
test_rotr_o2: test_rotr.c rotr.c rotr.h
	$(CC) $(CFLAGS) -DMEASURE -O2 -o $@ test_rotr.c rotr.c

# AddressSanitizer + UBSan: the exhaustive loop over all 16-bit inputs
# times 32 rotation amounts also exercises the sanitizers on every case.
test_rotr_asan: test_rotr.c rotr.c rotr.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_rotr.c rotr.c

run: all
	./test_rotr_o0
	./test_rotr_o2
	./test_rotr_asan

clean:
	rm -f test_rotr_o0 test_rotr_o2 test_rotr_asan

.PHONY: all run clean
