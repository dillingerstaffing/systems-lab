CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_satp_mode_validate

test_satp_mode_validate: test_satp_mode_validate.c satp_mode_validate.h satp_mode_validate.c
	$(CC) $(CFLAGS) -o $@ test_satp_mode_validate.c satp_mode_validate.c

run: test_satp_mode_validate
	./test_satp_mode_validate

sanitize: test_satp_mode_validate.c satp_mode_validate.h satp_mode_validate.c
	$(CC) $(SANFLAGS) -o test_satp_mode_validate_asan test_satp_mode_validate.c satp_mode_validate.c
	./test_satp_mode_validate_asan

opt0: test_satp_mode_validate.c satp_mode_validate.h satp_mode_validate.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_satp_mode_validate_o0 test_satp_mode_validate.c satp_mode_validate.c
	./test_satp_mode_validate_o0

bench: bench_satp_mode_validate.c satp_mode_validate.h satp_mode_validate.c
	$(CC) $(CFLAGS) -o bench_satp_mode_validate bench_satp_mode_validate.c satp_mode_validate.c
	./bench_satp_mode_validate

clean:
	rm -f test_satp_mode_validate test_satp_mode_validate_asan test_satp_mode_validate_o0 bench_satp_mode_validate *.o
