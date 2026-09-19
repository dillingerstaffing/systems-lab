CC = gcc
# -no-pie: xv6's kfree guard is (pa < end || pa >= PHYSTOP) with
# PHYSTOP = 0x88000000, so the 64 MiB fake arena must sit below 2 GiB.
# `end` is placed at 0x10000000 (--defsym) and the arena is mmaped there
# MAP_FIXED. A PIE build resolves the absolute `end` symbol to a garbage
# address under ASLR and trips the guard; -no-pie fixes it.
CFLAGS = -std=gnu11 -O2 -Wall -Wextra -Werror -no-pie \
         -Wno-builtin-declaration-mismatch \
         -Wl,--defsym,end=0x10000000
SANFLAGS = -std=gnu11 -O1 -g -Wall -Wextra -Werror -no-pie \
           -Wno-builtin-declaration-mismatch \
           -Wl,--defsym,end=0x10000000 -fsanitize=address,undefined

all: kalloc_test

kalloc_test: kalloc_test.c kalloc.c host_spinlock.c host_defs.c
	$(CC) $(CFLAGS) -o $@ kalloc_test.c kalloc.c host_spinlock.c host_defs.c -pthread

# The ok:/FAIL: lines are deterministic; the final acquires=/releases=
# counts line varies only through the fixed totals, so the harness greps
# for the assertion lines before checksumming.
run: all
	@for i in 1 2 3; do ./kalloc_test | grep -E '^(ok|FAIL):' ; done

sanitize: kalloc_test.c kalloc.c host_spinlock.c host_defs.c
	$(CC) $(SANFLAGS) -o kalloc_test_asan kalloc_test.c kalloc.c host_spinlock.c host_defs.c -pthread
	./kalloc_test_asan > /tmp/kalloc_asan.txt 2>&1; grep -E '^(ok|FAIL):' /tmp/kalloc_asan.txt

opt0: kalloc_test.c kalloc.c host_spinlock.c host_defs.c
	$(CC) -std=gnu11 -O0 -Wall -Wextra -Werror -no-pie -Wno-builtin-declaration-mismatch -Wl,--defsym,end=0x10000000 -o kalloc_test_o0 kalloc_test.c kalloc.c host_spinlock.c host_defs.c -pthread
	./kalloc_test_o0 > /tmp/kalloc_o0.txt 2>&1; grep -E '^(ok|FAIL):' /tmp/kalloc_o0.txt

clean:
	rm -f kalloc_test kalloc_test_asan kalloc_test_o0 *.o
