CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror

all: fuzz

fuzz: fuzz.c spsc.h bump.c bump.h
	$(CC) $(CFLAGS) -o $@ fuzz.c bump.c

# AddressSanitizer + UBSan: every pointer the allocator hands out is
# checked for out-of-bounds and misaligned arithmetic here as well.
asan: fuzz.c spsc.h bump.c bump.h
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -Wall -Wextra -Wpedantic \
		-o fuzz_asan fuzz.c bump.c

run: fuzz
	./fuzz

run-asan: asan
	./fuzz_asan

# Reproducibility check: two runs with the fixed seed must print the
# same digest line (timing lines legitimately differ).
determinism: fuzz
	./fuzz | grep '^digest:' > /tmp/fuzz_digest_1.txt
	./fuzz | grep '^digest:' > /tmp/fuzz_digest_2.txt
	diff /tmp/fuzz_digest_1.txt /tmp/fuzz_digest_2.txt \
		&& echo "deterministic: digests match"

clean:
	rm -f fuzz fuzz_asan

.PHONY: all asan run run-asan determinism clean
