CC = gcc
STD = -std=c11
WARN = -Wall -Wextra -Werror

.PHONY: all test-o0 test-o2 test-asan test-ubsan clean

all: test-o2

test-o0: strnlen.h test_strnlen.c
	$(CC) $(STD) $(WARN) -O0 -g -o test_o0 test_strnlen.c
	./test_o0

test-o2: strnlen.h test_strnlen.c
	$(CC) $(STD) $(WARN) -O2 -o test_o2 test_strnlen.c
	./test_o2

test-asan: strnlen.h test_strnlen.c
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
		-fno-sanitize-recover=all -o test_asan test_strnlen.c
	./test_asan

test-ubsan: strnlen.h test_strnlen.c
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=undefined \
		-fno-sanitize-recover=all -o test_ubsan test_strnlen.c
	./test_ubsan

clean:
	rm -f test_o0 test_o2 test_asan test_ubsan
