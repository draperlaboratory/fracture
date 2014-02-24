	.syntax unified
	.eabi_attribute 6, 2
	.eabi_attribute 8, 1
	.eabi_attribute 9, 1
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.file	"a.out.bc"
	.text
	.globl	main
	.align	2
	.type	main,%function
main:                                   @ @main
@ BB#0:                                 @ %entry
	push	{r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #12
	mov	r4, #0
	ldr	r5, .LCPI0_2
	b	.LBB0_2
.LBB0_1:                                @ %for.inc
                                        @   in Loop: Header=BB0_2 Depth=1
	mov	r0, r4
	bl	fib
	mov	r2, r0
	mov	r0, r5
	mov	r1, r4
	bl	printf
	add	r4, r4, #1
.LBB0_2:                                @ %for.cond
                                        @ =>This Inner Loop Header: Depth=1
	cmp	r4, #35
	blo	.LBB0_1
@ BB#3:                                 @ %for.cond2.preheader
	mov	r4, sp
	mov	r9, #0
	ldr	r8, .LCPI0_1
	mov	r6, #0
	add	r7, r4, #4
	add	r5, r4, #8
	b	.LBB0_5
.LBB0_4:                                @ %fastfib.exit
                                        @   in Loop: Header=BB0_5 Depth=1
	sub	r1, r0, #4
	cmp	r0, r4
	mov	r0, r8
	moveq	r1, r5
	ldr	r2, [r1]
	mov	r1, r6
	bl	printf
	add	r6, r6, #1
.LBB0_5:                                @ %for.cond2
                                        @ =>This Loop Header: Depth=1
                                        @     Child Loop BB0_7 Depth 2
	mov	r0, r4
	mov	r1, #0
	cmp	r6, #34
	bls	.LBB0_7
	b	.LBB0_15
.LBB0_6:                                @ %if.end23.i
                                        @   in Loop: Header=BB0_7 Depth=2
	add	r0, r0, #4
	add	r1, r1, #1
	cmp	r0, r5
	movhi	r0, r4
.LBB0_7:                                @ %for.cond.i
                                        @   Parent Loop BB0_5 Depth=1
                                        @ =>  This Inner Loop Header: Depth=2
	cmp	r1, r6
	bhi	.LBB0_4
@ BB#8:                                 @ %for.body.i
                                        @   in Loop: Header=BB0_7 Depth=2
	cmp	r1, #1
	bhi	.LBB0_10
@ BB#9:                                 @ %if.then.i
                                        @   in Loop: Header=BB0_7 Depth=2
	str	r1, [r0]
	b	.LBB0_6
.LBB0_10:                               @ %if.else.i
                                        @   in Loop: Header=BB0_7 Depth=2
	cmp	r0, r4
	bne	.LBB0_12
@ BB#11:                                @ %if.then4.i
                                        @   in Loop: Header=BB0_7 Depth=2
	ldmib	sp, {r2, r3}
	add	r2, r2, r3
	str	r2, [sp]
	b	.LBB0_6
.LBB0_12:                               @ %if.else8.i
                                        @   in Loop: Header=BB0_7 Depth=2
	ldr	r2, [sp]
	cmp	r0, r7
	bne	.LBB0_14
@ BB#13:                                @ %if.then12.i
                                        @   in Loop: Header=BB0_7 Depth=2
	ldr	r3, [sp, #8]
	add	r2, r2, r3
	str	r2, [sp, #4]
	b	.LBB0_6
.LBB0_14:                               @ %if.else17.i
                                        @   in Loop: Header=BB0_7 Depth=2
	ldr	r3, [sp, #4]
	add	r2, r2, r3
	str	r2, [r0]
	b	.LBB0_6
.LBB0_15:
	mov	r4, sp
	ldr	r6, .LCPI0_0
	b	.LBB0_17
.LBB0_16:                               @ %fastfib.exit32
                                        @   in Loop: Header=BB0_17 Depth=1
	sub	r1, r0, #4
	cmp	r0, r4
	mov	r0, r6
	moveq	r1, r5
	ldr	r2, [r1]
	mov	r1, r9
	bl	printf
	add	r9, r9, #1
.LBB0_17:                               @ %for.cond10
                                        @ =>This Loop Header: Depth=1
                                        @     Child Loop BB0_19 Depth 2
	mov	r1, #0
	mov	r0, r4
	cmp	r9, #34
	bls	.LBB0_19
	b	.LBB0_27
.LBB0_18:                               @ %if.end23.i26
                                        @   in Loop: Header=BB0_19 Depth=2
	add	r0, r0, #4
	add	r1, r1, #1
	cmp	r0, r5
	movhi	r0, r4
.LBB0_19:                               @ %for.cond.i8
                                        @   Parent Loop BB0_17 Depth=1
                                        @ =>  This Inner Loop Header: Depth=2
	cmp	r1, r9
	bhi	.LBB0_16
@ BB#20:                                @ %for.body.i10
                                        @   in Loop: Header=BB0_19 Depth=2
	cmp	r1, #1
	bhi	.LBB0_22
@ BB#21:                                @ %if.then.i11
                                        @   in Loop: Header=BB0_19 Depth=2
	str	r1, [r0]
	b	.LBB0_18
.LBB0_22:                               @ %if.else.i13
                                        @   in Loop: Header=BB0_19 Depth=2
	cmp	r0, r4
	bne	.LBB0_24
@ BB#23:                                @ %if.then4.i15
                                        @   in Loop: Header=BB0_19 Depth=2
	ldmib	sp, {r2, r3}
	add	r2, r2, r3
	str	r2, [sp]
	b	.LBB0_18
.LBB0_24:                               @ %if.else8.i17
                                        @   in Loop: Header=BB0_19 Depth=2
	ldr	r2, [sp]
	cmp	r0, r7
	bne	.LBB0_26
@ BB#25:                                @ %if.then12.i19
                                        @   in Loop: Header=BB0_19 Depth=2
	ldr	r3, [sp, #8]
	add	r2, r2, r3
	str	r2, [sp, #4]
	b	.LBB0_18
.LBB0_26:                               @ %if.else17.i21
                                        @   in Loop: Header=BB0_19 Depth=2
	ldr	r3, [sp, #4]
	add	r2, r2, r3
	str	r2, [r0]
	b	.LBB0_18
.LBB0_27:                               @ %for.end17
	mov	r0, #0
	add	sp, sp, #12
	pop	{r4, r5, r6, r7, r8, r9, lr}
	bx	lr
	.align	2
@ BB#28:
.LCPI0_0:
	.long	.L.str2
.LCPI0_1:
	.long	.L.str1
.LCPI0_2:
	.long	.L.str
.Ltmp0:
	.size	main, .Ltmp0-main

	.align	2
	.type	fib,%function
fib:                                    @ @fib
@ BB#0:                                 @ %entry
	push	{r4, r5, lr}
	mov	r4, r0
	cmp	r4, #2
	blo	.LBB1_2
@ BB#1:                                 @ %cond.false
	sub	r0, r4, #1
	bl	fib
	mov	r5, r0
	sub	r0, r4, #2
	bl	fib
	add	r4, r5, r0
.LBB1_2:                                @ %cond.end
	mov	r0, r4
	pop	{r4, r5, lr}
	bx	lr
.Ltmp1:
	.size	fib, .Ltmp1-fib

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


