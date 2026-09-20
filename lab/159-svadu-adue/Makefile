# lab/159-svadu-adue: Svadu, who writes the A and D bits.
#
# Bare-metal RISC-V (QEMU 8.2.2 virt, -bios none, Sv39). M-mode sets up
# PMP and page tables, delegates load/store page faults to S-mode via
# medeleg, then drops to S-mode with mret. The test runs in real S-mode;
# its faults are handled by an S-mode handler. menvcfg.ADUE is M-mode-only,
# so S-mode uses ecall to ask M-mode to flip it between phases.
#
# Requires: riscv64-unknown-elf-gcc (here the ubuntu-rv64 13.2.0 toolchain)
# and qemu-system-riscv64 8.2.2. The harness powers the machine off through
# the SiFive test finisher (0x5555 -> exit 0), so a clean exit means 4/4.
TC      := $(HOME)/workspace/toolchains/ubuntu-rv64/usr/bin
CROSS   := $(TC)/riscv64-unknown-elf-
CC      := $(CROSS)gcc
QEMU    := $(HOME)/workspace/qemu/usr/bin/qemu-system-riscv64

CFLAGS  := -Wall -Wextra -Werror -O2 -ffreestanding -nostdlib -nostartfiles \
           -no-pie -fno-pie -fno-pic \
           -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany

all: svadu.elf

svadu.elf: boot.S svadu.c link.ld
	$(CC) $(CFLAGS) -T link.ld -o $@ boot.S svadu.c

# The UART transcript is deterministic; PASS lines are the assertions.
run: svadu.elf
	$(QEMU) -machine virt -bios none -kernel svadu.elf -nographic

run1.txt run2.txt run3.txt: svadu.elf
	$(QEMU) -machine virt -bios none -kernel svadu.elf -nographic > $@

runs: run1.txt run2.txt run3.txt
	cmp run1.txt run2.txt && cmp run2.txt run3.txt && echo "3 runs byte-identical"

clean:
	rm -f svadu.elf *.o run1.txt run2.txt run3.txt
