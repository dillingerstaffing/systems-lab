CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror

all: test_parity_o0 test_parity_o2 test_parity_asan

test_parity_o0: test_parity.c parity.c parity.h
	$(CC) $(CFLAGS) -O0 -o $@ test_parity.c parity.c

test_parity_o2: test_parity.c parity.c parity.h
	$(CC) $(CFLAGS) -O2 -o $@ test_parity.c parity.c

# AddressSanitizer + UBSan: confirms the folds contain no undefined
# behavior (unsigned shifts only, so none is expected; the checkers
# prove it).
test_parity_asan: test_parity.c parity.c parity.h
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -o $@ test_parity.c parity.c

run: all
	./test_parity_o0
	./test_parity_o2
	./test_parity_asan

clean:
	rm -f test_parity_o0 test_parity_o2 test_parity_asan

.PHONY: all run clean
