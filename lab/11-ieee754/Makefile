CC = cc
CFLAGS = -Wall -Wextra -Werror -O2 -std=c11
LDLIBS = -lm

all: test_f32 test_f32_asan

test_f32: f32.c test_f32.c f32.h
	$(CC) $(CFLAGS) -o $@ f32.c test_f32.c $(LDLIBS)

test_f32_asan: f32.c test_f32.c f32.h
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -o $@ f32.c test_f32.c $(LDLIBS)

check: test_f32
	./test_f32

check-asan: test_f32_asan
	./test_f32_asan

clean:
	rm -f test_f32 test_f32_asan
