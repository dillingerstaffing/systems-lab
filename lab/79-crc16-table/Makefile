CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_crc16

test_crc16: test_crc16.c crc16.h
	$(CC) $(CFLAGS) -o $@ test_crc16.c

run: test_crc16
	./test_crc16

sanitize: test_crc16.c crc16.h
	$(CC) $(SANFLAGS) -o test_crc16_asan test_crc16.c
	./test_crc16_asan

opt0: test_crc16.c crc16.h
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_crc16_o0 test_crc16.c
	./test_crc16_o0

clean:
	rm -f test_crc16 test_crc16_asan test_crc16_o0

.PHONY: all run sanitize opt0 clean
