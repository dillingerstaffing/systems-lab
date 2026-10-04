    .text
    .globl naive
naive:
    lui a0, 0x12345
    addi a0, a0, 0xEEF
    ret
