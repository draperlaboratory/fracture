	.section	__TEXT,__text,regular,pure_instructions
	.section	__TEXT,__textcoal_nt,coalesced,pure_instructions
	.section	__TEXT,__const_coal,coalesced
	.section	__TEXT,__picsymbolstub4,symbol_stubs,none,16
	.section	__TEXT,__StaticInit,regular,pure_instructions
	.section	__TEXT,__cstring,cstring_literals
	.syntax unified
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.align	2
	.code	16                      @ @main
	.thumb_func	_main
_main:
@ BB#0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, #8
	movw	r0, :lower16:(L_.str-(LPC0_0+4))
	movt	r0, :upper16:(L_.str-(LPC0_0+4))
LPC0_0:
	add	r0, pc
	movs	r1, #0
	movt	r1, #0
	str	r1, [sp, #4]
	bl	_printf
	movs	r1, #0
	movt	r1, #0
	str	r0, [sp]                @ 4-byte Spill
	mov	r0, r1
	add	sp, #8
	pop	{r7, pc}

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 @ @.str
	.asciz	"Test\n"


.subsections_via_symbols
