CC      = gcc
STD     = -std=c11
WARN    = -Wall -Wextra -Werror
BIN     = test_O0 test_O2 test_asan bench_O2

all: $(BIN)

test_O0: test_bitperm.c bitperm.h
	$(CC) $(STD) $(WARN) -O0 -o $@ test_bitperm.c

test_O2: test_bitperm.c bitperm.h
	$(CC) $(STD) $(WARN) -O2 -o $@ test_bitperm.c

test_asan: test_bitperm.c bitperm.h
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
	    -fno-sanitize-recover=all -o $@ test_bitperm.c

bench_O2: bench_bitperm.c bitperm.h
	$(CC) $(STD) $(WARN) -O2 -o $@ bench_bitperm.c

check: test_O0 test_O2 test_asan
	./test_O0
	./test_O2
	./test_asan

clean:
	rm -f $(BIN)
