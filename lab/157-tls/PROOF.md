<!-- PROOF-HEADER
Checks: 21
Mismatches: 0
Checksum: 3526eb3fa18acc0bb10e38a31112e757c5d7f7f253df3a56ee2202439a220f83
Environment: Host (Linux 7.0.0-38-generic, x86-64, gcc 13.3.0, glibc 2.39)
Verdict: PASS
-->
# PROOF.md, lab/157-tls

`tls.c` and `errno.c` prove the central claim of thread-local storage
firsthand: a `__thread` variable has the same name in every thread but
different storage in each. The loader copies the TLS template into fresh
memory per thread, and each access resolves through the thread-pointer
register, so one thread's store never lands in another thread's copy.

21 checks, 0 mismatches: 3 runs x 7 assertions each (the two threads see
different addresses for the same name; both read the initializer 7 before
any store; the worker's store lands at 42 in its own copy; main still reads
7 after the join; errno behaves the same per thread: worker 5, main 0). The
checksum is the SHA-256 of the 21 assertion lines across the three runs;
raw addresses are excluded because ASLR moves them run to run. Clean under
`-std=c11 -O2 -Wall -Wextra -Werror`, `-O0`, and
`-fsanitize=address,undefined`, all with zero reports.

## Build log (genuine)

```
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o tls tls.c -pthread
build: clean, no warnings
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o errno errno.c -pthread
build: clean, no warnings
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -fPIC -shared -o libtls.so tlib.c
build: clean, no warnings
```

## Run output (genuine, one run; addresses vary run to run)

```
main:   &counter = 0x786b5c3b873c, counter = 7
worker: &counter = 0x786b5c3b76bc, counter = 7
worker: after store, counter = 42
main:   after join, counter = 7
ok: T1 addrs-differ
ok: T2 initial-values-7
ok: T3 worker-store-42
ok: T4 main-still-7
main:   &errno = 0x74ba6e0eb6c8
worker: &errno = 0x74ba6e0ea648, errno = 5
main:   after join, errno = 0 (untouched by the worker)
ok: T1 errno-addrs-differ
ok: T2 worker-errno-5
ok: T3 main-errno-0
```

## Disassembly evidence (genuine, this machine)

Initial-exec model, when the variable is defined in the binary being
linked: the offset is fixed at link time, so the access is a single
instruction off the thread pointer.

```
    110d:  mov    %fs:0xfffffffffffffffc,%ecx
```

General-dynamic model, from the shared library `libtls.so`: the library
cannot know the offset, so it loads an index and calls the resolver.

```
    1128:  data16 lea 0x2e90(%rip),%rdi
    1130:  data16 data16 rex.W call 1050 <__tls_get_addr@plt>
    1138:  mov    %rax,%rdx
```

RISC-V, same idea, `tp` as the thread pointer (cross-compiled with the
13.2.0 toolchain, `-O1`; runtime not exercised, codegen only):

```
   0:  lui   a5,0x0
   4:  add   a5,a5,tp
   8:  lw    a4,0(a5)
   c:  addiw a4,a4,1
   e:  sw    a4,0(a5)
```

## ELF evidence (genuine)

`readelf` on the linked binary shows the TLS template the loader copies
per thread: initialized data in `.tdata`, described by the `PT_TLS`
program header.

```
  [21] .tdata            PROGBITS         0000000000003d9c  00002d9c
  TLS            0x0000000000002d9c 0x0000000000003d9c 0x0000000000003d9c
```
