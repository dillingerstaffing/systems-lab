CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_ctz

test_ctz: test_ctz.c ctz.h ctz.c
	$(CC) $(CFLAGS) -o $@ test_ctz.c ctz.c

run: test_ctz
	./test_ctz

sanitize: test_ctz.c ctz.h ctz.c
	$(CC) $(SANFLAGS) -o test_ctz_asan test_ctz.c ctz.c
	./test_ctz_asan

opt0: test_ctz.c ctz.h ctz.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_ctz_o0 test_ctz.c ctz.c
	./test_ctz_o0

disasm: test_ctz.c ctz.h ctz.c
	$(CC) $(CFLAGS) -c -o ctz.o ctz.c
	$(CC) $(CFLAGS) -c -o test_ctz.o test_ctz.c
	objdump -d ctz.o | grep -Ei 'tzcnt|bsf|popcnt|l zcnt' || true
	objdump -d test_ctz.o | grep -Ei 'tzcnt|bsf|popcnt|l zcnt' || true

clean:
	rm -f test_ctz test_ctz_asan test_ctz_o0 *.o

.PHONY: all run sanitize opt0 disasm clean
