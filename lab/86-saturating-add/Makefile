CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_satadd

test_satadd: test_satadd.c sat_add32.h
	$(CC) $(CFLAGS) -o $@ test_satadd.c

run: test_satadd
	./test_satadd

sanitize: test_satadd.c sat_add32.h
	$(CC) $(SANFLAGS) -o test_satadd_asan test_satadd.c
	./test_satadd_asan

opt0: test_satadd.c sat_add32.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_satadd_o0 test_satadd.c
	./test_satadd_o0

clean:
	rm -f test_satadd test_satadd_asan test_satadd_o0

.PHONY: all run sanitize opt0 clean
