CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_dabble

test_dabble: test_dabble.c dabble.h
	$(CC) $(CFLAGS) -o $@ test_dabble.c

run: test_dabble
	./test_dabble

sanitize: test_dabble.c dabble.h
	$(CC) $(SANFLAGS) -o test_dabble_asan test_dabble.c
	./test_dabble_asan

opt0: test_dabble.c dabble.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_dabble_o0 test_dabble.c
	./test_dabble_o0

clean:
	rm -f test_dabble test_dabble_asan test_dabble_o0

.PHONY: all run sanitize opt0 clean
