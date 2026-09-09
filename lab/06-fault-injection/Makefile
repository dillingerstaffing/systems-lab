CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -pthread

all: test_fault

test_fault: test_fault.c spsc.h crc32.h crc32.c
	$(CC) $(CFLAGS) -o $@ test_fault.c crc32.c

# fault-off must exit 0 (clean transfer); fault-on must exit 1
# (corruption detected, loudly). Any other exit code fails the run.
run: test_fault
	./test_fault off
	./test_fault on; test $$? -eq 1

clean:
	rm -f test_fault

.PHONY: all run clean
