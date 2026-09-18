# lab/157-tls

Thread-local storage: the variable that is not the same variable.

Threads share the whole address space, so how can two threads each have
their own `counter`? The answer is a template and a pointer. The linker
collects every `__thread` variable into a TLS template (the `PT_TLS`
segment, `.tdata` for initialized and `.tbss` for zeroed data), and at
thread creation the loader copies the whole template into fresh memory for
the new thread. Each thread also gets a thread-pointer register (`%fs` on
x86-64, `tp` on RISC-V), and every access to a thread-local variable is
just address math off that register.

This lab proves it firsthand in three programs:

- `tls.c`: a `__thread int counter = 7` at file scope. main and a worker
  print its address and value; the worker stores 42. Four assertions:
  the addresses differ, both read 7 first, the worker's store lands at 42
  in its own copy, main still reads 7 after the join.
- `errno.c`: the most famous thread-local variable you never declared.
  The worker sets `errno = 5` (EIO); main's errno stays 0. Three
  assertions: the addresses differ, the worker's errno is 5, main's is 0.
- `tlib.c`: a shared library defining `__thread int shared_counter`,
  the general-dynamic model. A library cannot know the TLS offset at
  link time, so the access lowers to `lea` an index plus
  `call __tls_get_addr@plt`.

## The two code shapes, from the disassembly

When the variable is defined in the binary being linked, the linker knows
the offset and the access collapses to one instruction (initial-exec):

```
    mov    %fs:0xfffffffffffffffc,%ecx
```

On RISC-V the same idea is three instructions off `tp`:

```
    lui   a5,0x0
    add   a5,a5,tp
    lw    a4,0(a5)
```

## Why not the obvious alternatives

- pthread keys: a dynamic lookup on every access, a tax per read.
- A context pointer threaded through every function: signature churn.
- A thread-id-indexed array: a hash on every access.

The thread pointer is already sitting in a register, so the access costs
one load, and the per-thread template copy means errno, allocator state,
and RNG seeds stop colliding across threads.

## Build and run

`make run` builds and runs all three programs (3 runs, 21 assertions, 0
mismatches expected). The genuine build log, run output, disassembly, and
`readelf` evidence are in `PROOF.md`.
