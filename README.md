# systems-lab

Small systems-programming modules, each one real, each one verified.
Every module under `lab/` ships with its source, a test suite, and a
`PROOF.md` containing the genuine build log and run output. Nothing here is
a mock or a placeholder: if it does not compile and pass, it is not
committed.

## Modules

- `lab/01-spsc-ring-buffer`: wait-free single-producer/single-consumer ring
  buffer in C11. 10M-item two-thread stress test with order checking and
  checksum, measured at 190.4 Mops/s, plus empty/full edge tests and
  head/tail counter wrap-around tests.

## Building

Each module is self-contained:

```
cd lab/01-spsc-ring-buffer
make run
```
