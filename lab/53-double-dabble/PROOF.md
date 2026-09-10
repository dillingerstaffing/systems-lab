# PROOF.md: lab/53-double-dabble

Environment: gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64.

## Build

```
$ make clean && make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_dabble test_dabble.c
build_exit=0
```

Zero warnings under -Wall -Wextra -Werror.

## Runs

### -O2 (`./test_dabble`)

```
exhaustive_inputs=256
total_checks=1280
mismatches=0
throughput_values=100000000
throughput_time_s=5.447
throughput_ns_per_value=54.468
throughput_acc=000001b9
run_exit=0
```

### -O0 (`make opt0`: builds test_dabble_o0, runs it)

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_dabble_o0 test_dabble.c
./test_dabble_o0
exhaustive_inputs=256
total_checks=1280
mismatches=0
throughput_values=100000000
throughput_time_s=6.795
throughput_ns_per_value=67.953
throughput_acc=000001b9
```

### ASan+UBSan (`make sanitize`: builds test_dabble_asan, runs it)

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_dabble_asan test_dabble.c
./test_dabble_asan
exhaustive_inputs=256
total_checks=1280
mismatches=0
throughput_values=100000000
throughput_time_s=6.528
throughput_ns_per_value=65.280
throughput_acc=000001b9
```

## What the numbers mean

- total_checks=1280: every one of the 256 inputs contributes 5 checks:
  three nibble-layout checks (hundreds, tens, ones each in 0..9), one
  identity check (hundreds*100 + tens*10 + ones == input), and one
  differential check against snprintf "%03u" used as the oracle.
- mismatches=0 on all three builds: no check failed anywhere.
- throughput_acc=000001b9 identical on -O0, -O2, and ASan+UBSan:
  the same conversions were computed in all three builds. (Each of
  the 256 values occurs an odd number of times in the 100M stream, so
  the XOR fold equals the XOR of all 256 packed results.)
- throughput_ns_per_value=54.468 at -O2: measured with CLOCK_MONOTONIC
  over 100,000,000 conversions. The timed loop includes one xorshift32
  step per value, so the input bytes are runtime data the compiler
  cannot constant-fold; the printed accumulator prevents the loop from
  being discarded. -O0 and sanitizer timings are reported for
  completeness and are not claimed as performance figures.

The sanitizer builds reported no address or undefined-behavior
diagnostics: the run produced no sanitizer output and exited 0.
