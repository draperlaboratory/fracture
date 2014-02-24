	.syntax unified
	.eabi_attribute 6, 10
	.eabi_attribute 7, 65
	.eabi_attribute 8, 1
	.eabi_attribute 9, 2
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.file	"fib-arm-clang.ll"
	.text
	.globl	main
	.align	2
	.type	main,%function
main:                                   @ @main
@ BB#0:                                 @ %entry
	push	{r4, r5, lr}
	sub	sp, sp, #8
	ldr	r4, .LCPI0_2
	mov	r0, #0
	str	r0, [sp, #4]
	str	r0, [sp]
	b	.LBB0_2
.LBB0_1:                                @ %for.body
                                        @   in Loop: Header=BB0_2 Depth=1
	ldr	r5, [sp]
	mov	r0, r5
	bl	fib
	mov	r2, r0
	mov	r0, r4
	mov	r1, r5
	bl	printf
	ldr	r0, [sp]
	add	r0, r0, #1
	str	r0, [sp]
.LBB0_2:                                @ %for.cond
                                        @ =>This Inner Loop Header: Depth=1
	ldr	r0, [sp]
	cmp	r0, #34
	bls	.LBB0_1
@ BB#3:                                 @ %for.end
	ldr	r4, .LCPI0_1
	mov	r0, #0
	str	r0, [sp]
	b	.LBB0_5
.LBB0_4:                                @ %for.body4
                                        @   in Loop: Header=BB0_5 Depth=1
	ldr	r5, [sp]
	mov	r0, r5
	bl	fastfib
	mov	r2, r0
	mov	r0, r4
	mov	r1, r5
	bl	printf
	ldr	r0, [sp]
	add	r0, r0, #1
	str	r0, [sp]
.LBB0_5:                                @ %for.cond2
                                        @ =>This Inner Loop Header: Depth=1
	ldr	r0, [sp]
	cmp	r0, #34
	bls	.LBB0_4
@ BB#6:                                 @ %for.end9
	ldr	r4, .LCPI0_0
	mov	r0, #0
	str	r0, [sp]
	b	.LBB0_8
.LBB0_7:                                @ %for.body12
                                        @   in Loop: Header=BB0_8 Depth=1
	ldr	r5, [sp]
	mov	r0, r5
	bl	fastfib
	mov	r2, r0
	mov	r0, r4
	mov	r1, r5
	bl	printf
	ldr	r0, [sp]
	add	r0, r0, #1
	str	r0, [sp]
.LBB0_8:                                @ %for.cond10
                                        @ =>This Inner Loop Header: Depth=1
	ldr	r0, [sp]
	cmp	r0, #34
	bls	.LBB0_7
@ BB#9:                                 @ %for.end17
	mov	r0, #0
	add	sp, sp, #8
	pop	{r4, r5, lr}
	bx	lr
	.align	2
@ BB#10:
.LCPI0_0:
	.long	.L.str2
.LCPI0_1:
	.long	.L.str1
.LCPI0_2:
	.long	.L.str
.Ltmp0:
	.size	main, .Ltmp0-main

	.globl	fib
	.align	2
	.type	fib,%function
fib:                                    @ @fib
@ BB#0:                                 @ %entry
	push	{r4, lr}
	push	{r0}
	cmp	r0, #1
	bhi	.LBB1_2
@ BB#1:                                 @ %cond.true
	ldr	r0, [sp]
	b	.LBB1_3
.LBB1_2:                                @ %cond.false
	ldr	r0, [sp]
	sub	r0, r0, #1
	bl	fib
	mov	r4, r0
	ldr	r0, [sp]
	sub	r0, r0, #2
	bl	fib
	add	r0, r4, r0
.LBB1_3:                                @ %cond.end
	add	sp, sp, #4
	pop	{r4, lr}
	bx	lr
.Ltmp1:
	.size	fib, .Ltmp1-fib

	.globl	fastfib
	.align	2
	.type	fastfib,%function
fastfib:                                @ @fastfib
@ BB#0:                                 @ %entry
	sub	sp, sp, #24
	str	r0, [sp, #20]
	add	r0, sp, #8
	mov	r1, #0
	add	r12, r0, #8
	add	r2, r0, #4
	str	r0, [sp, #4]
	str	r1, [sp]
	b	.LBB2_2
.LBB2_1:                                @ %if.end23
                                        @   in Loop: Header=BB2_2 Depth=1
	ldr	r1, [sp, #4]
	add	r3, r1, #4
	ldr	r1, [sp]
	cmp	r3, r12
	str	r3, [sp, #4]
	strhi	r0, [sp, #4]
	add	r1, r1, #1
	str	r1, [sp]
.LBB2_2:                                @ %for.cond
                                        @ =>This Inner Loop Header: Depth=1
	ldr	r3, [sp, #20]
	ldr	r1, [sp]
	cmp	r1, r3
	bhi	.LBB2_9
@ BB#3:                                 @ %for.body
                                        @   in Loop: Header=BB2_2 Depth=1
	ldr	r1, [sp]
	cmp	r1, #1
	bhi	.LBB2_5
@ BB#4:                                 @ %if.then
                                        @   in Loop: Header=BB2_2 Depth=1
	ldm	sp, {r1, r3}
	str	r1, [r3]
	b	.LBB2_1
.LBB2_5:                                @ %if.else
                                        @   in Loop: Header=BB2_2 Depth=1
	ldr	r1, [sp, #4]
	cmp	r1, r0
	bne	.LBB2_7
@ BB#6:                                 @ %if.then4
                                        @   in Loop: Header=BB2_2 Depth=1
	ldr	r3, [sp, #12]
	ldr	r1, [sp, #16]
	b	.LBB2_8
.LBB2_7:                                @ %if.else8
                                        @   in Loop: Header=BB2_2 Depth=1
	ldr	r1, [sp, #4]
	ldr	r3, [sp, #8]
	cmp	r1, r2
	ldrne	r1, [sp, #12]
	ldreq	r1, [sp, #16]
.LBB2_8:                                @ %if.else17
                                        @   in Loop: Header=BB2_2 Depth=1
	add	r3, r3, r1
	ldr	r1, [sp, #4]
	str	r3, [r1]
	b	.LBB2_1
.LBB2_9:                                @ %for.end
	ldr	r1, [sp, #4]
	cmp	r1, r0
	ldr	r0, [sp, #4]
	ldreq	r0, [r0, #8]
	subne	r0, r0, #4
	ldrne	r0, [r0]
	add	sp, sp, #24
	bx	lr
.Ltmp2:
	.size	fastfib, .Ltmp2-fastfib

	.globl	fastfib_v2
	.align	2
	.type	fastfib_v2,%function
fastfib_v2:                             @ @fastfib_v2
@ BB#0:                                 @ %entry
	sub	sp, sp, #24
	str	r0, [sp, #16]
	mov	r1, #1
	mov	r0, #0
	str	r1, [sp, #8]
	ldr	r1, [sp, #16]
	str	r0, [sp, #12]
	cmp	r1, #0
	beq	.LBB3_4
	b	.LBB3_2
.LBB3_1:                                @ %for.body
                                        @   in Loop: Header=BB3_2 Depth=1
	ldr	r0, [sp, #8]
	ldr	r1, [sp, #12]
	str	r0, [sp, #4]
	ldr	r0, [sp, #8]
	add	r0, r1, r0
	str	r0, [sp, #8]
	ldr	r0, [sp, #4]
	str	r0, [sp, #12]
	ldr	r0, [sp]
	add	r0, r0, #1
.LBB3_2:                                @ %if.end
                                        @ =>This Inner Loop Header: Depth=1
	str	r0, [sp]
	ldr	r0, [sp, #16]
	ldr	r1, [sp]
	sub	r0, r0, #1
	cmp	r1, r0
	blo	.LBB3_1
@ BB#3:                                 @ %for.end
	ldr	r0, [sp, #8]
.LBB3_4:                                @ %for.end
	str	r0, [sp, #20]
	ldr	r0, [sp, #20]
	add	sp, sp, #24
	bx	lr
.Ltmp3:
	.size	fastfib_v2, .Ltmp3-fastfib_v2

	.type	.L.str,%object          @ @.str
	.section	.rodata.str1.1,"aMS",%progbits,1
.L.str:
	.asciz	 "fib(%u)=%u\n"
	.size	.L.str, 12

	.type	.L.str1,%object         @ @.str1
.L.str1:
	.asciz	 "fastfib(%u)=%u\n"
	.size	.L.str1, 16

	.type	.L.str2,%object         @ @.str2
.L.str2:
	.asciz	 "fastfib_v2(%u)=%u\n"
	.size	.L.str2, 19


