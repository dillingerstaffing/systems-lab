	.file	"tso.c"
	.option nopic
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zmmul1p0_zaamo1p0_zalrsc1p0_zca1p0_zcd1p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.align	1
	.globl	st_seq_cst
	.type	st_seq_cst, @function
st_seq_cst:
	lui	a5,%hi(x)
	addi	a5,a5,%lo(x)
	fence	rw,w
	sw	a0,0(a5)
	fence	rw,rw
	ret
	.size	st_seq_cst, .-st_seq_cst
	.align	1
	.globl	ld_seq_cst
	.type	ld_seq_cst, @function
ld_seq_cst:
	lui	a5,%hi(y)
	addi	a5,a5,%lo(y)
	fence	rw,rw
	lw	a0,0(a5)
	fence	r,rw
	sext.w	a0,a0
	ret
	.size	ld_seq_cst, .-ld_seq_cst
	.align	1
	.globl	st_release
	.type	st_release, @function
st_release:
	lui	a5,%hi(x)
	addi	a5,a5,%lo(x)
	fence	rw,w
	sw	a0,0(a5)
	ret
	.size	st_release, .-st_release
	.align	1
	.globl	ld_acquire
	.type	ld_acquire, @function
ld_acquire:
	lui	a5,%hi(y)
	addi	a5,a5,%lo(y)
	lw	a0,0(a5)
	fence	r,rw
	sext.w	a0,a0
	ret
	.size	ld_acquire, .-ld_acquire
	.globl	y
	.globl	x
	.section	.sbss,"aw",@nobits
	.align	2
	.type	y, @object
	.size	y, 4
y:
	.zero	4
	.type	x, @object
	.size	x, 4
x:
	.zero	4
	.ident	"GCC: (xPack GNU RISC-V Embedded GCC x86_64) 15.2.0"
	.section	.note.GNU-stack,"",@progbits
