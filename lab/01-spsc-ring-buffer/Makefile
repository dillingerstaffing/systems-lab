CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -pthread

all: test_spsc

test_spsc: test_spsc.c spsc.h
	$(CC) $(CFLAGS) -o $@ test_spsc.c

# ThreadSanitizer run: smaller workload (1M items), proves race-freedom.
tsan: test_spsc.c spsc.h
	$(CC) -std=c11 -O1 -g -fsanitize=thread -Wall -Wextra -pthread \
		-DSTRESS_N=1000000 -o test_spsc_tsan test_spsc.c

run: test_spsc
	./test_spsc

run-tsan: tsan
	./test_spsc_tsan

clean:
	rm -f test_spsc test_spsc_tsan

.PHONY: all run tsan run-tsan clean
