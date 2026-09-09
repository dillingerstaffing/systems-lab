# PROOF.md: lab/06-fault-injection

Date: 2026-09-08. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build log and run output.

## Build log

```
$ make clean && make
rm -f test_fault
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -o test_fault test_fault.c crc32.c
build exit=0
```

Zero warnings under `-Wall -Wextra -Wpedantic -Werror`.

## Run output

```
$ ./test_fault off
[fault] mode=off frames=200000 capacity=4096 words/frame=17
[fault] transferred 200000 frames (3400000 words) in 0.377 s
[fault] throughput: 18.0 Mwords/s (push+pop counted)
[fault] frames verified clean: 200000 / 200000
[fault] errors: 0
ALL FRAMES CLEAN
off exit=0
```

```
$ ./test_fault on
[fault] mode=on frames=200000 capacity=4096 words/frame=17
[fault] transferred 200000 frames (3400000 words) in 8.703 s
[fault] throughput: 0.8 Mwords/s (push+pop counted)
[fault] injected: 6250 (expected 6250), detected: 6250, clean frames ok: 193750, errors: 0
[fault] detected #0: frame 7, slot 126, word 7, bit 0 flipped, popped CRC 0xa447c3cc vs recomputed 0x31371759
[fault] detected #1: frame 39, slot 669, word 6, bit 0 flipped, popped CRC 0x2208d62f vs recomputed 0x8d4c4468
[fault] detected #2: frame 71, slot 1212, word 5, bit 0 flipped, popped CRC 0x5bd43a5c vs recomputed 0x71fc023e
FAULT DETECTED: every injected corruption caught by the integrity check
on exit=1
```

`make run` (fault-off must exit 0, fault-on must exit 1) passes.

## What was verified

- fault-off: 200,000 frames, 3,400,000 words through the SPSC ring
  buffer, every frame's CRC32 verified against its popped CRC word and
  every payload word verified against its deterministic value: 0
  errors, exit 0.
- fault-on: a third thread flipped one bit in a payload word of every
  32nd frame starting at frame 7. 6,250 corruptions injected, 6,250
  detected, 0 missed, 0 false positives; the remaining 193,750 frames
  verified clean. Each detection was attributed to the exact slot and
  bit the injector flipped (frame, word index, and xor difference all
  matched the injector's log), and the program exited 1, the designed
  loud signal that corruption was detected.
- Why the check cannot miss a single-bit flip: CRC32 detects every
  single-bit error in a message shorter than 2^32 - 1 bits (the
  generator polynomial does not divide x^k for small k, so a one-bit
  error pattern always leaves a nonzero remainder). Each payload here
  is 512 bits.
- Why every flip landed in an unread slot: the consumer parks on a
  gate before reading an injected frame (head is then exactly the
  frame's first slot), the injector waits until the frame is fully
  written (acquire-load of tail) and the consumer is parked, flips the
  bit, then releases the consumer. The gate's release/acquire atomics
  order the flip before the consumer's read, so the injection is not a
  data race.

## Attempted but not claimable

Nothing outstanding: both modes ran to completion with the exact
expected counts and exit codes on this machine. Timing varies between
runs (the fault-on rendezvous spins are scheduling-sensitive); the
throughput figures above are the measured values from these runs, not
guarantees.
