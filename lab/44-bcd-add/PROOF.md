# PROOF.md: lab/44-bcd-add

Environment: gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), x86_64.

## Build

```
$ make clean && make
rm -f test_bcd test_bcd_asan test_bcd_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_bcd test_bcd.c
build_exit=0
```

Zero warnings under -Wall -Wextra -Werror.

## Runs

### -O2 (`./test_bcd`)

```
add_cases=10000
sub_cases=10000
mismatches=0
add_carry_out_ones=4950
sub_borrow_out_ones=4950
fnv1a=ff64fdf0ec2142e5
throughput_acc=4b11f2805b31f280
throughput_time_s=0.132
throughput_ns_per_op=3.296
run_exit=0
```

### -O0 (`make opt0`)

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_bcd_o0 test_bcd.c
./test_bcd_o0
add_cases=10000
sub_cases=10000
mismatches=0
add_carry_out_ones=4950
sub_borrow_out_ones=4950
fnv1a=ff64fdf0ec2142e5
throughput_acc=4b11f2805b31f280
throughput_time_s=0.464
throughput_ns_per_op=11.609
run_exit=0
opt0_exit=0
```

### ASan+UBSan (`make sanitize`)

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_bcd_asan test_bcd.c
./test_bcd_asan
add_cases=10000
sub_cases=10000
mismatches=0
add_carry_out_ones=4950
sub_borrow_out_ones=4950
fnv1a=ff64fdf0ec2142e5
throughput_acc=4b11f2805b31f280
throughput_time_s=0.283
throughput_ns_per_op=7.075
run_exit=0
sanitize_exit=0
```

The sanitizer run printed no reports: zero ASan/UBSan findings.

## What was measured

- 20,000 differential cases: exhaustive over the entire defined domain,
  all 100x100 valid packed BCD pairs for `bcd_add` and all 100x100 for
  `bcd_sub`, each checked against the independent nibble-unpack
  reference (which shares no correction logic with the implementation).
  0 mismatches.
- Out-of-range behavior verified inside the same exhaustive sweep:
  carry-out fired on exactly 4,950 of 10,000 adds and borrow-out on
  exactly 4,950 of 10,000 subs, matching the counts computed by hand
  (add: 10,000 - 100*101/2 = 4,950 pairs with a+b >= 100; sub:
  (10,000 - 100)/2 = 4,950 pairs with a < b). Spot examples from the
  sweep: 0x99+0x01 -> 0x00 carry 1, 0x99+0x99 -> 0x98 carry 1,
  0x00-0x01 -> 0x99 borrow 1.
- FNV-1a checksum over all 20,000 results plus carry/borrow flags:
  `ff64fdf0ec2142e5`, identical across -O0, -O2, and ASan+UBSan builds.
- Throughput at -O2: 3.296 ns per op in the measured run (repeat runs
  ranged 3.3 to 4.6 ns per op on this machine), measured over
  20,000,000 iterations each doing one `bcd_add` and one `bcd_sub`
  (40,000,000 ops total) cycling through a 10,000-entry table of all
  valid pairs. Exact loop body:
  `acc += bcd_add(tbl[k], tbl2[k], &flag); acc += (uint64_t)flag << 56;`
  `acc += (uint64_t)bcd_sub(tbl[k], tbl2[k], &flag) << 32;`
  `acc += (uint64_t)flag << 48;` with k = i % 10000, and acc printed
  (`throughput_acc=4b11f2805b31f280`) so the calls are not optimized away.

## What was not verified

Bytes with a nibble above 9 are outside the module's contract
(declared in bcd.h); the 65,536-pair full-byte space was deliberately
not swept because the reference is undefined there. Verification covers
exactly the defined valid domain: all 10,000 valid add pairs and all
10,000 valid sub pairs.
