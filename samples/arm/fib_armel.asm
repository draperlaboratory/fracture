
fib_armel:     file format elf32-littlearm


Disassembly of section .init:

000082c8 <_init>:
    82c8:	e92d4010 	push	{r4, lr}
    82cc:	eb000021 	bl	8358 <call_gmon_start>
    82d0:	e8bd4010 	pop	{r4, lr}
    82d4:	e12fff1e 	bx	lr

Disassembly of section .plt:

000082d8 <.plt>:
    82d8:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
    82dc:	e59fe004 	ldr	lr, [pc, #4]	; 82e8 <_init+0x20>
    82e0:	e08fe00e 	add	lr, pc, lr
    82e4:	e5bef008 	ldr	pc, [lr, #8]!
    82e8:	000085dc 	.word	0x000085dc
    82ec:	e28fc600 	add	ip, pc, #0
    82f0:	e28cca08 	add	ip, ip, #32768	; 0x8000
    82f4:	e5bcf5dc 	ldr	pc, [ip, #1500]!	; 0x5dc
    82f8:	e28fc600 	add	ip, pc, #0
    82fc:	e28cca08 	add	ip, ip, #32768	; 0x8000
    8300:	e5bcf5d4 	ldr	pc, [ip, #1492]!	; 0x5d4
    8304:	e28fc600 	add	ip, pc, #0
    8308:	e28cca08 	add	ip, ip, #32768	; 0x8000
    830c:	e5bcf5cc 	ldr	pc, [ip, #1484]!	; 0x5cc
    8310:	e28fc600 	add	ip, pc, #0
    8314:	e28cca08 	add	ip, ip, #32768	; 0x8000
    8318:	e5bcf5c4 	ldr	pc, [ip, #1476]!	; 0x5c4

Disassembly of section .text:

0000831c <_start>:
    831c:	e3a0b000 	mov	fp, #0
    8320:	e3a0e000 	mov	lr, #0
    8324:	e49d1004 	pop	{r1}		; (ldr r1, [sp], #4)
    8328:	e1a0200d 	mov	r2, sp
    832c:	e52d2004 	push	{r2}		; (str r2, [sp, #-4]!)
    8330:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
    8334:	e59fc010 	ldr	ip, [pc, #16]	; 834c <_start+0x30>
    8338:	e52dc004 	push	{ip}		; (str ip, [sp, #-4]!)
    833c:	e59f000c 	ldr	r0, [pc, #12]	; 8350 <_start+0x34>
    8340:	e59f300c 	ldr	r3, [pc, #12]	; 8354 <_start+0x38>
    8344:	ebffffeb 	bl	82f8 <_init+0x30>
    8348:	ebffffe7 	bl	82ec <_init+0x24>
    834c:	00008708 	.word	0x00008708
    8350:	000083d4 	.word	0x000083d4
    8354:	0000870c 	.word	0x0000870c

00008358 <call_gmon_start>:
    8358:	e59f301c 	ldr	r3, [pc, #28]	; 837c <call_gmon_start+0x24>
    835c:	e59f201c 	ldr	r2, [pc, #28]	; 8380 <call_gmon_start+0x28>
    8360:	e08f3003 	add	r3, pc, r3
    8364:	e7932002 	ldr	r2, [r3, r2]
    8368:	e92d4010 	push	{r4, lr}
    836c:	e3520000 	cmp	r2, #0
    8370:	1bffffe3 	blne	8304 <_init+0x3c>
    8374:	e8bd4010 	pop	{r4, lr}
    8378:	e12fff1e 	bx	lr
    837c:	0000855c 	.word	0x0000855c
    8380:	0000001c 	.word	0x0000001c

00008384 <__do_global_dtors_aux>:
    8384:	e59f3010 	ldr	r3, [pc, #16]	; 839c <__do_global_dtors_aux+0x18>
    8388:	e5d32000 	ldrb	r2, [r3]
    838c:	e3520000 	cmp	r2, #0
    8390:	03a02001 	moveq	r2, #1
    8394:	05c32000 	strbeq	r2, [r3]
    8398:	e12fff1e 	bx	lr
    839c:	000108ec 	.word	0x000108ec

000083a0 <frame_dummy>:
    83a0:	e59f0024 	ldr	r0, [pc, #36]	; 83cc <frame_dummy+0x2c>
    83a4:	e92d4010 	push	{r4, lr}
    83a8:	e5903000 	ldr	r3, [r0]
    83ac:	e3530000 	cmp	r3, #0
    83b0:	0a000003 	beq	83c4 <frame_dummy+0x24>
    83b4:	e59f3014 	ldr	r3, [pc, #20]	; 83d0 <frame_dummy+0x30>
    83b8:	e3530000 	cmp	r3, #0
    83bc:	11a0e00f 	movne	lr, pc
    83c0:	112fff13 	bxne	r3
    83c4:	e8bd4010 	pop	{r4, lr}
    83c8:	e12fff1e 	bx	lr
    83cc:	000107d0 	.word	0x000107d0
    83d0:	00000000 	.word	0x00000000

000083d4 <main>:
    83d4:	e92d4810 	push	{r4, fp, lr}
    83d8:	e28db008 	add	fp, sp, #8
    83dc:	e24dd00c 	sub	sp, sp, #12
    83e0:	e3a03000 	mov	r3, #0
    83e4:	e50b3010 	str	r3, [fp, #-16]
    83e8:	ea00000a 	b	8418 <main+0x44>
    83ec:	e59f40cc 	ldr	r4, [pc, #204]	; 84c0 <main+0xec>
    83f0:	e51b0010 	ldr	r0, [fp, #-16]
    83f4:	eb000034 	bl	84cc <fib>
    83f8:	e1a03000 	mov	r3, r0
    83fc:	e1a00004 	mov	r0, r4
    8400:	e51b1010 	ldr	r1, [fp, #-16]
    8404:	e1a02003 	mov	r2, r3
    8408:	ebffffc0 	bl	8310 <_init+0x48>
    840c:	e51b3010 	ldr	r3, [fp, #-16]
    8410:	e2833001 	add	r3, r3, #1
    8414:	e50b3010 	str	r3, [fp, #-16]
    8418:	e51b3010 	ldr	r3, [fp, #-16]
    841c:	e3530022 	cmp	r3, #34	; 0x22
    8420:	9afffff1 	bls	83ec <main+0x18>
    8424:	e3a03000 	mov	r3, #0
    8428:	e50b3010 	str	r3, [fp, #-16]
    842c:	ea00000a 	b	845c <main+0x88>
    8430:	e59f408c 	ldr	r4, [pc, #140]	; 84c4 <main+0xf0>
    8434:	e51b0010 	ldr	r0, [fp, #-16]
    8438:	eb00003b 	bl	852c <fastfib>
    843c:	e1a03000 	mov	r3, r0
    8440:	e1a00004 	mov	r0, r4
    8444:	e51b1010 	ldr	r1, [fp, #-16]
    8448:	e1a02003 	mov	r2, r3
    844c:	ebffffaf 	bl	8310 <_init+0x48>
    8450:	e51b3010 	ldr	r3, [fp, #-16]
    8454:	e2833001 	add	r3, r3, #1
    8458:	e50b3010 	str	r3, [fp, #-16]
    845c:	e51b3010 	ldr	r3, [fp, #-16]
    8460:	e3530022 	cmp	r3, #34	; 0x22
    8464:	9afffff1 	bls	8430 <main+0x5c>
    8468:	e3a03000 	mov	r3, #0
    846c:	e50b3010 	str	r3, [fp, #-16]
    8470:	ea00000a 	b	84a0 <main+0xcc>
    8474:	e59f404c 	ldr	r4, [pc, #76]	; 84c8 <main+0xf4>
    8478:	e51b0010 	ldr	r0, [fp, #-16]
    847c:	eb00002a 	bl	852c <fastfib>
    8480:	e1a03000 	mov	r3, r0
    8484:	e1a00004 	mov	r0, r4
    8488:	e51b1010 	ldr	r1, [fp, #-16]
    848c:	e1a02003 	mov	r2, r3
    8490:	ebffff9e 	bl	8310 <_init+0x48>
    8494:	e51b3010 	ldr	r3, [fp, #-16]
    8498:	e2833001 	add	r3, r3, #1
    849c:	e50b3010 	str	r3, [fp, #-16]
    84a0:	e51b3010 	ldr	r3, [fp, #-16]
    84a4:	e3530022 	cmp	r3, #34	; 0x22
    84a8:	9afffff1 	bls	8474 <main+0xa0>
    84ac:	e3a03000 	mov	r3, #0
    84b0:	e1a00003 	mov	r0, r3
    84b4:	e24bd008 	sub	sp, fp, #8
    84b8:	e8bd4810 	pop	{r4, fp, lr}
    84bc:	e12fff1e 	bx	lr
    84c0:	0000878c 	.word	0x0000878c
    84c4:	00008798 	.word	0x00008798
    84c8:	000087a8 	.word	0x000087a8

000084cc <fib>:
    84cc:	e92d4810 	push	{r4, fp, lr}
    84d0:	e28db008 	add	fp, sp, #8
    84d4:	e24dd00c 	sub	sp, sp, #12
    84d8:	e50b0010 	str	r0, [fp, #-16]
    84dc:	e51b3010 	ldr	r3, [fp, #-16]
    84e0:	e3530001 	cmp	r3, #1
    84e4:	9a00000b 	bls	8518 <fib+0x4c>
    84e8:	e51b3010 	ldr	r3, [fp, #-16]
    84ec:	e2433001 	sub	r3, r3, #1
    84f0:	e1a00003 	mov	r0, r3
    84f4:	ebfffff4 	bl	84cc <fib>
    84f8:	e1a04000 	mov	r4, r0
    84fc:	e51b3010 	ldr	r3, [fp, #-16]
    8500:	e2433002 	sub	r3, r3, #2
    8504:	e1a00003 	mov	r0, r3
    8508:	ebffffef 	bl	84cc <fib>
    850c:	e1a03000 	mov	r3, r0
    8510:	e0843003 	add	r3, r4, r3
    8514:	ea000000 	b	851c <fib+0x50>
    8518:	e51b3010 	ldr	r3, [fp, #-16]
    851c:	e1a00003 	mov	r0, r3
    8520:	e24bd008 	sub	sp, fp, #8
    8524:	e8bd4810 	pop	{r4, fp, lr}
    8528:	e12fff1e 	bx	lr

0000852c <fastfib>:
    852c:	e52db004 	push	{fp}		; (str fp, [sp, #-4]!)
    8530:	e28db000 	add	fp, sp, #0
    8534:	e24dd024 	sub	sp, sp, #36	; 0x24
    8538:	e50b0020 	str	r0, [fp, #-32]
    853c:	e24b3018 	sub	r3, fp, #24
    8540:	e50b300c 	str	r3, [fp, #-12]
    8544:	e3a03000 	mov	r3, #0
    8548:	e50b3008 	str	r3, [fp, #-8]
    854c:	ea000035 	b	8628 <fastfib+0xfc>
    8550:	e51b3008 	ldr	r3, [fp, #-8]
    8554:	e3530001 	cmp	r3, #1
    8558:	8a000003 	bhi	856c <fastfib+0x40>
    855c:	e51b300c 	ldr	r3, [fp, #-12]
    8560:	e51b2008 	ldr	r2, [fp, #-8]
    8564:	e5832000 	str	r2, [r3]
    8568:	ea000021 	b	85f4 <fastfib+0xc8>
    856c:	e24b3018 	sub	r3, fp, #24
    8570:	e51b200c 	ldr	r2, [fp, #-12]
    8574:	e1520003 	cmp	r2, r3
    8578:	1a000009 	bne	85a4 <fastfib+0x78>
    857c:	e24b3018 	sub	r3, fp, #24
    8580:	e2833004 	add	r3, r3, #4
    8584:	e5932000 	ldr	r2, [r3]
    8588:	e24b3018 	sub	r3, fp, #24
    858c:	e2833008 	add	r3, r3, #8
    8590:	e5933000 	ldr	r3, [r3]
    8594:	e0822003 	add	r2, r2, r3
    8598:	e51b300c 	ldr	r3, [fp, #-12]
    859c:	e5832000 	str	r2, [r3]
    85a0:	ea000013 	b	85f4 <fastfib+0xc8>
    85a4:	e24b3018 	sub	r3, fp, #24
    85a8:	e2833004 	add	r3, r3, #4
    85ac:	e51b200c 	ldr	r2, [fp, #-12]
    85b0:	e1520003 	cmp	r2, r3
    85b4:	1a000007 	bne	85d8 <fastfib+0xac>
    85b8:	e51b2018 	ldr	r2, [fp, #-24]
    85bc:	e24b3018 	sub	r3, fp, #24
    85c0:	e2833008 	add	r3, r3, #8
    85c4:	e5933000 	ldr	r3, [r3]
    85c8:	e0822003 	add	r2, r2, r3
    85cc:	e51b300c 	ldr	r3, [fp, #-12]
    85d0:	e5832000 	str	r2, [r3]
    85d4:	ea000006 	b	85f4 <fastfib+0xc8>
    85d8:	e51b2018 	ldr	r2, [fp, #-24]
    85dc:	e24b3018 	sub	r3, fp, #24
    85e0:	e2833004 	add	r3, r3, #4
    85e4:	e5933000 	ldr	r3, [r3]
    85e8:	e0822003 	add	r2, r2, r3
    85ec:	e51b300c 	ldr	r3, [fp, #-12]
    85f0:	e5832000 	str	r2, [r3]
    85f4:	e51b300c 	ldr	r3, [fp, #-12]
    85f8:	e2833004 	add	r3, r3, #4
    85fc:	e50b300c 	str	r3, [fp, #-12]
    8600:	e24b3018 	sub	r3, fp, #24
    8604:	e2833008 	add	r3, r3, #8
    8608:	e51b200c 	ldr	r2, [fp, #-12]
    860c:	e1520003 	cmp	r2, r3
    8610:	9a000001 	bls	861c <fastfib+0xf0>
    8614:	e24b3018 	sub	r3, fp, #24
    8618:	e50b300c 	str	r3, [fp, #-12]
    861c:	e51b3008 	ldr	r3, [fp, #-8]
    8620:	e2833001 	add	r3, r3, #1
    8624:	e50b3008 	str	r3, [fp, #-8]
    8628:	e51b2008 	ldr	r2, [fp, #-8]
    862c:	e51b3020 	ldr	r3, [fp, #-32]
    8630:	e1520003 	cmp	r2, r3
    8634:	9affffc5 	bls	8550 <fastfib+0x24>
    8638:	e24b3018 	sub	r3, fp, #24
    863c:	e51b200c 	ldr	r2, [fp, #-12]
    8640:	e1520003 	cmp	r2, r3
    8644:	1a000003 	bne	8658 <fastfib+0x12c>
    8648:	e51b300c 	ldr	r3, [fp, #-12]
    864c:	e2833008 	add	r3, r3, #8
    8650:	e5933000 	ldr	r3, [r3]
    8654:	ea000002 	b	8664 <fastfib+0x138>
    8658:	e51b300c 	ldr	r3, [fp, #-12]
    865c:	e2433004 	sub	r3, r3, #4
    8660:	e5933000 	ldr	r3, [r3]
    8664:	e1a00003 	mov	r0, r3
    8668:	e28bd000 	add	sp, fp, #0
    866c:	e8bd0800 	pop	{fp}
    8670:	e12fff1e 	bx	lr

00008674 <fastfib_v2>:
    8674:	e52db004 	push	{fp}		; (str fp, [sp, #-4]!)
    8678:	e28db000 	add	fp, sp, #0
    867c:	e24dd01c 	sub	sp, sp, #28
    8680:	e50b0018 	str	r0, [fp, #-24]
    8684:	e3a03000 	mov	r3, #0
    8688:	e50b3014 	str	r3, [fp, #-20]
    868c:	e3a03001 	mov	r3, #1
    8690:	e50b3010 	str	r3, [fp, #-16]
    8694:	e51b3018 	ldr	r3, [fp, #-24]
    8698:	e3530000 	cmp	r3, #0
    869c:	1a000001 	bne	86a8 <fastfib_v2+0x34>
    86a0:	e3a03000 	mov	r3, #0
    86a4:	ea000013 	b	86f8 <fastfib_v2+0x84>
    86a8:	e3a03000 	mov	r3, #0
    86ac:	e50b3008 	str	r3, [fp, #-8]
    86b0:	ea00000a 	b	86e0 <fastfib_v2+0x6c>
    86b4:	e51b3010 	ldr	r3, [fp, #-16]
    86b8:	e50b300c 	str	r3, [fp, #-12]
    86bc:	e51b2010 	ldr	r2, [fp, #-16]
    86c0:	e51b3014 	ldr	r3, [fp, #-20]
    86c4:	e0823003 	add	r3, r2, r3
    86c8:	e50b3010 	str	r3, [fp, #-16]
    86cc:	e51b300c 	ldr	r3, [fp, #-12]
    86d0:	e50b3014 	str	r3, [fp, #-20]
    86d4:	e51b3008 	ldr	r3, [fp, #-8]
    86d8:	e2833001 	add	r3, r3, #1
    86dc:	e50b3008 	str	r3, [fp, #-8]
    86e0:	e51b3018 	ldr	r3, [fp, #-24]
    86e4:	e2432001 	sub	r2, r3, #1
    86e8:	e51b3008 	ldr	r3, [fp, #-8]
    86ec:	e1520003 	cmp	r2, r3
    86f0:	8affffef 	bhi	86b4 <fastfib_v2+0x40>
    86f4:	e51b3010 	ldr	r3, [fp, #-16]
    86f8:	e1a00003 	mov	r0, r3
    86fc:	e28bd000 	add	sp, fp, #0
    8700:	e8bd0800 	pop	{fp}
    8704:	e12fff1e 	bx	lr

00008708 <__libc_csu_fini>:
    8708:	e12fff1e 	bx	lr

0000870c <__libc_csu_init>:
    870c:	e92d47f0 	push	{r4, r5, r6, r7, r8, r9, sl, lr}
    8710:	e59fa058 	ldr	sl, [pc, #88]	; 8770 <__libc_csu_init+0x64>
    8714:	e59f5058 	ldr	r5, [pc, #88]	; 8774 <__libc_csu_init+0x68>
    8718:	e1a06000 	mov	r6, r0
    871c:	e1a07001 	mov	r7, r1
    8720:	e1a08002 	mov	r8, r2
    8724:	ebfffee7 	bl	82c8 <_init>
    8728:	e59f3048 	ldr	r3, [pc, #72]	; 8778 <__libc_csu_init+0x6c>
    872c:	e06a5005 	rsb	r5, sl, r5
    8730:	e08f3003 	add	r3, pc, r3
    8734:	e1b05145 	asrs	r5, r5, #2
    8738:	e083a00a 	add	sl, r3, sl
    873c:	0a000009 	beq	8768 <__libc_csu_init+0x5c>
    8740:	e3a04000 	mov	r4, #0
    8744:	e1a00006 	mov	r0, r6
    8748:	e1a01007 	mov	r1, r7
    874c:	e1a02008 	mov	r2, r8
    8750:	e79ac104 	ldr	ip, [sl, r4, lsl #2]
    8754:	e1a0e00f 	mov	lr, pc
    8758:	e12fff1c 	bx	ip
    875c:	e2844001 	add	r4, r4, #1
    8760:	e1540005 	cmp	r4, r5
    8764:	3afffff6 	bcc	8744 <__libc_csu_init+0x38>
    8768:	e8bd47f0 	pop	{r4, r5, r6, r7, r8, r9, sl, lr}
    876c:	e12fff1e 	bx	lr
    8770:	ffffff04 	.word	0xffffff04
    8774:	ffffff08 	.word	0xffffff08
    8778:	0000818c 	.word	0x0000818c

Disassembly of section .fini:

0000877c <_fini>:
    877c:	e92d4010 	push	{r4, lr}
    8780:	e8bd4010 	pop	{r4, lr}
    8784:	e12fff1e 	bx	lr
