CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L

all: test_fma_mod_o0 test_fma_mod_o2 test_fma_mod_asan

test_fma_mod_o0: test_fma_mod.c fma_mod.h
	$(CC) $(CFLAGS) -O0 -DBUILD_NAME='"-O0"' -o $@ test_fma_mod.c

test_fma_mod_o2: test_fma_mod.c fma_mod.h
	$(CC) $(CFLAGS) -O2 -DBUILD_NAME='"-O2"' -o $@ test_fma_mod.c

# AddressSanitizer + UBSan: fma_mod64 uses only unsigned 64-bit shifts,
# adds, multiplies, and comparisons, so no undefined behavior is
# possible; the sanitizers confirm it empirically on the full case set.
# (The test oracle alone uses unsigned __int128, which is also fully
# defined.)
test_fma_mod_asan: test_fma_mod.c fma_mod.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_fma_mod.c

run: all
	./test_fma_mod_o0
	./test_fma_mod_o2
	./test_fma_mod_asan

clean:
	rm -f test_fma_mod_o0 test_fma_mod_o2 test_fma_mod_asan

.PHONY: all run clean
