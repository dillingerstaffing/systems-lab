CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: pget

pget: pget.c
	$(CC) $(CFLAGS) -o $@ pget.c

run: pget
	./pget

sanitize: pget.c
	$(CC) $(SANFLAGS) -o pget_asan pget.c
	./pget_asan

opt0: pget.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o pget_o0 pget.c
	./pget_o0

clean:
	rm -f pget pget_asan pget_o0 *.o
