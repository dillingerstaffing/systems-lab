/* Svadu: hardware-updated Accessed/Dirty PTE bits.
 *
 * Question: who sets the A and D bits in a page-table entry, software or
 * the page-table walker? Without Svadu (menvcfg.ADUE=0) the walker raises
 * a page fault and software sets them. With Svadu (ADUE=1) the walker
 * sets them itself with an atomic update, no trap.
 *
 * Rig: M-mode bare metal, QEMU 8.2.2 virt, -bios none, Sv39. M-mode sets
 * up PMP and page tables, delegates load/store page faults to S-mode via
 * medeleg, then drops to S-mode with mret. The test runs in real S-mode;
 * its page faults are handled by an S-mode handler (stvec). ADUE lives in
 * menvcfg, which only M-mode can write, so S-mode uses ecall to ask
 * M-mode to flip it between phases.
 *
 * Test page VA 0x80100000, identity mapped, PTE starts A=D=0.
 *
 * Design note: the first version stayed in M-mode and used MPRV with
 * MPP=S so data accesses walked satp. It wedged: mret consumes MPP as
 * the return privilege, so the first trap return dropped the hart to
 * S-mode (the next to U-mode) and the retried store faulted forever in
 * the wrong mode. QEMU's helper_mret also clears MPRV on a return to a
 * less-privileged mode. You cannot mret back to M-mode while leaving
 * MPP=S for MPRV, so the honest structure is the one a real kernel
 * uses: run the test in S-mode and delegate the faults.
 */
typedef unsigned long u64;
typedef unsigned int u32;

#define UART ((volatile unsigned char*)0x10000000UL)
static void uputc(char c){ *UART = c; }
static void uputs(const char *s){ while(*s) uputc(*s++); }
static void uputhex(u64 v){ uputs("0x");
    for(int i=60;i>=0;i-=4) uputc("0123456789abcdef"[(v>>i)&0xf]); }
static void uputdec(u64 v){ char b[24]; int n=0;
    if(!v) uputc('0');
    while(v){ b[n++]='0'+(v%10); v/=10; }
    while(n--) uputc(b[n]); }

static u64 csr_read(u32 csr){ u64 v;
    __asm__ volatile("csrr %0, %1" : "=r"(v) : "i"(csr)); return v; }
static void csr_write(u32 csr, u64 v){
    __asm__ volatile("csrw %0, %1" :: "i"(csr), "r"(v)); }
static void sfence_vma(void){
    __asm__ volatile("sfence.vma" ::: "memory"); }

#define CSR_MSTATUS 0x300
#define CSR_MENVCFG 0x30A
#define CSR_SATP    0x180
#define CSR_MEDELEG 0x302
#define CSR_MTVEC   0x305
#define CSR_STVEC   0x105
#define CSR_MEPC    0x341
#define CSR_SCAUSE  0x142
#define CSR_SEPC    0x141
#define CSR_STVAL   0x143

#define PTE_V (1UL<<0)
#define PTE_R (1UL<<1)
#define PTE_W (1UL<<2)
#define PTE_X (1UL<<3)
#define PTE_A (1UL<<6)
#define PTE_D (1UL<<7)
#define PTE(ppn, fl) ((((u64)(ppn))<<10) | (fl))

#define TEST_VA 0x80100000UL   /* one page, identity mapped, A=D=0 to start */

__attribute__((aligned(4096))) static u64 root_pt[512];
__attribute__((aligned(4096))) static u64 l1_id[512];    /* VPN2=2: 0x80000000- */
__attribute__((aligned(4096))) static u64 l0_id[512];    /* 0x80000000-0x801FFFFF */

static volatile u64 trap_count;
static volatile u64 *test_pte;   /* &l0_id[256] */

extern void trap_entry_m(void);
extern void trap_entry_s(void);

/* S-mode trap handler: fix up A/D in software, retry the faulting access. */
u64 s_trap_c(u64 scause, u64 sepc, u64 stval){
    trap_count++;
    if(scause==15){            /* store/AMO page fault: software sets A and D */
        if(stval != TEST_VA){ uputs("S-UNEXPECTED store tval="); uputhex(stval); uputc('\n'); for(;;); }
        *test_pte |= (PTE_A|PTE_D);
        sfence_vma();          /* retry the faulting store (sepc unchanged) */
        return sepc;
    }
    if(scause==13){            /* load page fault: software sets A */
        if(stval != TEST_VA){ uputs("S-UNEXPECTED load tval="); uputhex(stval); uputc('\n'); for(;;); }
        *test_pte |= PTE_A;
        sfence_vma();          /* retry the faulting load */
        return sepc;
    }
    uputs("S-UNEXPECTED scause="); uputdec(scause);
    uputs(" sepc="); uputhex(sepc); uputs(" stval="); uputhex(stval); uputc('\n');
    for(;;);
}

/* M-mode trap handler: only the ADUE-flip ecall should arrive here. */
u64 m_trap_c(u64 mcause, u64 mepc, u64 mtval){
    (void)mtval;
    if(mcause==9){             /* ecall from S-mode: enable Svadu */
        u64 e = csr_read(CSR_MENVCFG);
        csr_write(CSR_MENVCFG, e | (1UL<<61));
        uputs("m-mode: ADUE=1 menvcfg="); uputhex(e | (1UL<<61)); uputc('\n');
        return mepc+4;
    }
    uputs("M-UNEXPECTED mcause="); uputdec(mcause);
    uputs(" mepc="); uputhex(mepc); uputc('\n');
    for(;;);
}

static int pass=0, total=0;
static void check(const char *name, int ok){
    total++; if(ok){ pass++; uputs("PASS "); } else uputs("FAIL "); uputs(name); uputc('\n'); }

static void build_tables(void){
    int i;
    for(i=0;i<512;i++)
        l0_id[i] = PTE(0x80000UL + i, PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D);
    l0_id[256] = PTE(0x80100UL, PTE_V|PTE_R|PTE_W);   /* test page: A=D=0 */
    l1_id[0] = PTE(((u64)l0_id)>>12, PTE_V);
    root_pt[2] = PTE(((u64)l1_id)>>12, PTE_V);
    /* 1GB megapage over [0, 0x40000000): UART and finisher for S-mode */
    root_pt[0] = PTE(0, PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D);

    test_pte = &l0_id[256];
}

static void pte_reset(void){
    *test_pte = PTE(0x80100UL, PTE_V|PTE_R|PTE_W);  /* A=D=0 again */
    sfence_vma();
}

static void poweroff(int ok){
    uputs("result "); uputdec(pass); uputc('/'); uputdec(total); uputs(" pass\n");
    *(volatile u32*)0x100000UL = ok ? 0x5555U : 0x13333U;
    for(;;);
}

/* Runs in S-mode. ecall flips ADUE in M-mode between the two phases. */
void s_main(void){
    volatile u32 *tp = (volatile u32*)TEST_VA;
    u32 v;

    uputs("s-mode up\n");

    /* ---- phase 0: ADUE=0, software maintains A/D ---- */
    pte_reset(); trap_count=0;
    *tp = 0xA5A5A5A5U;
    v = *tp;
    uputs("ph0 store: traps="); uputdec(trap_count);
    uputs(" pte="); uputhex(*test_pte); uputc('\n');
    check("S1 store with ADUE=0 traps once (scause 15), software sets A|D",
          trap_count==1 &&
          (*test_pte & (PTE_A|PTE_D))==(PTE_A|PTE_D) && v==0xA5A5A5A5U);

    pte_reset(); trap_count=0;
    v = *tp;
    uputs("ph0 load:  traps="); uputdec(trap_count);
    uputs(" pte="); uputhex(*test_pte); uputc('\n');
    check("S2 load with ADUE=0 traps once (scause 13), software sets A",
          trap_count==1 &&
          (*test_pte & PTE_A) && !(*test_pte & PTE_D) && v==0xA5A5A5A5U);

    /* ---- phase 1: ADUE=1, the walker maintains A/D ---- */
    __asm__ volatile("ecall");
    sfence_vma();

    pte_reset(); trap_count=0;
    *tp = 0x5A5A5A5AU;
    v = *tp;
    uputs("ph1 store: traps="); uputdec(trap_count);
    uputs(" pte="); uputhex(*test_pte); uputc('\n');
    check("S3 store with ADUE=1: zero traps, walker set A and D",
          trap_count==0 &&
          (*test_pte & (PTE_A|PTE_D))==(PTE_A|PTE_D) && v==0x5A5A5A5AU);

    pte_reset(); trap_count=0;
    v = *tp;
    uputs("ph1 load:  traps="); uputdec(trap_count);
    uputs(" pte="); uputhex(*test_pte); uputc('\n');
    check("S4 load with ADUE=1: zero traps, walker set A only",
          trap_count==0 &&
          (*test_pte & PTE_A) && !(*test_pte & PTE_D) && v==0x5A5A5A5AU);

    poweroff(pass==total);
}

/* Runs in M-mode. Sets up the machine, then drops to S-mode. */
void m_main(void){
    u64 e, e2, adue;

    uputs("SVADU up\n");
    build_tables();

    /* PMP: default PMP denies S-mode everything. Grant [0, 2^56)
       TOR R|W|X (the xv6 start.c pattern); M-mode stays exempt (L=0). */
    csr_write(0x3B0, 0x3FFFFFFFFFFFFFULL);  /* pmpaddr0 */
    csr_write(0x3A0, 0x0FULL);              /* pmpcfg0: TOR,R,W,X */

    /* probe menvcfg.ADUE (bit 61): does this hart implement Svadu? */
    e = csr_read(CSR_MENVCFG);
    csr_write(CSR_MENVCFG, e | (1UL<<61));
    e2 = csr_read(CSR_MENVCFG);
    adue = (e2>>61)&1UL;
    uputs("menvcfg="); uputhex(e2); uputs(" ADUE="); uputdec(adue); uputc('\n');
    if(!adue){ uputs("Svadu not implemented on this hart\n"); for(;;); }
    csr_write(CSR_MENVCFG, e2 & ~(1UL<<61));  /* phase 0 runs with ADUE=0 */

    csr_write(CSR_SATP, (8UL<<60) | (((u64)root_pt)>>12));
    csr_write(CSR_MEDELEG, (1UL<<13)|(1UL<<15));  /* load/store page faults to S-mode */
    csr_write(CSR_MTVEC, (u64)trap_entry_m);
    csr_write(CSR_STVEC, (u64)trap_entry_s);
    sfence_vma();

    /* drop to S-mode: MPP=S, then mret */
    csr_write(CSR_MSTATUS, (csr_read(CSR_MSTATUS) & ~(3UL<<11)) | (1UL<<11));
    csr_write(0x341, (u64)s_main);   /* mepc */
    __asm__ volatile("mret");
    __builtin_unreachable();
}
