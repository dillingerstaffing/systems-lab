# lab/161-ztso-fence-delete

Ztso is the RISC-V extension for Total Store Ordering (chapter 26 of the
unprivileged ISA manual, ratified January 2023). It adds no instructions,
no encodings, and no CSRs. It is a promise about memory order: every
store behaves as if it carries a release annotation, every load as if it
carries an acquire annotation, and every atomic read-modify-write orders
both ways.

Under the default RVWMO model, two stores can become visible to other
harts out of order. The message-passing litmus makes it concrete: hart 0
stores x=1 then y=1, hart 1 loads y then x. RVWMO allows the outcome
r1=1, r2=0 (y arrived before x). Under RVTSO that outcome is forbidden:
stores are observed in program order.

The promise has a price the compiler pays for you. A fence exists to
forbid reorderings the program must not see, and the spec renders
redundant "any non-I/O fences that do not have both PW and SR set".
`tso.c` holds four atomics compiled for `rv64gc` and for
`rv64gc_ztso`:

- seq_cst store: `fence rw,w; sw; fence rw,rw` becomes `sw; fence rw,rw`
- seq_cst load: `fence rw,rw; lw; fence r,rw` becomes `fence rw,rw; lw`
- release store: `fence rw,w; sw` becomes `sw`
- acquire load: `lw; fence r,rw` becomes `lw`

The compiler deletes exactly the fences the promise makes redundant:
`fence rw,w` lacks SR, `fence r,rw` lacks PW. Each sequentially
consistent op keeps one `fence rw,rw`, positioned against the single
reorder TSO still permits (a later load passing an earlier store, via
the spec's Load Value Axiom: a hart may forward its own store to a
later load before the store is visible elsewhere).

An extension with no instructions still signs the binary. `readelf -A`
shows the ztso object's `Tag_RISCV_arch` gains `_ztso1p0`; the base
object's does not. The spec asks for exactly this flag, so that
platforms without Ztso can refuse to run binaries that assume RVTSO.
You cannot disassemble Ztso. You read the arch string.

Why it exists: porting code written for TSO architectures (x86, some
SPARC), and letting hardware that is naturally TSO advertise the fact
to software. The alternative is paying for fences the machine never
needed.
