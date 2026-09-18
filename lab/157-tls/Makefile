CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SANFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined

all: tls errno libtls.so

tls: tls.c
	$(CC) $(CFLAGS) -o $@ tls.c -pthread

errno: errno.c
	$(CC) $(CFLAGS) -o $@ errno.c -pthread

libtls.so: tlib.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ tlib.c

# The ok:/FAIL: lines are deterministic; the raw address lines vary run to
# run, so the harness greps for the assertions before checksumming.
run: all
	@for i in 1 2 3; do ./tls | grep -E '^(ok|FAIL):' ; ./errno | grep -E '^(ok|FAIL):' ; done

sanitize: tls.c errno.c
	$(CC) $(SANFLAGS) -o tls_asan tls.c -pthread
	./tls_asan | grep -E '^(ok|FAIL):'
	$(CC) $(SANFLAGS) -o errno_asan errno.c -pthread
	./errno_asan | grep -E '^(ok|FAIL):'

opt0: tls.c errno.c
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o tls_o0 tls.c -pthread
	./tls_o0 | grep -E '^(ok|FAIL):'
	$(CC) -std=c11 -O0 -Wall -Wextra -Werror -o errno_o0 errno.c -pthread
	./errno_o0 | grep -E '^(ok|FAIL):'

clean:
	rm -f tls errno tls_asan errno_asan tls_o0 errno_o0 libtls.so *.o
