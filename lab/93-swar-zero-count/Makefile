CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Werror
O2FLAGS = $(CFLAGS) -O2

.PHONY: all test o0 o2 asan disasm clean

all: test

test: o0 o2 asan
	./test_o0 > build_o0.log 2>&1
	./test_o2 > build_o2.log 2>&1
	./test_asan > build_asan.log 2>&1
	cat build_o0.log build_o2.log build_asan.log

o0: main.c zero_count.h
	$(CC) $(CFLAGS) -O0 -o test_o0 main.c

o2: main.c zero_count.h
	$(CC) $(O2FLAGS) -o test_o2 main.c

asan: main.c zero_count.h
	$(CC) $(O2FLAGS) -fsanitize=address,undefined -o test_asan main.c

disasm: main.c zero_count.h
	$(CC) $(O2FLAGS) -S -o disasm.s main.c
	$(CC) $(O2FLAGS) -c -o /dev/null -fverbose-asm main.c 2>/dev/null || true

clean:
	rm -f test_o0 test_o2 test_asan disasm.s build_*.log
