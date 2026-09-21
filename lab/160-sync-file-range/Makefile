CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror

all: sfr

sfr: syncfilerange.c
	$(CC) $(CFLAGS) -o $@ syncfilerange.c

# Microsecond timings vary run to run on a shared VM, so the harness
# normalizes every digit run before checksumming. The stable assertions
# are the return codes, the errnos, and the range-combo-vs-fsync ordering.
run: all
	@for i in 1 2 3; do ./sfr > run$$i.txt; done
	@cat run1.txt run2.txt run3.txt | sed 's/[0-9][0-9]*/#/g' | sha256sum

clean:
	rm -f sfr run1.txt run2.txt run3.txt data.bin *.o
