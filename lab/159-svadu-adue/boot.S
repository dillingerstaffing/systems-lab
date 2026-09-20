    .text
    .globl _start
    .align 2
_start:
    la sp, stacks_top
    call m_main
hang:
    wfi
    j hang

    /* M-mode trap entry: save, C handler, restore, mret.
       m_trap_c(mcause, mepc, mtval) returns the mepc to resume at. */
    .globl trap_entry_m
    .align 2
trap_entry_m:
    addi sp, sp, -256
    sd ra, 0(sp);  sd t0, 8(sp);   sd t1, 16(sp);  sd t2, 24(sp)
    sd t3, 32(sp); sd t4, 40(sp);  sd t5, 48(sp);  sd t6, 56(sp)
    sd a0, 64(sp); sd a1, 72(sp);  sd a2, 80(sp);  sd a3, 88(sp)
    sd a4, 96(sp); sd a5, 104(sp); sd a6, 112(sp); sd a7, 120(sp)
    sd s0, 128(sp); sd s1, 136(sp); sd s2, 144(sp); sd s3, 152(sp)
    sd s4, 160(sp); sd s5, 168(sp); sd s6, 176(sp); sd s7, 184(sp)
    sd s8, 192(sp); sd s9, 200(sp); sd s10, 208(sp); sd s11, 216(sp)
    sd gp, 224(sp); sd tp, 232(sp)
    csrr a0, mcause
    csrr a1, mepc
    csrr a2, mtval
    call m_trap_c
    csrw mepc, a0
    ld ra, 0(sp);  ld t0, 8(sp);   ld t1, 16(sp);  ld t2, 24(sp)
    ld t3, 32(sp); ld t4, 40(sp);  ld t5, 48(sp);  ld t6, 56(sp)
    ld a0, 64(sp); ld a1, 72(sp);  ld a2, 80(sp);  ld a3, 88(sp)
    ld a4, 96(sp); ld a5, 104(sp); ld a6, 112(sp); ld a7, 120(sp)
    ld s0, 128(sp); ld s1, 136(sp); ld s2, 144(sp); ld s3, 152(sp)
    ld s4, 160(sp); ld s5, 168(sp); ld s6, 176(sp); ld s7, 184(sp)
    ld s8, 192(sp); ld s9, 200(sp); ld s10, 208(sp); ld s11, 216(sp)
    ld gp, 224(sp); ld tp, 232(sp)
    addi sp, sp, 256
    mret

    /* S-mode trap entry: save, C handler, restore, sret.
       s_trap_c(scause, sepc, stval) returns the sepc to resume at. */
    .globl trap_entry_s
    .align 2
trap_entry_s:
    addi sp, sp, -256
    sd ra, 0(sp);  sd t0, 8(sp);   sd t1, 16(sp);  sd t2, 24(sp)
    sd t3, 32(sp); sd t4, 40(sp);  sd t5, 48(sp);  sd t6, 56(sp)
    sd a0, 64(sp); sd a1, 72(sp);  sd a2, 80(sp);  sd a3, 88(sp)
    sd a4, 96(sp); sd a5, 104(sp); sd a6, 112(sp); sd a7, 120(sp)
    sd s0, 128(sp); sd s1, 136(sp); sd s2, 144(sp); sd s3, 152(sp)
    sd s4, 160(sp); sd s5, 168(sp); sd s6, 176(sp); sd s7, 184(sp)
    sd s8, 192(sp); sd s9, 200(sp); sd s10, 208(sp); sd s11, 216(sp)
    sd gp, 224(sp); sd tp, 232(sp)
    csrr a0, scause
    csrr a1, sepc
    csrr a2, stval
    call s_trap_c
    csrw sepc, a0
    ld ra, 0(sp);  ld t0, 8(sp);   ld t1, 16(sp);  ld t2, 24(sp)
    ld t3, 32(sp); ld t4, 40(sp);  ld t5, 48(sp);  ld t6, 56(sp)
    ld a0, 64(sp); ld a1, 72(sp);  ld a2, 80(sp);  ld a3, 88(sp)
    ld a4, 96(sp); ld a5, 104(sp); ld a6, 112(sp); ld a7, 120(sp)
    ld s0, 128(sp); ld s1, 136(sp); ld s2, 144(sp); ld s3, 152(sp)
    ld s4, 160(sp); ld s5, 168(sp); ld s6, 176(sp); ld s7, 184(sp)
    ld s8, 192(sp); ld s9, 200(sp); ld s10, 208(sp); ld s11, 216(sp)
    ld gp, 224(sp); ld tp, 232(sp)
    addi sp, sp, 256
    sret

    .bss
    .align 4
    .space 8192
stacks_top:
