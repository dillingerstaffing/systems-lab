CC = gcc
STD = -std=c11
WARN = -Wall -Wextra -Werror

.PHONY: all test-o0 test-o2 test-asan test-ubsan clean

all: test-o2

test-o0: fletcher16.h test_fletcher16.c
	$(CC) $(STD) $(WARN) -O0 -g -o test_o0 test_fletcher16.c
	./test_o0

test-o2: fletcher16.h test_fletcher16.c
	$(CC) $(STD) $(WARN) -O2 -o test_o2 test_fletcher16.c
	./test_o2

test-asan: fletcher16.h test_fletcher16.c
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
		-fno-sanitize-recover=all -o test_asan test_fletcher16.c
	./test_asan

test-ubsan: fletcher16.h test_fletcher16.c
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=undefined \
		-fno-sanitize-recover=all -o test_ubsan test_fletcher16.c
	./test_ubsan

clean:
	rm -f test_o0 test_o2 test_asan test_ubsan
