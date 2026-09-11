CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: test_satp_fields

test_satp_fields: test_satp_fields.c satp_fields.h satp_fields.c
	$(CC) $(CFLAGS) -o $@ test_satp_fields.c satp_fields.c

run: test_satp_fields
	./test_satp_fields

sanitize: test_satp_fields.c satp_fields.h satp_fields.c
	$(CC) $(SANFLAGS) -o test_satp_fields_asan test_satp_fields.c satp_fields.c
	./test_satp_fields_asan

opt0: test_satp_fields.c satp_fields.h satp_fields.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o test_satp_fields_o0 test_satp_fields.c satp_fields.c
	./test_satp_fields_o0

clean:
	rm -f test_satp_fields test_satp_fields_asan test_satp_fields_o0 *.o
