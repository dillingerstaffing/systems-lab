CC = gcc
STD = -std=c11
WARN = -Wall -Wextra -Werror

.PHONY: all test-o0 test-o2 test-asan test-ubsan bench-o2 clean

all: test-o2

test-o0: adler32.c test_adler32.c adler32.h
	$(CC) $(STD) $(WARN) -O0 -g -o test_o0 adler32.c test_adler32.c
	./test_o0

test-o2: adler32.c test_adler32.c adler32.h
	$(CC) $(STD) $(WARN) -O2 -o test_o2 adler32.c test_adler32.c
	./test_o2

test-asan: adler32.c test_adler32.c adler32.h
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
		-fno-sanitize-recover=all -o test_asan adler32.c test_adler32.c
	./test_asan

test-ubsan: adler32.c test_adler32.c adler32.h
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=undefined \
		-fno-sanitize-recover=all -o test_ubsan adler32.c test_adler32.c
	./test_ubsan

clean:
	rm -f test_o0 test_o2 test_asan test_ubsan
