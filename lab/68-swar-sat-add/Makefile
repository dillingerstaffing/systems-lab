CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_swar_sat_add

test_swar_sat_add: test_swar_sat_add.c swar_sat_add.h
	$(CC) $(CFLAGS) -o $@ test_swar_sat_add.c

run: test_swar_sat_add
	./test_swar_sat_add full

sanitize: test_swar_sat_add.c swar_sat_add.h
	$(CC) $(SANFLAGS) -o test_swar_sat_add_asan test_swar_sat_add.c
	./test_swar_sat_add_asan quick

opt0: test_swar_sat_add.c swar_sat_add.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_swar_sat_add_o0 test_swar_sat_add.c
	./test_swar_sat_add_o0 quick

clean:
	rm -f test_swar_sat_add test_swar_sat_add_asan test_swar_sat_add_o0

.PHONY: all run sanitize opt0 clean
