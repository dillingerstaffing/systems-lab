CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror -pthread

all: test_ticket_ring

test_ticket_ring: test_ticket_ring.c ticket.h ring.h
	$(CC) $(CFLAGS) -o $@ test_ticket_ring.c

run: test_ticket_ring
	./test_ticket_ring

asan: test_ticket_ring.c ticket.h ring.h
	$(CC) -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
		-Wall -Wextra -Werror -pthread -o test_ticket_ring_asan test_ticket_ring.c

run-asan: asan
	./test_ticket_ring_asan

clean:
	rm -f test_ticket_ring test_ticket_ring_asan

.PHONY: all run asan run-asan clean
