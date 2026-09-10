<!-- PROOF-HEADER
Checks: 1004160
Mismatches: 0
Throughput: memcpy10 1398.8 MiB/s, libc memcpy 21635.4 MiB/s (64 MiB x 20)
Environment: Host
-->

# PROOF: lab/10-from-scratch-memcpy

Date: 2026-09-08. Host: x86-64, gcc 13.3.0, `-Wall -Wextra -Werror -O2`.
Implementation: `memcpy10.c` (head/tail byte loops, word body with
byte-assembled source words). Oracle: libc `memcpy`.

## Build log (genuine)

```
$ make
cc -Wall -Wextra -Werror -O2 -std=c11 -o test_memcpy memcpy10.c test_memcpy.c
cc -Wall -Wextra -Werror -O2 -std=c11 -fsanitize=address,undefined -fno-omit-frame-pointer -o test_memcpy_asan memcpy10.c test_memcpy.c
```

(The build initially failed on `clock_gettime` under `-std=c11`
because `_POSIX_C_SOURCE` was missing; adding the feature-test macro
fixed it. No other warnings or errors.)

## Run output: plain build (genuine)

```
$ ./test_memcpy
differential: 1004160 cases, 0 mismatches
throughput (64 MiB x 20, checksum 400): memcpy10 1398.8 MiB/s, libc memcpy 21635.4 MiB/s
```

Two additional runs of the same binary:

```
differential: 1004160 cases, 0 mismatches
throughput (64 MiB x 20, checksum 400): memcpy10 1338.8 MiB/s, libc memcpy 20266.6 MiB/s

differential: 1004160 cases, 0 mismatches
throughput (64 MiB x 20, checksum 400): memcpy10 1375.0 MiB/s, libc memcpy 20085.1 MiB/s
```

## Run output: ASan + UBSan build (genuine)

```
$ ./test_memcpy_asan
differential: 1004160 cases, 0 mismatches
throughput (64 MiB x 20, checksum 400): memcpy10 461.7 MiB/s, libc memcpy 13816.4 MiB/s
```

Exit code 0; no sanitizer reports. (Throughput is lower here only
because the binary is instrumented.)

## What this establishes

- 1,004,160 differential cases (exhaustive sizes 0..64 x dmis 0..7 x
  smis 0..7, plus 1,000,000 random cases) byte-identical to libc
  `memcpy`: zero mismatches.
- Clean under AddressSanitizer and UndefinedBehaviorSanitizer over the
  same case set.
- Throughput measured on 64 MiB copies: memcpy10 ~1.34-1.40 GiB/s,
  libc memcpy ~20.1-21.6 GiB/s (plain build), same host.
- Return value == dest in every case.

## What is NOT claimed

- Overlapping regions are untested and unsupported, exactly like the
  standard `memcpy` contract.
- Numbers are host-specific (this x86-64 VM) and are reported as
  measured, not as guarantees.
