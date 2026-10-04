    .text
    .globl load_consts
load_consts:
    li a0, 0x7FF
    li a1, 0x800
    li a2, 0x12345678
    li a3, 0x12345EEF
    li a4, -1
    ret
