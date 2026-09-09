CC = gcc
STD = -std=c11
WARN = -Wall -Wextra -Werror

.PHONY: all test-o0 test-o2 test-asan test-ubsan clean

all: test-o2

test-o0: divround.h test_divround.c
	$(CC) $(STD) $(WARN) -O0 -g -o test_o0 test_divround.c
	./test_o0

test-o2: divround.h test_divround.c
	$(CC) $(STD) $(WARN) -O2 -o test_o2 test_divround.c
	./test_o2

test-asan: divround.h test_divround.c
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=address,undefined \
		-fno-sanitize-recover=all -o test_asan test_divround.c
	./test_asan

test-ubsan: divround.h test_divround.c
	$(CC) $(STD) $(WARN) -O1 -g -fsanitize=undefined \
		-fno-sanitize-recover=all -o test_ubsan test_divround.c
	./test_ubsan

clean:
	rm -f test_o0 test_o2 test_asan test_ubsan
