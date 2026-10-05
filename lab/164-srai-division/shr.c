// shr.c: two three-line functions, compiled with
// riscv64-unknown-elf-gcc 13.2.0 -O2 -march=rv64g -mabi=lp64d.
// Full disassembly in listing.txt.

int shr3(int x){ return x >> 3; }

int div8(int x){ return x / 8; }
