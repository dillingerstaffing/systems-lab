CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_zero_run_o0 test_zero_run_o2 test_zero_run_asan

test_zero_run_o0: test_zero_run.c zero_run.c zero_run.h
	$(CC) $(CFLAGS) -O0 -o $@ test_zero_run.c zero_run.c

test_zero_run_o2: test_zero_run.c zero_run.c zero_run.h
	$(CC) $(CFLAGS) -O2 -o $@ test_zero_run.c zero_run.c

# AddressSanitizer + UBSan: every shift in the cascade and the oracle
# runs under the sanitizers, so undefined behavior would fail loudly
# instead of silently.
test_zero_run_asan: test_zero_run.c zero_run.c zero_run.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_zero_run.c zero_run.c

run: all
	./test_zero_run_o0
	./test_zero_run_o2
	./test_zero_run_asan

clean:
	rm -f test_zero_run_o0 test_zero_run_o2 test_zero_run_asan

.PHONY: all run clean
