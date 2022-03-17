
build/rootfs/sbin/init:     file format elf32-littlearm


Disassembly of section .text:

00000100 <main>:
     100:	e1a0c00d 	mov	ip, sp
     104:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
     108:	e3e04000 	mvn	r4, #0
     10c:	e59f3278 	ldr	r3, [pc, #632]	; 38c <main+0x28c>
     110:	e24cb004 	sub	fp, ip, #4
     114:	e08f3003 	add	r3, pc, r3
     118:	e5834000 	str	r4, [r3]
     11c:	eb003302 	bl	cd2c <getuid>
     120:	e59f6268 	ldr	r6, [pc, #616]	; 390 <main+0x290>
     124:	e3500000 	cmp	r0, #0
     128:	e08f6006 	add	r6, pc, r6
     12c:	ba000006 	blt	14c <main+0x4c>
     130:	e59f025c 	ldr	r0, [pc, #604]	; 394 <main+0x294>
     134:	e08f0000 	add	r0, pc, r0
     138:	eb0000ac 	bl	3f0 <out>
     13c:	e1a00004 	mov	r0, r4
     140:	e24bd01c 	sub	sp, fp, #28
     144:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
     148:	e12fff1e 	bx	lr
     14c:	e59f0244 	ldr	r0, [pc, #580]	; 398 <main+0x298>
     150:	e08f0000 	add	r0, pc, r0
     154:	eb0000a5 	bl	3f0 <out>
     158:	e59f123c 	ldr	r1, [pc, #572]	; 39c <main+0x29c>
     15c:	e3a00017 	mov	r0, #23
     160:	e08f1001 	add	r1, pc, r1
     164:	eb001e78 	bl	7b4c <syscall1>
     168:	e59f0230 	ldr	r0, [pc, #560]	; 3a0 <main+0x2a0>
     16c:	e08f0000 	add	r0, pc, r0
     170:	eb00009e 	bl	3f0 <out>
     174:	eb0032bd 	bl	cc70 <fork>
     178:	e3500000 	cmp	r0, #0
     17c:	0a00007c 	beq	374 <main+0x274>
     180:	eb003207 	bl	c9a4 <proc_wait_ready>
     184:	e59f0218 	ldr	r0, [pc, #536]	; 3a4 <main+0x2a4>
     188:	e08f0000 	add	r0, pc, r0
     18c:	eb000097 	bl	3f0 <out>
     190:	e59f0210 	ldr	r0, [pc, #528]	; 3a8 <main+0x2a8>
     194:	e08f0000 	add	r0, pc, r0
     198:	eb000094 	bl	3f0 <out>
     19c:	eb0032b3 	bl	cc70 <fork>
     1a0:	e3500000 	cmp	r0, #0
     1a4:	0a00006c 	beq	35c <main+0x25c>
     1a8:	eb0031fd 	bl	c9a4 <proc_wait_ready>
     1ac:	e59f01f8 	ldr	r0, [pc, #504]	; 3ac <main+0x2ac>
     1b0:	e08f0000 	add	r0, pc, r0
     1b4:	eb00008d 	bl	3f0 <out>
     1b8:	e59f01f0 	ldr	r0, [pc, #496]	; 3b0 <main+0x2b0>
     1bc:	e08f0000 	add	r0, pc, r0
     1c0:	eb00008a 	bl	3f0 <out>
     1c4:	eb0032a9 	bl	cc70 <fork>
     1c8:	e2501000 	subs	r1, r0, #0
     1cc:	1a000060 	bne	354 <main+0x254>
     1d0:	e3a0002f 	mov	r0, #47	; 0x2f
     1d4:	e1a02001 	mov	r2, r1
     1d8:	e1a03001 	mov	r3, r1
     1dc:	eb001e57 	bl	7b40 <syscall3>
     1e0:	e3500000 	cmp	r0, #0
     1e4:	e3a00017 	mov	r0, #23
     1e8:	ba000054 	blt	340 <main+0x240>
     1ec:	e59f11c0 	ldr	r1, [pc, #448]	; 3b4 <main+0x2b4>
     1f0:	e08f1001 	add	r1, pc, r1
     1f4:	eb001e54 	bl	7b4c <syscall1>
     1f8:	eb000a4c 	bl	2b30 <romfsd_main>
     1fc:	e59f01b4 	ldr	r0, [pc, #436]	; 3b8 <main+0x2b8>
     200:	e08f0000 	add	r0, pc, r0
     204:	eb000079 	bl	3f0 <out>
     208:	e3a00000 	mov	r0, #0
     20c:	eb003332 	bl	cedc <setuid>
     210:	eb000144 	bl	728 <load_arch_devs>
     214:	eb000159 	bl	780 <load_extra_devs>
     218:	e59f019c 	ldr	r0, [pc, #412]	; 3bc <main+0x2bc>
     21c:	e08f0000 	add	r0, pc, r0
     220:	eb000125 	bl	6bc <load_devs>
     224:	e59f0194 	ldr	r0, [pc, #404]	; 3c0 <main+0x2c0>
     228:	e3a01002 	mov	r1, #2
     22c:	e08f0000 	add	r0, pc, r0
     230:	eb0035e6 	bl	d9d0 <open>
     234:	e59f3188 	ldr	r3, [pc, #392]	; 3c4 <main+0x2c4>
     238:	e3500000 	cmp	r0, #0
     23c:	e08f3003 	add	r3, pc, r3
     240:	e5830000 	str	r0, [r3]
     244:	c59f117c 	ldrgt	r1, [pc, #380]	; 3c8 <main+0x2c8>
     248:	c08f1001 	addgt	r1, pc, r1
     24c:	cb0035c2 	blgt	d95c <dprintf>
     250:	e59f0174 	ldr	r0, [pc, #372]	; 3cc <main+0x2cc>
     254:	e3a01000 	mov	r1, #0
     258:	e08f0000 	add	r0, pc, r0
     25c:	eb0035db 	bl	d9d0 <open>
     260:	e1a04000 	mov	r4, r0
     264:	e59f5164 	ldr	r5, [pc, #356]	; 3d0 <main+0x2d0>
     268:	e3a01000 	mov	r1, #0
     26c:	eb00324e 	bl	cbac <dup2>
     270:	e08f5005 	add	r5, pc, r5
     274:	e1a00004 	mov	r0, r4
     278:	e3a01001 	mov	r1, #1
     27c:	eb00324a 	bl	cbac <dup2>
     280:	e5950000 	ldr	r0, [r5]
     284:	e3500000 	cmp	r0, #0
     288:	da000028 	ble	330 <main+0x230>
     28c:	e3a01002 	mov	r1, #2
     290:	eb003245 	bl	cbac <dup2>
     294:	e5950000 	ldr	r0, [r5]
     298:	eb003631 	bl	db64 <close>
     29c:	e59f3130 	ldr	r3, [pc, #304]	; 3d4 <main+0x2d4>
     2a0:	e5853000 	str	r3, [r5]
     2a4:	e1a00004 	mov	r0, r4
     2a8:	eb00362d 	bl	db64 <close>
     2ac:	e59f0124 	ldr	r0, [pc, #292]	; 3d8 <main+0x2d8>
     2b0:	e08f0000 	add	r0, pc, r0
     2b4:	eb000100 	bl	6bc <load_devs>
     2b8:	e59f011c 	ldr	r0, [pc, #284]	; 3dc <main+0x2dc>
     2bc:	e3a01000 	mov	r1, #0
     2c0:	e08f0000 	add	r0, pc, r0
     2c4:	eb0035c1 	bl	d9d0 <open>
     2c8:	e2504000 	subs	r4, r0, #0
     2cc:	ba000011 	blt	318 <main+0x218>
     2d0:	e1a00004 	mov	r0, r4
     2d4:	eb00007c 	bl	4cc <read_line>
     2d8:	e2503000 	subs	r3, r0, #0
     2dc:	0a00000b 	beq	310 <main+0x210>
     2e0:	e5d31000 	ldrb	r1, [r3]
     2e4:	e3510000 	cmp	r1, #0
     2e8:	13510023 	cmpne	r1, #35	; 0x23
     2ec:	03a01001 	moveq	r1, #1
     2f0:	13a01000 	movne	r1, #0
     2f4:	0afffff5 	beq	2d0 <main+0x1d0>
     2f8:	e1a02001 	mov	r2, r1
     2fc:	eb000097 	bl	560 <run>
     300:	e1a00004 	mov	r0, r4
     304:	eb000070 	bl	4cc <read_line>
     308:	e2503000 	subs	r3, r0, #0
     30c:	1afffff3 	bne	2e0 <main+0x1e0>
     310:	e1a00004 	mov	r0, r4
     314:	eb003612 	bl	db64 <close>
     318:	e59f30c0 	ldr	r3, [pc, #192]	; 3e0 <main+0x2e0>
     31c:	e7964003 	ldr	r4, [r6, r3]
     320:	eb003289 	bl	cd4c <getpid>
     324:	e1a01004 	mov	r1, r4
     328:	eb0031ad 	bl	c9e4 <proc_block>
     32c:	eafffffb 	b	320 <main+0x220>
     330:	e1a00004 	mov	r0, r4
     334:	e3a01002 	mov	r1, #2
     338:	eb00321b 	bl	cbac <dup2>
     33c:	eaffffd8 	b	2a4 <main+0x1a4>
     340:	e59f109c 	ldr	r1, [pc, #156]	; 3e4 <main+0x2e4>
     344:	e08f1001 	add	r1, pc, r1
     348:	eb001dff 	bl	7b4c <syscall1>
     34c:	eb000b48 	bl	3074 <sdfsd_main>
     350:	eaffffa9 	b	1fc <main+0xfc>
     354:	eb003192 	bl	c9a4 <proc_wait_ready>
     358:	eaffffa7 	b	1fc <main+0xfc>
     35c:	e59f1084 	ldr	r1, [pc, #132]	; 3e8 <main+0x2e8>
     360:	e3a00017 	mov	r0, #23
     364:	e08f1001 	add	r1, pc, r1
     368:	eb001df7 	bl	7b4c <syscall1>
     36c:	eb0008e7 	bl	2710 <vfsd_main>
     370:	eaffff8d 	b	1ac <main+0xac>
     374:	e59f1070 	ldr	r1, [pc, #112]	; 3ec <main+0x2ec>
     378:	e3a00017 	mov	r0, #23
     37c:	e08f1001 	add	r1, pc, r1
     380:	eb001df1 	bl	7b4c <syscall1>
     384:	eb000281 	bl	d90 <core>
     388:	eaffff7d 	b	184 <main+0x84>
     38c:	0001793c 	andeq	r7, r1, ip, lsr r9
     390:	000178d0 	ldrdeq	r7, [r1], -r0
     394:	0000f0e0 	andeq	pc, r0, r0, ror #1
     398:	0000f0f0 	strdeq	pc, [r0], -r0
     39c:	0000f0fc 	strdeq	pc, [r0], -ip
     3a0:	0000f0f8 	strdeq	pc, [r0], -r8
     3a4:	0000f050 	andeq	pc, r0, r0, asr r0	; <UNPREDICTABLE>
     3a8:	0000f0f0 	strdeq	pc, [r0], -r0
     3ac:	0000f028 	andeq	pc, r0, r8, lsr #32
     3b0:	0000f0e8 	andeq	pc, r0, r8, ror #1
     3b4:	0000f0d8 	ldrdeq	pc, [r0], -r8
     3b8:	0000efd8 	ldrdeq	lr, [r0], -r8
     3bc:	0000f0b8 	strheq	pc, [r0], -r8	; <UNPREDICTABLE>
     3c0:	0000f0c0 	andeq	pc, r0, r0, asr #1
     3c4:	00017814 	andeq	r7, r1, r4, lsl r8
     3c8:	0000f0b4 	strheq	pc, [r0], -r4	; <UNPREDICTABLE>
     3cc:	0000f1d8 	ldrdeq	pc, [r0], -r8
     3d0:	000177e0 	andeq	r7, r1, r0, ror #15
     3d4:	fffffb2e 			; <UNDEFINED> instruction: 0xfffffb2e
     3d8:	0000f18c 	andeq	pc, r0, ip, lsl #3
     3dc:	0000f190 	muleq	r0, r0, r1
     3e0:	0000003c 	andeq	r0, r0, ip, lsr r0
     3e4:	0000ef78 	andeq	lr, r0, r8, ror pc
     3e8:	0000ef34 	andeq	lr, r0, r4, lsr pc
     3ec:	0000eefc 	strdeq	lr, [r0], -ip

000003f0 <out>:
     3f0:	e1a0c00d 	mov	ip, sp
     3f4:	e92d000f 	push	{r0, r1, r2, r3}
     3f8:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
     3fc:	e24cb014 	sub	fp, ip, #20
     400:	e24dd008 	sub	sp, sp, #8
     404:	e3a00000 	mov	r0, #0
     408:	eb001e1f 	bl	7c8c <str_new>
     40c:	e1a04000 	mov	r4, r0
     410:	e1a01000 	mov	r1, r0
     414:	e59f5078 	ldr	r5, [pc, #120]	; 494 <out+0xa4>
     418:	e59f0078 	ldr	r0, [pc, #120]	; 498 <out+0xa8>
     41c:	e28bc008 	add	ip, fp, #8
     420:	e59b2004 	ldr	r2, [fp, #4]
     424:	e1a0300c 	mov	r3, ip
     428:	e08f0000 	add	r0, pc, r0
     42c:	e08f5005 	add	r5, pc, r5
     430:	e50bc018 	str	ip, [fp, #-24]
     434:	eb003438 	bl	d51c <v_printf>
     438:	e1a00005 	mov	r0, r5
     43c:	e5941000 	ldr	r1, [r4]
     440:	eb001f9d 	bl	82bc <klog>
     444:	e59f3050 	ldr	r3, [pc, #80]	; 49c <out+0xac>
     448:	e08f3003 	add	r3, pc, r3
     44c:	e59f204c 	ldr	r2, [pc, #76]	; 4a0 <out+0xb0>
     450:	e5930000 	ldr	r0, [r3]
     454:	e1500002 	cmp	r0, r2
     458:	0a000008 	beq	480 <out+0x90>
     45c:	e3500000 	cmp	r0, #0
     460:	c1a01005 	movgt	r1, r5
     464:	c5942000 	ldrgt	r2, [r4]
     468:	cb00353b 	blgt	d95c <dprintf>
     46c:	e1a00004 	mov	r0, r4
     470:	eb001e78 	bl	7e58 <str_free>
     474:	e24bd014 	sub	sp, fp, #20
     478:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
     47c:	e12fff1e 	bx	lr
     480:	e1a01005 	mov	r1, r5
     484:	e3a00002 	mov	r0, #2
     488:	e5942000 	ldr	r2, [r4]
     48c:	eb003532 	bl	d95c <dprintf>
     490:	eafffff5 	b	46c <out+0x7c>
     494:	0000ed74 	andeq	lr, r0, r4, ror sp
     498:	00000074 	andeq	r0, r0, r4, ror r0
     49c:	00017608 	andeq	r7, r1, r8, lsl #12
     4a0:	fffffb2e 			; <UNDEFINED> instruction: 0xfffffb2e

000004a4 <outc>:
     4a4:	e1a0c00d 	mov	ip, sp
     4a8:	e1a03000 	mov	r3, r0
     4ac:	e92dd800 	push	{fp, ip, lr, pc}
     4b0:	e1a00001 	mov	r0, r1
     4b4:	e24cb004 	sub	fp, ip, #4
     4b8:	e1a01003 	mov	r1, r3
     4bc:	eb001e3b 	bl	7db0 <str_addc>
     4c0:	e24bd00c 	sub	sp, fp, #12
     4c4:	e89d6800 	ldm	sp, {fp, sp, lr}
     4c8:	e12fff1e 	bx	lr

000004cc <read_line>:
     4cc:	e1a0c00d 	mov	ip, sp
     4d0:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
     4d4:	e59f507c 	ldr	r5, [pc, #124]	; 558 <read_line+0x8c>
     4d8:	e24cb004 	sub	fp, ip, #4
     4dc:	e24dd008 	sub	sp, sp, #8
     4e0:	e08f5005 	add	r5, pc, r5
     4e4:	e1a07000 	mov	r7, r0
     4e8:	e3a04000 	mov	r4, #0
     4ec:	e24b601d 	sub	r6, fp, #29
     4f0:	e2455001 	sub	r5, r5, #1
     4f4:	ea000006 	b	514 <read_line+0x48>
     4f8:	e55b301d 	ldrb	r3, [fp, #-29]
     4fc:	e353000a 	cmp	r3, #10
     500:	0a00000c 	beq	538 <read_line+0x6c>
     504:	e2844001 	add	r4, r4, #1
     508:	e35400ff 	cmp	r4, #255	; 0xff
     50c:	e5e53001 	strb	r3, [r5, #1]!
     510:	0a000008 	beq	538 <read_line+0x6c>
     514:	e1a00007 	mov	r0, r7
     518:	e1a01006 	mov	r1, r6
     51c:	e3a02001 	mov	r2, #1
     520:	eb003211 	bl	cd6c <read>
     524:	e3500000 	cmp	r0, #0
     528:	cafffff2 	bgt	4f8 <read_line+0x2c>
     52c:	e3540000 	cmp	r4, #0
     530:	01a00004 	moveq	r0, r4
     534:	0a000004 	beq	54c <read_line+0x80>
     538:	e59f301c 	ldr	r3, [pc, #28]	; 55c <read_line+0x90>
     53c:	e3a02000 	mov	r2, #0
     540:	e08f3003 	add	r3, pc, r3
     544:	e1a00003 	mov	r0, r3
     548:	e7c32004 	strb	r2, [r3, r4]
     54c:	e24bd01c 	sub	sp, fp, #28
     550:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
     554:	e12fff1e 	bx	lr
     558:	00017578 	andeq	r7, r1, r8, ror r5
     55c:	00017518 	andeq	r7, r1, r8, lsl r5

00000560 <run>:
     560:	e1a0c00d 	mov	ip, sp
     564:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
     568:	e1a07000 	mov	r7, r0
     56c:	e59f0130 	ldr	r0, [pc, #304]	; 6a4 <run+0x144>
     570:	e24cb004 	sub	fp, ip, #4
     574:	e08f0000 	add	r0, pc, r0
     578:	e1a06001 	mov	r6, r1
     57c:	e1a08002 	mov	r8, r2
     580:	eb001dc1 	bl	7c8c <str_new>
     584:	e5d71000 	ldrb	r1, [r7]
     588:	e31100df 	tst	r1, #223	; 0xdf
     58c:	e1a05000 	mov	r5, r0
     590:	11a04007 	movne	r4, r7
     594:	0a000004 	beq	5ac <run+0x4c>
     598:	e1a00005 	mov	r0, r5
     59c:	eb001e03 	bl	7db0 <str_addc>
     5a0:	e5f41001 	ldrb	r1, [r4, #1]!
     5a4:	e31100df 	tst	r1, #223	; 0xdf
     5a8:	1afffffa 	bne	598 <run+0x38>
     5ac:	e3a01000 	mov	r1, #0
     5b0:	e1a00005 	mov	r0, r5
     5b4:	eb001dfd 	bl	7db0 <str_addc>
     5b8:	e5950000 	ldr	r0, [r5]
     5bc:	eb002150 	bl	8b04 <vfs_access>
     5c0:	e1a04000 	mov	r4, r0
     5c4:	e1a00005 	mov	r0, r5
     5c8:	eb001e22 	bl	7e58 <str_free>
     5cc:	e3540000 	cmp	r4, #0
     5d0:	1a00002d 	bne	68c <run+0x12c>
     5d4:	e3560000 	cmp	r6, #0
     5d8:	1a000020 	bne	660 <run+0x100>
     5dc:	eb0031a3 	bl	cc70 <fork>
     5e0:	e3500000 	cmp	r0, #0
     5e4:	1a000011 	bne	630 <run+0xd0>
     5e8:	e59f40b8 	ldr	r4, [pc, #184]	; 6a8 <run+0x148>
     5ec:	e08f4004 	add	r4, pc, r4
     5f0:	e5940000 	ldr	r0, [r4]
     5f4:	e3500000 	cmp	r0, #0
     5f8:	da000002 	ble	608 <run+0xa8>
     5fc:	eb003558 	bl	db64 <close>
     600:	e3e03000 	mvn	r3, #0
     604:	e5843000 	str	r3, [r4]
     608:	eb0030cc 	bl	c940 <proc_detach>
     60c:	e1a00007 	mov	r0, r7
     610:	eb00316c 	bl	cbc8 <exec>
     614:	e3500000 	cmp	r0, #0
     618:	0a000006 	beq	638 <run+0xd8>
     61c:	e3560000 	cmp	r6, #0
     620:	1a000013 	bne	674 <run+0x114>
     624:	e3e00000 	mvn	r0, #0
     628:	eb003315 	bl	d284 <exit>
     62c:	ea000001 	b	638 <run+0xd8>
     630:	e3580000 	cmp	r8, #0
     634:	1a000012 	bne	684 <run+0x124>
     638:	e3560000 	cmp	r6, #0
     63c:	01a00006 	moveq	r0, r6
     640:	0a000003 	beq	654 <run+0xf4>
     644:	e59f0060 	ldr	r0, [pc, #96]	; 6ac <run+0x14c>
     648:	e08f0000 	add	r0, pc, r0
     64c:	ebffff67 	bl	3f0 <out>
     650:	e3a00000 	mov	r0, #0
     654:	e24bd024 	sub	sp, fp, #36	; 0x24
     658:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
     65c:	e12fff1e 	bx	lr
     660:	e59f0048 	ldr	r0, [pc, #72]	; 6b0 <run+0x150>
     664:	e1a01007 	mov	r1, r7
     668:	e08f0000 	add	r0, pc, r0
     66c:	ebffff5f 	bl	3f0 <out>
     670:	eaffffd9 	b	5dc <run+0x7c>
     674:	e59f0038 	ldr	r0, [pc, #56]	; 6b4 <run+0x154>
     678:	e08f0000 	add	r0, pc, r0
     67c:	ebffff5b 	bl	3f0 <out>
     680:	eaffffe7 	b	624 <run+0xc4>
     684:	eb0030c6 	bl	c9a4 <proc_wait_ready>
     688:	eaffffea 	b	638 <run+0xd8>
     68c:	e59f0024 	ldr	r0, [pc, #36]	; 6b8 <run+0x158>
     690:	e1a01007 	mov	r1, r7
     694:	e08f0000 	add	r0, pc, r0
     698:	ebffff54 	bl	3f0 <out>
     69c:	e3e00000 	mvn	r0, #0
     6a0:	eaffffeb 	b	654 <run+0xf4>
     6a4:	0000ef78 	andeq	lr, r0, r8, ror pc
     6a8:	00017464 	andeq	r7, r1, r4, ror #8
     6ac:	0000eb90 	muleq	r0, r0, fp
     6b0:	0000eb58 	andeq	lr, r0, r8, asr fp
     6b4:	0000eb54 	andeq	lr, r0, r4, asr fp
     6b8:	0000eb10 	andeq	lr, r0, r0, lsl fp

000006bc <load_devs>:
     6bc:	e1a0c00d 	mov	ip, sp
     6c0:	e3a01000 	mov	r1, #0
     6c4:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
     6c8:	e24cb004 	sub	fp, ip, #4
     6cc:	eb0034bf 	bl	d9d0 <open>
     6d0:	e2504000 	subs	r4, r0, #0
     6d4:	ba000010 	blt	71c <load_devs+0x60>
     6d8:	e1a00004 	mov	r0, r4
     6dc:	ebffff7a 	bl	4cc <read_line>
     6e0:	e2503000 	subs	r3, r0, #0
     6e4:	0a00000a 	beq	714 <load_devs+0x58>
     6e8:	e5d33000 	ldrb	r3, [r3]
     6ec:	e3530000 	cmp	r3, #0
     6f0:	13530023 	cmpne	r3, #35	; 0x23
     6f4:	0afffff7 	beq	6d8 <load_devs+0x1c>
     6f8:	e3a01001 	mov	r1, #1
     6fc:	e1a02001 	mov	r2, r1
     700:	ebffff96 	bl	560 <run>
     704:	e1a00004 	mov	r0, r4
     708:	ebffff6f 	bl	4cc <read_line>
     70c:	e2503000 	subs	r3, r0, #0
     710:	1afffff4 	bne	6e8 <load_devs+0x2c>
     714:	e1a00004 	mov	r0, r4
     718:	eb003511 	bl	db64 <close>
     71c:	e24bd014 	sub	sp, fp, #20
     720:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
     724:	e12fff1e 	bx	lr

00000728 <load_arch_devs>:
     728:	e1a0c00d 	mov	ip, sp
     72c:	e92dd810 	push	{r4, fp, ip, lr, pc}
     730:	e24cb004 	sub	fp, ip, #4
     734:	e24ddf91 	sub	sp, sp, #580	; 0x244
     738:	e24b4f95 	sub	r4, fp, #596	; 0x254
     73c:	e1a01004 	mov	r1, r4
     740:	e3a0001e 	mov	r0, #30
     744:	eb001d00 	bl	7b4c <syscall1>
     748:	e59f2028 	ldr	r2, [pc, #40]	; 778 <load_arch_devs+0x50>
     74c:	e1a03004 	mov	r3, r4
     750:	e24b4f85 	sub	r4, fp, #532	; 0x214
     754:	e59f1020 	ldr	r1, [pc, #32]	; 77c <load_arch_devs+0x54>
     758:	e08f2002 	add	r2, pc, r2
     75c:	e1a00004 	mov	r0, r4
     760:	eb00345a 	bl	d8d0 <snprintf>
     764:	e1a00004 	mov	r0, r4
     768:	ebffffd3 	bl	6bc <load_devs>
     76c:	e24bd010 	sub	sp, fp, #16
     770:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
     774:	e12fff1e 	bx	lr
     778:	0000ea88 	andeq	lr, r0, r8, lsl #21
     77c:	000001ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>

00000780 <load_extra_devs>:
     780:	e1a0c00d 	mov	ip, sp
     784:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
     788:	e59f0074 	ldr	r0, [pc, #116]	; 804 <load_extra_devs+0x84>
     78c:	e24cb004 	sub	fp, ip, #4
     790:	e24ddf82 	sub	sp, sp, #520	; 0x208
     794:	e08f0000 	add	r0, pc, r0
     798:	eb0031f5 	bl	cf74 <opendir>
     79c:	e2505000 	subs	r5, r0, #0
     7a0:	0a000014 	beq	7f8 <load_extra_devs+0x78>
     7a4:	e59f705c 	ldr	r7, [pc, #92]	; 808 <load_extra_devs+0x88>
     7a8:	e59f605c 	ldr	r6, [pc, #92]	; 80c <load_extra_devs+0x8c>
     7ac:	e08f7007 	add	r7, pc, r7
     7b0:	e08f6006 	add	r6, pc, r6
     7b4:	e24b4f87 	sub	r4, fp, #540	; 0x21c
     7b8:	ea000006 	b	7d8 <load_extra_devs+0x58>
     7bc:	e58d0000 	str	r0, [sp]
     7c0:	e1a02007 	mov	r2, r7
     7c4:	e1a00004 	mov	r0, r4
     7c8:	e1a03006 	mov	r3, r6
     7cc:	eb00343f 	bl	d8d0 <snprintf>
     7d0:	e1a00004 	mov	r0, r4
     7d4:	ebffffb8 	bl	6bc <load_devs>
     7d8:	e1a00005 	mov	r0, r5
     7dc:	eb003218 	bl	d044 <readdir>
     7e0:	e3500000 	cmp	r0, #0
     7e4:	e59f1024 	ldr	r1, [pc, #36]	; 810 <load_extra_devs+0x90>
     7e8:	e280000b 	add	r0, r0, #11
     7ec:	1afffff2 	bne	7bc <load_extra_devs+0x3c>
     7f0:	e1a00005 	mov	r0, r5
     7f4:	eb003202 	bl	d004 <closedir>
     7f8:	e24bd01c 	sub	sp, fp, #28
     7fc:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
     800:	e12fff1e 	bx	lr
     804:	0000ea68 	andeq	lr, r0, r8, ror #20
     808:	0000ea60 	andeq	lr, r0, r0, ror #20
     80c:	0000ea4c 	andeq	lr, r0, ip, asr #20
     810:	000001ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>

00000814 <get_envs>:
     814:	e1a0c00d 	mov	ip, sp
     818:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
     81c:	e24cb004 	sub	fp, ip, #4
     820:	e1a04002 	mov	r4, r2
     824:	e1a05001 	mov	r5, r1
     828:	e1a06000 	mov	r6, r0
     82c:	eb002d05 	bl	bc48 <get_proto_factor>
     830:	e1a01006 	mov	r1, r6
     834:	e5903018 	ldr	r3, [r0, #24]
     838:	e1a00004 	mov	r0, r4
     83c:	e1a0e00f 	mov	lr, pc
     840:	e12fff13 	bx	r3
     844:	e5951000 	ldr	r1, [r5]
     848:	e5903018 	ldr	r3, [r0, #24]
     84c:	e1a00004 	mov	r0, r4
     850:	e1a0e00f 	mov	lr, pc
     854:	e12fff13 	bx	r3
     858:	e3a00000 	mov	r0, #0
     85c:	e24bd01c 	sub	sp, fp, #28
     860:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
     864:	e12fff1e 	bx	lr

00000868 <get_ipc_serv>:
     868:	e1a0c00d 	mov	ip, sp
     86c:	e92dd800 	push	{fp, ip, lr, pc}
     870:	e24cb004 	sub	fp, ip, #4
     874:	e24dd008 	sub	sp, sp, #8
     878:	e59f3028 	ldr	r3, [pc, #40]	; 8a8 <get_ipc_serv+0x40>
     87c:	e1a01000 	mov	r1, r0
     880:	e24b2010 	sub	r2, fp, #16
     884:	e79f0003 	ldr	r0, [pc, r3]
     888:	eb001bd9 	bl	77f4 <hashmap_get>
     88c:	e3700003 	cmn	r0, #3
     890:	03e00000 	mvneq	r0, #0
     894:	151b3010 	ldrne	r3, [fp, #-16]
     898:	15930000 	ldrne	r0, [r3]
     89c:	e24bd00c 	sub	sp, fp, #12
     8a0:	e89d6800 	ldm	sp, {fp, sp, lr}
     8a4:	e12fff1e 	bx	lr
     8a8:	000172d4 	ldrdeq	r7, [r1], -r4

000008ac <free_envs>:
     8ac:	e1a0c00d 	mov	ip, sp
     8b0:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
     8b4:	e1a03000 	mov	r3, r0
     8b8:	e1a04001 	mov	r4, r1
     8bc:	e24cb004 	sub	fp, ip, #4
     8c0:	e1a00002 	mov	r0, r2
     8c4:	e1a01003 	mov	r1, r3
     8c8:	eb001bef 	bl	788c <hashmap_remove>
     8cc:	e1a00004 	mov	r0, r4
     8d0:	eb001d60 	bl	7e58 <str_free>
     8d4:	e3a00000 	mov	r0, #0
     8d8:	e24bd014 	sub	sp, fp, #20
     8dc:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
     8e0:	e12fff1e 	bx	lr

000008e4 <set_env>:
     8e4:	e1a0c00d 	mov	ip, sp
     8e8:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
     8ec:	e24cb004 	sub	fp, ip, #4
     8f0:	e3a0c000 	mov	ip, #0
     8f4:	e24b301c 	sub	r3, fp, #28
     8f8:	e24dd00c 	sub	sp, sp, #12
     8fc:	e523c004 	str	ip, [r3, #-4]!
     900:	e1a04002 	mov	r4, r2
     904:	e1a02003 	mov	r2, r3
     908:	e1a06000 	mov	r6, r0
     90c:	e1a05001 	mov	r5, r1
     910:	eb001bb7 	bl	77f4 <hashmap_get>
     914:	e3500000 	cmp	r0, #0
     918:	0a000008 	beq	940 <set_env+0x5c>
     91c:	e1a00004 	mov	r0, r4
     920:	eb001cd9 	bl	7c8c <str_new>
     924:	e1a01005 	mov	r1, r5
     928:	e1a02000 	mov	r2, r0
     92c:	e1a00006 	mov	r0, r6
     930:	eb001b59 	bl	769c <hashmap_put>
     934:	e24bd018 	sub	sp, fp, #24
     938:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
     93c:	e12fff1e 	bx	lr
     940:	e51b0020 	ldr	r0, [fp, #-32]
     944:	e3500000 	cmp	r0, #0
     948:	0afffff3 	beq	91c <set_env+0x38>
     94c:	e1a01004 	mov	r1, r4
     950:	eb001cc2 	bl	7c60 <str_cpy>
     954:	e24bd018 	sub	sp, fp, #24
     958:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
     95c:	e12fff1e 	bx	lr

00000960 <handle_ipc>:
     960:	e1a0c00d 	mov	ip, sp
     964:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
     968:	e24cb004 	sub	fp, ip, #4
     96c:	e24dd008 	sub	sp, sp, #8
     970:	e1a04001 	mov	r4, r1
     974:	e1a07002 	mov	r7, r2
     978:	e1a05003 	mov	r5, r3
     97c:	eb002fe6 	bl	c91c <proc_getpid>
     980:	e1a06000 	mov	r6, r0
     984:	e3540007 	cmp	r4, #7
     988:	908ff104 	addls	pc, pc, r4, lsl #2
     98c:	ea00002f 	b	a50 <handle_ipc+0xf0>
     990:	ea00004e 	b	ad0 <handle_ipc+0x170>
     994:	ea000066 	b	b34 <handle_ipc+0x1d4>
     998:	ea000083 	b	bac <handle_ipc+0x24c>
     99c:	ea000091 	b	be8 <handle_ipc+0x288>
     9a0:	ea0000a1 	b	c2c <handle_ipc+0x2cc>
     9a4:	ea0000bc 	b	c9c <handle_ipc+0x33c>
     9a8:	ea000000 	b	9b0 <handle_ipc+0x50>
     9ac:	ea00002a 	b	a5c <handle_ipc+0xfc>
     9b0:	eb002ca4 	bl	bc48 <get_proto_factor>
     9b4:	e3e01000 	mvn	r1, #0
     9b8:	e5903014 	ldr	r3, [r0, #20]
     9bc:	e1a00005 	mov	r0, r5
     9c0:	e1a0e00f 	mov	lr, pc
     9c4:	e12fff13 	bx	r3
     9c8:	e356007f 	cmp	r6, #127	; 0x7f
     9cc:	8a00001f 	bhi	a50 <handle_ipc+0xf0>
     9d0:	e1a00007 	mov	r0, r7
     9d4:	eb002d86 	bl	bff4 <proto_read_str>
     9d8:	e1a01000 	mov	r1, r0
     9dc:	e3a00000 	mov	r0, #0
     9e0:	e59f3354 	ldr	r3, [pc, #852]	; d3c <handle_ipc+0x3dc>
     9e4:	e08f3003 	add	r3, pc, r3
     9e8:	e0836186 	add	r6, r3, r6, lsl #3
     9ec:	e5963008 	ldr	r3, [r6, #8]
     9f0:	e24b201c 	sub	r2, fp, #28
     9f4:	e5220004 	str	r0, [r2, #-4]!
     9f8:	e1a00003 	mov	r0, r3
     9fc:	eb001b7c 	bl	77f4 <hashmap_get>
     a00:	e2504000 	subs	r4, r0, #0
     a04:	1a000011 	bne	a50 <handle_ipc+0xf0>
     a08:	e51b6020 	ldr	r6, [fp, #-32]
     a0c:	e3560000 	cmp	r6, #0
     a10:	0a00000e 	beq	a50 <handle_ipc+0xf0>
     a14:	eb002c8b 	bl	bc48 <get_proto_factor>
     a18:	e590300c 	ldr	r3, [r0, #12]
     a1c:	e1a00005 	mov	r0, r5
     a20:	e1a0e00f 	mov	lr, pc
     a24:	e12fff13 	bx	r3
     a28:	e1a01004 	mov	r1, r4
     a2c:	e5903014 	ldr	r3, [r0, #20]
     a30:	e1a00005 	mov	r0, r5
     a34:	e1a0e00f 	mov	lr, pc
     a38:	e12fff13 	bx	r3
     a3c:	e5961000 	ldr	r1, [r6]
     a40:	e5903018 	ldr	r3, [r0, #24]
     a44:	e1a00005 	mov	r0, r5
     a48:	e1a0e00f 	mov	lr, pc
     a4c:	e12fff13 	bx	r3
     a50:	e24bd01c 	sub	sp, fp, #28
     a54:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
     a58:	e12fff1e 	bx	lr
     a5c:	eb002c79 	bl	bc48 <get_proto_factor>
     a60:	e3e01000 	mvn	r1, #0
     a64:	e5903014 	ldr	r3, [r0, #20]
     a68:	e1a00005 	mov	r0, r5
     a6c:	e1a0e00f 	mov	lr, pc
     a70:	e12fff13 	bx	r3
     a74:	e356007f 	cmp	r6, #127	; 0x7f
     a78:	8afffff4 	bhi	a50 <handle_ipc+0xf0>
     a7c:	eb002c71 	bl	bc48 <get_proto_factor>
     a80:	e590300c 	ldr	r3, [r0, #12]
     a84:	e1a00005 	mov	r0, r5
     a88:	e1a0e00f 	mov	lr, pc
     a8c:	e12fff13 	bx	r3
     a90:	e59f32a8 	ldr	r3, [pc, #680]	; d40 <handle_ipc+0x3e0>
     a94:	e08f3003 	add	r3, pc, r3
     a98:	e0836186 	add	r6, r3, r6, lsl #3
     a9c:	e5904014 	ldr	r4, [r0, #20]
     aa0:	e5960008 	ldr	r0, [r6, #8]
     aa4:	eb001be5 	bl	7a40 <hashmap_length>
     aa8:	e1a01000 	mov	r1, r0
     aac:	e1a00005 	mov	r0, r5
     ab0:	e1a0e00f 	mov	lr, pc
     ab4:	e12fff14 	bx	r4
     ab8:	e59f1284 	ldr	r1, [pc, #644]	; d44 <handle_ipc+0x3e4>
     abc:	e5960008 	ldr	r0, [r6, #8]
     ac0:	e1a02005 	mov	r2, r5
     ac4:	e08f1001 	add	r1, pc, r1
     ac8:	eb001bdf 	bl	7a4c <hashmap_iterate>
     acc:	eaffffdf 	b	a50 <handle_ipc+0xf0>
     ad0:	e1a00007 	mov	r0, r7
     ad4:	eb002d46 	bl	bff4 <proto_read_str>
     ad8:	e5d03000 	ldrb	r3, [r0]
     adc:	e3530000 	cmp	r3, #0
     ae0:	e1a04000 	mov	r4, r0
     ae4:	0a00008b 	beq	d18 <handle_ipc+0x3b8>
     ae8:	e3a00004 	mov	r0, #4
     aec:	eb0031f6 	bl	d2cc <malloc>
     af0:	e1a0c000 	mov	ip, r0
     af4:	e59f324c 	ldr	r3, [pc, #588]	; d48 <handle_ipc+0x3e8>
     af8:	e08f3003 	add	r3, pc, r3
     afc:	e1a02000 	mov	r2, r0
     b00:	e1a01004 	mov	r1, r4
     b04:	e5930000 	ldr	r0, [r3]
     b08:	e58c6000 	str	r6, [ip]
     b0c:	eb001ae2 	bl	769c <hashmap_put>
     b10:	e2504000 	subs	r4, r0, #0
     b14:	1a00007f 	bne	d18 <handle_ipc+0x3b8>
     b18:	eb002c4a 	bl	bc48 <get_proto_factor>
     b1c:	e1a01004 	mov	r1, r4
     b20:	e5903014 	ldr	r3, [r0, #20]
     b24:	e1a00005 	mov	r0, r5
     b28:	e1a0e00f 	mov	lr, pc
     b2c:	e12fff13 	bx	r3
     b30:	eaffffc6 	b	a50 <handle_ipc+0xf0>
     b34:	e1a00007 	mov	r0, r7
     b38:	eb002d2d 	bl	bff4 <proto_read_str>
     b3c:	e5d03000 	ldrb	r3, [r0]
     b40:	e3530000 	cmp	r3, #0
     b44:	e1a04000 	mov	r4, r0
     b48:	0a000072 	beq	d18 <handle_ipc+0x3b8>
     b4c:	e59f71f8 	ldr	r7, [pc, #504]	; d4c <handle_ipc+0x3ec>
     b50:	e08f7007 	add	r7, pc, r7
     b54:	e1a01000 	mov	r1, r0
     b58:	e24b2020 	sub	r2, fp, #32
     b5c:	e5970000 	ldr	r0, [r7]
     b60:	eb001b23 	bl	77f4 <hashmap_get>
     b64:	e3700003 	cmn	r0, #3
     b68:	0a00006a 	beq	d18 <handle_ipc+0x3b8>
     b6c:	e51b3020 	ldr	r3, [fp, #-32]
     b70:	e5933000 	ldr	r3, [r3]
     b74:	e1560003 	cmp	r6, r3
     b78:	1a000066 	bne	d18 <handle_ipc+0x3b8>
     b7c:	e1a01004 	mov	r1, r4
     b80:	e5970000 	ldr	r0, [r7]
     b84:	eb001b40 	bl	788c <hashmap_remove>
     b88:	e51b0020 	ldr	r0, [fp, #-32]
     b8c:	eb0031c5 	bl	d2a8 <free>
     b90:	eb002c2c 	bl	bc48 <get_proto_factor>
     b94:	e3a01000 	mov	r1, #0
     b98:	e5903014 	ldr	r3, [r0, #20]
     b9c:	e1a00005 	mov	r0, r5
     ba0:	e1a0e00f 	mov	lr, pc
     ba4:	e12fff13 	bx	r3
     ba8:	eaffffa8 	b	a50 <handle_ipc+0xf0>
     bac:	e1a00007 	mov	r0, r7
     bb0:	eb002d0f 	bl	bff4 <proto_read_str>
     bb4:	e5d03000 	ldrb	r3, [r0]
     bb8:	e3530000 	cmp	r3, #0
     bbc:	e1a04000 	mov	r4, r0
     bc0:	0a000054 	beq	d18 <handle_ipc+0x3b8>
     bc4:	eb002c1f 	bl	bc48 <get_proto_factor>
     bc8:	e5906014 	ldr	r6, [r0, #20]
     bcc:	e1a00004 	mov	r0, r4
     bd0:	ebffff24 	bl	868 <get_ipc_serv>
     bd4:	e1a01000 	mov	r1, r0
     bd8:	e1a00005 	mov	r0, r5
     bdc:	e1a0e00f 	mov	lr, pc
     be0:	e12fff16 	bx	r6
     be4:	eaffff99 	b	a50 <handle_ipc+0xf0>
     be8:	eb002c16 	bl	bc48 <get_proto_factor>
     bec:	e3e01000 	mvn	r1, #0
     bf0:	e5903014 	ldr	r3, [r0, #20]
     bf4:	e1a00005 	mov	r0, r5
     bf8:	e1a0e00f 	mov	lr, pc
     bfc:	e12fff13 	bx	r3
     c00:	e1a00007 	mov	r0, r7
     c04:	eb002cfa 	bl	bff4 <proto_read_str>
     c08:	e356007f 	cmp	r6, #127	; 0x7f
     c0c:	8affff8f 	bhi	a50 <handle_ipc+0xf0>
     c10:	e59f3138 	ldr	r3, [pc, #312]	; d50 <handle_ipc+0x3f0>
     c14:	e08f3003 	add	r3, pc, r3
     c18:	e0836186 	add	r6, r3, r6, lsl #3
     c1c:	e1a01000 	mov	r1, r0
     c20:	e5960004 	ldr	r0, [r6, #4]
     c24:	eb001c0d 	bl	7c60 <str_cpy>
     c28:	ea00002f 	b	cec <handle_ipc+0x38c>
     c2c:	eb002c05 	bl	bc48 <get_proto_factor>
     c30:	e3e01000 	mvn	r1, #0
     c34:	e5903014 	ldr	r3, [r0, #20]
     c38:	e1a00005 	mov	r0, r5
     c3c:	e1a0e00f 	mov	lr, pc
     c40:	e12fff13 	bx	r3
     c44:	e356007f 	cmp	r6, #127	; 0x7f
     c48:	8affff80 	bhi	a50 <handle_ipc+0xf0>
     c4c:	eb002bfd 	bl	bc48 <get_proto_factor>
     c50:	e590300c 	ldr	r3, [r0, #12]
     c54:	e1a00005 	mov	r0, r5
     c58:	e1a0e00f 	mov	lr, pc
     c5c:	e12fff13 	bx	r3
     c60:	e3a01000 	mov	r1, #0
     c64:	e5903014 	ldr	r3, [r0, #20]
     c68:	e1a00005 	mov	r0, r5
     c6c:	e1a0e00f 	mov	lr, pc
     c70:	e12fff13 	bx	r3
     c74:	e59f30d8 	ldr	r3, [pc, #216]	; d54 <handle_ipc+0x3f4>
     c78:	e08f3003 	add	r3, pc, r3
     c7c:	e0836186 	add	r6, r3, r6, lsl #3
     c80:	e5962004 	ldr	r2, [r6, #4]
     c84:	e5903018 	ldr	r3, [r0, #24]
     c88:	e5921000 	ldr	r1, [r2]
     c8c:	e1a00005 	mov	r0, r5
     c90:	e1a0e00f 	mov	lr, pc
     c94:	e12fff13 	bx	r3
     c98:	eaffff6c 	b	a50 <handle_ipc+0xf0>
     c9c:	eb002be9 	bl	bc48 <get_proto_factor>
     ca0:	e3e01000 	mvn	r1, #0
     ca4:	e5903014 	ldr	r3, [r0, #20]
     ca8:	e1a00005 	mov	r0, r5
     cac:	e1a0e00f 	mov	lr, pc
     cb0:	e12fff13 	bx	r3
     cb4:	e356007f 	cmp	r6, #127	; 0x7f
     cb8:	8affff64 	bhi	a50 <handle_ipc+0xf0>
     cbc:	e1a00007 	mov	r0, r7
     cc0:	eb002ccb 	bl	bff4 <proto_read_str>
     cc4:	e1a04000 	mov	r4, r0
     cc8:	e1a00007 	mov	r0, r7
     ccc:	eb002cc8 	bl	bff4 <proto_read_str>
     cd0:	e59f3080 	ldr	r3, [pc, #128]	; d58 <handle_ipc+0x3f8>
     cd4:	e08f3003 	add	r3, pc, r3
     cd8:	e0836186 	add	r6, r3, r6, lsl #3
     cdc:	e1a02000 	mov	r2, r0
     ce0:	e1a01004 	mov	r1, r4
     ce4:	e5960008 	ldr	r0, [r6, #8]
     ce8:	ebfffefd 	bl	8e4 <set_env>
     cec:	eb002bd5 	bl	bc48 <get_proto_factor>
     cf0:	e590300c 	ldr	r3, [r0, #12]
     cf4:	e1a00005 	mov	r0, r5
     cf8:	e1a0e00f 	mov	lr, pc
     cfc:	e12fff13 	bx	r3
     d00:	e3a01000 	mov	r1, #0
     d04:	e5903014 	ldr	r3, [r0, #20]
     d08:	e1a00005 	mov	r0, r5
     d0c:	e1a0e00f 	mov	lr, pc
     d10:	e12fff13 	bx	r3
     d14:	eaffff4d 	b	a50 <handle_ipc+0xf0>
     d18:	eb002bca 	bl	bc48 <get_proto_factor>
     d1c:	e3e01000 	mvn	r1, #0
     d20:	e5903014 	ldr	r3, [r0, #20]
     d24:	e1a00005 	mov	r0, r5
     d28:	e1a0e00f 	mov	lr, pc
     d2c:	e12fff13 	bx	r3
     d30:	e24bd01c 	sub	sp, fp, #28
     d34:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
     d38:	e12fff1e 	bx	lr
     d3c:	00017174 	andeq	r7, r1, r4, ror r1
     d40:	000170c4 	andeq	r7, r1, r4, asr #1
     d44:	fffffd48 			; <UNDEFINED> instruction: 0xfffffd48
     d48:	00017060 	andeq	r7, r1, r0, rrx
     d4c:	00017008 	andeq	r7, r1, r8
     d50:	00016f44 	andeq	r6, r1, r4, asr #30
     d54:	00016ee0 	andeq	r6, r1, r0, ror #29
     d58:	00016e84 	andeq	r6, r1, r4, lsl #29

00000d5c <copy_envs>:
     d5c:	e1a0c00d 	mov	ip, sp
     d60:	e92dd800 	push	{fp, ip, lr, pc}
     d64:	e24cb004 	sub	fp, ip, #4
     d68:	e1a0c000 	mov	ip, r0
     d6c:	e5913000 	ldr	r3, [r1]
     d70:	e1a00002 	mov	r0, r2
     d74:	e1a0100c 	mov	r1, ip
     d78:	e1a02003 	mov	r2, r3
     d7c:	ebfffed8 	bl	8e4 <set_env>
     d80:	e3a00000 	mov	r0, #0
     d84:	e24bd00c 	sub	sp, fp, #12
     d88:	e89d6800 	ldm	sp, {fp, sp, lr}
     d8c:	e12fff1e 	bx	lr

00000d90 <core>:
     d90:	e1a0c00d 	mov	ip, sp
     d94:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
     d98:	e24cb004 	sub	fp, ip, #4
     d9c:	e24dd02c 	sub	sp, sp, #44	; 0x2c
     da0:	e59f4210 	ldr	r4, [pc, #528]	; fb8 <core+0x228>
     da4:	e59f6210 	ldr	r6, [pc, #528]	; fbc <core+0x22c>
     da8:	e08f4004 	add	r4, pc, r4
     dac:	e2844004 	add	r4, r4, #4
     db0:	e08f6006 	add	r6, pc, r6
     db4:	e2845b01 	add	r5, r4, #1024	; 0x400
     db8:	e1a00006 	mov	r0, r6
     dbc:	eb001bb2 	bl	7c8c <str_new>
     dc0:	e5840000 	str	r0, [r4]
     dc4:	eb001b00 	bl	79cc <hashmap_new>
     dc8:	e2844008 	add	r4, r4, #8
     dcc:	e5040004 	str	r0, [r4, #-4]
     dd0:	e1540005 	cmp	r4, r5
     dd4:	1afffff7 	bne	db8 <core+0x28>
     dd8:	eb001afb 	bl	79cc <hashmap_new>
     ddc:	e59f31dc 	ldr	r3, [pc, #476]	; fc0 <core+0x230>
     de0:	e08f3003 	add	r3, pc, r3
     de4:	e5830000 	str	r0, [r3]
     de8:	e59f01d4 	ldr	r0, [pc, #468]	; fc4 <core+0x234>
     dec:	e3a01000 	mov	r1, #0
     df0:	e3a02001 	mov	r2, #1
     df4:	e08f0000 	add	r0, pc, r0
     df8:	eb00273d 	bl	aaf4 <ipc_serv_run>
     dfc:	e3a0002b 	mov	r0, #43	; 0x2b
     e00:	eb001b54 	bl	7b58 <syscall0>
     e04:	e59f31bc 	ldr	r3, [pc, #444]	; fc8 <core+0x238>
     e08:	e08f3003 	add	r3, pc, r3
     e0c:	e50b3048 	str	r3, [fp, #-72]	; 0x48
     e10:	e59f31b4 	ldr	r3, [pc, #436]	; fcc <core+0x23c>
     e14:	e08f3003 	add	r3, pc, r3
     e18:	e50b304c 	str	r3, [fp, #-76]	; 0x4c
     e1c:	e59f31ac 	ldr	r3, [pc, #428]	; fd0 <core+0x240>
     e20:	e59f81ac 	ldr	r8, [pc, #428]	; fd4 <core+0x244>
     e24:	e59f71ac 	ldr	r7, [pc, #428]	; fd8 <core+0x248>
     e28:	e08f3003 	add	r3, pc, r3
     e2c:	e08f8008 	add	r8, pc, r8
     e30:	e08f7007 	add	r7, pc, r7
     e34:	e50b3050 	str	r3, [fp, #-80]	; 0x50
     e38:	e24b5044 	sub	r5, fp, #68	; 0x44
     e3c:	ea000029 	b	ee8 <core+0x158>
     e40:	e3530003 	cmp	r3, #3
     e44:	1a000025 	bne	ee0 <core+0x150>
     e48:	e5949008 	ldr	r9, [r4, #8]
     e4c:	eb002b7d 	bl	bc48 <get_proto_factor>
     e50:	e5903004 	ldr	r3, [r0, #4]
     e54:	e1a00005 	mov	r0, r5
     e58:	e1a0e00f 	mov	lr, pc
     e5c:	e12fff13 	bx	r3
     e60:	e5941004 	ldr	r1, [r4, #4]
     e64:	e5903014 	ldr	r3, [r0, #20]
     e68:	e1a00005 	mov	r0, r5
     e6c:	e1a0e00f 	mov	lr, pc
     e70:	e12fff13 	bx	r3
     e74:	e5941008 	ldr	r1, [r4, #8]
     e78:	e5903014 	ldr	r3, [r0, #20]
     e7c:	e1a00005 	mov	r0, r5
     e80:	e1a0e00f 	mov	lr, pc
     e84:	e12fff13 	bx	r3
     e88:	e1a00007 	mov	r0, r7
     e8c:	ebfffe75 	bl	868 <get_ipc_serv>
     e90:	e3500000 	cmp	r0, #0
     e94:	c3a01015 	movgt	r1, #21
     e98:	c1a02005 	movgt	r2, r5
     e9c:	c3a03000 	movgt	r3, #0
     ea0:	cb0025e2 	blgt	a630 <ipc_call_wait>
     ea4:	e1a00005 	mov	r0, r5
     ea8:	eb002c2f 	bl	bf6c <proto_read_int>
     eac:	e1a06000 	mov	r6, r0
     eb0:	e1a00005 	mov	r0, r5
     eb4:	eb002c2c 	bl	bf6c <proto_read_int>
     eb8:	e356007f 	cmp	r6, #127	; 0x7f
     ebc:	9350007f 	cmpls	r0, #127	; 0x7f
     ec0:	9a000028 	bls	f68 <core+0x1d8>
     ec4:	eb002b5f 	bl	bc48 <get_proto_factor>
     ec8:	e590300c 	ldr	r3, [r0, #12]
     ecc:	e1a00005 	mov	r0, r5
     ed0:	e1a0e00f 	mov	lr, pc
     ed4:	e12fff13 	bx	r3
     ed8:	e1a00009 	mov	r0, r9
     edc:	eb002ecb 	bl	ca10 <proc_wakeup>
     ee0:	e1a00004 	mov	r0, r4
     ee4:	eb0030ef 	bl	d2a8 <free>
     ee8:	e3a0002d 	mov	r0, #45	; 0x2d
     eec:	eb001b19 	bl	7b58 <syscall0>
     ef0:	e2504000 	subs	r4, r0, #0
     ef4:	0a000019 	beq	f60 <core+0x1d0>
     ef8:	e5943000 	ldr	r3, [r4]
     efc:	e3530002 	cmp	r3, #2
     f00:	1affffce 	bne	e40 <core+0xb0>
     f04:	eb002b4f 	bl	bc48 <get_proto_factor>
     f08:	e5903004 	ldr	r3, [r0, #4]
     f0c:	e1a00005 	mov	r0, r5
     f10:	e1a0e00f 	mov	lr, pc
     f14:	e12fff13 	bx	r3
     f18:	e5941004 	ldr	r1, [r4, #4]
     f1c:	e5903014 	ldr	r3, [r0, #20]
     f20:	e1a00005 	mov	r0, r5
     f24:	e1a0e00f 	mov	lr, pc
     f28:	e12fff13 	bx	r3
     f2c:	e1a00008 	mov	r0, r8
     f30:	ebfffe4c 	bl	868 <get_ipc_serv>
     f34:	e3500000 	cmp	r0, #0
     f38:	c3a01016 	movgt	r1, #22
     f3c:	c1a02005 	movgt	r2, r5
     f40:	c3a03000 	movgt	r3, #0
     f44:	cb0025b9 	blgt	a630 <ipc_call_wait>
     f48:	eb002b3e 	bl	bc48 <get_proto_factor>
     f4c:	e590300c 	ldr	r3, [r0, #12]
     f50:	e1a00005 	mov	r0, r5
     f54:	e1a0e00f 	mov	lr, pc
     f58:	e12fff13 	bx	r3
     f5c:	eaffffdf 	b	ee0 <core+0x150>
     f60:	eb002fe6 	bl	cf00 <sleep>
     f64:	eaffffdf 	b	ee8 <core+0x158>
     f68:	e51b2048 	ldr	r2, [fp, #-72]	; 0x48
     f6c:	e082a186 	add	sl, r2, r6, lsl #3
     f70:	e59a3004 	ldr	r3, [sl, #4]
     f74:	e0826180 	add	r6, r2, r0, lsl #3
     f78:	e5931000 	ldr	r1, [r3]
     f7c:	e5960004 	ldr	r0, [r6, #4]
     f80:	eb001b36 	bl	7c60 <str_cpy>
     f84:	e5960008 	ldr	r0, [r6, #8]
     f88:	e51b104c 	ldr	r1, [fp, #-76]	; 0x4c
     f8c:	e1a02000 	mov	r2, r0
     f90:	eb001aad 	bl	7a4c <hashmap_iterate>
     f94:	e5960008 	ldr	r0, [r6, #8]
     f98:	eb001a6b 	bl	794c <hashmap_free>
     f9c:	eb001a8a 	bl	79cc <hashmap_new>
     fa0:	e5860008 	str	r0, [r6, #8]
     fa4:	e1a02000 	mov	r2, r0
     fa8:	e51b1050 	ldr	r1, [fp, #-80]	; 0x50
     fac:	e59a0008 	ldr	r0, [sl, #8]
     fb0:	eb001aa5 	bl	7a4c <hashmap_iterate>
     fb4:	eaffffc2 	b	ec4 <core+0x134>
     fb8:	00016db0 			; <UNDEFINED> instruction: 0x00016db0
     fbc:	0000e6b0 			; <UNDEFINED> instruction: 0x0000e6b0
     fc0:	00016d78 	andeq	r6, r1, r8, ror sp
     fc4:	fffffb64 			; <UNDEFINED> instruction: 0xfffffb64
     fc8:	00016d50 	andeq	r6, r1, r0, asr sp
     fcc:	fffffa90 			; <UNDEFINED> instruction: 0xfffffa90
     fd0:	ffffff2c 			; <UNDEFINED> instruction: 0xffffff2c
     fd4:	0000e638 	andeq	lr, r0, r8, lsr r6
     fd8:	0000e634 	andeq	lr, r0, r4, lsr r6

00000fdc <vfs_open>:
     fdc:	e3510000 	cmp	r1, #0
     fe0:	e92d4010 	push	{r4, lr}
     fe4:	0a000023 	beq	1078 <vfs_open+0x9c>
     fe8:	e59f3094 	ldr	r3, [pc, #148]	; 1084 <vfs_open+0xa8>
     fec:	e1a04080 	lsl	r4, r0, #1
     ff0:	e08f3003 	add	r3, pc, r3
     ff4:	e084e000 	add	lr, r4, r0
     ff8:	e3a0c003 	mov	ip, #3
     ffc:	e083348e 	add	r3, r3, lr, lsl #9
    1000:	ea000003 	b	1014 <vfs_open+0x38>
    1004:	e28cc001 	add	ip, ip, #1
    1008:	e35c0080 	cmp	ip, #128	; 0x80
    100c:	e283300c 	add	r3, r3, #12
    1010:	0a000018 	beq	1078 <vfs_open+0x9c>
    1014:	e593e024 	ldr	lr, [r3, #36]	; 0x24
    1018:	e35e0000 	cmp	lr, #0
    101c:	1afffff8 	bne	1004 <vfs_open+0x28>
    1020:	e0844000 	add	r4, r4, r0
    1024:	e59f005c 	ldr	r0, [pc, #92]	; 1088 <vfs_open+0xac>
    1028:	e08c308c 	add	r3, ip, ip, lsl #1
    102c:	e1a04484 	lsl	r4, r4, #9
    1030:	e08f0000 	add	r0, pc, r0
    1034:	e0844103 	add	r4, r4, r3, lsl #2
    1038:	e5913078 	ldr	r3, [r1, #120]	; 0x78
    103c:	e7a01004 	str	r1, [r0, r4]!
    1040:	e3120002 	tst	r2, #2
    1044:	e2833001 	add	r3, r3, #1
    1048:	e5802004 	str	r2, [r0, #4]
    104c:	e5813078 	str	r3, [r1, #120]	; 0x78
    1050:	01a0000c 	moveq	r0, ip
    1054:	1a000001 	bne	1060 <vfs_open+0x84>
    1058:	e8bd4010 	pop	{r4, lr}
    105c:	e12fff1e 	bx	lr
    1060:	e591307c 	ldr	r3, [r1, #124]	; 0x7c
    1064:	e2833001 	add	r3, r3, #1
    1068:	e1a0000c 	mov	r0, ip
    106c:	e581307c 	str	r3, [r1, #124]	; 0x7c
    1070:	e8bd4010 	pop	{r4, lr}
    1074:	e12fff1e 	bx	lr
    1078:	e3e00000 	mvn	r0, #0
    107c:	e8bd4010 	pop	{r4, lr}
    1080:	e12fff1e 	bx	lr
    1084:	00016f6c 	andeq	r6, r1, ip, ror #30
    1088:	00016f2c 	andeq	r6, r1, ip, lsr #30

0000108c <vfs_get_mount>:
    108c:	e1a0c00d 	mov	ip, sp
    1090:	e3510000 	cmp	r1, #0
    1094:	13500000 	cmpne	r0, #0
    1098:	e92dd800 	push	{fp, ip, lr, pc}
    109c:	e24cb004 	sub	fp, ip, #4
    10a0:	0a000005 	beq	10bc <vfs_get_mount+0x30>
    10a4:	e5903074 	ldr	r3, [r0, #116]	; 0x74
    10a8:	e3530000 	cmp	r3, #0
    10ac:	aa000006 	bge	10cc <vfs_get_mount+0x40>
    10b0:	e5900000 	ldr	r0, [r0]
    10b4:	e3500000 	cmp	r0, #0
    10b8:	1afffff9 	bne	10a4 <vfs_get_mount+0x18>
    10bc:	e3e00000 	mvn	r0, #0
    10c0:	e24bd00c 	sub	sp, fp, #12
    10c4:	e89d6800 	ldm	sp, {fp, sp, lr}
    10c8:	e12fff1e 	bx	lr
    10cc:	e59f2028 	ldr	r2, [pc, #40]	; 10fc <vfs_get_mount+0x70>
    10d0:	e08f2002 	add	r2, pc, r2
    10d4:	e0833303 	add	r3, r3, r3, lsl #6
    10d8:	e28220c0 	add	r2, r2, #192	; 0xc0
    10dc:	e1a00001 	mov	r0, r1
    10e0:	e0821183 	add	r1, r2, r3, lsl #3
    10e4:	e3a02f82 	mov	r2, #520	; 0x208
    10e8:	eb003382 	bl	def8 <memcpy>
    10ec:	e3a00000 	mov	r0, #0
    10f0:	e24bd00c 	sub	sp, fp, #12
    10f4:	e89d6800 	ldm	sp, {fp, sp, lr}
    10f8:	e12fff1e 	bx	lr
    10fc:	00046dcc 	andeq	r6, r4, ip, asr #27

00001100 <get_mount_pid>:
    1100:	e1a0c00d 	mov	ip, sp
    1104:	e92dd800 	push	{fp, ip, lr, pc}
    1108:	e24cb004 	sub	fp, ip, #4
    110c:	e24ddf82 	sub	sp, sp, #520	; 0x208
    1110:	e24b1f85 	sub	r1, fp, #532	; 0x214
    1114:	ebffffdc 	bl	108c <vfs_get_mount>
    1118:	e3500000 	cmp	r0, #0
    111c:	13e00000 	mvnne	r0, #0
    1120:	051b0214 	ldreq	r0, [fp, #-532]	; 0x214
    1124:	e24bd00c 	sub	sp, fp, #12
    1128:	e89d6800 	ldm	sp, {fp, sp, lr}
    112c:	e12fff1e 	bx	lr

00001130 <vfs_new_node>:
    1130:	e1a0c00d 	mov	ip, sp
    1134:	e3a00080 	mov	r0, #128	; 0x80
    1138:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    113c:	e24cb004 	sub	fp, ip, #4
    1140:	eb003061 	bl	d2cc <malloc>
    1144:	e1a04000 	mov	r4, r0
    1148:	e3a01000 	mov	r1, #0
    114c:	e3a02080 	mov	r2, #128	; 0x80
    1150:	eb0033a4 	bl	dfe8 <memset>
    1154:	e3e03000 	mvn	r3, #0
    1158:	e1a00004 	mov	r0, r4
    115c:	e5844018 	str	r4, [r4, #24]
    1160:	e5843074 	str	r3, [r4, #116]	; 0x74
    1164:	e24bd014 	sub	sp, fp, #20
    1168:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    116c:	e12fff1e 	bx	lr

00001170 <do_vfs_get_mount_by_id>:
    1170:	e1a0c00d 	mov	ip, sp
    1174:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    1178:	e24cb004 	sub	fp, ip, #4
    117c:	e24ddf82 	sub	sp, sp, #520	; 0x208
    1180:	e1a05001 	mov	r5, r1
    1184:	eb002b78 	bl	bf6c <proto_read_int>
    1188:	e350001f 	cmp	r0, #31
    118c:	8a00001c 	bhi	1204 <do_vfs_get_mount_by_id+0x94>
    1190:	e59f1090 	ldr	r1, [pc, #144]	; 1228 <do_vfs_get_mount_by_id+0xb8>
    1194:	e0800300 	add	r0, r0, r0, lsl #6
    1198:	e08f1001 	add	r1, pc, r1
    119c:	e1a00180 	lsl	r0, r0, #3
    11a0:	e0813000 	add	r3, r1, r0
    11a4:	e59330c4 	ldr	r3, [r3, #196]	; 0xc4
    11a8:	e3530000 	cmp	r3, #0
    11ac:	0a000014 	beq	1204 <do_vfs_get_mount_by_id+0x94>
    11b0:	e24b4f87 	sub	r4, fp, #540	; 0x21c
    11b4:	e28110c0 	add	r1, r1, #192	; 0xc0
    11b8:	e0811000 	add	r1, r1, r0
    11bc:	e3a02f82 	mov	r2, #520	; 0x208
    11c0:	e1a00004 	mov	r0, r4
    11c4:	eb00334b 	bl	def8 <memcpy>
    11c8:	eb002a9e 	bl	bc48 <get_proto_factor>
    11cc:	e3a01000 	mov	r1, #0
    11d0:	e5903014 	ldr	r3, [r0, #20]
    11d4:	e1a00005 	mov	r0, r5
    11d8:	e1a0e00f 	mov	lr, pc
    11dc:	e12fff13 	bx	r3
    11e0:	e1a01004 	mov	r1, r4
    11e4:	e5903010 	ldr	r3, [r0, #16]
    11e8:	e3a02f82 	mov	r2, #520	; 0x208
    11ec:	e1a00005 	mov	r0, r5
    11f0:	e1a0e00f 	mov	lr, pc
    11f4:	e12fff13 	bx	r3
    11f8:	e24bd014 	sub	sp, fp, #20
    11fc:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    1200:	e12fff1e 	bx	lr
    1204:	eb002a8f 	bl	bc48 <get_proto_factor>
    1208:	e3e01000 	mvn	r1, #0
    120c:	e5903014 	ldr	r3, [r0, #20]
    1210:	e1a00005 	mov	r0, r5
    1214:	e1a0e00f 	mov	lr, pc
    1218:	e12fff13 	bx	r3
    121c:	e24bd014 	sub	sp, fp, #20
    1220:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    1224:	e12fff1e 	bx	lr
    1228:	00046d04 	andeq	r6, r4, r4, lsl #26

0000122c <vfs_del>:
    122c:	e1a0c00d 	mov	ip, sp
    1230:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    1234:	e2516000 	subs	r6, r1, #0
    1238:	e24cb004 	sub	fp, ip, #4
    123c:	e1a05000 	mov	r5, r0
    1240:	0a000025 	beq	12dc <vfs_del+0xb0>
    1244:	e5963078 	ldr	r3, [r6, #120]	; 0x78
    1248:	e3530000 	cmp	r3, #0
    124c:	1a000022 	bne	12dc <vfs_del+0xb0>
    1250:	e5961004 	ldr	r1, [r6, #4]
    1254:	e3510000 	cmp	r1, #0
    1258:	0a000004 	beq	1270 <vfs_del+0x44>
    125c:	e591400c 	ldr	r4, [r1, #12]
    1260:	e1a00005 	mov	r0, r5
    1264:	ebfffff0 	bl	122c <vfs_del>
    1268:	e2541000 	subs	r1, r4, #0
    126c:	1afffffa 	bne	125c <vfs_del+0x30>
    1270:	e5963000 	ldr	r3, [r6]
    1274:	e3530000 	cmp	r3, #0
    1278:	0a00000a 	beq	12a8 <vfs_del+0x7c>
    127c:	e5932004 	ldr	r2, [r3, #4]
    1280:	e1520006 	cmp	r2, r6
    1284:	0596200c 	ldreq	r2, [r6, #12]
    1288:	05832004 	streq	r2, [r3, #4]
    128c:	e5932008 	ldr	r2, [r3, #8]
    1290:	e1520006 	cmp	r2, r6
    1294:	05962010 	ldreq	r2, [r6, #16]
    1298:	05832008 	streq	r2, [r3, #8]
    129c:	e5932014 	ldr	r2, [r3, #20]
    12a0:	e2422001 	sub	r2, r2, #1
    12a4:	e5832014 	str	r2, [r3, #20]
    12a8:	e596300c 	ldr	r3, [r6, #12]
    12ac:	e3530000 	cmp	r3, #0
    12b0:	15962010 	ldrne	r2, [r6, #16]
    12b4:	15832010 	strne	r2, [r3, #16]
    12b8:	e5962010 	ldr	r2, [r6, #16]
    12bc:	e3520000 	cmp	r2, #0
    12c0:	e1a00006 	mov	r0, r6
    12c4:	1582300c 	strne	r3, [r2, #12]
    12c8:	eb002ff6 	bl	d2a8 <free>
    12cc:	e3a00000 	mov	r0, #0
    12d0:	e24bd01c 	sub	sp, fp, #28
    12d4:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    12d8:	e12fff1e 	bx	lr
    12dc:	e3e00000 	mvn	r0, #0
    12e0:	eafffffa 	b	12d0 <vfs_del+0xa4>

000012e4 <proc_file_close>:
    12e4:	e1a0c00d 	mov	ip, sp
    12e8:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    12ec:	e24cb004 	sub	fp, ip, #4
    12f0:	e24ddf85 	sub	sp, sp, #532	; 0x214
    12f4:	e2525000 	subs	r5, r2, #0
    12f8:	e1a06000 	mov	r6, r0
    12fc:	e1a07001 	mov	r7, r1
    1300:	0a00000e 	beq	1340 <proc_file_close+0x5c>
    1304:	e5954000 	ldr	r4, [r5]
    1308:	e3540000 	cmp	r4, #0
    130c:	0a00000b 	beq	1340 <proc_file_close+0x5c>
    1310:	e5942078 	ldr	r2, [r4, #120]	; 0x78
    1314:	e3520000 	cmp	r2, #0
    1318:	12422001 	subne	r2, r2, #1
    131c:	15842078 	strne	r2, [r4, #120]	; 0x78
    1320:	e5952004 	ldr	r2, [r5, #4]
    1324:	e3120002 	tst	r2, #2
    1328:	1a000007 	bne	134c <proc_file_close+0x68>
    132c:	e594205c 	ldr	r2, [r4, #92]	; 0x5c
    1330:	e3520003 	cmp	r2, #3
    1334:	0a00002c 	beq	13ec <proc_file_close+0x108>
    1338:	e3530000 	cmp	r3, #0
    133c:	1a000007 	bne	1360 <proc_file_close+0x7c>
    1340:	e24bd020 	sub	sp, fp, #32
    1344:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    1348:	e12fff1e 	bx	lr
    134c:	e594207c 	ldr	r2, [r4, #124]	; 0x7c
    1350:	e3520000 	cmp	r2, #0
    1354:	12422001 	subne	r2, r2, #1
    1358:	1584207c 	strne	r2, [r4, #124]	; 0x7c
    135c:	eafffff2 	b	132c <proc_file_close+0x48>
    1360:	e24b5f8b 	sub	r5, fp, #556	; 0x22c
    1364:	e1a01005 	mov	r1, r5
    1368:	e1a00004 	mov	r0, r4
    136c:	ebffff46 	bl	108c <vfs_get_mount>
    1370:	e2508000 	subs	r8, r0, #0
    1374:	1afffff1 	bne	1340 <proc_file_close+0x5c>
    1378:	e51b322c 	ldr	r3, [fp, #-556]	; 0x22c
    137c:	e3530000 	cmp	r3, #0
    1380:	baffffee 	blt	1340 <proc_file_close+0x5c>
    1384:	e2841018 	add	r1, r4, #24
    1388:	e3a0205c 	mov	r2, #92	; 0x5c
    138c:	e24b0e22 	sub	r0, fp, #544	; 0x220
    1390:	e50b3224 	str	r3, [fp, #-548]	; 0x224
    1394:	e50b722c 	str	r7, [fp, #-556]	; 0x22c
    1398:	e50b6228 	str	r6, [fp, #-552]	; 0x228
    139c:	eb0032d5 	bl	def8 <memcpy>
    13a0:	e3a0006c 	mov	r0, #108	; 0x6c
    13a4:	eb002fc8 	bl	d2cc <malloc>
    13a8:	e1a01005 	mov	r1, r5
    13ac:	e3a0206c 	mov	r2, #108	; 0x6c
    13b0:	eb0032d0 	bl	def8 <memcpy>
    13b4:	e1a03000 	mov	r3, r0
    13b8:	e59f1078 	ldr	r1, [pc, #120]	; 1438 <proc_file_close+0x154>
    13bc:	e08f1001 	add	r1, pc, r1
    13c0:	e59121d0 	ldr	r2, [r1, #464]	; 0x1d0
    13c4:	e3520000 	cmp	r2, #0
    13c8:	e5808068 	str	r8, [r0, #104]	; 0x68
    13cc:	15820068 	strne	r0, [r2, #104]	; 0x68
    13d0:	e59f2064 	ldr	r2, [pc, #100]	; 143c <proc_file_close+0x158>
    13d4:	e08f2002 	add	r2, pc, r2
    13d8:	058101d4 	streq	r0, [r1, #468]	; 0x1d4
    13dc:	e59201d8 	ldr	r0, [r2, #472]	; 0x1d8
    13e0:	e58231d0 	str	r3, [r2, #464]	; 0x1d0
    13e4:	eb002d89 	bl	ca10 <proc_wakeup>
    13e8:	eaffffd4 	b	1340 <proc_file_close+0x5c>
    13ec:	e1a00004 	mov	r0, r4
    13f0:	e50b3230 	str	r3, [fp, #-560]	; 0x230
    13f4:	eb002d85 	bl	ca10 <proc_wakeup>
    13f8:	e5942078 	ldr	r2, [r4, #120]	; 0x78
    13fc:	e3520000 	cmp	r2, #0
    1400:	e51b3230 	ldr	r3, [fp, #-560]	; 0x230
    1404:	1affffcb 	bne	1338 <proc_file_close+0x54>
    1408:	e5940070 	ldr	r0, [r4, #112]	; 0x70
    140c:	e3500000 	cmp	r0, #0
    1410:	0a000001 	beq	141c <proc_file_close+0x138>
    1414:	eb002fa3 	bl	d2a8 <free>
    1418:	e51b3230 	ldr	r3, [fp, #-560]	; 0x230
    141c:	e1a00004 	mov	r0, r4
    1420:	e50b3230 	str	r3, [fp, #-560]	; 0x230
    1424:	eb002f9f 	bl	d2a8 <free>
    1428:	e3a02000 	mov	r2, #0
    142c:	e51b3230 	ldr	r3, [fp, #-560]	; 0x230
    1430:	e5852000 	str	r2, [r5]
    1434:	eaffffbf 	b	1338 <proc_file_close+0x54>
    1438:	0004aad0 	ldrdeq	sl, [r4], -r0
    143c:	0004aab8 			; <UNDEFINED> instruction: 0x0004aab8

00001440 <vfs_close>:
    1440:	e1a02fa0 	lsr	r2, r0, #31
    1444:	e351007f 	cmp	r1, #127	; 0x7f
    1448:	83822001 	orrhi	r2, r2, #1
    144c:	e1a0c00d 	mov	ip, sp
    1450:	e3520000 	cmp	r2, #0
    1454:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    1458:	e24cb004 	sub	fp, ip, #4
    145c:	1a000009 	bne	1488 <vfs_close+0x48>
    1460:	e350007f 	cmp	r0, #127	; 0x7f
    1464:	c1a04002 	movgt	r4, r2
    1468:	da000009 	ble	1494 <vfs_close+0x54>
    146c:	e1a02004 	mov	r2, r4
    1470:	e3a03000 	mov	r3, #0
    1474:	ebffff9a 	bl	12e4 <proc_file_close>
    1478:	e1a00004 	mov	r0, r4
    147c:	e3a01000 	mov	r1, #0
    1480:	e3a0200c 	mov	r2, #12
    1484:	eb0032d7 	bl	dfe8 <memset>
    1488:	e24bd014 	sub	sp, fp, #20
    148c:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    1490:	e12fff1e 	bx	lr
    1494:	e0804080 	add	r4, r0, r0, lsl #1
    1498:	e59f2014 	ldr	r2, [pc, #20]	; 14b4 <vfs_close+0x74>
    149c:	e1a03484 	lsl	r3, r4, #9
    14a0:	e0814081 	add	r4, r1, r1, lsl #1
    14a4:	e08f2002 	add	r2, pc, r2
    14a8:	e0834104 	add	r4, r3, r4, lsl #2
    14ac:	e0824004 	add	r4, r2, r4
    14b0:	eaffffed 	b	146c <vfs_close+0x2c>
    14b4:	00016ab8 			; <UNDEFINED> instruction: 0x00016ab8

000014b8 <vfs_remove.isra.2>:
    14b8:	e3500000 	cmp	r0, #0
    14bc:	012fff1e 	bxeq	lr
    14c0:	e5903000 	ldr	r3, [r0]
    14c4:	e3530000 	cmp	r3, #0
    14c8:	0a000013 	beq	151c <vfs_remove.isra.2+0x64>
    14cc:	e5932004 	ldr	r2, [r3, #4]
    14d0:	e590100c 	ldr	r1, [r0, #12]
    14d4:	e1500002 	cmp	r0, r2
    14d8:	e5932008 	ldr	r2, [r3, #8]
    14dc:	e593c014 	ldr	ip, [r3, #20]
    14e0:	05831004 	streq	r1, [r3, #4]
    14e4:	e1500002 	cmp	r0, r2
    14e8:	e5902010 	ldr	r2, [r0, #16]
    14ec:	e24cc001 	sub	ip, ip, #1
    14f0:	05832008 	streq	r2, [r3, #8]
    14f4:	e583c014 	str	ip, [r3, #20]
    14f8:	e3a03000 	mov	r3, #0
    14fc:	e3510000 	cmp	r1, #0
    1500:	15812010 	strne	r2, [r1, #16]
    1504:	15902010 	ldrne	r2, [r0, #16]
    1508:	e3520000 	cmp	r2, #0
    150c:	1582100c 	strne	r1, [r2, #12]
    1510:	e5803010 	str	r3, [r0, #16]
    1514:	e580300c 	str	r3, [r0, #12]
    1518:	e12fff1e 	bx	lr
    151c:	e280100c 	add	r1, r0, #12
    1520:	e8910006 	ldm	r1, {r1, r2}
    1524:	eafffff3 	b	14f8 <vfs_remove.isra.2+0x40>

00001528 <vfs_simple_get>:
    1528:	e1a0c00d 	mov	ip, sp
    152c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    1530:	e2504000 	subs	r4, r0, #0
    1534:	e24cb004 	sub	fp, ip, #4
    1538:	e1a05001 	mov	r5, r1
    153c:	0a000011 	beq	1588 <vfs_simple_get+0x60>
    1540:	e1a00001 	mov	r0, r1
    1544:	e3a0102f 	mov	r1, #47	; 0x2f
    1548:	eb0032e3 	bl	e0dc <strchr>
    154c:	e3500000 	cmp	r0, #0
    1550:	13a00000 	movne	r0, #0
    1554:	1a00000c 	bne	158c <vfs_simple_get+0x64>
    1558:	e5944004 	ldr	r4, [r4, #4]
    155c:	e3540000 	cmp	r4, #0
    1560:	1a000003 	bne	1574 <vfs_simple_get+0x4c>
    1564:	ea000007 	b	1588 <vfs_simple_get+0x60>
    1568:	e594400c 	ldr	r4, [r4, #12]
    156c:	e3540000 	cmp	r4, #0
    1570:	0a000004 	beq	1588 <vfs_simple_get+0x60>
    1574:	e284001c 	add	r0, r4, #28
    1578:	e1a01005 	mov	r1, r5
    157c:	eb00332e 	bl	e23c <strcmp>
    1580:	e3500000 	cmp	r0, #0
    1584:	1afffff7 	bne	1568 <vfs_simple_get+0x40>
    1588:	e1a00004 	mov	r0, r4
    158c:	e24bd014 	sub	sp, fp, #20
    1590:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    1594:	e12fff1e 	bx	lr

00001598 <vfs_get_by_name>:
    1598:	e1a0c00d 	mov	ip, sp
    159c:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    15a0:	e3500000 	cmp	r0, #0
    15a4:	e24cb004 	sub	fp, ip, #4
    15a8:	e24ddf82 	sub	sp, sp, #520	; 0x208
    15ac:	0a00001b 	beq	1620 <vfs_get_by_name+0x88>
    15b0:	e5d13000 	ldrb	r3, [r1]
    15b4:	e353002f 	cmp	r3, #47	; 0x2f
    15b8:	0a00001d 	beq	1634 <vfs_get_by_name+0x9c>
    15bc:	e3a05000 	mov	r5, #0
    15c0:	e24b8f8a 	sub	r8, fp, #552	; 0x228
    15c4:	e1a06001 	mov	r6, r1
    15c8:	e1a09005 	mov	r9, r5
    15cc:	e1a01005 	mov	r1, r5
    15d0:	e2484001 	sub	r4, r8, #1
    15d4:	e24b7029 	sub	r7, fp, #41	; 0x29
    15d8:	ea000001 	b	15e4 <vfs_get_by_name+0x4c>
    15dc:	e1540007 	cmp	r4, r7
    15e0:	0a00000e 	beq	1620 <vfs_get_by_name+0x88>
    15e4:	e4d63001 	ldrb	r3, [r6], #1
    15e8:	e3530000 	cmp	r3, #0
    15ec:	e2855001 	add	r5, r5, #1
    15f0:	e5e43001 	strb	r3, [r4, #1]!
    15f4:	0a000016 	beq	1654 <vfs_get_by_name+0xbc>
    15f8:	e353002f 	cmp	r3, #47	; 0x2f
    15fc:	1afffff6 	bne	15dc <vfs_get_by_name+0x44>
    1600:	e0881001 	add	r1, r8, r1
    1604:	e5c49000 	strb	r9, [r4]
    1608:	ebffffc6 	bl	1528 <vfs_simple_get>
    160c:	e3500000 	cmp	r0, #0
    1610:	0a000002 	beq	1620 <vfs_get_by_name+0x88>
    1614:	e1540007 	cmp	r4, r7
    1618:	e1a01005 	mov	r1, r5
    161c:	1afffff0 	bne	15e4 <vfs_get_by_name+0x4c>
    1620:	e3a00000 	mov	r0, #0
    1624:	e24bd024 	sub	sp, fp, #36	; 0x24
    1628:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    162c:	e12fff1e 	bx	lr
    1630:	e1a00003 	mov	r0, r3
    1634:	e5903000 	ldr	r3, [r0]
    1638:	e3530000 	cmp	r3, #0
    163c:	1afffffb 	bne	1630 <vfs_get_by_name+0x98>
    1640:	e5d13001 	ldrb	r3, [r1, #1]
    1644:	e3530000 	cmp	r3, #0
    1648:	0a000003 	beq	165c <vfs_get_by_name+0xc4>
    164c:	e2811001 	add	r1, r1, #1
    1650:	eaffffd9 	b	15bc <vfs_get_by_name+0x24>
    1654:	e0881001 	add	r1, r8, r1
    1658:	ebffffb2 	bl	1528 <vfs_simple_get>
    165c:	e24bd024 	sub	sp, fp, #36	; 0x24
    1660:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    1664:	e12fff1e 	bx	lr

00001668 <do_vfs_get_kids>:
    1668:	e1a0c00d 	mov	ip, sp
    166c:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    1670:	e24cb004 	sub	fp, ip, #4
    1674:	e24ddf9f 	sub	sp, sp, #636	; 0x27c
    1678:	e1a08001 	mov	r8, r1
    167c:	e1a04000 	mov	r4, r0
    1680:	eb002970 	bl	bc48 <get_proto_factor>
    1684:	e3a01000 	mov	r1, #0
    1688:	e5903014 	ldr	r3, [r0, #20]
    168c:	e1a00008 	mov	r0, r8
    1690:	e1a0e00f 	mov	lr, pc
    1694:	e12fff13 	bx	r3
    1698:	e1a00004 	mov	r0, r4
    169c:	e24b1e29 	sub	r1, fp, #656	; 0x290
    16a0:	e3a0205c 	mov	r2, #92	; 0x5c
    16a4:	eb0029c5 	bl	bdc0 <proto_read_to>
    16a8:	e350005c 	cmp	r0, #92	; 0x5c
    16ac:	0a000002 	beq	16bc <do_vfs_get_kids+0x54>
    16b0:	e24bd028 	sub	sp, fp, #40	; 0x28
    16b4:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    16b8:	e12fff1e 	bx	lr
    16bc:	e51b4290 	ldr	r4, [fp, #-656]	; 0x290
    16c0:	e3540000 	cmp	r4, #0
    16c4:	0afffff9 	beq	16b0 <do_vfs_get_kids+0x48>
    16c8:	e5947014 	ldr	r7, [r4, #20]
    16cc:	e3570000 	cmp	r7, #0
    16d0:	0afffff6 	beq	16b0 <do_vfs_get_kids+0x48>
    16d4:	e0870087 	add	r0, r7, r7, lsl #1
    16d8:	e0670180 	rsb	r0, r7, r0, lsl #3
    16dc:	e1a00100 	lsl	r0, r0, #2
    16e0:	eb002ef9 	bl	d2cc <malloc>
    16e4:	e250a000 	subs	sl, r0, #0
    16e8:	0afffff0 	beq	16b0 <do_vfs_get_kids+0x48>
    16ec:	e5944004 	ldr	r4, [r4, #4]
    16f0:	e3540000 	cmp	r4, #0
    16f4:	0affffed 	beq	16b0 <do_vfs_get_kids+0x48>
    16f8:	e3a06000 	mov	r6, #0
    16fc:	e59f30c8 	ldr	r3, [pc, #200]	; 17cc <do_vfs_get_kids+0x164>
    1700:	e08f3003 	add	r3, pc, r3
    1704:	e50b82a0 	str	r8, [fp, #-672]	; 0x2a0
    1708:	e1a05006 	mov	r5, r6
    170c:	e1a08003 	mov	r8, r3
    1710:	e2839f77 	add	r9, r3, #476	; 0x1dc
    1714:	e24b2f8d 	sub	r2, fp, #564	; 0x234
    1718:	e50b929c 	str	r9, [fp, #-668]	; 0x29c
    171c:	e50b2298 	str	r2, [fp, #-664]	; 0x298
    1720:	e2841018 	add	r1, r4, #24
    1724:	e3a0205c 	mov	r2, #92	; 0x5c
    1728:	e1a00009 	mov	r0, r9
    172c:	eb0031f1 	bl	def8 <memcpy>
    1730:	e1a00004 	mov	r0, r4
    1734:	e51b1298 	ldr	r1, [fp, #-664]	; 0x298
    1738:	ebfffe53 	bl	108c <vfs_get_mount>
    173c:	e3500000 	cmp	r0, #0
    1740:	13e03000 	mvnne	r3, #0
    1744:	051b3234 	ldreq	r3, [fp, #-564]	; 0x234
    1748:	e08a0006 	add	r0, sl, r6
    174c:	e51b129c 	ldr	r1, [fp, #-668]	; 0x29c
    1750:	e3a0205c 	mov	r2, #92	; 0x5c
    1754:	e5883230 	str	r3, [r8, #560]	; 0x230
    1758:	eb0031e6 	bl	def8 <memcpy>
    175c:	e594400c 	ldr	r4, [r4, #12]
    1760:	e2855001 	add	r5, r5, #1
    1764:	e3540000 	cmp	r4, #0
    1768:	11570005 	cmpne	r7, r5
    176c:	e286605c 	add	r6, r6, #92	; 0x5c
    1770:	8affffea 	bhi	1720 <do_vfs_get_kids+0xb8>
    1774:	e3550000 	cmp	r5, #0
    1778:	e51b82a0 	ldr	r8, [fp, #-672]	; 0x2a0
    177c:	0affffcb 	beq	16b0 <do_vfs_get_kids+0x48>
    1780:	eb002930 	bl	bc48 <get_proto_factor>
    1784:	e590300c 	ldr	r3, [r0, #12]
    1788:	e1a00008 	mov	r0, r8
    178c:	e1a0e00f 	mov	lr, pc
    1790:	e12fff13 	bx	r3
    1794:	e1a01005 	mov	r1, r5
    1798:	e5903014 	ldr	r3, [r0, #20]
    179c:	e1a00008 	mov	r0, r8
    17a0:	e1a0e00f 	mov	lr, pc
    17a4:	e12fff13 	bx	r3
    17a8:	e0852085 	add	r2, r5, r5, lsl #1
    17ac:	e0652182 	rsb	r2, r5, r2, lsl #3
    17b0:	e5903010 	ldr	r3, [r0, #16]
    17b4:	e1a0100a 	mov	r1, sl
    17b8:	e1a00008 	mov	r0, r8
    17bc:	e1a02102 	lsl	r2, r2, #2
    17c0:	e1a0e00f 	mov	lr, pc
    17c4:	e12fff13 	bx	r3
    17c8:	eaffffb8 	b	16b0 <do_vfs_get_kids+0x48>
    17cc:	0004a78c 	andeq	sl, r4, ip, lsl #15

000017d0 <vfs_root>:
    17d0:	e59f3008 	ldr	r3, [pc, #8]	; 17e0 <vfs_root+0x10>
    17d4:	e08f3003 	add	r3, pc, r3
    17d8:	e59301d8 	ldr	r0, [r3, #472]	; 0x1d8
    17dc:	e12fff1e 	bx	lr
    17e0:	0004a6b8 			; <UNDEFINED> instruction: 0x0004a6b8

000017e4 <handle>:
    17e4:	e1a0c00d 	mov	ip, sp
    17e8:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    17ec:	e24cb004 	sub	fp, ip, #4
    17f0:	e24dd0c4 	sub	sp, sp, #196	; 0xc4
    17f4:	e1a05001 	mov	r5, r1
    17f8:	e1a06002 	mov	r6, r2
    17fc:	e1a04003 	mov	r4, r3
    1800:	eb002c45 	bl	c91c <proc_getpid>
    1804:	e1a08000 	mov	r8, r0
    1808:	e3550016 	cmp	r5, #22
    180c:	908ff105 	addls	pc, pc, r5, lsl #2
    1810:	ea000046 	b	1930 <handle+0x14c>
    1814:	ea000285 	b	2230 <handle+0xa4c>
    1818:	ea000278 	b	2200 <handle+0xa1c>
    181c:	ea0001eb 	b	1fd0 <handle+0x7ec>
    1820:	ea0001d0 	b	1f68 <handle+0x784>
    1824:	ea00025a 	b	2194 <handle+0x9b0>
    1828:	ea00022a 	b	20d8 <handle+0x8f4>
    182c:	ea00018e 	b	1e6c <handle+0x688>
    1830:	ea00015c 	b	1da8 <handle+0x5c4>
    1834:	ea0002dc 	b	23ac <handle+0xbc8>
    1838:	ea00029b 	b	22ac <handle+0xac8>
    183c:	ea00032b 	b	24f0 <handle+0xd0c>
    1840:	ea0001b3 	b	1f14 <handle+0x730>
    1844:	ea000310 	b	248c <handle+0xca8>
    1848:	ea0001f0 	b	2010 <handle+0x82c>
    184c:	ea000138 	b	1d34 <handle+0x550>
    1850:	ea0000c6 	b	1b70 <handle+0x38c>
    1854:	ea000005 	b	1870 <handle+0x8c>
    1858:	ea0000bc 	b	1b50 <handle+0x36c>
    185c:	ea0000bf 	b	1b60 <handle+0x37c>
    1860:	ea00009f 	b	1ae4 <handle+0x300>
    1864:	ea000085 	b	1a80 <handle+0x29c>
    1868:	ea000050 	b	19b0 <handle+0x1cc>
    186c:	ea000032 	b	193c <handle+0x158>
    1870:	e1a00006 	mov	r0, r6
    1874:	e24b1088 	sub	r1, fp, #136	; 0x88
    1878:	e3a0205c 	mov	r2, #92	; 0x5c
    187c:	eb00294f 	bl	bdc0 <proto_read_to>
    1880:	e350005c 	cmp	r0, #92	; 0x5c
    1884:	1a000029 	bne	1930 <handle+0x14c>
    1888:	e51b4088 	ldr	r4, [fp, #-136]	; 0x88
    188c:	e3540000 	cmp	r4, #0
    1890:	0a000026 	beq	1930 <handle+0x14c>
    1894:	e5943074 	ldr	r3, [r4, #116]	; 0x74
    1898:	e3530000 	cmp	r3, #0
    189c:	ba000023 	blt	1930 <handle+0x14c>
    18a0:	e59f1e10 	ldr	r1, [pc, #3600]	; 26b8 <handle+0xed4>
    18a4:	e1a02303 	lsl	r2, r3, #6
    18a8:	e08f1001 	add	r1, pc, r1
    18ac:	e0820003 	add	r0, r2, r3
    18b0:	e0811180 	add	r1, r1, r0, lsl #3
    18b4:	e59150c4 	ldr	r5, [r1, #196]	; 0xc4
    18b8:	e3550000 	cmp	r5, #0
    18bc:	0a00001b 	beq	1930 <handle+0x14c>
    18c0:	e5946000 	ldr	r6, [r4]
    18c4:	e3560000 	cmp	r6, #0
    18c8:	0a00035e 	beq	2648 <handle+0xe64>
    18cc:	e1a00004 	mov	r0, r4
    18d0:	ebfffef8 	bl	14b8 <vfs_remove.isra.2>
    18d4:	e5953074 	ldr	r3, [r5, #116]	; 0x74
    18d8:	e3530000 	cmp	r3, #0
    18dc:	ba000370 	blt	26a4 <handle+0xec0>
    18e0:	e5963008 	ldr	r3, [r6, #8]
    18e4:	e5962014 	ldr	r2, [r6, #20]
    18e8:	e3530000 	cmp	r3, #0
    18ec:	e5856000 	str	r6, [r5]
    18f0:	1583500c 	strne	r5, [r3, #12]
    18f4:	15853010 	strne	r3, [r5, #16]
    18f8:	e5943074 	ldr	r3, [r4, #116]	; 0x74
    18fc:	e2822001 	add	r2, r2, #1
    1900:	05865004 	streq	r5, [r6, #4]
    1904:	e5865008 	str	r5, [r6, #8]
    1908:	e5862014 	str	r2, [r6, #20]
    190c:	e1a02303 	lsl	r2, r3, #6
    1910:	e59f0da4 	ldr	r0, [pc, #3492]	; 26bc <handle+0xed8>
    1914:	e08f0000 	add	r0, pc, r0
    1918:	e0823003 	add	r3, r2, r3
    191c:	e28000c0 	add	r0, r0, #192	; 0xc0
    1920:	e0800183 	add	r0, r0, r3, lsl #3
    1924:	e3a01000 	mov	r1, #0
    1928:	e3a02f82 	mov	r2, #520	; 0x208
    192c:	eb0031ad 	bl	dfe8 <memset>
    1930:	e24bd028 	sub	sp, fp, #40	; 0x28
    1934:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    1938:	e12fff1e 	bx	lr
    193c:	e1a00006 	mov	r0, r6
    1940:	eb002989 	bl	bf6c <proto_read_int>
    1944:	e2507000 	subs	r7, r0, #0
    1948:	bafffff8 	blt	1930 <handle+0x14c>
    194c:	e3a05000 	mov	r5, #0
    1950:	e0872087 	add	r2, r7, r7, lsl #1
    1954:	e59f4d64 	ldr	r4, [pc, #3428]	; 26c0 <handle+0xedc>
    1958:	e1a02482 	lsl	r2, r2, #9
    195c:	e08f4004 	add	r4, pc, r4
    1960:	e2826c06 	add	r6, r2, #1536	; 0x600
    1964:	e0846006 	add	r6, r4, r6
    1968:	e0844002 	add	r4, r4, r2
    196c:	e5943000 	ldr	r3, [r4]
    1970:	e3530000 	cmp	r3, #0
    1974:	0a000004 	beq	198c <handle+0x1a8>
    1978:	e1a00007 	mov	r0, r7
    197c:	e1a01005 	mov	r1, r5
    1980:	e1a02004 	mov	r2, r4
    1984:	e3a03001 	mov	r3, #1
    1988:	ebfffe55 	bl	12e4 <proc_file_close>
    198c:	e1a00004 	mov	r0, r4
    1990:	e3a01000 	mov	r1, #0
    1994:	e284400c 	add	r4, r4, #12
    1998:	e3a0200c 	mov	r2, #12
    199c:	eb003191 	bl	dfe8 <memset>
    19a0:	e1540006 	cmp	r4, r6
    19a4:	e2855001 	add	r5, r5, #1
    19a8:	1affffef 	bne	196c <handle+0x188>
    19ac:	eaffffdf 	b	1930 <handle+0x14c>
    19b0:	e1a00006 	mov	r0, r6
    19b4:	eb00296c 	bl	bf6c <proto_read_int>
    19b8:	e1a04000 	mov	r4, r0
    19bc:	e1a00006 	mov	r0, r6
    19c0:	eb002969 	bl	bf6c <proto_read_int>
    19c4:	e1903004 	orrs	r3, r0, r4
    19c8:	4affffd8 	bmi	1930 <handle+0x14c>
    19cc:	e0843084 	add	r3, r4, r4, lsl #1
    19d0:	e0640000 	rsb	r0, r4, r0
    19d4:	e59f4ce8 	ldr	r4, [pc, #3304]	; 26c4 <handle+0xee0>
    19d8:	e1a03483 	lsl	r3, r3, #9
    19dc:	e08f4004 	add	r4, pc, r4
    19e0:	e0800080 	add	r0, r0, r0, lsl #1
    19e4:	e2836c06 	add	r6, r3, #1536	; 0x600
    19e8:	e0846006 	add	r6, r4, r6
    19ec:	e1a07480 	lsl	r7, r0, #9
    19f0:	e0844003 	add	r4, r4, r3
    19f4:	ea000002 	b	1a04 <handle+0x220>
    19f8:	e284400c 	add	r4, r4, #12
    19fc:	e1540006 	cmp	r4, r6
    1a00:	0affffca 	beq	1930 <handle+0x14c>
    1a04:	e5945000 	ldr	r5, [r4]
    1a08:	e3550000 	cmp	r5, #0
    1a0c:	0afffff9 	beq	19f8 <handle+0x214>
    1a10:	e0840007 	add	r0, r4, r7
    1a14:	e1a01004 	mov	r1, r4
    1a18:	e3a0200c 	mov	r2, #12
    1a1c:	eb003135 	bl	def8 <memcpy>
    1a20:	e5953078 	ldr	r3, [r5, #120]	; 0x78
    1a24:	e2833001 	add	r3, r3, #1
    1a28:	e5853078 	str	r3, [r5, #120]	; 0x78
    1a2c:	e5943004 	ldr	r3, [r4, #4]
    1a30:	e3130002 	tst	r3, #2
    1a34:	1595307c 	ldrne	r3, [r5, #124]	; 0x7c
    1a38:	12833001 	addne	r3, r3, #1
    1a3c:	1585307c 	strne	r3, [r5, #124]	; 0x7c
    1a40:	e595305c 	ldr	r3, [r5, #92]	; 0x5c
    1a44:	e3530003 	cmp	r3, #3
    1a48:	1affffea 	bne	19f8 <handle+0x214>
    1a4c:	e5958070 	ldr	r8, [r5, #112]	; 0x70
    1a50:	e3580000 	cmp	r8, #0
    1a54:	1affffe7 	bne	19f8 <handle+0x214>
    1a58:	e59f0c68 	ldr	r0, [pc, #3176]	; 26c8 <handle+0xee4>
    1a5c:	eb002e1a 	bl	d2cc <malloc>
    1a60:	e1a09000 	mov	r9, r0
    1a64:	e1a01008 	mov	r1, r8
    1a68:	e59f2c58 	ldr	r2, [pc, #3160]	; 26c8 <handle+0xee4>
    1a6c:	eb00315d 	bl	dfe8 <memset>
    1a70:	e5859070 	str	r9, [r5, #112]	; 0x70
    1a74:	e1a00005 	mov	r0, r5
    1a78:	eb002be4 	bl	ca10 <proc_wakeup>
    1a7c:	eaffffdd 	b	19f8 <handle+0x214>
    1a80:	e1a00006 	mov	r0, r6
    1a84:	eb002938 	bl	bf6c <proto_read_int>
    1a88:	e1a05000 	mov	r5, r0
    1a8c:	e1a00006 	mov	r0, r6
    1a90:	eb002935 	bl	bf6c <proto_read_int>
    1a94:	e358007f 	cmp	r8, #127	; 0x7f
    1a98:	9355007f 	cmpls	r5, #127	; 0x7f
    1a9c:	83a06001 	movhi	r6, #1
    1aa0:	93a06000 	movls	r6, #0
    1aa4:	8a000298 	bhi	250c <handle+0xd28>
    1aa8:	e0888088 	add	r8, r8, r8, lsl #1
    1aac:	e59f3c18 	ldr	r3, [pc, #3096]	; 26cc <handle+0xee8>
    1ab0:	e1a02488 	lsl	r2, r8, #9
    1ab4:	e0855085 	add	r5, r5, r5, lsl #1
    1ab8:	e0825105 	add	r5, r2, r5, lsl #2
    1abc:	e08f3003 	add	r3, pc, r3
    1ac0:	e0833005 	add	r3, r3, r5
    1ac4:	e5830004 	str	r0, [r3, #4]
    1ac8:	eb00285e 	bl	bc48 <get_proto_factor>
    1acc:	e1a01006 	mov	r1, r6
    1ad0:	e5903014 	ldr	r3, [r0, #20]
    1ad4:	e1a00004 	mov	r0, r4
    1ad8:	e1a0e00f 	mov	lr, pc
    1adc:	e12fff13 	bx	r3
    1ae0:	eaffff92 	b	1930 <handle+0x14c>
    1ae4:	e1a00006 	mov	r0, r6
    1ae8:	eb00291f 	bl	bf6c <proto_read_int>
    1aec:	e358007f 	cmp	r8, #127	; 0x7f
    1af0:	9350007f 	cmpls	r0, #127	; 0x7f
    1af4:	e1a05000 	mov	r5, r0
    1af8:	83a06001 	movhi	r6, #1
    1afc:	93a06000 	movls	r6, #0
    1b00:	8a000281 	bhi	250c <handle+0xd28>
    1b04:	eb00284f 	bl	bc48 <get_proto_factor>
    1b08:	e1a01006 	mov	r1, r6
    1b0c:	e5903014 	ldr	r3, [r0, #20]
    1b10:	e1a00004 	mov	r0, r4
    1b14:	e1a0e00f 	mov	lr, pc
    1b18:	e12fff13 	bx	r3
    1b1c:	e0888088 	add	r8, r8, r8, lsl #1
    1b20:	e59f3ba8 	ldr	r3, [pc, #2984]	; 26d0 <handle+0xeec>
    1b24:	e1a02488 	lsl	r2, r8, #9
    1b28:	e0855085 	add	r5, r5, r5, lsl #1
    1b2c:	e0825105 	add	r5, r2, r5, lsl #2
    1b30:	e08f3003 	add	r3, pc, r3
    1b34:	e0833005 	add	r3, r3, r5
    1b38:	e5902014 	ldr	r2, [r0, #20]
    1b3c:	e5931004 	ldr	r1, [r3, #4]
    1b40:	e1a00004 	mov	r0, r4
    1b44:	e1a0e00f 	mov	lr, pc
    1b48:	e12fff12 	bx	r2
    1b4c:	eaffff77 	b	1930 <handle+0x14c>
    1b50:	e1a00006 	mov	r0, r6
    1b54:	e1a01004 	mov	r1, r4
    1b58:	ebfffd84 	bl	1170 <do_vfs_get_mount_by_id>
    1b5c:	eaffff73 	b	1930 <handle+0x14c>
    1b60:	e1a00006 	mov	r0, r6
    1b64:	e1a01004 	mov	r1, r4
    1b68:	ebfffebe 	bl	1668 <do_vfs_get_kids>
    1b6c:	eaffff6f 	b	1930 <handle+0x14c>
    1b70:	eb002834 	bl	bc48 <get_proto_factor>
    1b74:	e3e01000 	mvn	r1, #0
    1b78:	e5903014 	ldr	r3, [r0, #20]
    1b7c:	e1a00004 	mov	r0, r4
    1b80:	e1a0e00f 	mov	lr, pc
    1b84:	e12fff13 	bx	r3
    1b88:	e3a0205c 	mov	r2, #92	; 0x5c
    1b8c:	e1a00006 	mov	r0, r6
    1b90:	e24b10e4 	sub	r1, fp, #228	; 0xe4
    1b94:	eb002889 	bl	bdc0 <proto_read_to>
    1b98:	e350005c 	cmp	r0, #92	; 0x5c
    1b9c:	e1a02000 	mov	r2, r0
    1ba0:	1affff62 	bne	1930 <handle+0x14c>
    1ba4:	e1a00006 	mov	r0, r6
    1ba8:	e24b1088 	sub	r1, fp, #136	; 0x88
    1bac:	eb002883 	bl	bdc0 <proto_read_to>
    1bb0:	e350005c 	cmp	r0, #92	; 0x5c
    1bb4:	1affff5d 	bne	1930 <handle+0x14c>
    1bb8:	e51b60e4 	ldr	r6, [fp, #-228]	; 0xe4
    1bbc:	e51b5088 	ldr	r5, [fp, #-136]	; 0x88
    1bc0:	e3560000 	cmp	r6, #0
    1bc4:	13550000 	cmpne	r5, #0
    1bc8:	03a07001 	moveq	r7, #1
    1bcc:	13a07000 	movne	r7, #0
    1bd0:	0affff56 	beq	1930 <handle+0x14c>
    1bd4:	e5953074 	ldr	r3, [r5, #116]	; 0x74
    1bd8:	e3530000 	cmp	r3, #0
    1bdc:	aa000132 	bge	20ac <handle+0x8c8>
    1be0:	e59f3aec 	ldr	r3, [pc, #2796]	; 26d4 <handle+0xef0>
    1be4:	e08f3003 	add	r3, pc, r3
    1be8:	e28330c0 	add	r3, r3, #192	; 0xc0
    1bec:	ea000003 	b	1c00 <handle+0x41c>
    1bf0:	e2877001 	add	r7, r7, #1
    1bf4:	e3570020 	cmp	r7, #32
    1bf8:	e2833f82 	add	r3, r3, #520	; 0x208
    1bfc:	0a00012a 	beq	20ac <handle+0x8c8>
    1c00:	e5932004 	ldr	r2, [r3, #4]
    1c04:	e3520000 	cmp	r2, #0
    1c08:	1afffff8 	bne	1bf0 <handle+0x40c>
    1c0c:	e3a03f82 	mov	r3, #520	; 0x208
    1c10:	e0010793 	mul	r1, r3, r7
    1c14:	e59f2abc 	ldr	r2, [pc, #2748]	; 26d8 <handle+0xef4>
    1c18:	e59f0abc 	ldr	r0, [pc, #2748]	; 26dc <handle+0xef8>
    1c1c:	e08f2002 	add	r2, pc, r2
    1c20:	e0822001 	add	r2, r2, r1
    1c24:	e08f0000 	add	r0, pc, r0
    1c28:	e58280c0 	str	r8, [r2, #192]	; 0xc0
    1c2c:	e28230c8 	add	r3, r2, #200	; 0xc8
    1c30:	e58260c4 	str	r6, [r2, #196]	; 0xc4
    1c34:	e50b30e8 	str	r3, [fp, #-232]	; 0xe8
    1c38:	e50b00ec 	str	r0, [fp, #-236]	; 0xec
    1c3c:	eb001812 	bl	7c8c <str_new>
    1c40:	e1a09006 	mov	r9, r6
    1c44:	e1a08000 	mov	r8, r0
    1c48:	e51b00ec 	ldr	r0, [fp, #-236]	; 0xec
    1c4c:	eb00180e 	bl	7c8c <str_new>
    1c50:	e289101c 	add	r1, r9, #28
    1c54:	e1a0a000 	mov	sl, r0
    1c58:	eb001800 	bl	7c60 <str_cpy>
    1c5c:	e5981000 	ldr	r1, [r8]
    1c60:	e5d13000 	ldrb	r3, [r1]
    1c64:	e3530000 	cmp	r3, #0
    1c68:	0a000008 	beq	1c90 <handle+0x4ac>
    1c6c:	e5d9301c 	ldrb	r3, [r9, #28]
    1c70:	e353002f 	cmp	r3, #47	; 0x2f
    1c74:	0a000003 	beq	1c88 <handle+0x4a4>
    1c78:	e3a0102f 	mov	r1, #47	; 0x2f
    1c7c:	e1a0000a 	mov	r0, sl
    1c80:	eb00184a 	bl	7db0 <str_addc>
    1c84:	e5981000 	ldr	r1, [r8]
    1c88:	e1a0000a 	mov	r0, sl
    1c8c:	eb001820 	bl	7d14 <str_add>
    1c90:	e59a1000 	ldr	r1, [sl]
    1c94:	e1a00008 	mov	r0, r8
    1c98:	eb0017f0 	bl	7c60 <str_cpy>
    1c9c:	e1a0000a 	mov	r0, sl
    1ca0:	eb00186c 	bl	7e58 <str_free>
    1ca4:	e5999000 	ldr	r9, [r9]
    1ca8:	e3590000 	cmp	r9, #0
    1cac:	1affffe5 	bne	1c48 <handle+0x464>
    1cb0:	e59f9a28 	ldr	r9, [pc, #2600]	; 26e0 <handle+0xefc>
    1cb4:	e08f9009 	add	r9, pc, r9
    1cb8:	e289af8e 	add	sl, r9, #568	; 0x238
    1cbc:	e5981000 	ldr	r1, [r8]
    1cc0:	e59f2a1c 	ldr	r2, [pc, #2588]	; 26e4 <handle+0xf00>
    1cc4:	e1a0000a 	mov	r0, sl
    1cc8:	eb00327e 	bl	e6c8 <strncpy>
    1ccc:	e1a00008 	mov	r0, r8
    1cd0:	eb001860 	bl	7e58 <str_free>
    1cd4:	e1a0100a 	mov	r1, sl
    1cd8:	e51b00e8 	ldr	r0, [fp, #-232]	; 0xe8
    1cdc:	eb0031df 	bl	e460 <strcpy>
    1ce0:	e285001c 	add	r0, r5, #28
    1ce4:	e286101c 	add	r1, r6, #28
    1ce8:	eb0031dc 	bl	e460 <strcpy>
    1cec:	e5968000 	ldr	r8, [r6]
    1cf0:	e3580000 	cmp	r8, #0
    1cf4:	e5857074 	str	r7, [r5, #116]	; 0x74
    1cf8:	058951d8 	streq	r5, [r9, #472]	; 0x1d8
    1cfc:	0a0000ea 	beq	20ac <handle+0x8c8>
    1d00:	e1a00006 	mov	r0, r6
    1d04:	ebfffdeb 	bl	14b8 <vfs_remove.isra.2>
    1d08:	e5983008 	ldr	r3, [r8, #8]
    1d0c:	e3530000 	cmp	r3, #0
    1d10:	e5858000 	str	r8, [r5]
    1d14:	1583500c 	strne	r5, [r3, #12]
    1d18:	15853010 	strne	r3, [r5, #16]
    1d1c:	e5983014 	ldr	r3, [r8, #20]
    1d20:	e2833001 	add	r3, r3, #1
    1d24:	05885004 	streq	r5, [r8, #4]
    1d28:	e5885008 	str	r5, [r8, #8]
    1d2c:	e5883014 	str	r3, [r8, #20]
    1d30:	ea0000dd 	b	20ac <handle+0x8c8>
    1d34:	eb0027c3 	bl	bc48 <get_proto_factor>
    1d38:	e3e01000 	mvn	r1, #0
    1d3c:	e5903014 	ldr	r3, [r0, #20]
    1d40:	e1a00004 	mov	r0, r4
    1d44:	e1a0e00f 	mov	lr, pc
    1d48:	e12fff13 	bx	r3
    1d4c:	e1a00006 	mov	r0, r6
    1d50:	e24b1088 	sub	r1, fp, #136	; 0x88
    1d54:	e3a0205c 	mov	r2, #92	; 0x5c
    1d58:	eb002818 	bl	bdc0 <proto_read_to>
    1d5c:	e350005c 	cmp	r0, #92	; 0x5c
    1d60:	1afffef2 	bne	1930 <handle+0x14c>
    1d64:	e51b1088 	ldr	r1, [fp, #-136]	; 0x88
    1d68:	e3510000 	cmp	r1, #0
    1d6c:	0afffeef 	beq	1930 <handle+0x14c>
    1d70:	e1a00008 	mov	r0, r8
    1d74:	ebfffd2c 	bl	122c <vfs_del>
    1d78:	e1a05000 	mov	r5, r0
    1d7c:	eb0027b1 	bl	bc48 <get_proto_factor>
    1d80:	e590300c 	ldr	r3, [r0, #12]
    1d84:	e1a00004 	mov	r0, r4
    1d88:	e1a0e00f 	mov	lr, pc
    1d8c:	e12fff13 	bx	r3
    1d90:	e1a01005 	mov	r1, r5
    1d94:	e5903014 	ldr	r3, [r0, #20]
    1d98:	e1a00004 	mov	r0, r4
    1d9c:	e1a0e00f 	mov	lr, pc
    1da0:	e12fff13 	bx	r3
    1da4:	eafffee1 	b	1930 <handle+0x14c>
    1da8:	eb0027a6 	bl	bc48 <get_proto_factor>
    1dac:	e3e01000 	mvn	r1, #0
    1db0:	e5903014 	ldr	r3, [r0, #20]
    1db4:	e1a00004 	mov	r0, r4
    1db8:	e1a0e00f 	mov	lr, pc
    1dbc:	e12fff13 	bx	r3
    1dc0:	e1a00006 	mov	r0, r6
    1dc4:	e24b1088 	sub	r1, fp, #136	; 0x88
    1dc8:	e3a0205c 	mov	r2, #92	; 0x5c
    1dcc:	eb0027fb 	bl	bdc0 <proto_read_to>
    1dd0:	e350005c 	cmp	r0, #92	; 0x5c
    1dd4:	1afffed5 	bne	1930 <handle+0x14c>
    1dd8:	e51b7088 	ldr	r7, [fp, #-136]	; 0x88
    1ddc:	e1a00007 	mov	r0, r7
    1de0:	eb002b0a 	bl	ca10 <proc_wakeup>
    1de4:	e1a00006 	mov	r0, r6
    1de8:	eb00285f 	bl	bf6c <proto_read_int>
    1dec:	e2508000 	subs	r8, r0, #0
    1df0:	bafffece 	blt	1930 <handle+0x14c>
    1df4:	e51b6030 	ldr	r6, [fp, #-48]	; 0x30
    1df8:	e3560000 	cmp	r6, #0
    1dfc:	0a000215 	beq	2658 <handle+0xe74>
    1e00:	eb002d31 	bl	d2cc <malloc>
    1e04:	e1a05000 	mov	r5, r0
    1e08:	e1a02008 	mov	r2, r8
    1e0c:	e1a00006 	mov	r0, r6
    1e10:	e1a01005 	mov	r1, r5
    1e14:	eb002b19 	bl	ca80 <buffer_read>
    1e18:	e2506000 	subs	r6, r0, #0
    1e1c:	da000218 	ble	2684 <handle+0xea0>
    1e20:	eb002788 	bl	bc48 <get_proto_factor>
    1e24:	e590300c 	ldr	r3, [r0, #12]
    1e28:	e1a00004 	mov	r0, r4
    1e2c:	e1a0e00f 	mov	lr, pc
    1e30:	e12fff13 	bx	r3
    1e34:	e1a01006 	mov	r1, r6
    1e38:	e5903014 	ldr	r3, [r0, #20]
    1e3c:	e1a00004 	mov	r0, r4
    1e40:	e1a0e00f 	mov	lr, pc
    1e44:	e12fff13 	bx	r3
    1e48:	e1a02006 	mov	r2, r6
    1e4c:	e5903010 	ldr	r3, [r0, #16]
    1e50:	e1a01005 	mov	r1, r5
    1e54:	e1a00004 	mov	r0, r4
    1e58:	e1a0e00f 	mov	lr, pc
    1e5c:	e12fff13 	bx	r3
    1e60:	e1a00005 	mov	r0, r5
    1e64:	eb002d0f 	bl	d2a8 <free>
    1e68:	eafffeb0 	b	1930 <handle+0x14c>
    1e6c:	eb002775 	bl	bc48 <get_proto_factor>
    1e70:	e3e01000 	mvn	r1, #0
    1e74:	e5903014 	ldr	r3, [r0, #20]
    1e78:	e1a00004 	mov	r0, r4
    1e7c:	e1a0e00f 	mov	lr, pc
    1e80:	e12fff13 	bx	r3
    1e84:	e1a00006 	mov	r0, r6
    1e88:	e24b1088 	sub	r1, fp, #136	; 0x88
    1e8c:	e3a0205c 	mov	r2, #92	; 0x5c
    1e90:	eb0027ca 	bl	bdc0 <proto_read_to>
    1e94:	e350005c 	cmp	r0, #92	; 0x5c
    1e98:	1afffea4 	bne	1930 <handle+0x14c>
    1e9c:	e3a07000 	mov	r7, #0
    1ea0:	e51b5088 	ldr	r5, [fp, #-136]	; 0x88
    1ea4:	e1a00005 	mov	r0, r5
    1ea8:	eb002ad8 	bl	ca10 <proc_wakeup>
    1eac:	e24b102c 	sub	r1, fp, #44	; 0x2c
    1eb0:	e52170b8 	str	r7, [r1, #-184]!	; 0xb8
    1eb4:	e1a00006 	mov	r0, r6
    1eb8:	eb00279a 	bl	bd28 <proto_read>
    1ebc:	e2501000 	subs	r1, r0, #0
    1ec0:	0afffe9a 	beq	1930 <handle+0x14c>
    1ec4:	e51b6030 	ldr	r6, [fp, #-48]	; 0x30
    1ec8:	e1560007 	cmp	r6, r7
    1ecc:	0a0001e1 	beq	2658 <handle+0xe74>
    1ed0:	e1a00006 	mov	r0, r6
    1ed4:	e51b20e4 	ldr	r2, [fp, #-228]	; 0xe4
    1ed8:	eb002b07 	bl	cafc <buffer_write>
    1edc:	e3500000 	cmp	r0, #0
    1ee0:	e50b00e4 	str	r0, [fp, #-228]	; 0xe4
    1ee4:	da0001c7 	ble	2608 <handle+0xe24>
    1ee8:	eb002756 	bl	bc48 <get_proto_factor>
    1eec:	e590300c 	ldr	r3, [r0, #12]
    1ef0:	e1a00004 	mov	r0, r4
    1ef4:	e1a0e00f 	mov	lr, pc
    1ef8:	e12fff13 	bx	r3
    1efc:	e51b10e4 	ldr	r1, [fp, #-228]	; 0xe4
    1f00:	e5903014 	ldr	r3, [r0, #20]
    1f04:	e1a00004 	mov	r0, r4
    1f08:	e1a0e00f 	mov	lr, pc
    1f0c:	e12fff13 	bx	r3
    1f10:	eafffe86 	b	1930 <handle+0x14c>
    1f14:	e1a00006 	mov	r0, r6
    1f18:	eb002813 	bl	bf6c <proto_read_int>
    1f1c:	e1a05000 	mov	r5, r0
    1f20:	eb002748 	bl	bc48 <get_proto_factor>
    1f24:	e355007f 	cmp	r5, #127	; 0x7f
    1f28:	e5902014 	ldr	r2, [r0, #20]
    1f2c:	8a000182 	bhi	253c <handle+0xd58>
    1f30:	e358007f 	cmp	r8, #127	; 0x7f
    1f34:	8a00017b 	bhi	2528 <handle+0xd44>
    1f38:	e0888088 	add	r8, r8, r8, lsl #1
    1f3c:	e59f37a4 	ldr	r3, [pc, #1956]	; 26e8 <handle+0xf04>
    1f40:	e1a01488 	lsl	r1, r8, #9
    1f44:	e0855085 	add	r5, r5, r5, lsl #1
    1f48:	e08f3003 	add	r3, pc, r3
    1f4c:	e0815105 	add	r5, r1, r5, lsl #2
    1f50:	e7931005 	ldr	r1, [r3, r5]
    1f54:	e3510000 	cmp	r1, #0
    1f58:	0a000177 	beq	253c <handle+0xd58>
    1f5c:	e0833005 	add	r3, r3, r5
    1f60:	e5931008 	ldr	r1, [r3, #8]
    1f64:	eafffef5 	b	1b40 <handle+0x35c>
    1f68:	e24b7088 	sub	r7, fp, #136	; 0x88
    1f6c:	e1a00006 	mov	r0, r6
    1f70:	e1a01007 	mov	r1, r7
    1f74:	e3a0205c 	mov	r2, #92	; 0x5c
    1f78:	eb002790 	bl	bdc0 <proto_read_to>
    1f7c:	e350005c 	cmp	r0, #92	; 0x5c
    1f80:	e1a05000 	mov	r5, r0
    1f84:	1afffe69 	bne	1930 <handle+0x14c>
    1f88:	ebfffc68 	bl	1130 <vfs_new_node>
    1f8c:	e2503000 	subs	r3, r0, #0
    1f90:	0afffe66 	beq	1930 <handle+0x14c>
    1f94:	e3e0c000 	mvn	ip, #0
    1f98:	e1a01007 	mov	r1, r7
    1f9c:	e1a02005 	mov	r2, r5
    1fa0:	e2830018 	add	r0, r3, #24
    1fa4:	e50b3088 	str	r3, [fp, #-136]	; 0x88
    1fa8:	e50bc034 	str	ip, [fp, #-52]	; 0x34
    1fac:	eb002fd1 	bl	def8 <memcpy>
    1fb0:	eb002724 	bl	bc48 <get_proto_factor>
    1fb4:	e1a01007 	mov	r1, r7
    1fb8:	e5903010 	ldr	r3, [r0, #16]
    1fbc:	e1a02005 	mov	r2, r5
    1fc0:	e1a00004 	mov	r0, r4
    1fc4:	e1a0e00f 	mov	lr, pc
    1fc8:	e12fff13 	bx	r3
    1fcc:	eafffe57 	b	1930 <handle+0x14c>
    1fd0:	e24b5088 	sub	r5, fp, #136	; 0x88
    1fd4:	e3a0205c 	mov	r2, #92	; 0x5c
    1fd8:	e1a00006 	mov	r0, r6
    1fdc:	e1a01005 	mov	r1, r5
    1fe0:	eb002776 	bl	bdc0 <proto_read_to>
    1fe4:	e350005c 	cmp	r0, #92	; 0x5c
    1fe8:	e1a02000 	mov	r2, r0
    1fec:	0a000158 	beq	2554 <handle+0xd70>
    1ff0:	e3e05000 	mvn	r5, #0
    1ff4:	eb002713 	bl	bc48 <get_proto_factor>
    1ff8:	e1a01005 	mov	r1, r5
    1ffc:	e5903014 	ldr	r3, [r0, #20]
    2000:	e1a00004 	mov	r0, r4
    2004:	e1a0e00f 	mov	lr, pc
    2008:	e12fff13 	bx	r3
    200c:	eafffe47 	b	1930 <handle+0x14c>
    2010:	eb00270c 	bl	bc48 <get_proto_factor>
    2014:	e3e01000 	mvn	r1, #0
    2018:	e5903014 	ldr	r3, [r0, #20]
    201c:	e1a00004 	mov	r0, r4
    2020:	e1a0e00f 	mov	lr, pc
    2024:	e12fff13 	bx	r3
    2028:	e3a0205c 	mov	r2, #92	; 0x5c
    202c:	e1a00006 	mov	r0, r6
    2030:	e24b10e4 	sub	r1, fp, #228	; 0xe4
    2034:	eb002761 	bl	bdc0 <proto_read_to>
    2038:	e350005c 	cmp	r0, #92	; 0x5c
    203c:	e1a02000 	mov	r2, r0
    2040:	1afffe3a 	bne	1930 <handle+0x14c>
    2044:	e1a00006 	mov	r0, r6
    2048:	e24b1088 	sub	r1, fp, #136	; 0x88
    204c:	eb00275b 	bl	bdc0 <proto_read_to>
    2050:	e350005c 	cmp	r0, #92	; 0x5c
    2054:	1afffe35 	bne	1930 <handle+0x14c>
    2058:	e51b50e4 	ldr	r5, [fp, #-228]	; 0xe4
    205c:	e3550000 	cmp	r5, #0
    2060:	0afffe32 	beq	1930 <handle+0x14c>
    2064:	e1a00005 	mov	r0, r5
    2068:	e24b1084 	sub	r1, fp, #132	; 0x84
    206c:	ebfffd49 	bl	1598 <vfs_get_by_name>
    2070:	e3500000 	cmp	r0, #0
    2074:	1afffe2d 	bne	1930 <handle+0x14c>
    2078:	e51b3088 	ldr	r3, [fp, #-136]	; 0x88
    207c:	e3530000 	cmp	r3, #0
    2080:	0afffe2a 	beq	1930 <handle+0x14c>
    2084:	e5952008 	ldr	r2, [r5, #8]
    2088:	e3520000 	cmp	r2, #0
    208c:	e5835000 	str	r5, [r3]
    2090:	1582300c 	strne	r3, [r2, #12]
    2094:	15832010 	strne	r2, [r3, #16]
    2098:	e5952014 	ldr	r2, [r5, #20]
    209c:	e2822001 	add	r2, r2, #1
    20a0:	05853004 	streq	r3, [r5, #4]
    20a4:	e5853008 	str	r3, [r5, #8]
    20a8:	e5852014 	str	r2, [r5, #20]
    20ac:	eb0026e5 	bl	bc48 <get_proto_factor>
    20b0:	e590300c 	ldr	r3, [r0, #12]
    20b4:	e1a00004 	mov	r0, r4
    20b8:	e1a0e00f 	mov	lr, pc
    20bc:	e12fff13 	bx	r3
    20c0:	e3a01000 	mov	r1, #0
    20c4:	e5903014 	ldr	r3, [r0, #20]
    20c8:	e1a00004 	mov	r0, r4
    20cc:	e1a0e00f 	mov	lr, pc
    20d0:	e12fff13 	bx	r3
    20d4:	eafffe15 	b	1930 <handle+0x14c>
    20d8:	ebfffc14 	bl	1130 <vfs_new_node>
    20dc:	e1a05000 	mov	r5, r0
    20e0:	eb0026d8 	bl	bc48 <get_proto_factor>
    20e4:	e3e01000 	mvn	r1, #0
    20e8:	e5903014 	ldr	r3, [r0, #20]
    20ec:	e1a00004 	mov	r0, r4
    20f0:	e1a0e00f 	mov	lr, pc
    20f4:	e12fff13 	bx	r3
    20f8:	e3550000 	cmp	r5, #0
    20fc:	0afffe0b 	beq	1930 <handle+0x14c>
    2100:	e3a03003 	mov	r3, #3
    2104:	e3a06000 	mov	r6, #0
    2108:	e585305c 	str	r3, [r5, #92]	; 0x5c
    210c:	e5856070 	str	r6, [r5, #112]	; 0x70
    2110:	e1a00008 	mov	r0, r8
    2114:	e1a01005 	mov	r1, r5
    2118:	e3a02001 	mov	r2, #1
    211c:	ebfffbae 	bl	fdc <vfs_open>
    2120:	e2507000 	subs	r7, r0, #0
    2124:	ba000117 	blt	2588 <handle+0xda4>
    2128:	e1a00008 	mov	r0, r8
    212c:	e1a01005 	mov	r1, r5
    2130:	e3a02001 	mov	r2, #1
    2134:	ebfffba8 	bl	fdc <vfs_open>
    2138:	e2509000 	subs	r9, r0, #0
    213c:	ba00010e 	blt	257c <handle+0xd98>
    2140:	eb0026c0 	bl	bc48 <get_proto_factor>
    2144:	e590300c 	ldr	r3, [r0, #12]
    2148:	e1a00004 	mov	r0, r4
    214c:	e1a0e00f 	mov	lr, pc
    2150:	e12fff13 	bx	r3
    2154:	e1a01006 	mov	r1, r6
    2158:	e5903014 	ldr	r3, [r0, #20]
    215c:	e1a00004 	mov	r0, r4
    2160:	e1a0e00f 	mov	lr, pc
    2164:	e12fff13 	bx	r3
    2168:	e1a01007 	mov	r1, r7
    216c:	e5903014 	ldr	r3, [r0, #20]
    2170:	e1a00004 	mov	r0, r4
    2174:	e1a0e00f 	mov	lr, pc
    2178:	e12fff13 	bx	r3
    217c:	e1a01009 	mov	r1, r9
    2180:	e5903014 	ldr	r3, [r0, #20]
    2184:	e1a00004 	mov	r0, r4
    2188:	e1a0e00f 	mov	lr, pc
    218c:	e12fff13 	bx	r3
    2190:	eafffde6 	b	1930 <handle+0x14c>
    2194:	eb0026ab 	bl	bc48 <get_proto_factor>
    2198:	e3e01000 	mvn	r1, #0
    219c:	e5903014 	ldr	r3, [r0, #20]
    21a0:	e1a00004 	mov	r0, r4
    21a4:	e1a0e00f 	mov	lr, pc
    21a8:	e12fff13 	bx	r3
    21ac:	e1a00006 	mov	r0, r6
    21b0:	e24b1088 	sub	r1, fp, #136	; 0x88
    21b4:	e3a0205c 	mov	r2, #92	; 0x5c
    21b8:	eb002700 	bl	bdc0 <proto_read_to>
    21bc:	e350005c 	cmp	r0, #92	; 0x5c
    21c0:	1afffdda 	bne	1930 <handle+0x14c>
    21c4:	e1a00006 	mov	r0, r6
    21c8:	eb002767 	bl	bf6c <proto_read_int>
    21cc:	e51b1088 	ldr	r1, [fp, #-136]	; 0x88
    21d0:	e3510000 	cmp	r1, #0
    21d4:	e1a02000 	mov	r2, r0
    21d8:	0afffdd4 	beq	1930 <handle+0x14c>
    21dc:	e1a00008 	mov	r0, r8
    21e0:	ebfffb7d 	bl	fdc <vfs_open>
    21e4:	e1a05000 	mov	r5, r0
    21e8:	eb002696 	bl	bc48 <get_proto_factor>
    21ec:	e590300c 	ldr	r3, [r0, #12]
    21f0:	e1a00004 	mov	r0, r4
    21f4:	e1a0e00f 	mov	lr, pc
    21f8:	e12fff13 	bx	r3
    21fc:	eaffff7c 	b	1ff4 <handle+0x810>
    2200:	e1a00006 	mov	r0, r6
    2204:	eb002758 	bl	bf6c <proto_read_int>
    2208:	e358007f 	cmp	r8, #127	; 0x7f
    220c:	9350007f 	cmpls	r0, #127	; 0x7f
    2210:	9a0000df 	bls	2594 <handle+0xdb0>
    2214:	eb00268b 	bl	bc48 <get_proto_factor>
    2218:	e3a01000 	mov	r1, #0
    221c:	e5903014 	ldr	r3, [r0, #20]
    2220:	e1a00004 	mov	r0, r4
    2224:	e1a0e00f 	mov	lr, pc
    2228:	e12fff13 	bx	r3
    222c:	eafffdbf 	b	1930 <handle+0x14c>
    2230:	e1a00006 	mov	r0, r6
    2234:	eb00276e 	bl	bff4 <proto_read_str>
    2238:	e1a05000 	mov	r5, r0
    223c:	ebfffd63 	bl	17d0 <vfs_root>
    2240:	e1a01005 	mov	r1, r5
    2244:	ebfffcd3 	bl	1598 <vfs_get_by_name>
    2248:	e2505000 	subs	r5, r0, #0
    224c:	0affff68 	beq	1ff4 <handle+0x810>
    2250:	eb00267c 	bl	bc48 <get_proto_factor>
    2254:	e1a01005 	mov	r1, r5
    2258:	e5903014 	ldr	r3, [r0, #20]
    225c:	e1a00004 	mov	r0, r4
    2260:	e1a0e00f 	mov	lr, pc
    2264:	e12fff13 	bx	r3
    2268:	e59f647c 	ldr	r6, [pc, #1148]	; 26ec <handle+0xf08>
    226c:	e08f6006 	add	r6, pc, r6
    2270:	e2867f77 	add	r7, r6, #476	; 0x1dc
    2274:	e2851018 	add	r1, r5, #24
    2278:	e5908010 	ldr	r8, [r0, #16]
    227c:	e3a0205c 	mov	r2, #92	; 0x5c
    2280:	e1a00007 	mov	r0, r7
    2284:	eb002f1b 	bl	def8 <memcpy>
    2288:	e1a00005 	mov	r0, r5
    228c:	ebfffb9b 	bl	1100 <get_mount_pid>
    2290:	e1a01007 	mov	r1, r7
    2294:	e5860230 	str	r0, [r6, #560]	; 0x230
    2298:	e3a0205c 	mov	r2, #92	; 0x5c
    229c:	e1a00004 	mov	r0, r4
    22a0:	e1a0e00f 	mov	lr, pc
    22a4:	e12fff18 	bx	r8
    22a8:	eafffda0 	b	1930 <handle+0x14c>
    22ac:	e1a00006 	mov	r0, r6
    22b0:	eb00272d 	bl	bf6c <proto_read_int>
    22b4:	e1a09000 	mov	r9, r0
    22b8:	e1a00006 	mov	r0, r6
    22bc:	eb00272a 	bl	bf6c <proto_read_int>
    22c0:	e1a05000 	mov	r5, r0
    22c4:	eb00265f 	bl	bc48 <get_proto_factor>
    22c8:	e1590005 	cmp	r9, r5
    22cc:	e5906014 	ldr	r6, [r0, #20]
    22d0:	0a0000ca 	beq	2600 <handle+0xe1c>
    22d4:	e3590080 	cmp	r9, #128	; 0x80
    22d8:	93550080 	cmpls	r5, #128	; 0x80
    22dc:	83a07001 	movhi	r7, #1
    22e0:	93a07000 	movls	r7, #0
    22e4:	8a0000a2 	bhi	2574 <handle+0xd90>
    22e8:	e1a00008 	mov	r0, r8
    22ec:	e1a01005 	mov	r1, r5
    22f0:	ebfffc52 	bl	1440 <vfs_close>
    22f4:	e358007f 	cmp	r8, #127	; 0x7f
    22f8:	93a00000 	movls	r0, #0
    22fc:	83a00001 	movhi	r0, #1
    2300:	e3590080 	cmp	r9, #128	; 0x80
    2304:	03800001 	orreq	r0, r0, #1
    2308:	e3500000 	cmp	r0, #0
    230c:	1a00008c 	bne	2544 <handle+0xd60>
    2310:	e1a07088 	lsl	r7, r8, #1
    2314:	e0873008 	add	r3, r7, r8
    2318:	e1a0a089 	lsl	sl, r9, #1
    231c:	e59f23cc 	ldr	r2, [pc, #972]	; 26f0 <handle+0xf0c>
    2320:	e1a03483 	lsl	r3, r3, #9
    2324:	e08a1009 	add	r1, sl, r9
    2328:	e08f2002 	add	r2, pc, r2
    232c:	e0831101 	add	r1, r3, r1, lsl #2
    2330:	e792c001 	ldr	ip, [r2, r1]
    2334:	e35c0000 	cmp	ip, #0
    2338:	e0821001 	add	r1, r2, r1
    233c:	0a00008c 	beq	2574 <handle+0xd90>
    2340:	e3550080 	cmp	r5, #128	; 0x80
    2344:	10850085 	addne	r0, r5, r5, lsl #1
    2348:	10833100 	addne	r3, r3, r0, lsl #2
    234c:	10820003 	addne	r0, r2, r3
    2350:	e3a0200c 	mov	r2, #12
    2354:	eb002ee7 	bl	def8 <memcpy>
    2358:	e0873008 	add	r3, r7, r8
    235c:	e59f2390 	ldr	r2, [pc, #912]	; 26f4 <handle+0xf10>
    2360:	e08a1009 	add	r1, sl, r9
    2364:	e1a03483 	lsl	r3, r3, #9
    2368:	e08f2002 	add	r2, pc, r2
    236c:	e0833101 	add	r3, r3, r1, lsl #2
    2370:	e7b23003 	ldr	r3, [r2, r3]!
    2374:	e5921004 	ldr	r1, [r2, #4]
    2378:	e5932078 	ldr	r2, [r3, #120]	; 0x78
    237c:	e3110002 	tst	r1, #2
    2380:	e2822001 	add	r2, r2, #1
    2384:	e5832078 	str	r2, [r3, #120]	; 0x78
    2388:	0a00006f 	beq	254c <handle+0xd68>
    238c:	e1a01005 	mov	r1, r5
    2390:	e593207c 	ldr	r2, [r3, #124]	; 0x7c
    2394:	e2822001 	add	r2, r2, #1
    2398:	e583207c 	str	r2, [r3, #124]	; 0x7c
    239c:	e1a00004 	mov	r0, r4
    23a0:	e1a0e00f 	mov	lr, pc
    23a4:	e12fff16 	bx	r6
    23a8:	eafffd60 	b	1930 <handle+0x14c>
    23ac:	e1a00006 	mov	r0, r6
    23b0:	eb0026ed 	bl	bf6c <proto_read_int>
    23b4:	e1a09000 	mov	r9, r0
    23b8:	eb002622 	bl	bc48 <get_proto_factor>
    23bc:	e3590080 	cmp	r9, #128	; 0x80
    23c0:	e5906014 	ldr	r6, [r0, #20]
    23c4:	8a00006a 	bhi	2574 <handle+0xd90>
    23c8:	e59f3328 	ldr	r3, [pc, #808]	; 26f8 <handle+0xf14>
    23cc:	e1a07088 	lsl	r7, r8, #1
    23d0:	e08f3003 	add	r3, pc, r3
    23d4:	e0872008 	add	r2, r7, r8
    23d8:	e3a05003 	mov	r5, #3
    23dc:	e0833482 	add	r3, r3, r2, lsl #9
    23e0:	ea000003 	b	23f4 <handle+0xc10>
    23e4:	e2855001 	add	r5, r5, #1
    23e8:	e3550080 	cmp	r5, #128	; 0x80
    23ec:	e283300c 	add	r3, r3, #12
    23f0:	0a00005f 	beq	2574 <handle+0xd90>
    23f4:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    23f8:	e3520000 	cmp	r2, #0
    23fc:	1afffff8 	bne	23e4 <handle+0xc00>
    2400:	e358007f 	cmp	r8, #127	; 0x7f
    2404:	93a03000 	movls	r3, #0
    2408:	83a03001 	movhi	r3, #1
    240c:	e3590080 	cmp	r9, #128	; 0x80
    2410:	03833001 	orreq	r3, r3, #1
    2414:	e3530000 	cmp	r3, #0
    2418:	1a000045 	bne	2534 <handle+0xd50>
    241c:	e0873008 	add	r3, r7, r8
    2420:	e59f82d4 	ldr	r8, [pc, #724]	; 26fc <handle+0xf18>
    2424:	e0899089 	add	r9, r9, r9, lsl #1
    2428:	e1a03483 	lsl	r3, r3, #9
    242c:	e08f8008 	add	r8, pc, r8
    2430:	e0837109 	add	r7, r3, r9, lsl #2
    2434:	e7982007 	ldr	r2, [r8, r7]
    2438:	e3520000 	cmp	r2, #0
    243c:	e0889007 	add	r9, r8, r7
    2440:	0a00004b 	beq	2574 <handle+0xd90>
    2444:	e0852085 	add	r2, r5, r5, lsl #1
    2448:	e0830102 	add	r0, r3, r2, lsl #2
    244c:	e1a01009 	mov	r1, r9
    2450:	e3a0200c 	mov	r2, #12
    2454:	e0880000 	add	r0, r8, r0
    2458:	eb002ea6 	bl	def8 <memcpy>
    245c:	e7982007 	ldr	r2, [r8, r7]
    2460:	e5991004 	ldr	r1, [r9, #4]
    2464:	e5923078 	ldr	r3, [r2, #120]	; 0x78
    2468:	e3110002 	tst	r1, #2
    246c:	e2833001 	add	r3, r3, #1
    2470:	e5823078 	str	r3, [r2, #120]	; 0x78
    2474:	0a000034 	beq	254c <handle+0xd68>
    2478:	e592307c 	ldr	r3, [r2, #124]	; 0x7c
    247c:	e2833001 	add	r3, r3, #1
    2480:	e1a01005 	mov	r1, r5
    2484:	e582307c 	str	r3, [r2, #124]	; 0x7c
    2488:	eaffffc3 	b	239c <handle+0xbb8>
    248c:	e1a00006 	mov	r0, r6
    2490:	eb0026b5 	bl	bf6c <proto_read_int>
    2494:	e1a07000 	mov	r7, r0
    2498:	e1a00006 	mov	r0, r6
    249c:	eb0026b2 	bl	bf6c <proto_read_int>
    24a0:	e1a05000 	mov	r5, r0
    24a4:	eb0025e7 	bl	bc48 <get_proto_factor>
    24a8:	e357007f 	cmp	r7, #127	; 0x7f
    24ac:	e5902014 	ldr	r2, [r0, #20]
    24b0:	8a000021 	bhi	253c <handle+0xd58>
    24b4:	e358007f 	cmp	r8, #127	; 0x7f
    24b8:	8a00001a 	bhi	2528 <handle+0xd44>
    24bc:	e0888088 	add	r8, r8, r8, lsl #1
    24c0:	e59f3238 	ldr	r3, [pc, #568]	; 2700 <handle+0xf1c>
    24c4:	e1a01488 	lsl	r1, r8, #9
    24c8:	e0877087 	add	r7, r7, r7, lsl #1
    24cc:	e08f3003 	add	r3, pc, r3
    24d0:	e0817107 	add	r7, r1, r7, lsl #2
    24d4:	e7931007 	ldr	r1, [r3, r7]
    24d8:	e3510000 	cmp	r1, #0
    24dc:	0a000016 	beq	253c <handle+0xd58>
    24e0:	e0833007 	add	r3, r3, r7
    24e4:	e5835008 	str	r5, [r3, #8]
    24e8:	e1a01005 	mov	r1, r5
    24ec:	eafffd93 	b	1b40 <handle+0x35c>
    24f0:	e1a00006 	mov	r0, r6
    24f4:	eb00269c 	bl	bf6c <proto_read_int>
    24f8:	e2501000 	subs	r1, r0, #0
    24fc:	bafffd0b 	blt	1930 <handle+0x14c>
    2500:	e1a00008 	mov	r0, r8
    2504:	ebfffbcd 	bl	1440 <vfs_close>
    2508:	eafffd08 	b	1930 <handle+0x14c>
    250c:	eb0025cd 	bl	bc48 <get_proto_factor>
    2510:	e3e01000 	mvn	r1, #0
    2514:	e5903014 	ldr	r3, [r0, #20]
    2518:	e1a00004 	mov	r0, r4
    251c:	e1a0e00f 	mov	lr, pc
    2520:	e12fff13 	bx	r3
    2524:	eafffd01 	b	1930 <handle+0x14c>
    2528:	e3a03000 	mov	r3, #0
    252c:	e5933000 	ldr	r3, [r3]
    2530:	e7f000f0 	udf	#0
    2534:	e5923000 	ldr	r3, [r2]
    2538:	e7f000f0 	udf	#0
    253c:	e3e01000 	mvn	r1, #0
    2540:	eafffd7e 	b	1b40 <handle+0x35c>
    2544:	e5973000 	ldr	r3, [r7]
    2548:	e7f000f0 	udf	#0
    254c:	e1a01005 	mov	r1, r5
    2550:	eaffff91 	b	239c <handle+0xbb8>
    2554:	e51b0088 	ldr	r0, [fp, #-136]	; 0x88
    2558:	e3500000 	cmp	r0, #0
    255c:	0afffea3 	beq	1ff0 <handle+0x80c>
    2560:	e1a01005 	mov	r1, r5
    2564:	e2800018 	add	r0, r0, #24
    2568:	eb002e62 	bl	def8 <memcpy>
    256c:	e3a05000 	mov	r5, #0
    2570:	eafffe9f 	b	1ff4 <handle+0x810>
    2574:	e3e01000 	mvn	r1, #0
    2578:	eaffff87 	b	239c <handle+0xbb8>
    257c:	e1a00008 	mov	r0, r8
    2580:	e1a01007 	mov	r1, r7
    2584:	ebfffbad 	bl	1440 <vfs_close>
    2588:	e1a00005 	mov	r0, r5
    258c:	eb002b45 	bl	d2a8 <free>
    2590:	eafffce6 	b	1930 <handle+0x14c>
    2594:	e0888088 	add	r8, r8, r8, lsl #1
    2598:	e59f2164 	ldr	r2, [pc, #356]	; 2704 <handle+0xf20>
    259c:	e1a03488 	lsl	r3, r8, #9
    25a0:	e0800080 	add	r0, r0, r0, lsl #1
    25a4:	e08f2002 	add	r2, pc, r2
    25a8:	e0830100 	add	r0, r3, r0, lsl #2
    25ac:	e7927000 	ldr	r7, [r2, r0]
    25b0:	e3570000 	cmp	r7, #0
    25b4:	0affff16 	beq	2214 <handle+0xa30>
    25b8:	eb0025a2 	bl	bc48 <get_proto_factor>
    25bc:	e59f5144 	ldr	r5, [pc, #324]	; 2708 <handle+0xf24>
    25c0:	e08f5005 	add	r5, pc, r5
    25c4:	e2856f77 	add	r6, r5, #476	; 0x1dc
    25c8:	e2871018 	add	r1, r7, #24
    25cc:	e5908010 	ldr	r8, [r0, #16]
    25d0:	e3a0205c 	mov	r2, #92	; 0x5c
    25d4:	e1a00006 	mov	r0, r6
    25d8:	eb002e46 	bl	def8 <memcpy>
    25dc:	e1a00007 	mov	r0, r7
    25e0:	ebfffac6 	bl	1100 <get_mount_pid>
    25e4:	e1a01006 	mov	r1, r6
    25e8:	e5850230 	str	r0, [r5, #560]	; 0x230
    25ec:	e3a0205c 	mov	r2, #92	; 0x5c
    25f0:	e1a00004 	mov	r0, r4
    25f4:	e1a0e00f 	mov	lr, pc
    25f8:	e12fff18 	bx	r8
    25fc:	eafffccb 	b	1930 <handle+0x14c>
    2600:	e1a01009 	mov	r1, r9
    2604:	eaffff64 	b	239c <handle+0xbb8>
    2608:	e3550000 	cmp	r5, #0
    260c:	0afffcc7 	beq	1930 <handle+0x14c>
    2610:	e5953078 	ldr	r3, [r5, #120]	; 0x78
    2614:	e3530001 	cmp	r3, #1
    2618:	9afffcc4 	bls	1930 <handle+0x14c>
    261c:	eb002589 	bl	bc48 <get_proto_factor>
    2620:	e590300c 	ldr	r3, [r0, #12]
    2624:	e1a00004 	mov	r0, r4
    2628:	e1a0e00f 	mov	lr, pc
    262c:	e12fff13 	bx	r3
    2630:	e1a01007 	mov	r1, r7
    2634:	e5903014 	ldr	r3, [r0, #20]
    2638:	e1a00004 	mov	r0, r4
    263c:	e1a0e00f 	mov	lr, pc
    2640:	e12fff13 	bx	r3
    2644:	eafffcb9 	b	1930 <handle+0x14c>
    2648:	e59f10bc 	ldr	r1, [pc, #188]	; 270c <handle+0xf28>
    264c:	e08f1001 	add	r1, pc, r1
    2650:	e58151d8 	str	r5, [r1, #472]	; 0x1d8
    2654:	eafffcad 	b	1910 <handle+0x12c>
    2658:	eb00257a 	bl	bc48 <get_proto_factor>
    265c:	e590300c 	ldr	r3, [r0, #12]
    2660:	e1a00004 	mov	r0, r4
    2664:	e1a0e00f 	mov	lr, pc
    2668:	e12fff13 	bx	r3
    266c:	e1a01006 	mov	r1, r6
    2670:	e5903014 	ldr	r3, [r0, #20]
    2674:	e1a00004 	mov	r0, r4
    2678:	e1a0e00f 	mov	lr, pc
    267c:	e12fff13 	bx	r3
    2680:	eafffcaa 	b	1930 <handle+0x14c>
    2684:	e1a00005 	mov	r0, r5
    2688:	eb002b06 	bl	d2a8 <free>
    268c:	e3570000 	cmp	r7, #0
    2690:	0afffca6 	beq	1930 <handle+0x14c>
    2694:	e5973078 	ldr	r3, [r7, #120]	; 0x78
    2698:	e3530001 	cmp	r3, #1
    269c:	9afffca3 	bls	1930 <handle+0x14c>
    26a0:	eafffe81 	b	20ac <handle+0x8c8>
    26a4:	e1a00005 	mov	r0, r5
    26a8:	eb002afe 	bl	d2a8 <free>
    26ac:	e5943074 	ldr	r3, [r4, #116]	; 0x74
    26b0:	e1a02303 	lsl	r2, r3, #6
    26b4:	eafffc95 	b	1910 <handle+0x12c>
    26b8:	000465f4 	strdeq	r6, [r4], -r4
    26bc:	00046588 	andeq	r6, r4, r8, lsl #11
    26c0:	00016600 	andeq	r6, r1, r0, lsl #12
    26c4:	00016580 	andeq	r6, r1, r0, lsl #11
    26c8:	00000408 	andeq	r0, r0, r8, lsl #8
    26cc:	000164a0 	andeq	r6, r1, r0, lsr #9
    26d0:	0001642c 	andeq	r6, r1, ip, lsr #8
    26d4:	000462b8 			; <UNDEFINED> instruction: 0x000462b8
    26d8:	00046280 	andeq	r6, r4, r0, lsl #5
    26dc:	0000d8c8 	andeq	sp, r0, r8, asr #17
    26e0:	0004a1d8 	ldrdeq	sl, [r4], -r8
    26e4:	000001ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    26e8:	00016014 	andeq	r6, r1, r4, lsl r0
    26ec:	00049c20 	andeq	r9, r4, r0, lsr #24
    26f0:	00015c34 	andeq	r5, r1, r4, lsr ip
    26f4:	00015bf4 	strdeq	r5, [r1], -r4
    26f8:	00015b8c 	andeq	r5, r1, ip, lsl #23
    26fc:	00015b30 	andeq	r5, r1, r0, lsr fp
    2700:	00015a90 	muleq	r1, r0, sl
    2704:	000159b8 			; <UNDEFINED> instruction: 0x000159b8
    2708:	000498cc 	andeq	r9, r4, ip, asr #17
    270c:	00049840 	andeq	r9, r4, r0, asr #16

00002710 <vfsd_main>:
    2710:	e1a0c00d 	mov	ip, sp
    2714:	e3a02000 	mov	r2, #0
    2718:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    271c:	e59f31c4 	ldr	r3, [pc, #452]	; 28e8 <vfsd_main+0x1d8>
    2720:	e59f01c4 	ldr	r0, [pc, #452]	; 28ec <vfsd_main+0x1dc>
    2724:	e24cb004 	sub	fp, ip, #4
    2728:	e24dd08c 	sub	sp, sp, #140	; 0x8c
    272c:	e08f3003 	add	r3, pc, r3
    2730:	e08f0000 	add	r0, pc, r0
    2734:	e58321d0 	str	r2, [r3, #464]	; 0x1d0
    2738:	e58321d4 	str	r2, [r3, #468]	; 0x1d4
    273c:	eb002011 	bl	a788 <ipc_serv_reg>
    2740:	e3500000 	cmp	r0, #0
    2744:	1a000060 	bne	28cc <vfsd_main+0x1bc>
    2748:	e59f41a0 	ldr	r4, [pc, #416]	; 28f0 <vfsd_main+0x1e0>
    274c:	e08f4004 	add	r4, pc, r4
    2750:	e28440c0 	add	r4, r4, #192	; 0xc0
    2754:	e2845c41 	add	r5, r4, #16640	; 0x4100
    2758:	e1a00004 	mov	r0, r4
    275c:	e3a01000 	mov	r1, #0
    2760:	e2844f82 	add	r4, r4, #520	; 0x208
    2764:	e3a02f82 	mov	r2, #520	; 0x208
    2768:	eb002e1e 	bl	dfe8 <memset>
    276c:	e1540005 	cmp	r4, r5
    2770:	1afffff8 	bne	2758 <vfsd_main+0x48>
    2774:	e59f4178 	ldr	r4, [pc, #376]	; 28f4 <vfsd_main+0x1e4>
    2778:	e08f4004 	add	r4, pc, r4
    277c:	e2846803 	add	r6, r4, #196608	; 0x30000
    2780:	e2845c06 	add	r5, r4, #1536	; 0x600
    2784:	e1a00004 	mov	r0, r4
    2788:	e3a01000 	mov	r1, #0
    278c:	e284400c 	add	r4, r4, #12
    2790:	e3a0200c 	mov	r2, #12
    2794:	eb002e13 	bl	dfe8 <memset>
    2798:	e1550004 	cmp	r5, r4
    279c:	1afffff8 	bne	2784 <vfsd_main+0x74>
    27a0:	e1560005 	cmp	r6, r5
    27a4:	e1a04005 	mov	r4, r5
    27a8:	1afffff4 	bne	2780 <vfsd_main+0x70>
    27ac:	ebfffa5f 	bl	1130 <vfs_new_node>
    27b0:	e1a03000 	mov	r3, r0
    27b4:	e59f613c 	ldr	r6, [pc, #316]	; 28f8 <vfsd_main+0x1e8>
    27b8:	e59f113c 	ldr	r1, [pc, #316]	; 28fc <vfsd_main+0x1ec>
    27bc:	e3a02002 	mov	r2, #2
    27c0:	e08f1001 	add	r1, pc, r1
    27c4:	e08f6006 	add	r6, pc, r6
    27c8:	e280001c 	add	r0, r0, #28
    27cc:	e58631d8 	str	r3, [r6, #472]	; 0x1d8
    27d0:	eb002dc8 	bl	def8 <memcpy>
    27d4:	e59f0124 	ldr	r0, [pc, #292]	; 2900 <vfsd_main+0x1f0>
    27d8:	e3a01000 	mov	r1, #0
    27dc:	e08f0000 	add	r0, pc, r0
    27e0:	e3a02001 	mov	r2, #1
    27e4:	eb0020c2 	bl	aaf4 <ipc_serv_run>
    27e8:	e24b8090 	sub	r8, fp, #144	; 0x90
    27ec:	e24b40a8 	sub	r4, fp, #168	; 0xa8
    27f0:	e24b7084 	sub	r7, fp, #132	; 0x84
    27f4:	ea00002b 	b	28a8 <vfsd_main+0x198>
    27f8:	eb001f52 	bl	a548 <ipc_lock>
    27fc:	e59631d4 	ldr	r3, [r6, #468]	; 0x1d4
    2800:	e5933068 	ldr	r3, [r3, #104]	; 0x68
    2804:	e3a0206c 	mov	r2, #108	; 0x6c
    2808:	e3530000 	cmp	r3, #0
    280c:	e1a01005 	mov	r1, r5
    2810:	e1a00008 	mov	r0, r8
    2814:	058631d0 	streq	r3, [r6, #464]	; 0x1d0
    2818:	e58631d4 	str	r3, [r6, #468]	; 0x1d4
    281c:	eb002db5 	bl	def8 <memcpy>
    2820:	e1a00005 	mov	r0, r5
    2824:	eb002a9f 	bl	d2a8 <free>
    2828:	eb001f4e 	bl	a568 <ipc_unlock>
    282c:	eb002505 	bl	bc48 <get_proto_factor>
    2830:	e5903004 	ldr	r3, [r0, #4]
    2834:	e1a00004 	mov	r0, r4
    2838:	e1a0e00f 	mov	lr, pc
    283c:	e12fff13 	bx	r3
    2840:	e51b1090 	ldr	r1, [fp, #-144]	; 0x90
    2844:	e5903014 	ldr	r3, [r0, #20]
    2848:	e1a00004 	mov	r0, r4
    284c:	e1a0e00f 	mov	lr, pc
    2850:	e12fff13 	bx	r3
    2854:	e51b108c 	ldr	r1, [fp, #-140]	; 0x8c
    2858:	e5903014 	ldr	r3, [r0, #20]
    285c:	e1a00004 	mov	r0, r4
    2860:	e1a0e00f 	mov	lr, pc
    2864:	e12fff13 	bx	r3
    2868:	e1a01007 	mov	r1, r7
    286c:	e5903010 	ldr	r3, [r0, #16]
    2870:	e3a0205c 	mov	r2, #92	; 0x5c
    2874:	e1a00004 	mov	r0, r4
    2878:	e1a0e00f 	mov	lr, pc
    287c:	e12fff13 	bx	r3
    2880:	e3a03000 	mov	r3, #0
    2884:	e3a01003 	mov	r1, #3
    2888:	e1a02004 	mov	r2, r4
    288c:	e51b0088 	ldr	r0, [fp, #-136]	; 0x88
    2890:	eb001f3c 	bl	a588 <ipc_call>
    2894:	eb0024eb 	bl	bc48 <get_proto_factor>
    2898:	e590300c 	ldr	r3, [r0, #12]
    289c:	e1a00004 	mov	r0, r4
    28a0:	e1a0e00f 	mov	lr, pc
    28a4:	e12fff13 	bx	r3
    28a8:	e59651d4 	ldr	r5, [r6, #468]	; 0x1d4
    28ac:	e3550000 	cmp	r5, #0
    28b0:	1affffd0 	bne	27f8 <vfsd_main+0xe8>
    28b4:	eb002924 	bl	cd4c <getpid>
    28b8:	e59611d8 	ldr	r1, [r6, #472]	; 0x1d8
    28bc:	eb002848 	bl	c9e4 <proc_block>
    28c0:	e1a00005 	mov	r0, r5
    28c4:	eb00298d 	bl	cf00 <sleep>
    28c8:	eafffff6 	b	28a8 <vfsd_main+0x198>
    28cc:	e59f0030 	ldr	r0, [pc, #48]	; 2904 <vfsd_main+0x1f4>
    28d0:	e08f0000 	add	r0, pc, r0
    28d4:	eb001678 	bl	82bc <klog>
    28d8:	e3e00000 	mvn	r0, #0
    28dc:	e24bd020 	sub	sp, fp, #32
    28e0:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    28e4:	e12fff1e 	bx	lr
    28e8:	00049760 	andeq	r9, r4, r0, ror #14
    28ec:	0000cd34 	andeq	ip, r0, r4, lsr sp
    28f0:	00045750 	andeq	r5, r4, r0, asr r7
    28f4:	000157e4 	andeq	r5, r1, r4, ror #15
    28f8:	000496c8 	andeq	r9, r4, r8, asr #13
    28fc:	0000cca0 	andeq	ip, r0, r0, lsr #25
    2900:	fffff000 			; <UNDEFINED> instruction: 0xfffff000
    2904:	0000cba4 	andeq	ip, r0, r4, lsr #23

00002908 <memfs_read>:
    2908:	e1a0c00d 	mov	ip, sp
    290c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    2910:	e24cb004 	sub	fp, ip, #4
    2914:	e5924048 	ldr	r4, [r2, #72]	; 0x48
    2918:	e59b1008 	ldr	r1, [fp, #8]
    291c:	e59b0004 	ldr	r0, [fp, #4]
    2920:	e0614004 	rsb	r4, r1, r4
    2924:	e1540000 	cmp	r4, r0
    2928:	a1a04000 	movge	r4, r0
    292c:	e3540000 	cmp	r4, #0
    2930:	e5922058 	ldr	r2, [r2, #88]	; 0x58
    2934:	ba000007 	blt	2958 <memfs_read+0x50>
    2938:	10821001 	addne	r1, r2, r1
    293c:	11a00003 	movne	r0, r3
    2940:	11a02004 	movne	r2, r4
    2944:	1b002d6b 	blne	def8 <memcpy>
    2948:	e1a00004 	mov	r0, r4
    294c:	e24bd014 	sub	sp, fp, #20
    2950:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    2954:	e12fff1e 	bx	lr
    2958:	e3e04000 	mvn	r4, #0
    295c:	eafffff9 	b	2948 <memfs_read+0x40>

00002960 <add_nodes>:
    2960:	e1a0c00d 	mov	ip, sp
    2964:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    2968:	e24cb004 	sub	fp, ip, #4
    296c:	e24ddf9b 	sub	sp, sp, #620	; 0x26c
    2970:	e1a07000 	mov	r7, r0
    2974:	e1a04001 	mov	r4, r1
    2978:	e24b9e23 	sub	r9, fp, #560	; 0x230
    297c:	e24b5fa3 	sub	r5, fp, #652	; 0x28c
    2980:	e24b6fa2 	sub	r6, fp, #648	; 0x288
    2984:	e3a0002f 	mov	r0, #47	; 0x2f
    2988:	e1a01004 	mov	r1, r4
    298c:	e1a02009 	mov	r2, r9
    2990:	e3a03000 	mov	r3, #0
    2994:	eb001469 	bl	7b40 <syscall3>
    2998:	e2508000 	subs	r8, r0, #0
    299c:	ba000053 	blt	2af0 <add_nodes+0x190>
    29a0:	e55b3230 	ldrb	r3, [fp, #-560]	; 0x230
    29a4:	e3530000 	cmp	r3, #0
    29a8:	0a000026 	beq	2a48 <add_nodes+0xe8>
    29ac:	e1a00009 	mov	r0, r9
    29b0:	eb002ee6 	bl	e550 <strlen>
    29b4:	e2400001 	sub	r0, r0, #1
    29b8:	e7d93000 	ldrb	r3, [r9, r0]
    29bc:	e353002f 	cmp	r3, #47	; 0x2f
    29c0:	e0890000 	add	r0, r9, r0
    29c4:	1a000003 	bne	29d8 <add_nodes+0x78>
    29c8:	ea000041 	b	2ad4 <add_nodes+0x174>
    29cc:	e5703001 	ldrb	r3, [r0, #-1]!
    29d0:	e353002f 	cmp	r3, #47	; 0x2f
    29d4:	0a00003e 	beq	2ad4 <add_nodes+0x174>
    29d8:	e1500009 	cmp	r0, r9
    29dc:	1afffffa 	bne	29cc <add_nodes+0x6c>
    29e0:	e3580000 	cmp	r8, #0
    29e4:	1a00001d 	bne	2a60 <add_nodes+0x100>
    29e8:	e1a0a009 	mov	sl, r9
    29ec:	e3a0205c 	mov	r2, #92	; 0x5c
    29f0:	e3a01000 	mov	r1, #0
    29f4:	e1a00005 	mov	r0, r5
    29f8:	eb002d7a 	bl	dfe8 <memset>
    29fc:	e1a0100a 	mov	r1, sl
    2a00:	e1a00006 	mov	r0, r6
    2a04:	eb002e95 	bl	e460 <strcpy>
    2a08:	e3a03b01 	mov	r3, #1024	; 0x400
    2a0c:	e3a02000 	mov	r2, #0
    2a10:	e1a00005 	mov	r0, r5
    2a14:	e50b2248 	str	r2, [fp, #-584]	; 0x248
    2a18:	e50b3244 	str	r3, [fp, #-580]	; 0x244
    2a1c:	eb0016bd 	bl	8518 <vfs_new_node>
    2a20:	e1a00007 	mov	r0, r7
    2a24:	e1a01005 	mov	r1, r5
    2a28:	eb0018a6 	bl	8cc8 <vfs_add>
    2a2c:	e3500000 	cmp	r0, #0
    2a30:	1a000032 	bne	2b00 <add_nodes+0x1a0>
    2a34:	e2841001 	add	r1, r4, #1
    2a38:	e1a00005 	mov	r0, r5
    2a3c:	ebffffc7 	bl	2960 <add_nodes>
    2a40:	e1a04000 	mov	r4, r0
    2a44:	eaffffce 	b	2984 <add_nodes+0x24>
    2a48:	e3580000 	cmp	r8, #0
    2a4c:	1a000003 	bne	2a60 <add_nodes+0x100>
    2a50:	e2840001 	add	r0, r4, #1
    2a54:	e24bd028 	sub	sp, fp, #40	; 0x28
    2a58:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    2a5c:	e12fff1e 	bx	lr
    2a60:	e1a0a009 	mov	sl, r9
    2a64:	e1a00008 	mov	r0, r8
    2a68:	eb002a17 	bl	d2cc <malloc>
    2a6c:	e1a01004 	mov	r1, r4
    2a70:	e1a03000 	mov	r3, r0
    2a74:	e1a08000 	mov	r8, r0
    2a78:	e3a02000 	mov	r2, #0
    2a7c:	e3a0002f 	mov	r0, #47	; 0x2f
    2a80:	eb00142e 	bl	7b40 <syscall3>
    2a84:	e3a0205c 	mov	r2, #92	; 0x5c
    2a88:	e50b0290 	str	r0, [fp, #-656]	; 0x290
    2a8c:	e3a01000 	mov	r1, #0
    2a90:	e1a00005 	mov	r0, r5
    2a94:	eb002d53 	bl	dfe8 <memset>
    2a98:	e1a0100a 	mov	r1, sl
    2a9c:	e1a00006 	mov	r0, r6
    2aa0:	eb002e6e 	bl	e460 <strcpy>
    2aa4:	e3a02001 	mov	r2, #1
    2aa8:	e51b3290 	ldr	r3, [fp, #-656]	; 0x290
    2aac:	e1a00005 	mov	r0, r5
    2ab0:	e50b3244 	str	r3, [fp, #-580]	; 0x244
    2ab4:	e50b8234 	str	r8, [fp, #-564]	; 0x234
    2ab8:	e50b2248 	str	r2, [fp, #-584]	; 0x248
    2abc:	eb001695 	bl	8518 <vfs_new_node>
    2ac0:	e1a00007 	mov	r0, r7
    2ac4:	e1a01005 	mov	r1, r5
    2ac8:	e2844001 	add	r4, r4, #1
    2acc:	eb00187d 	bl	8cc8 <vfs_add>
    2ad0:	eaffffab 	b	2984 <add_nodes+0x24>
    2ad4:	e3580000 	cmp	r8, #0
    2ad8:	e280a001 	add	sl, r0, #1
    2adc:	1affffe0 	bne	2a64 <add_nodes+0x104>
    2ae0:	e5d03001 	ldrb	r3, [r0, #1]
    2ae4:	e3530000 	cmp	r3, #0
    2ae8:	1affffbf 	bne	29ec <add_nodes+0x8c>
    2aec:	eaffffd7 	b	2a50 <add_nodes+0xf0>
    2af0:	e3e00000 	mvn	r0, #0
    2af4:	e24bd028 	sub	sp, fp, #40	; 0x28
    2af8:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    2afc:	e12fff1e 	bx	lr
    2b00:	e1a00005 	mov	r0, r5
    2b04:	eb0018a6 	bl	8da4 <vfs_del>
    2b08:	eaffffc9 	b	2a34 <add_nodes+0xd4>

00002b0c <memfs_mount>:
    2b0c:	e1a0c00d 	mov	ip, sp
    2b10:	e3a01000 	mov	r1, #0
    2b14:	e92dd800 	push	{fp, ip, lr, pc}
    2b18:	e24cb004 	sub	fp, ip, #4
    2b1c:	ebffff8f 	bl	2960 <add_nodes>
    2b20:	e3a00000 	mov	r0, #0
    2b24:	e24bd00c 	sub	sp, fp, #12
    2b28:	e89d6800 	ldm	sp, {fp, sp, lr}
    2b2c:	e12fff1e 	bx	lr

00002b30 <romfsd_main>:
    2b30:	e1a0c00d 	mov	ip, sp
    2b34:	e92dd810 	push	{r4, fp, ip, lr, pc}
    2b38:	e24cb004 	sub	fp, ip, #4
    2b3c:	e24dd094 	sub	sp, sp, #148	; 0x94
    2b40:	e24b40a0 	sub	r4, fp, #160	; 0xa0
    2b44:	e1a00004 	mov	r0, r4
    2b48:	e3a01000 	mov	r1, #0
    2b4c:	e3a0208c 	mov	r2, #140	; 0x8c
    2b50:	eb002d24 	bl	dfe8 <memset>
    2b54:	e59f1048 	ldr	r1, [pc, #72]	; 2ba4 <romfsd_main+0x74>
    2b58:	e24b009f 	sub	r0, fp, #159	; 0x9f
    2b5c:	e08f1001 	add	r1, pc, r1
    2b60:	e3a0200c 	mov	r2, #12
    2b64:	eb002ce3 	bl	def8 <memcpy>
    2b68:	e59fc038 	ldr	ip, [pc, #56]	; 2ba8 <romfsd_main+0x78>
    2b6c:	e59f3038 	ldr	r3, [pc, #56]	; 2bac <romfsd_main+0x7c>
    2b70:	e59f1038 	ldr	r1, [pc, #56]	; 2bb0 <romfsd_main+0x80>
    2b74:	e08fc00c 	add	ip, pc, ip
    2b78:	e08f3003 	add	r3, pc, r3
    2b7c:	e1a00004 	mov	r0, r4
    2b80:	e08f1001 	add	r1, pc, r1
    2b84:	e3a02000 	mov	r2, #0
    2b88:	e50bc02c 	str	ip, [fp, #-44]	; 0x2c
    2b8c:	e50b3048 	str	r3, [fp, #-72]	; 0x48
    2b90:	eb002264 	bl	b528 <device_run>
    2b94:	e3a00000 	mov	r0, #0
    2b98:	e24bd010 	sub	sp, fp, #16
    2b9c:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
    2ba0:	e12fff1e 	bx	lr
    2ba4:	0000c934 	andeq	ip, r0, r4, lsr r9
    2ba8:	ffffff90 			; <UNDEFINED> instruction: 0xffffff90
    2bac:	fffffd88 			; <UNDEFINED> instruction: 0xfffffd88
    2bb0:	0000c8e0 	andeq	ip, r0, r0, ror #17

00002bb4 <sdext2_unlink>:
    2bb4:	e1a0c00d 	mov	ip, sp
    2bb8:	e1a00002 	mov	r0, r2
    2bbc:	e92dd800 	push	{fp, ip, lr, pc}
    2bc0:	e24cb004 	sub	fp, ip, #4
    2bc4:	eb0006ca 	bl	46f4 <ext2_unlink>
    2bc8:	e24bd00c 	sub	sp, fp, #12
    2bcc:	e89d6800 	ldm	sp, {fp, sp, lr}
    2bd0:	e12fff1e 	bx	lr

00002bd4 <sdext2_create>:
    2bd4:	e1a0c00d 	mov	ip, sp
    2bd8:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    2bdc:	e24cb004 	sub	fp, ip, #4
    2be0:	e24dd084 	sub	sp, sp, #132	; 0x84
    2be4:	e5904058 	ldr	r4, [r0, #88]	; 0x58
    2be8:	e3540000 	cmp	r4, #0
    2bec:	03a04002 	moveq	r4, #2
    2bf0:	e24b60a4 	sub	r6, fp, #164	; 0xa4
    2bf4:	e1a00002 	mov	r0, r2
    2bf8:	e1a07002 	mov	r7, r2
    2bfc:	e1a05001 	mov	r5, r1
    2c00:	e1a02006 	mov	r2, r6
    2c04:	e1a01004 	mov	r1, r4
    2c08:	eb00080a 	bl	4c38 <ext2_node_by_ino>
    2c0c:	e3500000 	cmp	r0, #0
    2c10:	1a00001c 	bne	2c88 <sdext2_create+0xb4>
    2c14:	e5953044 	ldr	r3, [r5, #68]	; 0x44
    2c18:	e3530000 	cmp	r3, #0
    2c1c:	0a000010 	beq	2c64 <sdext2_create+0x90>
    2c20:	e1a00007 	mov	r0, r7
    2c24:	e1a01006 	mov	r1, r6
    2c28:	e2852004 	add	r2, r5, #4
    2c2c:	e595304c 	ldr	r3, [r5, #76]	; 0x4c
    2c30:	eb0005ea 	bl	43e0 <ext2_create_file>
    2c34:	e1a08000 	mov	r8, r0
    2c38:	e3780001 	cmn	r8, #1
    2c3c:	0a000011 	beq	2c88 <sdext2_create+0xb4>
    2c40:	e1a00007 	mov	r0, r7
    2c44:	e1a01004 	mov	r1, r4
    2c48:	e1a02006 	mov	r2, r6
    2c4c:	eb00057c 	bl	4244 <put_node>
    2c50:	e3a00000 	mov	r0, #0
    2c54:	e5858058 	str	r8, [r5, #88]	; 0x58
    2c58:	e24bd020 	sub	sp, fp, #32
    2c5c:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    2c60:	e12fff1e 	bx	lr
    2c64:	e3a03b01 	mov	r3, #1024	; 0x400
    2c68:	e1a00007 	mov	r0, r7
    2c6c:	e5853048 	str	r3, [r5, #72]	; 0x48
    2c70:	e1a01006 	mov	r1, r6
    2c74:	e2852004 	add	r2, r5, #4
    2c78:	e595304c 	ldr	r3, [r5, #76]	; 0x4c
    2c7c:	eb00059f 	bl	4300 <ext2_create_dir>
    2c80:	e1a08000 	mov	r8, r0
    2c84:	eaffffeb 	b	2c38 <sdext2_create+0x64>
    2c88:	e3e00000 	mvn	r0, #0
    2c8c:	eafffff1 	b	2c58 <sdext2_create+0x84>

00002c90 <sdext2_write>:
    2c90:	e1a0c00d 	mov	ip, sp
    2c94:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    2c98:	e24cb004 	sub	fp, ip, #4
    2c9c:	e24dd088 	sub	sp, sp, #136	; 0x88
    2ca0:	e5924058 	ldr	r4, [r2, #88]	; 0x58
    2ca4:	e3540000 	cmp	r4, #0
    2ca8:	03a04002 	moveq	r4, #2
    2cac:	e24b709c 	sub	r7, fp, #156	; 0x9c
    2cb0:	e1a06002 	mov	r6, r2
    2cb4:	e1a01004 	mov	r1, r4
    2cb8:	e59b000c 	ldr	r0, [fp, #12]
    2cbc:	e1a02007 	mov	r2, r7
    2cc0:	e1a05003 	mov	r5, r3
    2cc4:	eb0007db 	bl	4c38 <ext2_node_by_ino>
    2cc8:	e3500000 	cmp	r0, #0
    2ccc:	1a000019 	bne	2d38 <sdext2_write+0xa8>
    2cd0:	e59b3008 	ldr	r3, [fp, #8]
    2cd4:	e1a02005 	mov	r2, r5
    2cd8:	e58d3000 	str	r3, [sp]
    2cdc:	e59b000c 	ldr	r0, [fp, #12]
    2ce0:	e1a01007 	mov	r1, r7
    2ce4:	e59b3004 	ldr	r3, [fp, #4]
    2ce8:	eb0005ee 	bl	44a8 <ext2_write>
    2cec:	e2505000 	subs	r5, r0, #0
    2cf0:	b1a00005 	movlt	r0, r5
    2cf4:	ba00000c 	blt	2d2c <sdext2_write+0x9c>
    2cf8:	e5960048 	ldr	r0, [r6, #72]	; 0x48
    2cfc:	e51b3098 	ldr	r3, [fp, #-152]	; 0x98
    2d00:	e0800005 	add	r0, r0, r5
    2d04:	e0833005 	add	r3, r3, r5
    2d08:	e1a01004 	mov	r1, r4
    2d0c:	e1a02007 	mov	r2, r7
    2d10:	e5860048 	str	r0, [r6, #72]	; 0x48
    2d14:	e59b000c 	ldr	r0, [fp, #12]
    2d18:	e50b3098 	str	r3, [fp, #-152]	; 0x98
    2d1c:	eb000548 	bl	4244 <put_node>
    2d20:	e1a00006 	mov	r0, r6
    2d24:	eb0017b7 	bl	8c08 <vfs_set>
    2d28:	e1a00005 	mov	r0, r5
    2d2c:	e24bd01c 	sub	sp, fp, #28
    2d30:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    2d34:	e12fff1e 	bx	lr
    2d38:	e3e00000 	mvn	r0, #0
    2d3c:	eafffffa 	b	2d2c <sdext2_write+0x9c>

00002d40 <sdext2_read>:
    2d40:	e1a0c00d 	mov	ip, sp
    2d44:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    2d48:	e24cb004 	sub	fp, ip, #4
    2d4c:	e24dd08c 	sub	sp, sp, #140	; 0x8c
    2d50:	e5921058 	ldr	r1, [r2, #88]	; 0x58
    2d54:	e24b50a4 	sub	r5, fp, #164	; 0xa4
    2d58:	e3510000 	cmp	r1, #0
    2d5c:	03a01002 	moveq	r1, #2
    2d60:	e1a04002 	mov	r4, r2
    2d64:	e59b000c 	ldr	r0, [fp, #12]
    2d68:	e1a02005 	mov	r2, r5
    2d6c:	e1a07003 	mov	r7, r3
    2d70:	e59b8004 	ldr	r8, [fp, #4]
    2d74:	e59b6008 	ldr	r6, [fp, #8]
    2d78:	eb0007ae 	bl	4c38 <ext2_node_by_ino>
    2d7c:	e3500000 	cmp	r0, #0
    2d80:	1a00000f 	bne	2dc4 <sdext2_read+0x84>
    2d84:	e5943048 	ldr	r3, [r4, #72]	; 0x48
    2d88:	e0663003 	rsb	r3, r6, r3
    2d8c:	e1530008 	cmp	r3, r8
    2d90:	a1a03008 	movge	r3, r8
    2d94:	e3530000 	cmp	r3, #0
    2d98:	ba000009 	blt	2dc4 <sdext2_read+0x84>
    2d9c:	01a00003 	moveq	r0, r3
    2da0:	0a000004 	beq	2db8 <sdext2_read+0x78>
    2da4:	e58d6000 	str	r6, [sp]
    2da8:	e1a01005 	mov	r1, r5
    2dac:	e1a02007 	mov	r2, r7
    2db0:	e59b000c 	ldr	r0, [fp, #12]
    2db4:	eb000501 	bl	41c0 <ext2_read>
    2db8:	e24bd020 	sub	sp, fp, #32
    2dbc:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    2dc0:	e12fff1e 	bx	lr
    2dc4:	e3e00000 	mvn	r0, #0
    2dc8:	e24bd020 	sub	sp, fp, #32
    2dcc:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    2dd0:	e12fff1e 	bx	lr

00002dd4 <add_nodes>:
    2dd4:	e1a0c00d 	mov	ip, sp
    2dd8:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    2ddc:	e24cb004 	sub	fp, ip, #4
    2de0:	e24ddc05 	sub	sp, sp, #1280	; 0x500
    2de4:	e24b3e4f 	sub	r3, fp, #1264	; 0x4f0
    2de8:	e243300c 	sub	r3, r3, #12
    2dec:	e24dd004 	sub	sp, sp, #4
    2df0:	e50b3528 	str	r3, [fp, #-1320]	; 0x528
    2df4:	e243300c 	sub	r3, r3, #12
    2df8:	e59f8218 	ldr	r8, [pc, #536]	; 3018 <add_nodes+0x244>
    2dfc:	e59fa218 	ldr	sl, [pc, #536]	; 301c <add_nodes+0x248>
    2e00:	e50b3520 	str	r3, [fp, #-1312]	; 0x520
    2e04:	e24b3e42 	sub	r3, fp, #1056	; 0x420
    2e08:	e243300c 	sub	r3, r3, #12
    2e0c:	e50b0510 	str	r0, [fp, #-1296]	; 0x510
    2e10:	e50b2524 	str	r2, [fp, #-1316]	; 0x524
    2e14:	e08f8008 	add	r8, pc, r8
    2e18:	e08fa00a 	add	sl, pc, sl
    2e1c:	e2817024 	add	r7, r1, #36	; 0x24
    2e20:	e2819054 	add	r9, r1, #84	; 0x54
    2e24:	e50b3514 	str	r3, [fp, #-1300]	; 0x514
    2e28:	ea000001 	b	2e34 <add_nodes+0x60>
    2e2c:	e1570009 	cmp	r7, r9
    2e30:	0a000056 	beq	2f90 <add_nodes+0x1bc>
    2e34:	e5b70004 	ldr	r0, [r7, #4]!
    2e38:	e3500000 	cmp	r0, #0
    2e3c:	0afffffa 	beq	2e2c <add_nodes+0x58>
    2e40:	e51b3510 	ldr	r3, [fp, #-1296]	; 0x510
    2e44:	e51b1514 	ldr	r1, [fp, #-1300]	; 0x514
    2e48:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    2e4c:	e1a0e00f 	mov	lr, pc
    2e50:	e12fff1c 	bx	ip
    2e54:	e51b342c 	ldr	r3, [fp, #-1068]	; 0x42c
    2e58:	e3530000 	cmp	r3, #0
    2e5c:	0afffff2 	beq	2e2c <add_nodes+0x58>
    2e60:	e3a02000 	mov	r2, #0
    2e64:	e55b3426 	ldrb	r3, [fp, #-1062]	; 0x426
    2e68:	e24b102c 	sub	r1, fp, #44	; 0x2c
    2e6c:	e1530002 	cmp	r3, r2
    2e70:	e0813003 	add	r3, r1, r3
    2e74:	e55363f8 	ldrb	r6, [r3, #-1016]	; 0x3f8
    2e78:	e54323f8 	strb	r2, [r3, #-1016]	; 0x3f8
    2e7c:	0affffea 	beq	2e2c <add_nodes+0x58>
    2e80:	e24b3e4a 	sub	r3, fp, #1184	; 0x4a0
    2e84:	e243300c 	sub	r3, r3, #12
    2e88:	e51b4514 	ldr	r4, [fp, #-1300]	; 0x514
    2e8c:	e50b3518 	str	r3, [fp, #-1304]	; 0x518
    2e90:	e50b951c 	str	r9, [fp, #-1308]	; 0x51c
    2e94:	ea00000e 	b	2ed4 <add_nodes+0x100>
    2e98:	e5d43006 	ldrb	r3, [r4, #6]
    2e9c:	e1d420b4 	ldrh	r2, [r4, #4]
    2ea0:	e0843003 	add	r3, r4, r3
    2ea4:	e0844002 	add	r4, r4, r2
    2ea8:	e24b202c 	sub	r2, fp, #44	; 0x2c
    2eac:	e1540002 	cmp	r4, r2
    2eb0:	e5c36008 	strb	r6, [r3, #8]
    2eb4:	2a000032 	bcs	2f84 <add_nodes+0x1b0>
    2eb8:	e3a02000 	mov	r2, #0
    2ebc:	e5d43006 	ldrb	r3, [r4, #6]
    2ec0:	e1530002 	cmp	r3, r2
    2ec4:	e0843003 	add	r3, r4, r3
    2ec8:	e5d36008 	ldrb	r6, [r3, #8]
    2ecc:	e5c32008 	strb	r2, [r3, #8]
    2ed0:	0a00002b 	beq	2f84 <add_nodes+0x1b0>
    2ed4:	e2845008 	add	r5, r4, #8
    2ed8:	e1a00005 	mov	r0, r5
    2edc:	e1a01008 	mov	r1, r8
    2ee0:	eb002cd5 	bl	e23c <strcmp>
    2ee4:	e3500000 	cmp	r0, #0
    2ee8:	0affffea 	beq	2e98 <add_nodes+0xc4>
    2eec:	e1a00005 	mov	r0, r5
    2ef0:	e1a0100a 	mov	r1, sl
    2ef4:	eb002cd0 	bl	e23c <strcmp>
    2ef8:	e3500000 	cmp	r0, #0
    2efc:	0affffe5 	beq	2e98 <add_nodes+0xc4>
    2f00:	e5949000 	ldr	r9, [r4]
    2f04:	e51b0510 	ldr	r0, [fp, #-1296]	; 0x510
    2f08:	e1a01009 	mov	r1, r9
    2f0c:	e51b2518 	ldr	r2, [fp, #-1304]	; 0x518
    2f10:	eb000748 	bl	4c38 <ext2_node_by_ino>
    2f14:	e250c000 	subs	ip, r0, #0
    2f18:	1affffde 	bne	2e98 <add_nodes+0xc4>
    2f1c:	e5d43007 	ldrb	r3, [r4, #7]
    2f20:	e3530002 	cmp	r3, #2
    2f24:	0a00001d 	beq	2fa0 <add_nodes+0x1cc>
    2f28:	e3530001 	cmp	r3, #1
    2f2c:	1affffd9 	bne	2e98 <add_nodes+0xc4>
    2f30:	e1a0100c 	mov	r1, ip
    2f34:	e3a0205c 	mov	r2, #92	; 0x5c
    2f38:	e51b0520 	ldr	r0, [fp, #-1312]	; 0x520
    2f3c:	e50b352c 	str	r3, [fp, #-1324]	; 0x52c
    2f40:	eb002c28 	bl	dfe8 <memset>
    2f44:	e51b3528 	ldr	r3, [fp, #-1320]	; 0x528
    2f48:	e1a01005 	mov	r1, r5
    2f4c:	e2430008 	sub	r0, r3, #8
    2f50:	eb002d42 	bl	e460 <strcpy>
    2f54:	e51b5520 	ldr	r5, [fp, #-1312]	; 0x520
    2f58:	e51b24a8 	ldr	r2, [fp, #-1192]	; 0x4a8
    2f5c:	e51b352c 	ldr	r3, [fp, #-1324]	; 0x52c
    2f60:	e1a00005 	mov	r0, r5
    2f64:	e50b34c4 	str	r3, [fp, #-1220]	; 0x4c4
    2f68:	e50b94b0 	str	r9, [fp, #-1200]	; 0x4b0
    2f6c:	e50b24c0 	str	r2, [fp, #-1216]	; 0x4c0
    2f70:	eb001568 	bl	8518 <vfs_new_node>
    2f74:	e51b0524 	ldr	r0, [fp, #-1316]	; 0x524
    2f78:	e1a01005 	mov	r1, r5
    2f7c:	eb001751 	bl	8cc8 <vfs_add>
    2f80:	eaffffc4 	b	2e98 <add_nodes+0xc4>
    2f84:	e51b951c 	ldr	r9, [fp, #-1308]	; 0x51c
    2f88:	e1570009 	cmp	r7, r9
    2f8c:	1affffa8 	bne	2e34 <add_nodes+0x60>
    2f90:	e3a00000 	mov	r0, #0
    2f94:	e24bd028 	sub	sp, fp, #40	; 0x28
    2f98:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    2f9c:	e12fff1e 	bx	lr
    2fa0:	e1a0100c 	mov	r1, ip
    2fa4:	e3a0205c 	mov	r2, #92	; 0x5c
    2fa8:	e51b0520 	ldr	r0, [fp, #-1312]	; 0x520
    2fac:	e50bc52c 	str	ip, [fp, #-1324]	; 0x52c
    2fb0:	eb002c0c 	bl	dfe8 <memset>
    2fb4:	e51b3528 	ldr	r3, [fp, #-1320]	; 0x528
    2fb8:	e1a01005 	mov	r1, r5
    2fbc:	e2430008 	sub	r0, r3, #8
    2fc0:	eb002d26 	bl	e460 <strcpy>
    2fc4:	e51b5520 	ldr	r5, [fp, #-1312]	; 0x520
    2fc8:	e51b34a8 	ldr	r3, [fp, #-1192]	; 0x4a8
    2fcc:	e51bc52c 	ldr	ip, [fp, #-1324]	; 0x52c
    2fd0:	e1a00005 	mov	r0, r5
    2fd4:	e50bc4c4 	str	ip, [fp, #-1220]	; 0x4c4
    2fd8:	e50b34c0 	str	r3, [fp, #-1216]	; 0x4c0
    2fdc:	e50b94b0 	str	r9, [fp, #-1200]	; 0x4b0
    2fe0:	eb00154c 	bl	8518 <vfs_new_node>
    2fe4:	e51b0524 	ldr	r0, [fp, #-1316]	; 0x524
    2fe8:	e1a01005 	mov	r1, r5
    2fec:	eb001735 	bl	8cc8 <vfs_add>
    2ff0:	e3500000 	cmp	r0, #0
    2ff4:	1a000004 	bne	300c <add_nodes+0x238>
    2ff8:	e51b0510 	ldr	r0, [fp, #-1296]	; 0x510
    2ffc:	e51b1518 	ldr	r1, [fp, #-1304]	; 0x518
    3000:	e51b2520 	ldr	r2, [fp, #-1312]	; 0x520
    3004:	ebffff72 	bl	2dd4 <add_nodes>
    3008:	eaffffa2 	b	2e98 <add_nodes+0xc4>
    300c:	e51b0520 	ldr	r0, [fp, #-1312]	; 0x520
    3010:	eb001763 	bl	8da4 <vfs_del>
    3014:	eafffff7 	b	2ff8 <add_nodes+0x224>
    3018:	0000c688 	andeq	ip, r0, r8, lsl #13
    301c:	0000c688 	andeq	ip, r0, r8, lsl #13

00003020 <sdext2_mount>:
    3020:	e1a0c00d 	mov	ip, sp
    3024:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    3028:	e1a05001 	mov	r5, r1
    302c:	e1a06000 	mov	r6, r0
    3030:	e24cb004 	sub	fp, ip, #4
    3034:	e1a00001 	mov	r0, r1
    3038:	e59f1030 	ldr	r1, [pc, #48]	; 3070 <sdext2_mount+0x50>
    303c:	e24dd084 	sub	sp, sp, #132	; 0x84
    3040:	e24b409c 	sub	r4, fp, #156	; 0x9c
    3044:	e1a02004 	mov	r2, r4
    3048:	e08f1001 	add	r1, pc, r1
    304c:	eb00070d 	bl	4c88 <ext2_node_by_fname>
    3050:	e1a00005 	mov	r0, r5
    3054:	e1a01004 	mov	r1, r4
    3058:	e1a02006 	mov	r2, r6
    305c:	ebffff5c 	bl	2dd4 <add_nodes>
    3060:	e3a00000 	mov	r0, #0
    3064:	e24bd018 	sub	sp, fp, #24
    3068:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    306c:	e12fff1e 	bx	lr
    3070:	0000c418 	andeq	ip, r0, r8, lsl r4

00003074 <sdfsd_main>:
    3074:	e1a0c00d 	mov	ip, sp
    3078:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    307c:	e24dde4a 	sub	sp, sp, #1184	; 0x4a0
    3080:	e24cb004 	sub	fp, ip, #4
    3084:	e24dd004 	sub	sp, sp, #4
    3088:	eb002727 	bl	cd2c <getuid>
    308c:	e59f4160 	ldr	r4, [pc, #352]	; 31f4 <sdfsd_main+0x180>
    3090:	e3500000 	cmp	r0, #0
    3094:	e08f4004 	add	r4, pc, r4
    3098:	aa000043 	bge	31ac <sdfsd_main+0x138>
    309c:	e59f0154 	ldr	r0, [pc, #340]	; 31f8 <sdfsd_main+0x184>
    30a0:	e08f0000 	add	r0, pc, r0
    30a4:	eb001484 	bl	82bc <klog>
    30a8:	eb000157 	bl	360c <sd_init>
    30ac:	e3500000 	cmp	r0, #0
    30b0:	1a000044 	bne	31c8 <sdfsd_main+0x154>
    30b4:	e59f5140 	ldr	r5, [pc, #320]	; 31fc <sdfsd_main+0x188>
    30b8:	e08f5005 	add	r5, pc, r5
    30bc:	e1a00005 	mov	r0, r5
    30c0:	eb00147d 	bl	82bc <klog>
    30c4:	e59f0134 	ldr	r0, [pc, #308]	; 3200 <sdfsd_main+0x18c>
    30c8:	e08f0000 	add	r0, pc, r0
    30cc:	eb00147a 	bl	82bc <klog>
    30d0:	e24b6e43 	sub	r6, fp, #1072	; 0x430
    30d4:	e59f2128 	ldr	r2, [pc, #296]	; 3204 <sdfsd_main+0x190>
    30d8:	e59f3128 	ldr	r3, [pc, #296]	; 3208 <sdfsd_main+0x194>
    30dc:	e2466004 	sub	r6, r6, #4
    30e0:	e7941002 	ldr	r1, [r4, r2]
    30e4:	e1a00006 	mov	r0, r6
    30e8:	e7942003 	ldr	r2, [r4, r3]
    30ec:	eb0006f7 	bl	4cd0 <ext2_init>
    30f0:	e2504000 	subs	r4, r0, #0
    30f4:	1a000038 	bne	31dc <sdfsd_main+0x168>
    30f8:	e1a00005 	mov	r0, r5
    30fc:	eb00146e 	bl	82bc <klog>
    3100:	e51b042c 	ldr	r0, [fp, #-1068]	; 0x42c
    3104:	e24b7d13 	sub	r7, fp, #1216	; 0x4c0
    3108:	e1a00080 	lsl	r0, r0, #1
    310c:	eb00010e 	bl	354c <sd_set_buffer>
    3110:	e1a01004 	mov	r1, r4
    3114:	e3a0208c 	mov	r2, #140	; 0x8c
    3118:	e1a00007 	mov	r0, r7
    311c:	eb002bb1 	bl	dfe8 <memset>
    3120:	e24b5e4b 	sub	r5, fp, #1200	; 0x4b0
    3124:	e59f10e0 	ldr	r1, [pc, #224]	; 320c <sdfsd_main+0x198>
    3128:	e2455004 	sub	r5, r5, #4
    312c:	e245000b 	sub	r0, r5, #11
    3130:	e08f1001 	add	r1, pc, r1
    3134:	e3a0200d 	mov	r2, #13
    3138:	eb002b6e 	bl	def8 <memcpy>
    313c:	e59f80cc 	ldr	r8, [pc, #204]	; 3210 <sdfsd_main+0x19c>
    3140:	e59f50cc 	ldr	r5, [pc, #204]	; 3214 <sdfsd_main+0x1a0>
    3144:	e59fe0cc 	ldr	lr, [pc, #204]	; 3218 <sdfsd_main+0x1a4>
    3148:	e59fc0cc 	ldr	ip, [pc, #204]	; 321c <sdfsd_main+0x1a8>
    314c:	e59f30cc 	ldr	r3, [pc, #204]	; 3220 <sdfsd_main+0x1ac>
    3150:	e59f10cc 	ldr	r1, [pc, #204]	; 3224 <sdfsd_main+0x1b0>
    3154:	e08fe00e 	add	lr, pc, lr
    3158:	e08fc00c 	add	ip, pc, ip
    315c:	e08f3003 	add	r3, pc, r3
    3160:	e08f1001 	add	r1, pc, r1
    3164:	e1a02004 	mov	r2, r4
    3168:	e08f8008 	add	r8, pc, r8
    316c:	e08f5005 	add	r5, pc, r5
    3170:	e1a00007 	mov	r0, r7
    3174:	e50be464 	str	lr, [fp, #-1124]	; 0x464
    3178:	e50bc470 	str	ip, [fp, #-1136]	; 0x470
    317c:	e50b3444 	str	r3, [fp, #-1092]	; 0x444
    3180:	e50b647c 	str	r6, [fp, #-1148]	; 0x47c
    3184:	e50b844c 	str	r8, [fp, #-1100]	; 0x44c
    3188:	e50b5468 	str	r5, [fp, #-1128]	; 0x468
    318c:	eb0020e5 	bl	b528 <device_run>
    3190:	e1a00006 	mov	r0, r6
    3194:	eb00071c 	bl	4e0c <ext2_quit>
    3198:	eb0000ff 	bl	359c <sd_quit>
    319c:	e1a00004 	mov	r0, r4
    31a0:	e24bd020 	sub	sp, fp, #32
    31a4:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    31a8:	e12fff1e 	bx	lr
    31ac:	e59f0074 	ldr	r0, [pc, #116]	; 3228 <sdfsd_main+0x1b4>
    31b0:	e08f0000 	add	r0, pc, r0
    31b4:	eb001440 	bl	82bc <klog>
    31b8:	e3e00000 	mvn	r0, #0
    31bc:	e24bd020 	sub	sp, fp, #32
    31c0:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    31c4:	e12fff1e 	bx	lr
    31c8:	e59f005c 	ldr	r0, [pc, #92]	; 322c <sdfsd_main+0x1b8>
    31cc:	e08f0000 	add	r0, pc, r0
    31d0:	eb001439 	bl	82bc <klog>
    31d4:	e3e00000 	mvn	r0, #0
    31d8:	eafffff0 	b	31a0 <sdfsd_main+0x12c>
    31dc:	eb0000ee 	bl	359c <sd_quit>
    31e0:	e59f0048 	ldr	r0, [pc, #72]	; 3230 <sdfsd_main+0x1bc>
    31e4:	e08f0000 	add	r0, pc, r0
    31e8:	eb001433 	bl	82bc <klog>
    31ec:	e3e00000 	mvn	r0, #0
    31f0:	eaffffea 	b	31a0 <sdfsd_main+0x12c>
    31f4:	00014964 	andeq	r4, r1, r4, ror #18
    31f8:	0000c430 	andeq	ip, r0, r0, lsr r4
    31fc:	0000c438 	andeq	ip, r0, r8, lsr r4
    3200:	0000c430 	andeq	ip, r0, r0, lsr r4
    3204:	00000010 	andeq	r0, r0, r0, lsl r0
    3208:	00000048 	andeq	r0, r0, r8, asr #32
    320c:	0000c3e0 	andeq	ip, r0, r0, ror #7
    3210:	fffffeb0 			; <UNDEFINED> instruction: 0xfffffeb0
    3214:	fffffbcc 			; <UNDEFINED> instruction: 0xfffffbcc
    3218:	fffffb34 			; <UNDEFINED> instruction: 0xfffffb34
    321c:	fffffa74 			; <UNDEFINED> instruction: 0xfffffa74
    3220:	fffffa50 			; <UNDEFINED> instruction: 0xfffffa50
    3224:	0000c300 	andeq	ip, r0, r0, lsl #6
    3228:	0000c2f4 	strdeq	ip, [r0], -r4
    322c:	0000c318 	andeq	ip, r0, r8, lsl r3
    3230:	0000c300 	andeq	ip, r0, r0, lsl #6

00003234 <sd_read_sector>:
    3234:	e1a0c00d 	mov	ip, sp
    3238:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    323c:	e1a04000 	mov	r4, r0
    3240:	e59f20d4 	ldr	r2, [pc, #212]	; 331c <sd_read_sector+0xe8>
    3244:	e08f2002 	add	r2, pc, r2
    3248:	e5920010 	ldr	r0, [r2, #16]
    324c:	e5923008 	ldr	r3, [r2, #8]
    3250:	e3500000 	cmp	r0, #0
    3254:	e24cb004 	sub	fp, ip, #4
    3258:	e1a05001 	mov	r5, r1
    325c:	e0633004 	rsb	r3, r3, r4
    3260:	0a00000d 	beq	329c <sd_read_sector+0x68>
    3264:	e5922014 	ldr	r2, [r2, #20]
    3268:	e1530002 	cmp	r3, r2
    326c:	2a00000a 	bcs	329c <sd_read_sector+0x68>
    3270:	e0803183 	add	r3, r0, r3, lsl #3
    3274:	e5931004 	ldr	r1, [r3, #4]
    3278:	e3510000 	cmp	r1, #0
    327c:	0a000006 	beq	329c <sd_read_sector+0x68>
    3280:	e1a00005 	mov	r0, r5
    3284:	e3a02c02 	mov	r2, #512	; 0x200
    3288:	eb002b1a 	bl	def8 <memcpy>
    328c:	e3a00c02 	mov	r0, #512	; 0x200
    3290:	e24bd01c 	sub	sp, fp, #28
    3294:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    3298:	e12fff1e 	bx	lr
    329c:	e59f607c 	ldr	r6, [pc, #124]	; 3320 <sd_read_sector+0xec>
    32a0:	e1a00004 	mov	r0, r4
    32a4:	e08f6006 	add	r6, pc, r6
    32a8:	e1a01005 	mov	r1, r5
    32ac:	e596c018 	ldr	ip, [r6, #24]
    32b0:	e1a0e00f 	mov	lr, pc
    32b4:	e12fff1c 	bx	ip
    32b8:	e3500000 	cmp	r0, #0
    32bc:	13a00000 	movne	r0, #0
    32c0:	1afffff2 	bne	3290 <sd_read_sector+0x5c>
    32c4:	e5963010 	ldr	r3, [r6, #16]
    32c8:	e5962008 	ldr	r2, [r6, #8]
    32cc:	e3530000 	cmp	r3, #0
    32d0:	e0624004 	rsb	r4, r2, r4
    32d4:	0a00000e 	beq	3314 <sd_read_sector+0xe0>
    32d8:	e5962014 	ldr	r2, [r6, #20]
    32dc:	e1540002 	cmp	r4, r2
    32e0:	2a00000b 	bcs	3314 <sd_read_sector+0xe0>
    32e4:	e0834184 	add	r4, r3, r4, lsl #3
    32e8:	e5940004 	ldr	r0, [r4, #4]
    32ec:	e3500000 	cmp	r0, #0
    32f0:	1b0027ec 	blne	d2a8 <free>
    32f4:	e3a00c02 	mov	r0, #512	; 0x200
    32f8:	eb0027f3 	bl	d2cc <malloc>
    32fc:	e1a01005 	mov	r1, r5
    3300:	e5840004 	str	r0, [r4, #4]
    3304:	e3a02c02 	mov	r2, #512	; 0x200
    3308:	eb002afa 	bl	def8 <memcpy>
    330c:	e3a00c02 	mov	r0, #512	; 0x200
    3310:	eaffffde 	b	3290 <sd_read_sector+0x5c>
    3314:	e3a00c02 	mov	r0, #512	; 0x200
    3318:	eaffffdc 	b	3290 <sd_read_sector+0x5c>
    331c:	00049080 	andeq	r9, r4, r0, lsl #1
    3320:	00049020 	andeq	r9, r4, r0, lsr #32

00003324 <sd_write_sector>:
    3324:	e1a0c00d 	mov	ip, sp
    3328:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    332c:	e59f4098 	ldr	r4, [pc, #152]	; 33cc <sd_write_sector+0xa8>
    3330:	e24cb004 	sub	fp, ip, #4
    3334:	e08f4004 	add	r4, pc, r4
    3338:	e1a05000 	mov	r5, r0
    333c:	e1a06001 	mov	r6, r1
    3340:	e594c01c 	ldr	ip, [r4, #28]
    3344:	e1a0e00f 	mov	lr, pc
    3348:	e12fff1c 	bx	ip
    334c:	e3500000 	cmp	r0, #0
    3350:	13a00000 	movne	r0, #0
    3354:	0a000002 	beq	3364 <sd_write_sector+0x40>
    3358:	e24bd01c 	sub	sp, fp, #28
    335c:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    3360:	e12fff1e 	bx	lr
    3364:	e5943010 	ldr	r3, [r4, #16]
    3368:	e5942008 	ldr	r2, [r4, #8]
    336c:	e3530000 	cmp	r3, #0
    3370:	e0625005 	rsb	r5, r2, r5
    3374:	0a000010 	beq	33bc <sd_write_sector+0x98>
    3378:	e5942014 	ldr	r2, [r4, #20]
    337c:	e1550002 	cmp	r5, r2
    3380:	2a00000d 	bcs	33bc <sd_write_sector+0x98>
    3384:	e0835185 	add	r5, r3, r5, lsl #3
    3388:	e5950004 	ldr	r0, [r5, #4]
    338c:	e3500000 	cmp	r0, #0
    3390:	1b0027c4 	blne	d2a8 <free>
    3394:	e3a00c02 	mov	r0, #512	; 0x200
    3398:	eb0027cb 	bl	d2cc <malloc>
    339c:	e1a01006 	mov	r1, r6
    33a0:	e5850004 	str	r0, [r5, #4]
    33a4:	e3a02c02 	mov	r2, #512	; 0x200
    33a8:	eb002ad2 	bl	def8 <memcpy>
    33ac:	e3a00c02 	mov	r0, #512	; 0x200
    33b0:	e24bd01c 	sub	sp, fp, #28
    33b4:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    33b8:	e12fff1e 	bx	lr
    33bc:	e3a00c02 	mov	r0, #512	; 0x200
    33c0:	e24bd01c 	sub	sp, fp, #28
    33c4:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    33c8:	e12fff1e 	bx	lr
    33cc:	00048f90 	muleq	r4, r0, pc	; <UNPREDICTABLE>

000033d0 <sd_read>:
    33d0:	e1a0c00d 	mov	ip, sp
    33d4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    33d8:	e59f3048 	ldr	r3, [pc, #72]	; 3428 <sd_read+0x58>
    33dc:	e08f3003 	add	r3, pc, r3
    33e0:	e5934008 	ldr	r4, [r3, #8]
    33e4:	e0844080 	add	r4, r4, r0, lsl #1
    33e8:	e24cb004 	sub	fp, ip, #4
    33ec:	e1a00004 	mov	r0, r4
    33f0:	e1a05001 	mov	r5, r1
    33f4:	ebffff8e 	bl	3234 <sd_read_sector>
    33f8:	e3500c02 	cmp	r0, #512	; 0x200
    33fc:	1a000007 	bne	3420 <sd_read+0x50>
    3400:	e2840001 	add	r0, r4, #1
    3404:	e2851c02 	add	r1, r5, #512	; 0x200
    3408:	ebffff89 	bl	3234 <sd_read_sector>
    340c:	e2500c02 	subs	r0, r0, #512	; 0x200
    3410:	13e00000 	mvnne	r0, #0
    3414:	e24bd014 	sub	sp, fp, #20
    3418:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    341c:	e12fff1e 	bx	lr
    3420:	e3e00000 	mvn	r0, #0
    3424:	eafffffa 	b	3414 <sd_read+0x44>
    3428:	00048ee8 	andeq	r8, r4, r8, ror #29

0000342c <sd_write>:
    342c:	e1a0c00d 	mov	ip, sp
    3430:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    3434:	e59f3048 	ldr	r3, [pc, #72]	; 3484 <sd_write+0x58>
    3438:	e08f3003 	add	r3, pc, r3
    343c:	e5934008 	ldr	r4, [r3, #8]
    3440:	e0844080 	add	r4, r4, r0, lsl #1
    3444:	e24cb004 	sub	fp, ip, #4
    3448:	e1a00004 	mov	r0, r4
    344c:	e1a05001 	mov	r5, r1
    3450:	ebffffb3 	bl	3324 <sd_write_sector>
    3454:	e3500c02 	cmp	r0, #512	; 0x200
    3458:	1a000007 	bne	347c <sd_write+0x50>
    345c:	e2840001 	add	r0, r4, #1
    3460:	e2851c02 	add	r1, r5, #512	; 0x200
    3464:	ebffffae 	bl	3324 <sd_write_sector>
    3468:	e2500c02 	subs	r0, r0, #512	; 0x200
    346c:	13e00000 	mvnne	r0, #0
    3470:	e24bd014 	sub	sp, fp, #20
    3474:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    3478:	e12fff1e 	bx	lr
    347c:	e3e00000 	mvn	r0, #0
    3480:	eafffffa 	b	3470 <sd_write+0x44>
    3484:	00048e8c 	andeq	r8, r4, ip, lsl #29

00003488 <read_partition>:
    3488:	e1a0c00d 	mov	ip, sp
    348c:	e92dd800 	push	{fp, ip, lr, pc}
    3490:	e24cb004 	sub	fp, ip, #4
    3494:	e24ddc02 	sub	sp, sp, #512	; 0x200
    3498:	e3a00000 	mov	r0, #0
    349c:	e24b1f83 	sub	r1, fp, #524	; 0x20c
    34a0:	ebffff63 	bl	3234 <sd_read_sector>
    34a4:	e3500c02 	cmp	r0, #512	; 0x200
    34a8:	1a00000f 	bne	34ec <read_partition+0x64>
    34ac:	e55b300e 	ldrb	r3, [fp, #-14]
    34b0:	e3530055 	cmp	r3, #85	; 0x55
    34b4:	1a00000c 	bne	34ec <read_partition+0x64>
    34b8:	e55b300d 	ldrb	r3, [fp, #-13]
    34bc:	e35300aa 	cmp	r3, #170	; 0xaa
    34c0:	1a000009 	bne	34ec <read_partition+0x64>
    34c4:	e59f0030 	ldr	r0, [pc, #48]	; 34fc <read_partition+0x74>
    34c8:	e08f0000 	add	r0, pc, r0
    34cc:	e24b104e 	sub	r1, fp, #78	; 0x4e
    34d0:	e3a02040 	mov	r2, #64	; 0x40
    34d4:	e2800020 	add	r0, r0, #32
    34d8:	eb002a86 	bl	def8 <memcpy>
    34dc:	e3a00000 	mov	r0, #0
    34e0:	e24bd00c 	sub	sp, fp, #12
    34e4:	e89d6800 	ldm	sp, {fp, sp, lr}
    34e8:	e12fff1e 	bx	lr
    34ec:	e3e00000 	mvn	r0, #0
    34f0:	e24bd00c 	sub	sp, fp, #12
    34f4:	e89d6800 	ldm	sp, {fp, sp, lr}
    34f8:	e12fff1e 	bx	lr
    34fc:	00048dfc 	strdeq	r8, [r4], -ip

00003500 <partition_get>:
    3500:	e1a0c00d 	mov	ip, sp
    3504:	e3500003 	cmp	r0, #3
    3508:	e92dd800 	push	{fp, ip, lr, pc}
    350c:	e24cb004 	sub	fp, ip, #4
    3510:	8a00000a 	bhi	3540 <partition_get+0x40>
    3514:	e59f302c 	ldr	r3, [pc, #44]	; 3548 <partition_get+0x48>
    3518:	e08f3003 	add	r3, pc, r3
    351c:	e0833200 	add	r3, r3, r0, lsl #4
    3520:	e3a02010 	mov	r2, #16
    3524:	e1a00001 	mov	r0, r1
    3528:	e2831020 	add	r1, r3, #32
    352c:	eb002a71 	bl	def8 <memcpy>
    3530:	e3a00000 	mov	r0, #0
    3534:	e24bd00c 	sub	sp, fp, #12
    3538:	e89d6800 	ldm	sp, {fp, sp, lr}
    353c:	e12fff1e 	bx	lr
    3540:	e3e00000 	mvn	r0, #0
    3544:	eafffffa 	b	3534 <partition_get+0x34>
    3548:	00048dac 	andeq	r8, r4, ip, lsr #27

0000354c <sd_set_buffer>:
    354c:	e1a0c00d 	mov	ip, sp
    3550:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    3554:	e1a05180 	lsl	r5, r0, #3
    3558:	e24cb004 	sub	fp, ip, #4
    355c:	e1a04000 	mov	r4, r0
    3560:	e1a00005 	mov	r0, r5
    3564:	eb002758 	bl	d2cc <malloc>
    3568:	e1a02005 	mov	r2, r5
    356c:	e3a01000 	mov	r1, #0
    3570:	e1a06000 	mov	r6, r0
    3574:	eb002a9b 	bl	dfe8 <memset>
    3578:	e3a00000 	mov	r0, #0
    357c:	e59f3014 	ldr	r3, [pc, #20]	; 3598 <sd_set_buffer+0x4c>
    3580:	e08f3003 	add	r3, pc, r3
    3584:	e5834014 	str	r4, [r3, #20]
    3588:	e5836010 	str	r6, [r3, #16]
    358c:	e24bd01c 	sub	sp, fp, #28
    3590:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    3594:	e12fff1e 	bx	lr
    3598:	00048d44 	andeq	r8, r4, r4, asr #26

0000359c <sd_quit>:
    359c:	e1a0c00d 	mov	ip, sp
    35a0:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    35a4:	e59f3058 	ldr	r3, [pc, #88]	; 3604 <sd_quit+0x68>
    35a8:	e08f3003 	add	r3, pc, r3
    35ac:	e5934014 	ldr	r4, [r3, #20]
    35b0:	e3540000 	cmp	r4, #0
    35b4:	e24cb004 	sub	fp, ip, #4
    35b8:	e5936010 	ldr	r6, [r3, #16]
    35bc:	0a000006 	beq	35dc <sd_quit+0x40>
    35c0:	e0865184 	add	r5, r6, r4, lsl #3
    35c4:	e5150004 	ldr	r0, [r5, #-4]
    35c8:	e3500000 	cmp	r0, #0
    35cc:	1b002735 	blne	d2a8 <free>
    35d0:	e2544001 	subs	r4, r4, #1
    35d4:	e2455008 	sub	r5, r5, #8
    35d8:	1afffff9 	bne	35c4 <sd_quit+0x28>
    35dc:	e1a00006 	mov	r0, r6
    35e0:	eb002730 	bl	d2a8 <free>
    35e4:	e3a00000 	mov	r0, #0
    35e8:	e59f3018 	ldr	r3, [pc, #24]	; 3608 <sd_quit+0x6c>
    35ec:	e08f3003 	add	r3, pc, r3
    35f0:	e5830010 	str	r0, [r3, #16]
    35f4:	e5830014 	str	r0, [r3, #20]
    35f8:	e24bd01c 	sub	sp, fp, #28
    35fc:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    3600:	e12fff1e 	bx	lr
    3604:	00048d1c 	andeq	r8, r4, ip, lsl sp
    3608:	00048cd8 	ldrdeq	r8, [r4], -r8	; <UNPREDICTABLE>

0000360c <sd_init>:
    360c:	e1a0c00d 	mov	ip, sp
    3610:	e3a03000 	mov	r3, #0
    3614:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    3618:	e59f4114 	ldr	r4, [pc, #276]	; 3734 <sd_init+0x128>
    361c:	e24cb004 	sub	fp, ip, #4
    3620:	e24dd040 	sub	sp, sp, #64	; 0x40
    3624:	e08f4004 	add	r4, pc, r4
    3628:	e1a01003 	mov	r1, r3
    362c:	e3a02010 	mov	r2, #16
    3630:	e24b5054 	sub	r5, fp, #84	; 0x54
    3634:	e1a00004 	mov	r0, r4
    3638:	e5843010 	str	r3, [r4, #16]
    363c:	e5843014 	str	r3, [r4, #20]
    3640:	eb002a68 	bl	dfe8 <memset>
    3644:	e1a01005 	mov	r1, r5
    3648:	e3a0001e 	mov	r0, #30
    364c:	eb00113e 	bl	7b4c <syscall1>
    3650:	e59f10e0 	ldr	r1, [pc, #224]	; 3738 <sd_init+0x12c>
    3654:	e1a00005 	mov	r0, r5
    3658:	e08f1001 	add	r1, pc, r1
    365c:	e3a02005 	mov	r2, #5
    3660:	eb002bd2 	bl	e5b0 <strncmp>
    3664:	e59f30d0 	ldr	r3, [pc, #208]	; 373c <sd_init+0x130>
    3668:	e3500000 	cmp	r0, #0
    366c:	e08f3003 	add	r3, pc, r3
    3670:	1a000018 	bne	36d8 <sd_init+0xcc>
    3674:	e59f20c4 	ldr	r2, [pc, #196]	; 3740 <sd_init+0x134>
    3678:	e7932002 	ldr	r2, [r3, r2]
    367c:	e59f10c0 	ldr	r1, [pc, #192]	; 3744 <sd_init+0x138>
    3680:	e5842060 	str	r2, [r4, #96]	; 0x60
    3684:	e7930001 	ldr	r0, [r3, r1]
    3688:	e59f10b8 	ldr	r1, [pc, #184]	; 3748 <sd_init+0x13c>
    368c:	e5840018 	str	r0, [r4, #24]
    3690:	e7933001 	ldr	r3, [r3, r1]
    3694:	e584301c 	str	r3, [r4, #28]
    3698:	e1a0e00f 	mov	lr, pc
    369c:	e12fff12 	bx	r2
    36a0:	e2504000 	subs	r4, r0, #0
    36a4:	1a000020 	bne	372c <sd_init+0x120>
    36a8:	ebffff76 	bl	3488 <read_partition>
    36ac:	e3500000 	cmp	r0, #0
    36b0:	0a000013 	beq	3704 <sd_init+0xf8>
    36b4:	e59f0090 	ldr	r0, [pc, #144]	; 374c <sd_init+0x140>
    36b8:	e3a01000 	mov	r1, #0
    36bc:	e08f0000 	add	r0, pc, r0
    36c0:	e3a02010 	mov	r2, #16
    36c4:	eb002a47 	bl	dfe8 <memset>
    36c8:	e1a00004 	mov	r0, r4
    36cc:	e24bd014 	sub	sp, fp, #20
    36d0:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    36d4:	e12fff1e 	bx	lr
    36d8:	e59f2070 	ldr	r2, [pc, #112]	; 3750 <sd_init+0x144>
    36dc:	e7931002 	ldr	r1, [r3, r2]
    36e0:	e5841060 	str	r1, [r4, #96]	; 0x60
    36e4:	e1a02001 	mov	r2, r1
    36e8:	e59f1064 	ldr	r1, [pc, #100]	; 3754 <sd_init+0x148>
    36ec:	e7930001 	ldr	r0, [r3, r1]
    36f0:	e59f1060 	ldr	r1, [pc, #96]	; 3758 <sd_init+0x14c>
    36f4:	e5840018 	str	r0, [r4, #24]
    36f8:	e7933001 	ldr	r3, [r3, r1]
    36fc:	e584301c 	str	r3, [r4, #28]
    3700:	eaffffe4 	b	3698 <sd_init+0x8c>
    3704:	e59f1050 	ldr	r1, [pc, #80]	; 375c <sd_init+0x150>
    3708:	e3a00001 	mov	r0, #1
    370c:	e08f1001 	add	r1, pc, r1
    3710:	ebffff7a 	bl	3500 <partition_get>
    3714:	e3500000 	cmp	r0, #0
    3718:	1affffe5 	bne	36b4 <sd_init+0xa8>
    371c:	e1a00004 	mov	r0, r4
    3720:	e24bd014 	sub	sp, fp, #20
    3724:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    3728:	e12fff1e 	bx	lr
    372c:	e3e04000 	mvn	r4, #0
    3730:	eaffffe4 	b	36c8 <sd_init+0xbc>
    3734:	00048ca0 	andeq	r8, r4, r0, lsr #25
    3738:	0000bec8 	andeq	fp, r0, r8, asr #29
    373c:	0001438c 	andeq	r4, r1, ip, lsl #7
    3740:	00000040 	andeq	r0, r0, r0, asr #32
    3744:	00000054 	andeq	r0, r0, r4, asr r0
    3748:	00000014 	andeq	r0, r0, r4, lsl r0
    374c:	00048c08 	andeq	r8, r4, r8, lsl #24
    3750:	00000038 	andeq	r0, r0, r8, lsr r0
    3754:	0000001c 	andeq	r0, r0, ip, lsl r0
    3758:	00000020 	andeq	r0, r0, r0, lsr #32
    375c:	00048bb8 			; <UNDEFINED> instruction: 0x00048bb8

00003760 <set_super>:
    3760:	e1a0c00d 	mov	ip, sp
    3764:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    3768:	e1a04000 	mov	r4, r0
    376c:	e24cb004 	sub	fp, ip, #4
    3770:	e24b5e41 	sub	r5, fp, #1040	; 0x410
    3774:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    3778:	e2455004 	sub	r5, r5, #4
    377c:	e2801004 	add	r1, r0, #4
    3780:	e1a00005 	mov	r0, r5
    3784:	e3a02b01 	mov	r2, #1024	; 0x400
    3788:	eb0029da 	bl	def8 <memcpy>
    378c:	e5943000 	ldr	r3, [r4]
    3790:	e3530001 	cmp	r3, #1
    3794:	c59f002c 	ldrgt	r0, [pc, #44]	; 37c8 <set_super+0x68>
    3798:	c1a01005 	movgt	r1, r5
    379c:	c594c40c 	ldrgt	ip, [r4, #1036]	; 0x40c
    37a0:	c1a0e00f 	movgt	lr, pc
    37a4:	c12fff1c 	bxgt	ip
    37a8:	e1a01005 	mov	r1, r5
    37ac:	e3a00001 	mov	r0, #1
    37b0:	e594c40c 	ldr	ip, [r4, #1036]	; 0x40c
    37b4:	e1a0e00f 	mov	lr, pc
    37b8:	e12fff1c 	bx	ip
    37bc:	e24bd014 	sub	sp, fp, #20
    37c0:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    37c4:	e12fff1e 	bx	lr
    37c8:	00002001 	andeq	r2, r0, r1

000037cc <get_node_by_ino>:
    37cc:	e1a0c00d 	mov	ip, sp
    37d0:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    37d4:	e1a04001 	mov	r4, r1
    37d8:	e1a06000 	mov	r6, r0
    37dc:	e24cb004 	sub	fp, ip, #4
    37e0:	e590102c 	ldr	r1, [r0, #44]	; 0x2c
    37e4:	e1a00004 	mov	r0, r4
    37e8:	e1a05002 	mov	r5, r2
    37ec:	eb002236 	bl	c0cc <div_u32>
    37f0:	e596102c 	ldr	r1, [r6, #44]	; 0x2c
    37f4:	e0010190 	mul	r1, r0, r1
    37f8:	e5963404 	ldr	r3, [r6, #1028]	; 0x404
    37fc:	e0611004 	rsb	r1, r1, r4
    3800:	e0830280 	add	r0, r3, r0, lsl #5
    3804:	e2514001 	subs	r4, r1, #1
    3808:	e5903008 	ldr	r3, [r0, #8]
    380c:	42810006 	addmi	r0, r1, #6
    3810:	51a00004 	movpl	r0, r4
    3814:	e08301c0 	add	r0, r3, r0, asr #3
    3818:	e1a01005 	mov	r1, r5
    381c:	e596c408 	ldr	ip, [r6, #1032]	; 0x408
    3820:	e1a0e00f 	mov	lr, pc
    3824:	e12fff1c 	bx	ip
    3828:	e1a00fc4 	asr	r0, r4, #31
    382c:	e1a00ea0 	lsr	r0, r0, #29
    3830:	e0844000 	add	r4, r4, r0
    3834:	e2044007 	and	r4, r4, #7
    3838:	e0600004 	rsb	r0, r0, r4
    383c:	e0850380 	add	r0, r5, r0, lsl #7
    3840:	e24bd01c 	sub	sp, fp, #28
    3844:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    3848:	e12fff1e 	bx	lr

0000384c <set_gd.isra.3>:
    384c:	e1a0c00d 	mov	ip, sp
    3850:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    3854:	e24cb004 	sub	fp, ip, #4
    3858:	e1a06000 	mov	r6, r0
    385c:	e1a05001 	mov	r5, r1
    3860:	e3a00b01 	mov	r0, #1024	; 0x400
    3864:	e3a01020 	mov	r1, #32
    3868:	e1a07002 	mov	r7, r2
    386c:	eb002216 	bl	c0cc <div_u32>
    3870:	e1a04000 	mov	r4, r0
    3874:	e1a00007 	mov	r0, r7
    3878:	e1a01004 	mov	r1, r4
    387c:	eb002212 	bl	c0cc <div_u32>
    3880:	e0040490 	mul	r4, r0, r4
    3884:	e5961000 	ldr	r1, [r6]
    3888:	e5953000 	ldr	r3, [r5]
    388c:	e0811284 	add	r1, r1, r4, lsl #5
    3890:	e2800002 	add	r0, r0, #2
    3894:	e24bd01c 	sub	sp, fp, #28
    3898:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    389c:	e12fff13 	bx	r3

000038a0 <ext2_ialloc>:
    38a0:	e1a0c00d 	mov	ip, sp
    38a4:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    38a8:	e24cb004 	sub	fp, ip, #4
    38ac:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    38b0:	e5903004 	ldr	r3, [r0, #4]
    38b4:	e3530000 	cmp	r3, #0
    38b8:	e1a05000 	mov	r5, r0
    38bc:	1a000003 	bne	38d0 <ext2_ialloc+0x30>
    38c0:	e3a00000 	mov	r0, #0
    38c4:	e24bd01c 	sub	sp, fp, #28
    38c8:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    38cc:	e12fff1e 	bx	lr
    38d0:	e3a04000 	mov	r4, #0
    38d4:	e590102c 	ldr	r1, [r0, #44]	; 0x2c
    38d8:	e1a00004 	mov	r0, r4
    38dc:	eb002288 	bl	c304 <mod_u32>
    38e0:	e24b6e41 	sub	r6, fp, #1040	; 0x410
    38e4:	e3500000 	cmp	r0, #0
    38e8:	e1a07004 	mov	r7, r4
    38ec:	e246600c 	sub	r6, r6, #12
    38f0:	0a000037 	beq	39d4 <ext2_ialloc+0x134>
    38f4:	e595102c 	ldr	r1, [r5, #44]	; 0x2c
    38f8:	e0030791 	mul	r3, r1, r7
    38fc:	e0633004 	rsb	r3, r3, r4
    3900:	e3530000 	cmp	r3, #0
    3904:	e2832007 	add	r2, r3, #7
    3908:	a1a02003 	movge	r2, r3
    390c:	e1a00fc3 	asr	r0, r3, #31
    3910:	e1a00ea0 	lsr	r0, r0, #29
    3914:	e0833000 	add	r3, r3, r0
    3918:	e7d6c1c2 	ldrb	ip, [r6, r2, asr #3]
    391c:	e2033007 	and	r3, r3, #7
    3920:	e0603003 	rsb	r3, r0, r3
    3924:	e1a0035c 	asr	r0, ip, r3
    3928:	e3100001 	tst	r0, #1
    392c:	e08621c2 	add	r2, r6, r2, asr #3
    3930:	1a00001f 	bne	39b4 <ext2_ialloc+0x114>
    3934:	e3a00001 	mov	r0, #1
    3938:	e5951404 	ldr	r1, [r5, #1028]	; 0x404
    393c:	e18c3310 	orr	r3, ip, r0, lsl r3
    3940:	e5c23000 	strb	r3, [r2]
    3944:	e0817287 	add	r7, r1, r7, lsl #5
    3948:	e5970004 	ldr	r0, [r7, #4]
    394c:	e1a01006 	mov	r1, r6
    3950:	e595c40c 	ldr	ip, [r5, #1036]	; 0x40c
    3954:	e1a0e00f 	mov	lr, pc
    3958:	e12fff1c 	bx	ip
    395c:	e595102c 	ldr	r1, [r5, #44]	; 0x2c
    3960:	e1a00004 	mov	r0, r4
    3964:	eb0021d8 	bl	c0cc <div_u32>
    3968:	e5953404 	ldr	r3, [r5, #1028]	; 0x404
    396c:	e0833280 	add	r3, r3, r0, lsl #5
    3970:	e1d3c0be 	ldrh	ip, [r3, #14]
    3974:	e2851b01 	add	r1, r5, #1024	; 0x400
    3978:	e24cc001 	sub	ip, ip, #1
    397c:	e1a02000 	mov	r2, r0
    3980:	e1c3c0be 	strh	ip, [r3, #14]
    3984:	e2810004 	add	r0, r1, #4
    3988:	e281100c 	add	r1, r1, #12
    398c:	ebffffae 	bl	384c <set_gd.isra.3>
    3990:	e5953014 	ldr	r3, [r5, #20]
    3994:	e2433001 	sub	r3, r3, #1
    3998:	e5853014 	str	r3, [r5, #20]
    399c:	e1a00005 	mov	r0, r5
    39a0:	ebffff6e 	bl	3760 <set_super>
    39a4:	e2840001 	add	r0, r4, #1
    39a8:	e24bd01c 	sub	sp, fp, #28
    39ac:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    39b0:	e12fff1e 	bx	lr
    39b4:	e5953004 	ldr	r3, [r5, #4]
    39b8:	e2844001 	add	r4, r4, #1
    39bc:	e1530004 	cmp	r3, r4
    39c0:	9affffbe 	bls	38c0 <ext2_ialloc+0x20>
    39c4:	e1a00004 	mov	r0, r4
    39c8:	eb00224d 	bl	c304 <mod_u32>
    39cc:	e3500000 	cmp	r0, #0
    39d0:	1affffc7 	bne	38f4 <ext2_ialloc+0x54>
    39d4:	e595102c 	ldr	r1, [r5, #44]	; 0x2c
    39d8:	e1a00004 	mov	r0, r4
    39dc:	eb0021ba 	bl	c0cc <div_u32>
    39e0:	e5953404 	ldr	r3, [r5, #1028]	; 0x404
    39e4:	e0833280 	add	r3, r3, r0, lsl #5
    39e8:	e1a07000 	mov	r7, r0
    39ec:	e1a01006 	mov	r1, r6
    39f0:	e5930004 	ldr	r0, [r3, #4]
    39f4:	e595c408 	ldr	ip, [r5, #1032]	; 0x408
    39f8:	e1a0e00f 	mov	lr, pc
    39fc:	e12fff1c 	bx	ip
    3a00:	eaffffbb 	b	38f4 <ext2_ialloc+0x54>

00003a04 <ext2_balloc>:
    3a04:	e1a0c00d 	mov	ip, sp
    3a08:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    3a0c:	e24cb004 	sub	fp, ip, #4
    3a10:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    3a14:	e1a04000 	mov	r4, r0
    3a18:	e5903404 	ldr	r3, [r0, #1028]	; 0x404
    3a1c:	e24b5e41 	sub	r5, fp, #1040	; 0x410
    3a20:	e245500c 	sub	r5, r5, #12
    3a24:	e5930000 	ldr	r0, [r3]
    3a28:	e1a01005 	mov	r1, r5
    3a2c:	e594c408 	ldr	ip, [r4, #1032]	; 0x408
    3a30:	e1a0e00f 	mov	lr, pc
    3a34:	e12fff1c 	bx	ip
    3a38:	e5943008 	ldr	r3, [r4, #8]
    3a3c:	e3530000 	cmp	r3, #0
    3a40:	1a000003 	bne	3a54 <ext2_balloc+0x50>
    3a44:	e3a00000 	mov	r0, #0
    3a48:	e24bd01c 	sub	sp, fp, #28
    3a4c:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    3a50:	e12fff1e 	bx	lr
    3a54:	e3a06000 	mov	r6, #0
    3a58:	e5941024 	ldr	r1, [r4, #36]	; 0x24
    3a5c:	e1a00006 	mov	r0, r6
    3a60:	eb002227 	bl	c304 <mod_u32>
    3a64:	e3500000 	cmp	r0, #0
    3a68:	e1a07006 	mov	r7, r6
    3a6c:	0a000036 	beq	3b4c <ext2_balloc+0x148>
    3a70:	e5941024 	ldr	r1, [r4, #36]	; 0x24
    3a74:	e0030791 	mul	r3, r1, r7
    3a78:	e0633006 	rsb	r3, r3, r6
    3a7c:	e3530000 	cmp	r3, #0
    3a80:	e2832007 	add	r2, r3, #7
    3a84:	a1a02003 	movge	r2, r3
    3a88:	e1a00fc3 	asr	r0, r3, #31
    3a8c:	e1a00ea0 	lsr	r0, r0, #29
    3a90:	e0833000 	add	r3, r3, r0
    3a94:	e7d5c1c2 	ldrb	ip, [r5, r2, asr #3]
    3a98:	e2033007 	and	r3, r3, #7
    3a9c:	e0603003 	rsb	r3, r0, r3
    3aa0:	e1a0035c 	asr	r0, ip, r3
    3aa4:	e3100001 	tst	r0, #1
    3aa8:	e08521c2 	add	r2, r5, r2, asr #3
    3aac:	1a00001e 	bne	3b2c <ext2_balloc+0x128>
    3ab0:	e3a01001 	mov	r1, #1
    3ab4:	e18c3311 	orr	r3, ip, r1, lsl r3
    3ab8:	e5c23000 	strb	r3, [r2]
    3abc:	e5943404 	ldr	r3, [r4, #1028]	; 0x404
    3ac0:	e1a01005 	mov	r1, r5
    3ac4:	e7930287 	ldr	r0, [r3, r7, lsl #5]
    3ac8:	e594c40c 	ldr	ip, [r4, #1036]	; 0x40c
    3acc:	e1a0e00f 	mov	lr, pc
    3ad0:	e12fff1c 	bx	ip
    3ad4:	e5941024 	ldr	r1, [r4, #36]	; 0x24
    3ad8:	e1a00006 	mov	r0, r6
    3adc:	eb00217a 	bl	c0cc <div_u32>
    3ae0:	e5943404 	ldr	r3, [r4, #1028]	; 0x404
    3ae4:	e0833280 	add	r3, r3, r0, lsl #5
    3ae8:	e1d3c0bc 	ldrh	ip, [r3, #12]
    3aec:	e2841b01 	add	r1, r4, #1024	; 0x400
    3af0:	e24cc001 	sub	ip, ip, #1
    3af4:	e1a02000 	mov	r2, r0
    3af8:	e1c3c0bc 	strh	ip, [r3, #12]
    3afc:	e2810004 	add	r0, r1, #4
    3b00:	e281100c 	add	r1, r1, #12
    3b04:	ebffff50 	bl	384c <set_gd.isra.3>
    3b08:	e5943010 	ldr	r3, [r4, #16]
    3b0c:	e2433001 	sub	r3, r3, #1
    3b10:	e5843010 	str	r3, [r4, #16]
    3b14:	e1a00004 	mov	r0, r4
    3b18:	ebffff10 	bl	3760 <set_super>
    3b1c:	e2860001 	add	r0, r6, #1
    3b20:	e24bd01c 	sub	sp, fp, #28
    3b24:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    3b28:	e12fff1e 	bx	lr
    3b2c:	e5943008 	ldr	r3, [r4, #8]
    3b30:	e2866001 	add	r6, r6, #1
    3b34:	e1530006 	cmp	r3, r6
    3b38:	9affffc1 	bls	3a44 <ext2_balloc+0x40>
    3b3c:	e1a00006 	mov	r0, r6
    3b40:	eb0021ef 	bl	c304 <mod_u32>
    3b44:	e3500000 	cmp	r0, #0
    3b48:	1affffc8 	bne	3a70 <ext2_balloc+0x6c>
    3b4c:	e5941024 	ldr	r1, [r4, #36]	; 0x24
    3b50:	e1a00006 	mov	r0, r6
    3b54:	eb00215c 	bl	c0cc <div_u32>
    3b58:	e5943404 	ldr	r3, [r4, #1028]	; 0x404
    3b5c:	e1a07000 	mov	r7, r0
    3b60:	e1a01005 	mov	r1, r5
    3b64:	e7930280 	ldr	r0, [r3, r0, lsl #5]
    3b68:	e594c408 	ldr	ip, [r4, #1032]	; 0x408
    3b6c:	e1a0e00f 	mov	lr, pc
    3b70:	e12fff1c 	bx	ip
    3b74:	eaffffbd 	b	3a70 <ext2_balloc+0x6c>

00003b78 <enter_child>:
    3b78:	e1a0c00d 	mov	ip, sp
    3b7c:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    3b80:	e24dde41 	sub	sp, sp, #1040	; 0x410
    3b84:	e24cb004 	sub	fp, ip, #4
    3b88:	e24dd004 	sub	sp, sp, #4
    3b8c:	e1a07000 	mov	r7, r0
    3b90:	e1a00003 	mov	r0, r3
    3b94:	e1a04001 	mov	r4, r1
    3b98:	e50b3438 	str	r3, [fp, #-1080]	; 0x438
    3b9c:	e50b1430 	str	r1, [fp, #-1072]	; 0x430
    3ba0:	e50b2434 	str	r2, [fp, #-1076]	; 0x434
    3ba4:	eb002a69 	bl	e550 <strlen>
    3ba8:	e3a09000 	mov	r9, #0
    3bac:	e290600b 	adds	r6, r0, #11
    3bb0:	4280600e 	addmi	r6, r0, #14
    3bb4:	e24bae42 	sub	sl, fp, #1056	; 0x420
    3bb8:	e5945028 	ldr	r5, [r4, #40]	; 0x28
    3bbc:	e3c66003 	bic	r6, r6, #3
    3bc0:	e284802c 	add	r8, r4, #44	; 0x2c
    3bc4:	e24aa00c 	sub	sl, sl, #12
    3bc8:	e1a00005 	mov	r0, r5
    3bcc:	e1a0100a 	mov	r1, sl
    3bd0:	e597c408 	ldr	ip, [r7, #1032]	; 0x408
    3bd4:	e1a0e00f 	mov	lr, pc
    3bd8:	e12fff1c 	bx	ip
    3bdc:	e51b542c 	ldr	r5, [fp, #-1068]	; 0x42c
    3be0:	e3550000 	cmp	r5, #0
    3be4:	0a000019 	beq	3c50 <enter_child+0xd8>
    3be8:	e1a0400a 	mov	r4, sl
    3bec:	ea000000 	b	3bf4 <enter_child+0x7c>
    3bf0:	e1a04001 	mov	r4, r1
    3bf4:	e1d400b4 	ldrh	r0, [r4, #4]
    3bf8:	e24b302c 	sub	r3, fp, #44	; 0x2c
    3bfc:	e0841000 	add	r1, r4, r0
    3c00:	e1510003 	cmp	r1, r3
    3c04:	3afffff9 	bcc	3bf0 <enter_child+0x78>
    3c08:	e5d4e006 	ldrb	lr, [r4, #6]
    3c0c:	e28ee00b 	add	lr, lr, #11
    3c10:	e3cee003 	bic	lr, lr, #3
    3c14:	e06e1000 	rsb	r1, lr, r0
    3c18:	e1510006 	cmp	r1, r6
    3c1c:	aa000026 	bge	3cbc <enter_child+0x144>
    3c20:	e2899001 	add	r9, r9, #1
    3c24:	e359000c 	cmp	r9, #12
    3c28:	0a000004 	beq	3c40 <enter_child+0xc8>
    3c2c:	e5985000 	ldr	r5, [r8]
    3c30:	e3550000 	cmp	r5, #0
    3c34:	0a000030 	beq	3cfc <enter_child+0x184>
    3c38:	e2888004 	add	r8, r8, #4
    3c3c:	eaffffe1 	b	3bc8 <enter_child+0x50>
    3c40:	e3e00000 	mvn	r0, #0
    3c44:	e24bd028 	sub	sp, fp, #40	; 0x28
    3c48:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    3c4c:	e12fff1e 	bx	lr
    3c50:	e3a02b01 	mov	r2, #1024	; 0x400
    3c54:	e51b4438 	ldr	r4, [fp, #-1080]	; 0x438
    3c58:	e51b3434 	ldr	r3, [fp, #-1076]	; 0x434
    3c5c:	e1a00004 	mov	r0, r4
    3c60:	e50b342c 	str	r3, [fp, #-1068]	; 0x42c
    3c64:	e1ca20b4 	strh	r2, [sl, #4]
    3c68:	eb002a38 	bl	e550 <strlen>
    3c6c:	e1a0e000 	mov	lr, r0
    3c70:	e1a01004 	mov	r1, r4
    3c74:	e5db3004 	ldrb	r3, [fp, #4]
    3c78:	e2802001 	add	r2, r0, #1
    3c7c:	e54be426 	strb	lr, [fp, #-1062]	; 0x426
    3c80:	e24b0e42 	sub	r0, fp, #1056	; 0x420
    3c84:	e54b3425 	strb	r3, [fp, #-1061]	; 0x425
    3c88:	e2400004 	sub	r0, r0, #4
    3c8c:	eb002899 	bl	def8 <memcpy>
    3c90:	e51b2430 	ldr	r2, [fp, #-1072]	; 0x430
    3c94:	e289300a 	add	r3, r9, #10
    3c98:	e7920103 	ldr	r0, [r2, r3, lsl #2]
    3c9c:	e1a0100a 	mov	r1, sl
    3ca0:	e597c40c 	ldr	ip, [r7, #1036]	; 0x40c
    3ca4:	e1a0e00f 	mov	lr, pc
    3ca8:	e12fff1c 	bx	ip
    3cac:	e3a00000 	mov	r0, #0
    3cb0:	e24bd028 	sub	sp, fp, #40	; 0x28
    3cb4:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    3cb8:	e12fff1e 	bx	lr
    3cbc:	e51b3434 	ldr	r3, [fp, #-1076]	; 0x434
    3cc0:	e51b6438 	ldr	r6, [fp, #-1080]	; 0x438
    3cc4:	e084500e 	add	r5, r4, lr
    3cc8:	e1c4e0b4 	strh	lr, [r4, #4]
    3ccc:	e1a00006 	mov	r0, r6
    3cd0:	e784300e 	str	r3, [r4, lr]
    3cd4:	e1c510b4 	strh	r1, [r5, #4]
    3cd8:	eb002a1c 	bl	e550 <strlen>
    3cdc:	e1a02000 	mov	r2, r0
    3ce0:	e5db3004 	ldrb	r3, [fp, #4]
    3ce4:	e5c50006 	strb	r0, [r5, #6]
    3ce8:	e5c53007 	strb	r3, [r5, #7]
    3cec:	e2850008 	add	r0, r5, #8
    3cf0:	e2822001 	add	r2, r2, #1
    3cf4:	e1a01006 	mov	r1, r6
    3cf8:	eaffffe3 	b	3c8c <enter_child+0x114>
    3cfc:	e1a00007 	mov	r0, r7
    3d00:	ebffff3f 	bl	3a04 <ext2_balloc>
    3d04:	e2502000 	subs	r2, r0, #0
    3d08:	daffffcc 	ble	3c40 <enter_child+0xc8>
    3d0c:	e51b3430 	ldr	r3, [fp, #-1072]	; 0x430
    3d10:	e0836109 	add	r6, r3, r9, lsl #2
    3d14:	e5862028 	str	r2, [r6, #40]	; 0x28
    3d18:	e5932004 	ldr	r2, [r3, #4]
    3d1c:	e2822b01 	add	r2, r2, #1024	; 0x400
    3d20:	e1a0100a 	mov	r1, sl
    3d24:	e5832004 	str	r2, [r3, #4]
    3d28:	e597c408 	ldr	ip, [r7, #1032]	; 0x408
    3d2c:	e1a0e00f 	mov	lr, pc
    3d30:	e12fff1c 	bx	ip
    3d34:	e3a03b01 	mov	r3, #1024	; 0x400
    3d38:	e51b2434 	ldr	r2, [fp, #-1076]	; 0x434
    3d3c:	e51b8438 	ldr	r8, [fp, #-1080]	; 0x438
    3d40:	e5842000 	str	r2, [r4]
    3d44:	e1c430b4 	strh	r3, [r4, #4]
    3d48:	e1a00008 	mov	r0, r8
    3d4c:	eb0029ff 	bl	e550 <strlen>
    3d50:	e5db3004 	ldrb	r3, [fp, #4]
    3d54:	e2802001 	add	r2, r0, #1
    3d58:	e5c43007 	strb	r3, [r4, #7]
    3d5c:	e5c40006 	strb	r0, [r4, #6]
    3d60:	e1a01008 	mov	r1, r8
    3d64:	e2840008 	add	r0, r4, #8
    3d68:	eb002862 	bl	def8 <memcpy>
    3d6c:	e5960028 	ldr	r0, [r6, #40]	; 0x28
    3d70:	e1a0100a 	mov	r1, sl
    3d74:	e597c40c 	ldr	ip, [r7, #1036]	; 0x40c
    3d78:	e1a0e00f 	mov	lr, pc
    3d7c:	e12fff1c 	bx	ip
    3d80:	e1a00005 	mov	r0, r5
    3d84:	e24bd028 	sub	sp, fp, #40	; 0x28
    3d88:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    3d8c:	e12fff1e 	bx	lr

00003d90 <search.isra.6>:
    3d90:	e1a0c00d 	mov	ip, sp
    3d94:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    3d98:	e24cb004 	sub	fp, ip, #4
    3d9c:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    3da0:	e24b3e42 	sub	r3, fp, #1056	; 0x420
    3da4:	e24dd00c 	sub	sp, sp, #12
    3da8:	e243300c 	sub	r3, r3, #12
    3dac:	e1a06002 	mov	r6, r2
    3db0:	e50b0434 	str	r0, [fp, #-1076]	; 0x434
    3db4:	e3a05000 	mov	r5, #0
    3db8:	e2817024 	add	r7, r1, #36	; 0x24
    3dbc:	e2818054 	add	r8, r1, #84	; 0x54
    3dc0:	e50b3430 	str	r3, [fp, #-1072]	; 0x430
    3dc4:	ea000001 	b	3dd0 <search.isra.6+0x40>
    3dc8:	e1570008 	cmp	r7, r8
    3dcc:	0a000021 	beq	3e58 <search.isra.6+0xc8>
    3dd0:	e5b70004 	ldr	r0, [r7, #4]!
    3dd4:	e3500000 	cmp	r0, #0
    3dd8:	0afffffa 	beq	3dc8 <search.isra.6+0x38>
    3ddc:	e51b3434 	ldr	r3, [fp, #-1076]	; 0x434
    3de0:	e51b1430 	ldr	r1, [fp, #-1072]	; 0x430
    3de4:	e593c000 	ldr	ip, [r3]
    3de8:	e1a0e00f 	mov	lr, pc
    3dec:	e12fff1c 	bx	ip
    3df0:	e55b3426 	ldrb	r3, [fp, #-1062]	; 0x426
    3df4:	e3530000 	cmp	r3, #0
    3df8:	151ba430 	ldrne	sl, [fp, #-1072]	; 0x430
    3dfc:	1a000009 	bne	3e28 <search.isra.6+0x98>
    3e00:	eafffff0 	b	3dc8 <search.isra.6+0x38>
    3e04:	e1da10b4 	ldrh	r1, [sl, #4]
    3e08:	e24b302c 	sub	r3, fp, #44	; 0x2c
    3e0c:	e08aa001 	add	sl, sl, r1
    3e10:	e15a0003 	cmp	sl, r3
    3e14:	e5c94008 	strb	r4, [r9, #8]
    3e18:	2affffea 	bcs	3dc8 <search.isra.6+0x38>
    3e1c:	e5da3006 	ldrb	r3, [sl, #6]
    3e20:	e3530000 	cmp	r3, #0
    3e24:	0affffe7 	beq	3dc8 <search.isra.6+0x38>
    3e28:	e08a9003 	add	r9, sl, r3
    3e2c:	e28a0008 	add	r0, sl, #8
    3e30:	e1a01006 	mov	r1, r6
    3e34:	e5d94008 	ldrb	r4, [r9, #8]
    3e38:	e5c95008 	strb	r5, [r9, #8]
    3e3c:	eb0028fe 	bl	e23c <strcmp>
    3e40:	e3500000 	cmp	r0, #0
    3e44:	1affffee 	bne	3e04 <search.isra.6+0x74>
    3e48:	e59a0000 	ldr	r0, [sl]
    3e4c:	e24bd028 	sub	sp, fp, #40	; 0x28
    3e50:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    3e54:	e12fff1e 	bx	lr
    3e58:	e3e00000 	mvn	r0, #0
    3e5c:	e24bd028 	sub	sp, fp, #40	; 0x28
    3e60:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    3e64:	e12fff1e 	bx	lr

00003e68 <ext2_ino_by_fname.part.8>:
    3e68:	e1a0c00d 	mov	ip, sp
    3e6c:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    3e70:	e24ddd12 	sub	sp, sp, #1152	; 0x480
    3e74:	e24cb004 	sub	fp, ip, #4
    3e78:	e24dd00c 	sub	sp, sp, #12
    3e7c:	e1a08001 	mov	r8, r1
    3e80:	e3a05000 	mov	r5, #0
    3e84:	e5d82000 	ldrb	r2, [r8]
    3e88:	e24b3e4a 	sub	r3, fp, #1184	; 0x4a0
    3e8c:	e24b6e42 	sub	r6, fp, #1056	; 0x420
    3e90:	e243300c 	sub	r3, r3, #12
    3e94:	e352002f 	cmp	r2, #47	; 0x2f
    3e98:	e1a07000 	mov	r7, r0
    3e9c:	e1a04005 	mov	r4, r5
    3ea0:	e246600c 	sub	r6, r6, #12
    3ea4:	e50b34b0 	str	r3, [fp, #-1200]	; 0x4b0
    3ea8:	0a00001d 	beq	3f24 <ext2_ino_by_fname.part.8+0xbc>
    3eac:	e3a03000 	mov	r3, #0
    3eb0:	e2461001 	sub	r1, r6, #1
    3eb4:	ea000007 	b	3ed8 <ext2_ino_by_fname.part.8+0x70>
    3eb8:	e5f80001 	ldrb	r0, [r8, #1]!
    3ebc:	e3500000 	cmp	r0, #0
    3ec0:	e5e12001 	strb	r2, [r1, #1]!
    3ec4:	e2833001 	add	r3, r3, #1
    3ec8:	0a000004 	beq	3ee0 <ext2_ino_by_fname.part.8+0x78>
    3ecc:	e353003f 	cmp	r3, #63	; 0x3f
    3ed0:	0a000002 	beq	3ee0 <ext2_ino_by_fname.part.8+0x78>
    3ed4:	e1a02000 	mov	r2, r0
    3ed8:	e352002f 	cmp	r2, #47	; 0x2f
    3edc:	1afffff5 	bne	3eb8 <ext2_ino_by_fname.part.8+0x50>
    3ee0:	e24b202c 	sub	r2, fp, #44	; 0x2c
    3ee4:	e0823003 	add	r3, r2, r3
    3ee8:	e1a00006 	mov	r0, r6
    3eec:	e5434400 	strb	r4, [r3, #-1024]	; 0x400
    3ef0:	eb000f65 	bl	7c8c <str_new>
    3ef4:	e51b34b0 	ldr	r3, [fp, #-1200]	; 0x4b0
    3ef8:	e7830105 	str	r0, [r3, r5, lsl #2]
    3efc:	e2855001 	add	r5, r5, #1
    3f00:	e3550020 	cmp	r5, #32
    3f04:	0a000009 	beq	3f30 <ext2_ino_by_fname.part.8+0xc8>
    3f08:	e5d83000 	ldrb	r3, [r8]
    3f0c:	e3530000 	cmp	r3, #0
    3f10:	0a000006 	beq	3f30 <ext2_ino_by_fname.part.8+0xc8>
    3f14:	e2888001 	add	r8, r8, #1
    3f18:	e5d82000 	ldrb	r2, [r8]
    3f1c:	e352002f 	cmp	r2, #47	; 0x2f
    3f20:	1affffe1 	bne	3eac <ext2_ino_by_fname.part.8+0x44>
    3f24:	e5d82001 	ldrb	r2, [r8, #1]
    3f28:	e2888001 	add	r8, r8, #1
    3f2c:	eaffffde 	b	3eac <ext2_ino_by_fname.part.8+0x44>
    3f30:	e5973000 	ldr	r3, [r7]
    3f34:	e3530000 	cmp	r3, #0
    3f38:	da000032 	ble	4008 <ext2_ino_by_fname.part.8+0x1a0>
    3f3c:	e3a0a000 	mov	sl, #0
    3f40:	e2453107 	sub	r3, r5, #-1073741823	; 0xc0000001
    3f44:	e51b24b0 	ldr	r2, [fp, #-1200]	; 0x4b0
    3f48:	e2878b01 	add	r8, r7, #1024	; 0x400
    3f4c:	e50b34b4 	str	r3, [fp, #-1204]	; 0x4b4
    3f50:	e0829103 	add	r9, r2, r3, lsl #2
    3f54:	e2888008 	add	r8, r8, #8
    3f58:	e5973404 	ldr	r3, [r7, #1028]	; 0x404
    3f5c:	e083328a 	add	r3, r3, sl, lsl #5
    3f60:	e5930008 	ldr	r0, [r3, #8]
    3f64:	e1a01006 	mov	r1, r6
    3f68:	e597c408 	ldr	ip, [r7, #1032]	; 0x408
    3f6c:	e1a0e00f 	mov	lr, pc
    3f70:	e12fff1c 	bx	ip
    3f74:	e3500000 	cmp	r0, #0
    3f78:	1a00001c 	bne	3ff0 <ext2_ino_by_fname.part.8+0x188>
    3f7c:	e51b34b0 	ldr	r3, [fp, #-1200]	; 0x4b0
    3f80:	e24b1feb 	sub	r1, fp, #940	; 0x3ac
    3f84:	e2434004 	sub	r4, r3, #4
    3f88:	e5b43004 	ldr	r3, [r4, #4]!
    3f8c:	e1a00008 	mov	r0, r8
    3f90:	e5932000 	ldr	r2, [r3]
    3f94:	ebffff7d 	bl	3d90 <search.isra.6>
    3f98:	e2505000 	subs	r5, r0, #0
    3f9c:	ba000013 	blt	3ff0 <ext2_ino_by_fname.part.8+0x188>
    3fa0:	e1a01005 	mov	r1, r5
    3fa4:	e1a00007 	mov	r0, r7
    3fa8:	e1a02006 	mov	r2, r6
    3fac:	ebfffe06 	bl	37cc <get_node_by_ino>
    3fb0:	e2501000 	subs	r1, r0, #0
    3fb4:	0a00000d 	beq	3ff0 <ext2_ino_by_fname.part.8+0x188>
    3fb8:	e1540009 	cmp	r4, r9
    3fbc:	1afffff1 	bne	3f88 <ext2_ino_by_fname.part.8+0x120>
    3fc0:	e51b34b0 	ldr	r3, [fp, #-1200]	; 0x4b0
    3fc4:	e51b24b4 	ldr	r2, [fp, #-1204]	; 0x4b4
    3fc8:	e2434004 	sub	r4, r3, #4
    3fcc:	e0836102 	add	r6, r3, r2, lsl #2
    3fd0:	e5b40004 	ldr	r0, [r4, #4]!
    3fd4:	eb000f9f 	bl	7e58 <str_free>
    3fd8:	e1540006 	cmp	r4, r6
    3fdc:	1afffffb 	bne	3fd0 <ext2_ino_by_fname.part.8+0x168>
    3fe0:	e1a00005 	mov	r0, r5
    3fe4:	e24bd028 	sub	sp, fp, #40	; 0x28
    3fe8:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    3fec:	e12fff1e 	bx	lr
    3ff0:	e5973000 	ldr	r3, [r7]
    3ff4:	e28aa001 	add	sl, sl, #1
    3ff8:	e15a0003 	cmp	sl, r3
    3ffc:	baffffd5 	blt	3f58 <ext2_ino_by_fname.part.8+0xf0>
    4000:	e3e05000 	mvn	r5, #0
    4004:	eaffffed 	b	3fc0 <ext2_ino_by_fname.part.8+0x158>
    4008:	e2453107 	sub	r3, r5, #-1073741823	; 0xc0000001
    400c:	e50b34b4 	str	r3, [fp, #-1204]	; 0x4b4
    4010:	eafffffa 	b	4000 <ext2_ino_by_fname.part.8+0x198>

00004014 <ext2_read_block>:
    4014:	e1a0c00d 	mov	ip, sp
    4018:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    401c:	e24ddb02 	sub	sp, sp, #2048	; 0x800
    4020:	e24cb004 	sub	fp, ip, #4
    4024:	e24dd00c 	sub	sp, sp, #12
    4028:	e59be004 	ldr	lr, [fp, #4]
    402c:	e35e0000 	cmp	lr, #0
    4030:	e28ecfff 	add	ip, lr, #1020	; 0x3fc
    4034:	a1a0c00e 	movge	ip, lr
    4038:	e1a04fce 	asr	r4, lr, #31
    403c:	e1a05b24 	lsr	r5, r4, #22
    4040:	e08e7005 	add	r7, lr, r5
    4044:	e1a07b07 	lsl	r7, r7, #22
    4048:	e0654b27 	rsb	r4, r5, r7, lsr #22
    404c:	b28cc003 	addlt	ip, ip, #3
    4050:	e2649b01 	rsb	r9, r4, #1024	; 0x400
    4054:	e1590003 	cmp	r9, r3
    4058:	e1a0c54c 	asr	ip, ip, #10
    405c:	e5915004 	ldr	r5, [r1, #4]
    4060:	a1a06003 	movge	r6, r3
    4064:	b1a06009 	movlt	r6, r9
    4068:	e35c000b 	cmp	ip, #11
    406c:	e1a03000 	mov	r3, r0
    4070:	e1a08002 	mov	r8, r2
    4074:	e06e5005 	rsb	r5, lr, r5
    4078:	ca000021 	bgt	4104 <ext2_read_block+0xf0>
    407c:	e28cc00a 	add	ip, ip, #10
    4080:	e24b7e42 	sub	r7, fp, #1056	; 0x420
    4084:	e791010c 	ldr	r0, [r1, ip, lsl #2]
    4088:	e247700c 	sub	r7, r7, #12
    408c:	e1a01007 	mov	r1, r7
    4090:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    4094:	e1a0e00f 	mov	lr, pc
    4098:	e12fff1c 	bx	ip
    409c:	e3500000 	cmp	r0, #0
    40a0:	1a000044 	bne	41b8 <ext2_read_block+0x1a4>
    40a4:	e1a0a000 	mov	sl, r0
    40a8:	e0877004 	add	r7, r7, r4
    40ac:	e1550006 	cmp	r5, r6
    40b0:	b1a04005 	movlt	r4, r5
    40b4:	a1a04006 	movge	r4, r6
    40b8:	e1a00008 	mov	r0, r8
    40bc:	e1a02004 	mov	r2, r4
    40c0:	e1a01007 	mov	r1, r7
    40c4:	eb00278b 	bl	def8 <memcpy>
    40c8:	e0555004 	subs	r5, r5, r4
    40cc:	03a03001 	moveq	r3, #1
    40d0:	13a03000 	movne	r3, #0
    40d4:	e0566004 	subs	r6, r6, r4
    40d8:	03833001 	orreq	r3, r3, #1
    40dc:	e3530000 	cmp	r3, #0
    40e0:	e08aa004 	add	sl, sl, r4
    40e4:	e0649009 	rsb	r9, r4, r9
    40e8:	1a000001 	bne	40f4 <ext2_read_block+0xe0>
    40ec:	e3590000 	cmp	r9, #0
    40f0:	1affffed 	bne	40ac <ext2_read_block+0x98>
    40f4:	e1a0000a 	mov	r0, sl
    40f8:	e24bd028 	sub	sp, fp, #40	; 0x28
    40fc:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    4100:	e12fff1e 	bx	lr
    4104:	e24ca00c 	sub	sl, ip, #12
    4108:	e35a00ff 	cmp	sl, #255	; 0xff
    410c:	9a00001c 	bls	4184 <ext2_read_block+0x170>
    4110:	e591005c 	ldr	r0, [r1, #92]	; 0x5c
    4114:	e24b1e82 	sub	r1, fp, #2080	; 0x820
    4118:	e24ccf43 	sub	ip, ip, #268	; 0x10c
    411c:	e241100c 	sub	r1, r1, #12
    4120:	e1a0744c 	asr	r7, ip, #8
    4124:	e20ca0ff 	and	sl, ip, #255	; 0xff
    4128:	e50b3830 	str	r3, [fp, #-2096]	; 0x830
    412c:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    4130:	e1a0e00f 	mov	lr, pc
    4134:	e12fff1c 	bx	ip
    4138:	e3500000 	cmp	r0, #0
    413c:	1a00001d 	bne	41b8 <ext2_read_block+0x1a4>
    4140:	e24b302c 	sub	r3, fp, #44	; 0x2c
    4144:	e0832107 	add	r2, r3, r7, lsl #2
    4148:	e24b7e42 	sub	r7, fp, #1056	; 0x420
    414c:	e247700c 	sub	r7, r7, #12
    4150:	e5120800 	ldr	r0, [r2, #-2048]	; 0x800
    4154:	e1a01007 	mov	r1, r7
    4158:	e51b3830 	ldr	r3, [fp, #-2096]	; 0x830
    415c:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    4160:	e1a0e00f 	mov	lr, pc
    4164:	e12fff1c 	bx	ip
    4168:	e3500000 	cmp	r0, #0
    416c:	1a000011 	bne	41b8 <ext2_read_block+0x1a4>
    4170:	e24b302c 	sub	r3, fp, #44	; 0x2c
    4174:	e083a10a 	add	sl, r3, sl, lsl #2
    4178:	e51a0400 	ldr	r0, [sl, #-1024]	; 0x400
    417c:	e51b3830 	ldr	r3, [fp, #-2096]	; 0x830
    4180:	eaffffc1 	b	408c <ext2_read_block+0x78>
    4184:	e24b7e42 	sub	r7, fp, #1056	; 0x420
    4188:	e247700c 	sub	r7, r7, #12
    418c:	e5910058 	ldr	r0, [r1, #88]	; 0x58
    4190:	e1a01007 	mov	r1, r7
    4194:	e50b3830 	str	r3, [fp, #-2096]	; 0x830
    4198:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    419c:	e1a0e00f 	mov	lr, pc
    41a0:	e12fff1c 	bx	ip
    41a4:	e24b302c 	sub	r3, fp, #44	; 0x2c
    41a8:	e083a10a 	add	sl, r3, sl, lsl #2
    41ac:	e51a0400 	ldr	r0, [sl, #-1024]	; 0x400
    41b0:	e51b3830 	ldr	r3, [fp, #-2096]	; 0x830
    41b4:	eaffffb4 	b	408c <ext2_read_block+0x78>
    41b8:	e3e00000 	mvn	r0, #0
    41bc:	eaffffcd 	b	40f8 <ext2_read_block+0xe4>

000041c0 <ext2_read>:
    41c0:	e1a0c00d 	mov	ip, sp
    41c4:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    41c8:	e24cb004 	sub	fp, ip, #4
    41cc:	e24dd008 	sub	sp, sp, #8
    41d0:	e2539000 	subs	r9, r3, #0
    41d4:	e1a07000 	mov	r7, r0
    41d8:	e1a08001 	mov	r8, r1
    41dc:	e1a05002 	mov	r5, r2
    41e0:	e59b6004 	ldr	r6, [fp, #4]
    41e4:	c1a04009 	movgt	r4, r9
    41e8:	ca000004 	bgt	4200 <ext2_read+0x40>
    41ec:	ea000010 	b	4234 <ext2_read+0x74>
    41f0:	e3540000 	cmp	r4, #0
    41f4:	e0866000 	add	r6, r6, r0
    41f8:	e0855000 	add	r5, r5, r0
    41fc:	da00000c 	ble	4234 <ext2_read+0x74>
    4200:	e1a03004 	mov	r3, r4
    4204:	e58d6000 	str	r6, [sp]
    4208:	e1a00007 	mov	r0, r7
    420c:	e1a01008 	mov	r1, r8
    4210:	e1a02005 	mov	r2, r5
    4214:	ebffff7e 	bl	4014 <ext2_read_block>
    4218:	e3500000 	cmp	r0, #0
    421c:	e0604004 	rsb	r4, r0, r4
    4220:	cafffff2 	bgt	41f0 <ext2_read+0x30>
    4224:	e3a00000 	mov	r0, #0
    4228:	e24bd024 	sub	sp, fp, #36	; 0x24
    422c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    4230:	e12fff1e 	bx	lr
    4234:	e1a00009 	mov	r0, r9
    4238:	e24bd024 	sub	sp, fp, #36	; 0x24
    423c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    4240:	e12fff1e 	bx	lr

00004244 <put_node>:
    4244:	e1a0c00d 	mov	ip, sp
    4248:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    424c:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    4250:	e24cb004 	sub	fp, ip, #4
    4254:	e24dd004 	sub	sp, sp, #4
    4258:	e1a04001 	mov	r4, r1
    425c:	e1a05000 	mov	r5, r0
    4260:	e590102c 	ldr	r1, [r0, #44]	; 0x2c
    4264:	e1a00004 	mov	r0, r4
    4268:	e1a07002 	mov	r7, r2
    426c:	eb001f96 	bl	c0cc <div_u32>
    4270:	e595102c 	ldr	r1, [r5, #44]	; 0x2c
    4274:	e0010190 	mul	r1, r0, r1
    4278:	e5953404 	ldr	r3, [r5, #1028]	; 0x404
    427c:	e0611004 	rsb	r1, r1, r4
    4280:	e0830280 	add	r0, r3, r0, lsl #5
    4284:	e2514001 	subs	r4, r1, #1
    4288:	e5903008 	ldr	r3, [r0, #8]
    428c:	42818006 	addmi	r8, r1, #6
    4290:	51a08004 	movpl	r8, r4
    4294:	e24b6e42 	sub	r6, fp, #1056	; 0x420
    4298:	e08381c8 	add	r8, r3, r8, asr #3
    429c:	e2466004 	sub	r6, r6, #4
    42a0:	e1a01006 	mov	r1, r6
    42a4:	e1a00008 	mov	r0, r8
    42a8:	e595c408 	ldr	ip, [r5, #1032]	; 0x408
    42ac:	e1a0e00f 	mov	lr, pc
    42b0:	e12fff1c 	bx	ip
    42b4:	e1a00fc4 	asr	r0, r4, #31
    42b8:	e1a00ea0 	lsr	r0, r0, #29
    42bc:	e0844000 	add	r4, r4, r0
    42c0:	e2044007 	and	r4, r4, #7
    42c4:	e24b3024 	sub	r3, fp, #36	; 0x24
    42c8:	e0600004 	rsb	r0, r0, r4
    42cc:	e0830380 	add	r0, r3, r0, lsl #7
    42d0:	e1a01007 	mov	r1, r7
    42d4:	e3a02080 	mov	r2, #128	; 0x80
    42d8:	e2400b01 	sub	r0, r0, #1024	; 0x400
    42dc:	eb002705 	bl	def8 <memcpy>
    42e0:	e1a00008 	mov	r0, r8
    42e4:	e1a01006 	mov	r1, r6
    42e8:	e595c40c 	ldr	ip, [r5, #1036]	; 0x40c
    42ec:	e1a0e00f 	mov	lr, pc
    42f0:	e12fff1c 	bx	ip
    42f4:	e24bd020 	sub	sp, fp, #32
    42f8:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    42fc:	e12fff1e 	bx	lr

00004300 <ext2_create_dir>:
    4300:	e1a0c00d 	mov	ip, sp
    4304:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    4308:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    430c:	e24cb004 	sub	fp, ip, #4
    4310:	e24dd00c 	sub	sp, sp, #12
    4314:	e1a08003 	mov	r8, r3
    4318:	e1a05000 	mov	r5, r0
    431c:	e1a07001 	mov	r7, r1
    4320:	e1a06002 	mov	r6, r2
    4324:	ebfffd5d 	bl	38a0 <ext2_ialloc>
    4328:	e1a04000 	mov	r4, r0
    432c:	e1a00005 	mov	r0, r5
    4330:	ebfffdb3 	bl	3a04 <ext2_balloc>
    4334:	e24b2e42 	sub	r2, fp, #1056	; 0x420
    4338:	e1a09000 	mov	r9, r0
    433c:	e1a01004 	mov	r1, r4
    4340:	e242200c 	sub	r2, r2, #12
    4344:	e1a00005 	mov	r0, r5
    4348:	ebfffd1f 	bl	37cc <get_node_by_ino>
    434c:	e1a02000 	mov	r2, r0
    4350:	e3a03000 	mov	r3, #0
    4354:	e3a0eb01 	mov	lr, #1024	; 0x400
    4358:	e3a00002 	mov	r0, #2
    435c:	e1a01002 	mov	r1, r2
    4360:	e1a0a828 	lsr	sl, r8, #16
    4364:	e1c230b0 	strh	r3, [r2]
    4368:	e1c231ba 	strh	r3, [r2, #26]
    436c:	e5823008 	str	r3, [r2, #8]
    4370:	e582300c 	str	r3, [r2, #12]
    4374:	e5823010 	str	r3, [r2, #16]
    4378:	e1c280b2 	strh	r8, [r2, #2]
    437c:	e1c2a1b8 	strh	sl, [r2, #24]
    4380:	e582e004 	str	lr, [r2, #4]
    4384:	e582001c 	str	r0, [r2, #28]
    4388:	e5a19028 	str	r9, [r1, #40]!	; 0x28
    438c:	e1a0c003 	mov	ip, r3
    4390:	e2823060 	add	r3, r2, #96	; 0x60
    4394:	e5a1c004 	str	ip, [r1, #4]!
    4398:	e1510003 	cmp	r1, r3
    439c:	1afffffc 	bne	4394 <ext2_create_dir+0x94>
    43a0:	e1a00005 	mov	r0, r5
    43a4:	e1a01004 	mov	r1, r4
    43a8:	ebffffa5 	bl	4244 <put_node>
    43ac:	e3a02002 	mov	r2, #2
    43b0:	e1a00005 	mov	r0, r5
    43b4:	e58d2000 	str	r2, [sp]
    43b8:	e1a01007 	mov	r1, r7
    43bc:	e1a03006 	mov	r3, r6
    43c0:	e1a02004 	mov	r2, r4
    43c4:	ebfffdeb 	bl	3b78 <enter_child>
    43c8:	e3500000 	cmp	r0, #0
    43cc:	a1a00004 	movge	r0, r4
    43d0:	b3e00000 	mvnlt	r0, #0
    43d4:	e24bd028 	sub	sp, fp, #40	; 0x28
    43d8:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    43dc:	e12fff1e 	bx	lr

000043e0 <ext2_create_file>:
    43e0:	e1a0c00d 	mov	ip, sp
    43e4:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    43e8:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    43ec:	e24cb004 	sub	fp, ip, #4
    43f0:	e24dd00c 	sub	sp, sp, #12
    43f4:	e1a08003 	mov	r8, r3
    43f8:	e1a05000 	mov	r5, r0
    43fc:	e1a07001 	mov	r7, r1
    4400:	e1a06002 	mov	r6, r2
    4404:	ebfffd25 	bl	38a0 <ext2_ialloc>
    4408:	e1a04000 	mov	r4, r0
    440c:	e24b2e42 	sub	r2, fp, #1056	; 0x420
    4410:	e1a01004 	mov	r1, r4
    4414:	e2422004 	sub	r2, r2, #4
    4418:	e1a00005 	mov	r0, r5
    441c:	ebfffcea 	bl	37cc <get_node_by_ino>
    4420:	e3a01000 	mov	r1, #0
    4424:	e1a02000 	mov	r2, r0
    4428:	e1a03828 	lsr	r3, r8, #16
    442c:	e1c080b2 	strh	r8, [r0, #2]
    4430:	e1a0c001 	mov	ip, r1
    4434:	e1c031b8 	strh	r3, [r0, #24]
    4438:	e1c010b0 	strh	r1, [r0]
    443c:	e5801004 	str	r1, [r0, #4]
    4440:	e1c011ba 	strh	r1, [r0, #26]
    4444:	e5801008 	str	r1, [r0, #8]
    4448:	e580100c 	str	r1, [r0, #12]
    444c:	e5801010 	str	r1, [r0, #16]
    4450:	e580101c 	str	r1, [r0, #28]
    4454:	e2803024 	add	r3, r0, #36	; 0x24
    4458:	e2801060 	add	r1, r0, #96	; 0x60
    445c:	e5a3c004 	str	ip, [r3, #4]!
    4460:	e1530001 	cmp	r3, r1
    4464:	1afffffc 	bne	445c <ext2_create_file+0x7c>
    4468:	e1a00005 	mov	r0, r5
    446c:	e1a01004 	mov	r1, r4
    4470:	ebffff73 	bl	4244 <put_node>
    4474:	e3a02001 	mov	r2, #1
    4478:	e1a00005 	mov	r0, r5
    447c:	e58d2000 	str	r2, [sp]
    4480:	e1a01007 	mov	r1, r7
    4484:	e1a03006 	mov	r3, r6
    4488:	e1a02004 	mov	r2, r4
    448c:	ebfffdb9 	bl	3b78 <enter_child>
    4490:	e3500000 	cmp	r0, #0
    4494:	a1a00004 	movge	r0, r4
    4498:	b3e00000 	mvnlt	r0, #0
    449c:	e24bd020 	sub	sp, fp, #32
    44a0:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    44a4:	e12fff1e 	bx	lr

000044a8 <ext2_write>:
    44a8:	e1a0c00d 	mov	ip, sp
    44ac:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    44b0:	e24cb004 	sub	fp, ip, #4
    44b4:	e24dd00c 	sub	sp, sp, #12
    44b8:	e59b6004 	ldr	r6, [fp, #4]
    44bc:	e3560000 	cmp	r6, #0
    44c0:	e286afff 	add	sl, r6, #1020	; 0x3fc
    44c4:	a1a0a006 	movge	sl, r6
    44c8:	e1a04fc6 	asr	r4, r6, #31
    44cc:	b28aa003 	addlt	sl, sl, #3
    44d0:	e1a0cb24 	lsr	ip, r4, #22
    44d4:	e086400c 	add	r4, r6, ip
    44d8:	e1a0a54a 	asr	sl, sl, #10
    44dc:	e1a04b04 	lsl	r4, r4, #22
    44e0:	e35a000b 	cmp	sl, #11
    44e4:	e1a07001 	mov	r7, r1
    44e8:	e1a08002 	mov	r8, r2
    44ec:	e1a05003 	mov	r5, r3
    44f0:	e06c4b24 	rsb	r4, ip, r4, lsr #22
    44f4:	e50b0030 	str	r0, [fp, #-48]	; 0x30
    44f8:	ca00002f 	bgt	45bc <ext2_write+0x114>
    44fc:	e081a10a 	add	sl, r1, sl, lsl #2
    4500:	e59a3028 	ldr	r3, [sl, #40]	; 0x28
    4504:	e3530000 	cmp	r3, #0
    4508:	0a00004a 	beq	4638 <ext2_write+0x190>
    450c:	e50b3034 	str	r3, [fp, #-52]	; 0x34
    4510:	e59f3178 	ldr	r3, [pc, #376]	; 4690 <ext2_write+0x1e8>
    4514:	e08f3003 	add	r3, pc, r3
    4518:	e1a09003 	mov	r9, r3
    451c:	e3a0a000 	mov	sl, #0
    4520:	e1a01003 	mov	r1, r3
    4524:	e51b0034 	ldr	r0, [fp, #-52]	; 0x34
    4528:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    452c:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    4530:	e1a0e00f 	mov	lr, pc
    4534:	e12fff1c 	bx	ip
    4538:	e0893004 	add	r3, r9, r4
    453c:	e2649b01 	rsb	r9, r4, #1024	; 0x400
    4540:	e1550009 	cmp	r5, r9
    4544:	b1a04005 	movlt	r4, r5
    4548:	a1a04009 	movge	r4, r9
    454c:	e1a02005 	mov	r2, r5
    4550:	e1a00003 	mov	r0, r3
    4554:	e1a01008 	mov	r1, r8
    4558:	eb002666 	bl	def8 <memcpy>
    455c:	e5972004 	ldr	r2, [r7, #4]
    4560:	e0866004 	add	r6, r6, r4
    4564:	e1560002 	cmp	r6, r2
    4568:	e0645005 	rsb	r5, r4, r5
    456c:	e08aa004 	add	sl, sl, r4
    4570:	e0649009 	rsb	r9, r4, r9
    4574:	c0844002 	addgt	r4, r4, r2
    4578:	c5874004 	strgt	r4, [r7, #4]
    457c:	e3550000 	cmp	r5, #0
    4580:	e1a03000 	mov	r3, r0
    4584:	da000001 	ble	4590 <ext2_write+0xe8>
    4588:	e3590000 	cmp	r9, #0
    458c:	caffffeb 	bgt	4540 <ext2_write+0x98>
    4590:	e59f10fc 	ldr	r1, [pc, #252]	; 4694 <ext2_write+0x1ec>
    4594:	e51b0034 	ldr	r0, [fp, #-52]	; 0x34
    4598:	e08f1001 	add	r1, pc, r1
    459c:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    45a0:	e593c40c 	ldr	ip, [r3, #1036]	; 0x40c
    45a4:	e1a0e00f 	mov	lr, pc
    45a8:	e12fff1c 	bx	ip
    45ac:	e1a0000a 	mov	r0, sl
    45b0:	e24bd028 	sub	sp, fp, #40	; 0x28
    45b4:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    45b8:	e12fff1e 	bx	lr
    45bc:	e24a300c 	sub	r3, sl, #12
    45c0:	e35300ff 	cmp	r3, #255	; 0xff
    45c4:	83a0a000 	movhi	sl, #0
    45c8:	8afffff7 	bhi	45ac <ext2_write+0x104>
    45cc:	e5911058 	ldr	r1, [r1, #88]	; 0x58
    45d0:	e3510000 	cmp	r1, #0
    45d4:	0a00001b 	beq	4648 <ext2_write+0x1a0>
    45d8:	e59f90b8 	ldr	r9, [pc, #184]	; 4698 <ext2_write+0x1f0>
    45dc:	e08f9009 	add	r9, pc, r9
    45e0:	e1a00001 	mov	r0, r1
    45e4:	e24aa133 	sub	sl, sl, #-1073741812	; 0xc000000c
    45e8:	e1a01009 	mov	r1, r9
    45ec:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    45f0:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    45f4:	e1a0e00f 	mov	lr, pc
    45f8:	e12fff1c 	bx	ip
    45fc:	e799210a 	ldr	r2, [r9, sl, lsl #2]
    4600:	e3520000 	cmp	r2, #0
    4604:	150b2034 	strne	r2, [fp, #-52]	; 0x34
    4608:	1affffc0 	bne	4510 <ext2_write+0x68>
    460c:	e51b0030 	ldr	r0, [fp, #-48]	; 0x30
    4610:	ebfffcfb 	bl	3a04 <ext2_balloc>
    4614:	e789010a 	str	r0, [r9, sl, lsl #2]
    4618:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    461c:	e1a01009 	mov	r1, r9
    4620:	e5970058 	ldr	r0, [r7, #88]	; 0x58
    4624:	e593c40c 	ldr	ip, [r3, #1036]	; 0x40c
    4628:	e1a0e00f 	mov	lr, pc
    462c:	e12fff1c 	bx	ip
    4630:	e799310a 	ldr	r3, [r9, sl, lsl #2]
    4634:	eaffffb4 	b	450c <ext2_write+0x64>
    4638:	ebfffcf1 	bl	3a04 <ext2_balloc>
    463c:	e1a03000 	mov	r3, r0
    4640:	e58a0028 	str	r0, [sl, #40]	; 0x28
    4644:	eaffffb0 	b	450c <ext2_write+0x64>
    4648:	e50b1034 	str	r1, [fp, #-52]	; 0x34
    464c:	ebfffcec 	bl	3a04 <ext2_balloc>
    4650:	e59f3044 	ldr	r3, [pc, #68]	; 469c <ext2_write+0x1f4>
    4654:	e08f3003 	add	r3, pc, r3
    4658:	e1a09003 	mov	r9, r3
    465c:	e5870058 	str	r0, [r7, #88]	; 0x58
    4660:	e51b1034 	ldr	r1, [fp, #-52]	; 0x34
    4664:	e1a00003 	mov	r0, r3
    4668:	e3a02b01 	mov	r2, #1024	; 0x400
    466c:	eb00265d 	bl	dfe8 <memset>
    4670:	e1a01009 	mov	r1, r9
    4674:	e5970058 	ldr	r0, [r7, #88]	; 0x58
    4678:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    467c:	e593c40c 	ldr	ip, [r3, #1036]	; 0x40c
    4680:	e1a0e00f 	mov	lr, pc
    4684:	e12fff1c 	bx	ip
    4688:	e5971058 	ldr	r1, [r7, #88]	; 0x58
    468c:	eaffffd1 	b	45d8 <ext2_write+0x130>
    4690:	00047e14 	andeq	r7, r4, r4, lsl lr
    4694:	00047d90 	muleq	r4, r0, sp
    4698:	00047d4c 	andeq	r7, r4, ip, asr #26
    469c:	00047cd4 	ldrdeq	r7, [r4], -r4

000046a0 <ext2_ino_by_fname>:
    46a0:	e1a0c00d 	mov	ip, sp
    46a4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    46a8:	e1a04001 	mov	r4, r1
    46ac:	e59f103c 	ldr	r1, [pc, #60]	; 46f0 <ext2_ino_by_fname+0x50>
    46b0:	e24cb004 	sub	fp, ip, #4
    46b4:	e1a05000 	mov	r5, r0
    46b8:	e08f1001 	add	r1, pc, r1
    46bc:	e1a00004 	mov	r0, r4
    46c0:	eb0026dd 	bl	e23c <strcmp>
    46c4:	e3500000 	cmp	r0, #0
    46c8:	1a000003 	bne	46dc <ext2_ino_by_fname+0x3c>
    46cc:	e3a00002 	mov	r0, #2
    46d0:	e24bd014 	sub	sp, fp, #20
    46d4:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    46d8:	e12fff1e 	bx	lr
    46dc:	e1a00005 	mov	r0, r5
    46e0:	e1a01004 	mov	r1, r4
    46e4:	e24bd014 	sub	sp, fp, #20
    46e8:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    46ec:	eafffddd 	b	3e68 <ext2_ino_by_fname.part.8>
    46f0:	0000ada8 	andeq	sl, r0, r8, lsr #27

000046f4 <ext2_unlink>:
    46f4:	e1a0c00d 	mov	ip, sp
    46f8:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    46fc:	e59f5530 	ldr	r5, [pc, #1328]	; 4c34 <ext2_unlink+0x540>
    4700:	e24ddec2 	sub	sp, sp, #3104	; 0xc20
    4704:	e24cb004 	sub	fp, ip, #4
    4708:	e24dd00c 	sub	sp, sp, #12
    470c:	e08f5005 	add	r5, pc, r5
    4710:	e1a04000 	mov	r4, r0
    4714:	e1a00005 	mov	r0, r5
    4718:	e1a07001 	mov	r7, r1
    471c:	eb000d5a 	bl	7c8c <str_new>
    4720:	e1a06000 	mov	r6, r0
    4724:	e1a00005 	mov	r0, r5
    4728:	eb000d57 	bl	7c8c <str_new>
    472c:	e1a01006 	mov	r1, r6
    4730:	e1a02000 	mov	r2, r0
    4734:	e50b0c48 	str	r0, [fp, #-3144]	; 0xc48
    4738:	e1a00007 	mov	r0, r7
    473c:	eb001340 	bl	9444 <vfs_parse_name>
    4740:	e1a00004 	mov	r0, r4
    4744:	e5961000 	ldr	r1, [r6]
    4748:	ebffffd4 	bl	46a0 <ext2_ino_by_fname>
    474c:	e2503000 	subs	r3, r0, #0
    4750:	e50b3c4c 	str	r3, [fp, #-3148]	; 0xc4c
    4754:	ba00012c 	blt	4c0c <ext2_unlink+0x518>
    4758:	e24b3ec2 	sub	r3, fp, #3104	; 0xc20
    475c:	e243300c 	sub	r3, r3, #12
    4760:	e1a02003 	mov	r2, r3
    4764:	e51b1c4c 	ldr	r1, [fp, #-3148]	; 0xc4c
    4768:	e1a00004 	mov	r0, r4
    476c:	e50b3c54 	str	r3, [fp, #-3156]	; 0xc54
    4770:	ebfffc15 	bl	37cc <get_node_by_ino>
    4774:	e1a05000 	mov	r5, r0
    4778:	e50b0c44 	str	r0, [fp, #-3140]	; 0xc44
    477c:	e1a00006 	mov	r0, r6
    4780:	eb000db4 	bl	7e58 <str_free>
    4784:	e3550000 	cmp	r5, #0
    4788:	0a000125 	beq	4c24 <ext2_unlink+0x530>
    478c:	e51b5c44 	ldr	r5, [fp, #-3140]	; 0xc44
    4790:	e51b6c48 	ldr	r6, [fp, #-3144]	; 0xc48
    4794:	e2843b01 	add	r3, r4, #1024	; 0x400
    4798:	e2830008 	add	r0, r3, #8
    479c:	e1a01005 	mov	r1, r5
    47a0:	e5962000 	ldr	r2, [r6]
    47a4:	e50b3c50 	str	r3, [fp, #-3152]	; 0xc50
    47a8:	ebfffd78 	bl	3d90 <search.isra.6>
    47ac:	e50b0c3c 	str	r0, [fp, #-3132]	; 0xc3c
    47b0:	e5950028 	ldr	r0, [r5, #40]	; 0x28
    47b4:	e3500000 	cmp	r0, #0
    47b8:	e1a03005 	mov	r3, r5
    47bc:	e5967000 	ldr	r7, [r6]
    47c0:	0a00004a 	beq	48f0 <ext2_unlink+0x1fc>
    47c4:	e3a05000 	mov	r5, #0
    47c8:	e24bae42 	sub	sl, fp, #1056	; 0x420
    47cc:	e24aa00c 	sub	sl, sl, #12
    47d0:	e50b4c40 	str	r4, [fp, #-3136]	; 0xc40
    47d4:	e1a09005 	mov	r9, r5
    47d8:	e1a0400a 	mov	r4, sl
    47dc:	e2833028 	add	r3, r3, #40	; 0x28
    47e0:	e24b8e82 	sub	r8, fp, #2080	; 0x820
    47e4:	e50b3c38 	str	r3, [fp, #-3128]	; 0xc38
    47e8:	e50b5c30 	str	r5, [fp, #-3120]	; 0xc30
    47ec:	e50b5c34 	str	r5, [fp, #-3124]	; 0xc34
    47f0:	e248800c 	sub	r8, r8, #12
    47f4:	e51b3c40 	ldr	r3, [fp, #-3136]	; 0xc40
    47f8:	e1a01008 	mov	r1, r8
    47fc:	e593c408 	ldr	ip, [r3, #1032]	; 0x408
    4800:	e1a0e00f 	mov	lr, pc
    4804:	e12fff1c 	bx	ip
    4808:	e1a03004 	mov	r3, r4
    480c:	e1a0a008 	mov	sl, r8
    4810:	e1a04009 	mov	r4, r9
    4814:	e3a06000 	mov	r6, #0
    4818:	e1a09005 	mov	r9, r5
    481c:	e1a05003 	mov	r5, r3
    4820:	ea000018 	b	4888 <ext2_unlink+0x194>
    4824:	e59a3000 	ldr	r3, [sl]
    4828:	e3530000 	cmp	r3, #0
    482c:	0a000019 	beq	4898 <ext2_unlink+0x1a4>
    4830:	e28a0008 	add	r0, sl, #8
    4834:	e1a01007 	mov	r1, r7
    4838:	eb00267f 	bl	e23c <strcmp>
    483c:	e3500000 	cmp	r0, #0
    4840:	11a0900a 	movne	r9, sl
    4844:	e1da00b4 	ldrh	r0, [sl, #4]
    4848:	1a00000a 	bne	4878 <ext2_unlink+0x184>
    484c:	e3500b01 	cmp	r0, #1024	; 0x400
    4850:	0a0000b6 	beq	4b30 <ext2_unlink+0x43c>
    4854:	e3590000 	cmp	r9, #0
    4858:	0a0000b0 	beq	4b20 <ext2_unlink+0x42c>
    485c:	e068400a 	rsb	r4, r8, sl
    4860:	e0803004 	add	r3, r0, r4
    4864:	e3530b01 	cmp	r3, #1024	; 0x400
    4868:	0a0000bc 	beq	4b60 <ext2_unlink+0x46c>
    486c:	e50b4c30 	str	r4, [fp, #-3120]	; 0xc30
    4870:	e1a04000 	mov	r4, r0
    4874:	e3a06001 	mov	r6, #1
    4878:	e08a3000 	add	r3, sl, r0
    487c:	e1530005 	cmp	r3, r5
    4880:	2a000009 	bcs	48ac <ext2_unlink+0x1b8>
    4884:	e1a0a003 	mov	sl, r3
    4888:	e3560000 	cmp	r6, #0
    488c:	0affffe4 	beq	4824 <ext2_unlink+0x130>
    4890:	e1da00b4 	ldrh	r0, [sl, #4]
    4894:	eafffff6 	b	4874 <ext2_unlink+0x180>
    4898:	e1da00b4 	ldrh	r0, [sl, #4]
    489c:	e08a3000 	add	r3, sl, r0
    48a0:	e1530005 	cmp	r3, r5
    48a4:	e1a0900a 	mov	r9, sl
    48a8:	3afffff5 	bcc	4884 <ext2_unlink+0x190>
    48ac:	e1a03005 	mov	r3, r5
    48b0:	e3560001 	cmp	r6, #1
    48b4:	e1a05009 	mov	r5, r9
    48b8:	e1a09004 	mov	r9, r4
    48bc:	e1a04003 	mov	r4, r3
    48c0:	0a0000b3 	beq	4b94 <ext2_unlink+0x4a0>
    48c4:	e51b3c34 	ldr	r3, [fp, #-3124]	; 0xc34
    48c8:	e2833001 	add	r3, r3, #1
    48cc:	e353000c 	cmp	r3, #12
    48d0:	e50b3c34 	str	r3, [fp, #-3124]	; 0xc34
    48d4:	0a000004 	beq	48ec <ext2_unlink+0x1f8>
    48d8:	e51b3c38 	ldr	r3, [fp, #-3128]	; 0xc38
    48dc:	e5b30004 	ldr	r0, [r3, #4]!
    48e0:	e3500000 	cmp	r0, #0
    48e4:	e50b3c38 	str	r3, [fp, #-3128]	; 0xc38
    48e8:	1affffc1 	bne	47f4 <ext2_unlink+0x100>
    48ec:	e51b4c40 	ldr	r4, [fp, #-3136]	; 0xc40
    48f0:	e51b0c48 	ldr	r0, [fp, #-3144]	; 0xc48
    48f4:	eb000d57 	bl	7e58 <str_free>
    48f8:	e1a00004 	mov	r0, r4
    48fc:	e51b1c4c 	ldr	r1, [fp, #-3148]	; 0xc4c
    4900:	e51b2c44 	ldr	r2, [fp, #-3140]	; 0xc44
    4904:	ebfffe4e 	bl	4244 <put_node>
    4908:	e51b3c3c 	ldr	r3, [fp, #-3132]	; 0xc3c
    490c:	e3530000 	cmp	r3, #0
    4910:	ba0000bb 	blt	4c04 <ext2_unlink+0x510>
    4914:	e51b2c54 	ldr	r2, [fp, #-3156]	; 0xc54
    4918:	e1a00004 	mov	r0, r4
    491c:	e51b1c3c 	ldr	r1, [fp, #-3132]	; 0xc3c
    4920:	ebfffba9 	bl	37cc <get_node_by_ino>
    4924:	e2503000 	subs	r3, r0, #0
    4928:	e50b3c38 	str	r3, [fp, #-3128]	; 0xc38
    492c:	0a0000b4 	beq	4c04 <ext2_unlink+0x510>
    4930:	e3a08001 	mov	r8, #1
    4934:	e2836024 	add	r6, r3, #36	; 0x24
    4938:	e2837054 	add	r7, r3, #84	; 0x54
    493c:	e51b3c50 	ldr	r3, [fp, #-3152]	; 0xc50
    4940:	e2832004 	add	r2, r3, #4
    4944:	e283300c 	add	r3, r3, #12
    4948:	e24b5e42 	sub	r5, fp, #1056	; 0x420
    494c:	e50b2c30 	str	r2, [fp, #-3120]	; 0xc30
    4950:	e50b3c34 	str	r3, [fp, #-3124]	; 0xc34
    4954:	e245500c 	sub	r5, r5, #12
    4958:	e5b69004 	ldr	r9, [r6, #4]!
    495c:	e3590000 	cmp	r9, #0
    4960:	0a00002c 	beq	4a18 <ext2_unlink+0x324>
    4964:	da000029 	ble	4a10 <ext2_unlink+0x31c>
    4968:	e5941024 	ldr	r1, [r4, #36]	; 0x24
    496c:	e1a00009 	mov	r0, r9
    4970:	eb001dd5 	bl	c0cc <div_u32>
    4974:	e5942404 	ldr	r2, [r4, #1028]	; 0x404
    4978:	e1a0a000 	mov	sl, r0
    497c:	e1a01005 	mov	r1, r5
    4980:	e7920280 	ldr	r0, [r2, r0, lsl #5]
    4984:	e594c408 	ldr	ip, [r4, #1032]	; 0x408
    4988:	e1a0e00f 	mov	lr, pc
    498c:	e12fff1c 	bx	ip
    4990:	e2492001 	sub	r2, r9, #1
    4994:	e3500000 	cmp	r0, #0
    4998:	e202e007 	and	lr, r2, #7
    499c:	1a00001b 	bne	4a10 <ext2_unlink+0x31c>
    49a0:	e7d511c2 	ldrb	r1, [r5, r2, asr #3]
    49a4:	e1c11e18 	bic	r1, r1, r8, lsl lr
    49a8:	e7c511c2 	strb	r1, [r5, r2, asr #3]
    49ac:	e5942404 	ldr	r2, [r4, #1028]	; 0x404
    49b0:	e1a01005 	mov	r1, r5
    49b4:	e792028a 	ldr	r0, [r2, sl, lsl #5]
    49b8:	e594c40c 	ldr	ip, [r4, #1036]	; 0x40c
    49bc:	e1a0e00f 	mov	lr, pc
    49c0:	e12fff1c 	bx	ip
    49c4:	e3500000 	cmp	r0, #0
    49c8:	1a000010 	bne	4a10 <ext2_unlink+0x31c>
    49cc:	e5941024 	ldr	r1, [r4, #36]	; 0x24
    49d0:	e1a00009 	mov	r0, r9
    49d4:	eb001dbc 	bl	c0cc <div_u32>
    49d8:	e5943404 	ldr	r3, [r4, #1028]	; 0x404
    49dc:	e0833280 	add	r3, r3, r0, lsl #5
    49e0:	e1d310bc 	ldrh	r1, [r3, #12]
    49e4:	e2811001 	add	r1, r1, #1
    49e8:	e1c310bc 	strh	r1, [r3, #12]
    49ec:	e1a02000 	mov	r2, r0
    49f0:	e51b1c34 	ldr	r1, [fp, #-3124]	; 0xc34
    49f4:	e51b0c30 	ldr	r0, [fp, #-3120]	; 0xc30
    49f8:	ebfffb93 	bl	384c <set_gd.isra.3>
    49fc:	e5943010 	ldr	r3, [r4, #16]
    4a00:	e2833001 	add	r3, r3, #1
    4a04:	e5843010 	str	r3, [r4, #16]
    4a08:	e1a00004 	mov	r0, r4
    4a0c:	ebfffb53 	bl	3760 <set_super>
    4a10:	e1560007 	cmp	r6, r7
    4a14:	1affffcf 	bne	4958 <ext2_unlink+0x264>
    4a18:	e5943004 	ldr	r3, [r4, #4]
    4a1c:	e51b2c3c 	ldr	r2, [fp, #-3132]	; 0xc3c
    4a20:	e1520003 	cmp	r2, r3
    4a24:	ca000035 	bgt	4b00 <ext2_unlink+0x40c>
    4a28:	e51b7c3c 	ldr	r7, [fp, #-3132]	; 0xc3c
    4a2c:	e594102c 	ldr	r1, [r4, #44]	; 0x2c
    4a30:	e1a00007 	mov	r0, r7
    4a34:	eb001da4 	bl	c0cc <div_u32>
    4a38:	e5943404 	ldr	r3, [r4, #1028]	; 0x404
    4a3c:	e1a06280 	lsl	r6, r0, #5
    4a40:	e24b5e42 	sub	r5, fp, #1056	; 0x420
    4a44:	e0833006 	add	r3, r3, r6
    4a48:	e245500c 	sub	r5, r5, #12
    4a4c:	e5930004 	ldr	r0, [r3, #4]
    4a50:	e1a01005 	mov	r1, r5
    4a54:	e594c408 	ldr	ip, [r4, #1032]	; 0x408
    4a58:	e1a0e00f 	mov	lr, pc
    4a5c:	e12fff1c 	bx	ip
    4a60:	e3500000 	cmp	r0, #0
    4a64:	1a000025 	bne	4b00 <ext2_unlink+0x40c>
    4a68:	e3a00001 	mov	r0, #1
    4a6c:	e2573001 	subs	r3, r7, #1
    4a70:	e1a02fc3 	asr	r2, r3, #31
    4a74:	e1a01ea2 	lsr	r1, r2, #29
    4a78:	e0832001 	add	r2, r3, r1
    4a7c:	42873006 	addmi	r3, r7, #6
    4a80:	e7d5c1c3 	ldrb	ip, [r5, r3, asr #3]
    4a84:	e2022007 	and	r2, r2, #7
    4a88:	e0612002 	rsb	r2, r1, r2
    4a8c:	e1cc2210 	bic	r2, ip, r0, lsl r2
    4a90:	e5941404 	ldr	r1, [r4, #1028]	; 0x404
    4a94:	e7c521c3 	strb	r2, [r5, r3, asr #3]
    4a98:	e0816006 	add	r6, r1, r6
    4a9c:	e5960004 	ldr	r0, [r6, #4]
    4aa0:	e1a01005 	mov	r1, r5
    4aa4:	e594c40c 	ldr	ip, [r4, #1036]	; 0x40c
    4aa8:	e1a0e00f 	mov	lr, pc
    4aac:	e12fff1c 	bx	ip
    4ab0:	e3500000 	cmp	r0, #0
    4ab4:	1a000011 	bne	4b00 <ext2_unlink+0x40c>
    4ab8:	e594102c 	ldr	r1, [r4, #44]	; 0x2c
    4abc:	e1a00007 	mov	r0, r7
    4ac0:	eb001d81 	bl	c0cc <div_u32>
    4ac4:	e5943404 	ldr	r3, [r4, #1028]	; 0x404
    4ac8:	e0833280 	add	r3, r3, r0, lsl #5
    4acc:	e1d310be 	ldrh	r1, [r3, #14]
    4ad0:	e2811001 	add	r1, r1, #1
    4ad4:	e1c310be 	strh	r1, [r3, #14]
    4ad8:	e51b3c50 	ldr	r3, [fp, #-3152]	; 0xc50
    4adc:	e1a02000 	mov	r2, r0
    4ae0:	e283100c 	add	r1, r3, #12
    4ae4:	e2830004 	add	r0, r3, #4
    4ae8:	ebfffb57 	bl	384c <set_gd.isra.3>
    4aec:	e5943014 	ldr	r3, [r4, #20]
    4af0:	e2833001 	add	r3, r3, #1
    4af4:	e5843014 	str	r3, [r4, #20]
    4af8:	e1a00004 	mov	r0, r4
    4afc:	ebfffb17 	bl	3760 <set_super>
    4b00:	e1a00004 	mov	r0, r4
    4b04:	e51b1c3c 	ldr	r1, [fp, #-3132]	; 0xc3c
    4b08:	e51b2c38 	ldr	r2, [fp, #-3128]	; 0xc38
    4b0c:	ebfffdcc 	bl	4244 <put_node>
    4b10:	e3a00000 	mov	r0, #0
    4b14:	e24bd028 	sub	sp, fp, #40	; 0x28
    4b18:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    4b1c:	e12fff1e 	bx	lr
    4b20:	e068300a 	rsb	r3, r8, sl
    4b24:	e50b3c30 	str	r3, [fp, #-3120]	; 0xc30
    4b28:	e1a04000 	mov	r4, r0
    4b2c:	eaffff50 	b	4874 <ext2_unlink+0x180>
    4b30:	e1a0900a 	mov	r9, sl
    4b34:	e51b3c34 	ldr	r3, [fp, #-3124]	; 0xc34
    4b38:	e283a00a 	add	sl, r3, #10
    4b3c:	e51b3c44 	ldr	r3, [fp, #-3140]	; 0xc44
    4b40:	e51b4c40 	ldr	r4, [fp, #-3136]	; 0xc40
    4b44:	e793010a 	ldr	r0, [r3, sl, lsl #2]
    4b48:	e1a01008 	mov	r1, r8
    4b4c:	e5896000 	str	r6, [r9]
    4b50:	e594c40c 	ldr	ip, [r4, #1036]	; 0x40c
    4b54:	e1a0e00f 	mov	lr, pc
    4b58:	e12fff1c 	bx	ip
    4b5c:	eaffff63 	b	48f0 <ext2_unlink+0x1fc>
    4b60:	e51b2c34 	ldr	r2, [fp, #-3124]	; 0xc34
    4b64:	e0693008 	rsb	r3, r9, r8
    4b68:	e282a00a 	add	sl, r2, #10
    4b6c:	e51b2c44 	ldr	r2, [fp, #-3140]	; 0xc44
    4b70:	e2833b01 	add	r3, r3, #1024	; 0x400
    4b74:	e792010a 	ldr	r0, [r2, sl, lsl #2]
    4b78:	e51b4c40 	ldr	r4, [fp, #-3136]	; 0xc40
    4b7c:	e1c930b4 	strh	r3, [r9, #4]
    4b80:	e1a01008 	mov	r1, r8
    4b84:	e594c40c 	ldr	ip, [r4, #1036]	; 0x40c
    4b88:	e1a0e00f 	mov	lr, pc
    4b8c:	e12fff1c 	bx	ip
    4b90:	eaffff56 	b	48f0 <ext2_unlink+0x1fc>
    4b94:	e1a07009 	mov	r7, r9
    4b98:	e1a0900a 	mov	r9, sl
    4b9c:	e1a0a003 	mov	sl, r3
    4ba0:	e0805007 	add	r5, r0, r7
    4ba4:	e51b6c30 	ldr	r6, [fp, #-3120]	; 0xc30
    4ba8:	e1a05805 	lsl	r5, r5, #16
    4bac:	e1a05825 	lsr	r5, r5, #16
    4bb0:	e1a01008 	mov	r1, r8
    4bb4:	e1a02006 	mov	r2, r6
    4bb8:	e1c950b4 	strh	r5, [r9, #4]
    4bbc:	e1a0000a 	mov	r0, sl
    4bc0:	e51b4c40 	ldr	r4, [fp, #-3136]	; 0xc40
    4bc4:	eb0024cb 	bl	def8 <memcpy>
    4bc8:	e0851006 	add	r1, r5, r6
    4bcc:	e0872006 	add	r2, r7, r6
    4bd0:	e0881001 	add	r1, r8, r1
    4bd4:	e2622b01 	rsb	r2, r2, #1024	; 0x400
    4bd8:	e08a0006 	add	r0, sl, r6
    4bdc:	eb0024c5 	bl	def8 <memcpy>
    4be0:	e51b3c34 	ldr	r3, [fp, #-3124]	; 0xc34
    4be4:	e51b2c44 	ldr	r2, [fp, #-3140]	; 0xc44
    4be8:	e283300a 	add	r3, r3, #10
    4bec:	e7920103 	ldr	r0, [r2, r3, lsl #2]
    4bf0:	e1a0100a 	mov	r1, sl
    4bf4:	e594c40c 	ldr	ip, [r4, #1036]	; 0x40c
    4bf8:	e1a0e00f 	mov	lr, pc
    4bfc:	e12fff1c 	bx	ip
    4c00:	eaffff3a 	b	48f0 <ext2_unlink+0x1fc>
    4c04:	e3e00000 	mvn	r0, #0
    4c08:	eaffffc1 	b	4b14 <ext2_unlink+0x420>
    4c0c:	e1a00006 	mov	r0, r6
    4c10:	eb000c90 	bl	7e58 <str_free>
    4c14:	e51b0c48 	ldr	r0, [fp, #-3144]	; 0xc48
    4c18:	eb000c8e 	bl	7e58 <str_free>
    4c1c:	e3e00000 	mvn	r0, #0
    4c20:	eaffffbb 	b	4b14 <ext2_unlink+0x420>
    4c24:	e51b0c48 	ldr	r0, [fp, #-3144]	; 0xc48
    4c28:	eb000c8a 	bl	7e58 <str_free>
    4c2c:	e3e00000 	mvn	r0, #0
    4c30:	eaffffb7 	b	4b14 <ext2_unlink+0x420>
    4c34:	0000ade0 	andeq	sl, r0, r0, ror #27

00004c38 <ext2_node_by_ino>:
    4c38:	e1a0c00d 	mov	ip, sp
    4c3c:	e92dd810 	push	{r4, fp, ip, lr, pc}
    4c40:	e24cb004 	sub	fp, ip, #4
    4c44:	e24ddb01 	sub	sp, sp, #1024	; 0x400
    4c48:	e24dd004 	sub	sp, sp, #4
    4c4c:	e1a04002 	mov	r4, r2
    4c50:	e24b2e41 	sub	r2, fp, #1040	; 0x410
    4c54:	e2422004 	sub	r2, r2, #4
    4c58:	ebfffadb 	bl	37cc <get_node_by_ino>
    4c5c:	e2501000 	subs	r1, r0, #0
    4c60:	0a000006 	beq	4c80 <ext2_node_by_ino+0x48>
    4c64:	e1a00004 	mov	r0, r4
    4c68:	e3a02080 	mov	r2, #128	; 0x80
    4c6c:	eb0024a1 	bl	def8 <memcpy>
    4c70:	e3a00000 	mov	r0, #0
    4c74:	e24bd010 	sub	sp, fp, #16
    4c78:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
    4c7c:	e12fff1e 	bx	lr
    4c80:	e3e00000 	mvn	r0, #0
    4c84:	eafffffa 	b	4c74 <ext2_node_by_ino+0x3c>

00004c88 <ext2_node_by_fname>:
    4c88:	e1a0c00d 	mov	ip, sp
    4c8c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    4c90:	e24cb004 	sub	fp, ip, #4
    4c94:	e1a04002 	mov	r4, r2
    4c98:	e1a05000 	mov	r5, r0
    4c9c:	ebfffe7f 	bl	46a0 <ext2_ino_by_fname>
    4ca0:	e2501000 	subs	r1, r0, #0
    4ca4:	da000004 	ble	4cbc <ext2_node_by_fname+0x34>
    4ca8:	e1a00005 	mov	r0, r5
    4cac:	e1a02004 	mov	r2, r4
    4cb0:	e24bd014 	sub	sp, fp, #20
    4cb4:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    4cb8:	eaffffde 	b	4c38 <ext2_node_by_ino>
    4cbc:	e3e00000 	mvn	r0, #0
    4cc0:	e24bd014 	sub	sp, fp, #20
    4cc4:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    4cc8:	e12fff1e 	bx	lr

00004ccc <ext2_rmdir>:
    4ccc:	eafffe88 	b	46f4 <ext2_unlink>

00004cd0 <ext2_init>:
    4cd0:	e1a0c00d 	mov	ip, sp
    4cd4:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    4cd8:	e1a0a000 	mov	sl, r0
    4cdc:	e24cb004 	sub	fp, ip, #4
    4ce0:	e24ddb02 	sub	sp, sp, #2048	; 0x800
    4ce4:	e24b4e82 	sub	r4, fp, #2080	; 0x820
    4ce8:	e24dd00c 	sub	sp, sp, #12
    4cec:	e244400c 	sub	r4, r4, #12
    4cf0:	e1a03001 	mov	r3, r1
    4cf4:	e580240c 	str	r2, [r0, #1036]	; 0x40c
    4cf8:	e58a1408 	str	r1, [sl, #1032]	; 0x408
    4cfc:	e3a00001 	mov	r0, #1
    4d00:	e1a01004 	mov	r1, r4
    4d04:	e1a0e00f 	mov	lr, pc
    4d08:	e12fff13 	bx	r3
    4d0c:	e2503000 	subs	r3, r0, #0
    4d10:	e50b3834 	str	r3, [fp, #-2100]	; 0x834
    4d14:	1a000039 	bne	4e00 <ext2_init+0x130>
    4d18:	e3a02b01 	mov	r2, #1024	; 0x400
    4d1c:	e1a01004 	mov	r1, r4
    4d20:	e28a0004 	add	r0, sl, #4
    4d24:	eb002473 	bl	def8 <memcpy>
    4d28:	e59a1024 	ldr	r1, [sl, #36]	; 0x24
    4d2c:	e59a0008 	ldr	r0, [sl, #8]
    4d30:	eb001ce5 	bl	c0cc <div_u32>
    4d34:	e59a1024 	ldr	r1, [sl, #36]	; 0x24
    4d38:	e1a04000 	mov	r4, r0
    4d3c:	e59a0008 	ldr	r0, [sl, #8]
    4d40:	eb001d6f 	bl	c304 <mod_u32>
    4d44:	e3500000 	cmp	r0, #0
    4d48:	12844001 	addne	r4, r4, #1
    4d4c:	e58a4000 	str	r4, [sl]
    4d50:	e1a00284 	lsl	r0, r4, #5
    4d54:	eb00215c 	bl	d2cc <malloc>
    4d58:	e3a01020 	mov	r1, #32
    4d5c:	e58a0404 	str	r0, [sl, #1028]	; 0x404
    4d60:	e3a00b01 	mov	r0, #1024	; 0x400
    4d64:	eb001cd8 	bl	c0cc <div_u32>
    4d68:	e3a04000 	mov	r4, #0
    4d6c:	e3a07002 	mov	r7, #2
    4d70:	e24b8e42 	sub	r8, fp, #1056	; 0x420
    4d74:	e50b0830 	str	r0, [fp, #-2096]	; 0x830
    4d78:	e248800c 	sub	r8, r8, #12
    4d7c:	e1a00007 	mov	r0, r7
    4d80:	e1a01008 	mov	r1, r8
    4d84:	e59ac408 	ldr	ip, [sl, #1032]	; 0x408
    4d88:	e1a0e00f 	mov	lr, pc
    4d8c:	e12fff1c 	bx	ip
    4d90:	e51b3830 	ldr	r3, [fp, #-2096]	; 0x830
    4d94:	e3530000 	cmp	r3, #0
    4d98:	da000014 	ble	4df0 <ext2_init+0x120>
    4d9c:	e1a09008 	mov	r9, r8
    4da0:	e1a05284 	lsl	r5, r4, #5
    4da4:	e0836004 	add	r6, r3, r4
    4da8:	ea000003 	b	4dbc <ext2_init+0xec>
    4dac:	e1540006 	cmp	r4, r6
    4db0:	e2855020 	add	r5, r5, #32
    4db4:	e2899020 	add	r9, r9, #32
    4db8:	0a00000d 	beq	4df4 <ext2_init+0x124>
    4dbc:	e59a0404 	ldr	r0, [sl, #1028]	; 0x404
    4dc0:	e3a02020 	mov	r2, #32
    4dc4:	e1a01009 	mov	r1, r9
    4dc8:	e0800005 	add	r0, r0, r5
    4dcc:	eb002449 	bl	def8 <memcpy>
    4dd0:	e59a2000 	ldr	r2, [sl]
    4dd4:	e2844001 	add	r4, r4, #1
    4dd8:	e1540002 	cmp	r4, r2
    4ddc:	bafffff2 	blt	4dac <ext2_init+0xdc>
    4de0:	e51b0834 	ldr	r0, [fp, #-2100]	; 0x834
    4de4:	e24bd028 	sub	sp, fp, #40	; 0x28
    4de8:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    4dec:	e12fff1e 	bx	lr
    4df0:	e1a06004 	mov	r6, r4
    4df4:	e2877001 	add	r7, r7, #1
    4df8:	e1a04006 	mov	r4, r6
    4dfc:	eaffffde 	b	4d7c <ext2_init+0xac>
    4e00:	e3e03000 	mvn	r3, #0
    4e04:	e50b3834 	str	r3, [fp, #-2100]	; 0x834
    4e08:	eafffff4 	b	4de0 <ext2_init+0x110>

00004e0c <ext2_quit>:
    4e0c:	e1a0c00d 	mov	ip, sp
    4e10:	e5900404 	ldr	r0, [r0, #1028]	; 0x404
    4e14:	e92dd800 	push	{fp, ip, lr, pc}
    4e18:	e24cb004 	sub	fp, ip, #4
    4e1c:	eb002121 	bl	d2a8 <free>
    4e20:	e24bd00c 	sub	sp, fp, #12
    4e24:	e89d6800 	ldm	sp, {fp, sp, lr}
    4e28:	e12fff1e 	bx	lr

00004e2c <ext2_readfile>:
    4e2c:	e1a0c00d 	mov	ip, sp
    4e30:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    4e34:	e2528000 	subs	r8, r2, #0
    4e38:	13e03000 	mvnne	r3, #0
    4e3c:	e24cb004 	sub	fp, ip, #4
    4e40:	e24dd088 	sub	sp, sp, #136	; 0x88
    4e44:	15883000 	strne	r3, [r8]
    4e48:	e1a06000 	mov	r6, r0
    4e4c:	ebfffe13 	bl	46a0 <ext2_ino_by_fname>
    4e50:	e2501000 	subs	r1, r0, #0
    4e54:	ba00001e 	blt	4ed4 <ext2_readfile+0xa8>
    4e58:	e24b70a4 	sub	r7, fp, #164	; 0xa4
    4e5c:	e1a02007 	mov	r2, r7
    4e60:	e1a00006 	mov	r0, r6
    4e64:	ebffff73 	bl	4c38 <ext2_node_by_ino>
    4e68:	e2504000 	subs	r4, r0, #0
    4e6c:	1a000018 	bne	4ed4 <ext2_readfile+0xa8>
    4e70:	e51b00a0 	ldr	r0, [fp, #-160]	; 0xa0
    4e74:	eb002114 	bl	d2cc <malloc>
    4e78:	e2509000 	subs	r9, r0, #0
    4e7c:	0a000015 	beq	4ed8 <ext2_readfile+0xac>
    4e80:	e1a05009 	mov	r5, r9
    4e84:	ea000004 	b	4e9c <ext2_readfile+0x70>
    4e88:	e51b30a0 	ldr	r3, [fp, #-160]	; 0xa0
    4e8c:	e1540003 	cmp	r4, r3
    4e90:	2a000009 	bcs	4ebc <ext2_readfile+0x90>
    4e94:	e0844000 	add	r4, r4, r0
    4e98:	e0855000 	add	r5, r5, r0
    4e9c:	e58d4000 	str	r4, [sp]
    4ea0:	e1a00006 	mov	r0, r6
    4ea4:	e1a01007 	mov	r1, r7
    4ea8:	e1a02005 	mov	r2, r5
    4eac:	e3a03b01 	mov	r3, #1024	; 0x400
    4eb0:	ebfffcc2 	bl	41c0 <ext2_read>
    4eb4:	e3500000 	cmp	r0, #0
    4eb8:	cafffff2 	bgt	4e88 <ext2_readfile+0x5c>
    4ebc:	e1a00009 	mov	r0, r9
    4ec0:	e3580000 	cmp	r8, #0
    4ec4:	15884000 	strne	r4, [r8]
    4ec8:	e24bd024 	sub	sp, fp, #36	; 0x24
    4ecc:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    4ed0:	e12fff1e 	bx	lr
    4ed4:	e3a09000 	mov	r9, #0
    4ed8:	e1a00009 	mov	r0, r9
    4edc:	e24bd024 	sub	sp, fp, #36	; 0x24
    4ee0:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    4ee4:	e12fff1e 	bx	lr

00004ee8 <sd_clk>:
    4ee8:	e1a0c00d 	mov	ip, sp
    4eec:	e1a01000 	mov	r1, r0
    4ef0:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    4ef4:	e59f01ac 	ldr	r0, [pc, #428]	; 50a8 <sd_clk+0x1c0>
    4ef8:	e24cb004 	sub	fp, ip, #4
    4efc:	eb001c72 	bl	c0cc <div_u32>
    4f00:	e59f71a4 	ldr	r7, [pc, #420]	; 50ac <sd_clk+0x1c4>
    4f04:	e59f31a4 	ldr	r3, [pc, #420]	; 50b0 <sd_clk+0x1c8>
    4f08:	e08f7007 	add	r7, pc, r7
    4f0c:	e7975003 	ldr	r5, [r7, r3]
    4f10:	e5953000 	ldr	r3, [r5]
    4f14:	e2833603 	add	r3, r3, #3145728	; 0x300000
    4f18:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    4f1c:	e3120003 	tst	r2, #3
    4f20:	e1a06000 	mov	r6, r0
    4f24:	0a00000c 	beq	4f5c <sd_clk+0x74>
    4f28:	e59f4184 	ldr	r4, [pc, #388]	; 50b4 <sd_clk+0x1cc>
    4f2c:	ea000001 	b	4f38 <sd_clk+0x50>
    4f30:	e2544001 	subs	r4, r4, #1
    4f34:	3a000057 	bcc	5098 <sd_clk+0x1b0>
    4f38:	e3a00ffa 	mov	r0, #1000	; 0x3e8
    4f3c:	eb001ffb 	bl	cf30 <usleep>
    4f40:	e5953000 	ldr	r3, [r5]
    4f44:	e2833603 	add	r3, r3, #3145728	; 0x300000
    4f48:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    4f4c:	e3120003 	tst	r2, #3
    4f50:	1afffff6 	bne	4f30 <sd_clk+0x48>
    4f54:	e3540000 	cmp	r4, #0
    4f58:	0a00004e 	beq	5098 <sd_clk+0x1b0>
    4f5c:	e593202c 	ldr	r2, [r3, #44]	; 0x2c
    4f60:	e3c22004 	bic	r2, r2, #4
    4f64:	e583202c 	str	r2, [r3, #44]	; 0x2c
    4f68:	e59f0148 	ldr	r0, [pc, #328]	; 50b8 <sd_clk+0x1d0>
    4f6c:	eb001fef 	bl	cf30 <usleep>
    4f70:	e2563001 	subs	r3, r6, #1
    4f74:	1a000034 	bne	504c <sd_clk+0x164>
    4f78:	e59f213c 	ldr	r2, [pc, #316]	; 50bc <sd_clk+0x1d4>
    4f7c:	e7972002 	ldr	r2, [r7, r2]
    4f80:	e5921000 	ldr	r1, [r2]
    4f84:	e3510001 	cmp	r1, #1
    4f88:	93a06001 	movls	r6, #1
    4f8c:	91a06316 	lslls	r6, r6, r3
    4f90:	e3560002 	cmp	r6, #2
    4f94:	e5923000 	ldr	r3, [r2]
    4f98:	33a06002 	movcc	r6, #2
    4f9c:	e3530001 	cmp	r3, #1
    4fa0:	93a01000 	movls	r1, #0
    4fa4:	e5950000 	ldr	r0, [r5]
    4fa8:	e2800603 	add	r0, r0, #3145728	; 0x300000
    4fac:	e590202c 	ldr	r2, [r0, #44]	; 0x2c
    4fb0:	e3c22cff 	bic	r2, r2, #65280	; 0xff00
    4fb4:	82063c03 	andhi	r3, r6, #768	; 0x300
    4fb8:	e3c220c0 	bic	r2, r2, #192	; 0xc0
    4fbc:	e1a06c06 	lsl	r6, r6, #24
    4fc0:	81a01123 	lsrhi	r1, r3, #2
    4fc4:	e1823826 	orr	r3, r2, r6, lsr #16
    4fc8:	e1833001 	orr	r3, r3, r1
    4fcc:	e580302c 	str	r3, [r0, #44]	; 0x2c
    4fd0:	e59f00e0 	ldr	r0, [pc, #224]	; 50b8 <sd_clk+0x1d0>
    4fd4:	eb001fd5 	bl	cf30 <usleep>
    4fd8:	e5953000 	ldr	r3, [r5]
    4fdc:	e2833603 	add	r3, r3, #3145728	; 0x300000
    4fe0:	e593202c 	ldr	r2, [r3, #44]	; 0x2c
    4fe4:	e3822004 	orr	r2, r2, #4
    4fe8:	e583202c 	str	r2, [r3, #44]	; 0x2c
    4fec:	e59f00c4 	ldr	r0, [pc, #196]	; 50b8 <sd_clk+0x1d0>
    4ff0:	eb001fce 	bl	cf30 <usleep>
    4ff4:	e5953000 	ldr	r3, [r5]
    4ff8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    4ffc:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
    5000:	e3130002 	tst	r3, #2
    5004:	1a00000c 	bne	503c <sd_clk+0x154>
    5008:	e59f40b0 	ldr	r4, [pc, #176]	; 50c0 <sd_clk+0x1d8>
    500c:	ea000001 	b	5018 <sd_clk+0x130>
    5010:	e2544001 	subs	r4, r4, #1
    5014:	3a00001f 	bcc	5098 <sd_clk+0x1b0>
    5018:	e59f0098 	ldr	r0, [pc, #152]	; 50b8 <sd_clk+0x1d0>
    501c:	eb001fc3 	bl	cf30 <usleep>
    5020:	e5953000 	ldr	r3, [r5]
    5024:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5028:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
    502c:	e3130002 	tst	r3, #2
    5030:	0afffff6 	beq	5010 <sd_clk+0x128>
    5034:	e3540000 	cmp	r4, #0
    5038:	0a000016 	beq	5098 <sd_clk+0x1b0>
    503c:	e3a00000 	mov	r0, #0
    5040:	e24bd01c 	sub	sp, fp, #28
    5044:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    5048:	e12fff1e 	bx	lr
    504c:	e1b02823 	lsrs	r2, r3, #16
    5050:	03a02010 	moveq	r2, #16
    5054:	13a02020 	movne	r2, #32
    5058:	01a03803 	lsleq	r3, r3, #16
    505c:	e31304ff 	tst	r3, #-16777216	; 0xff000000
    5060:	01a03403 	lsleq	r3, r3, #8
    5064:	02422008 	subeq	r2, r2, #8
    5068:	e313020f 	tst	r3, #-268435456	; 0xf0000000
    506c:	01a03203 	lsleq	r3, r3, #4
    5070:	02422004 	subeq	r2, r2, #4
    5074:	e3130103 	tst	r3, #-1073741824	; 0xc0000000
    5078:	01a03103 	lsleq	r3, r3, #2
    507c:	02422002 	subeq	r2, r2, #2
    5080:	e3530000 	cmp	r3, #0
    5084:	a2422001 	subge	r2, r2, #1
    5088:	e2423001 	sub	r3, r2, #1
    508c:	e3530007 	cmp	r3, #7
    5090:	23a03007 	movcs	r3, #7
    5094:	eaffffb7 	b	4f78 <sd_clk+0x90>
    5098:	e3e00001 	mvn	r0, #1
    509c:	e24bd01c 	sub	sp, fp, #28
    50a0:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    50a4:	e12fff1e 	bx	lr
    50a8:	027bc86a 	rsbseq	ip, fp, #6946816	; 0x6a0000
    50ac:	00012af0 	strdeq	r2, [r1], -r0
    50b0:	00000024 	andeq	r0, r0, r4, lsr #32
    50b4:	0001869f 	muleq	r1, pc, r6	; <UNPREDICTABLE>
    50b8:	00002710 	andeq	r2, r0, r0, lsl r7
    50bc:	00000050 	andeq	r0, r0, r0, asr r0
    50c0:	0000270f 	andeq	r2, r0, pc, lsl #14

000050c4 <sd_cmd>:
    50c4:	e1a0c00d 	mov	ip, sp
    50c8:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    50cc:	e24cb004 	sub	fp, ip, #4
    50d0:	e24dd00c 	sub	sp, sp, #12
    50d4:	e3a03000 	mov	r3, #0
    50d8:	e59fc498 	ldr	ip, [pc, #1176]	; 5578 <sd_cmd+0x4b4>
    50dc:	e59f2498 	ldr	r2, [pc, #1176]	; 557c <sd_cmd+0x4b8>
    50e0:	e08fc00c 	add	ip, pc, ip
    50e4:	e79c7002 	ldr	r7, [ip, r2]
    50e8:	e2504000 	subs	r4, r0, #0
    50ec:	e1a08001 	mov	r8, r1
    50f0:	e5873000 	str	r3, [r7]
    50f4:	ba000070 	blt	52bc <sd_cmd+0x1f8>
    50f8:	e59f3480 	ldr	r3, [pc, #1152]	; 5580 <sd_cmd+0x4bc>
    50fc:	e79c9003 	ldr	r9, [ip, r3]
    5100:	e5993000 	ldr	r3, [r9]
    5104:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5108:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    510c:	e3120001 	tst	r2, #1
    5110:	0a000017 	beq	5174 <sd_cmd+0xb0>
    5114:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5118:	e59f2464 	ldr	r2, [pc, #1124]	; 5584 <sd_cmd+0x4c0>
    511c:	e0022001 	and	r2, r2, r1
    5120:	e3520000 	cmp	r2, #0
    5124:	e2833030 	add	r3, r3, #48	; 0x30
    5128:	1a000012 	bne	5178 <sd_cmd+0xb4>
    512c:	e59f5454 	ldr	r5, [pc, #1108]	; 5588 <sd_cmd+0x4c4>
    5130:	ea000005 	b	514c <sd_cmd+0x88>
    5134:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5138:	e0022001 	and	r2, r2, r1
    513c:	e3520000 	cmp	r2, #0
    5140:	1a000009 	bne	516c <sd_cmd+0xa8>
    5144:	e2555001 	subs	r5, r5, #1
    5148:	3a0000e6 	bcc	54e8 <sd_cmd+0x424>
    514c:	e3a00000 	mov	r0, #0
    5150:	eb001f76 	bl	cf30 <usleep>
    5154:	e5993000 	ldr	r3, [r9]
    5158:	e2833603 	add	r3, r3, #3145728	; 0x300000
    515c:	e5931024 	ldr	r1, [r3, #36]	; 0x24
    5160:	e3110001 	tst	r1, #1
    5164:	e59f2418 	ldr	r2, [pc, #1048]	; 5584 <sd_cmd+0x4c0>
    5168:	1afffff1 	bne	5134 <sd_cmd+0x70>
    516c:	e3550000 	cmp	r5, #0
    5170:	0a0000dc 	beq	54e8 <sd_cmd+0x424>
    5174:	e2833030 	add	r3, r3, #48	; 0x30
    5178:	e5931000 	ldr	r1, [r3]
    517c:	e59f2400 	ldr	r2, [pc, #1024]	; 5584 <sd_cmd+0x4c0>
    5180:	e0022001 	and	r2, r2, r1
    5184:	e3520000 	cmp	r2, #0
    5188:	1a0000d6 	bne	54e8 <sd_cmd+0x424>
    518c:	e28424c9 	add	r2, r4, #-922746880	; 0xc9000000
    5190:	e2726000 	rsbs	r6, r2, #0
    5194:	e0a66002 	adc	r6, r6, r2
    5198:	e5932000 	ldr	r2, [r3]
    519c:	e5832000 	str	r2, [r3]
    51a0:	e5993000 	ldr	r3, [r9]
    51a4:	e59f53e0 	ldr	r5, [pc, #992]	; 558c <sd_cmd+0x4c8>
    51a8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    51ac:	e5838008 	str	r8, [r3, #8]
    51b0:	e0651004 	rsb	r1, r5, r4
    51b4:	e2715000 	rsbs	r5, r1, #0
    51b8:	e5993000 	ldr	r3, [r9]
    51bc:	e0a55001 	adc	r5, r5, r1
    51c0:	e2833603 	add	r3, r3, #3145728	; 0x300000
    51c4:	e1962005 	orrs	r2, r6, r5
    51c8:	e583400c 	str	r4, [r3, #12]
    51cc:	1a0000b9 	bne	54b8 <sd_cmd+0x3f4>
    51d0:	e5992000 	ldr	r2, [r9]
    51d4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    51d8:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    51dc:	e59f33ac 	ldr	r3, [pc, #940]	; 5590 <sd_cmd+0x4cc>
    51e0:	e0033001 	and	r3, r3, r1
    51e4:	e3530000 	cmp	r3, #0
    51e8:	e2823030 	add	r3, r2, #48	; 0x30
    51ec:	1a0000cf 	bne	5530 <sd_cmd+0x46c>
    51f0:	e59fa39c 	ldr	sl, [pc, #924]	; 5594 <sd_cmd+0x4d0>
    51f4:	ea000001 	b	5200 <sd_cmd+0x13c>
    51f8:	e25aa001 	subs	sl, sl, #1
    51fc:	3a0000b0 	bcc	54c4 <sd_cmd+0x400>
    5200:	e3a00000 	mov	r0, #0
    5204:	eb001f49 	bl	cf30 <usleep>
    5208:	e5993000 	ldr	r3, [r9]
    520c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5210:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5214:	e59f2374 	ldr	r2, [pc, #884]	; 5590 <sd_cmd+0x4cc>
    5218:	e0022001 	and	r2, r2, r1
    521c:	e3520000 	cmp	r2, #0
    5220:	e2833030 	add	r3, r3, #48	; 0x30
    5224:	0afffff3 	beq	51f8 <sd_cmd+0x134>
    5228:	e35a0000 	cmp	sl, #0
    522c:	e5932000 	ldr	r2, [r3]
    5230:	0a0000a4 	beq	54c8 <sd_cmd+0x404>
    5234:	e3120811 	tst	r2, #1114112	; 0x110000
    5238:	1a0000a2 	bne	54c8 <sd_cmd+0x404>
    523c:	e59f0340 	ldr	r0, [pc, #832]	; 5584 <sd_cmd+0x4c0>
    5240:	e0000002 	and	r0, r0, r2
    5244:	e3500000 	cmp	r0, #0
    5248:	15832000 	strne	r2, [r3]
    524c:	13e03001 	mvnne	r3, #1
    5250:	1a00009e 	bne	54d0 <sd_cmd+0x40c>
    5254:	e3a02001 	mov	r2, #1
    5258:	e3540000 	cmp	r4, #0
    525c:	03866001 	orreq	r6, r6, #1
    5260:	e5832000 	str	r2, [r3]
    5264:	e5993000 	ldr	r3, [r9]
    5268:	e3560000 	cmp	r6, #0
    526c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5270:	e5932010 	ldr	r2, [r3, #16]
    5274:	1a00000d 	bne	52b0 <sd_cmd+0x1ec>
    5278:	e59f1318 	ldr	r1, [pc, #792]	; 5598 <sd_cmd+0x4d4>
    527c:	e1540001 	cmp	r4, r1
    5280:	02020020 	andeq	r0, r2, #32
    5284:	0a000009 	beq	52b0 <sd_cmd+0x1ec>
    5288:	e3550000 	cmp	r5, #0
    528c:	1a000099 	bne	54f8 <sd_cmd+0x434>
    5290:	e59f1304 	ldr	r1, [pc, #772]	; 559c <sd_cmd+0x4d8>
    5294:	e1540001 	cmp	r4, r1
    5298:	0a00009d 	beq	5514 <sd_cmd+0x450>
    529c:	e59f32fc 	ldr	r3, [pc, #764]	; 55a0 <sd_cmd+0x4dc>
    52a0:	e1540003 	cmp	r4, r3
    52a4:	0a0000a3 	beq	5538 <sd_cmd+0x474>
    52a8:	e59f02f4 	ldr	r0, [pc, #756]	; 55a4 <sd_cmd+0x4e0>
    52ac:	e0000002 	and	r0, r0, r2
    52b0:	e24bd028 	sub	sp, fp, #40	; 0x28
    52b4:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    52b8:	e12fff1e 	bx	lr
    52bc:	e59f22e4 	ldr	r2, [pc, #740]	; 55a8 <sd_cmd+0x4e4>
    52c0:	e79c6002 	ldr	r6, [ip, r2]
    52c4:	e5961000 	ldr	r1, [r6]
    52c8:	e5960000 	ldr	r0, [r6]
    52cc:	e59f22ac 	ldr	r2, [pc, #684]	; 5580 <sd_cmd+0x4bc>
    52d0:	e5873000 	str	r3, [r7]
    52d4:	e50b0030 	str	r0, [fp, #-48]	; 0x30
    52d8:	e79c9002 	ldr	r9, [ip, r2]
    52dc:	e5993000 	ldr	r3, [r9]
    52e0:	e2833603 	add	r3, r3, #3145728	; 0x300000
    52e4:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    52e8:	e3510000 	cmp	r1, #0
    52ec:	e59f52a4 	ldr	r5, [pc, #676]	; 5598 <sd_cmd+0x4d4>
    52f0:	03a05437 	moveq	r5, #922746880	; 0x37000000
    52f4:	e3120001 	tst	r2, #1
    52f8:	0a000017 	beq	535c <sd_cmd+0x298>
    52fc:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5300:	e59f227c 	ldr	r2, [pc, #636]	; 5584 <sd_cmd+0x4c0>
    5304:	e0022001 	and	r2, r2, r1
    5308:	e3520000 	cmp	r2, #0
    530c:	e2833030 	add	r3, r3, #48	; 0x30
    5310:	1a000012 	bne	5360 <sd_cmd+0x29c>
    5314:	e59fa26c 	ldr	sl, [pc, #620]	; 5588 <sd_cmd+0x4c4>
    5318:	ea000005 	b	5334 <sd_cmd+0x270>
    531c:	e5930030 	ldr	r0, [r3, #48]	; 0x30
    5320:	e0022000 	and	r2, r2, r0
    5324:	e3520000 	cmp	r2, #0
    5328:	1a000009 	bne	5354 <sd_cmd+0x290>
    532c:	e25aa001 	subs	sl, sl, #1
    5330:	3a000074 	bcc	5508 <sd_cmd+0x444>
    5334:	e3a00000 	mov	r0, #0
    5338:	eb001efc 	bl	cf30 <usleep>
    533c:	e5993000 	ldr	r3, [r9]
    5340:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5344:	e5930024 	ldr	r0, [r3, #36]	; 0x24
    5348:	e3100001 	tst	r0, #1
    534c:	e59f2230 	ldr	r2, [pc, #560]	; 5584 <sd_cmd+0x4c0>
    5350:	1afffff1 	bne	531c <sd_cmd+0x258>
    5354:	e35a0000 	cmp	sl, #0
    5358:	0a00006a 	beq	5508 <sd_cmd+0x444>
    535c:	e2833030 	add	r3, r3, #48	; 0x30
    5360:	e5931000 	ldr	r1, [r3]
    5364:	e59f2218 	ldr	r2, [pc, #536]	; 5584 <sd_cmd+0x4c0>
    5368:	e0022001 	and	r2, r2, r1
    536c:	e3520000 	cmp	r2, #0
    5370:	1a000064 	bne	5508 <sd_cmd+0x444>
    5374:	e5932000 	ldr	r2, [r3]
    5378:	e5832000 	str	r2, [r3]
    537c:	e5993000 	ldr	r3, [r9]
    5380:	e51b2030 	ldr	r2, [fp, #-48]	; 0x30
    5384:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5388:	e5832008 	str	r2, [r3, #8]
    538c:	e5993000 	ldr	r3, [r9]
    5390:	e3550437 	cmp	r5, #922746880	; 0x37000000
    5394:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5398:	e583500c 	str	r5, [r3, #12]
    539c:	0a00004e 	beq	54dc <sd_cmd+0x418>
    53a0:	e5992000 	ldr	r2, [r9]
    53a4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    53a8:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    53ac:	e59f31dc 	ldr	r3, [pc, #476]	; 5590 <sd_cmd+0x4cc>
    53b0:	e0033001 	and	r3, r3, r1
    53b4:	e3530000 	cmp	r3, #0
    53b8:	e2823030 	add	r3, r2, #48	; 0x30
    53bc:	1a00006b 	bne	5570 <sd_cmd+0x4ac>
    53c0:	e59fa1cc 	ldr	sl, [pc, #460]	; 5594 <sd_cmd+0x4d0>
    53c4:	ea000001 	b	53d0 <sd_cmd+0x30c>
    53c8:	e25aa001 	subs	sl, sl, #1
    53cc:	3a00002f 	bcc	5490 <sd_cmd+0x3cc>
    53d0:	e3a00000 	mov	r0, #0
    53d4:	eb001ed5 	bl	cf30 <usleep>
    53d8:	e5993000 	ldr	r3, [r9]
    53dc:	e2833603 	add	r3, r3, #3145728	; 0x300000
    53e0:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    53e4:	e59f21a4 	ldr	r2, [pc, #420]	; 5590 <sd_cmd+0x4cc>
    53e8:	e0022001 	and	r2, r2, r1
    53ec:	e3520000 	cmp	r2, #0
    53f0:	e2833030 	add	r3, r3, #48	; 0x30
    53f4:	0afffff3 	beq	53c8 <sd_cmd+0x304>
    53f8:	e35a0000 	cmp	sl, #0
    53fc:	e5931000 	ldr	r1, [r3]
    5400:	0a000023 	beq	5494 <sd_cmd+0x3d0>
    5404:	e3110811 	tst	r1, #1114112	; 0x110000
    5408:	1a000021 	bne	5494 <sd_cmd+0x3d0>
    540c:	e59f2170 	ldr	r2, [pc, #368]	; 5584 <sd_cmd+0x4c0>
    5410:	e0022001 	and	r2, r2, r1
    5414:	e3520000 	cmp	r2, #0
    5418:	15831000 	strne	r1, [r3]
    541c:	13e03001 	mvnne	r3, #1
    5420:	1a00001d 	bne	549c <sd_cmd+0x3d8>
    5424:	e3a02001 	mov	r2, #1
    5428:	e5832000 	str	r2, [r3]
    542c:	e5993000 	ldr	r3, [r9]
    5430:	e3550437 	cmp	r5, #922746880	; 0x37000000
    5434:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5438:	e5931010 	ldr	r1, [r3, #16]
    543c:	0a000017 	beq	54a0 <sd_cmd+0x3dc>
    5440:	e59f2150 	ldr	r2, [pc, #336]	; 5598 <sd_cmd+0x4d4>
    5444:	e1550002 	cmp	r5, r2
    5448:	159f2154 	ldrne	r2, [pc, #340]	; 55a4 <sd_cmd+0x4e0>
    544c:	02012020 	andeq	r2, r1, #32
    5450:	10022001 	andne	r2, r2, r1
    5454:	e5961000 	ldr	r1, [r6]
    5458:	e2722001 	rsbs	r2, r2, #1
    545c:	33a02000 	movcc	r2, #0
    5460:	e3510000 	cmp	r1, #0
    5464:	03a02000 	moveq	r2, #0
    5468:	e3520000 	cmp	r2, #0
    546c:	1a000001 	bne	5478 <sd_cmd+0x3b4>
    5470:	e3c44102 	bic	r4, r4, #-2147483648	; 0x80000000
    5474:	eaffff23 	b	5108 <sd_cmd+0x44>
    5478:	e3e03001 	mvn	r3, #1
    547c:	e3a00000 	mov	r0, #0
    5480:	e5873000 	str	r3, [r7]
    5484:	e24bd028 	sub	sp, fp, #40	; 0x28
    5488:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    548c:	e12fff1e 	bx	lr
    5490:	e5931000 	ldr	r1, [r3]
    5494:	e5831000 	str	r1, [r3]
    5498:	e3e03000 	mvn	r3, #0
    549c:	e5873000 	str	r3, [r7]
    54a0:	e5963000 	ldr	r3, [r6]
    54a4:	e3530000 	cmp	r3, #0
    54a8:	1afffff2 	bne	5478 <sd_cmd+0x3b4>
    54ac:	e5993000 	ldr	r3, [r9]
    54b0:	e2833603 	add	r3, r3, #3145728	; 0x300000
    54b4:	eaffffed 	b	5470 <sd_cmd+0x3ac>
    54b8:	e59f00ec 	ldr	r0, [pc, #236]	; 55ac <sd_cmd+0x4e8>
    54bc:	eb001e9b 	bl	cf30 <usleep>
    54c0:	eaffff42 	b	51d0 <sd_cmd+0x10c>
    54c4:	e5932000 	ldr	r2, [r3]
    54c8:	e5832000 	str	r2, [r3]
    54cc:	e3e03000 	mvn	r3, #0
    54d0:	e5873000 	str	r3, [r7]
    54d4:	e3a00000 	mov	r0, #0
    54d8:	eaffff74 	b	52b0 <sd_cmd+0x1ec>
    54dc:	e59f00c8 	ldr	r0, [pc, #200]	; 55ac <sd_cmd+0x4e8>
    54e0:	eb001e92 	bl	cf30 <usleep>
    54e4:	eaffffad 	b	53a0 <sd_cmd+0x2dc>
    54e8:	e3e03000 	mvn	r3, #0
    54ec:	e3a00000 	mov	r0, #0
    54f0:	e5873000 	str	r3, [r7]
    54f4:	eaffff6d 	b	52b0 <sd_cmd+0x1ec>
    54f8:	e1580002 	cmp	r8, r2
    54fc:	13e00001 	mvnne	r0, #1
    5500:	03a00000 	moveq	r0, #0
    5504:	eaffff69 	b	52b0 <sd_cmd+0x1ec>
    5508:	e3e03000 	mvn	r3, #0
    550c:	e5873000 	str	r3, [r7]
    5510:	eaffffe2 	b	54a0 <sd_cmd+0x3dc>
    5514:	e593001c 	ldr	r0, [r3, #28]
    5518:	e5931018 	ldr	r1, [r3, #24]
    551c:	e1800002 	orr	r0, r0, r2
    5520:	e5933014 	ldr	r3, [r3, #20]
    5524:	e1800001 	orr	r0, r0, r1
    5528:	e1800003 	orr	r0, r0, r3
    552c:	eaffff5f 	b	52b0 <sd_cmd+0x1ec>
    5530:	e5922030 	ldr	r2, [r2, #48]	; 0x30
    5534:	eaffff3e 	b	5234 <sd_cmd+0x170>
    5538:	e2023901 	and	r3, r2, #16384	; 0x4000
    553c:	e2021a02 	and	r1, r2, #8192	; 0x2000
    5540:	e1a03403 	lsl	r3, r3, #8
    5544:	e1833301 	orr	r3, r3, r1, lsl #6
    5548:	e1a01982 	lsl	r1, r2, #19
    554c:	e18339a1 	orr	r3, r3, r1, lsr #19
    5550:	e2020902 	and	r0, r2, #32768	; 0x8000
    5554:	e59f1048 	ldr	r1, [pc, #72]	; 55a4 <sd_cmd+0x4e0>
    5558:	e1833400 	orr	r3, r3, r0, lsl #8
    555c:	e0011003 	and	r1, r1, r3
    5560:	e1a00822 	lsr	r0, r2, #16
    5564:	e1a00800 	lsl	r0, r0, #16
    5568:	e5871000 	str	r1, [r7]
    556c:	eaffff4f 	b	52b0 <sd_cmd+0x1ec>
    5570:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    5574:	eaffffa2 	b	5404 <sd_cmd+0x340>
    5578:	00012918 	andeq	r2, r1, r8, lsl r9
    557c:	00000028 	andeq	r0, r0, r8, lsr #32
    5580:	00000024 	andeq	r0, r0, r4, lsr #32
    5584:	017e8000 	cmneq	lr, r0
    5588:	000f423f 	andeq	r4, pc, pc, lsr r2	; <UNPREDICTABLE>
    558c:	08020000 	stmdaeq	r2, {}	; <UNPREDICTABLE>
    5590:	017e8001 	cmneq	lr, r1
    5594:	0000270f 	andeq	r2, r0, pc, lsl #14
    5598:	37020000 	strcc	r0, [r2, -r0]
    559c:	02010000 	andeq	r0, r1, #0
    55a0:	03020000 	movweq	r0, #8192	; 0x2000
    55a4:	fff9c004 			; <UNDEFINED> instruction: 0xfff9c004
    55a8:	0000002c 	andeq	r0, r0, ip, lsr #32
    55ac:	000186a0 	andeq	r8, r1, r0, lsr #13

000055b0 <bcm283x_sd_init>:
    55b0:	e1a0c00d 	mov	ip, sp
    55b4:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    55b8:	e24cb004 	sub	fp, ip, #4
    55bc:	e24dd014 	sub	sp, sp, #20
    55c0:	e3a00000 	mov	r0, #0
    55c4:	eb001d5f 	bl	cb48 <mmio_map>
    55c8:	e3a01001 	mov	r1, #1
    55cc:	e3a0c002 	mov	ip, #2
    55d0:	e3a0e902 	mov	lr, #32768	; 0x8000
    55d4:	e59f6f64 	ldr	r6, [pc, #3940]	; 6540 <bcm283x_sd_init+0xf90>
    55d8:	e59f2f64 	ldr	r2, [pc, #3940]	; 6544 <bcm283x_sd_init+0xf94>
    55dc:	e59f3f64 	ldr	r3, [pc, #3940]	; 6548 <bcm283x_sd_init+0xf98>
    55e0:	e08f6006 	add	r6, pc, r6
    55e4:	e08f2002 	add	r2, pc, r2
    55e8:	e796a003 	ldr	sl, [r6, r3]
    55ec:	e5821204 	str	r1, [r2, #516]	; 0x204
    55f0:	e5821208 	str	r1, [r2, #520]	; 0x208
    55f4:	e3a02000 	mov	r2, #0
    55f8:	e58a0000 	str	r0, [sl]
    55fc:	e2800602 	add	r0, r0, #2097152	; 0x200000
    5600:	e5904010 	ldr	r4, [r0, #16]
    5604:	e3c4460e 	bic	r4, r4, #14680064	; 0xe00000
    5608:	e5804010 	str	r4, [r0, #16]
    560c:	e59a1000 	ldr	r1, [sl]
    5610:	e2811602 	add	r1, r1, #2097152	; 0x200000
    5614:	e581c094 	str	ip, [r1, #148]	; 0x94
    5618:	e59a1000 	ldr	r1, [sl]
    561c:	e2811602 	add	r1, r1, #2097152	; 0x200000
    5620:	e581e09c 	str	lr, [r1, #156]	; 0x9c
    5624:	e59a1000 	ldr	r1, [sl]
    5628:	e3a00803 	mov	r0, #196608	; 0x30000
    562c:	e2811602 	add	r1, r1, #2097152	; 0x200000
    5630:	e5812094 	str	r2, [r1, #148]	; 0x94
    5634:	e59a1000 	ldr	r1, [sl]
    5638:	e2811602 	add	r1, r1, #2097152	; 0x200000
    563c:	e581209c 	str	r2, [r1, #156]	; 0x9c
    5640:	e59a1000 	ldr	r1, [sl]
    5644:	e2811602 	add	r1, r1, #2097152	; 0x200000
    5648:	e591e068 	ldr	lr, [r1, #104]	; 0x68
    564c:	e38ee902 	orr	lr, lr, #32768	; 0x8000
    5650:	e581e068 	str	lr, [r1, #104]	; 0x68
    5654:	e59a1000 	ldr	r1, [sl]
    5658:	e2811602 	add	r1, r1, #2097152	; 0x200000
    565c:	e591e010 	ldr	lr, [r1, #16]
    5660:	e38ee43f 	orr	lr, lr, #1056964608	; 0x3f000000
    5664:	e581e010 	str	lr, [r1, #16]
    5668:	e59a1000 	ldr	r1, [sl]
    566c:	e2811602 	add	r1, r1, #2097152	; 0x200000
    5670:	e581c094 	str	ip, [r1, #148]	; 0x94
    5674:	e59a1000 	ldr	r1, [sl]
    5678:	e2811602 	add	r1, r1, #2097152	; 0x200000
    567c:	e581009c 	str	r0, [r1, #156]	; 0x9c
    5680:	e59a1000 	ldr	r1, [sl]
    5684:	e2811602 	add	r1, r1, #2097152	; 0x200000
    5688:	e5812094 	str	r2, [r1, #148]	; 0x94
    568c:	e59a1000 	ldr	r1, [sl]
    5690:	e2811602 	add	r1, r1, #2097152	; 0x200000
    5694:	e581209c 	str	r2, [r1, #156]	; 0x9c
    5698:	e59a0000 	ldr	r0, [sl]
    569c:	e2800602 	add	r0, r0, #2097152	; 0x200000
    56a0:	e5901014 	ldr	r1, [r0, #20]
    56a4:	e1e01621 	mvn	r1, r1, lsr #12
    56a8:	e1e01601 	mvn	r1, r1, lsl #12
    56ac:	e3a0e70f 	mov	lr, #3932160	; 0x3c0000
    56b0:	e5801014 	str	r1, [r0, #20]
    56b4:	e59a1000 	ldr	r1, [sl]
    56b8:	e2811602 	add	r1, r1, #2097152	; 0x200000
    56bc:	e581c094 	str	ip, [r1, #148]	; 0x94
    56c0:	e59a1000 	ldr	r1, [sl]
    56c4:	e2811602 	add	r1, r1, #2097152	; 0x200000
    56c8:	e581e09c 	str	lr, [r1, #156]	; 0x9c
    56cc:	e59a1000 	ldr	r1, [sl]
    56d0:	e2811602 	add	r1, r1, #2097152	; 0x200000
    56d4:	e5812094 	str	r2, [r1, #148]	; 0x94
    56d8:	e59a1000 	ldr	r1, [sl]
    56dc:	e2811602 	add	r1, r1, #2097152	; 0x200000
    56e0:	e581209c 	str	r2, [r1, #156]	; 0x9c
    56e4:	e59a0000 	ldr	r0, [sl]
    56e8:	e2800603 	add	r0, r0, #3145728	; 0x300000
    56ec:	e59010fc 	ldr	r1, [r0, #252]	; 0xfc
    56f0:	e59fce54 	ldr	ip, [pc, #3668]	; 654c <bcm283x_sd_init+0xf9c>
    56f4:	e20118ff 	and	r1, r1, #16711680	; 0xff0000
    56f8:	e796c00c 	ldr	ip, [r6, ip]
    56fc:	e1a01821 	lsr	r1, r1, #16
    5700:	e58c1000 	str	r1, [ip]
    5704:	e5802028 	str	r2, [r0, #40]	; 0x28
    5708:	e59a2000 	ldr	r2, [sl]
    570c:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5710:	e592102c 	ldr	r1, [r2, #44]	; 0x2c
    5714:	e3811401 	orr	r1, r1, #16777216	; 0x1000000
    5718:	e59f8e30 	ldr	r8, [pc, #3632]	; 6550 <bcm283x_sd_init+0xfa0>
    571c:	e3a09000 	mov	r9, #0
    5720:	e3e04000 	mvn	r4, #0
    5724:	e3e05000 	mvn	r5, #0
    5728:	e582102c 	str	r1, [r2, #44]	; 0x2c
    572c:	ea000004 	b	5744 <bcm283x_sd_init+0x194>
    5730:	e2588001 	subs	r8, r8, #1
    5734:	e2c99000 	sbc	r9, r9, #0
    5738:	e1590005 	cmp	r9, r5
    573c:	01580004 	cmpeq	r8, r4
    5740:	0a000084 	beq	5958 <bcm283x_sd_init+0x3a8>
    5744:	e59f0e04 	ldr	r0, [pc, #3588]	; 6550 <bcm283x_sd_init+0xfa0>
    5748:	eb001df8 	bl	cf30 <usleep>
    574c:	e59a2000 	ldr	r2, [sl]
    5750:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5754:	e592302c 	ldr	r3, [r2, #44]	; 0x2c
    5758:	e3130401 	tst	r3, #16777216	; 0x1000000
    575c:	e282202c 	add	r2, r2, #44	; 0x2c
    5760:	1afffff2 	bne	5730 <bcm283x_sd_init+0x180>
    5764:	e1983009 	orrs	r3, r8, r9
    5768:	0a00007a 	beq	5958 <bcm283x_sd_init+0x3a8>
    576c:	e5921000 	ldr	r1, [r2]
    5770:	e381180e 	orr	r1, r1, #917504	; 0xe0000
    5774:	e3811001 	orr	r1, r1, #1
    5778:	e5821000 	str	r1, [r2]
    577c:	e59f0dcc 	ldr	r0, [pc, #3532]	; 6550 <bcm283x_sd_init+0xfa0>
    5780:	eb001dea 	bl	cf30 <usleep>
    5784:	e59f0dc8 	ldr	r0, [pc, #3528]	; 6554 <bcm283x_sd_init+0xfa4>
    5788:	ebfffdd6 	bl	4ee8 <sd_clk>
    578c:	e2508000 	subs	r8, r0, #0
    5790:	0a000003 	beq	57a4 <bcm283x_sd_init+0x1f4>
    5794:	e1a00008 	mov	r0, r8
    5798:	e24bd028 	sub	sp, fp, #40	; 0x28
    579c:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    57a0:	e12fff1e 	bx	lr
    57a4:	e3e01000 	mvn	r1, #0
    57a8:	e59a2000 	ldr	r2, [sl]
    57ac:	e2822603 	add	r2, r2, #3145728	; 0x300000
    57b0:	e5821038 	str	r1, [r2, #56]	; 0x38
    57b4:	e59a2000 	ldr	r2, [sl]
    57b8:	e2822603 	add	r2, r2, #3145728	; 0x300000
    57bc:	e5821034 	str	r1, [r2, #52]	; 0x34
    57c0:	e59f2d90 	ldr	r2, [pc, #3472]	; 6558 <bcm283x_sd_init+0xfa8>
    57c4:	e7964002 	ldr	r4, [r6, r2]
    57c8:	e59f2d8c 	ldr	r2, [pc, #3468]	; 655c <bcm283x_sd_init+0xfac>
    57cc:	e5848000 	str	r8, [r4]
    57d0:	e7965002 	ldr	r5, [r6, r2]
    57d4:	e59f2d84 	ldr	r2, [pc, #3460]	; 6560 <bcm283x_sd_init+0xfb0>
    57d8:	e5858000 	str	r8, [r5]
    57dc:	e7963002 	ldr	r3, [r6, r2]
    57e0:	e59a2000 	ldr	r2, [sl]
    57e4:	e5838004 	str	r8, [r3, #4]
    57e8:	e5838000 	str	r8, [r3]
    57ec:	e5848000 	str	r8, [r4]
    57f0:	e2822603 	add	r2, r2, #3145728	; 0x300000
    57f4:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    57f8:	e3110001 	tst	r1, #1
    57fc:	e50b3030 	str	r3, [fp, #-48]	; 0x30
    5800:	0a000017 	beq	5864 <bcm283x_sd_init+0x2b4>
    5804:	e5920030 	ldr	r0, [r2, #48]	; 0x30
    5808:	e59f1d54 	ldr	r1, [pc, #3412]	; 6564 <bcm283x_sd_init+0xfb4>
    580c:	e0011000 	and	r1, r1, r0
    5810:	e3510000 	cmp	r1, #0
    5814:	e2822030 	add	r2, r2, #48	; 0x30
    5818:	1a000012 	bne	5868 <bcm283x_sd_init+0x2b8>
    581c:	e59f6d44 	ldr	r6, [pc, #3396]	; 6568 <bcm283x_sd_init+0xfb8>
    5820:	ea000005 	b	583c <bcm283x_sd_init+0x28c>
    5824:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    5828:	e0033001 	and	r3, r3, r1
    582c:	e3530000 	cmp	r3, #0
    5830:	1a000009 	bne	585c <bcm283x_sd_init+0x2ac>
    5834:	e2566001 	subs	r6, r6, #1
    5838:	3a000250 	bcc	6180 <bcm283x_sd_init+0xbd0>
    583c:	e3a00000 	mov	r0, #0
    5840:	eb001dba 	bl	cf30 <usleep>
    5844:	e59a2000 	ldr	r2, [sl]
    5848:	e2822603 	add	r2, r2, #3145728	; 0x300000
    584c:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    5850:	e3110001 	tst	r1, #1
    5854:	e59f3d08 	ldr	r3, [pc, #3336]	; 6564 <bcm283x_sd_init+0xfb4>
    5858:	1afffff1 	bne	5824 <bcm283x_sd_init+0x274>
    585c:	e3560000 	cmp	r6, #0
    5860:	0a000246 	beq	6180 <bcm283x_sd_init+0xbd0>
    5864:	e2822030 	add	r2, r2, #48	; 0x30
    5868:	e5920000 	ldr	r0, [r2]
    586c:	e59f1cf0 	ldr	r1, [pc, #3312]	; 6564 <bcm283x_sd_init+0xfb4>
    5870:	e0011000 	and	r1, r1, r0
    5874:	e3510000 	cmp	r1, #0
    5878:	1a000240 	bne	6180 <bcm283x_sd_init+0xbd0>
    587c:	e5920000 	ldr	r0, [r2]
    5880:	e5820000 	str	r0, [r2]
    5884:	e59a2000 	ldr	r2, [sl]
    5888:	e2822603 	add	r2, r2, #3145728	; 0x300000
    588c:	e5821008 	str	r1, [r2, #8]
    5890:	e59a2000 	ldr	r2, [sl]
    5894:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5898:	e582100c 	str	r1, [r2, #12]
    589c:	e59a1000 	ldr	r1, [sl]
    58a0:	e2811603 	add	r1, r1, #3145728	; 0x300000
    58a4:	e5910030 	ldr	r0, [r1, #48]	; 0x30
    58a8:	e59f2cbc 	ldr	r2, [pc, #3260]	; 656c <bcm283x_sd_init+0xfbc>
    58ac:	e0022000 	and	r2, r2, r0
    58b0:	e3520000 	cmp	r2, #0
    58b4:	059f6cb4 	ldreq	r6, [pc, #3252]	; 6570 <bcm283x_sd_init+0xfc0>
    58b8:	e2812030 	add	r2, r1, #48	; 0x30
    58bc:	0a000002 	beq	58cc <bcm283x_sd_init+0x31c>
    58c0:	ea000233 	b	6194 <bcm283x_sd_init+0xbe4>
    58c4:	e2566001 	subs	r6, r6, #1
    58c8:	3a000220 	bcc	6150 <bcm283x_sd_init+0xba0>
    58cc:	e3a00000 	mov	r0, #0
    58d0:	eb001d96 	bl	cf30 <usleep>
    58d4:	e59a3000 	ldr	r3, [sl]
    58d8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    58dc:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    58e0:	e59f2c84 	ldr	r2, [pc, #3204]	; 656c <bcm283x_sd_init+0xfbc>
    58e4:	e0022001 	and	r2, r2, r1
    58e8:	e3520000 	cmp	r2, #0
    58ec:	e2833030 	add	r3, r3, #48	; 0x30
    58f0:	0afffff3 	beq	58c4 <bcm283x_sd_init+0x314>
    58f4:	e3560000 	cmp	r6, #0
    58f8:	e1a02003 	mov	r2, r3
    58fc:	e5930000 	ldr	r0, [r3]
    5900:	0a000214 	beq	6158 <bcm283x_sd_init+0xba8>
    5904:	e3100811 	tst	r0, #1114112	; 0x110000
    5908:	1a000212 	bne	6158 <bcm283x_sd_init+0xba8>
    590c:	e59f1c50 	ldr	r1, [pc, #3152]	; 6564 <bcm283x_sd_init+0xfb4>
    5910:	e0011000 	and	r1, r1, r0
    5914:	e3510000 	cmp	r1, #0
    5918:	15820000 	strne	r0, [r2]
    591c:	13e02001 	mvnne	r2, #1
    5920:	1a00020e 	bne	6160 <bcm283x_sd_init+0xbb0>
    5924:	e3a01001 	mov	r1, #1
    5928:	e5821000 	str	r1, [r2]
    592c:	e59a2000 	ldr	r2, [sl]
    5930:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5934:	e5922010 	ldr	r2, [r2, #16]
    5938:	e5942000 	ldr	r2, [r4]
    593c:	e3520000 	cmp	r2, #0
    5940:	0a000009 	beq	596c <bcm283x_sd_init+0x3bc>
    5944:	e5948000 	ldr	r8, [r4]
    5948:	e1a00008 	mov	r0, r8
    594c:	e24bd028 	sub	sp, fp, #40	; 0x28
    5950:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    5954:	e12fff1e 	bx	lr
    5958:	e3e08001 	mvn	r8, #1
    595c:	e1a00008 	mov	r0, r8
    5960:	e24bd028 	sub	sp, fp, #40	; 0x28
    5964:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    5968:	e12fff1e 	bx	lr
    596c:	e59f0c00 	ldr	r0, [pc, #3072]	; 6574 <bcm283x_sd_init+0xfc4>
    5970:	e59f1c00 	ldr	r1, [pc, #3072]	; 6578 <bcm283x_sd_init+0xfc8>
    5974:	ebfffdd2 	bl	50c4 <sd_cmd>
    5978:	e5942000 	ldr	r2, [r4]
    597c:	e3520000 	cmp	r2, #0
    5980:	1affffef 	bne	5944 <bcm283x_sd_init+0x394>
    5984:	e3a06006 	mov	r6, #6
    5988:	e3a07000 	mov	r7, #0
    598c:	e59f9be8 	ldr	r9, [pc, #3048]	; 657c <bcm283x_sd_init+0xfcc>
    5990:	e3e03000 	mvn	r3, #0
    5994:	e3e02000 	mvn	r2, #0
    5998:	e2566001 	subs	r6, r6, #1
    599c:	e2c77000 	sbc	r7, r7, #0
    59a0:	e1570003 	cmp	r7, r3
    59a4:	01560002 	cmpeq	r6, r2
    59a8:	0a0001e1 	beq	6134 <bcm283x_sd_init+0xb84>
    59ac:	e3a03000 	mov	r3, #0
    59b0:	e5843000 	str	r3, [r4]
    59b4:	e5950000 	ldr	r0, [r5]
    59b8:	e1500003 	cmp	r0, r3
    59bc:	11a00009 	movne	r0, r9
    59c0:	03a00437 	moveq	r0, #922746880	; 0x37000000
    59c4:	e5951000 	ldr	r1, [r5]
    59c8:	ebfffdbd 	bl	50c4 <sd_cmd>
    59cc:	e5953000 	ldr	r3, [r5]
    59d0:	e2700001 	rsbs	r0, r0, #1
    59d4:	33a00000 	movcc	r0, #0
    59d8:	e3530000 	cmp	r3, #0
    59dc:	03a00000 	moveq	r0, #0
    59e0:	e3500000 	cmp	r0, #0
    59e4:	1a0001df 	bne	6168 <bcm283x_sd_init+0xbb8>
    59e8:	e59a3000 	ldr	r3, [sl]
    59ec:	e2833603 	add	r3, r3, #3145728	; 0x300000
    59f0:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    59f4:	e3120001 	tst	r2, #1
    59f8:	0a000017 	beq	5a5c <bcm283x_sd_init+0x4ac>
    59fc:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5a00:	e59f2b5c 	ldr	r2, [pc, #2908]	; 6564 <bcm283x_sd_init+0xfb4>
    5a04:	e0022001 	and	r2, r2, r1
    5a08:	e3520000 	cmp	r2, #0
    5a0c:	e2833030 	add	r3, r3, #48	; 0x30
    5a10:	059f8b50 	ldreq	r8, [pc, #2896]	; 6568 <bcm283x_sd_init+0xfb8>
    5a14:	0a000006 	beq	5a34 <bcm283x_sd_init+0x484>
    5a18:	ea000010 	b	5a60 <bcm283x_sd_init+0x4b0>
    5a1c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5a20:	e0022001 	and	r2, r2, r1
    5a24:	e3520000 	cmp	r2, #0
    5a28:	1a000009 	bne	5a54 <bcm283x_sd_init+0x4a4>
    5a2c:	e2588001 	subs	r8, r8, #1
    5a30:	3a0001cf 	bcc	6174 <bcm283x_sd_init+0xbc4>
    5a34:	e3a00000 	mov	r0, #0
    5a38:	eb001d3c 	bl	cf30 <usleep>
    5a3c:	e59a3000 	ldr	r3, [sl]
    5a40:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5a44:	e5931024 	ldr	r1, [r3, #36]	; 0x24
    5a48:	e3110001 	tst	r1, #1
    5a4c:	e59f2b10 	ldr	r2, [pc, #2832]	; 6564 <bcm283x_sd_init+0xfb4>
    5a50:	1afffff1 	bne	5a1c <bcm283x_sd_init+0x46c>
    5a54:	e3580000 	cmp	r8, #0
    5a58:	0a0001c5 	beq	6174 <bcm283x_sd_init+0xbc4>
    5a5c:	e2833030 	add	r3, r3, #48	; 0x30
    5a60:	e5931000 	ldr	r1, [r3]
    5a64:	e59f2af8 	ldr	r2, [pc, #2808]	; 6564 <bcm283x_sd_init+0xfb4>
    5a68:	e0022001 	and	r2, r2, r1
    5a6c:	e3520000 	cmp	r2, #0
    5a70:	1a0001bf 	bne	6174 <bcm283x_sd_init+0xbc4>
    5a74:	e5932000 	ldr	r2, [r3]
    5a78:	e5832000 	str	r2, [r3]
    5a7c:	e59a3000 	ldr	r3, [sl]
    5a80:	e59f2af8 	ldr	r2, [pc, #2808]	; 6580 <bcm283x_sd_init+0xfd0>
    5a84:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5a88:	e5832008 	str	r2, [r3, #8]
    5a8c:	e59a3000 	ldr	r3, [sl]
    5a90:	e59f2aec 	ldr	r2, [pc, #2796]	; 6584 <bcm283x_sd_init+0xfd4>
    5a94:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5a98:	e583200c 	str	r2, [r3, #12]
    5a9c:	e59a2000 	ldr	r2, [sl]
    5aa0:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5aa4:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    5aa8:	e59f3abc 	ldr	r3, [pc, #2748]	; 656c <bcm283x_sd_init+0xfbc>
    5aac:	e0033001 	and	r3, r3, r1
    5ab0:	e3530000 	cmp	r3, #0
    5ab4:	059f8ab4 	ldreq	r8, [pc, #2740]	; 6570 <bcm283x_sd_init+0xfc0>
    5ab8:	e2823030 	add	r3, r2, #48	; 0x30
    5abc:	0a000002 	beq	5acc <bcm283x_sd_init+0x51c>
    5ac0:	ea0001b1 	b	618c <bcm283x_sd_init+0xbdc>
    5ac4:	e2588001 	subs	r8, r8, #1
    5ac8:	3a00019b 	bcc	613c <bcm283x_sd_init+0xb8c>
    5acc:	e3a00000 	mov	r0, #0
    5ad0:	eb001d16 	bl	cf30 <usleep>
    5ad4:	e59a3000 	ldr	r3, [sl]
    5ad8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5adc:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5ae0:	e59f2a84 	ldr	r2, [pc, #2692]	; 656c <bcm283x_sd_init+0xfbc>
    5ae4:	e0022001 	and	r2, r2, r1
    5ae8:	e3520000 	cmp	r2, #0
    5aec:	e2833030 	add	r3, r3, #48	; 0x30
    5af0:	0afffff3 	beq	5ac4 <bcm283x_sd_init+0x514>
    5af4:	e3580000 	cmp	r8, #0
    5af8:	e5932000 	ldr	r2, [r3]
    5afc:	0a00018f 	beq	6140 <bcm283x_sd_init+0xb90>
    5b00:	e3120811 	tst	r2, #1114112	; 0x110000
    5b04:	1a00018d 	bne	6140 <bcm283x_sd_init+0xb90>
    5b08:	e59f8a54 	ldr	r8, [pc, #2644]	; 6564 <bcm283x_sd_init+0xfb4>
    5b0c:	e0088002 	and	r8, r8, r2
    5b10:	e3580000 	cmp	r8, #0
    5b14:	15832000 	strne	r2, [r3]
    5b18:	13e03001 	mvnne	r3, #1
    5b1c:	1a000189 	bne	6148 <bcm283x_sd_init+0xb98>
    5b20:	e3a02001 	mov	r2, #1
    5b24:	e3a01000 	mov	r1, #0
    5b28:	e5832000 	str	r2, [r3]
    5b2c:	e59a3000 	ldr	r3, [sl]
    5b30:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5b34:	e593c010 	ldr	ip, [r3, #16]
    5b38:	e59f2a48 	ldr	r2, [pc, #2632]	; 6588 <bcm283x_sd_init+0xfd8>
    5b3c:	e002200c 	and	r2, r2, ip
    5b40:	e2020102 	and	r0, r2, #-2147483648	; 0x80000000
    5b44:	e1901001 	orrs	r1, r0, r1
    5b48:	e1a03fc2 	asr	r3, r2, #31
    5b4c:	0affff8f 	beq	5990 <bcm283x_sd_init+0x3e0>
    5b50:	e50b203c 	str	r2, [fp, #-60]	; 0x3c
    5b54:	e50b3038 	str	r3, [fp, #-56]	; 0x38
    5b58:	e59f2a2c 	ldr	r2, [pc, #2604]	; 658c <bcm283x_sd_init+0xfdc>
    5b5c:	e002200c 	and	r2, r2, ip
    5b60:	e3520000 	cmp	r2, #0
    5b64:	0a000007 	beq	5b88 <bcm283x_sd_init+0x5d8>
    5b68:	e31c0101 	tst	ip, #1073741824	; 0x40000000
    5b6c:	0a000005 	beq	5b88 <bcm283x_sd_init+0x5d8>
    5b70:	e5942000 	ldr	r2, [r4]
    5b74:	e3720001 	cmn	r2, #1
    5b78:	0a000002 	beq	5b88 <bcm283x_sd_init+0x5d8>
    5b7c:	e5942000 	ldr	r2, [r4]
    5b80:	e3520000 	cmp	r2, #0
    5b84:	1affff6e 	bne	5944 <bcm283x_sd_init+0x394>
    5b88:	e1963007 	orrs	r3, r6, r7
    5b8c:	0a000168 	beq	6134 <bcm283x_sd_init+0xb84>
    5b90:	e3a01000 	mov	r1, #0
    5b94:	e59f09f4 	ldr	r0, [pc, #2548]	; 6590 <bcm283x_sd_init+0xfe0>
    5b98:	e24b303c 	sub	r3, fp, #60	; 0x3c
    5b9c:	e893000c 	ldm	r3, {r2, r3}
    5ba0:	e0033001 	and	r3, r3, r1
    5ba4:	e0022000 	and	r2, r2, r0
    5ba8:	e1a00002 	mov	r0, r2
    5bac:	e1a01003 	mov	r1, r3
    5bb0:	e1903001 	orrs	r3, r0, r1
    5bb4:	0affff67 	beq	5958 <bcm283x_sd_init+0x3a8>
    5bb8:	e3a01000 	mov	r1, #0
    5bbc:	e59a2000 	ldr	r2, [sl]
    5bc0:	e5841000 	str	r1, [r4]
    5bc4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5bc8:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    5bcc:	e3110001 	tst	r1, #1
    5bd0:	02822030 	addeq	r2, r2, #48	; 0x30
    5bd4:	0a00001a 	beq	5c44 <bcm283x_sd_init+0x694>
    5bd8:	e5920030 	ldr	r0, [r2, #48]	; 0x30
    5bdc:	e59f1980 	ldr	r1, [pc, #2432]	; 6564 <bcm283x_sd_init+0xfb4>
    5be0:	e0011000 	and	r1, r1, r0
    5be4:	e3510000 	cmp	r1, #0
    5be8:	e2822030 	add	r2, r2, #48	; 0x30
    5bec:	1a000014 	bne	5c44 <bcm283x_sd_init+0x694>
    5bf0:	e59f6970 	ldr	r6, [pc, #2416]	; 6568 <bcm283x_sd_init+0xfb8>
    5bf4:	ea000006 	b	5c14 <bcm283x_sd_init+0x664>
    5bf8:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    5bfc:	e0033001 	and	r3, r3, r1
    5c00:	e3530000 	cmp	r3, #0
    5c04:	e2822030 	add	r2, r2, #48	; 0x30
    5c08:	1a00000b 	bne	5c3c <bcm283x_sd_init+0x68c>
    5c0c:	e2566001 	subs	r6, r6, #1
    5c10:	3a000179 	bcc	61fc <bcm283x_sd_init+0xc4c>
    5c14:	e3a00000 	mov	r0, #0
    5c18:	eb001cc4 	bl	cf30 <usleep>
    5c1c:	e59a2000 	ldr	r2, [sl]
    5c20:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5c24:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    5c28:	e3110001 	tst	r1, #1
    5c2c:	e59f3930 	ldr	r3, [pc, #2352]	; 6564 <bcm283x_sd_init+0xfb4>
    5c30:	e282c024 	add	ip, r2, #36	; 0x24
    5c34:	1affffef 	bne	5bf8 <bcm283x_sd_init+0x648>
    5c38:	e2822030 	add	r2, r2, #48	; 0x30
    5c3c:	e3560000 	cmp	r6, #0
    5c40:	0a00016d 	beq	61fc <bcm283x_sd_init+0xc4c>
    5c44:	e5920000 	ldr	r0, [r2]
    5c48:	e59f1914 	ldr	r1, [pc, #2324]	; 6564 <bcm283x_sd_init+0xfb4>
    5c4c:	e0011000 	and	r1, r1, r0
    5c50:	e3510000 	cmp	r1, #0
    5c54:	1a000164 	bne	61ec <bcm283x_sd_init+0xc3c>
    5c58:	e5920000 	ldr	r0, [r2]
    5c5c:	e5820000 	str	r0, [r2]
    5c60:	e59a2000 	ldr	r2, [sl]
    5c64:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5c68:	e5821008 	str	r1, [r2, #8]
    5c6c:	e59a2000 	ldr	r2, [sl]
    5c70:	e59f191c 	ldr	r1, [pc, #2332]	; 6594 <bcm283x_sd_init+0xfe4>
    5c74:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5c78:	e582100c 	str	r1, [r2, #12]
    5c7c:	e59a2000 	ldr	r2, [sl]
    5c80:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5c84:	e5920030 	ldr	r0, [r2, #48]	; 0x30
    5c88:	e59f18dc 	ldr	r1, [pc, #2268]	; 656c <bcm283x_sd_init+0xfbc>
    5c8c:	e0011000 	and	r1, r1, r0
    5c90:	e3510000 	cmp	r1, #0
    5c94:	e2820030 	add	r0, r2, #48	; 0x30
    5c98:	059f68d0 	ldreq	r6, [pc, #2256]	; 6570 <bcm283x_sd_init+0xfc0>
    5c9c:	0a000002 	beq	5cac <bcm283x_sd_init+0x6fc>
    5ca0:	ea00015a 	b	6210 <bcm283x_sd_init+0xc60>
    5ca4:	e2566001 	subs	r6, r6, #1
    5ca8:	3a000142 	bcc	61b8 <bcm283x_sd_init+0xc08>
    5cac:	e3a00000 	mov	r0, #0
    5cb0:	eb001c9e 	bl	cf30 <usleep>
    5cb4:	e59a3000 	ldr	r3, [sl]
    5cb8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5cbc:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5cc0:	e59f28a4 	ldr	r2, [pc, #2212]	; 656c <bcm283x_sd_init+0xfbc>
    5cc4:	e0022001 	and	r2, r2, r1
    5cc8:	e3520000 	cmp	r2, #0
    5ccc:	e2833030 	add	r3, r3, #48	; 0x30
    5cd0:	0afffff3 	beq	5ca4 <bcm283x_sd_init+0x6f4>
    5cd4:	e3560000 	cmp	r6, #0
    5cd8:	e1a02003 	mov	r2, r3
    5cdc:	e1a00003 	mov	r0, r3
    5ce0:	e5931000 	ldr	r1, [r3]
    5ce4:	0a000135 	beq	61c0 <bcm283x_sd_init+0xc10>
    5ce8:	e3110811 	tst	r1, #1114112	; 0x110000
    5cec:	1a000149 	bne	6218 <bcm283x_sd_init+0xc68>
    5cf0:	e59f286c 	ldr	r2, [pc, #2156]	; 6564 <bcm283x_sd_init+0xfb4>
    5cf4:	e0022001 	and	r2, r2, r1
    5cf8:	e3520000 	cmp	r2, #0
    5cfc:	15801000 	strne	r1, [r0]
    5d00:	13e02001 	mvnne	r2, #1
    5d04:	1a00012f 	bne	61c8 <bcm283x_sd_init+0xc18>
    5d08:	e3a02001 	mov	r2, #1
    5d0c:	e5802000 	str	r2, [r0]
    5d10:	e59a2000 	ldr	r2, [sl]
    5d14:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5d18:	e5921010 	ldr	r1, [r2, #16]
    5d1c:	e282c024 	add	ip, r2, #36	; 0x24
    5d20:	e592101c 	ldr	r1, [r2, #28]
    5d24:	e5921018 	ldr	r1, [r2, #24]
    5d28:	e5921014 	ldr	r1, [r2, #20]
    5d2c:	e2822030 	add	r2, r2, #48	; 0x30
    5d30:	e3a01000 	mov	r1, #0
    5d34:	e5841000 	str	r1, [r4]
    5d38:	e59c1000 	ldr	r1, [ip]
    5d3c:	e3110001 	tst	r1, #1
    5d40:	0a000017 	beq	5da4 <bcm283x_sd_init+0x7f4>
    5d44:	e5920000 	ldr	r0, [r2]
    5d48:	e59f1814 	ldr	r1, [pc, #2068]	; 6564 <bcm283x_sd_init+0xfb4>
    5d4c:	e0011000 	and	r1, r1, r0
    5d50:	e3510000 	cmp	r1, #0
    5d54:	1a000012 	bne	5da4 <bcm283x_sd_init+0x7f4>
    5d58:	e59f6808 	ldr	r6, [pc, #2056]	; 6568 <bcm283x_sd_init+0xfb8>
    5d5c:	ea000005 	b	5d78 <bcm283x_sd_init+0x7c8>
    5d60:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    5d64:	e0033001 	and	r3, r3, r1
    5d68:	e3530000 	cmp	r3, #0
    5d6c:	1a000009 	bne	5d98 <bcm283x_sd_init+0x7e8>
    5d70:	e2566001 	subs	r6, r6, #1
    5d74:	3a000119 	bcc	61e0 <bcm283x_sd_init+0xc30>
    5d78:	e3a00000 	mov	r0, #0
    5d7c:	eb001c6b 	bl	cf30 <usleep>
    5d80:	e59a2000 	ldr	r2, [sl]
    5d84:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5d88:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    5d8c:	e3110001 	tst	r1, #1
    5d90:	e59f37cc 	ldr	r3, [pc, #1996]	; 6564 <bcm283x_sd_init+0xfb4>
    5d94:	1afffff1 	bne	5d60 <bcm283x_sd_init+0x7b0>
    5d98:	e3560000 	cmp	r6, #0
    5d9c:	0a00010f 	beq	61e0 <bcm283x_sd_init+0xc30>
    5da0:	e2822030 	add	r2, r2, #48	; 0x30
    5da4:	e5920000 	ldr	r0, [r2]
    5da8:	e59f17b4 	ldr	r1, [pc, #1972]	; 6564 <bcm283x_sd_init+0xfb4>
    5dac:	e0011000 	and	r1, r1, r0
    5db0:	e3510000 	cmp	r1, #0
    5db4:	1a000109 	bne	61e0 <bcm283x_sd_init+0xc30>
    5db8:	e5920000 	ldr	r0, [r2]
    5dbc:	e5820000 	str	r0, [r2]
    5dc0:	e59a2000 	ldr	r2, [sl]
    5dc4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5dc8:	e5821008 	str	r1, [r2, #8]
    5dcc:	e59a2000 	ldr	r2, [sl]
    5dd0:	e59f17c0 	ldr	r1, [pc, #1984]	; 6598 <bcm283x_sd_init+0xfe8>
    5dd4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5dd8:	e582100c 	str	r1, [r2, #12]
    5ddc:	e59a2000 	ldr	r2, [sl]
    5de0:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5de4:	e5920030 	ldr	r0, [r2, #48]	; 0x30
    5de8:	e59f177c 	ldr	r1, [pc, #1916]	; 656c <bcm283x_sd_init+0xfbc>
    5dec:	e0011000 	and	r1, r1, r0
    5df0:	e3510000 	cmp	r1, #0
    5df4:	e2820030 	add	r0, r2, #48	; 0x30
    5df8:	059f6770 	ldreq	r6, [pc, #1904]	; 6570 <bcm283x_sd_init+0xfc0>
    5dfc:	0a000002 	beq	5e0c <bcm283x_sd_init+0x85c>
    5e00:	ea000100 	b	6208 <bcm283x_sd_init+0xc58>
    5e04:	e2566001 	subs	r6, r6, #1
    5e08:	3a0000e3 	bcc	619c <bcm283x_sd_init+0xbec>
    5e0c:	e3a00000 	mov	r0, #0
    5e10:	eb001c46 	bl	cf30 <usleep>
    5e14:	e59a3000 	ldr	r3, [sl]
    5e18:	e2833603 	add	r3, r3, #3145728	; 0x300000
    5e1c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    5e20:	e59f2744 	ldr	r2, [pc, #1860]	; 656c <bcm283x_sd_init+0xfbc>
    5e24:	e0022001 	and	r2, r2, r1
    5e28:	e3520000 	cmp	r2, #0
    5e2c:	e2833030 	add	r3, r3, #48	; 0x30
    5e30:	0afffff3 	beq	5e04 <bcm283x_sd_init+0x854>
    5e34:	e3560000 	cmp	r6, #0
    5e38:	e1a02003 	mov	r2, r3
    5e3c:	e1a00003 	mov	r0, r3
    5e40:	e5931000 	ldr	r1, [r3]
    5e44:	0a0000d6 	beq	61a4 <bcm283x_sd_init+0xbf4>
    5e48:	e3110811 	tst	r1, #1114112	; 0x110000
    5e4c:	1a0000f3 	bne	6220 <bcm283x_sd_init+0xc70>
    5e50:	e59f270c 	ldr	r2, [pc, #1804]	; 6564 <bcm283x_sd_init+0xfb4>
    5e54:	e0022001 	and	r2, r2, r1
    5e58:	e3520000 	cmp	r2, #0
    5e5c:	15801000 	strne	r1, [r0]
    5e60:	13e02001 	mvnne	r2, #1
    5e64:	1a0000d0 	bne	61ac <bcm283x_sd_init+0xbfc>
    5e68:	e3a02001 	mov	r2, #1
    5e6c:	e5802000 	str	r2, [r0]
    5e70:	e59a2000 	ldr	r2, [sl]
    5e74:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5e78:	e5928010 	ldr	r8, [r2, #16]
    5e7c:	e2082901 	and	r2, r8, #16384	; 0x4000
    5e80:	e2081a02 	and	r1, r8, #8192	; 0x2000
    5e84:	e1a02402 	lsl	r2, r2, #8
    5e88:	e1822301 	orr	r2, r2, r1, lsl #6
    5e8c:	e1a01988 	lsl	r1, r8, #19
    5e90:	e18229a1 	orr	r2, r2, r1, lsr #19
    5e94:	e2080902 	and	r0, r8, #32768	; 0x8000
    5e98:	e59f16e8 	ldr	r1, [pc, #1768]	; 6588 <bcm283x_sd_init+0xfd8>
    5e9c:	e1822400 	orr	r2, r2, r0, lsl #8
    5ea0:	e0011002 	and	r1, r1, r2
    5ea4:	e1a08828 	lsr	r8, r8, #16
    5ea8:	e5841000 	str	r1, [r4]
    5eac:	e1a08808 	lsl	r8, r8, #16
    5eb0:	e5858000 	str	r8, [r5]
    5eb4:	e5942000 	ldr	r2, [r4]
    5eb8:	e3520000 	cmp	r2, #0
    5ebc:	1afffea0 	bne	5944 <bcm283x_sd_init+0x394>
    5ec0:	e5951000 	ldr	r1, [r5]
    5ec4:	e59f06d0 	ldr	r0, [pc, #1744]	; 659c <bcm283x_sd_init+0xfec>
    5ec8:	ebfffc7d 	bl	50c4 <sd_cmd>
    5ecc:	e5942000 	ldr	r2, [r4]
    5ed0:	e3520000 	cmp	r2, #0
    5ed4:	1afffe9a 	bne	5944 <bcm283x_sd_init+0x394>
    5ed8:	e59f06c0 	ldr	r0, [pc, #1728]	; 65a0 <bcm283x_sd_init+0xff0>
    5edc:	ebfffc01 	bl	4ee8 <sd_clk>
    5ee0:	e2508000 	subs	r8, r0, #0
    5ee4:	1afffe2a 	bne	5794 <bcm283x_sd_init+0x1e4>
    5ee8:	e59a2000 	ldr	r2, [sl]
    5eec:	e2820603 	add	r0, r2, #3145728	; 0x300000
    5ef0:	e5902024 	ldr	r2, [r0, #36]	; 0x24
    5ef4:	e3120002 	tst	r2, #2
    5ef8:	0a000017 	beq	5f5c <bcm283x_sd_init+0x9ac>
    5efc:	e5901030 	ldr	r1, [r0, #48]	; 0x30
    5f00:	e59f265c 	ldr	r2, [pc, #1628]	; 6564 <bcm283x_sd_init+0xfb4>
    5f04:	e0022001 	and	r2, r2, r1
    5f08:	e3520000 	cmp	r2, #0
    5f0c:	e2802030 	add	r2, r0, #48	; 0x30
    5f10:	1a000012 	bne	5f60 <bcm283x_sd_init+0x9b0>
    5f14:	e59f664c 	ldr	r6, [pc, #1612]	; 6568 <bcm283x_sd_init+0xfb8>
    5f18:	ea000005 	b	5f34 <bcm283x_sd_init+0x984>
    5f1c:	e5902030 	ldr	r2, [r0, #48]	; 0x30
    5f20:	e0033002 	and	r3, r3, r2
    5f24:	e3530000 	cmp	r3, #0
    5f28:	1a000009 	bne	5f54 <bcm283x_sd_init+0x9a4>
    5f2c:	e2566001 	subs	r6, r6, #1
    5f30:	3a00007f 	bcc	6134 <bcm283x_sd_init+0xb84>
    5f34:	e3a00000 	mov	r0, #0
    5f38:	eb001bfc 	bl	cf30 <usleep>
    5f3c:	e59a0000 	ldr	r0, [sl]
    5f40:	e2800603 	add	r0, r0, #3145728	; 0x300000
    5f44:	e5902024 	ldr	r2, [r0, #36]	; 0x24
    5f48:	e3120002 	tst	r2, #2
    5f4c:	e59f3610 	ldr	r3, [pc, #1552]	; 6564 <bcm283x_sd_init+0xfb4>
    5f50:	1afffff1 	bne	5f1c <bcm283x_sd_init+0x96c>
    5f54:	e3560000 	cmp	r6, #0
    5f58:	0a000075 	beq	6134 <bcm283x_sd_init+0xb84>
    5f5c:	e2802030 	add	r2, r0, #48	; 0x30
    5f60:	e5921000 	ldr	r1, [r2]
    5f64:	e59f25f8 	ldr	r2, [pc, #1528]	; 6564 <bcm283x_sd_init+0xfb4>
    5f68:	e0022001 	and	r2, r2, r1
    5f6c:	e3520000 	cmp	r2, #0
    5f70:	1a00006f 	bne	6134 <bcm283x_sd_init+0xb84>
    5f74:	e59f1628 	ldr	r1, [pc, #1576]	; 65a4 <bcm283x_sd_init+0xff4>
    5f78:	e5801004 	str	r1, [r0, #4]
    5f7c:	e5842000 	str	r2, [r4]
    5f80:	e59a1000 	ldr	r1, [sl]
    5f84:	e5956000 	ldr	r6, [r5]
    5f88:	e5957000 	ldr	r7, [r5]
    5f8c:	e5842000 	str	r2, [r4]
    5f90:	e2812603 	add	r2, r1, #3145728	; 0x300000
    5f94:	e59f05e0 	ldr	r0, [pc, #1504]	; 657c <bcm283x_sd_init+0xfcc>
    5f98:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    5f9c:	e3560000 	cmp	r6, #0
    5fa0:	11a06000 	movne	r6, r0
    5fa4:	03a06437 	moveq	r6, #922746880	; 0x37000000
    5fa8:	e3110001 	tst	r1, #1
    5fac:	0a000017 	beq	6010 <bcm283x_sd_init+0xa60>
    5fb0:	e5920030 	ldr	r0, [r2, #48]	; 0x30
    5fb4:	e59f15a8 	ldr	r1, [pc, #1448]	; 6564 <bcm283x_sd_init+0xfb4>
    5fb8:	e0011000 	and	r1, r1, r0
    5fbc:	e3510000 	cmp	r1, #0
    5fc0:	e2822030 	add	r2, r2, #48	; 0x30
    5fc4:	1a000012 	bne	6014 <bcm283x_sd_init+0xa64>
    5fc8:	e59f9598 	ldr	r9, [pc, #1432]	; 6568 <bcm283x_sd_init+0xfb8>
    5fcc:	ea000005 	b	5fe8 <bcm283x_sd_init+0xa38>
    5fd0:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    5fd4:	e0033001 	and	r3, r3, r1
    5fd8:	e3530000 	cmp	r3, #0
    5fdc:	1a000009 	bne	6008 <bcm283x_sd_init+0xa58>
    5fe0:	e2599001 	subs	r9, r9, #1
    5fe4:	3a00008f 	bcc	6228 <bcm283x_sd_init+0xc78>
    5fe8:	e3a00000 	mov	r0, #0
    5fec:	eb001bcf 	bl	cf30 <usleep>
    5ff0:	e59a2000 	ldr	r2, [sl]
    5ff4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    5ff8:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    5ffc:	e3110001 	tst	r1, #1
    6000:	e59f355c 	ldr	r3, [pc, #1372]	; 6564 <bcm283x_sd_init+0xfb4>
    6004:	1afffff1 	bne	5fd0 <bcm283x_sd_init+0xa20>
    6008:	e3590000 	cmp	r9, #0
    600c:	0a000085 	beq	6228 <bcm283x_sd_init+0xc78>
    6010:	e2822030 	add	r2, r2, #48	; 0x30
    6014:	e5920000 	ldr	r0, [r2]
    6018:	e59f1544 	ldr	r1, [pc, #1348]	; 6564 <bcm283x_sd_init+0xfb4>
    601c:	e0011000 	and	r1, r1, r0
    6020:	e3510000 	cmp	r1, #0
    6024:	1a00007f 	bne	6228 <bcm283x_sd_init+0xc78>
    6028:	e5921000 	ldr	r1, [r2]
    602c:	e5821000 	str	r1, [r2]
    6030:	e59a2000 	ldr	r2, [sl]
    6034:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6038:	e5827008 	str	r7, [r2, #8]
    603c:	e59a2000 	ldr	r2, [sl]
    6040:	e3560437 	cmp	r6, #922746880	; 0x37000000
    6044:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6048:	e582600c 	str	r6, [r2, #12]
    604c:	0a000138 	beq	6534 <bcm283x_sd_init+0xf84>
    6050:	e59a1000 	ldr	r1, [sl]
    6054:	e2811603 	add	r1, r1, #3145728	; 0x300000
    6058:	e5910030 	ldr	r0, [r1, #48]	; 0x30
    605c:	e59f2508 	ldr	r2, [pc, #1288]	; 656c <bcm283x_sd_init+0xfbc>
    6060:	e0022000 	and	r2, r2, r0
    6064:	e3520000 	cmp	r2, #0
    6068:	e2812030 	add	r2, r1, #48	; 0x30
    606c:	1a00012e 	bne	652c <bcm283x_sd_init+0xf7c>
    6070:	e59f74f8 	ldr	r7, [pc, #1272]	; 6570 <bcm283x_sd_init+0xfc0>
    6074:	ea000001 	b	6080 <bcm283x_sd_init+0xad0>
    6078:	e2577001 	subs	r7, r7, #1
    607c:	3a000125 	bcc	6518 <bcm283x_sd_init+0xf68>
    6080:	e3a00000 	mov	r0, #0
    6084:	eb001ba9 	bl	cf30 <usleep>
    6088:	e59a2000 	ldr	r2, [sl]
    608c:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6090:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6094:	e59f34d0 	ldr	r3, [pc, #1232]	; 656c <bcm283x_sd_init+0xfbc>
    6098:	e0033001 	and	r3, r3, r1
    609c:	e3530000 	cmp	r3, #0
    60a0:	e2822030 	add	r2, r2, #48	; 0x30
    60a4:	0afffff3 	beq	6078 <bcm283x_sd_init+0xac8>
    60a8:	e3570000 	cmp	r7, #0
    60ac:	e5921000 	ldr	r1, [r2]
    60b0:	0a000119 	beq	651c <bcm283x_sd_init+0xf6c>
    60b4:	e3110811 	tst	r1, #1114112	; 0x110000
    60b8:	1a000117 	bne	651c <bcm283x_sd_init+0xf6c>
    60bc:	e59f04a0 	ldr	r0, [pc, #1184]	; 6564 <bcm283x_sd_init+0xfb4>
    60c0:	e0000001 	and	r0, r0, r1
    60c4:	e3500000 	cmp	r0, #0
    60c8:	15821000 	strne	r1, [r2]
    60cc:	13e02001 	mvnne	r2, #1
    60d0:	1a000113 	bne	6524 <bcm283x_sd_init+0xf74>
    60d4:	e3a01001 	mov	r1, #1
    60d8:	e5821000 	str	r1, [r2]
    60dc:	e59a2000 	ldr	r2, [sl]
    60e0:	e3560437 	cmp	r6, #922746880	; 0x37000000
    60e4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    60e8:	e5920010 	ldr	r0, [r2, #16]
    60ec:	0a00004f 	beq	6230 <bcm283x_sd_init+0xc80>
    60f0:	e59f1484 	ldr	r1, [pc, #1156]	; 657c <bcm283x_sd_init+0xfcc>
    60f4:	e1560001 	cmp	r6, r1
    60f8:	159f1488 	ldrne	r1, [pc, #1160]	; 6588 <bcm283x_sd_init+0xfd8>
    60fc:	02001020 	andeq	r1, r0, #32
    6100:	10011000 	andne	r1, r1, r0
    6104:	e5950000 	ldr	r0, [r5]
    6108:	e2711001 	rsbs	r1, r1, #1
    610c:	33a01000 	movcc	r1, #0
    6110:	e3500000 	cmp	r0, #0
    6114:	03a01000 	moveq	r1, #0
    6118:	e3510000 	cmp	r1, #0
    611c:	0a000048 	beq	6244 <bcm283x_sd_init+0xc94>
    6120:	e3e02001 	mvn	r2, #1
    6124:	e5842000 	str	r2, [r4]
    6128:	ea000095 	b	6384 <bcm283x_sd_init+0xdd4>
    612c:	e5912000 	ldr	r2, [r1]
    6130:	e5812000 	str	r2, [r1]
    6134:	e3e08000 	mvn	r8, #0
    6138:	eafffd95 	b	5794 <bcm283x_sd_init+0x1e4>
    613c:	e5932000 	ldr	r2, [r3]
    6140:	e5832000 	str	r2, [r3]
    6144:	e3e03000 	mvn	r3, #0
    6148:	e5843000 	str	r3, [r4]
    614c:	eafffe0f 	b	5990 <bcm283x_sd_init+0x3e0>
    6150:	e1a02003 	mov	r2, r3
    6154:	e5930000 	ldr	r0, [r3]
    6158:	e5820000 	str	r0, [r2]
    615c:	e3e02000 	mvn	r2, #0
    6160:	e5842000 	str	r2, [r4]
    6164:	eafffdf3 	b	5938 <bcm283x_sd_init+0x388>
    6168:	e3e03001 	mvn	r3, #1
    616c:	e5843000 	str	r3, [r4]
    6170:	eafffe06 	b	5990 <bcm283x_sd_init+0x3e0>
    6174:	e3e03000 	mvn	r3, #0
    6178:	e5843000 	str	r3, [r4]
    617c:	eafffe03 	b	5990 <bcm283x_sd_init+0x3e0>
    6180:	e3e02000 	mvn	r2, #0
    6184:	e5842000 	str	r2, [r4]
    6188:	eafffdea 	b	5938 <bcm283x_sd_init+0x388>
    618c:	e5922030 	ldr	r2, [r2, #48]	; 0x30
    6190:	eafffe5a 	b	5b00 <bcm283x_sd_init+0x550>
    6194:	e5910030 	ldr	r0, [r1, #48]	; 0x30
    6198:	eafffdd9 	b	5904 <bcm283x_sd_init+0x354>
    619c:	e1a02003 	mov	r2, r3
    61a0:	e5931000 	ldr	r1, [r3]
    61a4:	e5821000 	str	r1, [r2]
    61a8:	e3e02000 	mvn	r2, #0
    61ac:	e5842000 	str	r2, [r4]
    61b0:	e3a08000 	mov	r8, #0
    61b4:	eaffff3d 	b	5eb0 <bcm283x_sd_init+0x900>
    61b8:	e1a02003 	mov	r2, r3
    61bc:	e5931000 	ldr	r1, [r3]
    61c0:	e5821000 	str	r1, [r2]
    61c4:	e3e02000 	mvn	r2, #0
    61c8:	e59ac000 	ldr	ip, [sl]
    61cc:	e28cc603 	add	ip, ip, #3145728	; 0x300000
    61d0:	e5842000 	str	r2, [r4]
    61d4:	e28c2030 	add	r2, ip, #48	; 0x30
    61d8:	e28cc024 	add	ip, ip, #36	; 0x24
    61dc:	eafffed3 	b	5d30 <bcm283x_sd_init+0x780>
    61e0:	e3e02000 	mvn	r2, #0
    61e4:	e5842000 	str	r2, [r4]
    61e8:	eaffff30 	b	5eb0 <bcm283x_sd_init+0x900>
    61ec:	e59a2000 	ldr	r2, [sl]
    61f0:	e2822603 	add	r2, r2, #3145728	; 0x300000
    61f4:	e282c024 	add	ip, r2, #36	; 0x24
    61f8:	e2822030 	add	r2, r2, #48	; 0x30
    61fc:	e3e01000 	mvn	r1, #0
    6200:	e5841000 	str	r1, [r4]
    6204:	eafffec9 	b	5d30 <bcm283x_sd_init+0x780>
    6208:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    620c:	eaffff0d 	b	5e48 <bcm283x_sd_init+0x898>
    6210:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6214:	eafffeb3 	b	5ce8 <bcm283x_sd_init+0x738>
    6218:	e1a02000 	mov	r2, r0
    621c:	eaffffe7 	b	61c0 <bcm283x_sd_init+0xc10>
    6220:	e1a02000 	mov	r2, r0
    6224:	eaffffde 	b	61a4 <bcm283x_sd_init+0xbf4>
    6228:	e3e02000 	mvn	r2, #0
    622c:	e5842000 	str	r2, [r4]
    6230:	e5952000 	ldr	r2, [r5]
    6234:	e3520000 	cmp	r2, #0
    6238:	1affffb8 	bne	6120 <bcm283x_sd_init+0xb70>
    623c:	e59a2000 	ldr	r2, [sl]
    6240:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6244:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    6248:	e3110001 	tst	r1, #1
    624c:	0a000017 	beq	62b0 <bcm283x_sd_init+0xd00>
    6250:	e5920030 	ldr	r0, [r2, #48]	; 0x30
    6254:	e59f1308 	ldr	r1, [pc, #776]	; 6564 <bcm283x_sd_init+0xfb4>
    6258:	e0011000 	and	r1, r1, r0
    625c:	e3510000 	cmp	r1, #0
    6260:	e2822030 	add	r2, r2, #48	; 0x30
    6264:	059f62fc 	ldreq	r6, [pc, #764]	; 6568 <bcm283x_sd_init+0xfb8>
    6268:	0a000006 	beq	6288 <bcm283x_sd_init+0xcd8>
    626c:	ea000010 	b	62b4 <bcm283x_sd_init+0xd04>
    6270:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6274:	e0033001 	and	r3, r3, r1
    6278:	e3530000 	cmp	r3, #0
    627c:	1a000009 	bne	62a8 <bcm283x_sd_init+0xcf8>
    6280:	e2566001 	subs	r6, r6, #1
    6284:	3a0000a0 	bcc	650c <bcm283x_sd_init+0xf5c>
    6288:	e3a00000 	mov	r0, #0
    628c:	eb001b27 	bl	cf30 <usleep>
    6290:	e59a2000 	ldr	r2, [sl]
    6294:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6298:	e5921024 	ldr	r1, [r2, #36]	; 0x24
    629c:	e3110001 	tst	r1, #1
    62a0:	e59f32bc 	ldr	r3, [pc, #700]	; 6564 <bcm283x_sd_init+0xfb4>
    62a4:	1afffff1 	bne	6270 <bcm283x_sd_init+0xcc0>
    62a8:	e3560000 	cmp	r6, #0
    62ac:	0a000096 	beq	650c <bcm283x_sd_init+0xf5c>
    62b0:	e2822030 	add	r2, r2, #48	; 0x30
    62b4:	e5920000 	ldr	r0, [r2]
    62b8:	e59f12a4 	ldr	r1, [pc, #676]	; 6564 <bcm283x_sd_init+0xfb4>
    62bc:	e0011000 	and	r1, r1, r0
    62c0:	e3510000 	cmp	r1, #0
    62c4:	1a000090 	bne	650c <bcm283x_sd_init+0xf5c>
    62c8:	e5920000 	ldr	r0, [r2]
    62cc:	e5820000 	str	r0, [r2]
    62d0:	e59a2000 	ldr	r2, [sl]
    62d4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    62d8:	e5821008 	str	r1, [r2, #8]
    62dc:	e59a2000 	ldr	r2, [sl]
    62e0:	e59f12c0 	ldr	r1, [pc, #704]	; 65a8 <bcm283x_sd_init+0xff8>
    62e4:	e2822603 	add	r2, r2, #3145728	; 0x300000
    62e8:	e582100c 	str	r1, [r2, #12]
    62ec:	e59a1000 	ldr	r1, [sl]
    62f0:	e2811603 	add	r1, r1, #3145728	; 0x300000
    62f4:	e5910030 	ldr	r0, [r1, #48]	; 0x30
    62f8:	e59f226c 	ldr	r2, [pc, #620]	; 656c <bcm283x_sd_init+0xfbc>
    62fc:	e0022000 	and	r2, r2, r0
    6300:	e3520000 	cmp	r2, #0
    6304:	059f6264 	ldreq	r6, [pc, #612]	; 6570 <bcm283x_sd_init+0xfc0>
    6308:	e2812030 	add	r2, r1, #48	; 0x30
    630c:	0a000002 	beq	631c <bcm283x_sd_init+0xd6c>
    6310:	ea00007b 	b	6504 <bcm283x_sd_init+0xf54>
    6314:	e2566001 	subs	r6, r6, #1
    6318:	3a00005e 	bcc	6498 <bcm283x_sd_init+0xee8>
    631c:	e3a00000 	mov	r0, #0
    6320:	eb001b02 	bl	cf30 <usleep>
    6324:	e59a3000 	ldr	r3, [sl]
    6328:	e2833603 	add	r3, r3, #3145728	; 0x300000
    632c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6330:	e59f2234 	ldr	r2, [pc, #564]	; 656c <bcm283x_sd_init+0xfbc>
    6334:	e0022001 	and	r2, r2, r1
    6338:	e3520000 	cmp	r2, #0
    633c:	e2832030 	add	r2, r3, #48	; 0x30
    6340:	0afffff3 	beq	6314 <bcm283x_sd_init+0xd64>
    6344:	e3560000 	cmp	r6, #0
    6348:	e5921000 	ldr	r1, [r2]
    634c:	0a000052 	beq	649c <bcm283x_sd_init+0xeec>
    6350:	e3110811 	tst	r1, #1114112	; 0x110000
    6354:	1a000050 	bne	649c <bcm283x_sd_init+0xeec>
    6358:	e59f0204 	ldr	r0, [pc, #516]	; 6564 <bcm283x_sd_init+0xfb4>
    635c:	e0000001 	and	r0, r0, r1
    6360:	e3500000 	cmp	r0, #0
    6364:	15821000 	strne	r1, [r2]
    6368:	13e02001 	mvnne	r2, #1
    636c:	1a00004c 	bne	64a4 <bcm283x_sd_init+0xef4>
    6370:	e3a01001 	mov	r1, #1
    6374:	e5821000 	str	r1, [r2]
    6378:	e59a2000 	ldr	r2, [sl]
    637c:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6380:	e5922010 	ldr	r2, [r2, #16]
    6384:	e5942000 	ldr	r2, [r4]
    6388:	e3520000 	cmp	r2, #0
    638c:	0a000009 	beq	63b8 <bcm283x_sd_init+0xe08>
    6390:	e51b1030 	ldr	r1, [fp, #-48]	; 0x30
    6394:	e5913000 	ldr	r3, [r1]
    6398:	e3c33001 	bic	r3, r3, #1
    639c:	e5813000 	str	r3, [r1]
    63a0:	e51b203c 	ldr	r2, [fp, #-60]	; 0x3c
    63a4:	e5913000 	ldr	r3, [r1]
    63a8:	e1a02082 	lsl	r2, r2, #1
    63ac:	e1833fa2 	orr	r3, r3, r2, lsr #31
    63b0:	e5813000 	str	r3, [r1]
    63b4:	eafffcf6 	b	5794 <bcm283x_sd_init+0x1e4>
    63b8:	e59a2000 	ldr	r2, [sl]
    63bc:	e2822603 	add	r2, r2, #3145728	; 0x300000
    63c0:	e5920030 	ldr	r0, [r2, #48]	; 0x30
    63c4:	e59f11e0 	ldr	r1, [pc, #480]	; 65ac <bcm283x_sd_init+0xffc>
    63c8:	e0011000 	and	r1, r1, r0
    63cc:	e3510000 	cmp	r1, #0
    63d0:	e2821030 	add	r1, r2, #48	; 0x30
    63d4:	1a000048 	bne	64fc <bcm283x_sd_init+0xf4c>
    63d8:	e59f6190 	ldr	r6, [pc, #400]	; 6570 <bcm283x_sd_init+0xfc0>
    63dc:	ea000001 	b	63e8 <bcm283x_sd_init+0xe38>
    63e0:	e2566001 	subs	r6, r6, #1
    63e4:	3affff50 	bcc	612c <bcm283x_sd_init+0xb7c>
    63e8:	e3a00000 	mov	r0, #0
    63ec:	eb001acf 	bl	cf30 <usleep>
    63f0:	e59a1000 	ldr	r1, [sl]
    63f4:	e2811603 	add	r1, r1, #3145728	; 0x300000
    63f8:	e5912030 	ldr	r2, [r1, #48]	; 0x30
    63fc:	e59f31a8 	ldr	r3, [pc, #424]	; 65ac <bcm283x_sd_init+0xffc>
    6400:	e0033002 	and	r3, r3, r2
    6404:	e3530000 	cmp	r3, #0
    6408:	e2811030 	add	r1, r1, #48	; 0x30
    640c:	0afffff3 	beq	63e0 <bcm283x_sd_init+0xe30>
    6410:	e3560000 	cmp	r6, #0
    6414:	e5912000 	ldr	r2, [r1]
    6418:	0affff44 	beq	6130 <bcm283x_sd_init+0xb80>
    641c:	e3120811 	tst	r2, #1114112	; 0x110000
    6420:	1affff42 	bne	6130 <bcm283x_sd_init+0xb80>
    6424:	e59f0138 	ldr	r0, [pc, #312]	; 6564 <bcm283x_sd_init+0xfb4>
    6428:	e0000002 	and	r0, r0, r2
    642c:	e3500000 	cmp	r0, #0
    6430:	03a02020 	moveq	r2, #32
    6434:	03a06000 	moveq	r6, #0
    6438:	03a07000 	moveq	r7, #0
    643c:	05812000 	streq	r2, [r1]
    6440:	0a00000a 	beq	6470 <bcm283x_sd_init+0xec0>
    6444:	eaffff39 	b	6130 <bcm283x_sd_init+0xb80>
    6448:	e2960001 	adds	r0, r6, #1
    644c:	e2a71000 	adc	r1, r7, #0
    6450:	e3500002 	cmp	r0, #2
    6454:	e2d12000 	sbcs	r2, r1, #0
    6458:	e5933020 	ldr	r3, [r3, #32]
    645c:	e51b2030 	ldr	r2, [fp, #-48]	; 0x30
    6460:	e7823106 	str	r3, [r2, r6, lsl #2]
    6464:	aa000010 	bge	64ac <bcm283x_sd_init+0xefc>
    6468:	e1a06000 	mov	r6, r0
    646c:	e1a07001 	mov	r7, r1
    6470:	e59a3000 	ldr	r3, [sl]
    6474:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6478:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    647c:	e3120b02 	tst	r2, #2048	; 0x800
    6480:	1afffff0 	bne	6448 <bcm283x_sd_init+0xe98>
    6484:	e3a00ffa 	mov	r0, #1000	; 0x3e8
    6488:	eb001aa8 	bl	cf30 <usleep>
    648c:	e1a00006 	mov	r0, r6
    6490:	e1a01007 	mov	r1, r7
    6494:	eafffff3 	b	6468 <bcm283x_sd_init+0xeb8>
    6498:	e5921000 	ldr	r1, [r2]
    649c:	e5821000 	str	r1, [r2]
    64a0:	e3e02000 	mvn	r2, #0
    64a4:	e5842000 	str	r2, [r4]
    64a8:	eaffffb5 	b	6384 <bcm283x_sd_init+0xdd4>
    64ac:	e3510000 	cmp	r1, #0
    64b0:	03500002 	cmpeq	r0, #2
    64b4:	1affff1e 	bne	6134 <bcm283x_sd_init+0xb84>
    64b8:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    64bc:	e5932000 	ldr	r2, [r3]
    64c0:	e3120b01 	tst	r2, #1024	; 0x400
    64c4:	0affffb1 	beq	6390 <bcm283x_sd_init+0xde0>
    64c8:	e5951000 	ldr	r1, [r5]
    64cc:	e59f00dc 	ldr	r0, [pc, #220]	; 65b0 <bcm283x_sd_init+0x1000>
    64d0:	e3811002 	orr	r1, r1, #2
    64d4:	ebfffafa 	bl	50c4 <sd_cmd>
    64d8:	e5942000 	ldr	r2, [r4]
    64dc:	e3520000 	cmp	r2, #0
    64e0:	1afffd17 	bne	5944 <bcm283x_sd_init+0x394>
    64e4:	e59a3000 	ldr	r3, [sl]
    64e8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    64ec:	e5932028 	ldr	r2, [r3, #40]	; 0x28
    64f0:	e3822002 	orr	r2, r2, #2
    64f4:	e5832028 	str	r2, [r3, #40]	; 0x28
    64f8:	eaffffa4 	b	6390 <bcm283x_sd_init+0xde0>
    64fc:	e5922030 	ldr	r2, [r2, #48]	; 0x30
    6500:	eaffffc5 	b	641c <bcm283x_sd_init+0xe6c>
    6504:	e5911030 	ldr	r1, [r1, #48]	; 0x30
    6508:	eaffff90 	b	6350 <bcm283x_sd_init+0xda0>
    650c:	e3e02000 	mvn	r2, #0
    6510:	e5842000 	str	r2, [r4]
    6514:	eaffff9a 	b	6384 <bcm283x_sd_init+0xdd4>
    6518:	e5921000 	ldr	r1, [r2]
    651c:	e5821000 	str	r1, [r2]
    6520:	e3e02000 	mvn	r2, #0
    6524:	e5842000 	str	r2, [r4]
    6528:	eaffff40 	b	6230 <bcm283x_sd_init+0xc80>
    652c:	e5911030 	ldr	r1, [r1, #48]	; 0x30
    6530:	eafffedf 	b	60b4 <bcm283x_sd_init+0xb04>
    6534:	e59f0078 	ldr	r0, [pc, #120]	; 65b4 <bcm283x_sd_init+0x1004>
    6538:	eb001a7c 	bl	cf30 <usleep>
    653c:	eafffec3 	b	6050 <bcm283x_sd_init+0xaa0>
    6540:	00012418 	andeq	r2, r1, r8, lsl r4
    6544:	00047144 	andeq	r7, r4, r4, asr #2
    6548:	00000024 	andeq	r0, r0, r4, lsr #32
    654c:	00000050 	andeq	r0, r0, r0, asr r0
    6550:	00002710 	andeq	r2, r0, r0, lsl r7
    6554:	00061a80 	andeq	r1, r6, r0, lsl #21
    6558:	00000028 	andeq	r0, r0, r8, lsr #32
    655c:	0000002c 	andeq	r0, r0, ip, lsr #32
    6560:	00000034 	andeq	r0, r0, r4, lsr r0
    6564:	017e8000 	cmneq	lr, r0
    6568:	000f423f 	andeq	r4, pc, pc, lsr r2	; <UNPREDICTABLE>
    656c:	017e8001 	cmneq	lr, r1
    6570:	0000270f 	andeq	r2, r0, pc, lsl #14
    6574:	08020000 	stmdaeq	r2, {}	; <UNPREDICTABLE>
    6578:	000001aa 	andeq	r0, r0, sl, lsr #3
    657c:	37020000 	strcc	r0, [r2, -r0]
    6580:	51ff8000 	mvnspl	r8, r0
    6584:	29020000 	stmdbcs	r2, {}	; <UNPREDICTABLE>
    6588:	fff9c004 			; <UNDEFINED> instruction: 0xfff9c004
    658c:	00f98000 	rscseq	r8, r9, r0
    6590:	00ff8000 	rscseq	r8, pc, r0
    6594:	02010000 	andeq	r0, r1, #0
    6598:	03020000 	movweq	r0, #8192	; 0x2000
    659c:	07030000 	streq	r0, [r3, -r0]
    65a0:	00bebc20 	adcseq	fp, lr, r0, lsr #24
    65a4:	00010008 	andeq	r0, r1, r8
    65a8:	33220010 			; <UNDEFINED> instruction: 0x33220010
    65ac:	017e8020 	cmneq	lr, r0, lsr #32
    65b0:	86020000 	strhi	r0, [r2], -r0
    65b4:	000186a0 	andeq	r8, r1, r0, lsr #13

000065b8 <bcm283x_sd_read_sector>:
    65b8:	e1a0c00d 	mov	ip, sp
    65bc:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    65c0:	e24cb004 	sub	fp, ip, #4
    65c4:	e24dd00c 	sub	sp, sp, #12
    65c8:	e59f246c 	ldr	r2, [pc, #1132]	; 6a3c <bcm283x_sd_read_sector+0x484>
    65cc:	e08f2002 	add	r2, pc, r2
    65d0:	e5923204 	ldr	r3, [r2, #516]	; 0x204
    65d4:	e59f8464 	ldr	r8, [pc, #1124]	; 6a40 <bcm283x_sd_read_sector+0x488>
    65d8:	e3530000 	cmp	r3, #0
    65dc:	e1a07000 	mov	r7, r0
    65e0:	e1a05001 	mov	r5, r1
    65e4:	e08f8008 	add	r8, pc, r8
    65e8:	0a000102 	beq	69f8 <bcm283x_sd_read_sector+0x440>
    65ec:	e3a01000 	mov	r1, #0
    65f0:	e5820000 	str	r0, [r2]
    65f4:	e59f3448 	ldr	r3, [pc, #1096]	; 6a44 <bcm283x_sd_read_sector+0x48c>
    65f8:	e5821204 	str	r1, [r2, #516]	; 0x204
    65fc:	e7984003 	ldr	r4, [r8, r3]
    6600:	e5943000 	ldr	r3, [r4]
    6604:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6608:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    660c:	e3120002 	tst	r2, #2
    6610:	0a000017 	beq	6674 <bcm283x_sd_read_sector+0xbc>
    6614:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6618:	e59f2428 	ldr	r2, [pc, #1064]	; 6a48 <bcm283x_sd_read_sector+0x490>
    661c:	e0022001 	and	r2, r2, r1
    6620:	e3520000 	cmp	r2, #0
    6624:	e2832030 	add	r2, r3, #48	; 0x30
    6628:	1a000012 	bne	6678 <bcm283x_sd_read_sector+0xc0>
    662c:	e59f6418 	ldr	r6, [pc, #1048]	; 6a4c <bcm283x_sd_read_sector+0x494>
    6630:	ea000005 	b	664c <bcm283x_sd_read_sector+0x94>
    6634:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6638:	e0022001 	and	r2, r2, r1
    663c:	e3520000 	cmp	r2, #0
    6640:	1a000009 	bne	666c <bcm283x_sd_read_sector+0xb4>
    6644:	e2566001 	subs	r6, r6, #1
    6648:	3a0000e6 	bcc	69e8 <bcm283x_sd_read_sector+0x430>
    664c:	e3a00000 	mov	r0, #0
    6650:	eb001a36 	bl	cf30 <usleep>
    6654:	e5943000 	ldr	r3, [r4]
    6658:	e2833603 	add	r3, r3, #3145728	; 0x300000
    665c:	e5931024 	ldr	r1, [r3, #36]	; 0x24
    6660:	e3110002 	tst	r1, #2
    6664:	e59f23dc 	ldr	r2, [pc, #988]	; 6a48 <bcm283x_sd_read_sector+0x490>
    6668:	1afffff1 	bne	6634 <bcm283x_sd_read_sector+0x7c>
    666c:	e3560000 	cmp	r6, #0
    6670:	0a0000dc 	beq	69e8 <bcm283x_sd_read_sector+0x430>
    6674:	e2832030 	add	r2, r3, #48	; 0x30
    6678:	e5922000 	ldr	r2, [r2]
    667c:	e59f13c4 	ldr	r1, [pc, #964]	; 6a48 <bcm283x_sd_read_sector+0x490>
    6680:	e0011002 	and	r1, r1, r2
    6684:	e3510000 	cmp	r1, #0
    6688:	1a0000d6 	bne	69e8 <bcm283x_sd_read_sector+0x430>
    668c:	e59f03bc 	ldr	r0, [pc, #956]	; 6a50 <bcm283x_sd_read_sector+0x498>
    6690:	e59f23bc 	ldr	r2, [pc, #956]	; 6a54 <bcm283x_sd_read_sector+0x49c>
    6694:	e5830004 	str	r0, [r3, #4]
    6698:	e7983002 	ldr	r3, [r8, r2]
    669c:	e5933000 	ldr	r3, [r3]
    66a0:	e2132001 	ands	r2, r3, #1
    66a4:	0a000074 	beq	687c <bcm283x_sd_read_sector+0x2c4>
    66a8:	e59f33a8 	ldr	r3, [pc, #936]	; 6a58 <bcm283x_sd_read_sector+0x4a0>
    66ac:	e7986003 	ldr	r6, [r8, r3]
    66b0:	e5943000 	ldr	r3, [r4]
    66b4:	e5861000 	str	r1, [r6]
    66b8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    66bc:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    66c0:	e3120001 	tst	r2, #1
    66c4:	0a000017 	beq	6728 <bcm283x_sd_read_sector+0x170>
    66c8:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    66cc:	e59f2374 	ldr	r2, [pc, #884]	; 6a48 <bcm283x_sd_read_sector+0x490>
    66d0:	e0022001 	and	r2, r2, r1
    66d4:	e3520000 	cmp	r2, #0
    66d8:	e2833030 	add	r3, r3, #48	; 0x30
    66dc:	1a000012 	bne	672c <bcm283x_sd_read_sector+0x174>
    66e0:	e59f8364 	ldr	r8, [pc, #868]	; 6a4c <bcm283x_sd_read_sector+0x494>
    66e4:	ea000005 	b	6700 <bcm283x_sd_read_sector+0x148>
    66e8:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    66ec:	e0022001 	and	r2, r2, r1
    66f0:	e3520000 	cmp	r2, #0
    66f4:	1a000009 	bne	6720 <bcm283x_sd_read_sector+0x168>
    66f8:	e2588001 	subs	r8, r8, #1
    66fc:	3a0000cb 	bcc	6a30 <bcm283x_sd_read_sector+0x478>
    6700:	e3a00000 	mov	r0, #0
    6704:	eb001a09 	bl	cf30 <usleep>
    6708:	e5943000 	ldr	r3, [r4]
    670c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6710:	e5931024 	ldr	r1, [r3, #36]	; 0x24
    6714:	e3110001 	tst	r1, #1
    6718:	e59f2328 	ldr	r2, [pc, #808]	; 6a48 <bcm283x_sd_read_sector+0x490>
    671c:	1afffff1 	bne	66e8 <bcm283x_sd_read_sector+0x130>
    6720:	e3580000 	cmp	r8, #0
    6724:	0a0000c1 	beq	6a30 <bcm283x_sd_read_sector+0x478>
    6728:	e2833030 	add	r3, r3, #48	; 0x30
    672c:	e5931000 	ldr	r1, [r3]
    6730:	e59f2310 	ldr	r2, [pc, #784]	; 6a48 <bcm283x_sd_read_sector+0x490>
    6734:	e0022001 	and	r2, r2, r1
    6738:	e3520000 	cmp	r2, #0
    673c:	1a0000bb 	bne	6a30 <bcm283x_sd_read_sector+0x478>
    6740:	e5932000 	ldr	r2, [r3]
    6744:	e5832000 	str	r2, [r3]
    6748:	e5943000 	ldr	r3, [r4]
    674c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6750:	e5837008 	str	r7, [r3, #8]
    6754:	e5943000 	ldr	r3, [r4]
    6758:	e59f22fc 	ldr	r2, [pc, #764]	; 6a5c <bcm283x_sd_read_sector+0x4a4>
    675c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6760:	e583200c 	str	r2, [r3, #12]
    6764:	e5942000 	ldr	r2, [r4]
    6768:	e2822603 	add	r2, r2, #3145728	; 0x300000
    676c:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6770:	e59f32e8 	ldr	r3, [pc, #744]	; 6a60 <bcm283x_sd_read_sector+0x4a8>
    6774:	e0033001 	and	r3, r3, r1
    6778:	e3530000 	cmp	r3, #0
    677c:	059f72e0 	ldreq	r7, [pc, #736]	; 6a64 <bcm283x_sd_read_sector+0x4ac>
    6780:	e2823030 	add	r3, r2, #48	; 0x30
    6784:	0a000002 	beq	6794 <bcm283x_sd_read_sector+0x1dc>
    6788:	ea0000a4 	b	6a20 <bcm283x_sd_read_sector+0x468>
    678c:	e2577001 	subs	r7, r7, #1
    6790:	3a00009d 	bcc	6a0c <bcm283x_sd_read_sector+0x454>
    6794:	e3a00000 	mov	r0, #0
    6798:	eb0019e4 	bl	cf30 <usleep>
    679c:	e5943000 	ldr	r3, [r4]
    67a0:	e2833603 	add	r3, r3, #3145728	; 0x300000
    67a4:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    67a8:	e59f22b0 	ldr	r2, [pc, #688]	; 6a60 <bcm283x_sd_read_sector+0x4a8>
    67ac:	e0022001 	and	r2, r2, r1
    67b0:	e3520000 	cmp	r2, #0
    67b4:	e2833030 	add	r3, r3, #48	; 0x30
    67b8:	0afffff3 	beq	678c <bcm283x_sd_read_sector+0x1d4>
    67bc:	e3570000 	cmp	r7, #0
    67c0:	e5932000 	ldr	r2, [r3]
    67c4:	0a000091 	beq	6a10 <bcm283x_sd_read_sector+0x458>
    67c8:	e3120811 	tst	r2, #1114112	; 0x110000
    67cc:	1a00008f 	bne	6a10 <bcm283x_sd_read_sector+0x458>
    67d0:	e59f1270 	ldr	r1, [pc, #624]	; 6a48 <bcm283x_sd_read_sector+0x490>
    67d4:	e0011002 	and	r1, r1, r2
    67d8:	e3510000 	cmp	r1, #0
    67dc:	15832000 	strne	r2, [r3]
    67e0:	13e03001 	mvnne	r3, #1
    67e4:	1a00008b 	bne	6a18 <bcm283x_sd_read_sector+0x460>
    67e8:	e3a02001 	mov	r2, #1
    67ec:	e5832000 	str	r2, [r3]
    67f0:	e5943000 	ldr	r3, [r4]
    67f4:	e2833603 	add	r3, r3, #3145728	; 0x300000
    67f8:	e5933010 	ldr	r3, [r3, #16]
    67fc:	e5967000 	ldr	r7, [r6]
    6800:	e3570000 	cmp	r7, #0
    6804:	1a00007b 	bne	69f8 <bcm283x_sd_read_sector+0x440>
    6808:	e59f8258 	ldr	r8, [pc, #600]	; 6a68 <bcm283x_sd_read_sector+0x4b0>
    680c:	e08f8008 	add	r8, pc, r8
    6810:	e1a0a008 	mov	sl, r8
    6814:	e59f6250 	ldr	r6, [pc, #592]	; 6a6c <bcm283x_sd_read_sector+0x4b4>
    6818:	e50b8030 	str	r8, [fp, #-48]	; 0x30
    681c:	e08f6006 	add	r6, pc, r6
    6820:	e2889c02 	add	r9, r8, #512	; 0x200
    6824:	e5983204 	ldr	r3, [r8, #516]	; 0x204
    6828:	e3530001 	cmp	r3, #1
    682c:	0a000007 	beq	6850 <bcm283x_sd_read_sector+0x298>
    6830:	e5942000 	ldr	r2, [r4]
    6834:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6838:	e5923030 	ldr	r3, [r2, #48]	; 0x30
    683c:	e59f022c 	ldr	r0, [pc, #556]	; 6a70 <bcm283x_sd_read_sector+0x4b8>
    6840:	e0000003 	and	r0, r0, r3
    6844:	e3500000 	cmp	r0, #0
    6848:	1a000052 	bne	6998 <bcm283x_sd_read_sector+0x3e0>
    684c:	eb0019b7 	bl	cf30 <usleep>
    6850:	e59a3204 	ldr	r3, [sl, #516]	; 0x204
    6854:	e3530000 	cmp	r3, #0
    6858:	0afffff1 	beq	6824 <bcm283x_sd_read_sector+0x26c>
    685c:	e1a00005 	mov	r0, r5
    6860:	e28a1004 	add	r1, sl, #4
    6864:	e3a02c02 	mov	r2, #512	; 0x200
    6868:	eb001da2 	bl	def8 <memcpy>
    686c:	e1a00007 	mov	r0, r7
    6870:	e24bd028 	sub	sp, fp, #40	; 0x28
    6874:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    6878:	e12fff1e 	bx	lr
    687c:	e59f31d4 	ldr	r3, [pc, #468]	; 6a58 <bcm283x_sd_read_sector+0x4a0>
    6880:	e7986003 	ldr	r6, [r8, r3]
    6884:	e5943000 	ldr	r3, [r4]
    6888:	e5862000 	str	r2, [r6]
    688c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6890:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    6894:	e3120001 	tst	r2, #1
    6898:	e1a07487 	lsl	r7, r7, #9
    689c:	0a000017 	beq	6900 <bcm283x_sd_read_sector+0x348>
    68a0:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    68a4:	e59f219c 	ldr	r2, [pc, #412]	; 6a48 <bcm283x_sd_read_sector+0x490>
    68a8:	e0022001 	and	r2, r2, r1
    68ac:	e3520000 	cmp	r2, #0
    68b0:	e2833030 	add	r3, r3, #48	; 0x30
    68b4:	1a000012 	bne	6904 <bcm283x_sd_read_sector+0x34c>
    68b8:	e59f818c 	ldr	r8, [pc, #396]	; 6a4c <bcm283x_sd_read_sector+0x494>
    68bc:	ea000005 	b	68d8 <bcm283x_sd_read_sector+0x320>
    68c0:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    68c4:	e0022001 	and	r2, r2, r1
    68c8:	e3520000 	cmp	r2, #0
    68cc:	1a000009 	bne	68f8 <bcm283x_sd_read_sector+0x340>
    68d0:	e2588001 	subs	r8, r8, #1
    68d4:	3a000055 	bcc	6a30 <bcm283x_sd_read_sector+0x478>
    68d8:	e3a00000 	mov	r0, #0
    68dc:	eb001993 	bl	cf30 <usleep>
    68e0:	e5943000 	ldr	r3, [r4]
    68e4:	e2833603 	add	r3, r3, #3145728	; 0x300000
    68e8:	e5931024 	ldr	r1, [r3, #36]	; 0x24
    68ec:	e3110001 	tst	r1, #1
    68f0:	e59f2150 	ldr	r2, [pc, #336]	; 6a48 <bcm283x_sd_read_sector+0x490>
    68f4:	1afffff1 	bne	68c0 <bcm283x_sd_read_sector+0x308>
    68f8:	e3580000 	cmp	r8, #0
    68fc:	0a00004b 	beq	6a30 <bcm283x_sd_read_sector+0x478>
    6900:	e2833030 	add	r3, r3, #48	; 0x30
    6904:	e5931000 	ldr	r1, [r3]
    6908:	e59f2138 	ldr	r2, [pc, #312]	; 6a48 <bcm283x_sd_read_sector+0x490>
    690c:	e0022001 	and	r2, r2, r1
    6910:	e3520000 	cmp	r2, #0
    6914:	1a000045 	bne	6a30 <bcm283x_sd_read_sector+0x478>
    6918:	e5932000 	ldr	r2, [r3]
    691c:	e5832000 	str	r2, [r3]
    6920:	e5943000 	ldr	r3, [r4]
    6924:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6928:	e5837008 	str	r7, [r3, #8]
    692c:	e5943000 	ldr	r3, [r4]
    6930:	e59f2124 	ldr	r2, [pc, #292]	; 6a5c <bcm283x_sd_read_sector+0x4a4>
    6934:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6938:	e583200c 	str	r2, [r3, #12]
    693c:	e5942000 	ldr	r2, [r4]
    6940:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6944:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6948:	e59f3110 	ldr	r3, [pc, #272]	; 6a60 <bcm283x_sd_read_sector+0x4a8>
    694c:	e0033001 	and	r3, r3, r1
    6950:	e3530000 	cmp	r3, #0
    6954:	059f7108 	ldreq	r7, [pc, #264]	; 6a64 <bcm283x_sd_read_sector+0x4ac>
    6958:	e2823030 	add	r3, r2, #48	; 0x30
    695c:	0a000002 	beq	696c <bcm283x_sd_read_sector+0x3b4>
    6960:	ea00002e 	b	6a20 <bcm283x_sd_read_sector+0x468>
    6964:	e2577001 	subs	r7, r7, #1
    6968:	3a000027 	bcc	6a0c <bcm283x_sd_read_sector+0x454>
    696c:	e3a00000 	mov	r0, #0
    6970:	eb00196e 	bl	cf30 <usleep>
    6974:	e5943000 	ldr	r3, [r4]
    6978:	e2833603 	add	r3, r3, #3145728	; 0x300000
    697c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6980:	e59f20d8 	ldr	r2, [pc, #216]	; 6a60 <bcm283x_sd_read_sector+0x4a8>
    6984:	e0022001 	and	r2, r2, r1
    6988:	e3520000 	cmp	r2, #0
    698c:	e2833030 	add	r3, r3, #48	; 0x30
    6990:	0afffff3 	beq	6964 <bcm283x_sd_read_sector+0x3ac>
    6994:	eaffff88 	b	67bc <bcm283x_sd_read_sector+0x204>
    6998:	e5923030 	ldr	r3, [r2, #48]	; 0x30
    699c:	e3130811 	tst	r3, #1114112	; 0x110000
    69a0:	1a000020 	bne	6a28 <bcm283x_sd_read_sector+0x470>
    69a4:	e59f009c 	ldr	r0, [pc, #156]	; 6a48 <bcm283x_sd_read_sector+0x490>
    69a8:	e0000003 	and	r0, r0, r3
    69ac:	e3500000 	cmp	r0, #0
    69b0:	1a00001c 	bne	6a28 <bcm283x_sd_read_sector+0x470>
    69b4:	e3a00020 	mov	r0, #32
    69b8:	e5820030 	str	r0, [r2, #48]	; 0x30
    69bc:	e5940000 	ldr	r0, [r4]
    69c0:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    69c4:	e2800603 	add	r0, r0, #3145728	; 0x300000
    69c8:	e2800020 	add	r0, r0, #32
    69cc:	e5902000 	ldr	r2, [r0]
    69d0:	e5a32004 	str	r2, [r3, #4]!
    69d4:	e1530009 	cmp	r3, r9
    69d8:	1afffffb 	bne	69cc <bcm283x_sd_read_sector+0x414>
    69dc:	e3a03001 	mov	r3, #1
    69e0:	e5863204 	str	r3, [r6, #516]	; 0x204
    69e4:	eaffff99 	b	6850 <bcm283x_sd_read_sector+0x298>
    69e8:	e3e02000 	mvn	r2, #0
    69ec:	e59f3064 	ldr	r3, [pc, #100]	; 6a58 <bcm283x_sd_read_sector+0x4a0>
    69f0:	e7983003 	ldr	r3, [r8, r3]
    69f4:	e5832000 	str	r2, [r3]
    69f8:	e3e07000 	mvn	r7, #0
    69fc:	e1a00007 	mov	r0, r7
    6a00:	e24bd028 	sub	sp, fp, #40	; 0x28
    6a04:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    6a08:	e12fff1e 	bx	lr
    6a0c:	e5932000 	ldr	r2, [r3]
    6a10:	e5832000 	str	r2, [r3]
    6a14:	e3e03000 	mvn	r3, #0
    6a18:	e5863000 	str	r3, [r6]
    6a1c:	eaffff76 	b	67fc <bcm283x_sd_read_sector+0x244>
    6a20:	e5922030 	ldr	r2, [r2, #48]	; 0x30
    6a24:	eaffff67 	b	67c8 <bcm283x_sd_read_sector+0x210>
    6a28:	e5823030 	str	r3, [r2, #48]	; 0x30
    6a2c:	eaffff87 	b	6850 <bcm283x_sd_read_sector+0x298>
    6a30:	e3e03000 	mvn	r3, #0
    6a34:	e5863000 	str	r3, [r6]
    6a38:	eaffff6f 	b	67fc <bcm283x_sd_read_sector+0x244>
    6a3c:	0004615c 	andeq	r6, r4, ip, asr r1
    6a40:	00011414 	andeq	r1, r1, r4, lsl r4
    6a44:	00000024 	andeq	r0, r0, r4, lsr #32
    6a48:	017e8000 	cmneq	lr, r0
    6a4c:	000f423f 	andeq	r4, pc, pc, lsr r2	; <UNPREDICTABLE>
    6a50:	00010200 	andeq	r0, r1, r0, lsl #4
    6a54:	00000034 	andeq	r0, r0, r4, lsr r0
    6a58:	00000028 	andeq	r0, r0, r8, lsr #32
    6a5c:	11220010 			; <UNDEFINED> instruction: 0x11220010
    6a60:	017e8001 	cmneq	lr, r1
    6a64:	0000270f 	andeq	r2, r0, pc, lsl #14
    6a68:	00045f1c 	andeq	r5, r4, ip, lsl pc
    6a6c:	00045f0c 	andeq	r5, r4, ip, lsl #30
    6a70:	017e8020 	cmneq	lr, r0, lsr #32

00006a74 <bcm283x_sd_write_sector>:
    6a74:	e1a0c00d 	mov	ip, sp
    6a78:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    6a7c:	e59f6564 	ldr	r6, [pc, #1380]	; 6fe8 <bcm283x_sd_write_sector+0x574>
    6a80:	e59f3564 	ldr	r3, [pc, #1380]	; 6fec <bcm283x_sd_write_sector+0x578>
    6a84:	e08f6006 	add	r6, pc, r6
    6a88:	e7964003 	ldr	r4, [r6, r3]
    6a8c:	e5942000 	ldr	r2, [r4]
    6a90:	e2823603 	add	r3, r2, #3145728	; 0x300000
    6a94:	e593e024 	ldr	lr, [r3, #36]	; 0x24
    6a98:	e24cb004 	sub	fp, ip, #4
    6a9c:	e59fc54c 	ldr	ip, [pc, #1356]	; 6ff0 <bcm283x_sd_write_sector+0x57c>
    6aa0:	e00cc00e 	and	ip, ip, lr
    6aa4:	e35c0000 	cmp	ip, #0
    6aa8:	e1a07000 	mov	r7, r0
    6aac:	e1a05001 	mov	r5, r1
    6ab0:	02833030 	addeq	r3, r3, #48	; 0x30
    6ab4:	0a00001b 	beq	6b28 <bcm283x_sd_write_sector+0xb4>
    6ab8:	e5930030 	ldr	r0, [r3, #48]	; 0x30
    6abc:	e59f1530 	ldr	r1, [pc, #1328]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6ac0:	e0011000 	and	r1, r1, r0
    6ac4:	e3510000 	cmp	r1, #0
    6ac8:	e2833030 	add	r3, r3, #48	; 0x30
    6acc:	1a000015 	bne	6b28 <bcm283x_sd_write_sector+0xb4>
    6ad0:	e59f8520 	ldr	r8, [pc, #1312]	; 6ff8 <bcm283x_sd_write_sector+0x584>
    6ad4:	ea000005 	b	6af0 <bcm283x_sd_write_sector+0x7c>
    6ad8:	e5913030 	ldr	r3, [r1, #48]	; 0x30
    6adc:	e0022003 	and	r2, r2, r3
    6ae0:	e3520000 	cmp	r2, #0
    6ae4:	1a00000b 	bne	6b18 <bcm283x_sd_write_sector+0xa4>
    6ae8:	e2588001 	subs	r8, r8, #1
    6aec:	3a000129 	bcc	6f98 <bcm283x_sd_write_sector+0x524>
    6af0:	e3a00000 	mov	r0, #0
    6af4:	eb00190d 	bl	cf30 <usleep>
    6af8:	e5940000 	ldr	r0, [r4]
    6afc:	e2801603 	add	r1, r0, #3145728	; 0x300000
    6b00:	e591c024 	ldr	ip, [r1, #36]	; 0x24
    6b04:	e59f34e4 	ldr	r3, [pc, #1252]	; 6ff0 <bcm283x_sd_write_sector+0x57c>
    6b08:	e003300c 	and	r3, r3, ip
    6b0c:	e3530000 	cmp	r3, #0
    6b10:	e59f24dc 	ldr	r2, [pc, #1244]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6b14:	1affffef 	bne	6ad8 <bcm283x_sd_write_sector+0x64>
    6b18:	e3580000 	cmp	r8, #0
    6b1c:	e1a02000 	mov	r2, r0
    6b20:	0a00011c 	beq	6f98 <bcm283x_sd_write_sector+0x524>
    6b24:	e2813030 	add	r3, r1, #48	; 0x30
    6b28:	e5933000 	ldr	r3, [r3]
    6b2c:	e59f14c0 	ldr	r1, [pc, #1216]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6b30:	e0011003 	and	r1, r1, r3
    6b34:	e3510000 	cmp	r1, #0
    6b38:	1a000116 	bne	6f98 <bcm283x_sd_write_sector+0x524>
    6b3c:	e59f34b8 	ldr	r3, [pc, #1208]	; 6ffc <bcm283x_sd_write_sector+0x588>
    6b40:	e2820603 	add	r0, r2, #3145728	; 0x300000
    6b44:	e5803004 	str	r3, [r0, #4]
    6b48:	e59f34b0 	ldr	r3, [pc, #1200]	; 7000 <bcm283x_sd_write_sector+0x58c>
    6b4c:	e7963003 	ldr	r3, [r6, r3]
    6b50:	e5933000 	ldr	r3, [r3]
    6b54:	e2132001 	ands	r2, r3, #1
    6b58:	0a0000a6 	beq	6df8 <bcm283x_sd_write_sector+0x384>
    6b5c:	e59f34a0 	ldr	r3, [pc, #1184]	; 7004 <bcm283x_sd_write_sector+0x590>
    6b60:	e7966003 	ldr	r6, [r6, r3]
    6b64:	e5943000 	ldr	r3, [r4]
    6b68:	e5861000 	str	r1, [r6]
    6b6c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6b70:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    6b74:	e3120001 	tst	r2, #1
    6b78:	0a000017 	beq	6bdc <bcm283x_sd_write_sector+0x168>
    6b7c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6b80:	e59f246c 	ldr	r2, [pc, #1132]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6b84:	e0022001 	and	r2, r2, r1
    6b88:	e3520000 	cmp	r2, #0
    6b8c:	e2833030 	add	r3, r3, #48	; 0x30
    6b90:	1a000012 	bne	6be0 <bcm283x_sd_write_sector+0x16c>
    6b94:	e59f845c 	ldr	r8, [pc, #1116]	; 6ff8 <bcm283x_sd_write_sector+0x584>
    6b98:	ea000005 	b	6bb4 <bcm283x_sd_write_sector+0x140>
    6b9c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6ba0:	e0022001 	and	r2, r2, r1
    6ba4:	e3520000 	cmp	r2, #0
    6ba8:	1a000009 	bne	6bd4 <bcm283x_sd_write_sector+0x160>
    6bac:	e2588001 	subs	r8, r8, #1
    6bb0:	3a0000fd 	bcc	6fac <bcm283x_sd_write_sector+0x538>
    6bb4:	e3a00000 	mov	r0, #0
    6bb8:	eb0018dc 	bl	cf30 <usleep>
    6bbc:	e5943000 	ldr	r3, [r4]
    6bc0:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6bc4:	e5931024 	ldr	r1, [r3, #36]	; 0x24
    6bc8:	e3110001 	tst	r1, #1
    6bcc:	e59f2420 	ldr	r2, [pc, #1056]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6bd0:	1afffff1 	bne	6b9c <bcm283x_sd_write_sector+0x128>
    6bd4:	e3580000 	cmp	r8, #0
    6bd8:	0a0000f3 	beq	6fac <bcm283x_sd_write_sector+0x538>
    6bdc:	e2833030 	add	r3, r3, #48	; 0x30
    6be0:	e5931000 	ldr	r1, [r3]
    6be4:	e59f2408 	ldr	r2, [pc, #1032]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6be8:	e0022001 	and	r2, r2, r1
    6bec:	e3520000 	cmp	r2, #0
    6bf0:	1a0000ed 	bne	6fac <bcm283x_sd_write_sector+0x538>
    6bf4:	e5932000 	ldr	r2, [r3]
    6bf8:	e5832000 	str	r2, [r3]
    6bfc:	e5943000 	ldr	r3, [r4]
    6c00:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6c04:	e5837008 	str	r7, [r3, #8]
    6c08:	e5943000 	ldr	r3, [r4]
    6c0c:	e59f23f4 	ldr	r2, [pc, #1012]	; 7008 <bcm283x_sd_write_sector+0x594>
    6c10:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6c14:	e583200c 	str	r2, [r3, #12]
    6c18:	e5942000 	ldr	r2, [r4]
    6c1c:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6c20:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6c24:	e59f33e0 	ldr	r3, [pc, #992]	; 700c <bcm283x_sd_write_sector+0x598>
    6c28:	e0033001 	and	r3, r3, r1
    6c2c:	e3530000 	cmp	r3, #0
    6c30:	059f73d8 	ldreq	r7, [pc, #984]	; 7010 <bcm283x_sd_write_sector+0x59c>
    6c34:	e2823030 	add	r3, r2, #48	; 0x30
    6c38:	0a000002 	beq	6c48 <bcm283x_sd_write_sector+0x1d4>
    6c3c:	ea0000e5 	b	6fd8 <bcm283x_sd_write_sector+0x564>
    6c40:	e2577001 	subs	r7, r7, #1
    6c44:	3a0000ce 	bcc	6f84 <bcm283x_sd_write_sector+0x510>
    6c48:	e3a00000 	mov	r0, #0
    6c4c:	eb0018b7 	bl	cf30 <usleep>
    6c50:	e5943000 	ldr	r3, [r4]
    6c54:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6c58:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6c5c:	e59f23a8 	ldr	r2, [pc, #936]	; 700c <bcm283x_sd_write_sector+0x598>
    6c60:	e0022001 	and	r2, r2, r1
    6c64:	e3520000 	cmp	r2, #0
    6c68:	e2833030 	add	r3, r3, #48	; 0x30
    6c6c:	0afffff3 	beq	6c40 <bcm283x_sd_write_sector+0x1cc>
    6c70:	e3570000 	cmp	r7, #0
    6c74:	e5931000 	ldr	r1, [r3]
    6c78:	0a0000c2 	beq	6f88 <bcm283x_sd_write_sector+0x514>
    6c7c:	e3110811 	tst	r1, #1114112	; 0x110000
    6c80:	1a0000c0 	bne	6f88 <bcm283x_sd_write_sector+0x514>
    6c84:	e59f2368 	ldr	r2, [pc, #872]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6c88:	e0022001 	and	r2, r2, r1
    6c8c:	e3520000 	cmp	r2, #0
    6c90:	15831000 	strne	r1, [r3]
    6c94:	13e03001 	mvnne	r3, #1
    6c98:	1a0000bc 	bne	6f90 <bcm283x_sd_write_sector+0x51c>
    6c9c:	e3a02001 	mov	r2, #1
    6ca0:	e5832000 	str	r2, [r3]
    6ca4:	e5943000 	ldr	r3, [r4]
    6ca8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6cac:	e5933010 	ldr	r3, [r3, #16]
    6cb0:	e5963000 	ldr	r3, [r6]
    6cb4:	e3530000 	cmp	r3, #0
    6cb8:	1a0000a8 	bne	6f60 <bcm283x_sd_write_sector+0x4ec>
    6cbc:	e5942000 	ldr	r2, [r4]
    6cc0:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6cc4:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6cc8:	e59f3344 	ldr	r3, [pc, #836]	; 7014 <bcm283x_sd_write_sector+0x5a0>
    6ccc:	e0033001 	and	r3, r3, r1
    6cd0:	e3530000 	cmp	r3, #0
    6cd4:	e2823030 	add	r3, r2, #48	; 0x30
    6cd8:	1a0000b8 	bne	6fc0 <bcm283x_sd_write_sector+0x54c>
    6cdc:	e59f732c 	ldr	r7, [pc, #812]	; 7010 <bcm283x_sd_write_sector+0x59c>
    6ce0:	ea000001 	b	6cec <bcm283x_sd_write_sector+0x278>
    6ce4:	e2577001 	subs	r7, r7, #1
    6ce8:	3a0000a0 	bcc	6f70 <bcm283x_sd_write_sector+0x4fc>
    6cec:	e3a00000 	mov	r0, #0
    6cf0:	eb00188e 	bl	cf30 <usleep>
    6cf4:	e5943000 	ldr	r3, [r4]
    6cf8:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6cfc:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6d00:	e59f230c 	ldr	r2, [pc, #780]	; 7014 <bcm283x_sd_write_sector+0x5a0>
    6d04:	e0022001 	and	r2, r2, r1
    6d08:	e3520000 	cmp	r2, #0
    6d0c:	e2833030 	add	r3, r3, #48	; 0x30
    6d10:	0afffff3 	beq	6ce4 <bcm283x_sd_write_sector+0x270>
    6d14:	e3570000 	cmp	r7, #0
    6d18:	e5931000 	ldr	r1, [r3]
    6d1c:	0a000094 	beq	6f74 <bcm283x_sd_write_sector+0x500>
    6d20:	e3110811 	tst	r1, #1114112	; 0x110000
    6d24:	1a000092 	bne	6f74 <bcm283x_sd_write_sector+0x500>
    6d28:	e59f22c4 	ldr	r2, [pc, #708]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6d2c:	e0022001 	and	r2, r2, r1
    6d30:	e3520000 	cmp	r2, #0
    6d34:	03a02010 	moveq	r2, #16
    6d38:	05832000 	streq	r2, [r3]
    6d3c:	02452004 	subeq	r2, r5, #4
    6d40:	02855f7f 	addeq	r5, r5, #508	; 0x1fc
    6d44:	1a00009a 	bne	6fb4 <bcm283x_sd_write_sector+0x540>
    6d48:	e5943000 	ldr	r3, [r4]
    6d4c:	e5b21004 	ldr	r1, [r2, #4]!
    6d50:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6d54:	e1520005 	cmp	r2, r5
    6d58:	e5831020 	str	r1, [r3, #32]
    6d5c:	1afffff9 	bne	6d48 <bcm283x_sd_write_sector+0x2d4>
    6d60:	e5942000 	ldr	r2, [r4]
    6d64:	e2822603 	add	r2, r2, #3145728	; 0x300000
    6d68:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6d6c:	e59f32a4 	ldr	r3, [pc, #676]	; 7018 <bcm283x_sd_write_sector+0x5a4>
    6d70:	e0033001 	and	r3, r3, r1
    6d74:	e3530000 	cmp	r3, #0
    6d78:	e2823030 	add	r3, r2, #48	; 0x30
    6d7c:	1a000097 	bne	6fe0 <bcm283x_sd_write_sector+0x56c>
    6d80:	e59f5288 	ldr	r5, [pc, #648]	; 7010 <bcm283x_sd_write_sector+0x59c>
    6d84:	ea000001 	b	6d90 <bcm283x_sd_write_sector+0x31c>
    6d88:	e2555001 	subs	r5, r5, #1
    6d8c:	3a000077 	bcc	6f70 <bcm283x_sd_write_sector+0x4fc>
    6d90:	e3a00000 	mov	r0, #0
    6d94:	eb001865 	bl	cf30 <usleep>
    6d98:	e5943000 	ldr	r3, [r4]
    6d9c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6da0:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6da4:	e59f226c 	ldr	r2, [pc, #620]	; 7018 <bcm283x_sd_write_sector+0x5a4>
    6da8:	e0022001 	and	r2, r2, r1
    6dac:	e3520000 	cmp	r2, #0
    6db0:	e2833030 	add	r3, r3, #48	; 0x30
    6db4:	0afffff3 	beq	6d88 <bcm283x_sd_write_sector+0x314>
    6db8:	e3550000 	cmp	r5, #0
    6dbc:	e5931000 	ldr	r1, [r3]
    6dc0:	0a00006b 	beq	6f74 <bcm283x_sd_write_sector+0x500>
    6dc4:	e3110811 	tst	r1, #1114112	; 0x110000
    6dc8:	1a000069 	bne	6f74 <bcm283x_sd_write_sector+0x500>
    6dcc:	e59f2220 	ldr	r2, [pc, #544]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6dd0:	e0022001 	and	r2, r2, r1
    6dd4:	e3520000 	cmp	r2, #0
    6dd8:	1a000075 	bne	6fb4 <bcm283x_sd_write_sector+0x540>
    6ddc:	e3a02002 	mov	r2, #2
    6de0:	e5832000 	str	r2, [r3]
    6de4:	e5963000 	ldr	r3, [r6]
    6de8:	e3530000 	cmp	r3, #0
    6dec:	03e00000 	mvneq	r0, #0
    6df0:	0a00005b 	beq	6f64 <bcm283x_sd_write_sector+0x4f0>
    6df4:	ea000059 	b	6f60 <bcm283x_sd_write_sector+0x4ec>
    6df8:	e59f3204 	ldr	r3, [pc, #516]	; 7004 <bcm283x_sd_write_sector+0x590>
    6dfc:	e7966003 	ldr	r6, [r6, r3]
    6e00:	e5943000 	ldr	r3, [r4]
    6e04:	e5862000 	str	r2, [r6]
    6e08:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6e0c:	e5932024 	ldr	r2, [r3, #36]	; 0x24
    6e10:	e3120001 	tst	r2, #1
    6e14:	e1a07487 	lsl	r7, r7, #9
    6e18:	0a000017 	beq	6e7c <bcm283x_sd_write_sector+0x408>
    6e1c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6e20:	e59f21cc 	ldr	r2, [pc, #460]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6e24:	e0022001 	and	r2, r2, r1
    6e28:	e3520000 	cmp	r2, #0
    6e2c:	e2833030 	add	r3, r3, #48	; 0x30
    6e30:	1a000012 	bne	6e80 <bcm283x_sd_write_sector+0x40c>
    6e34:	e59f81bc 	ldr	r8, [pc, #444]	; 6ff8 <bcm283x_sd_write_sector+0x584>
    6e38:	ea000005 	b	6e54 <bcm283x_sd_write_sector+0x3e0>
    6e3c:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6e40:	e0022001 	and	r2, r2, r1
    6e44:	e3520000 	cmp	r2, #0
    6e48:	1a000009 	bne	6e74 <bcm283x_sd_write_sector+0x400>
    6e4c:	e2588001 	subs	r8, r8, #1
    6e50:	3a000055 	bcc	6fac <bcm283x_sd_write_sector+0x538>
    6e54:	e3a00000 	mov	r0, #0
    6e58:	eb001834 	bl	cf30 <usleep>
    6e5c:	e5943000 	ldr	r3, [r4]
    6e60:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6e64:	e5931024 	ldr	r1, [r3, #36]	; 0x24
    6e68:	e3110001 	tst	r1, #1
    6e6c:	e59f2180 	ldr	r2, [pc, #384]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6e70:	1afffff1 	bne	6e3c <bcm283x_sd_write_sector+0x3c8>
    6e74:	e3580000 	cmp	r8, #0
    6e78:	0a00004b 	beq	6fac <bcm283x_sd_write_sector+0x538>
    6e7c:	e2833030 	add	r3, r3, #48	; 0x30
    6e80:	e5931000 	ldr	r1, [r3]
    6e84:	e59f2168 	ldr	r2, [pc, #360]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6e88:	e0022001 	and	r2, r2, r1
    6e8c:	e3520000 	cmp	r2, #0
    6e90:	1a000045 	bne	6fac <bcm283x_sd_write_sector+0x538>
    6e94:	e5932000 	ldr	r2, [r3]
    6e98:	e5832000 	str	r2, [r3]
    6e9c:	e5943000 	ldr	r3, [r4]
    6ea0:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6ea4:	e5837008 	str	r7, [r3, #8]
    6ea8:	e5943000 	ldr	r3, [r4]
    6eac:	e59f2154 	ldr	r2, [pc, #340]	; 7008 <bcm283x_sd_write_sector+0x594>
    6eb0:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6eb4:	e583200c 	str	r2, [r3, #12]
    6eb8:	e5943000 	ldr	r3, [r4]
    6ebc:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6ec0:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6ec4:	e59f2140 	ldr	r2, [pc, #320]	; 700c <bcm283x_sd_write_sector+0x598>
    6ec8:	e0022001 	and	r2, r2, r1
    6ecc:	e3520000 	cmp	r2, #0
    6ed0:	059f7138 	ldreq	r7, [pc, #312]	; 7010 <bcm283x_sd_write_sector+0x59c>
    6ed4:	e2832030 	add	r2, r3, #48	; 0x30
    6ed8:	0a000002 	beq	6ee8 <bcm283x_sd_write_sector+0x474>
    6edc:	ea000039 	b	6fc8 <bcm283x_sd_write_sector+0x554>
    6ee0:	e2577001 	subs	r7, r7, #1
    6ee4:	3a000026 	bcc	6f84 <bcm283x_sd_write_sector+0x510>
    6ee8:	e3a00000 	mov	r0, #0
    6eec:	eb00180f 	bl	cf30 <usleep>
    6ef0:	e5943000 	ldr	r3, [r4]
    6ef4:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6ef8:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6efc:	e59f2108 	ldr	r2, [pc, #264]	; 700c <bcm283x_sd_write_sector+0x598>
    6f00:	e0022001 	and	r2, r2, r1
    6f04:	e3520000 	cmp	r2, #0
    6f08:	e2833030 	add	r3, r3, #48	; 0x30
    6f0c:	0afffff3 	beq	6ee0 <bcm283x_sd_write_sector+0x46c>
    6f10:	e3570000 	cmp	r7, #0
    6f14:	e1a02003 	mov	r2, r3
    6f18:	e5931000 	ldr	r1, [r3]
    6f1c:	0a000019 	beq	6f88 <bcm283x_sd_write_sector+0x514>
    6f20:	e3110811 	tst	r1, #1114112	; 0x110000
    6f24:	1a000029 	bne	6fd0 <bcm283x_sd_write_sector+0x55c>
    6f28:	e59f30c4 	ldr	r3, [pc, #196]	; 6ff4 <bcm283x_sd_write_sector+0x580>
    6f2c:	e0033001 	and	r3, r3, r1
    6f30:	e3530000 	cmp	r3, #0
    6f34:	15821000 	strne	r1, [r2]
    6f38:	13e03001 	mvnne	r3, #1
    6f3c:	1a000013 	bne	6f90 <bcm283x_sd_write_sector+0x51c>
    6f40:	e3a03001 	mov	r3, #1
    6f44:	e5823000 	str	r3, [r2]
    6f48:	e5943000 	ldr	r3, [r4]
    6f4c:	e2833603 	add	r3, r3, #3145728	; 0x300000
    6f50:	e5933010 	ldr	r3, [r3, #16]
    6f54:	e5963000 	ldr	r3, [r6]
    6f58:	e3530000 	cmp	r3, #0
    6f5c:	0affff56 	beq	6cbc <bcm283x_sd_write_sector+0x248>
    6f60:	e3a00000 	mov	r0, #0
    6f64:	e24bd024 	sub	sp, fp, #36	; 0x24
    6f68:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    6f6c:	e12fff1e 	bx	lr
    6f70:	e5931000 	ldr	r1, [r3]
    6f74:	e5831000 	str	r1, [r3]
    6f78:	e3e03000 	mvn	r3, #0
    6f7c:	e5863000 	str	r3, [r6]
    6f80:	eafffff6 	b	6f60 <bcm283x_sd_write_sector+0x4ec>
    6f84:	e5931000 	ldr	r1, [r3]
    6f88:	e5831000 	str	r1, [r3]
    6f8c:	e3e03000 	mvn	r3, #0
    6f90:	e5863000 	str	r3, [r6]
    6f94:	eaffffee 	b	6f54 <bcm283x_sd_write_sector+0x4e0>
    6f98:	e3e02000 	mvn	r2, #0
    6f9c:	e59f3060 	ldr	r3, [pc, #96]	; 7004 <bcm283x_sd_write_sector+0x590>
    6fa0:	e7963003 	ldr	r3, [r6, r3]
    6fa4:	e5832000 	str	r2, [r3]
    6fa8:	eaffffec 	b	6f60 <bcm283x_sd_write_sector+0x4ec>
    6fac:	e3e03000 	mvn	r3, #0
    6fb0:	eafffff6 	b	6f90 <bcm283x_sd_write_sector+0x51c>
    6fb4:	e5831000 	str	r1, [r3]
    6fb8:	e3e03001 	mvn	r3, #1
    6fbc:	eaffffee 	b	6f7c <bcm283x_sd_write_sector+0x508>
    6fc0:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6fc4:	eaffff55 	b	6d20 <bcm283x_sd_write_sector+0x2ac>
    6fc8:	e5931030 	ldr	r1, [r3, #48]	; 0x30
    6fcc:	eaffffd3 	b	6f20 <bcm283x_sd_write_sector+0x4ac>
    6fd0:	e1a03002 	mov	r3, r2
    6fd4:	eaffffeb 	b	6f88 <bcm283x_sd_write_sector+0x514>
    6fd8:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6fdc:	eaffff26 	b	6c7c <bcm283x_sd_write_sector+0x208>
    6fe0:	e5921030 	ldr	r1, [r2, #48]	; 0x30
    6fe4:	eaffff76 	b	6dc4 <bcm283x_sd_write_sector+0x350>
    6fe8:	00010f74 	andeq	r0, r1, r4, ror pc
    6fec:	00000024 	andeq	r0, r0, r4, lsr #32
    6ff0:	00000402 	andeq	r0, r0, r2, lsl #8
    6ff4:	017e8000 	cmneq	lr, r0
    6ff8:	000f423f 	andeq	r4, pc, pc, lsr r2	; <UNPREDICTABLE>
    6ffc:	00010200 	andeq	r0, r1, r0, lsl #4
    7000:	00000034 	andeq	r0, r0, r4, lsr r0
    7004:	00000028 	andeq	r0, r0, r8, lsr #32
    7008:	18220010 	stmdane	r2!, {r4}
    700c:	017e8001 	cmneq	lr, r1
    7010:	0000270f 	andeq	r2, r0, pc, lsl #14
    7014:	017e8010 	cmneq	lr, r0, lsl r0
    7018:	017e8002 	cmneq	lr, r2

0000701c <sd_dev_handle>:
    701c:	e1a0c00d 	mov	ip, sp
    7020:	e92dd800 	push	{fp, ip, lr, pc}
    7024:	e59f31a4 	ldr	r3, [pc, #420]	; 71d0 <sd_dev_handle+0x1b4>
    7028:	e59f21a4 	ldr	r2, [pc, #420]	; 71d4 <sd_dev_handle+0x1b8>
    702c:	e08f3003 	add	r3, pc, r3
    7030:	e7932002 	ldr	r2, [r3, r2]
    7034:	e5923000 	ldr	r3, [r2]
    7038:	e2833a05 	add	r3, r3, #20480	; 0x5000
    703c:	e5931034 	ldr	r1, [r3, #52]	; 0x34
    7040:	e3110802 	tst	r1, #131072	; 0x20000
    7044:	e24cb004 	sub	fp, ip, #4
    7048:	0a00000c 	beq	7080 <sd_dev_handle+0x64>
    704c:	e59f0184 	ldr	r0, [pc, #388]	; 71d8 <sd_dev_handle+0x1bc>
    7050:	e311002a 	tst	r1, #42	; 0x2a
    7054:	e08f0000 	add	r0, pc, r0
    7058:	e590c400 	ldr	ip, [r0, #1024]	; 0x400
    705c:	e5901408 	ldr	r1, [r0, #1032]	; 0x408
    7060:	0a000032 	beq	7130 <sd_dev_handle+0x114>
    7064:	e3510000 	cmp	r1, #0
    7068:	0a000047 	beq	718c <sd_dev_handle+0x170>
    706c:	e3e02000 	mvn	r2, #0
    7070:	e5832038 	str	r2, [r3, #56]	; 0x38
    7074:	e24bd00c 	sub	sp, fp, #12
    7078:	e89d6800 	ldm	sp, {fp, sp, lr}
    707c:	e12fff1e 	bx	lr
    7080:	e2110701 	ands	r0, r1, #262144	; 0x40000
    7084:	0a00004d 	beq	71c0 <sd_dev_handle+0x1a4>
    7088:	e59f014c 	ldr	r0, [pc, #332]	; 71dc <sd_dev_handle+0x1c0>
    708c:	e311000a 	tst	r1, #10
    7090:	e08f0000 	add	r0, pc, r0
    7094:	e590e404 	ldr	lr, [r0, #1028]	; 0x404
    7098:	e590140c 	ldr	r1, [r0, #1036]	; 0x40c
    709c:	1a000014 	bne	70f4 <sd_dev_handle+0xd8>
    70a0:	e3510000 	cmp	r1, #0
    70a4:	0a000014 	beq	70fc <sd_dev_handle+0xe0>
    70a8:	e24e1004 	sub	r1, lr, #4
    70ac:	e28ec03c 	add	ip, lr, #60	; 0x3c
    70b0:	ea000001 	b	70bc <sd_dev_handle+0xa0>
    70b4:	e5923000 	ldr	r3, [r2]
    70b8:	e2833a05 	add	r3, r3, #20480	; 0x5000
    70bc:	e5b10004 	ldr	r0, [r1, #4]!
    70c0:	e151000c 	cmp	r1, ip
    70c4:	e5830080 	str	r0, [r3, #128]	; 0x80
    70c8:	1afffff9 	bne	70b4 <sd_dev_handle+0x98>
    70cc:	e59f010c 	ldr	r0, [pc, #268]	; 71e0 <sd_dev_handle+0x1c4>
    70d0:	e08f0000 	add	r0, pc, r0
    70d4:	e590140c 	ldr	r1, [r0, #1036]	; 0x40c
    70d8:	e5923000 	ldr	r3, [r2]
    70dc:	e2411040 	sub	r1, r1, #64	; 0x40
    70e0:	e580140c 	str	r1, [r0, #1036]	; 0x40c
    70e4:	e28ee040 	add	lr, lr, #64	; 0x40
    70e8:	e2833a05 	add	r3, r3, #20480	; 0x5000
    70ec:	e580e404 	str	lr, [r0, #1028]	; 0x404
    70f0:	e5930034 	ldr	r0, [r3, #52]	; 0x34
    70f4:	e3510000 	cmp	r1, #0
    70f8:	1affffdb 	bne	706c <sd_dev_handle+0x50>
    70fc:	e3a01000 	mov	r1, #0
    7100:	e3a00001 	mov	r0, #1
    7104:	e5831008 	str	r1, [r3, #8]
    7108:	e5923000 	ldr	r3, [r2]
    710c:	e59f10d0 	ldr	r1, [pc, #208]	; 71e4 <sd_dev_handle+0x1c8>
    7110:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7114:	e583100c 	str	r1, [r3, #12]
    7118:	e59f10c8 	ldr	r1, [pc, #200]	; 71e8 <sd_dev_handle+0x1cc>
    711c:	e5923000 	ldr	r3, [r2]
    7120:	e08f1001 	add	r1, pc, r1
    7124:	e5810414 	str	r0, [r1, #1044]	; 0x414
    7128:	e2833a05 	add	r3, r3, #20480	; 0x5000
    712c:	eaffffce 	b	706c <sd_dev_handle+0x50>
    7130:	e3510000 	cmp	r1, #0
    7134:	0a000014 	beq	718c <sd_dev_handle+0x170>
    7138:	e24c1004 	sub	r1, ip, #4
    713c:	e28c003c 	add	r0, ip, #60	; 0x3c
    7140:	ea000001 	b	714c <sd_dev_handle+0x130>
    7144:	e5923000 	ldr	r3, [r2]
    7148:	e2833a05 	add	r3, r3, #20480	; 0x5000
    714c:	e5933080 	ldr	r3, [r3, #128]	; 0x80
    7150:	e5a13004 	str	r3, [r1, #4]!
    7154:	e1510000 	cmp	r1, r0
    7158:	1afffff9 	bne	7144 <sd_dev_handle+0x128>
    715c:	e59f0088 	ldr	r0, [pc, #136]	; 71ec <sd_dev_handle+0x1d0>
    7160:	e08f0000 	add	r0, pc, r0
    7164:	e5901408 	ldr	r1, [r0, #1032]	; 0x408
    7168:	e5923000 	ldr	r3, [r2]
    716c:	e2411040 	sub	r1, r1, #64	; 0x40
    7170:	e5801408 	str	r1, [r0, #1032]	; 0x408
    7174:	e28cc040 	add	ip, ip, #64	; 0x40
    7178:	e2833a05 	add	r3, r3, #20480	; 0x5000
    717c:	e3510000 	cmp	r1, #0
    7180:	e580c400 	str	ip, [r0, #1024]	; 0x400
    7184:	e5930034 	ldr	r0, [r3, #52]	; 0x34
    7188:	1affffb7 	bne	706c <sd_dev_handle+0x50>
    718c:	e3a01000 	mov	r1, #0
    7190:	e3a00001 	mov	r0, #1
    7194:	e5831008 	str	r1, [r3, #8]
    7198:	e5923000 	ldr	r3, [r2]
    719c:	e59f1040 	ldr	r1, [pc, #64]	; 71e4 <sd_dev_handle+0x1c8>
    71a0:	e2833a05 	add	r3, r3, #20480	; 0x5000
    71a4:	e583100c 	str	r1, [r3, #12]
    71a8:	e59f1040 	ldr	r1, [pc, #64]	; 71f0 <sd_dev_handle+0x1d4>
    71ac:	e5923000 	ldr	r3, [r2]
    71b0:	e08f1001 	add	r1, pc, r1
    71b4:	e5810410 	str	r0, [r1, #1040]	; 0x410
    71b8:	e2833a05 	add	r3, r3, #20480	; 0x5000
    71bc:	eaffffaa 	b	706c <sd_dev_handle+0x50>
    71c0:	eb00174e 	bl	cf00 <sleep>
    71c4:	e24bd00c 	sub	sp, fp, #12
    71c8:	e89d6800 	ldm	sp, {fp, sp, lr}
    71cc:	e12fff1e 	bx	lr
    71d0:	000109cc 	andeq	r0, r1, ip, asr #19
    71d4:	00000024 	andeq	r0, r0, r4, lsr #32
    71d8:	000458e0 	andeq	r5, r4, r0, ror #17
    71dc:	000458a4 	andeq	r5, r4, r4, lsr #17
    71e0:	00045864 	andeq	r5, r4, r4, ror #16
    71e4:	0000054c 	andeq	r0, r0, ip, asr #10
    71e8:	00045814 	andeq	r5, r4, r4, lsl r8
    71ec:	000457d4 	ldrdeq	r5, [r4], -r4
    71f0:	00045784 	andeq	r5, r4, r4, lsl #15

000071f4 <versatilepb_sd_init>:
    71f4:	e1a0c00d 	mov	ip, sp
    71f8:	e3a00000 	mov	r0, #0
    71fc:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    7200:	e24cb004 	sub	fp, ip, #4
    7204:	eb00164f 	bl	cb48 <mmio_map>
    7208:	e59f213c 	ldr	r2, [pc, #316]	; 734c <versatilepb_sd_init+0x158>
    720c:	e59f313c 	ldr	r3, [pc, #316]	; 7350 <versatilepb_sd_init+0x15c>
    7210:	e08f2002 	add	r2, pc, r2
    7214:	e59f5138 	ldr	r5, [pc, #312]	; 7354 <versatilepb_sd_init+0x160>
    7218:	e7924003 	ldr	r4, [r2, r3]
    721c:	e08f5005 	add	r5, pc, r5
    7220:	e5840000 	str	r0, [r4]
    7224:	e3a01000 	mov	r1, #0
    7228:	e59f2128 	ldr	r2, [pc, #296]	; 7358 <versatilepb_sd_init+0x164>
    722c:	e1a00005 	mov	r0, r5
    7230:	eb001b6c 	bl	dfe8 <memset>
    7234:	e3a02001 	mov	r2, #1
    7238:	e3a0e0bf 	mov	lr, #191	; 0xbf
    723c:	e3a0c0c6 	mov	ip, #198	; 0xc6
    7240:	e3a00000 	mov	r0, #0
    7244:	e3a01b01 	mov	r1, #1024	; 0x400
    7248:	e5943000 	ldr	r3, [r4]
    724c:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7250:	e5852410 	str	r2, [r5, #1040]	; 0x410
    7254:	e5852414 	str	r2, [r5, #1044]	; 0x414
    7258:	e583e000 	str	lr, [r3]
    725c:	e5943000 	ldr	r3, [r4]
    7260:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7264:	e583c004 	str	ip, [r3, #4]
    7268:	e5943000 	ldr	r3, [r4]
    726c:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7270:	e5830008 	str	r0, [r3, #8]
    7274:	e5943000 	ldr	r3, [r4]
    7278:	e2833a05 	add	r3, r3, #20480	; 0x5000
    727c:	e583100c 	str	r1, [r3, #12]
    7280:	e5943000 	ldr	r3, [r4]
    7284:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7288:	e5830008 	str	r0, [r3, #8]
    728c:	e5943000 	ldr	r3, [r4]
    7290:	e59f10c4 	ldr	r1, [pc, #196]	; 735c <versatilepb_sd_init+0x168>
    7294:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7298:	e583100c 	str	r1, [r3, #12]
    729c:	e5943000 	ldr	r3, [r4]
    72a0:	e2833a05 	add	r3, r3, #20480	; 0x5000
    72a4:	e5832008 	str	r2, [r3, #8]
    72a8:	e5943000 	ldr	r3, [r4]
    72ac:	e59f20ac 	ldr	r2, [pc, #172]	; 7360 <versatilepb_sd_init+0x16c>
    72b0:	e2833a05 	add	r3, r3, #20480	; 0x5000
    72b4:	e583200c 	str	r2, [r3, #12]
    72b8:	e5943000 	ldr	r3, [r4]
    72bc:	e3a0cc02 	mov	ip, #512	; 0x200
    72c0:	e2833a05 	add	r3, r3, #20480	; 0x5000
    72c4:	e5830008 	str	r0, [r3, #8]
    72c8:	e5943000 	ldr	r3, [r4]
    72cc:	e59f2090 	ldr	r2, [pc, #144]	; 7364 <versatilepb_sd_init+0x170>
    72d0:	e3a01e55 	mov	r1, #1360	; 0x550
    72d4:	e2833a05 	add	r3, r3, #20480	; 0x5000
    72d8:	e583200c 	str	r2, [r3, #12]
    72dc:	e3a02806 	mov	r2, #393216	; 0x60000
    72e0:	e5943000 	ldr	r3, [r4]
    72e4:	e59fe07c 	ldr	lr, [pc, #124]	; 7368 <versatilepb_sd_init+0x174>
    72e8:	e2833a05 	add	r3, r3, #20480	; 0x5000
    72ec:	e583e008 	str	lr, [r3, #8]
    72f0:	e5943000 	ldr	r3, [r4]
    72f4:	e59f5070 	ldr	r5, [pc, #112]	; 736c <versatilepb_sd_init+0x178>
    72f8:	e2833a05 	add	r3, r3, #20480	; 0x5000
    72fc:	e583500c 	str	r5, [r3, #12]
    7300:	e5943000 	ldr	r3, [r4]
    7304:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7308:	e583e008 	str	lr, [r3, #8]
    730c:	e5943000 	ldr	r3, [r4]
    7310:	e59fe058 	ldr	lr, [pc, #88]	; 7370 <versatilepb_sd_init+0x17c>
    7314:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7318:	e583e00c 	str	lr, [r3, #12]
    731c:	e5943000 	ldr	r3, [r4]
    7320:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7324:	e583c008 	str	ip, [r3, #8]
    7328:	e5943000 	ldr	r3, [r4]
    732c:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7330:	e583100c 	str	r1, [r3, #12]
    7334:	e5943000 	ldr	r3, [r4]
    7338:	e2833a05 	add	r3, r3, #20480	; 0x5000
    733c:	e583203c 	str	r2, [r3, #60]	; 0x3c
    7340:	e24bd014 	sub	sp, fp, #20
    7344:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    7348:	e12fff1e 	bx	lr
    734c:	000107e8 	andeq	r0, r1, r8, ror #15
    7350:	00000024 	andeq	r0, r0, r4, lsr #32
    7354:	00045718 	andeq	r5, r4, r8, lsl r7
    7358:	00000418 	andeq	r0, r0, r8, lsl r4
    735c:	00000577 	andeq	r0, r0, r7, ror r5
    7360:	00000469 	andeq	r0, r0, r9, ror #8
    7364:	000005c2 	andeq	r0, r0, r2, asr #11
    7368:	45670000 	strbmi	r0, [r7, #-0]!
    736c:	00000543 	andeq	r0, r0, r3, asr #10
    7370:	00000547 	andeq	r0, r0, r7, asr #10

00007374 <versatilepb_sd_read_sector>:
    7374:	e1a0c00d 	mov	ip, sp
    7378:	e59f30c4 	ldr	r3, [pc, #196]	; 7444 <versatilepb_sd_read_sector+0xd0>
    737c:	e08f3003 	add	r3, pc, r3
    7380:	e5932410 	ldr	r2, [r3, #1040]	; 0x410
    7384:	e3520000 	cmp	r2, #0
    7388:	e59f20b8 	ldr	r2, [pc, #184]	; 7448 <versatilepb_sd_read_sector+0xd4>
    738c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    7390:	e08f2002 	add	r2, pc, r2
    7394:	e24cb004 	sub	fp, ip, #4
    7398:	e1a05001 	mov	r5, r1
    739c:	0a000026 	beq	743c <versatilepb_sd_read_sector+0xc8>
    73a0:	e3a0cc02 	mov	ip, #512	; 0x200
    73a4:	e3a0e000 	mov	lr, #0
    73a8:	e3a01093 	mov	r1, #147	; 0x93
    73ac:	e583e410 	str	lr, [r3, #1040]	; 0x410
    73b0:	e5833400 	str	r3, [r3, #1024]	; 0x400
    73b4:	e583c408 	str	ip, [r3, #1032]	; 0x408
    73b8:	e1a04003 	mov	r4, r3
    73bc:	e59f3088 	ldr	r3, [pc, #136]	; 744c <versatilepb_sd_read_sector+0xd8>
    73c0:	e7923003 	ldr	r3, [r2, r3]
    73c4:	e5932000 	ldr	r2, [r3]
    73c8:	e24ee801 	sub	lr, lr, #65536	; 0x10000
    73cc:	e2822a05 	add	r2, r2, #20480	; 0x5000
    73d0:	e582e024 	str	lr, [r2, #36]	; 0x24
    73d4:	e5932000 	ldr	r2, [r3]
    73d8:	e2822a05 	add	r2, r2, #20480	; 0x5000
    73dc:	e582c028 	str	ip, [r2, #40]	; 0x28
    73e0:	e5932000 	ldr	r2, [r3]
    73e4:	e1a00480 	lsl	r0, r0, #9
    73e8:	e2822a05 	add	r2, r2, #20480	; 0x5000
    73ec:	e5820008 	str	r0, [r2, #8]
    73f0:	e5932000 	ldr	r2, [r3]
    73f4:	e59f0054 	ldr	r0, [pc, #84]	; 7450 <versatilepb_sd_read_sector+0xdc>
    73f8:	e2822a05 	add	r2, r2, #20480	; 0x5000
    73fc:	e582000c 	str	r0, [r2, #12]
    7400:	e5933000 	ldr	r3, [r3]
    7404:	e2833a05 	add	r3, r3, #20480	; 0x5000
    7408:	e583102c 	str	r1, [r3, #44]	; 0x2c
    740c:	ebffff02 	bl	701c <sd_dev_handle>
    7410:	e5943410 	ldr	r3, [r4, #1040]	; 0x410
    7414:	e3530000 	cmp	r3, #0
    7418:	0afffffb 	beq	740c <versatilepb_sd_read_sector+0x98>
    741c:	e1a00005 	mov	r0, r5
    7420:	e1a01004 	mov	r1, r4
    7424:	e3a02c02 	mov	r2, #512	; 0x200
    7428:	eb001ab2 	bl	def8 <memcpy>
    742c:	e3a00000 	mov	r0, #0
    7430:	e24bd014 	sub	sp, fp, #20
    7434:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    7438:	e12fff1e 	bx	lr
    743c:	e3e00000 	mvn	r0, #0
    7440:	eafffffa 	b	7430 <versatilepb_sd_read_sector+0xbc>
    7444:	000455b8 			; <UNDEFINED> instruction: 0x000455b8
    7448:	00010668 	andeq	r0, r1, r8, ror #12
    744c:	00000024 	andeq	r0, r0, r4, lsr #32
    7450:	00000552 	andeq	r0, r0, r2, asr r5

00007454 <versatilepb_sd_write_sector>:
    7454:	e1a0c00d 	mov	ip, sp
    7458:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    745c:	e59f40c0 	ldr	r4, [pc, #192]	; 7524 <versatilepb_sd_write_sector+0xd0>
    7460:	e08f4004 	add	r4, pc, r4
    7464:	e5943414 	ldr	r3, [r4, #1044]	; 0x414
    7468:	e59f80b8 	ldr	r8, [pc, #184]	; 7528 <versatilepb_sd_write_sector+0xd4>
    746c:	e3530000 	cmp	r3, #0
    7470:	e24cb004 	sub	fp, ip, #4
    7474:	e1a05000 	mov	r5, r0
    7478:	e08f8008 	add	r8, pc, r8
    747c:	0a000026 	beq	751c <versatilepb_sd_write_sector+0xc8>
    7480:	e3a07c02 	mov	r7, #512	; 0x200
    7484:	e0843007 	add	r3, r4, r7
    7488:	e1a02007 	mov	r2, r7
    748c:	e1a00003 	mov	r0, r3
    7490:	eb001a98 	bl	def8 <memcpy>
    7494:	e3a02000 	mov	r2, #0
    7498:	e3a01091 	mov	r1, #145	; 0x91
    749c:	e1a06004 	mov	r6, r4
    74a0:	e5840404 	str	r0, [r4, #1028]	; 0x404
    74a4:	e5842414 	str	r2, [r4, #1044]	; 0x414
    74a8:	e584740c 	str	r7, [r4, #1036]	; 0x40c
    74ac:	e59f3078 	ldr	r3, [pc, #120]	; 752c <versatilepb_sd_write_sector+0xd8>
    74b0:	e7983003 	ldr	r3, [r8, r3]
    74b4:	e5932000 	ldr	r2, [r3]
    74b8:	e59f0070 	ldr	r0, [pc, #112]	; 7530 <versatilepb_sd_write_sector+0xdc>
    74bc:	e2822a05 	add	r2, r2, #20480	; 0x5000
    74c0:	e5820024 	str	r0, [r2, #36]	; 0x24
    74c4:	e5932000 	ldr	r2, [r3]
    74c8:	e2822a05 	add	r2, r2, #20480	; 0x5000
    74cc:	e5827028 	str	r7, [r2, #40]	; 0x28
    74d0:	e5932000 	ldr	r2, [r3]
    74d4:	e1a05485 	lsl	r5, r5, #9
    74d8:	e2822a05 	add	r2, r2, #20480	; 0x5000
    74dc:	e5825008 	str	r5, [r2, #8]
    74e0:	e5932000 	ldr	r2, [r3]
    74e4:	e59f0048 	ldr	r0, [pc, #72]	; 7534 <versatilepb_sd_write_sector+0xe0>
    74e8:	e2822a05 	add	r2, r2, #20480	; 0x5000
    74ec:	e582000c 	str	r0, [r2, #12]
    74f0:	e5933000 	ldr	r3, [r3]
    74f4:	e2833a05 	add	r3, r3, #20480	; 0x5000
    74f8:	e583102c 	str	r1, [r3, #44]	; 0x2c
    74fc:	ebfffec6 	bl	701c <sd_dev_handle>
    7500:	e5963414 	ldr	r3, [r6, #1044]	; 0x414
    7504:	e3530000 	cmp	r3, #0
    7508:	0afffffb 	beq	74fc <versatilepb_sd_write_sector+0xa8>
    750c:	e3a00000 	mov	r0, #0
    7510:	e24bd024 	sub	sp, fp, #36	; 0x24
    7514:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    7518:	e12fff1e 	bx	lr
    751c:	e3e00000 	mvn	r0, #0
    7520:	eafffffa 	b	7510 <versatilepb_sd_write_sector+0xbc>
    7524:	000454d4 	ldrdeq	r5, [r4], -r4
    7528:	00010580 	andeq	r0, r1, r0, lsl #11
    752c:	00000024 	andeq	r0, r0, r4, lsr #32
    7530:	ffff0000 			; <UNDEFINED> instruction: 0xffff0000
    7534:	00000559 	andeq	r0, r0, r9, asr r5

00007538 <crc32>:
    7538:	e3510000 	cmp	r1, #0
    753c:	0a00000c 	beq	7574 <crc32+0x3c>
    7540:	e1a02000 	mov	r2, r0
    7544:	e3a00000 	mov	r0, #0
    7548:	e59fc02c 	ldr	ip, [pc, #44]	; 757c <crc32+0x44>
    754c:	e0821001 	add	r1, r2, r1
    7550:	e08fc00c 	add	ip, pc, ip
    7554:	e4d23001 	ldrb	r3, [r2], #1
    7558:	e0233000 	eor	r3, r3, r0
    755c:	e20330ff 	and	r3, r3, #255	; 0xff
    7560:	e79c3103 	ldr	r3, [ip, r3, lsl #2]
    7564:	e1520001 	cmp	r2, r1
    7568:	e0230420 	eor	r0, r3, r0, lsr #8
    756c:	1afffff8 	bne	7554 <crc32+0x1c>
    7570:	e12fff1e 	bx	lr
    7574:	e1a00001 	mov	r0, r1
    7578:	e12fff1e 	bx	lr
    757c:	00007fd8 	ldrdeq	r7, [r0], -r8

00007580 <hashmap_hash_int>:
    7580:	e1a0c00d 	mov	ip, sp
    7584:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    7588:	e24cb004 	sub	fp, ip, #4
    758c:	e1a04000 	mov	r4, r0
    7590:	e1a00001 	mov	r0, r1
    7594:	e1a05001 	mov	r5, r1
    7598:	eb001bec 	bl	e550 <strlen>
    759c:	e1a01000 	mov	r1, r0
    75a0:	e1a00005 	mov	r0, r5
    75a4:	ebffffe3 	bl	7538 <crc32>
    75a8:	e0800600 	add	r0, r0, r0, lsl #12
    75ac:	e0203b20 	eor	r3, r0, r0, lsr #22
    75b0:	e0833203 	add	r3, r3, r3, lsl #4
    75b4:	e02334a3 	eor	r3, r3, r3, lsr #9
    75b8:	e0833503 	add	r3, r3, r3, lsl #10
    75bc:	e0233123 	eor	r3, r3, r3, lsr #2
    75c0:	e0833383 	add	r3, r3, r3, lsl #7
    75c4:	e0233623 	eor	r3, r3, r3, lsr #12
    75c8:	e1a031a3 	lsr	r3, r3, #3
    75cc:	e0830103 	add	r0, r3, r3, lsl #2
    75d0:	e0630280 	rsb	r0, r3, r0, lsl #5
    75d4:	e0800280 	add	r0, r0, r0, lsl #5
    75d8:	e0630100 	rsb	r0, r3, r0, lsl #2
    75dc:	e0600280 	rsb	r0, r0, r0, lsl #5
    75e0:	e0600400 	rsb	r0, r0, r0, lsl #8
    75e4:	e5941000 	ldr	r1, [r4]
    75e8:	e0830200 	add	r0, r3, r0, lsl #4
    75ec:	eb001344 	bl	c304 <mod_u32>
    75f0:	e24bd014 	sub	sp, fp, #20
    75f4:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    75f8:	e12fff1e 	bx	lr

000075fc <hashmap_hash>:
    75fc:	e1a0c00d 	mov	ip, sp
    7600:	e5903000 	ldr	r3, [r0]
    7604:	e5902004 	ldr	r2, [r0, #4]
    7608:	e0833fa3 	add	r3, r3, r3, lsr #31
    760c:	e15200c3 	cmp	r2, r3, asr #1
    7610:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    7614:	e24cb004 	sub	fp, ip, #4
    7618:	e1a06000 	mov	r6, r0
    761c:	e1a07001 	mov	r7, r1
    7620:	aa000015 	bge	767c <hashmap_hash+0x80>
    7624:	ebffffd5 	bl	7580 <hashmap_hash_int>
    7628:	e3a05008 	mov	r5, #8
    762c:	e1a04000 	mov	r4, r0
    7630:	e5962008 	ldr	r2, [r6, #8]
    7634:	e0841084 	add	r1, r4, r4, lsl #1
    7638:	e0823101 	add	r3, r2, r1, lsl #2
    763c:	e5933004 	ldr	r3, [r3, #4]
    7640:	e3530000 	cmp	r3, #0
    7644:	0a000010 	beq	768c <hashmap_hash+0x90>
    7648:	e3530001 	cmp	r3, #1
    764c:	1a000004 	bne	7664 <hashmap_hash+0x68>
    7650:	e7920101 	ldr	r0, [r2, r1, lsl #2]
    7654:	e1a01007 	mov	r1, r7
    7658:	eb001af7 	bl	e23c <strcmp>
    765c:	e3500000 	cmp	r0, #0
    7660:	0a000009 	beq	768c <hashmap_hash+0x90>
    7664:	e2840001 	add	r0, r4, #1
    7668:	e5961000 	ldr	r1, [r6]
    766c:	eb001324 	bl	c304 <mod_u32>
    7670:	e2555001 	subs	r5, r5, #1
    7674:	e1a04000 	mov	r4, r0
    7678:	1affffec 	bne	7630 <hashmap_hash+0x34>
    767c:	e3e00001 	mvn	r0, #1
    7680:	e24bd01c 	sub	sp, fp, #28
    7684:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    7688:	e12fff1e 	bx	lr
    768c:	e1a00004 	mov	r0, r4
    7690:	e24bd01c 	sub	sp, fp, #28
    7694:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    7698:	e12fff1e 	bx	lr

0000769c <hashmap_put>:
    769c:	e1a0c00d 	mov	ip, sp
    76a0:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    76a4:	e24cb004 	sub	fp, ip, #4
    76a8:	e1a07002 	mov	r7, r2
    76ac:	e1a05000 	mov	r5, r0
    76b0:	e1a06001 	mov	r6, r1
    76b4:	ebffffd0 	bl	75fc <hashmap_hash>
    76b8:	e3700002 	cmn	r0, #2
    76bc:	0a000005 	beq	76d8 <hashmap_put+0x3c>
    76c0:	ea00000b 	b	76f4 <hashmap_put+0x58>
    76c4:	e1a00005 	mov	r0, r5
    76c8:	e1a01006 	mov	r1, r6
    76cc:	ebffffca 	bl	75fc <hashmap_hash>
    76d0:	e3700002 	cmn	r0, #2
    76d4:	1a000006 	bne	76f4 <hashmap_put+0x58>
    76d8:	e1a00005 	mov	r0, r5
    76dc:	eb000019 	bl	7748 <hashmap_rehash>
    76e0:	e3700001 	cmn	r0, #1
    76e4:	1afffff6 	bne	76c4 <hashmap_put+0x28>
    76e8:	e24bd024 	sub	sp, fp, #36	; 0x24
    76ec:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    76f0:	e12fff1e 	bx	lr
    76f4:	e5959008 	ldr	r9, [r5, #8]
    76f8:	e0800080 	add	r0, r0, r0, lsl #1
    76fc:	e1a04100 	lsl	r4, r0, #2
    7700:	e0898004 	add	r8, r9, r4
    7704:	e5887008 	str	r7, [r8, #8]
    7708:	e1a00006 	mov	r0, r6
    770c:	eb001b8f 	bl	e550 <strlen>
    7710:	e2800001 	add	r0, r0, #1
    7714:	eb0016ec 	bl	d2cc <malloc>
    7718:	e1a01006 	mov	r1, r6
    771c:	e7890004 	str	r0, [r9, r4]
    7720:	eb001b4e 	bl	e460 <strcpy>
    7724:	e3a02001 	mov	r2, #1
    7728:	e3a00000 	mov	r0, #0
    772c:	e5953004 	ldr	r3, [r5, #4]
    7730:	e0833002 	add	r3, r3, r2
    7734:	e5882004 	str	r2, [r8, #4]
    7738:	e5853004 	str	r3, [r5, #4]
    773c:	e24bd024 	sub	sp, fp, #36	; 0x24
    7740:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    7744:	e12fff1e 	bx	lr

00007748 <hashmap_rehash>:
    7748:	e1a0c00d 	mov	ip, sp
    774c:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    7750:	e5906000 	ldr	r6, [r0]
    7754:	e1a04086 	lsl	r4, r6, #1
    7758:	e24cb004 	sub	fp, ip, #4
    775c:	e1a07000 	mov	r7, r0
    7760:	e3a0100c 	mov	r1, #12
    7764:	e1a00004 	mov	r0, r4
    7768:	eb0016b6 	bl	d248 <calloc>
    776c:	e3500000 	cmp	r0, #0
    7770:	0a00001d 	beq	77ec <hashmap_rehash+0xa4>
    7774:	e3a05000 	mov	r5, #0
    7778:	e5978008 	ldr	r8, [r7, #8]
    777c:	e1560005 	cmp	r6, r5
    7780:	e8870030 	stm	r7, {r4, r5}
    7784:	e5870008 	str	r0, [r7, #8]
    7788:	c2884004 	addgt	r4, r8, #4
    778c:	ca000003 	bgt	77a0 <hashmap_rehash+0x58>
    7790:	ea00000f 	b	77d4 <hashmap_rehash+0x8c>
    7794:	e1550006 	cmp	r5, r6
    7798:	e284400c 	add	r4, r4, #12
    779c:	0a00000c 	beq	77d4 <hashmap_rehash+0x8c>
    77a0:	e5943000 	ldr	r3, [r4]
    77a4:	e3530000 	cmp	r3, #0
    77a8:	e2855001 	add	r5, r5, #1
    77ac:	0afffff8 	beq	7794 <hashmap_rehash+0x4c>
    77b0:	e1a00007 	mov	r0, r7
    77b4:	e5141004 	ldr	r1, [r4, #-4]
    77b8:	e5942004 	ldr	r2, [r4, #4]
    77bc:	ebffffb6 	bl	769c <hashmap_put>
    77c0:	e3500000 	cmp	r0, #0
    77c4:	0afffff2 	beq	7794 <hashmap_rehash+0x4c>
    77c8:	e24bd024 	sub	sp, fp, #36	; 0x24
    77cc:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    77d0:	e12fff1e 	bx	lr
    77d4:	e1a00008 	mov	r0, r8
    77d8:	eb0016b2 	bl	d2a8 <free>
    77dc:	e3a00000 	mov	r0, #0
    77e0:	e24bd024 	sub	sp, fp, #36	; 0x24
    77e4:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    77e8:	e12fff1e 	bx	lr
    77ec:	e3e00000 	mvn	r0, #0
    77f0:	eafffff4 	b	77c8 <hashmap_rehash+0x80>

000077f4 <hashmap_get>:
    77f4:	e1a0c00d 	mov	ip, sp
    77f8:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    77fc:	e24cb004 	sub	fp, ip, #4
    7800:	e1a08002 	mov	r8, r2
    7804:	e1a06000 	mov	r6, r0
    7808:	e1a07001 	mov	r7, r1
    780c:	ebffff5b 	bl	7580 <hashmap_hash_int>
    7810:	e3a05008 	mov	r5, #8
    7814:	e1a04000 	mov	r4, r0
    7818:	e5963008 	ldr	r3, [r6, #8]
    781c:	e0842084 	add	r2, r4, r4, lsl #1
    7820:	e0839102 	add	r9, r3, r2, lsl #2
    7824:	e5991004 	ldr	r1, [r9, #4]
    7828:	e3510001 	cmp	r1, #1
    782c:	1a000006 	bne	784c <hashmap_get+0x58>
    7830:	e7930102 	ldr	r0, [r3, r2, lsl #2]
    7834:	e3500000 	cmp	r0, #0
    7838:	0a000003 	beq	784c <hashmap_get+0x58>
    783c:	e1a01007 	mov	r1, r7
    7840:	eb001a7d 	bl	e23c <strcmp>
    7844:	e3500000 	cmp	r0, #0
    7848:	0a00000a 	beq	7878 <hashmap_get+0x84>
    784c:	e2840001 	add	r0, r4, #1
    7850:	e5961000 	ldr	r1, [r6]
    7854:	eb0012aa 	bl	c304 <mod_u32>
    7858:	e2555001 	subs	r5, r5, #1
    785c:	e1a04000 	mov	r4, r0
    7860:	1affffec 	bne	7818 <hashmap_get+0x24>
    7864:	e3e00002 	mvn	r0, #2
    7868:	e5885000 	str	r5, [r8]
    786c:	e24bd024 	sub	sp, fp, #36	; 0x24
    7870:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    7874:	e12fff1e 	bx	lr
    7878:	e5993008 	ldr	r3, [r9, #8]
    787c:	e5883000 	str	r3, [r8]
    7880:	e24bd024 	sub	sp, fp, #36	; 0x24
    7884:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    7888:	e12fff1e 	bx	lr

0000788c <hashmap_remove>:
    788c:	e1a0c00d 	mov	ip, sp
    7890:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    7894:	e24cb004 	sub	fp, ip, #4
    7898:	e24dd00c 	sub	sp, sp, #12
    789c:	e1a06000 	mov	r6, r0
    78a0:	e1a08001 	mov	r8, r1
    78a4:	ebffff35 	bl	7580 <hashmap_hash_int>
    78a8:	e3a05008 	mov	r5, #8
    78ac:	e1a04000 	mov	r4, r0
    78b0:	e0842084 	add	r2, r4, r4, lsl #1
    78b4:	e5963008 	ldr	r3, [r6, #8]
    78b8:	e1a09102 	lsl	r9, r2, #2
    78bc:	e0837009 	add	r7, r3, r9
    78c0:	e5971004 	ldr	r1, [r7, #4]
    78c4:	e3510001 	cmp	r1, #1
    78c8:	1a000005 	bne	78e4 <hashmap_remove+0x58>
    78cc:	e793a102 	ldr	sl, [r3, r2, lsl #2]
    78d0:	e1a01008 	mov	r1, r8
    78d4:	e1a0000a 	mov	r0, sl
    78d8:	eb001a57 	bl	e23c <strcmp>
    78dc:	e2503000 	subs	r3, r0, #0
    78e0:	0a00000a 	beq	7910 <hashmap_remove+0x84>
    78e4:	e2840001 	add	r0, r4, #1
    78e8:	e5961000 	ldr	r1, [r6]
    78ec:	eb001284 	bl	c304 <mod_u32>
    78f0:	e2555001 	subs	r5, r5, #1
    78f4:	e1a04000 	mov	r4, r0
    78f8:	1affffec 	bne	78b0 <hashmap_remove+0x24>
    78fc:	e3e03002 	mvn	r3, #2
    7900:	e1a00003 	mov	r0, r3
    7904:	e24bd028 	sub	sp, fp, #40	; 0x28
    7908:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    790c:	e12fff1e 	bx	lr
    7910:	e5873004 	str	r3, [r7, #4]
    7914:	e5873008 	str	r3, [r7, #8]
    7918:	e1a0000a 	mov	r0, sl
    791c:	e50b3030 	str	r3, [fp, #-48]	; 0x30
    7920:	eb001660 	bl	d2a8 <free>
    7924:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
    7928:	e1a00003 	mov	r0, r3
    792c:	e5962004 	ldr	r2, [r6, #4]
    7930:	e5961008 	ldr	r1, [r6, #8]
    7934:	e2422001 	sub	r2, r2, #1
    7938:	e7813009 	str	r3, [r1, r9]
    793c:	e5862004 	str	r2, [r6, #4]
    7940:	e24bd028 	sub	sp, fp, #40	; 0x28
    7944:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    7948:	e12fff1e 	bx	lr

0000794c <hashmap_free>:
    794c:	e1a0c00d 	mov	ip, sp
    7950:	e5901000 	ldr	r1, [r0]
    7954:	e3510000 	cmp	r1, #0
    7958:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    795c:	e24cb004 	sub	fp, ip, #4
    7960:	e1a06000 	mov	r6, r0
    7964:	d5903008 	ldrle	r3, [r0, #8]
    7968:	da000010 	ble	79b0 <hashmap_free+0x64>
    796c:	e3a04000 	mov	r4, #0
    7970:	e1a05004 	mov	r5, r4
    7974:	e5903008 	ldr	r3, [r0, #8]
    7978:	e0832004 	add	r2, r3, r4
    797c:	e5922004 	ldr	r2, [r2, #4]
    7980:	e3520000 	cmp	r2, #0
    7984:	e2855001 	add	r5, r5, #1
    7988:	0a000005 	beq	79a4 <hashmap_free+0x58>
    798c:	e7930004 	ldr	r0, [r3, r4]
    7990:	e3500000 	cmp	r0, #0
    7994:	0a000002 	beq	79a4 <hashmap_free+0x58>
    7998:	eb001642 	bl	d2a8 <free>
    799c:	e5963008 	ldr	r3, [r6, #8]
    79a0:	e5961000 	ldr	r1, [r6]
    79a4:	e1510005 	cmp	r1, r5
    79a8:	e284400c 	add	r4, r4, #12
    79ac:	cafffff1 	bgt	7978 <hashmap_free+0x2c>
    79b0:	e1a00003 	mov	r0, r3
    79b4:	eb00163b 	bl	d2a8 <free>
    79b8:	e1a00006 	mov	r0, r6
    79bc:	eb001639 	bl	d2a8 <free>
    79c0:	e24bd01c 	sub	sp, fp, #28
    79c4:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    79c8:	e12fff1e 	bx	lr

000079cc <hashmap_new>:
    79cc:	e1a0c00d 	mov	ip, sp
    79d0:	e3a0000c 	mov	r0, #12
    79d4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    79d8:	e24cb004 	sub	fp, ip, #4
    79dc:	eb00163a 	bl	d2cc <malloc>
    79e0:	e2505000 	subs	r5, r0, #0
    79e4:	01a04005 	moveq	r4, r5
    79e8:	0a00000a 	beq	7a18 <hashmap_new+0x4c>
    79ec:	e3a00080 	mov	r0, #128	; 0x80
    79f0:	e3a0100c 	mov	r1, #12
    79f4:	eb001613 	bl	d248 <calloc>
    79f8:	e3500000 	cmp	r0, #0
    79fc:	e1a04000 	mov	r4, r0
    7a00:	e5850008 	str	r0, [r5, #8]
    7a04:	0a000007 	beq	7a28 <hashmap_new+0x5c>
    7a08:	e3a02080 	mov	r2, #128	; 0x80
    7a0c:	e3a03000 	mov	r3, #0
    7a10:	e1a04005 	mov	r4, r5
    7a14:	e885000c 	stm	r5, {r2, r3}
    7a18:	e1a00004 	mov	r0, r4
    7a1c:	e24bd014 	sub	sp, fp, #20
    7a20:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    7a24:	e12fff1e 	bx	lr
    7a28:	e1a00005 	mov	r0, r5
    7a2c:	ebffffc6 	bl	794c <hashmap_free>
    7a30:	e1a00004 	mov	r0, r4
    7a34:	e24bd014 	sub	sp, fp, #20
    7a38:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    7a3c:	e12fff1e 	bx	lr

00007a40 <hashmap_length>:
    7a40:	e3500000 	cmp	r0, #0
    7a44:	15900004 	ldrne	r0, [r0, #4]
    7a48:	e12fff1e 	bx	lr

00007a4c <hashmap_iterate>:
    7a4c:	e1a0c00d 	mov	ip, sp
    7a50:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    7a54:	e24cb004 	sub	fp, ip, #4
    7a58:	e1a06001 	mov	r6, r1
    7a5c:	e1a07002 	mov	r7, r2
    7a60:	e1a08000 	mov	r8, r0
    7a64:	ebfffff5 	bl	7a40 <hashmap_length>
    7a68:	e3500000 	cmp	r0, #0
    7a6c:	da00001b 	ble	7ae0 <hashmap_iterate+0x94>
    7a70:	e598c000 	ldr	ip, [r8]
    7a74:	e35c0000 	cmp	ip, #0
    7a78:	c3a04000 	movgt	r4, #0
    7a7c:	c1a05004 	movgt	r5, r4
    7a80:	da000012 	ble	7ad0 <hashmap_iterate+0x84>
    7a84:	e5983008 	ldr	r3, [r8, #8]
    7a88:	e0832004 	add	r2, r3, r4
    7a8c:	e5921004 	ldr	r1, [r2, #4]
    7a90:	e3510000 	cmp	r1, #0
    7a94:	e2855001 	add	r5, r5, #1
    7a98:	0a000009 	beq	7ac4 <hashmap_iterate+0x78>
    7a9c:	e7930004 	ldr	r0, [r3, r4]
    7aa0:	e3500000 	cmp	r0, #0
    7aa4:	0a000006 	beq	7ac4 <hashmap_iterate+0x78>
    7aa8:	e5921008 	ldr	r1, [r2, #8]
    7aac:	e1a02007 	mov	r2, r7
    7ab0:	e1a0e00f 	mov	lr, pc
    7ab4:	e12fff16 	bx	r6
    7ab8:	e3500000 	cmp	r0, #0
    7abc:	1a000004 	bne	7ad4 <hashmap_iterate+0x88>
    7ac0:	e598c000 	ldr	ip, [r8]
    7ac4:	e15c0005 	cmp	ip, r5
    7ac8:	e284400c 	add	r4, r4, #12
    7acc:	caffffec 	bgt	7a84 <hashmap_iterate+0x38>
    7ad0:	e3a00000 	mov	r0, #0
    7ad4:	e24bd024 	sub	sp, fp, #36	; 0x24
    7ad8:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    7adc:	e12fff1e 	bx	lr
    7ae0:	e3e00002 	mvn	r0, #2
    7ae4:	eafffffa 	b	7ad4 <hashmap_iterate+0x88>

00007ae8 <syscall3_raw>:
    7ae8:	e92d4030 	push	{r4, r5, lr}
    7aec:	e1a0c002 	mov	ip, r2
    7af0:	e24dd00c 	sub	sp, sp, #12
    7af4:	e1a0e000 	mov	lr, r0
    7af8:	e1a04001 	mov	r4, r1
    7afc:	e1a05003 	mov	r5, r3
    7b00:	e92d4000 	stmfd	sp!, {lr}
    7b04:	e10f0000 	mrs	r0, CPSR
    7b08:	e92d0001 	stmfd	sp!, {r0}
    7b0c:	e1a0000e 	mov	r0, lr
    7b10:	e1a01004 	mov	r1, r4
    7b14:	e1a0200c 	mov	r2, ip
    7b18:	e1a03005 	mov	r3, r5
    7b1c:	ef000000 	svc	0x00000000
    7b20:	e1a0c000 	mov	ip, r0
    7b24:	e8bd0002 	ldmfd	sp!, {r1}
    7b28:	e8bd4000 	ldmfd	sp!, {lr}
    7b2c:	e58dc004 	str	ip, [sp, #4]
    7b30:	e59d0004 	ldr	r0, [sp, #4]
    7b34:	e28dd00c 	add	sp, sp, #12
    7b38:	e8bd4030 	pop	{r4, r5, lr}
    7b3c:	e12fff1e 	bx	lr

00007b40 <syscall3>:
    7b40:	eaffffe8 	b	7ae8 <syscall3_raw>

00007b44 <syscall2>:
    7b44:	e3a03000 	mov	r3, #0
    7b48:	eaffffe6 	b	7ae8 <syscall3_raw>

00007b4c <syscall1>:
    7b4c:	e3a02000 	mov	r2, #0
    7b50:	e1a03002 	mov	r3, r2
    7b54:	eaffffe3 	b	7ae8 <syscall3_raw>

00007b58 <syscall0>:
    7b58:	e3a01000 	mov	r1, #0
    7b5c:	e1a02001 	mov	r2, r1
    7b60:	e1a03001 	mov	r3, r1
    7b64:	eaffffdf 	b	7ae8 <syscall3_raw>

00007b68 <str_reset>:
    7b68:	e1a0c00d 	mov	ip, sp
    7b6c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    7b70:	e5903000 	ldr	r3, [r0]
    7b74:	e3530000 	cmp	r3, #0
    7b78:	e24cb004 	sub	fp, ip, #4
    7b7c:	e1a04000 	mov	r4, r0
    7b80:	0a000005 	beq	7b9c <str_reset+0x34>
    7b84:	e3a02000 	mov	r2, #0
    7b88:	e5c32000 	strb	r2, [r3]
    7b8c:	e5842008 	str	r2, [r4, #8]
    7b90:	e24bd014 	sub	sp, fp, #20
    7b94:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    7b98:	e12fff1e 	bx	lr
    7b9c:	e3a00010 	mov	r0, #16
    7ba0:	eb0015c9 	bl	d2cc <malloc>
    7ba4:	e3a02010 	mov	r2, #16
    7ba8:	e1a03000 	mov	r3, r0
    7bac:	e8840005 	stm	r4, {r0, r2}
    7bb0:	eafffff3 	b	7b84 <str_reset+0x1c>

00007bb4 <str_ncpy>:
    7bb4:	e1a0c00d 	mov	ip, sp
    7bb8:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    7bbc:	e2515000 	subs	r5, r1, #0
    7bc0:	e24cb004 	sub	fp, ip, #4
    7bc4:	e1a04000 	mov	r4, r0
    7bc8:	e1a06002 	mov	r6, r2
    7bcc:	0a000016 	beq	7c2c <str_ncpy+0x78>
    7bd0:	e5d53000 	ldrb	r3, [r5]
    7bd4:	e3520000 	cmp	r2, #0
    7bd8:	13530000 	cmpne	r3, #0
    7bdc:	0a000012 	beq	7c2c <str_ncpy+0x78>
    7be0:	e1a00005 	mov	r0, r5
    7be4:	eb001a59 	bl	e550 <strlen>
    7be8:	e1500006 	cmp	r0, r6
    7bec:	31a06000 	movcc	r6, r0
    7bf0:	e5943004 	ldr	r3, [r4, #4]
    7bf4:	e1530006 	cmp	r3, r6
    7bf8:	9a000011 	bls	7c44 <str_ncpy+0x90>
    7bfc:	e5940000 	ldr	r0, [r4]
    7c00:	e1a02006 	mov	r2, r6
    7c04:	e1a01005 	mov	r1, r5
    7c08:	eb001aae 	bl	e6c8 <strncpy>
    7c0c:	e3a02000 	mov	r2, #0
    7c10:	e5943000 	ldr	r3, [r4]
    7c14:	e7c32006 	strb	r2, [r3, r6]
    7c18:	e5940000 	ldr	r0, [r4]
    7c1c:	e5846008 	str	r6, [r4, #8]
    7c20:	e24bd01c 	sub	sp, fp, #28
    7c24:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    7c28:	e12fff1e 	bx	lr
    7c2c:	e1a00004 	mov	r0, r4
    7c30:	ebffffcc 	bl	7b68 <str_reset>
    7c34:	e5940000 	ldr	r0, [r4]
    7c38:	e24bd01c 	sub	sp, fp, #28
    7c3c:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    7c40:	e12fff1e 	bx	lr
    7c44:	e2867010 	add	r7, r6, #16
    7c48:	e1a01007 	mov	r1, r7
    7c4c:	e5940000 	ldr	r0, [r4]
    7c50:	eb0015a6 	bl	d2f0 <realloc>
    7c54:	e1a03000 	mov	r3, r0
    7c58:	e8840088 	stm	r4, {r3, r7}
    7c5c:	eaffffe7 	b	7c00 <str_ncpy+0x4c>

00007c60 <str_cpy>:
    7c60:	e1a0c00d 	mov	ip, sp
    7c64:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    7c68:	e1a04000 	mov	r4, r0
    7c6c:	e24cb004 	sub	fp, ip, #4
    7c70:	e59f2010 	ldr	r2, [pc, #16]	; 7c88 <str_cpy+0x28>
    7c74:	ebffffce 	bl	7bb4 <str_ncpy>
    7c78:	e5940000 	ldr	r0, [r4]
    7c7c:	e24bd014 	sub	sp, fp, #20
    7c80:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    7c84:	e12fff1e 	bx	lr
    7c88:	0000ffff 	strdeq	pc, [r0], -pc	; <UNPREDICTABLE>

00007c8c <str_new>:
    7c8c:	e1a0c00d 	mov	ip, sp
    7c90:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    7c94:	e24cb004 	sub	fp, ip, #4
    7c98:	e1a05000 	mov	r5, r0
    7c9c:	e3a0000c 	mov	r0, #12
    7ca0:	eb001589 	bl	d2cc <malloc>
    7ca4:	e3a03000 	mov	r3, #0
    7ca8:	e1a04000 	mov	r4, r0
    7cac:	e1a01005 	mov	r1, r5
    7cb0:	e5803000 	str	r3, [r0]
    7cb4:	e5803004 	str	r3, [r0, #4]
    7cb8:	e5803008 	str	r3, [r0, #8]
    7cbc:	ebffffe7 	bl	7c60 <str_cpy>
    7cc0:	e1a00004 	mov	r0, r4
    7cc4:	e24bd014 	sub	sp, fp, #20
    7cc8:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    7ccc:	e12fff1e 	bx	lr

00007cd0 <str_new_by_size>:
    7cd0:	e1a0c00d 	mov	ip, sp
    7cd4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    7cd8:	e1a05000 	mov	r5, r0
    7cdc:	e24cb004 	sub	fp, ip, #4
    7ce0:	e3a0000c 	mov	r0, #12
    7ce4:	eb001578 	bl	d2cc <malloc>
    7ce8:	e1a04000 	mov	r4, r0
    7cec:	e1a00005 	mov	r0, r5
    7cf0:	eb001575 	bl	d2cc <malloc>
    7cf4:	e3a03000 	mov	r3, #0
    7cf8:	e8840021 	stm	r4, {r0, r5}
    7cfc:	e5c03000 	strb	r3, [r0]
    7d00:	e1a00004 	mov	r0, r4
    7d04:	e5843008 	str	r3, [r4, #8]
    7d08:	e24bd014 	sub	sp, fp, #20
    7d0c:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    7d10:	e12fff1e 	bx	lr

00007d14 <str_add>:
    7d14:	e1a0c00d 	mov	ip, sp
    7d18:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    7d1c:	e2516000 	subs	r6, r1, #0
    7d20:	e24cb004 	sub	fp, ip, #4
    7d24:	e1a04000 	mov	r4, r0
    7d28:	0a000014 	beq	7d80 <str_add+0x6c>
    7d2c:	e5d63000 	ldrb	r3, [r6]
    7d30:	e3530000 	cmp	r3, #0
    7d34:	0a000011 	beq	7d80 <str_add+0x6c>
    7d38:	e1a00006 	mov	r0, r6
    7d3c:	eb001a03 	bl	e550 <strlen>
    7d40:	e1a05000 	mov	r5, r0
    7d44:	e5940008 	ldr	r0, [r4, #8]
    7d48:	e5942004 	ldr	r2, [r4, #4]
    7d4c:	e0851000 	add	r1, r5, r0
    7d50:	e1510002 	cmp	r1, r2
    7d54:	2a00000d 	bcs	7d90 <str_add+0x7c>
    7d58:	e5942000 	ldr	r2, [r4]
    7d5c:	e0820000 	add	r0, r2, r0
    7d60:	e1a01006 	mov	r1, r6
    7d64:	eb0019bd 	bl	e460 <strcpy>
    7d68:	e3a02000 	mov	r2, #0
    7d6c:	e5941008 	ldr	r1, [r4, #8]
    7d70:	e5943000 	ldr	r3, [r4]
    7d74:	e0855001 	add	r5, r5, r1
    7d78:	e5845008 	str	r5, [r4, #8]
    7d7c:	e7c32005 	strb	r2, [r3, r5]
    7d80:	e5940000 	ldr	r0, [r4]
    7d84:	e24bd01c 	sub	sp, fp, #28
    7d88:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    7d8c:	e12fff1e 	bx	lr
    7d90:	e2817010 	add	r7, r1, #16
    7d94:	e1a01007 	mov	r1, r7
    7d98:	e5940000 	ldr	r0, [r4]
    7d9c:	eb001553 	bl	d2f0 <realloc>
    7da0:	e1a02000 	mov	r2, r0
    7da4:	e8840081 	stm	r4, {r0, r7}
    7da8:	e5940008 	ldr	r0, [r4, #8]
    7dac:	eaffffea 	b	7d5c <str_add+0x48>

00007db0 <str_addc>:
    7db0:	e1a0c00d 	mov	ip, sp
    7db4:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    7db8:	e2516000 	subs	r6, r1, #0
    7dbc:	e24cb004 	sub	fp, ip, #4
    7dc0:	e1a04000 	mov	r4, r0
    7dc4:	0a000010 	beq	7e0c <str_addc+0x5c>
    7dc8:	e5901008 	ldr	r1, [r0, #8]
    7dcc:	e5903004 	ldr	r3, [r0, #4]
    7dd0:	e2812001 	add	r2, r1, #1
    7dd4:	e1520003 	cmp	r2, r3
    7dd8:	2a000012 	bcs	7e28 <str_addc+0x78>
    7ddc:	e5903000 	ldr	r3, [r0]
    7de0:	e3a00000 	mov	r0, #0
    7de4:	e7c36001 	strb	r6, [r3, r1]
    7de8:	e5943008 	ldr	r3, [r4, #8]
    7dec:	e5942000 	ldr	r2, [r4]
    7df0:	e2833001 	add	r3, r3, #1
    7df4:	e5843008 	str	r3, [r4, #8]
    7df8:	e7c20003 	strb	r0, [r2, r3]
    7dfc:	e5940000 	ldr	r0, [r4]
    7e00:	e24bd01c 	sub	sp, fp, #28
    7e04:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    7e08:	e12fff1e 	bx	lr
    7e0c:	e5902000 	ldr	r2, [r0]
    7e10:	e5903008 	ldr	r3, [r0, #8]
    7e14:	e7c26003 	strb	r6, [r2, r3]
    7e18:	e5900000 	ldr	r0, [r0]
    7e1c:	e24bd01c 	sub	sp, fp, #28
    7e20:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    7e24:	e12fff1e 	bx	lr
    7e28:	e2815010 	add	r5, r1, #16
    7e2c:	e1a01005 	mov	r1, r5
    7e30:	e5900000 	ldr	r0, [r0]
    7e34:	eb00152d 	bl	d2f0 <realloc>
    7e38:	e5941008 	ldr	r1, [r4, #8]
    7e3c:	e1a03000 	mov	r3, r0
    7e40:	e8840021 	stm	r4, {r0, r5}
    7e44:	eaffffe5 	b	7de0 <str_addc+0x30>

00007e48 <outc>:
    7e48:	e1a03000 	mov	r3, r0
    7e4c:	e1a00001 	mov	r0, r1
    7e50:	e1a01003 	mov	r1, r3
    7e54:	eaffffd5 	b	7db0 <str_addc>

00007e58 <str_free>:
    7e58:	e1a0c00d 	mov	ip, sp
    7e5c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    7e60:	e2504000 	subs	r4, r0, #0
    7e64:	e24cb004 	sub	fp, ip, #4
    7e68:	0a000004 	beq	7e80 <str_free+0x28>
    7e6c:	e5940000 	ldr	r0, [r4]
    7e70:	e3500000 	cmp	r0, #0
    7e74:	1b00150b 	blne	d2a8 <free>
    7e78:	e1a00004 	mov	r0, r4
    7e7c:	eb001509 	bl	d2a8 <free>
    7e80:	e24bd014 	sub	sp, fp, #20
    7e84:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    7e88:	e12fff1e 	bx	lr

00007e8c <str_from_int>:
    7e8c:	e1a0c00d 	mov	ip, sp
    7e90:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    7e94:	e2413002 	sub	r3, r1, #2
    7e98:	e3530022 	cmp	r3, #34	; 0x22
    7e9c:	e1a04001 	mov	r4, r1
    7ea0:	83a0500a 	movhi	r5, #10
    7ea4:	e59f80a8 	ldr	r8, [pc, #168]	; 7f54 <str_from_int+0xc8>
    7ea8:	e59f60a8 	ldr	r6, [pc, #168]	; 7f58 <str_from_int+0xcc>
    7eac:	e24cb004 	sub	fp, ip, #4
    7eb0:	e1a07000 	mov	r7, r0
    7eb4:	81a04005 	movhi	r4, r5
    7eb8:	91a05004 	movls	r5, r4
    7ebc:	e08f8008 	add	r8, pc, r8
    7ec0:	e08f6006 	add	r6, pc, r6
    7ec4:	ea000001 	b	7ed0 <str_from_int+0x44>
    7ec8:	e1a08002 	mov	r8, r2
    7ecc:	e1a07000 	mov	r7, r0
    7ed0:	e1a00007 	mov	r0, r7
    7ed4:	e1a01005 	mov	r1, r5
    7ed8:	eb00107b 	bl	c0cc <div_u32>
    7edc:	e1a02008 	mov	r2, r8
    7ee0:	e0030094 	mul	r3, r4, r0
    7ee4:	e0633007 	rsb	r3, r3, r7
    7ee8:	e0863003 	add	r3, r6, r3
    7eec:	e5d33023 	ldrb	r3, [r3, #35]	; 0x23
    7ef0:	e3500000 	cmp	r0, #0
    7ef4:	e4c23001 	strb	r3, [r2], #1
    7ef8:	1afffff2 	bne	7ec8 <str_from_int+0x3c>
    7efc:	e3570000 	cmp	r7, #0
    7f00:	b3a0302d 	movlt	r3, #45	; 0x2d
    7f04:	e3a00000 	mov	r0, #0
    7f08:	b5c83001 	strblt	r3, [r8, #1]
    7f0c:	e59f3048 	ldr	r3, [pc, #72]	; 7f5c <str_from_int+0xd0>
    7f10:	b2882002 	addlt	r2, r8, #2
    7f14:	e08f3003 	add	r3, pc, r3
    7f18:	e2421001 	sub	r1, r2, #1
    7f1c:	e1510003 	cmp	r1, r3
    7f20:	e5c20000 	strb	r0, [r2]
    7f24:	9a000005 	bls	7f40 <str_from_int+0xb4>
    7f28:	e5d12000 	ldrb	r2, [r1]
    7f2c:	e5d30000 	ldrb	r0, [r3]
    7f30:	e4410001 	strb	r0, [r1], #-1
    7f34:	e4c32001 	strb	r2, [r3], #1
    7f38:	e1510003 	cmp	r1, r3
    7f3c:	8afffff9 	bhi	7f28 <str_from_int+0x9c>
    7f40:	e59f0018 	ldr	r0, [pc, #24]	; 7f60 <str_from_int+0xd4>
    7f44:	e08f0000 	add	r0, pc, r0
    7f48:	e24bd024 	sub	sp, fp, #36	; 0x24
    7f4c:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    7f50:	e12fff1e 	bx	lr
    7f54:	00044e90 	muleq	r4, r0, lr
    7f58:	00007a68 	andeq	r7, r0, r8, ror #20
    7f5c:	00044e38 	andeq	r4, r4, r8, lsr lr
    7f60:	00044e08 	andeq	r4, r4, r8, lsl #28

00007f64 <str_addc_int>:
    7f64:	e1a0c00d 	mov	ip, sp
    7f68:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    7f6c:	e1a04000 	mov	r4, r0
    7f70:	e24cb004 	sub	fp, ip, #4
    7f74:	e1a00001 	mov	r0, r1
    7f78:	e1a01002 	mov	r1, r2
    7f7c:	ebffffc2 	bl	7e8c <str_from_int>
    7f80:	e1a01000 	mov	r1, r0
    7f84:	e1a00004 	mov	r0, r4
    7f88:	e24bd014 	sub	sp, fp, #20
    7f8c:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    7f90:	eaffff5f 	b	7d14 <str_add>

00007f94 <str_from_bool>:
    7f94:	e3500000 	cmp	r0, #0
    7f98:	1a000002 	bne	7fa8 <str_from_bool+0x14>
    7f9c:	e59f0010 	ldr	r0, [pc, #16]	; 7fb4 <str_from_bool+0x20>
    7fa0:	e08f0000 	add	r0, pc, r0
    7fa4:	e12fff1e 	bx	lr
    7fa8:	e59f0008 	ldr	r0, [pc, #8]	; 7fb8 <str_from_bool+0x24>
    7fac:	e08f0000 	add	r0, pc, r0
    7fb0:	e12fff1e 	bx	lr
    7fb4:	000079d8 	ldrdeq	r7, [r0], -r8
    7fb8:	000079c4 	andeq	r7, r0, r4, asr #19

00007fbc <str_from_float>:
    7fbc:	e59f3010 	ldr	r3, [pc, #16]	; 7fd4 <str_from_float+0x18>
    7fc0:	e59f2010 	ldr	r2, [pc, #16]	; 7fd8 <str_from_float+0x1c>
    7fc4:	e08f3003 	add	r3, pc, r3
    7fc8:	e1a00003 	mov	r0, r3
    7fcc:	e5832000 	str	r2, [r3]
    7fd0:	e12fff1e 	bx	lr
    7fd4:	00044d88 	andeq	r4, r4, r8, lsl #27
    7fd8:	00302e30 	eorseq	r2, r0, r0, lsr lr

00007fdc <str_addc_float>:
    7fdc:	e1a0c00d 	mov	ip, sp
    7fe0:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    7fe4:	e1a04000 	mov	r4, r0
    7fe8:	e24cb004 	sub	fp, ip, #4
    7fec:	e1a00001 	mov	r0, r1
    7ff0:	ebfffff1 	bl	7fbc <str_from_float>
    7ff4:	e1a01000 	mov	r1, r0
    7ff8:	e1a00004 	mov	r0, r4
    7ffc:	e24bd014 	sub	sp, fp, #20
    8000:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    8004:	eaffff42 	b	7d14 <str_add>

00008008 <str_to_int>:
    8008:	e1a0c00d 	mov	ip, sp
    800c:	e59f1030 	ldr	r1, [pc, #48]	; 8044 <str_to_int+0x3c>
    8010:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    8014:	e08f1001 	add	r1, pc, r1
    8018:	e24cb004 	sub	fp, ip, #4
    801c:	e1a04000 	mov	r4, r0
    8020:	eb001b0e 	bl	ec60 <strstr>
    8024:	e3500000 	cmp	r0, #0
    8028:	13a01010 	movne	r1, #16
    802c:	e1a00004 	mov	r0, r4
    8030:	03a0100a 	moveq	r1, #10
    8034:	eb00142b 	bl	d0e8 <atoi_base>
    8038:	e24bd014 	sub	sp, fp, #20
    803c:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    8040:	e12fff1e 	bx	lr
    8044:	0000796c 	andeq	r7, r0, ip, ror #18

00008048 <str_to_bool>:
    8048:	e1a0c00d 	mov	ip, sp
    804c:	e59f1040 	ldr	r1, [pc, #64]	; 8094 <str_to_bool+0x4c>
    8050:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    8054:	e08f1001 	add	r1, pc, r1
    8058:	e24cb004 	sub	fp, ip, #4
    805c:	e1a04000 	mov	r4, r0
    8060:	eb001875 	bl	e23c <strcmp>
    8064:	e3500000 	cmp	r0, #0
    8068:	03a00001 	moveq	r0, #1
    806c:	0a000005 	beq	8088 <str_to_bool+0x40>
    8070:	e59f1020 	ldr	r1, [pc, #32]	; 8098 <str_to_bool+0x50>
    8074:	e1a00004 	mov	r0, r4
    8078:	e08f1001 	add	r1, pc, r1
    807c:	eb00186e 	bl	e23c <strcmp>
    8080:	e2700001 	rsbs	r0, r0, #1
    8084:	33a00000 	movcc	r0, #0
    8088:	e24bd014 	sub	sp, fp, #20
    808c:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    8090:	e12fff1e 	bx	lr
    8094:	0000791c 	andeq	r7, r0, ip, lsl r9
    8098:	0000790c 	andeq	r7, r0, ip, lsl #18

0000809c <str_to_float>:
    809c:	e3a00000 	mov	r0, #0
    80a0:	e12fff1e 	bx	lr

000080a4 <str_to>:
    80a4:	e1a0c00d 	mov	ip, sp
    80a8:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    80ac:	e2528000 	subs	r8, r2, #0
    80b0:	e24cb004 	sub	fp, ip, #4
    80b4:	e1a05000 	mov	r5, r0
    80b8:	e1a06001 	mov	r6, r1
    80bc:	e1a07003 	mov	r7, r3
    80c0:	11a00008 	movne	r0, r8
    80c4:	1bfffea7 	blne	7b68 <str_reset>
    80c8:	e3570000 	cmp	r7, #0
    80cc:	e5d51000 	ldrb	r1, [r5]
    80d0:	0a000013 	beq	8124 <str_to+0x80>
    80d4:	e3510000 	cmp	r1, #0
    80d8:	0a00000d 	beq	8114 <str_to+0x70>
    80dc:	e3510009 	cmp	r1, #9
    80e0:	13510020 	cmpne	r1, #32
    80e4:	13a04001 	movne	r4, #1
    80e8:	03a04000 	moveq	r4, #0
    80ec:	01a03005 	moveq	r3, r5
    80f0:	0a000003 	beq	8104 <str_to+0x60>
    80f4:	ea000021 	b	8180 <str_to+0xdc>
    80f8:	e3510009 	cmp	r1, #9
    80fc:	13510020 	cmpne	r1, #32
    8100:	1a00001f 	bne	8184 <str_to+0xe0>
    8104:	e5f31001 	ldrb	r1, [r3, #1]!
    8108:	e3510000 	cmp	r1, #0
    810c:	e2844001 	add	r4, r4, #1
    8110:	1afffff8 	bne	80f8 <str_to+0x54>
    8114:	e3e00000 	mvn	r0, #0
    8118:	e24bd024 	sub	sp, fp, #36	; 0x24
    811c:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    8120:	e12fff1e 	bx	lr
    8124:	e3510000 	cmp	r1, #0
    8128:	0afffff9 	beq	8114 <str_to+0x70>
    812c:	e1560001 	cmp	r6, r1
    8130:	e1a04007 	mov	r4, r7
    8134:	0a00000d 	beq	8170 <str_to+0xcc>
    8138:	e0855004 	add	r5, r5, r4
    813c:	ea000001 	b	8148 <str_to+0xa4>
    8140:	e1510006 	cmp	r1, r6
    8144:	0a000010 	beq	818c <str_to+0xe8>
    8148:	e3580000 	cmp	r8, #0
    814c:	11a00008 	movne	r0, r8
    8150:	1bffff16 	blne	7db0 <str_addc>
    8154:	e5f51001 	ldrb	r1, [r5, #1]!
    8158:	e3510000 	cmp	r1, #0
    815c:	e2844001 	add	r4, r4, #1
    8160:	1afffff6 	bne	8140 <str_to+0x9c>
    8164:	eaffffea 	b	8114 <str_to+0x70>
    8168:	e2833001 	add	r3, r3, #1
    816c:	e5883008 	str	r3, [r8, #8]
    8170:	e1a00004 	mov	r0, r4
    8174:	e24bd024 	sub	sp, fp, #36	; 0x24
    8178:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    817c:	e12fff1e 	bx	lr
    8180:	e3a04000 	mov	r4, #0
    8184:	e1560001 	cmp	r6, r1
    8188:	1affffea 	bne	8138 <str_to+0x94>
    818c:	e3580000 	cmp	r8, #0
    8190:	13570000 	cmpne	r7, #0
    8194:	0afffff5 	beq	8170 <str_to+0xcc>
    8198:	e5983008 	ldr	r3, [r8, #8]
    819c:	e2533001 	subs	r3, r3, #1
    81a0:	4afffff2 	bmi	8170 <str_to+0xcc>
    81a4:	e5982000 	ldr	r2, [r8]
    81a8:	e7d21003 	ldrb	r1, [r2, r3]
    81ac:	e3510020 	cmp	r1, #32
    81b0:	13510009 	cmpne	r1, #9
    81b4:	e0822003 	add	r2, r2, r3
    81b8:	03a00000 	moveq	r0, #0
    81bc:	0a000006 	beq	81dc <str_to+0x138>
    81c0:	eaffffe8 	b	8168 <str_to+0xc4>
    81c4:	e5982000 	ldr	r2, [r8]
    81c8:	e7d21003 	ldrb	r1, [r2, r3]
    81cc:	e3510020 	cmp	r1, #32
    81d0:	13510009 	cmpne	r1, #9
    81d4:	e0822003 	add	r2, r2, r3
    81d8:	1affffe2 	bne	8168 <str_to+0xc4>
    81dc:	e2433001 	sub	r3, r3, #1
    81e0:	e3730001 	cmn	r3, #1
    81e4:	e5c20000 	strb	r0, [r2]
    81e8:	1afffff5 	bne	81c4 <str_to+0x120>
    81ec:	eaffffdf 	b	8170 <str_to+0xcc>

000081f0 <str_format>:
    81f0:	e1a0c00d 	mov	ip, sp
    81f4:	e92d000e 	push	{r1, r2, r3}
    81f8:	e92dd810 	push	{r4, fp, ip, lr, pc}
    81fc:	e1a04000 	mov	r4, r0
    8200:	e24cb010 	sub	fp, ip, #16
    8204:	e24dd008 	sub	sp, sp, #8
    8208:	ebfffe56 	bl	7b68 <str_reset>
    820c:	e59f0028 	ldr	r0, [pc, #40]	; 823c <str_format+0x4c>
    8210:	e28bc008 	add	ip, fp, #8
    8214:	e1a01004 	mov	r1, r4
    8218:	e59b2004 	ldr	r2, [fp, #4]
    821c:	e1a0300c 	mov	r3, ip
    8220:	e08f0000 	add	r0, pc, r0
    8224:	e50bc014 	str	ip, [fp, #-20]
    8228:	eb0014bb 	bl	d51c <v_printf>
    822c:	e1a00004 	mov	r0, r4
    8230:	e24bd010 	sub	sp, fp, #16
    8234:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
    8238:	e12fff1e 	bx	lr
    823c:	fffffc20 			; <UNDEFINED> instruction: 0xfffffc20

00008240 <str_format_new>:
    8240:	e1a0c00d 	mov	ip, sp
    8244:	e92d000f 	push	{r0, r1, r2, r3}
    8248:	e92dd810 	push	{r4, fp, ip, lr, pc}
    824c:	e24cb014 	sub	fp, ip, #20
    8250:	e24dd00c 	sub	sp, sp, #12
    8254:	e3a00000 	mov	r0, #0
    8258:	ebfffe8b 	bl	7c8c <str_new>
    825c:	e1a04000 	mov	r4, r0
    8260:	e1a01000 	mov	r1, r0
    8264:	e59f0024 	ldr	r0, [pc, #36]	; 8290 <str_format_new+0x50>
    8268:	e28bc008 	add	ip, fp, #8
    826c:	e59b2004 	ldr	r2, [fp, #4]
    8270:	e1a0300c 	mov	r3, ip
    8274:	e08f0000 	add	r0, pc, r0
    8278:	e50bc018 	str	ip, [fp, #-24]
    827c:	eb0014a6 	bl	d51c <v_printf>
    8280:	e1a00004 	mov	r0, r4
    8284:	e24bd010 	sub	sp, fp, #16
    8288:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
    828c:	e12fff1e 	bx	lr
    8290:	fffffbcc 			; <UNDEFINED> instruction: 0xfffffbcc

00008294 <outc>:
    8294:	e1a0c00d 	mov	ip, sp
    8298:	e1a03000 	mov	r3, r0
    829c:	e92dd800 	push	{fp, ip, lr, pc}
    82a0:	e1a00001 	mov	r0, r1
    82a4:	e24cb004 	sub	fp, ip, #4
    82a8:	e1a01003 	mov	r1, r3
    82ac:	ebfffebf 	bl	7db0 <str_addc>
    82b0:	e24bd00c 	sub	sp, fp, #12
    82b4:	e89d6800 	ldm	sp, {fp, sp, lr}
    82b8:	e12fff1e 	bx	lr

000082bc <klog>:
    82bc:	e1a0c00d 	mov	ip, sp
    82c0:	e92d000f 	push	{r0, r1, r2, r3}
    82c4:	e92dd810 	push	{r4, fp, ip, lr, pc}
    82c8:	e59f0054 	ldr	r0, [pc, #84]	; 8324 <klog+0x68>
    82cc:	e24cb014 	sub	fp, ip, #20
    82d0:	e24dd00c 	sub	sp, sp, #12
    82d4:	e08f0000 	add	r0, pc, r0
    82d8:	ebfffe6b 	bl	7c8c <str_new>
    82dc:	e1a04000 	mov	r4, r0
    82e0:	e1a01000 	mov	r1, r0
    82e4:	e59f003c 	ldr	r0, [pc, #60]	; 8328 <klog+0x6c>
    82e8:	e28bc008 	add	ip, fp, #8
    82ec:	e1a0300c 	mov	r3, ip
    82f0:	e59b2004 	ldr	r2, [fp, #4]
    82f4:	e08f0000 	add	r0, pc, r0
    82f8:	e50bc018 	str	ip, [fp, #-24]
    82fc:	eb001486 	bl	d51c <v_printf>
    8300:	e5941000 	ldr	r1, [r4]
    8304:	e5942008 	ldr	r2, [r4, #8]
    8308:	e3a00001 	mov	r0, #1
    830c:	ebfffe0c 	bl	7b44 <syscall2>
    8310:	e1a00004 	mov	r0, r4
    8314:	ebfffecf 	bl	7e58 <str_free>
    8318:	e24bd010 	sub	sp, fp, #16
    831c:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
    8320:	e12fff1e 	bx	lr
    8324:	00007218 	andeq	r7, r0, r8, lsl r2
    8328:	ffffff98 			; <UNDEFINED> instruction: 0xffffff98

0000832c <_start>:
    832c:	e1a0c00d 	mov	ip, sp
    8330:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    8334:	e3a05000 	mov	r5, #0
    8338:	e24cb004 	sub	fp, ip, #4
    833c:	e24dd040 	sub	sp, sp, #64	; 0x40
    8340:	eb000a2a 	bl	abf0 <sys_signal_init>
    8344:	eb00115a 	bl	c8b4 <proc_init>
    8348:	e3a02002 	mov	r2, #2
    834c:	e3a00001 	mov	r0, #1
    8350:	e59f4198 	ldr	r4, [pc, #408]	; 84f0 <_start+0x1c4>
    8354:	e59f3198 	ldr	r3, [pc, #408]	; 84f4 <_start+0x1c8>
    8358:	e08f4004 	add	r4, pc, r4
    835c:	e5845000 	str	r5, [r4]
    8360:	e5845004 	str	r5, [r4, #4]
    8364:	e59f118c 	ldr	r1, [pc, #396]	; 84f8 <_start+0x1cc>
    8368:	e08f3003 	add	r3, pc, r3
    836c:	e793c001 	ldr	ip, [r3, r1]
    8370:	e58c4000 	str	r4, [ip]
    8374:	e5840008 	str	r0, [r4, #8]
    8378:	e584200c 	str	r2, [r4, #12]
    837c:	e59f1178 	ldr	r1, [pc, #376]	; 84fc <_start+0x1d0>
    8380:	e7931001 	ldr	r1, [r3, r1]
    8384:	e2840008 	add	r0, r4, #8
    8388:	e5842010 	str	r2, [r4, #16]
    838c:	e5842014 	str	r2, [r4, #20]
    8390:	e5810000 	str	r0, [r1]
    8394:	e59f2164 	ldr	r2, [pc, #356]	; 8500 <_start+0x1d4>
    8398:	e7933002 	ldr	r3, [r3, r2]
    839c:	e2842010 	add	r2, r4, #16
    83a0:	e5832000 	str	r2, [r3]
    83a4:	e5c45018 	strb	r5, [r4, #24]
    83a8:	e5845418 	str	r5, [r4, #1048]	; 0x418
    83ac:	eb001266 	bl	cd4c <getpid>
    83b0:	e59f314c 	ldr	r3, [pc, #332]	; 8504 <_start+0x1d8>
    83b4:	e1a01000 	mov	r1, r0
    83b8:	e2842018 	add	r2, r4, #24
    83bc:	e3a00016 	mov	r0, #22
    83c0:	ebfffdde 	bl	7b40 <syscall3>
    83c4:	e24b1064 	sub	r1, fp, #100	; 0x64
    83c8:	e1a00005 	mov	r0, r5
    83cc:	e1a08005 	mov	r8, r5
    83d0:	e1a07001 	mov	r7, r1
    83d4:	e59f912c 	ldr	r9, [pc, #300]	; 8508 <_start+0x1dc>
    83d8:	e5943418 	ldr	r3, [r4, #1048]	; 0x418
    83dc:	e08f9009 	add	r9, pc, r9
    83e0:	e2899018 	add	r9, r9, #24
    83e4:	e3a0e000 	mov	lr, #0
    83e8:	e084c003 	add	ip, r4, r3
    83ec:	e5dc2018 	ldrb	r2, [ip, #24]
    83f0:	e3520000 	cmp	r2, #0
    83f4:	e1a06003 	mov	r6, r3
    83f8:	e1a0500e 	mov	r5, lr
    83fc:	0a000009 	beq	8428 <_start+0xfc>
    8400:	e35e0000 	cmp	lr, #0
    8404:	e2833001 	add	r3, r3, #1
    8408:	0a00001d 	beq	8484 <_start+0x158>
    840c:	e3520022 	cmp	r2, #34	; 0x22
    8410:	0a00002f 	beq	84d4 <_start+0x1a8>
    8414:	e084c003 	add	ip, r4, r3
    8418:	e5dc2018 	ldrb	r2, [ip, #24]
    841c:	e3520000 	cmp	r2, #0
    8420:	e1a06003 	mov	r6, r3
    8424:	1afffff5 	bne	8400 <_start+0xd4>
    8428:	e3550000 	cmp	r5, #0
    842c:	0a00002b 	beq	84e0 <_start+0x1b4>
    8430:	e5d52000 	ldrb	r2, [r5]
    8434:	e3520000 	cmp	r2, #0
    8438:	0a000021 	beq	84c4 <_start+0x198>
    843c:	e2800001 	add	r0, r0, #1
    8440:	e3500010 	cmp	r0, #16
    8444:	e4875004 	str	r5, [r7], #4
    8448:	1affffe5 	bne	83e4 <_start+0xb8>
    844c:	e59f20b8 	ldr	r2, [pc, #184]	; 850c <_start+0x1e0>
    8450:	e08f2002 	add	r2, pc, r2
    8454:	e5823418 	str	r3, [r2, #1048]	; 0x418
    8458:	ebffdf28 	bl	100 <main>
    845c:	e1a04000 	mov	r4, r0
    8460:	e3a00000 	mov	r0, #0
    8464:	eb0015be 	bl	db64 <close>
    8468:	e3a00001 	mov	r0, #1
    846c:	eb0015bc 	bl	db64 <close>
    8470:	e1a00004 	mov	r0, r4
    8474:	eb001382 	bl	d284 <exit>
    8478:	e24bd024 	sub	sp, fp, #36	; 0x24
    847c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    8480:	e12fff1e 	bx	lr
    8484:	e3520020 	cmp	r2, #32
    8488:	0a000007 	beq	84ac <_start+0x180>
    848c:	e3550000 	cmp	r5, #0
    8490:	1affffdf 	bne	8414 <_start+0xe8>
    8494:	e3520022 	cmp	r2, #34	; 0x22
    8498:	02863002 	addeq	r3, r6, #2
    849c:	e2435001 	sub	r5, r3, #1
    84a0:	03a0e001 	moveq	lr, #1
    84a4:	e0895005 	add	r5, r9, r5
    84a8:	eaffffd9 	b	8414 <_start+0xe8>
    84ac:	e3550000 	cmp	r5, #0
    84b0:	0affffd7 	beq	8414 <_start+0xe8>
    84b4:	e5cce018 	strb	lr, [ip, #24]
    84b8:	e5d52000 	ldrb	r2, [r5]
    84bc:	e3520000 	cmp	r2, #0
    84c0:	1affffdd 	bne	843c <_start+0x110>
    84c4:	e59f2044 	ldr	r2, [pc, #68]	; 8510 <_start+0x1e4>
    84c8:	e08f2002 	add	r2, pc, r2
    84cc:	e5823418 	str	r3, [r2, #1048]	; 0x418
    84d0:	eaffffe0 	b	8458 <_start+0x12c>
    84d4:	e3550000 	cmp	r5, #0
    84d8:	e5cc8018 	strb	r8, [ip, #24]
    84dc:	1affffd3 	bne	8430 <_start+0x104>
    84e0:	e59f202c 	ldr	r2, [pc, #44]	; 8514 <_start+0x1e8>
    84e4:	e08f2002 	add	r2, pc, r2
    84e8:	e5823418 	str	r3, [r2, #1048]	; 0x418
    84ec:	eaffffd9 	b	8458 <_start+0x12c>
    84f0:	00044a38 	andeq	r4, r4, r8, lsr sl
    84f4:	0000f690 	muleq	r0, r0, r6
    84f8:	00000030 	andeq	r0, r0, r0, lsr r0
    84fc:	0000000c 	andeq	r0, r0, ip
    8500:	0000004c 	andeq	r0, r0, ip, asr #32
    8504:	000003ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    8508:	000449b4 			; <UNDEFINED> instruction: 0x000449b4
    850c:	00044940 	andeq	r4, r4, r0, asr #18
    8510:	000448c8 	andeq	r4, r4, r8, asr #17
    8514:	000448ac 	andeq	r4, r4, ip, lsr #17

00008518 <vfs_new_node>:
    8518:	e1a0c00d 	mov	ip, sp
    851c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    8520:	e24cb004 	sub	fp, ip, #4
    8524:	e24dd030 	sub	sp, sp, #48	; 0x30
    8528:	e1a07000 	mov	r7, r0
    852c:	eb000dc5 	bl	bc48 <get_proto_factor>
    8530:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8534:	e5903004 	ldr	r3, [r0, #4]
    8538:	e1a00004 	mov	r0, r4
    853c:	e1a0e00f 	mov	lr, pc
    8540:	e12fff13 	bx	r3
    8544:	e1a01007 	mov	r1, r7
    8548:	e3a0205c 	mov	r2, #92	; 0x5c
    854c:	e5903010 	ldr	r3, [r0, #16]
    8550:	e1a00004 	mov	r0, r4
    8554:	e1a0e00f 	mov	lr, pc
    8558:	e12fff13 	bx	r3
    855c:	eb000db9 	bl	bc48 <get_proto_factor>
    8560:	e24b5034 	sub	r5, fp, #52	; 0x34
    8564:	e5903004 	ldr	r3, [r0, #4]
    8568:	e1a00005 	mov	r0, r5
    856c:	e1a0e00f 	mov	lr, pc
    8570:	e12fff13 	bx	r3
    8574:	eb0010d4 	bl	c8cc <get_vfsd_pid>
    8578:	e1a02004 	mov	r2, r4
    857c:	e1a03005 	mov	r3, r5
    8580:	e3a01003 	mov	r1, #3
    8584:	eb0007ff 	bl	a588 <ipc_call>
    8588:	e1a06000 	mov	r6, r0
    858c:	eb000dad 	bl	bc48 <get_proto_factor>
    8590:	e590300c 	ldr	r3, [r0, #12]
    8594:	e1a00004 	mov	r0, r4
    8598:	e1a0e00f 	mov	lr, pc
    859c:	e12fff13 	bx	r3
    85a0:	e3560000 	cmp	r6, #0
    85a4:	01a01007 	moveq	r1, r7
    85a8:	01a00005 	moveq	r0, r5
    85ac:	03a0205c 	moveq	r2, #92	; 0x5c
    85b0:	0b000e02 	bleq	bdc0 <proto_read_to>
    85b4:	eb000da3 	bl	bc48 <get_proto_factor>
    85b8:	e590300c 	ldr	r3, [r0, #12]
    85bc:	e1a00005 	mov	r0, r5
    85c0:	e1a0e00f 	mov	lr, pc
    85c4:	e12fff13 	bx	r3
    85c8:	e1a00006 	mov	r0, r6
    85cc:	e24bd01c 	sub	sp, fp, #28
    85d0:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    85d4:	e12fff1e 	bx	lr

000085d8 <vfs_fullname>:
    85d8:	e1a0c00d 	mov	ip, sp
    85dc:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    85e0:	e1a04000 	mov	r4, r0
    85e4:	e59f009c 	ldr	r0, [pc, #156]	; 8688 <vfs_fullname+0xb0>
    85e8:	e24cb004 	sub	fp, ip, #4
    85ec:	e24ddf81 	sub	sp, sp, #516	; 0x204
    85f0:	e08f0000 	add	r0, pc, r0
    85f4:	ebfffda4 	bl	7c8c <str_new>
    85f8:	e5d43000 	ldrb	r3, [r4]
    85fc:	e353002f 	cmp	r3, #47	; 0x2f
    8600:	e1a05000 	mov	r5, r0
    8604:	0a00001c 	beq	867c <vfs_fullname+0xa4>
    8608:	e24b6f87 	sub	r6, fp, #540	; 0x21c
    860c:	e59f1078 	ldr	r1, [pc, #120]	; 868c <vfs_fullname+0xb4>
    8610:	e1a00006 	mov	r0, r6
    8614:	eb00119d 	bl	cc90 <getcwd>
    8618:	e1a00005 	mov	r0, r5
    861c:	e1a01006 	mov	r1, r6
    8620:	ebfffd8e 	bl	7c60 <str_cpy>
    8624:	e55b321b 	ldrb	r3, [fp, #-539]	; 0x21b
    8628:	e3530000 	cmp	r3, #0
    862c:	1a00000e 	bne	866c <vfs_fullname+0x94>
    8630:	e1a01004 	mov	r1, r4
    8634:	e1a00005 	mov	r0, r5
    8638:	ebfffdb5 	bl	7d14 <str_add>
    863c:	e59f404c 	ldr	r4, [pc, #76]	; 8690 <vfs_fullname+0xb8>
    8640:	e08f4004 	add	r4, pc, r4
    8644:	e5951000 	ldr	r1, [r5]
    8648:	e59f203c 	ldr	r2, [pc, #60]	; 868c <vfs_fullname+0xb4>
    864c:	e1a00004 	mov	r0, r4
    8650:	eb00181c 	bl	e6c8 <strncpy>
    8654:	e1a00005 	mov	r0, r5
    8658:	ebfffdfe 	bl	7e58 <str_free>
    865c:	e1a00004 	mov	r0, r4
    8660:	e24bd018 	sub	sp, fp, #24
    8664:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    8668:	e12fff1e 	bx	lr
    866c:	e1a00005 	mov	r0, r5
    8670:	e3a0102f 	mov	r1, #47	; 0x2f
    8674:	ebfffdcd 	bl	7db0 <str_addc>
    8678:	eaffffec 	b	8630 <vfs_fullname+0x58>
    867c:	e1a01004 	mov	r1, r4
    8680:	ebfffd76 	bl	7c60 <str_cpy>
    8684:	eaffffec 	b	863c <vfs_fullname+0x64>
    8688:	00006efc 	strdeq	r6, [r0], -ip
    868c:	000001ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    8690:	00044b78 	andeq	r4, r4, r8, ror fp

00008694 <vfs_open>:
    8694:	e1a0c00d 	mov	ip, sp
    8698:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    869c:	e24cb004 	sub	fp, ip, #4
    86a0:	e24dd034 	sub	sp, sp, #52	; 0x34
    86a4:	e1a05001 	mov	r5, r1
    86a8:	e1a06000 	mov	r6, r0
    86ac:	eb000d65 	bl	bc48 <get_proto_factor>
    86b0:	e24b404c 	sub	r4, fp, #76	; 0x4c
    86b4:	e5903004 	ldr	r3, [r0, #4]
    86b8:	e1a00004 	mov	r0, r4
    86bc:	e1a0e00f 	mov	lr, pc
    86c0:	e12fff13 	bx	r3
    86c4:	e3a0205c 	mov	r2, #92	; 0x5c
    86c8:	e1a01006 	mov	r1, r6
    86cc:	e5903010 	ldr	r3, [r0, #16]
    86d0:	e1a00004 	mov	r0, r4
    86d4:	e1a0e00f 	mov	lr, pc
    86d8:	e12fff13 	bx	r3
    86dc:	e1a01005 	mov	r1, r5
    86e0:	e5903014 	ldr	r3, [r0, #20]
    86e4:	e1a00004 	mov	r0, r4
    86e8:	e1a0e00f 	mov	lr, pc
    86ec:	e12fff13 	bx	r3
    86f0:	eb000d54 	bl	bc48 <get_proto_factor>
    86f4:	e24b5034 	sub	r5, fp, #52	; 0x34
    86f8:	e5903004 	ldr	r3, [r0, #4]
    86fc:	e1a00005 	mov	r0, r5
    8700:	e1a0e00f 	mov	lr, pc
    8704:	e12fff13 	bx	r3
    8708:	eb00106f 	bl	c8cc <get_vfsd_pid>
    870c:	e1a02004 	mov	r2, r4
    8710:	e1a03005 	mov	r3, r5
    8714:	e3a01004 	mov	r1, #4
    8718:	eb00079a 	bl	a588 <ipc_call>
    871c:	e1a06000 	mov	r6, r0
    8720:	eb000d48 	bl	bc48 <get_proto_factor>
    8724:	e590300c 	ldr	r3, [r0, #12]
    8728:	e1a00004 	mov	r0, r4
    872c:	e1a0e00f 	mov	lr, pc
    8730:	e12fff13 	bx	r3
    8734:	e3560000 	cmp	r6, #0
    8738:	1a000002 	bne	8748 <vfs_open+0xb4>
    873c:	e1a00005 	mov	r0, r5
    8740:	eb000e09 	bl	bf6c <proto_read_int>
    8744:	e1a06000 	mov	r6, r0
    8748:	eb000d3e 	bl	bc48 <get_proto_factor>
    874c:	e590300c 	ldr	r3, [r0, #12]
    8750:	e1a00005 	mov	r0, r5
    8754:	e1a0e00f 	mov	lr, pc
    8758:	e12fff13 	bx	r3
    875c:	e1a00006 	mov	r0, r6
    8760:	e24bd018 	sub	sp, fp, #24
    8764:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    8768:	e12fff1e 	bx	lr

0000876c <vfs_dup>:
    876c:	e1a0c00d 	mov	ip, sp
    8770:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    8774:	e24cb004 	sub	fp, ip, #4
    8778:	e24dd034 	sub	sp, sp, #52	; 0x34
    877c:	e1a05000 	mov	r5, r0
    8780:	eb000d30 	bl	bc48 <get_proto_factor>
    8784:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8788:	e5903004 	ldr	r3, [r0, #4]
    878c:	e1a00004 	mov	r0, r4
    8790:	e1a0e00f 	mov	lr, pc
    8794:	e12fff13 	bx	r3
    8798:	e1a01005 	mov	r1, r5
    879c:	e5903014 	ldr	r3, [r0, #20]
    87a0:	e1a00004 	mov	r0, r4
    87a4:	e1a0e00f 	mov	lr, pc
    87a8:	e12fff13 	bx	r3
    87ac:	eb000d25 	bl	bc48 <get_proto_factor>
    87b0:	e24b5034 	sub	r5, fp, #52	; 0x34
    87b4:	e5903004 	ldr	r3, [r0, #4]
    87b8:	e1a00005 	mov	r0, r5
    87bc:	e1a0e00f 	mov	lr, pc
    87c0:	e12fff13 	bx	r3
    87c4:	eb001040 	bl	c8cc <get_vfsd_pid>
    87c8:	e1a02004 	mov	r2, r4
    87cc:	e1a03005 	mov	r3, r5
    87d0:	e3a01008 	mov	r1, #8
    87d4:	eb00076b 	bl	a588 <ipc_call>
    87d8:	e1a06000 	mov	r6, r0
    87dc:	eb000d19 	bl	bc48 <get_proto_factor>
    87e0:	e590300c 	ldr	r3, [r0, #12]
    87e4:	e1a00004 	mov	r0, r4
    87e8:	e1a0e00f 	mov	lr, pc
    87ec:	e12fff13 	bx	r3
    87f0:	e3560000 	cmp	r6, #0
    87f4:	1a000002 	bne	8804 <vfs_dup+0x98>
    87f8:	e1a00005 	mov	r0, r5
    87fc:	eb000dda 	bl	bf6c <proto_read_int>
    8800:	e1a06000 	mov	r6, r0
    8804:	eb000d0f 	bl	bc48 <get_proto_factor>
    8808:	e590300c 	ldr	r3, [r0, #12]
    880c:	e1a00005 	mov	r0, r5
    8810:	e1a0e00f 	mov	lr, pc
    8814:	e12fff13 	bx	r3
    8818:	e1a00006 	mov	r0, r6
    881c:	e24bd018 	sub	sp, fp, #24
    8820:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    8824:	e12fff1e 	bx	lr

00008828 <vfs_close>:
    8828:	e1a0c00d 	mov	ip, sp
    882c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    8830:	e24cb004 	sub	fp, ip, #4
    8834:	e24dd018 	sub	sp, sp, #24
    8838:	e1a05000 	mov	r5, r0
    883c:	eb000d01 	bl	bc48 <get_proto_factor>
    8840:	e24b402c 	sub	r4, fp, #44	; 0x2c
    8844:	e5903004 	ldr	r3, [r0, #4]
    8848:	e1a00004 	mov	r0, r4
    884c:	e1a0e00f 	mov	lr, pc
    8850:	e12fff13 	bx	r3
    8854:	e1a01005 	mov	r1, r5
    8858:	e5903014 	ldr	r3, [r0, #20]
    885c:	e1a00004 	mov	r0, r4
    8860:	e1a0e00f 	mov	lr, pc
    8864:	e12fff13 	bx	r3
    8868:	eb001017 	bl	c8cc <get_vfsd_pid>
    886c:	e1a02004 	mov	r2, r4
    8870:	e3a0100a 	mov	r1, #10
    8874:	e3a03000 	mov	r3, #0
    8878:	eb000742 	bl	a588 <ipc_call>
    887c:	e1a05000 	mov	r5, r0
    8880:	eb000cf0 	bl	bc48 <get_proto_factor>
    8884:	e590300c 	ldr	r3, [r0, #12]
    8888:	e1a00004 	mov	r0, r4
    888c:	e1a0e00f 	mov	lr, pc
    8890:	e12fff13 	bx	r3
    8894:	e1a00005 	mov	r0, r5
    8898:	e24bd014 	sub	sp, fp, #20
    889c:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    88a0:	e12fff1e 	bx	lr

000088a4 <vfs_dup2>:
    88a4:	e1a0c00d 	mov	ip, sp
    88a8:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    88ac:	e24cb004 	sub	fp, ip, #4
    88b0:	e24dd034 	sub	sp, sp, #52	; 0x34
    88b4:	e1a05001 	mov	r5, r1
    88b8:	e1a06000 	mov	r6, r0
    88bc:	eb000ce1 	bl	bc48 <get_proto_factor>
    88c0:	e24b404c 	sub	r4, fp, #76	; 0x4c
    88c4:	e5903004 	ldr	r3, [r0, #4]
    88c8:	e1a00004 	mov	r0, r4
    88cc:	e1a0e00f 	mov	lr, pc
    88d0:	e12fff13 	bx	r3
    88d4:	e1a01006 	mov	r1, r6
    88d8:	e5903014 	ldr	r3, [r0, #20]
    88dc:	e1a00004 	mov	r0, r4
    88e0:	e1a0e00f 	mov	lr, pc
    88e4:	e12fff13 	bx	r3
    88e8:	e1a01005 	mov	r1, r5
    88ec:	e5903014 	ldr	r3, [r0, #20]
    88f0:	e1a00004 	mov	r0, r4
    88f4:	e1a0e00f 	mov	lr, pc
    88f8:	e12fff13 	bx	r3
    88fc:	eb000cd1 	bl	bc48 <get_proto_factor>
    8900:	e24b5034 	sub	r5, fp, #52	; 0x34
    8904:	e5903004 	ldr	r3, [r0, #4]
    8908:	e1a00005 	mov	r0, r5
    890c:	e1a0e00f 	mov	lr, pc
    8910:	e12fff13 	bx	r3
    8914:	eb000fec 	bl	c8cc <get_vfsd_pid>
    8918:	e1a02004 	mov	r2, r4
    891c:	e1a03005 	mov	r3, r5
    8920:	e3a01009 	mov	r1, #9
    8924:	eb000717 	bl	a588 <ipc_call>
    8928:	e1a06000 	mov	r6, r0
    892c:	eb000cc5 	bl	bc48 <get_proto_factor>
    8930:	e590300c 	ldr	r3, [r0, #12]
    8934:	e1a00004 	mov	r0, r4
    8938:	e1a0e00f 	mov	lr, pc
    893c:	e12fff13 	bx	r3
    8940:	e3560000 	cmp	r6, #0
    8944:	1a000002 	bne	8954 <vfs_dup2+0xb0>
    8948:	e1a00005 	mov	r0, r5
    894c:	eb000d86 	bl	bf6c <proto_read_int>
    8950:	e1a06000 	mov	r6, r0
    8954:	eb000cbb 	bl	bc48 <get_proto_factor>
    8958:	e590300c 	ldr	r3, [r0, #12]
    895c:	e1a00005 	mov	r0, r5
    8960:	e1a0e00f 	mov	lr, pc
    8964:	e12fff13 	bx	r3
    8968:	e1a00006 	mov	r0, r6
    896c:	e24bd018 	sub	sp, fp, #24
    8970:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    8974:	e12fff1e 	bx	lr

00008978 <vfs_open_pipe>:
    8978:	e1a0c00d 	mov	ip, sp
    897c:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    8980:	e24cb004 	sub	fp, ip, #4
    8984:	e24dd01c 	sub	sp, sp, #28
    8988:	e1a06000 	mov	r6, r0
    898c:	eb000cad 	bl	bc48 <get_proto_factor>
    8990:	e24b4034 	sub	r4, fp, #52	; 0x34
    8994:	e5903004 	ldr	r3, [r0, #4]
    8998:	e1a00004 	mov	r0, r4
    899c:	e1a0e00f 	mov	lr, pc
    89a0:	e12fff13 	bx	r3
    89a4:	eb000fc8 	bl	c8cc <get_vfsd_pid>
    89a8:	e1a03004 	mov	r3, r4
    89ac:	e3a01005 	mov	r1, #5
    89b0:	e3a02000 	mov	r2, #0
    89b4:	eb0006f3 	bl	a588 <ipc_call>
    89b8:	e2505000 	subs	r5, r0, #0
    89bc:	0a000008 	beq	89e4 <vfs_open_pipe+0x6c>
    89c0:	eb000ca0 	bl	bc48 <get_proto_factor>
    89c4:	e590300c 	ldr	r3, [r0, #12]
    89c8:	e1a00004 	mov	r0, r4
    89cc:	e1a0e00f 	mov	lr, pc
    89d0:	e12fff13 	bx	r3
    89d4:	e1a00005 	mov	r0, r5
    89d8:	e24bd018 	sub	sp, fp, #24
    89dc:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    89e0:	e12fff1e 	bx	lr
    89e4:	e1a00004 	mov	r0, r4
    89e8:	eb000d5f 	bl	bf6c <proto_read_int>
    89ec:	e3500000 	cmp	r0, #0
    89f0:	1a000006 	bne	8a10 <vfs_open_pipe+0x98>
    89f4:	e1a00004 	mov	r0, r4
    89f8:	eb000d5b 	bl	bf6c <proto_read_int>
    89fc:	e5860000 	str	r0, [r6]
    8a00:	e1a00004 	mov	r0, r4
    8a04:	eb000d58 	bl	bf6c <proto_read_int>
    8a08:	e5860004 	str	r0, [r6, #4]
    8a0c:	eaffffeb 	b	89c0 <vfs_open_pipe+0x48>
    8a10:	e3e05000 	mvn	r5, #0
    8a14:	eaffffe9 	b	89c0 <vfs_open_pipe+0x48>

00008a18 <vfs_get>:
    8a18:	e1a0c00d 	mov	ip, sp
    8a1c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    8a20:	e24cb004 	sub	fp, ip, #4
    8a24:	e24dd030 	sub	sp, sp, #48	; 0x30
    8a28:	e1a07001 	mov	r7, r1
    8a2c:	ebfffee9 	bl	85d8 <vfs_fullname>
    8a30:	e1a05000 	mov	r5, r0
    8a34:	eb000c83 	bl	bc48 <get_proto_factor>
    8a38:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8a3c:	e5903004 	ldr	r3, [r0, #4]
    8a40:	e1a00004 	mov	r0, r4
    8a44:	e1a0e00f 	mov	lr, pc
    8a48:	e12fff13 	bx	r3
    8a4c:	e1a01005 	mov	r1, r5
    8a50:	e5903018 	ldr	r3, [r0, #24]
    8a54:	e1a00004 	mov	r0, r4
    8a58:	e1a0e00f 	mov	lr, pc
    8a5c:	e12fff13 	bx	r3
    8a60:	eb000c78 	bl	bc48 <get_proto_factor>
    8a64:	e24b5034 	sub	r5, fp, #52	; 0x34
    8a68:	e5903004 	ldr	r3, [r0, #4]
    8a6c:	e1a00005 	mov	r0, r5
    8a70:	e1a0e00f 	mov	lr, pc
    8a74:	e12fff13 	bx	r3
    8a78:	eb000f93 	bl	c8cc <get_vfsd_pid>
    8a7c:	e1a02004 	mov	r2, r4
    8a80:	e1a03005 	mov	r3, r5
    8a84:	e3a01000 	mov	r1, #0
    8a88:	eb0006be 	bl	a588 <ipc_call>
    8a8c:	e1a06000 	mov	r6, r0
    8a90:	eb000c6c 	bl	bc48 <get_proto_factor>
    8a94:	e590300c 	ldr	r3, [r0, #12]
    8a98:	e1a00004 	mov	r0, r4
    8a9c:	e1a0e00f 	mov	lr, pc
    8aa0:	e12fff13 	bx	r3
    8aa4:	e3560000 	cmp	r6, #0
    8aa8:	0a000008 	beq	8ad0 <vfs_get+0xb8>
    8aac:	eb000c65 	bl	bc48 <get_proto_factor>
    8ab0:	e590300c 	ldr	r3, [r0, #12]
    8ab4:	e1a00005 	mov	r0, r5
    8ab8:	e1a0e00f 	mov	lr, pc
    8abc:	e12fff13 	bx	r3
    8ac0:	e1a00006 	mov	r0, r6
    8ac4:	e24bd01c 	sub	sp, fp, #28
    8ac8:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    8acc:	e12fff1e 	bx	lr
    8ad0:	e1a00005 	mov	r0, r5
    8ad4:	eb000d24 	bl	bf6c <proto_read_int>
    8ad8:	e3500000 	cmp	r0, #0
    8adc:	0a000006 	beq	8afc <vfs_get+0xe4>
    8ae0:	e3570000 	cmp	r7, #0
    8ae4:	0afffff0 	beq	8aac <vfs_get+0x94>
    8ae8:	e1a01007 	mov	r1, r7
    8aec:	e1a00005 	mov	r0, r5
    8af0:	e3a0205c 	mov	r2, #92	; 0x5c
    8af4:	eb000cb1 	bl	bdc0 <proto_read_to>
    8af8:	eaffffeb 	b	8aac <vfs_get+0x94>
    8afc:	e3e06000 	mvn	r6, #0
    8b00:	eaffffe9 	b	8aac <vfs_get+0x94>

00008b04 <vfs_access>:
    8b04:	e3a01000 	mov	r1, #0
    8b08:	eaffffc2 	b	8a18 <vfs_get>

00008b0c <vfs_kids>:
    8b0c:	e1a0c00d 	mov	ip, sp
    8b10:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    8b14:	e24cb004 	sub	fp, ip, #4
    8b18:	e24dd030 	sub	sp, sp, #48	; 0x30
    8b1c:	e1a05000 	mov	r5, r0
    8b20:	e1a07001 	mov	r7, r1
    8b24:	eb000c47 	bl	bc48 <get_proto_factor>
    8b28:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8b2c:	e5903004 	ldr	r3, [r0, #4]
    8b30:	e1a00004 	mov	r0, r4
    8b34:	e1a0e00f 	mov	lr, pc
    8b38:	e12fff13 	bx	r3
    8b3c:	e1a01005 	mov	r1, r5
    8b40:	e3a0205c 	mov	r2, #92	; 0x5c
    8b44:	e5903010 	ldr	r3, [r0, #16]
    8b48:	e1a00004 	mov	r0, r4
    8b4c:	e1a0e00f 	mov	lr, pc
    8b50:	e12fff13 	bx	r3
    8b54:	eb000c3b 	bl	bc48 <get_proto_factor>
    8b58:	e24b5034 	sub	r5, fp, #52	; 0x34
    8b5c:	e5903004 	ldr	r3, [r0, #4]
    8b60:	e1a00005 	mov	r0, r5
    8b64:	e1a0e00f 	mov	lr, pc
    8b68:	e12fff13 	bx	r3
    8b6c:	eb000f56 	bl	c8cc <get_vfsd_pid>
    8b70:	e1a02004 	mov	r2, r4
    8b74:	e1a03005 	mov	r3, r5
    8b78:	e3a01012 	mov	r1, #18
    8b7c:	eb000681 	bl	a588 <ipc_call>
    8b80:	e1a06000 	mov	r6, r0
    8b84:	eb000c2f 	bl	bc48 <get_proto_factor>
    8b88:	e590300c 	ldr	r3, [r0, #12]
    8b8c:	e1a00004 	mov	r0, r4
    8b90:	e1a0e00f 	mov	lr, pc
    8b94:	e12fff13 	bx	r3
    8b98:	e3560000 	cmp	r6, #0
    8b9c:	1a000017 	bne	8c00 <vfs_kids+0xf4>
    8ba0:	e1a00005 	mov	r0, r5
    8ba4:	eb000cf0 	bl	bf6c <proto_read_int>
    8ba8:	e3500000 	cmp	r0, #0
    8bac:	e5870000 	str	r0, [r7]
    8bb0:	0a000012 	beq	8c00 <vfs_kids+0xf4>
    8bb4:	e0804080 	add	r4, r0, r0, lsl #1
    8bb8:	e0600184 	rsb	r0, r0, r4, lsl #3
    8bbc:	e1a04100 	lsl	r4, r0, #2
    8bc0:	e1a00004 	mov	r0, r4
    8bc4:	eb0011c0 	bl	d2cc <malloc>
    8bc8:	e1a06000 	mov	r6, r0
    8bcc:	e1a02004 	mov	r2, r4
    8bd0:	e1a00005 	mov	r0, r5
    8bd4:	e1a01006 	mov	r1, r6
    8bd8:	eb000c78 	bl	bdc0 <proto_read_to>
    8bdc:	eb000c19 	bl	bc48 <get_proto_factor>
    8be0:	e590300c 	ldr	r3, [r0, #12]
    8be4:	e1a00005 	mov	r0, r5
    8be8:	e1a0e00f 	mov	lr, pc
    8bec:	e12fff13 	bx	r3
    8bf0:	e1a00006 	mov	r0, r6
    8bf4:	e24bd01c 	sub	sp, fp, #28
    8bf8:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    8bfc:	e12fff1e 	bx	lr
    8c00:	e3a06000 	mov	r6, #0
    8c04:	eafffff4 	b	8bdc <vfs_kids+0xd0>

00008c08 <vfs_set>:
    8c08:	e1a0c00d 	mov	ip, sp
    8c0c:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    8c10:	e24cb004 	sub	fp, ip, #4
    8c14:	e24dd034 	sub	sp, sp, #52	; 0x34
    8c18:	e1a05000 	mov	r5, r0
    8c1c:	eb000c09 	bl	bc48 <get_proto_factor>
    8c20:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8c24:	e5903004 	ldr	r3, [r0, #4]
    8c28:	e1a00004 	mov	r0, r4
    8c2c:	e1a0e00f 	mov	lr, pc
    8c30:	e12fff13 	bx	r3
    8c34:	e1a01005 	mov	r1, r5
    8c38:	e3a0205c 	mov	r2, #92	; 0x5c
    8c3c:	e5903010 	ldr	r3, [r0, #16]
    8c40:	e1a00004 	mov	r0, r4
    8c44:	e1a0e00f 	mov	lr, pc
    8c48:	e12fff13 	bx	r3
    8c4c:	eb000bfd 	bl	bc48 <get_proto_factor>
    8c50:	e24b5034 	sub	r5, fp, #52	; 0x34
    8c54:	e5903004 	ldr	r3, [r0, #4]
    8c58:	e1a00005 	mov	r0, r5
    8c5c:	e1a0e00f 	mov	lr, pc
    8c60:	e12fff13 	bx	r3
    8c64:	eb000f18 	bl	c8cc <get_vfsd_pid>
    8c68:	e1a02004 	mov	r2, r4
    8c6c:	e1a03005 	mov	r3, r5
    8c70:	e3a01002 	mov	r1, #2
    8c74:	eb000643 	bl	a588 <ipc_call>
    8c78:	e1a06000 	mov	r6, r0
    8c7c:	eb000bf1 	bl	bc48 <get_proto_factor>
    8c80:	e590300c 	ldr	r3, [r0, #12]
    8c84:	e1a00004 	mov	r0, r4
    8c88:	e1a0e00f 	mov	lr, pc
    8c8c:	e12fff13 	bx	r3
    8c90:	e3560000 	cmp	r6, #0
    8c94:	1a000002 	bne	8ca4 <vfs_set+0x9c>
    8c98:	e1a00005 	mov	r0, r5
    8c9c:	eb000cb2 	bl	bf6c <proto_read_int>
    8ca0:	e1a06000 	mov	r6, r0
    8ca4:	eb000be7 	bl	bc48 <get_proto_factor>
    8ca8:	e590300c 	ldr	r3, [r0, #12]
    8cac:	e1a00005 	mov	r0, r5
    8cb0:	e1a0e00f 	mov	lr, pc
    8cb4:	e12fff13 	bx	r3
    8cb8:	e1a00006 	mov	r0, r6
    8cbc:	e24bd018 	sub	sp, fp, #24
    8cc0:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    8cc4:	e12fff1e 	bx	lr

00008cc8 <vfs_add>:
    8cc8:	e1a0c00d 	mov	ip, sp
    8ccc:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    8cd0:	e24cb004 	sub	fp, ip, #4
    8cd4:	e24dd034 	sub	sp, sp, #52	; 0x34
    8cd8:	e1a05001 	mov	r5, r1
    8cdc:	e1a06000 	mov	r6, r0
    8ce0:	eb000bd8 	bl	bc48 <get_proto_factor>
    8ce4:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8ce8:	e5903004 	ldr	r3, [r0, #4]
    8cec:	e1a00004 	mov	r0, r4
    8cf0:	e1a0e00f 	mov	lr, pc
    8cf4:	e12fff13 	bx	r3
    8cf8:	e1a01006 	mov	r1, r6
    8cfc:	e5903010 	ldr	r3, [r0, #16]
    8d00:	e3a0205c 	mov	r2, #92	; 0x5c
    8d04:	e1a00004 	mov	r0, r4
    8d08:	e1a0e00f 	mov	lr, pc
    8d0c:	e12fff13 	bx	r3
    8d10:	e1a01005 	mov	r1, r5
    8d14:	e3a0205c 	mov	r2, #92	; 0x5c
    8d18:	e5903010 	ldr	r3, [r0, #16]
    8d1c:	e1a00004 	mov	r0, r4
    8d20:	e1a0e00f 	mov	lr, pc
    8d24:	e12fff13 	bx	r3
    8d28:	eb000bc6 	bl	bc48 <get_proto_factor>
    8d2c:	e24b5034 	sub	r5, fp, #52	; 0x34
    8d30:	e5903004 	ldr	r3, [r0, #4]
    8d34:	e1a00005 	mov	r0, r5
    8d38:	e1a0e00f 	mov	lr, pc
    8d3c:	e12fff13 	bx	r3
    8d40:	eb000ee1 	bl	c8cc <get_vfsd_pid>
    8d44:	e1a02004 	mov	r2, r4
    8d48:	e1a03005 	mov	r3, r5
    8d4c:	e3a0100d 	mov	r1, #13
    8d50:	eb00060c 	bl	a588 <ipc_call>
    8d54:	e1a06000 	mov	r6, r0
    8d58:	eb000bba 	bl	bc48 <get_proto_factor>
    8d5c:	e590300c 	ldr	r3, [r0, #12]
    8d60:	e1a00004 	mov	r0, r4
    8d64:	e1a0e00f 	mov	lr, pc
    8d68:	e12fff13 	bx	r3
    8d6c:	e3560000 	cmp	r6, #0
    8d70:	1a000002 	bne	8d80 <vfs_add+0xb8>
    8d74:	e1a00005 	mov	r0, r5
    8d78:	eb000c7b 	bl	bf6c <proto_read_int>
    8d7c:	e1a06000 	mov	r6, r0
    8d80:	eb000bb0 	bl	bc48 <get_proto_factor>
    8d84:	e590300c 	ldr	r3, [r0, #12]
    8d88:	e1a00005 	mov	r0, r5
    8d8c:	e1a0e00f 	mov	lr, pc
    8d90:	e12fff13 	bx	r3
    8d94:	e1a00006 	mov	r0, r6
    8d98:	e24bd018 	sub	sp, fp, #24
    8d9c:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    8da0:	e12fff1e 	bx	lr

00008da4 <vfs_del>:
    8da4:	e1a0c00d 	mov	ip, sp
    8da8:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    8dac:	e24cb004 	sub	fp, ip, #4
    8db0:	e24dd034 	sub	sp, sp, #52	; 0x34
    8db4:	e1a05000 	mov	r5, r0
    8db8:	eb000ba2 	bl	bc48 <get_proto_factor>
    8dbc:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8dc0:	e5903004 	ldr	r3, [r0, #4]
    8dc4:	e1a00004 	mov	r0, r4
    8dc8:	e1a0e00f 	mov	lr, pc
    8dcc:	e12fff13 	bx	r3
    8dd0:	e1a01005 	mov	r1, r5
    8dd4:	e3a0205c 	mov	r2, #92	; 0x5c
    8dd8:	e5903010 	ldr	r3, [r0, #16]
    8ddc:	e1a00004 	mov	r0, r4
    8de0:	e1a0e00f 	mov	lr, pc
    8de4:	e12fff13 	bx	r3
    8de8:	eb000b96 	bl	bc48 <get_proto_factor>
    8dec:	e24b5034 	sub	r5, fp, #52	; 0x34
    8df0:	e5903004 	ldr	r3, [r0, #4]
    8df4:	e1a00005 	mov	r0, r5
    8df8:	e1a0e00f 	mov	lr, pc
    8dfc:	e12fff13 	bx	r3
    8e00:	eb000eb1 	bl	c8cc <get_vfsd_pid>
    8e04:	e1a02004 	mov	r2, r4
    8e08:	e1a03005 	mov	r3, r5
    8e0c:	e3a0100e 	mov	r1, #14
    8e10:	eb0005dc 	bl	a588 <ipc_call>
    8e14:	e1a06000 	mov	r6, r0
    8e18:	eb000b8a 	bl	bc48 <get_proto_factor>
    8e1c:	e590300c 	ldr	r3, [r0, #12]
    8e20:	e1a00004 	mov	r0, r4
    8e24:	e1a0e00f 	mov	lr, pc
    8e28:	e12fff13 	bx	r3
    8e2c:	e3560000 	cmp	r6, #0
    8e30:	1a000002 	bne	8e40 <vfs_del+0x9c>
    8e34:	e1a00005 	mov	r0, r5
    8e38:	eb000c4b 	bl	bf6c <proto_read_int>
    8e3c:	e1a06000 	mov	r6, r0
    8e40:	eb000b80 	bl	bc48 <get_proto_factor>
    8e44:	e590300c 	ldr	r3, [r0, #12]
    8e48:	e1a00005 	mov	r0, r5
    8e4c:	e1a0e00f 	mov	lr, pc
    8e50:	e12fff13 	bx	r3
    8e54:	e1a00006 	mov	r0, r6
    8e58:	e24bd018 	sub	sp, fp, #24
    8e5c:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    8e60:	e12fff1e 	bx	lr

00008e64 <vfs_get_mount_by_id>:
    8e64:	e1a0c00d 	mov	ip, sp
    8e68:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    8e6c:	e24cb004 	sub	fp, ip, #4
    8e70:	e24dd030 	sub	sp, sp, #48	; 0x30
    8e74:	e1a05000 	mov	r5, r0
    8e78:	e1a07001 	mov	r7, r1
    8e7c:	eb000b71 	bl	bc48 <get_proto_factor>
    8e80:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8e84:	e5903004 	ldr	r3, [r0, #4]
    8e88:	e1a00004 	mov	r0, r4
    8e8c:	e1a0e00f 	mov	lr, pc
    8e90:	e12fff13 	bx	r3
    8e94:	e1a01005 	mov	r1, r5
    8e98:	e5903014 	ldr	r3, [r0, #20]
    8e9c:	e1a00004 	mov	r0, r4
    8ea0:	e1a0e00f 	mov	lr, pc
    8ea4:	e12fff13 	bx	r3
    8ea8:	eb000b66 	bl	bc48 <get_proto_factor>
    8eac:	e24b5034 	sub	r5, fp, #52	; 0x34
    8eb0:	e5903004 	ldr	r3, [r0, #4]
    8eb4:	e1a00005 	mov	r0, r5
    8eb8:	e1a0e00f 	mov	lr, pc
    8ebc:	e12fff13 	bx	r3
    8ec0:	eb000e81 	bl	c8cc <get_vfsd_pid>
    8ec4:	e1a02004 	mov	r2, r4
    8ec8:	e1a03005 	mov	r3, r5
    8ecc:	e3a01011 	mov	r1, #17
    8ed0:	eb0005ac 	bl	a588 <ipc_call>
    8ed4:	e1a06000 	mov	r6, r0
    8ed8:	eb000b5a 	bl	bc48 <get_proto_factor>
    8edc:	e590300c 	ldr	r3, [r0, #12]
    8ee0:	e1a00004 	mov	r0, r4
    8ee4:	e1a0e00f 	mov	lr, pc
    8ee8:	e12fff13 	bx	r3
    8eec:	e3560000 	cmp	r6, #0
    8ef0:	0a000008 	beq	8f18 <vfs_get_mount_by_id+0xb4>
    8ef4:	eb000b53 	bl	bc48 <get_proto_factor>
    8ef8:	e590300c 	ldr	r3, [r0, #12]
    8efc:	e1a00005 	mov	r0, r5
    8f00:	e1a0e00f 	mov	lr, pc
    8f04:	e12fff13 	bx	r3
    8f08:	e1a00006 	mov	r0, r6
    8f0c:	e24bd01c 	sub	sp, fp, #28
    8f10:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    8f14:	e12fff1e 	bx	lr
    8f18:	e1a00005 	mov	r0, r5
    8f1c:	eb000c12 	bl	bf6c <proto_read_int>
    8f20:	e2506000 	subs	r6, r0, #0
    8f24:	1afffff2 	bne	8ef4 <vfs_get_mount_by_id+0x90>
    8f28:	e1a01007 	mov	r1, r7
    8f2c:	e1a00005 	mov	r0, r5
    8f30:	e3a02f82 	mov	r2, #520	; 0x208
    8f34:	eb000ba1 	bl	bdc0 <proto_read_to>
    8f38:	eaffffed 	b	8ef4 <vfs_get_mount_by_id+0x90>

00008f3c <vfs_mount>:
    8f3c:	e1a0c00d 	mov	ip, sp
    8f40:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    8f44:	e24cb004 	sub	fp, ip, #4
    8f48:	e24dd034 	sub	sp, sp, #52	; 0x34
    8f4c:	e1a05001 	mov	r5, r1
    8f50:	e1a06000 	mov	r6, r0
    8f54:	eb000b3b 	bl	bc48 <get_proto_factor>
    8f58:	e24b404c 	sub	r4, fp, #76	; 0x4c
    8f5c:	e5903004 	ldr	r3, [r0, #4]
    8f60:	e1a00004 	mov	r0, r4
    8f64:	e1a0e00f 	mov	lr, pc
    8f68:	e12fff13 	bx	r3
    8f6c:	e1a01006 	mov	r1, r6
    8f70:	e5903010 	ldr	r3, [r0, #16]
    8f74:	e3a0205c 	mov	r2, #92	; 0x5c
    8f78:	e1a00004 	mov	r0, r4
    8f7c:	e1a0e00f 	mov	lr, pc
    8f80:	e12fff13 	bx	r3
    8f84:	e1a01005 	mov	r1, r5
    8f88:	e3a0205c 	mov	r2, #92	; 0x5c
    8f8c:	e5903010 	ldr	r3, [r0, #16]
    8f90:	e1a00004 	mov	r0, r4
    8f94:	e1a0e00f 	mov	lr, pc
    8f98:	e12fff13 	bx	r3
    8f9c:	eb000b29 	bl	bc48 <get_proto_factor>
    8fa0:	e24b5034 	sub	r5, fp, #52	; 0x34
    8fa4:	e5903004 	ldr	r3, [r0, #4]
    8fa8:	e1a00005 	mov	r0, r5
    8fac:	e1a0e00f 	mov	lr, pc
    8fb0:	e12fff13 	bx	r3
    8fb4:	eb000e44 	bl	c8cc <get_vfsd_pid>
    8fb8:	e1a02004 	mov	r2, r4
    8fbc:	e1a03005 	mov	r3, r5
    8fc0:	e3a0100f 	mov	r1, #15
    8fc4:	eb00056f 	bl	a588 <ipc_call>
    8fc8:	e1a06000 	mov	r6, r0
    8fcc:	eb000b1d 	bl	bc48 <get_proto_factor>
    8fd0:	e590300c 	ldr	r3, [r0, #12]
    8fd4:	e1a00004 	mov	r0, r4
    8fd8:	e1a0e00f 	mov	lr, pc
    8fdc:	e12fff13 	bx	r3
    8fe0:	e3560000 	cmp	r6, #0
    8fe4:	1a000002 	bne	8ff4 <vfs_mount+0xb8>
    8fe8:	e1a00005 	mov	r0, r5
    8fec:	eb000bde 	bl	bf6c <proto_read_int>
    8ff0:	e1a06000 	mov	r6, r0
    8ff4:	eb000b13 	bl	bc48 <get_proto_factor>
    8ff8:	e590300c 	ldr	r3, [r0, #12]
    8ffc:	e1a00005 	mov	r0, r5
    9000:	e1a0e00f 	mov	lr, pc
    9004:	e12fff13 	bx	r3
    9008:	e1a00006 	mov	r0, r6
    900c:	e24bd018 	sub	sp, fp, #24
    9010:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    9014:	e12fff1e 	bx	lr

00009018 <vfs_umount>:
    9018:	e1a0c00d 	mov	ip, sp
    901c:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    9020:	e24cb004 	sub	fp, ip, #4
    9024:	e24dd034 	sub	sp, sp, #52	; 0x34
    9028:	e1a05000 	mov	r5, r0
    902c:	eb000b05 	bl	bc48 <get_proto_factor>
    9030:	e24b404c 	sub	r4, fp, #76	; 0x4c
    9034:	e5903004 	ldr	r3, [r0, #4]
    9038:	e1a00004 	mov	r0, r4
    903c:	e1a0e00f 	mov	lr, pc
    9040:	e12fff13 	bx	r3
    9044:	e1a01005 	mov	r1, r5
    9048:	e3a0205c 	mov	r2, #92	; 0x5c
    904c:	e5903010 	ldr	r3, [r0, #16]
    9050:	e1a00004 	mov	r0, r4
    9054:	e1a0e00f 	mov	lr, pc
    9058:	e12fff13 	bx	r3
    905c:	eb000af9 	bl	bc48 <get_proto_factor>
    9060:	e24b5034 	sub	r5, fp, #52	; 0x34
    9064:	e5903004 	ldr	r3, [r0, #4]
    9068:	e1a00005 	mov	r0, r5
    906c:	e1a0e00f 	mov	lr, pc
    9070:	e12fff13 	bx	r3
    9074:	eb000e14 	bl	c8cc <get_vfsd_pid>
    9078:	e1a02004 	mov	r2, r4
    907c:	e1a03005 	mov	r3, r5
    9080:	e3a01010 	mov	r1, #16
    9084:	eb00053f 	bl	a588 <ipc_call>
    9088:	e1a06000 	mov	r6, r0
    908c:	eb000aed 	bl	bc48 <get_proto_factor>
    9090:	e590300c 	ldr	r3, [r0, #12]
    9094:	e1a00004 	mov	r0, r4
    9098:	e1a0e00f 	mov	lr, pc
    909c:	e12fff13 	bx	r3
    90a0:	e3560000 	cmp	r6, #0
    90a4:	1a000002 	bne	90b4 <vfs_umount+0x9c>
    90a8:	e1a00005 	mov	r0, r5
    90ac:	eb000bae 	bl	bf6c <proto_read_int>
    90b0:	e1a06000 	mov	r6, r0
    90b4:	eb000ae3 	bl	bc48 <get_proto_factor>
    90b8:	e590300c 	ldr	r3, [r0, #12]
    90bc:	e1a00005 	mov	r0, r5
    90c0:	e1a0e00f 	mov	lr, pc
    90c4:	e12fff13 	bx	r3
    90c8:	e1a00006 	mov	r0, r6
    90cc:	e24bd018 	sub	sp, fp, #24
    90d0:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    90d4:	e12fff1e 	bx	lr

000090d8 <vfs_get_by_fd>:
    90d8:	e1a0c00d 	mov	ip, sp
    90dc:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    90e0:	e24cb004 	sub	fp, ip, #4
    90e4:	e24dd030 	sub	sp, sp, #48	; 0x30
    90e8:	e1a07001 	mov	r7, r1
    90ec:	e1a05000 	mov	r5, r0
    90f0:	eb000ad4 	bl	bc48 <get_proto_factor>
    90f4:	e24b404c 	sub	r4, fp, #76	; 0x4c
    90f8:	e5903004 	ldr	r3, [r0, #4]
    90fc:	e1a00004 	mov	r0, r4
    9100:	e1a0e00f 	mov	lr, pc
    9104:	e12fff13 	bx	r3
    9108:	e1a01005 	mov	r1, r5
    910c:	e5903014 	ldr	r3, [r0, #20]
    9110:	e1a00004 	mov	r0, r4
    9114:	e1a0e00f 	mov	lr, pc
    9118:	e12fff13 	bx	r3
    911c:	eb000ac9 	bl	bc48 <get_proto_factor>
    9120:	e24b5034 	sub	r5, fp, #52	; 0x34
    9124:	e5903004 	ldr	r3, [r0, #4]
    9128:	e1a00005 	mov	r0, r5
    912c:	e1a0e00f 	mov	lr, pc
    9130:	e12fff13 	bx	r3
    9134:	eb000de4 	bl	c8cc <get_vfsd_pid>
    9138:	e1a02004 	mov	r2, r4
    913c:	e1a03005 	mov	r3, r5
    9140:	e3a01001 	mov	r1, #1
    9144:	eb00050f 	bl	a588 <ipc_call>
    9148:	e1a06000 	mov	r6, r0
    914c:	eb000abd 	bl	bc48 <get_proto_factor>
    9150:	e590300c 	ldr	r3, [r0, #12]
    9154:	e1a00004 	mov	r0, r4
    9158:	e1a0e00f 	mov	lr, pc
    915c:	e12fff13 	bx	r3
    9160:	e2763001 	rsbs	r3, r6, #1
    9164:	33a03000 	movcc	r3, #0
    9168:	e3570000 	cmp	r7, #0
    916c:	03a03000 	moveq	r3, #0
    9170:	e3530000 	cmp	r3, #0
    9174:	11a01007 	movne	r1, r7
    9178:	11a00005 	movne	r0, r5
    917c:	13a0205c 	movne	r2, #92	; 0x5c
    9180:	1b000b0e 	blne	bdc0 <proto_read_to>
    9184:	eb000aaf 	bl	bc48 <get_proto_factor>
    9188:	e590300c 	ldr	r3, [r0, #12]
    918c:	e1a00005 	mov	r0, r5
    9190:	e1a0e00f 	mov	lr, pc
    9194:	e12fff13 	bx	r3
    9198:	e1a00006 	mov	r0, r6
    919c:	e24bd01c 	sub	sp, fp, #28
    91a0:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    91a4:	e12fff1e 	bx	lr

000091a8 <vfs_tell>:
    91a8:	e1a0c00d 	mov	ip, sp
    91ac:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    91b0:	e24cb004 	sub	fp, ip, #4
    91b4:	e24dd034 	sub	sp, sp, #52	; 0x34
    91b8:	e1a05000 	mov	r5, r0
    91bc:	eb000aa1 	bl	bc48 <get_proto_factor>
    91c0:	e24b404c 	sub	r4, fp, #76	; 0x4c
    91c4:	e5903004 	ldr	r3, [r0, #4]
    91c8:	e1a00004 	mov	r0, r4
    91cc:	e1a0e00f 	mov	lr, pc
    91d0:	e12fff13 	bx	r3
    91d4:	e1a01005 	mov	r1, r5
    91d8:	e5903014 	ldr	r3, [r0, #20]
    91dc:	e1a00004 	mov	r0, r4
    91e0:	e1a0e00f 	mov	lr, pc
    91e4:	e12fff13 	bx	r3
    91e8:	eb000a96 	bl	bc48 <get_proto_factor>
    91ec:	e24b5034 	sub	r5, fp, #52	; 0x34
    91f0:	e5903004 	ldr	r3, [r0, #4]
    91f4:	e1a00005 	mov	r0, r5
    91f8:	e1a0e00f 	mov	lr, pc
    91fc:	e12fff13 	bx	r3
    9200:	eb000db1 	bl	c8cc <get_vfsd_pid>
    9204:	e1a02004 	mov	r2, r4
    9208:	e1a03005 	mov	r3, r5
    920c:	e3a0100b 	mov	r1, #11
    9210:	eb0004dc 	bl	a588 <ipc_call>
    9214:	e1a06000 	mov	r6, r0
    9218:	eb000a8a 	bl	bc48 <get_proto_factor>
    921c:	e590300c 	ldr	r3, [r0, #12]
    9220:	e1a00004 	mov	r0, r4
    9224:	e1a0e00f 	mov	lr, pc
    9228:	e12fff13 	bx	r3
    922c:	e3560000 	cmp	r6, #0
    9230:	1a000002 	bne	9240 <vfs_tell+0x98>
    9234:	e1a00005 	mov	r0, r5
    9238:	eb000b4b 	bl	bf6c <proto_read_int>
    923c:	e1a06000 	mov	r6, r0
    9240:	eb000a80 	bl	bc48 <get_proto_factor>
    9244:	e590300c 	ldr	r3, [r0, #12]
    9248:	e1a00005 	mov	r0, r5
    924c:	e1a0e00f 	mov	lr, pc
    9250:	e12fff13 	bx	r3
    9254:	e1a00006 	mov	r0, r6
    9258:	e24bd018 	sub	sp, fp, #24
    925c:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    9260:	e12fff1e 	bx	lr

00009264 <vfs_seek>:
    9264:	e1a0c00d 	mov	ip, sp
    9268:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    926c:	e24cb004 	sub	fp, ip, #4
    9270:	e24dd034 	sub	sp, sp, #52	; 0x34
    9274:	e1a05001 	mov	r5, r1
    9278:	e1a06000 	mov	r6, r0
    927c:	eb000a71 	bl	bc48 <get_proto_factor>
    9280:	e24b404c 	sub	r4, fp, #76	; 0x4c
    9284:	e5903004 	ldr	r3, [r0, #4]
    9288:	e1a00004 	mov	r0, r4
    928c:	e1a0e00f 	mov	lr, pc
    9290:	e12fff13 	bx	r3
    9294:	e1a01006 	mov	r1, r6
    9298:	e5903014 	ldr	r3, [r0, #20]
    929c:	e1a00004 	mov	r0, r4
    92a0:	e1a0e00f 	mov	lr, pc
    92a4:	e12fff13 	bx	r3
    92a8:	e1a01005 	mov	r1, r5
    92ac:	e5903014 	ldr	r3, [r0, #20]
    92b0:	e1a00004 	mov	r0, r4
    92b4:	e1a0e00f 	mov	lr, pc
    92b8:	e12fff13 	bx	r3
    92bc:	eb000a61 	bl	bc48 <get_proto_factor>
    92c0:	e24b5034 	sub	r5, fp, #52	; 0x34
    92c4:	e5903004 	ldr	r3, [r0, #4]
    92c8:	e1a00005 	mov	r0, r5
    92cc:	e1a0e00f 	mov	lr, pc
    92d0:	e12fff13 	bx	r3
    92d4:	eb000d7c 	bl	c8cc <get_vfsd_pid>
    92d8:	e1a02004 	mov	r2, r4
    92dc:	e1a03005 	mov	r3, r5
    92e0:	e3a0100c 	mov	r1, #12
    92e4:	eb0004a7 	bl	a588 <ipc_call>
    92e8:	e1a06000 	mov	r6, r0
    92ec:	eb000a55 	bl	bc48 <get_proto_factor>
    92f0:	e590300c 	ldr	r3, [r0, #12]
    92f4:	e1a00004 	mov	r0, r4
    92f8:	e1a0e00f 	mov	lr, pc
    92fc:	e12fff13 	bx	r3
    9300:	e3560000 	cmp	r6, #0
    9304:	1a000002 	bne	9314 <vfs_seek+0xb0>
    9308:	e1a00005 	mov	r0, r5
    930c:	eb000b16 	bl	bf6c <proto_read_int>
    9310:	e1a06000 	mov	r6, r0
    9314:	eb000a4b 	bl	bc48 <get_proto_factor>
    9318:	e590300c 	ldr	r3, [r0, #12]
    931c:	e1a00005 	mov	r0, r5
    9320:	e1a0e00f 	mov	lr, pc
    9324:	e12fff13 	bx	r3
    9328:	e1a00006 	mov	r0, r6
    932c:	e24bd018 	sub	sp, fp, #24
    9330:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    9334:	e12fff1e 	bx	lr

00009338 <vfs_readfile>:
    9338:	e1a0c00d 	mov	ip, sp
    933c:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    9340:	e24cb004 	sub	fp, ip, #4
    9344:	e24dd060 	sub	sp, sp, #96	; 0x60
    9348:	e1a09001 	mov	r9, r1
    934c:	ebfffca1 	bl	85d8 <vfs_fullname>
    9350:	e24b1080 	sub	r1, fp, #128	; 0x80
    9354:	e1a05000 	mov	r5, r0
    9358:	ebfffdae 	bl	8a18 <vfs_get>
    935c:	e59f70d8 	ldr	r7, [pc, #216]	; 943c <vfs_readfile+0x104>
    9360:	e2504000 	subs	r4, r0, #0
    9364:	e08f7007 	add	r7, pc, r7
    9368:	1a00002f 	bne	942c <vfs_readfile+0xf4>
    936c:	e51b0038 	ldr	r0, [fp, #-56]	; 0x38
    9370:	e3500000 	cmp	r0, #0
    9374:	0a00002c 	beq	942c <vfs_readfile+0xf4>
    9378:	eb000fd3 	bl	d2cc <malloc>
    937c:	e2508000 	subs	r8, r0, #0
    9380:	0a000029 	beq	942c <vfs_readfile+0xf4>
    9384:	e1a01004 	mov	r1, r4
    9388:	e1a00005 	mov	r0, r5
    938c:	eb00118f 	bl	d9d0 <open>
    9390:	e2506000 	subs	r6, r0, #0
    9394:	e51b4038 	ldr	r4, [fp, #-56]	; 0x38
    9398:	ba00000f 	blt	93dc <vfs_readfile+0xa4>
    939c:	e3540000 	cmp	r4, #0
    93a0:	da00000b 	ble	93d4 <vfs_readfile+0x9c>
    93a4:	e1a05008 	mov	r5, r8
    93a8:	e1a00006 	mov	r0, r6
    93ac:	e1a01005 	mov	r1, r5
    93b0:	e1a02004 	mov	r2, r4
    93b4:	eb000e6c 	bl	cd6c <read>
    93b8:	e3500000 	cmp	r0, #0
    93bc:	ba000010 	blt	9404 <vfs_readfile+0xcc>
    93c0:	0afffff8 	beq	93a8 <vfs_readfile+0x70>
    93c4:	e0604004 	rsb	r4, r0, r4
    93c8:	e3540000 	cmp	r4, #0
    93cc:	e0855000 	add	r5, r5, r0
    93d0:	cafffff4 	bgt	93a8 <vfs_readfile+0x70>
    93d4:	e1a00006 	mov	r0, r6
    93d8:	eb0011e1 	bl	db64 <close>
    93dc:	e3540000 	cmp	r4, #0
    93e0:	1a00000f 	bne	9424 <vfs_readfile+0xec>
    93e4:	e3590000 	cmp	r9, #0
    93e8:	11a00008 	movne	r0, r8
    93ec:	01a00008 	moveq	r0, r8
    93f0:	151b3038 	ldrne	r3, [fp, #-56]	; 0x38
    93f4:	15893000 	strne	r3, [r9]
    93f8:	e24bd024 	sub	sp, fp, #36	; 0x24
    93fc:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    9400:	e12fff1e 	bx	lr
    9404:	e59f3034 	ldr	r3, [pc, #52]	; 9440 <vfs_readfile+0x108>
    9408:	e7973003 	ldr	r3, [r7, r3]
    940c:	e5933000 	ldr	r3, [r3]
    9410:	e3530001 	cmp	r3, #1
    9414:	0affffe3 	beq	93a8 <vfs_readfile+0x70>
    9418:	e1a00006 	mov	r0, r6
    941c:	eb0011d0 	bl	db64 <close>
    9420:	eaffffed 	b	93dc <vfs_readfile+0xa4>
    9424:	e1a00008 	mov	r0, r8
    9428:	eb000f9e 	bl	d2a8 <free>
    942c:	e3a00000 	mov	r0, #0
    9430:	e24bd024 	sub	sp, fp, #36	; 0x24
    9434:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    9438:	e12fff1e 	bx	lr
    943c:	0000e694 	muleq	r0, r4, r6
    9440:	00000018 	andeq	r0, r0, r8, lsl r0

00009444 <vfs_parse_name>:
    9444:	e1a0c00d 	mov	ip, sp
    9448:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    944c:	e24cb004 	sub	fp, ip, #4
    9450:	e1a07001 	mov	r7, r1
    9454:	e1a05002 	mov	r5, r2
    9458:	ebfffc5e 	bl	85d8 <vfs_fullname>
    945c:	ebfffa0a 	bl	7c8c <str_new>
    9460:	e5908000 	ldr	r8, [r0]
    9464:	e1a04000 	mov	r4, r0
    9468:	e1a00008 	mov	r0, r8
    946c:	eb001437 	bl	e550 <strlen>
    9470:	e3500000 	cmp	r0, #0
    9474:	ba00000f 	blt	94b8 <vfs_parse_name+0x74>
    9478:	e7d83000 	ldrb	r3, [r8, r0]
    947c:	e353002f 	cmp	r3, #47	; 0x2f
    9480:	e0882000 	add	r2, r8, r0
    9484:	01a06000 	moveq	r6, r0
    9488:	0a00001f 	beq	950c <vfs_parse_name+0xc8>
    948c:	e2403001 	sub	r3, r0, #1
    9490:	e0883003 	add	r3, r8, r3
    9494:	ea000005 	b	94b0 <vfs_parse_name+0x6c>
    9498:	e5d31000 	ldrb	r1, [r3]
    949c:	e351002f 	cmp	r1, #47	; 0x2f
    94a0:	e1a02003 	mov	r2, r3
    94a4:	e1a06000 	mov	r6, r0
    94a8:	e2433001 	sub	r3, r3, #1
    94ac:	0a000016 	beq	950c <vfs_parse_name+0xc8>
    94b0:	e2500001 	subs	r0, r0, #1
    94b4:	2afffff7 	bcs	9498 <vfs_parse_name+0x54>
    94b8:	e1a06000 	mov	r6, r0
    94bc:	e1a01008 	mov	r1, r8
    94c0:	e1a00007 	mov	r0, r7
    94c4:	ebfff9e5 	bl	7c60 <str_cpy>
    94c8:	e2861001 	add	r1, r6, #1
    94cc:	e0881001 	add	r1, r8, r1
    94d0:	e1a00005 	mov	r0, r5
    94d4:	ebfff9e1 	bl	7c60 <str_cpy>
    94d8:	e5973000 	ldr	r3, [r7]
    94dc:	e5d33000 	ldrb	r3, [r3]
    94e0:	e3530000 	cmp	r3, #0
    94e4:	059f102c 	ldreq	r1, [pc, #44]	; 9518 <vfs_parse_name+0xd4>
    94e8:	01a00007 	moveq	r0, r7
    94ec:	008f1001 	addeq	r1, pc, r1
    94f0:	0bfff9da 	bleq	7c60 <str_cpy>
    94f4:	e1a00004 	mov	r0, r4
    94f8:	ebfffa56 	bl	7e58 <str_free>
    94fc:	e3a00000 	mov	r0, #0
    9500:	e24bd024 	sub	sp, fp, #36	; 0x24
    9504:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    9508:	e12fff1e 	bx	lr
    950c:	e3a03000 	mov	r3, #0
    9510:	e5c23000 	strb	r3, [r2]
    9514:	eaffffe8 	b	94bc <vfs_parse_name+0x78>
    9518:	00005f74 	andeq	r5, r0, r4, ror pc

0000951c <vfs_create>:
    951c:	e1a0c00d 	mov	ip, sp
    9520:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    9524:	e59f51e0 	ldr	r5, [pc, #480]	; 970c <vfs_create+0x1f0>
    9528:	e24cb004 	sub	fp, ip, #4
    952c:	e24dd090 	sub	sp, sp, #144	; 0x90
    9530:	e08f5005 	add	r5, pc, r5
    9534:	e1a09000 	mov	r9, r0
    9538:	e1a00005 	mov	r0, r5
    953c:	e1a08003 	mov	r8, r3
    9540:	e1a04001 	mov	r4, r1
    9544:	e1a07002 	mov	r7, r2
    9548:	ebfff9cf 	bl	7c8c <str_new>
    954c:	e1a06000 	mov	r6, r0
    9550:	e1a00005 	mov	r0, r5
    9554:	ebfff9cc 	bl	7c8c <str_new>
    9558:	e1a05000 	mov	r5, r0
    955c:	e1a01006 	mov	r1, r6
    9560:	e1a00009 	mov	r0, r9
    9564:	e1a02005 	mov	r2, r5
    9568:	e24b9080 	sub	r9, fp, #128	; 0x80
    956c:	ebffffb4 	bl	9444 <vfs_parse_name>
    9570:	e1a01009 	mov	r1, r9
    9574:	e5960000 	ldr	r0, [r6]
    9578:	ebfffd26 	bl	8a18 <vfs_get>
    957c:	e2501000 	subs	r1, r0, #0
    9580:	1a000057 	bne	96e4 <vfs_create+0x1c8>
    9584:	e3a0205c 	mov	r2, #92	; 0x5c
    9588:	e1a00004 	mov	r0, r4
    958c:	eb001295 	bl	dfe8 <memset>
    9590:	e5951000 	ldr	r1, [r5]
    9594:	e2840004 	add	r0, r4, #4
    9598:	eb0013b0 	bl	e460 <strcpy>
    959c:	e1a00005 	mov	r0, r5
    95a0:	e5847044 	str	r7, [r4, #68]	; 0x44
    95a4:	ebfffa2b 	bl	7e58 <str_free>
    95a8:	e1a00006 	mov	r0, r6
    95ac:	ebfffa29 	bl	7e58 <str_free>
    95b0:	e1a00004 	mov	r0, r4
    95b4:	ebfffbd7 	bl	8518 <vfs_new_node>
    95b8:	e1a00009 	mov	r0, r9
    95bc:	e1a01004 	mov	r1, r4
    95c0:	ebfffdc0 	bl	8cc8 <vfs_add>
    95c4:	e3500000 	cmp	r0, #0
    95c8:	1a00004b 	bne	96fc <vfs_create+0x1e0>
    95cc:	e3580000 	cmp	r8, #0
    95d0:	1a000031 	bne	969c <vfs_create+0x180>
    95d4:	e3570000 	cmp	r7, #0
    95d8:	03a03b01 	moveq	r3, #1024	; 0x400
    95dc:	05843048 	streq	r3, [r4, #72]	; 0x48
    95e0:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
    95e4:	e5843054 	str	r3, [r4, #84]	; 0x54
    95e8:	eb000996 	bl	bc48 <get_proto_factor>
    95ec:	e24b7098 	sub	r7, fp, #152	; 0x98
    95f0:	e5903004 	ldr	r3, [r0, #4]
    95f4:	e1a00007 	mov	r0, r7
    95f8:	e1a0e00f 	mov	lr, pc
    95fc:	e12fff13 	bx	r3
    9600:	eb000990 	bl	bc48 <get_proto_factor>
    9604:	e24b60b0 	sub	r6, fp, #176	; 0xb0
    9608:	e5903004 	ldr	r3, [r0, #4]
    960c:	e1a00006 	mov	r0, r6
    9610:	e1a0e00f 	mov	lr, pc
    9614:	e12fff13 	bx	r3
    9618:	e1a01009 	mov	r1, r9
    961c:	e5903010 	ldr	r3, [r0, #16]
    9620:	e3a0205c 	mov	r2, #92	; 0x5c
    9624:	e1a00006 	mov	r0, r6
    9628:	e1a0e00f 	mov	lr, pc
    962c:	e12fff13 	bx	r3
    9630:	e1a01004 	mov	r1, r4
    9634:	e5903010 	ldr	r3, [r0, #16]
    9638:	e3a0205c 	mov	r2, #92	; 0x5c
    963c:	e1a00006 	mov	r0, r6
    9640:	e1a0e00f 	mov	lr, pc
    9644:	e12fff13 	bx	r3
    9648:	e1a02006 	mov	r2, r6
    964c:	e1a03007 	mov	r3, r7
    9650:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    9654:	e3a01002 	mov	r1, #2
    9658:	eb0003ca 	bl	a588 <ipc_call>
    965c:	e3500000 	cmp	r0, #0
    9660:	0a000010 	beq	96a8 <vfs_create+0x18c>
    9664:	e1a00004 	mov	r0, r4
    9668:	ebfffdcd 	bl	8da4 <vfs_del>
    966c:	e3e05000 	mvn	r5, #0
    9670:	eb000974 	bl	bc48 <get_proto_factor>
    9674:	e590300c 	ldr	r3, [r0, #12]
    9678:	e1a00006 	mov	r0, r6
    967c:	e1a0e00f 	mov	lr, pc
    9680:	e12fff13 	bx	r3
    9684:	eb00096f 	bl	bc48 <get_proto_factor>
    9688:	e590300c 	ldr	r3, [r0, #12]
    968c:	e1a00007 	mov	r0, r7
    9690:	e1a0e00f 	mov	lr, pc
    9694:	e12fff13 	bx	r3
    9698:	e1a00005 	mov	r0, r5
    969c:	e24bd024 	sub	sp, fp, #36	; 0x24
    96a0:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    96a4:	e12fff1e 	bx	lr
    96a8:	e1a00007 	mov	r0, r7
    96ac:	eb000a2e 	bl	bf6c <proto_read_int>
    96b0:	e2505000 	subs	r5, r0, #0
    96b4:	1a000007 	bne	96d8 <vfs_create+0x1bc>
    96b8:	e1a01004 	mov	r1, r4
    96bc:	e3a0205c 	mov	r2, #92	; 0x5c
    96c0:	e1a00007 	mov	r0, r7
    96c4:	eb0009bd 	bl	bdc0 <proto_read_to>
    96c8:	e1a00004 	mov	r0, r4
    96cc:	ebfffd4d 	bl	8c08 <vfs_set>
    96d0:	e1a05000 	mov	r5, r0
    96d4:	eaffffe5 	b	9670 <vfs_create+0x154>
    96d8:	e1a00004 	mov	r0, r4
    96dc:	ebfffdb0 	bl	8da4 <vfs_del>
    96e0:	eaffffe2 	b	9670 <vfs_create+0x154>
    96e4:	e1a00006 	mov	r0, r6
    96e8:	ebfff9da 	bl	7e58 <str_free>
    96ec:	e1a00005 	mov	r0, r5
    96f0:	ebfff9d8 	bl	7e58 <str_free>
    96f4:	e3e00000 	mvn	r0, #0
    96f8:	eaffffe7 	b	969c <vfs_create+0x180>
    96fc:	e1a00004 	mov	r0, r4
    9700:	ebfffda7 	bl	8da4 <vfs_del>
    9704:	e3e00000 	mvn	r0, #0
    9708:	eaffffe3 	b	969c <vfs_create+0x180>
    970c:	00005fbc 			; <UNDEFINED> instruction: 0x00005fbc

00009710 <vfs_fcntl>:
    9710:	e1a0c00d 	mov	ip, sp
    9714:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    9718:	e24cb004 	sub	fp, ip, #4
    971c:	e24dd090 	sub	sp, sp, #144	; 0x90
    9720:	e24b6080 	sub	r6, fp, #128	; 0x80
    9724:	e1a08001 	mov	r8, r1
    9728:	e1a01006 	mov	r1, r6
    972c:	e1a05002 	mov	r5, r2
    9730:	e1a07003 	mov	r7, r3
    9734:	e1a09000 	mov	r9, r0
    9738:	ebfffe66 	bl	90d8 <vfs_get_by_fd>
    973c:	e3500000 	cmp	r0, #0
    9740:	1a000063 	bne	98d4 <vfs_fcntl+0x1c4>
    9744:	eb00093f 	bl	bc48 <get_proto_factor>
    9748:	e24b40b0 	sub	r4, fp, #176	; 0xb0
    974c:	e5903004 	ldr	r3, [r0, #4]
    9750:	e1a00004 	mov	r0, r4
    9754:	e1a0e00f 	mov	lr, pc
    9758:	e12fff13 	bx	r3
    975c:	e1a01009 	mov	r1, r9
    9760:	e5903014 	ldr	r3, [r0, #20]
    9764:	e1a00004 	mov	r0, r4
    9768:	e1a0e00f 	mov	lr, pc
    976c:	e12fff13 	bx	r3
    9770:	e1a01006 	mov	r1, r6
    9774:	e5903010 	ldr	r3, [r0, #16]
    9778:	e3a0205c 	mov	r2, #92	; 0x5c
    977c:	e1a00004 	mov	r0, r4
    9780:	e1a0e00f 	mov	lr, pc
    9784:	e12fff13 	bx	r3
    9788:	e1a01008 	mov	r1, r8
    978c:	e5903014 	ldr	r3, [r0, #20]
    9790:	e1a00004 	mov	r0, r4
    9794:	e1a0e00f 	mov	lr, pc
    9798:	e12fff13 	bx	r3
    979c:	e3550000 	cmp	r5, #0
    97a0:	0a000033 	beq	9874 <vfs_fcntl+0x164>
    97a4:	eb000927 	bl	bc48 <get_proto_factor>
    97a8:	e5951004 	ldr	r1, [r5, #4]
    97ac:	e5903010 	ldr	r3, [r0, #16]
    97b0:	e5952008 	ldr	r2, [r5, #8]
    97b4:	e1a00004 	mov	r0, r4
    97b8:	e1a0e00f 	mov	lr, pc
    97bc:	e12fff13 	bx	r3
    97c0:	e3570000 	cmp	r7, #0
    97c4:	0a000033 	beq	9898 <vfs_fcntl+0x188>
    97c8:	eb00091e 	bl	bc48 <get_proto_factor>
    97cc:	e24b6098 	sub	r6, fp, #152	; 0x98
    97d0:	e5903004 	ldr	r3, [r0, #4]
    97d4:	e1a00006 	mov	r0, r6
    97d8:	e1a0e00f 	mov	lr, pc
    97dc:	e12fff13 	bx	r3
    97e0:	e1a03006 	mov	r3, r6
    97e4:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    97e8:	e3a0100a 	mov	r1, #10
    97ec:	e1a02004 	mov	r2, r4
    97f0:	eb000364 	bl	a588 <ipc_call>
    97f4:	e3500000 	cmp	r0, #0
    97f8:	13e05000 	mvnne	r5, #0
    97fc:	0a00000d 	beq	9838 <vfs_fcntl+0x128>
    9800:	eb000910 	bl	bc48 <get_proto_factor>
    9804:	e590300c 	ldr	r3, [r0, #12]
    9808:	e1a00004 	mov	r0, r4
    980c:	e1a0e00f 	mov	lr, pc
    9810:	e12fff13 	bx	r3
    9814:	eb00090b 	bl	bc48 <get_proto_factor>
    9818:	e590300c 	ldr	r3, [r0, #12]
    981c:	e1a00006 	mov	r0, r6
    9820:	e1a0e00f 	mov	lr, pc
    9824:	e12fff13 	bx	r3
    9828:	e1a00005 	mov	r0, r5
    982c:	e24bd024 	sub	sp, fp, #36	; 0x24
    9830:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    9834:	e12fff1e 	bx	lr
    9838:	e1a00006 	mov	r0, r6
    983c:	eb0009ca 	bl	bf6c <proto_read_int>
    9840:	e24b10b4 	sub	r1, fp, #180	; 0xb4
    9844:	e1a05000 	mov	r5, r0
    9848:	e1a00006 	mov	r0, r6
    984c:	eb000935 	bl	bd28 <proto_read>
    9850:	e1a08000 	mov	r8, r0
    9854:	eb0008fb 	bl	bc48 <get_proto_factor>
    9858:	e1a01008 	mov	r1, r8
    985c:	e5903008 	ldr	r3, [r0, #8]
    9860:	e51b20b4 	ldr	r2, [fp, #-180]	; 0xb4
    9864:	e1a00007 	mov	r0, r7
    9868:	e1a0e00f 	mov	lr, pc
    986c:	e12fff13 	bx	r3
    9870:	eaffffe2 	b	9800 <vfs_fcntl+0xf0>
    9874:	eb0008f3 	bl	bc48 <get_proto_factor>
    9878:	e1a01005 	mov	r1, r5
    987c:	e5903010 	ldr	r3, [r0, #16]
    9880:	e1a02005 	mov	r2, r5
    9884:	e1a00004 	mov	r0, r4
    9888:	e1a0e00f 	mov	lr, pc
    988c:	e12fff13 	bx	r3
    9890:	e3570000 	cmp	r7, #0
    9894:	1affffcb 	bne	97c8 <vfs_fcntl+0xb8>
    9898:	e1a03007 	mov	r3, r7
    989c:	e3a0100a 	mov	r1, #10
    98a0:	e1a02004 	mov	r2, r4
    98a4:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    98a8:	eb000336 	bl	a588 <ipc_call>
    98ac:	e1a05000 	mov	r5, r0
    98b0:	eb0008e4 	bl	bc48 <get_proto_factor>
    98b4:	e590300c 	ldr	r3, [r0, #12]
    98b8:	e1a00004 	mov	r0, r4
    98bc:	e1a0e00f 	mov	lr, pc
    98c0:	e12fff13 	bx	r3
    98c4:	e1a00005 	mov	r0, r5
    98c8:	e24bd024 	sub	sp, fp, #36	; 0x24
    98cc:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    98d0:	e12fff1e 	bx	lr
    98d4:	e3e00000 	mvn	r0, #0
    98d8:	eaffffd3 	b	982c <vfs_fcntl+0x11c>

000098dc <vfs_dma>:
    98dc:	e1a0c00d 	mov	ip, sp
    98e0:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    98e4:	e24cb004 	sub	fp, ip, #4
    98e8:	e24dd094 	sub	sp, sp, #148	; 0x94
    98ec:	e24b6080 	sub	r6, fp, #128	; 0x80
    98f0:	e1a08001 	mov	r8, r1
    98f4:	e1a01006 	mov	r1, r6
    98f8:	e1a07000 	mov	r7, r0
    98fc:	ebfffdf5 	bl	90d8 <vfs_get_by_fd>
    9900:	e3500000 	cmp	r0, #0
    9904:	1a000035 	bne	99e0 <vfs_dma+0x104>
    9908:	eb0008ce 	bl	bc48 <get_proto_factor>
    990c:	e24b5098 	sub	r5, fp, #152	; 0x98
    9910:	e5903004 	ldr	r3, [r0, #4]
    9914:	e1a00005 	mov	r0, r5
    9918:	e1a0e00f 	mov	lr, pc
    991c:	e12fff13 	bx	r3
    9920:	eb0008c8 	bl	bc48 <get_proto_factor>
    9924:	e24b40b0 	sub	r4, fp, #176	; 0xb0
    9928:	e5903004 	ldr	r3, [r0, #4]
    992c:	e1a00004 	mov	r0, r4
    9930:	e1a0e00f 	mov	lr, pc
    9934:	e12fff13 	bx	r3
    9938:	e1a01007 	mov	r1, r7
    993c:	e5903014 	ldr	r3, [r0, #20]
    9940:	e1a00004 	mov	r0, r4
    9944:	e1a0e00f 	mov	lr, pc
    9948:	e12fff13 	bx	r3
    994c:	e1a01006 	mov	r1, r6
    9950:	e5903010 	ldr	r3, [r0, #16]
    9954:	e3a0205c 	mov	r2, #92	; 0x5c
    9958:	e1a00004 	mov	r0, r4
    995c:	e1a0e00f 	mov	lr, pc
    9960:	e12fff13 	bx	r3
    9964:	e1a02004 	mov	r2, r4
    9968:	e1a03005 	mov	r3, r5
    996c:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    9970:	e3a01009 	mov	r1, #9
    9974:	eb000303 	bl	a588 <ipc_call>
    9978:	e3500000 	cmp	r0, #0
    997c:	13e06000 	mvnne	r6, #0
    9980:	0a00000d 	beq	99bc <vfs_dma+0xe0>
    9984:	eb0008af 	bl	bc48 <get_proto_factor>
    9988:	e590300c 	ldr	r3, [r0, #12]
    998c:	e1a00004 	mov	r0, r4
    9990:	e1a0e00f 	mov	lr, pc
    9994:	e12fff13 	bx	r3
    9998:	eb0008aa 	bl	bc48 <get_proto_factor>
    999c:	e590300c 	ldr	r3, [r0, #12]
    99a0:	e1a00005 	mov	r0, r5
    99a4:	e1a0e00f 	mov	lr, pc
    99a8:	e12fff13 	bx	r3
    99ac:	e1a00006 	mov	r0, r6
    99b0:	e24bd020 	sub	sp, fp, #32
    99b4:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    99b8:	e12fff1e 	bx	lr
    99bc:	e1a00005 	mov	r0, r5
    99c0:	eb000969 	bl	bf6c <proto_read_int>
    99c4:	e3580000 	cmp	r8, #0
    99c8:	e1a06000 	mov	r6, r0
    99cc:	0affffec 	beq	9984 <vfs_dma+0xa8>
    99d0:	e1a00005 	mov	r0, r5
    99d4:	eb000964 	bl	bf6c <proto_read_int>
    99d8:	e5880000 	str	r0, [r8]
    99dc:	eaffffe8 	b	9984 <vfs_dma+0xa8>
    99e0:	e3e00000 	mvn	r0, #0
    99e4:	eafffff1 	b	99b0 <vfs_dma+0xd4>

000099e8 <vfs_flush>:
    99e8:	e1a0c00d 	mov	ip, sp
    99ec:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    99f0:	e24cb004 	sub	fp, ip, #4
    99f4:	e24dd07c 	sub	sp, sp, #124	; 0x7c
    99f8:	e24b4080 	sub	r4, fp, #128	; 0x80
    99fc:	e1a05001 	mov	r5, r1
    9a00:	e1a01004 	mov	r1, r4
    9a04:	e1a06000 	mov	r6, r0
    9a08:	ebfffdb2 	bl	90d8 <vfs_get_by_fd>
    9a0c:	e2507000 	subs	r7, r0, #0
    9a10:	13a00000 	movne	r0, #0
    9a14:	0a000002 	beq	9a24 <vfs_flush+0x3c>
    9a18:	e24bd020 	sub	sp, fp, #32
    9a1c:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    9a20:	e12fff1e 	bx	lr
    9a24:	eb000887 	bl	bc48 <get_proto_factor>
    9a28:	e24b8098 	sub	r8, fp, #152	; 0x98
    9a2c:	e5903004 	ldr	r3, [r0, #4]
    9a30:	e1a00008 	mov	r0, r8
    9a34:	e1a0e00f 	mov	lr, pc
    9a38:	e12fff13 	bx	r3
    9a3c:	e1a01006 	mov	r1, r6
    9a40:	e5903014 	ldr	r3, [r0, #20]
    9a44:	e1a00008 	mov	r0, r8
    9a48:	e1a0e00f 	mov	lr, pc
    9a4c:	e12fff13 	bx	r3
    9a50:	e1a01004 	mov	r1, r4
    9a54:	e5903010 	ldr	r3, [r0, #16]
    9a58:	e3a0205c 	mov	r2, #92	; 0x5c
    9a5c:	e1a00008 	mov	r0, r8
    9a60:	e1a0e00f 	mov	lr, pc
    9a64:	e12fff13 	bx	r3
    9a68:	e3550000 	cmp	r5, #0
    9a6c:	0a00000b 	beq	9aa0 <vfs_flush+0xb8>
    9a70:	e1a03007 	mov	r3, r7
    9a74:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    9a78:	e3a0100e 	mov	r1, #14
    9a7c:	e1a02008 	mov	r2, r8
    9a80:	eb0002ea 	bl	a630 <ipc_call_wait>
    9a84:	eb00086f 	bl	bc48 <get_proto_factor>
    9a88:	e590300c 	ldr	r3, [r0, #12]
    9a8c:	e1a00008 	mov	r0, r8
    9a90:	e1a0e00f 	mov	lr, pc
    9a94:	e12fff13 	bx	r3
    9a98:	e3e00000 	mvn	r0, #0
    9a9c:	eaffffdd 	b	9a18 <vfs_flush+0x30>
    9aa0:	e1a03005 	mov	r3, r5
    9aa4:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    9aa8:	e3a0100e 	mov	r1, #14
    9aac:	e1a02008 	mov	r2, r8
    9ab0:	eb0002b4 	bl	a588 <ipc_call>
    9ab4:	eafffff2 	b	9a84 <vfs_flush+0x9c>

00009ab8 <vfs_write_block>:
    9ab8:	e1a0c00d 	mov	ip, sp
    9abc:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    9ac0:	e24cb004 	sub	fp, ip, #4
    9ac4:	e24dd030 	sub	sp, sp, #48	; 0x30
    9ac8:	e1a09001 	mov	r9, r1
    9acc:	e1a08002 	mov	r8, r2
    9ad0:	e1a07003 	mov	r7, r3
    9ad4:	e1a06000 	mov	r6, r0
    9ad8:	eb00085a 	bl	bc48 <get_proto_factor>
    9adc:	e24b503c 	sub	r5, fp, #60	; 0x3c
    9ae0:	e5903004 	ldr	r3, [r0, #4]
    9ae4:	e1a00005 	mov	r0, r5
    9ae8:	e1a0e00f 	mov	lr, pc
    9aec:	e12fff13 	bx	r3
    9af0:	eb000854 	bl	bc48 <get_proto_factor>
    9af4:	e24b4054 	sub	r4, fp, #84	; 0x54
    9af8:	e5903004 	ldr	r3, [r0, #4]
    9afc:	e1a00004 	mov	r0, r4
    9b00:	e1a0e00f 	mov	lr, pc
    9b04:	e12fff13 	bx	r3
    9b08:	e1a02008 	mov	r2, r8
    9b0c:	e5903010 	ldr	r3, [r0, #16]
    9b10:	e1a01009 	mov	r1, r9
    9b14:	e1a00004 	mov	r0, r4
    9b18:	e1a0e00f 	mov	lr, pc
    9b1c:	e12fff13 	bx	r3
    9b20:	e1a01007 	mov	r1, r7
    9b24:	e5903014 	ldr	r3, [r0, #20]
    9b28:	e1a00004 	mov	r0, r4
    9b2c:	e1a0e00f 	mov	lr, pc
    9b30:	e12fff13 	bx	r3
    9b34:	e1a00006 	mov	r0, r6
    9b38:	e1a02004 	mov	r2, r4
    9b3c:	e1a03005 	mov	r3, r5
    9b40:	e3a01007 	mov	r1, #7
    9b44:	eb00028f 	bl	a588 <ipc_call>
    9b48:	e59f7074 	ldr	r7, [pc, #116]	; 9bc4 <vfs_write_block+0x10c>
    9b4c:	e3500000 	cmp	r0, #0
    9b50:	e08f7007 	add	r7, pc, r7
    9b54:	1a000012 	bne	9ba4 <vfs_write_block+0xec>
    9b58:	e1a00005 	mov	r0, r5
    9b5c:	eb000902 	bl	bf6c <proto_read_int>
    9b60:	e3700002 	cmn	r0, #2
    9b64:	e1a06000 	mov	r6, r0
    9b68:	0a00000f 	beq	9bac <vfs_write_block+0xf4>
    9b6c:	eb000835 	bl	bc48 <get_proto_factor>
    9b70:	e590300c 	ldr	r3, [r0, #12]
    9b74:	e1a00004 	mov	r0, r4
    9b78:	e1a0e00f 	mov	lr, pc
    9b7c:	e12fff13 	bx	r3
    9b80:	eb000830 	bl	bc48 <get_proto_factor>
    9b84:	e590300c 	ldr	r3, [r0, #12]
    9b88:	e1a00005 	mov	r0, r5
    9b8c:	e1a0e00f 	mov	lr, pc
    9b90:	e12fff13 	bx	r3
    9b94:	e1a00006 	mov	r0, r6
    9b98:	e24bd024 	sub	sp, fp, #36	; 0x24
    9b9c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    9ba0:	e12fff1e 	bx	lr
    9ba4:	e3e06000 	mvn	r6, #0
    9ba8:	eaffffef 	b	9b6c <vfs_write_block+0xb4>
    9bac:	e3a02001 	mov	r2, #1
    9bb0:	e59f3010 	ldr	r3, [pc, #16]	; 9bc8 <vfs_write_block+0x110>
    9bb4:	e7973003 	ldr	r3, [r7, r3]
    9bb8:	e3e06000 	mvn	r6, #0
    9bbc:	e5832000 	str	r2, [r3]
    9bc0:	eaffffe9 	b	9b6c <vfs_write_block+0xb4>
    9bc4:	0000dea8 	andeq	sp, r0, r8, lsr #29
    9bc8:	00000018 	andeq	r0, r0, r8, lsl r0

00009bcc <vfs_read_block>:
    9bcc:	e1a0c00d 	mov	ip, sp
    9bd0:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    9bd4:	e24cb004 	sub	fp, ip, #4
    9bd8:	e24dd03c 	sub	sp, sp, #60	; 0x3c
    9bdc:	e1a08000 	mov	r8, r0
    9be0:	e50b1060 	str	r1, [fp, #-96]	; 0x60
    9be4:	e1a00002 	mov	r0, r2
    9be8:	e3a01001 	mov	r1, #1
    9bec:	e1a06002 	mov	r6, r2
    9bf0:	e1a09003 	mov	r9, r3
    9bf4:	eb000b11 	bl	c840 <shm_alloc>
    9bf8:	e59fa130 	ldr	sl, [pc, #304]	; 9d30 <vfs_read_block+0x164>
    9bfc:	e2507000 	subs	r7, r0, #0
    9c00:	e08fa00a 	add	sl, pc, sl
    9c04:	ba000047 	blt	9d28 <vfs_read_block+0x15c>
    9c08:	eb000b17 	bl	c86c <shm_map>
    9c0c:	e2503000 	subs	r3, r0, #0
    9c10:	e50b3064 	str	r3, [fp, #-100]	; 0x64
    9c14:	0a000043 	beq	9d28 <vfs_read_block+0x15c>
    9c18:	eb00080a 	bl	bc48 <get_proto_factor>
    9c1c:	e24b5044 	sub	r5, fp, #68	; 0x44
    9c20:	e5903004 	ldr	r3, [r0, #4]
    9c24:	e1a00005 	mov	r0, r5
    9c28:	e1a0e00f 	mov	lr, pc
    9c2c:	e12fff13 	bx	r3
    9c30:	eb000804 	bl	bc48 <get_proto_factor>
    9c34:	e24b405c 	sub	r4, fp, #92	; 0x5c
    9c38:	e5903004 	ldr	r3, [r0, #4]
    9c3c:	e1a00004 	mov	r0, r4
    9c40:	e1a0e00f 	mov	lr, pc
    9c44:	e12fff13 	bx	r3
    9c48:	e1a01006 	mov	r1, r6
    9c4c:	e5903014 	ldr	r3, [r0, #20]
    9c50:	e1a00004 	mov	r0, r4
    9c54:	e1a0e00f 	mov	lr, pc
    9c58:	e12fff13 	bx	r3
    9c5c:	e1a01009 	mov	r1, r9
    9c60:	e5903014 	ldr	r3, [r0, #20]
    9c64:	e1a00004 	mov	r0, r4
    9c68:	e1a0e00f 	mov	lr, pc
    9c6c:	e12fff13 	bx	r3
    9c70:	e1a01007 	mov	r1, r7
    9c74:	e5903014 	ldr	r3, [r0, #20]
    9c78:	e1a00004 	mov	r0, r4
    9c7c:	e1a0e00f 	mov	lr, pc
    9c80:	e12fff13 	bx	r3
    9c84:	e1a00008 	mov	r0, r8
    9c88:	e1a02004 	mov	r2, r4
    9c8c:	e1a03005 	mov	r3, r5
    9c90:	e3a01006 	mov	r1, #6
    9c94:	eb00023b 	bl	a588 <ipc_call>
    9c98:	e3500000 	cmp	r0, #0
    9c9c:	13e06000 	mvnne	r6, #0
    9ca0:	0a00000f 	beq	9ce4 <vfs_read_block+0x118>
    9ca4:	eb0007e7 	bl	bc48 <get_proto_factor>
    9ca8:	e590300c 	ldr	r3, [r0, #12]
    9cac:	e1a00004 	mov	r0, r4
    9cb0:	e1a0e00f 	mov	lr, pc
    9cb4:	e12fff13 	bx	r3
    9cb8:	eb0007e2 	bl	bc48 <get_proto_factor>
    9cbc:	e590300c 	ldr	r3, [r0, #12]
    9cc0:	e1a00005 	mov	r0, r5
    9cc4:	e1a0e00f 	mov	lr, pc
    9cc8:	e12fff13 	bx	r3
    9ccc:	e1a00007 	mov	r0, r7
    9cd0:	eb000aee 	bl	c890 <shm_unmap>
    9cd4:	e1a00006 	mov	r0, r6
    9cd8:	e24bd028 	sub	sp, fp, #40	; 0x28
    9cdc:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    9ce0:	e12fff1e 	bx	lr
    9ce4:	e1a00005 	mov	r0, r5
    9ce8:	eb00089f 	bl	bf6c <proto_read_int>
    9cec:	e2506000 	subs	r6, r0, #0
    9cf0:	da000004 	ble	9d08 <vfs_read_block+0x13c>
    9cf4:	e51b0060 	ldr	r0, [fp, #-96]	; 0x60
    9cf8:	e51b1064 	ldr	r1, [fp, #-100]	; 0x64
    9cfc:	e1a02006 	mov	r2, r6
    9d00:	eb00107c 	bl	def8 <memcpy>
    9d04:	eaffffe6 	b	9ca4 <vfs_read_block+0xd8>
    9d08:	e3760002 	cmn	r6, #2
    9d0c:	1affffe4 	bne	9ca4 <vfs_read_block+0xd8>
    9d10:	e3a02001 	mov	r2, #1
    9d14:	e59f3018 	ldr	r3, [pc, #24]	; 9d34 <vfs_read_block+0x168>
    9d18:	e79a3003 	ldr	r3, [sl, r3]
    9d1c:	e3e06000 	mvn	r6, #0
    9d20:	e5832000 	str	r2, [r3]
    9d24:	eaffffde 	b	9ca4 <vfs_read_block+0xd8>
    9d28:	e3e00000 	mvn	r0, #0
    9d2c:	eaffffe9 	b	9cd8 <vfs_read_block+0x10c>
    9d30:	0000ddf8 	strdeq	sp, [r0], -r8
    9d34:	00000018 	andeq	r0, r0, r8, lsl r0

00009d38 <vfs_read_pipe>:
    9d38:	e1a0c00d 	mov	ip, sp
    9d3c:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    9d40:	e24cb004 	sub	fp, ip, #4
    9d44:	e24dd03c 	sub	sp, sp, #60	; 0x3c
    9d48:	e1a09002 	mov	r9, r2
    9d4c:	e50b1060 	str	r1, [fp, #-96]	; 0x60
    9d50:	e1a08003 	mov	r8, r3
    9d54:	e1a07000 	mov	r7, r0
    9d58:	eb0007ba 	bl	bc48 <get_proto_factor>
    9d5c:	e24b405c 	sub	r4, fp, #92	; 0x5c
    9d60:	e5903004 	ldr	r3, [r0, #4]
    9d64:	e1a00004 	mov	r0, r4
    9d68:	e1a0e00f 	mov	lr, pc
    9d6c:	e12fff13 	bx	r3
    9d70:	e3a0205c 	mov	r2, #92	; 0x5c
    9d74:	e5903010 	ldr	r3, [r0, #16]
    9d78:	e1a01007 	mov	r1, r7
    9d7c:	e1a00004 	mov	r0, r4
    9d80:	e1a0e00f 	mov	lr, pc
    9d84:	e12fff13 	bx	r3
    9d88:	e1a01009 	mov	r1, r9
    9d8c:	e5903014 	ldr	r3, [r0, #20]
    9d90:	e1a00004 	mov	r0, r4
    9d94:	e1a0e00f 	mov	lr, pc
    9d98:	e12fff13 	bx	r3
    9d9c:	e1a01008 	mov	r1, r8
    9da0:	e5903014 	ldr	r3, [r0, #20]
    9da4:	e1a00004 	mov	r0, r4
    9da8:	e1a0e00f 	mov	lr, pc
    9dac:	e12fff13 	bx	r3
    9db0:	eb0007a4 	bl	bc48 <get_proto_factor>
    9db4:	e24b6044 	sub	r6, fp, #68	; 0x44
    9db8:	e5903004 	ldr	r3, [r0, #4]
    9dbc:	e1a00006 	mov	r0, r6
    9dc0:	e1a0e00f 	mov	lr, pc
    9dc4:	e12fff13 	bx	r3
    9dc8:	eb000abf 	bl	c8cc <get_vfsd_pid>
    9dcc:	e1a02004 	mov	r2, r4
    9dd0:	e1a03006 	mov	r3, r6
    9dd4:	e3a01007 	mov	r1, #7
    9dd8:	e1a0a000 	mov	sl, r0
    9ddc:	eb0001e9 	bl	a588 <ipc_call>
    9de0:	e1a05000 	mov	r5, r0
    9de4:	eb000797 	bl	bc48 <get_proto_factor>
    9de8:	e590300c 	ldr	r3, [r0, #12]
    9dec:	e1a00004 	mov	r0, r4
    9df0:	e59f40ac 	ldr	r4, [pc, #172]	; 9ea4 <vfs_read_pipe+0x16c>
    9df4:	e1a0e00f 	mov	lr, pc
    9df8:	e12fff13 	bx	r3
    9dfc:	e3550000 	cmp	r5, #0
    9e00:	e08f4004 	add	r4, pc, r4
    9e04:	0a000008 	beq	9e2c <vfs_read_pipe+0xf4>
    9e08:	eb00078e 	bl	bc48 <get_proto_factor>
    9e0c:	e590300c 	ldr	r3, [r0, #12]
    9e10:	e1a00006 	mov	r0, r6
    9e14:	e1a0e00f 	mov	lr, pc
    9e18:	e12fff13 	bx	r3
    9e1c:	e1c50fc5 	bic	r0, r5, r5, asr #31
    9e20:	e24bd028 	sub	sp, fp, #40	; 0x28
    9e24:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    9e28:	e12fff1e 	bx	lr
    9e2c:	e1a00006 	mov	r0, r6
    9e30:	eb00084d 	bl	bf6c <proto_read_int>
    9e34:	e2505000 	subs	r5, r0, #0
    9e38:	da000004 	ble	9e50 <vfs_read_pipe+0x118>
    9e3c:	e51b1060 	ldr	r1, [fp, #-96]	; 0x60
    9e40:	e1a02009 	mov	r2, r9
    9e44:	e1a00006 	mov	r0, r6
    9e48:	eb0007dc 	bl	bdc0 <proto_read_to>
    9e4c:	e1a05000 	mov	r5, r0
    9e50:	eb00077c 	bl	bc48 <get_proto_factor>
    9e54:	e590300c 	ldr	r3, [r0, #12]
    9e58:	e1a00006 	mov	r0, r6
    9e5c:	e1a0e00f 	mov	lr, pc
    9e60:	e12fff13 	bx	r3
    9e64:	e2753001 	rsbs	r3, r5, #1
    9e68:	33a03000 	movcc	r3, #0
    9e6c:	e1130008 	tst	r3, r8
    9e70:	0a000008 	beq	9e98 <vfs_read_pipe+0x160>
    9e74:	e1a0000a 	mov	r0, sl
    9e78:	e5971000 	ldr	r1, [r7]
    9e7c:	eb000ad8 	bl	c9e4 <proc_block>
    9e80:	e3a02001 	mov	r2, #1
    9e84:	e59f301c 	ldr	r3, [pc, #28]	; 9ea8 <vfs_read_pipe+0x170>
    9e88:	e7943003 	ldr	r3, [r4, r3]
    9e8c:	e3e00000 	mvn	r0, #0
    9e90:	e5832000 	str	r2, [r3]
    9e94:	eaffffe1 	b	9e20 <vfs_read_pipe+0xe8>
    9e98:	e3530000 	cmp	r3, #0
    9e9c:	1afffff7 	bne	9e80 <vfs_read_pipe+0x148>
    9ea0:	eaffffdd 	b	9e1c <vfs_read_pipe+0xe4>
    9ea4:	0000dbf8 	strdeq	sp, [r0], -r8
    9ea8:	00000018 	andeq	r0, r0, r8, lsl r0

00009eac <vfs_read>:
    9eac:	e1a0c00d 	mov	ip, sp
    9eb0:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    9eb4:	e24cb004 	sub	fp, ip, #4
    9eb8:	e24dd044 	sub	sp, sp, #68	; 0x44
    9ebc:	e1a07001 	mov	r7, r1
    9ec0:	e1a08003 	mov	r8, r3
    9ec4:	e5911044 	ldr	r1, [r1, #68]	; 0x44
    9ec8:	e59f31e8 	ldr	r3, [pc, #488]	; a0b8 <vfs_read+0x20c>
    9ecc:	e3510001 	cmp	r1, #1
    9ed0:	e08f3003 	add	r3, pc, r3
    9ed4:	e50b306c 	str	r3, [fp, #-108]	; 0x6c
    9ed8:	13a03000 	movne	r3, #0
    9edc:	e50b2068 	str	r2, [fp, #-104]	; 0x68
    9ee0:	e50b0060 	str	r0, [fp, #-96]	; 0x60
    9ee4:	150b3064 	strne	r3, [fp, #-100]	; 0x64
    9ee8:	0a000060 	beq	a070 <vfs_read+0x1c4>
    9eec:	e358001f 	cmp	r8, #31
    9ef0:	9a000009 	bls	9f1c <vfs_read+0x70>
    9ef4:	e1a00008 	mov	r0, r8
    9ef8:	e3a01001 	mov	r1, #1
    9efc:	eb000a4f 	bl	c840 <shm_alloc>
    9f00:	e2509000 	subs	r9, r0, #0
    9f04:	ba000002 	blt	9f14 <vfs_read+0x68>
    9f08:	eb000a57 	bl	c86c <shm_map>
    9f0c:	e250a000 	subs	sl, r0, #0
    9f10:	1a000003 	bne	9f24 <vfs_read+0x78>
    9f14:	e3e00000 	mvn	r0, #0
    9f18:	ea00003f 	b	a01c <vfs_read+0x170>
    9f1c:	e3a0a000 	mov	sl, #0
    9f20:	e3e09000 	mvn	r9, #0
    9f24:	eb000747 	bl	bc48 <get_proto_factor>
    9f28:	e24b6044 	sub	r6, fp, #68	; 0x44
    9f2c:	e5903004 	ldr	r3, [r0, #4]
    9f30:	e1a00006 	mov	r0, r6
    9f34:	e1a0e00f 	mov	lr, pc
    9f38:	e12fff13 	bx	r3
    9f3c:	eb000741 	bl	bc48 <get_proto_factor>
    9f40:	e24b405c 	sub	r4, fp, #92	; 0x5c
    9f44:	e5903004 	ldr	r3, [r0, #4]
    9f48:	e1a00004 	mov	r0, r4
    9f4c:	e1a0e00f 	mov	lr, pc
    9f50:	e12fff13 	bx	r3
    9f54:	e51b1060 	ldr	r1, [fp, #-96]	; 0x60
    9f58:	e5903014 	ldr	r3, [r0, #20]
    9f5c:	e1a00004 	mov	r0, r4
    9f60:	e1a0e00f 	mov	lr, pc
    9f64:	e12fff13 	bx	r3
    9f68:	e3a0205c 	mov	r2, #92	; 0x5c
    9f6c:	e5903010 	ldr	r3, [r0, #16]
    9f70:	e1a01007 	mov	r1, r7
    9f74:	e1a00004 	mov	r0, r4
    9f78:	e1a0e00f 	mov	lr, pc
    9f7c:	e12fff13 	bx	r3
    9f80:	e1a01008 	mov	r1, r8
    9f84:	e5903014 	ldr	r3, [r0, #20]
    9f88:	e1a00004 	mov	r0, r4
    9f8c:	e1a0e00f 	mov	lr, pc
    9f90:	e12fff13 	bx	r3
    9f94:	e51b1064 	ldr	r1, [fp, #-100]	; 0x64
    9f98:	e5903014 	ldr	r3, [r0, #20]
    9f9c:	e1a00004 	mov	r0, r4
    9fa0:	e1a0e00f 	mov	lr, pc
    9fa4:	e12fff13 	bx	r3
    9fa8:	e1a01009 	mov	r1, r9
    9fac:	e5903014 	ldr	r3, [r0, #20]
    9fb0:	e1a00004 	mov	r0, r4
    9fb4:	e1a0e00f 	mov	lr, pc
    9fb8:	e12fff13 	bx	r3
    9fbc:	e1a02004 	mov	r2, r4
    9fc0:	e1a03006 	mov	r3, r6
    9fc4:	e5970054 	ldr	r0, [r7, #84]	; 0x54
    9fc8:	e3a01004 	mov	r1, #4
    9fcc:	eb00016d 	bl	a588 <ipc_call>
    9fd0:	e3500000 	cmp	r0, #0
    9fd4:	13e05000 	mvnne	r5, #0
    9fd8:	0a000012 	beq	a028 <vfs_read+0x17c>
    9fdc:	eb000719 	bl	bc48 <get_proto_factor>
    9fe0:	e590300c 	ldr	r3, [r0, #12]
    9fe4:	e1a00004 	mov	r0, r4
    9fe8:	e1a0e00f 	mov	lr, pc
    9fec:	e12fff13 	bx	r3
    9ff0:	eb000714 	bl	bc48 <get_proto_factor>
    9ff4:	e590300c 	ldr	r3, [r0, #12]
    9ff8:	e1a00006 	mov	r0, r6
    9ffc:	e1a0e00f 	mov	lr, pc
    a000:	e12fff13 	bx	r3
    a004:	e35a0000 	cmp	sl, #0
    a008:	01a00005 	moveq	r0, r5
    a00c:	0a000002 	beq	a01c <vfs_read+0x170>
    a010:	e1a00009 	mov	r0, r9
    a014:	eb000a1d 	bl	c890 <shm_unmap>
    a018:	e1a00005 	mov	r0, r5
    a01c:	e24bd028 	sub	sp, fp, #40	; 0x28
    a020:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    a024:	e12fff1e 	bx	lr
    a028:	e1a00006 	mov	r0, r6
    a02c:	eb0007ce 	bl	bf6c <proto_read_int>
    a030:	e2505000 	subs	r5, r0, #0
    a034:	da000011 	ble	a080 <vfs_read+0x1d4>
    a038:	e35a0000 	cmp	sl, #0
    a03c:	0a000018 	beq	a0a4 <vfs_read+0x1f8>
    a040:	e51b0068 	ldr	r0, [fp, #-104]	; 0x68
    a044:	e1a0100a 	mov	r1, sl
    a048:	e1a02005 	mov	r2, r5
    a04c:	eb000fa9 	bl	def8 <memcpy>
    a050:	e5973044 	ldr	r3, [r7, #68]	; 0x44
    a054:	e3530001 	cmp	r3, #1
    a058:	1affffdf 	bne	9fdc <vfs_read+0x130>
    a05c:	e51b3064 	ldr	r3, [fp, #-100]	; 0x64
    a060:	e51b0060 	ldr	r0, [fp, #-96]	; 0x60
    a064:	e0831005 	add	r1, r3, r5
    a068:	ebfffc7d 	bl	9264 <vfs_seek>
    a06c:	eaffffda 	b	9fdc <vfs_read+0x130>
    a070:	ebfffc4c 	bl	91a8 <vfs_tell>
    a074:	e1c03fc0 	bic	r3, r0, r0, asr #31
    a078:	e50b3064 	str	r3, [fp, #-100]	; 0x64
    a07c:	eaffff9a 	b	9eec <vfs_read+0x40>
    a080:	e3750002 	cmn	r5, #2
    a084:	1affffd4 	bne	9fdc <vfs_read+0x130>
    a088:	e3a02001 	mov	r2, #1
    a08c:	e59f3028 	ldr	r3, [pc, #40]	; a0bc <vfs_read+0x210>
    a090:	e51b106c 	ldr	r1, [fp, #-108]	; 0x6c
    a094:	e7913003 	ldr	r3, [r1, r3]
    a098:	e3e05000 	mvn	r5, #0
    a09c:	e5832000 	str	r2, [r3]
    a0a0:	eaffffcd 	b	9fdc <vfs_read+0x130>
    a0a4:	e51b1068 	ldr	r1, [fp, #-104]	; 0x68
    a0a8:	e1a02008 	mov	r2, r8
    a0ac:	e1a00006 	mov	r0, r6
    a0b0:	eb000742 	bl	bdc0 <proto_read_to>
    a0b4:	eaffffe5 	b	a050 <vfs_read+0x1a4>
    a0b8:	0000db28 	andeq	sp, r0, r8, lsr #22
    a0bc:	00000018 	andeq	r0, r0, r8, lsl r0

0000a0c0 <vfs_write_pipe>:
    a0c0:	e1a0c00d 	mov	ip, sp
    a0c4:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    a0c8:	e24cb004 	sub	fp, ip, #4
    a0cc:	e24dd030 	sub	sp, sp, #48	; 0x30
    a0d0:	e1a06001 	mov	r6, r1
    a0d4:	e1a05002 	mov	r5, r2
    a0d8:	e1a08003 	mov	r8, r3
    a0dc:	e1a07000 	mov	r7, r0
    a0e0:	eb0006d8 	bl	bc48 <get_proto_factor>
    a0e4:	e24b4054 	sub	r4, fp, #84	; 0x54
    a0e8:	e5903004 	ldr	r3, [r0, #4]
    a0ec:	e1a00004 	mov	r0, r4
    a0f0:	e1a0e00f 	mov	lr, pc
    a0f4:	e12fff13 	bx	r3
    a0f8:	e1a01007 	mov	r1, r7
    a0fc:	e5903010 	ldr	r3, [r0, #16]
    a100:	e3a0205c 	mov	r2, #92	; 0x5c
    a104:	e1a00004 	mov	r0, r4
    a108:	e1a0e00f 	mov	lr, pc
    a10c:	e12fff13 	bx	r3
    a110:	e1a02005 	mov	r2, r5
    a114:	e1a01006 	mov	r1, r6
    a118:	e5903010 	ldr	r3, [r0, #16]
    a11c:	e1a00004 	mov	r0, r4
    a120:	e1a0e00f 	mov	lr, pc
    a124:	e12fff13 	bx	r3
    a128:	e1a01008 	mov	r1, r8
    a12c:	e5903014 	ldr	r3, [r0, #20]
    a130:	e1a00004 	mov	r0, r4
    a134:	e1a0e00f 	mov	lr, pc
    a138:	e12fff13 	bx	r3
    a13c:	eb0006c1 	bl	bc48 <get_proto_factor>
    a140:	e24b603c 	sub	r6, fp, #60	; 0x3c
    a144:	e5903004 	ldr	r3, [r0, #4]
    a148:	e1a00006 	mov	r0, r6
    a14c:	e1a0e00f 	mov	lr, pc
    a150:	e12fff13 	bx	r3
    a154:	eb0009dc 	bl	c8cc <get_vfsd_pid>
    a158:	e1a02004 	mov	r2, r4
    a15c:	e1a03006 	mov	r3, r6
    a160:	e3a01006 	mov	r1, #6
    a164:	e1a09000 	mov	r9, r0
    a168:	eb000106 	bl	a588 <ipc_call>
    a16c:	e1a05000 	mov	r5, r0
    a170:	eb0006b4 	bl	bc48 <get_proto_factor>
    a174:	e590300c 	ldr	r3, [r0, #12]
    a178:	e1a00004 	mov	r0, r4
    a17c:	e59f4094 	ldr	r4, [pc, #148]	; a218 <vfs_write_pipe+0x158>
    a180:	e1a0e00f 	mov	lr, pc
    a184:	e12fff13 	bx	r3
    a188:	e3550000 	cmp	r5, #0
    a18c:	e08f4004 	add	r4, pc, r4
    a190:	0a000008 	beq	a1b8 <vfs_write_pipe+0xf8>
    a194:	eb0006ab 	bl	bc48 <get_proto_factor>
    a198:	e590300c 	ldr	r3, [r0, #12]
    a19c:	e1a00006 	mov	r0, r6
    a1a0:	e1a0e00f 	mov	lr, pc
    a1a4:	e12fff13 	bx	r3
    a1a8:	e1c50fc5 	bic	r0, r5, r5, asr #31
    a1ac:	e24bd024 	sub	sp, fp, #36	; 0x24
    a1b0:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    a1b4:	e12fff1e 	bx	lr
    a1b8:	e1a00006 	mov	r0, r6
    a1bc:	eb00076a 	bl	bf6c <proto_read_int>
    a1c0:	e1a05000 	mov	r5, r0
    a1c4:	eb00069f 	bl	bc48 <get_proto_factor>
    a1c8:	e590300c 	ldr	r3, [r0, #12]
    a1cc:	e1a00006 	mov	r0, r6
    a1d0:	e1a0e00f 	mov	lr, pc
    a1d4:	e12fff13 	bx	r3
    a1d8:	e2753001 	rsbs	r3, r5, #1
    a1dc:	33a03000 	movcc	r3, #0
    a1e0:	e1130008 	tst	r3, r8
    a1e4:	0a000008 	beq	a20c <vfs_write_pipe+0x14c>
    a1e8:	e1a00009 	mov	r0, r9
    a1ec:	e5971000 	ldr	r1, [r7]
    a1f0:	eb0009fb 	bl	c9e4 <proc_block>
    a1f4:	e3a02001 	mov	r2, #1
    a1f8:	e59f301c 	ldr	r3, [pc, #28]	; a21c <vfs_write_pipe+0x15c>
    a1fc:	e7943003 	ldr	r3, [r4, r3]
    a200:	e3e00000 	mvn	r0, #0
    a204:	e5832000 	str	r2, [r3]
    a208:	eaffffe7 	b	a1ac <vfs_write_pipe+0xec>
    a20c:	e3530000 	cmp	r3, #0
    a210:	1afffff7 	bne	a1f4 <vfs_write_pipe+0x134>
    a214:	eaffffe3 	b	a1a8 <vfs_write_pipe+0xe8>
    a218:	0000d86c 	andeq	sp, r0, ip, ror #16
    a21c:	00000018 	andeq	r0, r0, r8, lsl r0

0000a220 <vfs_write>:
    a220:	e1a0c00d 	mov	ip, sp
    a224:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    a228:	e24cb004 	sub	fp, ip, #4
    a22c:	e24dd044 	sub	sp, sp, #68	; 0x44
    a230:	e1a05001 	mov	r5, r1
    a234:	e1a07003 	mov	r7, r3
    a238:	e5911044 	ldr	r1, [r1, #68]	; 0x44
    a23c:	e59f3200 	ldr	r3, [pc, #512]	; a444 <vfs_write+0x224>
    a240:	e3510000 	cmp	r1, #0
    a244:	e08f3003 	add	r3, pc, r3
    a248:	e50b2064 	str	r2, [fp, #-100]	; 0x64
    a24c:	e50b3068 	str	r3, [fp, #-104]	; 0x68
    a250:	e1a09000 	mov	r9, r0
    a254:	0a000078 	beq	a43c <vfs_write+0x21c>
    a258:	e3510001 	cmp	r1, #1
    a25c:	13a0a000 	movne	sl, #0
    a260:	0a000072 	beq	a430 <vfs_write+0x210>
    a264:	e357001f 	cmp	r7, #31
    a268:	8a000047 	bhi	a38c <vfs_write+0x16c>
    a26c:	e3a03000 	mov	r3, #0
    a270:	e3e08000 	mvn	r8, #0
    a274:	e50b3060 	str	r3, [fp, #-96]	; 0x60
    a278:	eb000672 	bl	bc48 <get_proto_factor>
    a27c:	e24b6044 	sub	r6, fp, #68	; 0x44
    a280:	e5903004 	ldr	r3, [r0, #4]
    a284:	e1a00006 	mov	r0, r6
    a288:	e1a0e00f 	mov	lr, pc
    a28c:	e12fff13 	bx	r3
    a290:	eb00066c 	bl	bc48 <get_proto_factor>
    a294:	e24b405c 	sub	r4, fp, #92	; 0x5c
    a298:	e5903004 	ldr	r3, [r0, #4]
    a29c:	e1a00004 	mov	r0, r4
    a2a0:	e1a0e00f 	mov	lr, pc
    a2a4:	e12fff13 	bx	r3
    a2a8:	e1a01009 	mov	r1, r9
    a2ac:	e5903014 	ldr	r3, [r0, #20]
    a2b0:	e1a00004 	mov	r0, r4
    a2b4:	e1a0e00f 	mov	lr, pc
    a2b8:	e12fff13 	bx	r3
    a2bc:	e3a0205c 	mov	r2, #92	; 0x5c
    a2c0:	e5903010 	ldr	r3, [r0, #16]
    a2c4:	e1a01005 	mov	r1, r5
    a2c8:	e1a00004 	mov	r0, r4
    a2cc:	e1a0e00f 	mov	lr, pc
    a2d0:	e12fff13 	bx	r3
    a2d4:	e1a0100a 	mov	r1, sl
    a2d8:	e5903014 	ldr	r3, [r0, #20]
    a2dc:	e1a00004 	mov	r0, r4
    a2e0:	e1a0e00f 	mov	lr, pc
    a2e4:	e12fff13 	bx	r3
    a2e8:	e1a01008 	mov	r1, r8
    a2ec:	e5903014 	ldr	r3, [r0, #20]
    a2f0:	e1a00004 	mov	r0, r4
    a2f4:	e1a0e00f 	mov	lr, pc
    a2f8:	e12fff13 	bx	r3
    a2fc:	e3780001 	cmn	r8, #1
    a300:	0a000039 	beq	a3ec <vfs_write+0x1cc>
    a304:	eb00064f 	bl	bc48 <get_proto_factor>
    a308:	e1a01007 	mov	r1, r7
    a30c:	e5903014 	ldr	r3, [r0, #20]
    a310:	e1a00004 	mov	r0, r4
    a314:	e1a0e00f 	mov	lr, pc
    a318:	e12fff13 	bx	r3
    a31c:	e5950054 	ldr	r0, [r5, #84]	; 0x54
    a320:	e3a01005 	mov	r1, #5
    a324:	e1a02004 	mov	r2, r4
    a328:	e1a03006 	mov	r3, r6
    a32c:	eb000095 	bl	a588 <ipc_call>
    a330:	e3500000 	cmp	r0, #0
    a334:	13e07000 	mvnne	r7, #0
    a338:	0a000020 	beq	a3c0 <vfs_write+0x1a0>
    a33c:	eb000641 	bl	bc48 <get_proto_factor>
    a340:	e590300c 	ldr	r3, [r0, #12]
    a344:	e1a00004 	mov	r0, r4
    a348:	e1a0e00f 	mov	lr, pc
    a34c:	e12fff13 	bx	r3
    a350:	eb00063c 	bl	bc48 <get_proto_factor>
    a354:	e590300c 	ldr	r3, [r0, #12]
    a358:	e1a00006 	mov	r0, r6
    a35c:	e1a0e00f 	mov	lr, pc
    a360:	e12fff13 	bx	r3
    a364:	e51b3060 	ldr	r3, [fp, #-96]	; 0x60
    a368:	e3530000 	cmp	r3, #0
    a36c:	01a00007 	moveq	r0, r7
    a370:	0a000002 	beq	a380 <vfs_write+0x160>
    a374:	e1a00008 	mov	r0, r8
    a378:	eb000944 	bl	c890 <shm_unmap>
    a37c:	e1a00007 	mov	r0, r7
    a380:	e24bd028 	sub	sp, fp, #40	; 0x28
    a384:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    a388:	e12fff1e 	bx	lr
    a38c:	e1a00007 	mov	r0, r7
    a390:	e3a01001 	mov	r1, #1
    a394:	eb000929 	bl	c840 <shm_alloc>
    a398:	e2508000 	subs	r8, r0, #0
    a39c:	ba000026 	blt	a43c <vfs_write+0x21c>
    a3a0:	eb000931 	bl	c86c <shm_map>
    a3a4:	e2503000 	subs	r3, r0, #0
    a3a8:	e50b3060 	str	r3, [fp, #-96]	; 0x60
    a3ac:	0a000022 	beq	a43c <vfs_write+0x21c>
    a3b0:	e51b1064 	ldr	r1, [fp, #-100]	; 0x64
    a3b4:	e1a02007 	mov	r2, r7
    a3b8:	eb000ece 	bl	def8 <memcpy>
    a3bc:	eaffffad 	b	a278 <vfs_write+0x58>
    a3c0:	e1a00006 	mov	r0, r6
    a3c4:	eb0006e8 	bl	bf6c <proto_read_int>
    a3c8:	e2507000 	subs	r7, r0, #0
    a3cc:	da00000e 	ble	a40c <vfs_write+0x1ec>
    a3d0:	e5953044 	ldr	r3, [r5, #68]	; 0x44
    a3d4:	e3530001 	cmp	r3, #1
    a3d8:	1affffd7 	bne	a33c <vfs_write+0x11c>
    a3dc:	e1a00009 	mov	r0, r9
    a3e0:	e08a1007 	add	r1, sl, r7
    a3e4:	ebfffb9e 	bl	9264 <vfs_seek>
    a3e8:	eaffffd3 	b	a33c <vfs_write+0x11c>
    a3ec:	eb000615 	bl	bc48 <get_proto_factor>
    a3f0:	e51b1064 	ldr	r1, [fp, #-100]	; 0x64
    a3f4:	e5903010 	ldr	r3, [r0, #16]
    a3f8:	e1a02007 	mov	r2, r7
    a3fc:	e1a00004 	mov	r0, r4
    a400:	e1a0e00f 	mov	lr, pc
    a404:	e12fff13 	bx	r3
    a408:	eaffffc3 	b	a31c <vfs_write+0xfc>
    a40c:	e3770002 	cmn	r7, #2
    a410:	1affffc9 	bne	a33c <vfs_write+0x11c>
    a414:	e3a02001 	mov	r2, #1
    a418:	e59f3028 	ldr	r3, [pc, #40]	; a448 <vfs_write+0x228>
    a41c:	e51b1068 	ldr	r1, [fp, #-104]	; 0x68
    a420:	e7913003 	ldr	r3, [r1, r3]
    a424:	e3e07000 	mvn	r7, #0
    a428:	e5832000 	str	r2, [r3]
    a42c:	eaffffc2 	b	a33c <vfs_write+0x11c>
    a430:	ebfffb5c 	bl	91a8 <vfs_tell>
    a434:	e1c0afc0 	bic	sl, r0, r0, asr #31
    a438:	eaffff89 	b	a264 <vfs_write+0x44>
    a43c:	e3e00000 	mvn	r0, #0
    a440:	eaffffce 	b	a380 <vfs_write+0x160>
    a444:	0000d7b4 			; <UNDEFINED> instruction: 0x0000d7b4
    a448:	00000018 	andeq	r0, r0, r8, lsl r0

0000a44c <handle_ipc>:
    a44c:	e1a0c00d 	mov	ip, sp
    a450:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    a454:	e1a05000 	mov	r5, r0
    a458:	e24cb004 	sub	fp, ip, #4
    a45c:	e24dd028 	sub	sp, sp, #40	; 0x28
    a460:	e1a07001 	mov	r7, r1
    a464:	e3a00025 	mov	r0, #37	; 0x25
    a468:	e1a01005 	mov	r1, r5
    a46c:	e24b203c 	sub	r2, fp, #60	; 0x3c
    a470:	e24b3038 	sub	r3, fp, #56	; 0x38
    a474:	ebfff5b1 	bl	7b40 <syscall3>
    a478:	e51b1038 	ldr	r1, [fp, #-56]	; 0x38
    a47c:	e3510000 	cmp	r1, #0
    a480:	e1a06000 	mov	r6, r0
    a484:	ba000020 	blt	a50c <handle_ipc+0xc0>
    a488:	eb0005ee 	bl	bc48 <get_proto_factor>
    a48c:	e24b4034 	sub	r4, fp, #52	; 0x34
    a490:	e5903004 	ldr	r3, [r0, #4]
    a494:	e1a00004 	mov	r0, r4
    a498:	e1a0e00f 	mov	lr, pc
    a49c:	e12fff13 	bx	r3
    a4a0:	e59fc098 	ldr	ip, [pc, #152]	; a540 <handle_ipc+0xf4>
    a4a4:	e24b003c 	sub	r0, fp, #60	; 0x3c
    a4a8:	e8900003 	ldm	r0, {r0, r1}
    a4ac:	e1a03004 	mov	r3, r4
    a4b0:	e08fc00c 	add	ip, pc, ip
    a4b4:	e1a02006 	mov	r2, r6
    a4b8:	e58d7000 	str	r7, [sp]
    a4bc:	e59cc000 	ldr	ip, [ip]
    a4c0:	e1a0e00f 	mov	lr, pc
    a4c4:	e12fff1c 	bx	ip
    a4c8:	e1a00006 	mov	r0, r6
    a4cc:	eb0006e9 	bl	c078 <proto_free>
    a4d0:	e1a02004 	mov	r2, r4
    a4d4:	e1a01005 	mov	r1, r5
    a4d8:	e3a00026 	mov	r0, #38	; 0x26
    a4dc:	ebfff598 	bl	7b44 <syscall2>
    a4e0:	eb0005d8 	bl	bc48 <get_proto_factor>
    a4e4:	e590300c 	ldr	r3, [r0, #12]
    a4e8:	e1a00004 	mov	r0, r4
    a4ec:	e1a0e00f 	mov	lr, pc
    a4f0:	e12fff13 	bx	r3
    a4f4:	e1a01005 	mov	r1, r5
    a4f8:	e3a00028 	mov	r0, #40	; 0x28
    a4fc:	ebfff592 	bl	7b4c <syscall1>
    a500:	e24bd01c 	sub	sp, fp, #28
    a504:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    a508:	e12fff1e 	bx	lr
    a50c:	e59fc030 	ldr	ip, [pc, #48]	; a544 <handle_ipc+0xf8>
    a510:	e51b003c 	ldr	r0, [fp, #-60]	; 0x3c
    a514:	e58d7000 	str	r7, [sp]
    a518:	e3c11102 	bic	r1, r1, #-2147483648	; 0x80000000
    a51c:	e08fc00c 	add	ip, pc, ip
    a520:	e1a02006 	mov	r2, r6
    a524:	e3a03000 	mov	r3, #0
    a528:	e59cc000 	ldr	ip, [ip]
    a52c:	e1a0e00f 	mov	lr, pc
    a530:	e12fff1c 	bx	ip
    a534:	e1a00006 	mov	r0, r6
    a538:	eb0006ce 	bl	c078 <proto_free>
    a53c:	eaffffec 	b	a4f4 <handle_ipc+0xa8>
    a540:	00042f08 	andeq	r2, r4, r8, lsl #30
    a544:	00042e9c 	muleq	r4, ip, lr

0000a548 <ipc_lock>:
    a548:	e1a0c00d 	mov	ip, sp
    a54c:	e3a00029 	mov	r0, #41	; 0x29
    a550:	e92dd800 	push	{fp, ip, lr, pc}
    a554:	e24cb004 	sub	fp, ip, #4
    a558:	ebfff57e 	bl	7b58 <syscall0>
    a55c:	e24bd00c 	sub	sp, fp, #12
    a560:	e89d6800 	ldm	sp, {fp, sp, lr}
    a564:	e12fff1e 	bx	lr

0000a568 <ipc_unlock>:
    a568:	e1a0c00d 	mov	ip, sp
    a56c:	e3a0002a 	mov	r0, #42	; 0x2a
    a570:	e92dd800 	push	{fp, ip, lr, pc}
    a574:	e24cb004 	sub	fp, ip, #4
    a578:	ebfff576 	bl	7b58 <syscall0>
    a57c:	e24bd00c 	sub	sp, fp, #12
    a580:	e89d6800 	ldm	sp, {fp, sp, lr}
    a584:	e12fff1e 	bx	lr

0000a588 <ipc_call>:
    a588:	e1a0c00d 	mov	ip, sp
    a58c:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
    a590:	e2508000 	subs	r8, r0, #0
    a594:	e24cb004 	sub	fp, ip, #4
    a598:	e1a06001 	mov	r6, r1
    a59c:	e1a07002 	mov	r7, r2
    a5a0:	e1a04003 	mov	r4, r3
    a5a4:	ba00001f 	blt	a628 <ipc_call+0xa0>
    a5a8:	e3540000 	cmp	r4, #0
    a5ac:	03866102 	orreq	r6, r6, #-2147483648	; 0x80000000
    a5b0:	e3a00024 	mov	r0, #36	; 0x24
    a5b4:	e1a01008 	mov	r1, r8
    a5b8:	e1a03007 	mov	r3, r7
    a5bc:	e1a02006 	mov	r2, r6
    a5c0:	ebfff55e 	bl	7b40 <syscall3>
    a5c4:	e3700001 	cmn	r0, #1
    a5c8:	e1a05000 	mov	r5, r0
    a5cc:	0afffff5 	beq	a5a8 <ipc_call+0x20>
    a5d0:	e3500000 	cmp	r0, #0
    a5d4:	0a000013 	beq	a628 <ipc_call+0xa0>
    a5d8:	e3540000 	cmp	r4, #0
    a5dc:	01a00004 	moveq	r0, r4
    a5e0:	0a00000d 	beq	a61c <ipc_call+0x94>
    a5e4:	eb000597 	bl	bc48 <get_proto_factor>
    a5e8:	e590300c 	ldr	r3, [r0, #12]
    a5ec:	e1a00004 	mov	r0, r4
    a5f0:	e1a0e00f 	mov	lr, pc
    a5f4:	e12fff13 	bx	r3
    a5f8:	ea000001 	b	a604 <ipc_call+0x7c>
    a5fc:	e3a00000 	mov	r0, #0
    a600:	eb000a3e 	bl	cf00 <sleep>
    a604:	e3a00027 	mov	r0, #39	; 0x27
    a608:	e1a01005 	mov	r1, r5
    a60c:	e1a02004 	mov	r2, r4
    a610:	ebfff54b 	bl	7b44 <syscall2>
    a614:	e3700001 	cmn	r0, #1
    a618:	0afffff7 	beq	a5fc <ipc_call+0x74>
    a61c:	e24bd024 	sub	sp, fp, #36	; 0x24
    a620:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
    a624:	e12fff1e 	bx	lr
    a628:	e3e00000 	mvn	r0, #0
    a62c:	eafffffa 	b	a61c <ipc_call+0x94>

0000a630 <ipc_call_wait>:
    a630:	e1a0c00d 	mov	ip, sp
    a634:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    a638:	e24cb004 	sub	fp, ip, #4
    a63c:	e24dd01c 	sub	sp, sp, #28
    a640:	e2535000 	subs	r5, r3, #0
    a644:	e1a06000 	mov	r6, r0
    a648:	e1a07001 	mov	r7, r1
    a64c:	e1a08002 	mov	r8, r2
    a650:	0a00001e 	beq	a6d0 <ipc_call_wait+0xa0>
    a654:	e3500000 	cmp	r0, #0
    a658:	ba000046 	blt	a778 <ipc_call_wait+0x148>
    a65c:	e3a00024 	mov	r0, #36	; 0x24
    a660:	e1a01006 	mov	r1, r6
    a664:	e1a02007 	mov	r2, r7
    a668:	e1a03008 	mov	r3, r8
    a66c:	ebfff533 	bl	7b40 <syscall3>
    a670:	e3700001 	cmn	r0, #1
    a674:	e1a04000 	mov	r4, r0
    a678:	0afffff7 	beq	a65c <ipc_call_wait+0x2c>
    a67c:	e3500000 	cmp	r0, #0
    a680:	0a00003c 	beq	a778 <ipc_call_wait+0x148>
    a684:	eb00056f 	bl	bc48 <get_proto_factor>
    a688:	e590300c 	ldr	r3, [r0, #12]
    a68c:	e1a00005 	mov	r0, r5
    a690:	e1a0e00f 	mov	lr, pc
    a694:	e12fff13 	bx	r3
    a698:	ea000001 	b	a6a4 <ipc_call_wait+0x74>
    a69c:	e3a00000 	mov	r0, #0
    a6a0:	eb000a16 	bl	cf00 <sleep>
    a6a4:	e3a00027 	mov	r0, #39	; 0x27
    a6a8:	e1a01004 	mov	r1, r4
    a6ac:	e1a02005 	mov	r2, r5
    a6b0:	ebfff523 	bl	7b44 <syscall2>
    a6b4:	e3700001 	cmn	r0, #1
    a6b8:	0afffff7 	beq	a69c <ipc_call_wait+0x6c>
    a6bc:	e1a04000 	mov	r4, r0
    a6c0:	e1a00004 	mov	r0, r4
    a6c4:	e24bd020 	sub	sp, fp, #32
    a6c8:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    a6cc:	e12fff1e 	bx	lr
    a6d0:	eb00055c 	bl	bc48 <get_proto_factor>
    a6d4:	e24b503c 	sub	r5, fp, #60	; 0x3c
    a6d8:	e5903004 	ldr	r3, [r0, #4]
    a6dc:	e1a00005 	mov	r0, r5
    a6e0:	e1a0e00f 	mov	lr, pc
    a6e4:	e12fff13 	bx	r3
    a6e8:	e3560000 	cmp	r6, #0
    a6ec:	ba000023 	blt	a780 <ipc_call_wait+0x150>
    a6f0:	e3a00024 	mov	r0, #36	; 0x24
    a6f4:	e1a01006 	mov	r1, r6
    a6f8:	e1a02007 	mov	r2, r7
    a6fc:	e1a03008 	mov	r3, r8
    a700:	ebfff50e 	bl	7b40 <syscall3>
    a704:	e3700001 	cmn	r0, #1
    a708:	e1a04000 	mov	r4, r0
    a70c:	0afffff7 	beq	a6f0 <ipc_call_wait+0xc0>
    a710:	e3500000 	cmp	r0, #0
    a714:	0a000019 	beq	a780 <ipc_call_wait+0x150>
    a718:	eb00054a 	bl	bc48 <get_proto_factor>
    a71c:	e590300c 	ldr	r3, [r0, #12]
    a720:	e1a00005 	mov	r0, r5
    a724:	e1a0e00f 	mov	lr, pc
    a728:	e12fff13 	bx	r3
    a72c:	ea000001 	b	a738 <ipc_call_wait+0x108>
    a730:	e3a00000 	mov	r0, #0
    a734:	eb0009f1 	bl	cf00 <sleep>
    a738:	e3a00027 	mov	r0, #39	; 0x27
    a73c:	e1a01004 	mov	r1, r4
    a740:	e1a02005 	mov	r2, r5
    a744:	ebfff4fe 	bl	7b44 <syscall2>
    a748:	e3700001 	cmn	r0, #1
    a74c:	0afffff7 	beq	a730 <ipc_call_wait+0x100>
    a750:	e1a04000 	mov	r4, r0
    a754:	eb00053b 	bl	bc48 <get_proto_factor>
    a758:	e590300c 	ldr	r3, [r0, #12]
    a75c:	e1a00005 	mov	r0, r5
    a760:	e1a0e00f 	mov	lr, pc
    a764:	e12fff13 	bx	r3
    a768:	e1a00004 	mov	r0, r4
    a76c:	e24bd020 	sub	sp, fp, #32
    a770:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    a774:	e12fff1e 	bx	lr
    a778:	e3e04000 	mvn	r4, #0
    a77c:	eaffffcf 	b	a6c0 <ipc_call_wait+0x90>
    a780:	e3e04000 	mvn	r4, #0
    a784:	eafffff2 	b	a754 <ipc_call_wait+0x124>

0000a788 <ipc_serv_reg>:
    a788:	e1a0c00d 	mov	ip, sp
    a78c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    a790:	e24cb004 	sub	fp, ip, #4
    a794:	e24dd030 	sub	sp, sp, #48	; 0x30
    a798:	e1a05000 	mov	r5, r0
    a79c:	e3a0002c 	mov	r0, #44	; 0x2c
    a7a0:	ebfff4ec 	bl	7b58 <syscall0>
    a7a4:	e2507000 	subs	r7, r0, #0
    a7a8:	ba00003d 	blt	a8a4 <ipc_serv_reg+0x11c>
    a7ac:	eb000525 	bl	bc48 <get_proto_factor>
    a7b0:	e24b4034 	sub	r4, fp, #52	; 0x34
    a7b4:	e5903004 	ldr	r3, [r0, #4]
    a7b8:	e1a00004 	mov	r0, r4
    a7bc:	e1a0e00f 	mov	lr, pc
    a7c0:	e12fff13 	bx	r3
    a7c4:	eb00051f 	bl	bc48 <get_proto_factor>
    a7c8:	e24b604c 	sub	r6, fp, #76	; 0x4c
    a7cc:	e5903004 	ldr	r3, [r0, #4]
    a7d0:	e1a00006 	mov	r0, r6
    a7d4:	e1a0e00f 	mov	lr, pc
    a7d8:	e12fff13 	bx	r3
    a7dc:	e1a01005 	mov	r1, r5
    a7e0:	e5903018 	ldr	r3, [r0, #24]
    a7e4:	e1a00006 	mov	r0, r6
    a7e8:	e1a0e00f 	mov	lr, pc
    a7ec:	e12fff13 	bx	r3
    a7f0:	e3a00024 	mov	r0, #36	; 0x24
    a7f4:	e1a01007 	mov	r1, r7
    a7f8:	e3a02000 	mov	r2, #0
    a7fc:	e1a03006 	mov	r3, r6
    a800:	ebfff4ce 	bl	7b40 <syscall3>
    a804:	e3700001 	cmn	r0, #1
    a808:	e1a05000 	mov	r5, r0
    a80c:	0afffff7 	beq	a7f0 <ipc_serv_reg+0x68>
    a810:	e3500000 	cmp	r0, #0
    a814:	0a00000f 	beq	a858 <ipc_serv_reg+0xd0>
    a818:	eb00050a 	bl	bc48 <get_proto_factor>
    a81c:	e590300c 	ldr	r3, [r0, #12]
    a820:	e1a00004 	mov	r0, r4
    a824:	e1a0e00f 	mov	lr, pc
    a828:	e12fff13 	bx	r3
    a82c:	ea000001 	b	a838 <ipc_serv_reg+0xb0>
    a830:	e3a00000 	mov	r0, #0
    a834:	eb0009b1 	bl	cf00 <sleep>
    a838:	e3a00027 	mov	r0, #39	; 0x27
    a83c:	e1a01005 	mov	r1, r5
    a840:	e1a02004 	mov	r2, r4
    a844:	ebfff4be 	bl	7b44 <syscall2>
    a848:	e3700001 	cmn	r0, #1
    a84c:	0afffff7 	beq	a830 <ipc_serv_reg+0xa8>
    a850:	e3500000 	cmp	r0, #0
    a854:	0a00000e 	beq	a894 <ipc_serv_reg+0x10c>
    a858:	e3e05000 	mvn	r5, #0
    a85c:	eb0004f9 	bl	bc48 <get_proto_factor>
    a860:	e590300c 	ldr	r3, [r0, #12]
    a864:	e1a00006 	mov	r0, r6
    a868:	e1a0e00f 	mov	lr, pc
    a86c:	e12fff13 	bx	r3
    a870:	eb0004f4 	bl	bc48 <get_proto_factor>
    a874:	e590300c 	ldr	r3, [r0, #12]
    a878:	e1a00004 	mov	r0, r4
    a87c:	e1a0e00f 	mov	lr, pc
    a880:	e12fff13 	bx	r3
    a884:	e1a00005 	mov	r0, r5
    a888:	e24bd01c 	sub	sp, fp, #28
    a88c:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    a890:	e12fff1e 	bx	lr
    a894:	e1a00004 	mov	r0, r4
    a898:	eb0005b3 	bl	bf6c <proto_read_int>
    a89c:	e1a05000 	mov	r5, r0
    a8a0:	eaffffed 	b	a85c <ipc_serv_reg+0xd4>
    a8a4:	e3e00000 	mvn	r0, #0
    a8a8:	eafffff6 	b	a888 <ipc_serv_reg+0x100>

0000a8ac <ipc_serv_get>:
    a8ac:	e1a0c00d 	mov	ip, sp
    a8b0:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    a8b4:	e24cb004 	sub	fp, ip, #4
    a8b8:	e24dd030 	sub	sp, sp, #48	; 0x30
    a8bc:	e1a05000 	mov	r5, r0
    a8c0:	e3a0002c 	mov	r0, #44	; 0x2c
    a8c4:	ebfff4a3 	bl	7b58 <syscall0>
    a8c8:	e2507000 	subs	r7, r0, #0
    a8cc:	ba00003d 	blt	a9c8 <ipc_serv_get+0x11c>
    a8d0:	eb0004dc 	bl	bc48 <get_proto_factor>
    a8d4:	e24b4034 	sub	r4, fp, #52	; 0x34
    a8d8:	e5903004 	ldr	r3, [r0, #4]
    a8dc:	e1a00004 	mov	r0, r4
    a8e0:	e1a0e00f 	mov	lr, pc
    a8e4:	e12fff13 	bx	r3
    a8e8:	eb0004d6 	bl	bc48 <get_proto_factor>
    a8ec:	e24b604c 	sub	r6, fp, #76	; 0x4c
    a8f0:	e5903004 	ldr	r3, [r0, #4]
    a8f4:	e1a00006 	mov	r0, r6
    a8f8:	e1a0e00f 	mov	lr, pc
    a8fc:	e12fff13 	bx	r3
    a900:	e1a01005 	mov	r1, r5
    a904:	e5903018 	ldr	r3, [r0, #24]
    a908:	e1a00006 	mov	r0, r6
    a90c:	e1a0e00f 	mov	lr, pc
    a910:	e12fff13 	bx	r3
    a914:	e3a00024 	mov	r0, #36	; 0x24
    a918:	e1a01007 	mov	r1, r7
    a91c:	e3a02002 	mov	r2, #2
    a920:	e1a03006 	mov	r3, r6
    a924:	ebfff485 	bl	7b40 <syscall3>
    a928:	e3700001 	cmn	r0, #1
    a92c:	e1a05000 	mov	r5, r0
    a930:	0afffff7 	beq	a914 <ipc_serv_get+0x68>
    a934:	e3500000 	cmp	r0, #0
    a938:	0a00000f 	beq	a97c <ipc_serv_get+0xd0>
    a93c:	eb0004c1 	bl	bc48 <get_proto_factor>
    a940:	e590300c 	ldr	r3, [r0, #12]
    a944:	e1a00004 	mov	r0, r4
    a948:	e1a0e00f 	mov	lr, pc
    a94c:	e12fff13 	bx	r3
    a950:	ea000001 	b	a95c <ipc_serv_get+0xb0>
    a954:	e3a00000 	mov	r0, #0
    a958:	eb000968 	bl	cf00 <sleep>
    a95c:	e3a00027 	mov	r0, #39	; 0x27
    a960:	e1a01005 	mov	r1, r5
    a964:	e1a02004 	mov	r2, r4
    a968:	ebfff475 	bl	7b44 <syscall2>
    a96c:	e3700001 	cmn	r0, #1
    a970:	0afffff7 	beq	a954 <ipc_serv_get+0xa8>
    a974:	e3500000 	cmp	r0, #0
    a978:	0a00000e 	beq	a9b8 <ipc_serv_get+0x10c>
    a97c:	e3e05000 	mvn	r5, #0
    a980:	eb0004b0 	bl	bc48 <get_proto_factor>
    a984:	e590300c 	ldr	r3, [r0, #12]
    a988:	e1a00006 	mov	r0, r6
    a98c:	e1a0e00f 	mov	lr, pc
    a990:	e12fff13 	bx	r3
    a994:	eb0004ab 	bl	bc48 <get_proto_factor>
    a998:	e590300c 	ldr	r3, [r0, #12]
    a99c:	e1a00004 	mov	r0, r4
    a9a0:	e1a0e00f 	mov	lr, pc
    a9a4:	e12fff13 	bx	r3
    a9a8:	e1a00005 	mov	r0, r5
    a9ac:	e24bd01c 	sub	sp, fp, #28
    a9b0:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    a9b4:	e12fff1e 	bx	lr
    a9b8:	e1a00004 	mov	r0, r4
    a9bc:	eb00056a 	bl	bf6c <proto_read_int>
    a9c0:	e1a05000 	mov	r5, r0
    a9c4:	eaffffed 	b	a980 <ipc_serv_get+0xd4>
    a9c8:	e3e00000 	mvn	r0, #0
    a9cc:	eafffff6 	b	a9ac <ipc_serv_get+0x100>

0000a9d0 <ipc_serv_unreg>:
    a9d0:	e1a0c00d 	mov	ip, sp
    a9d4:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    a9d8:	e24cb004 	sub	fp, ip, #4
    a9dc:	e24dd030 	sub	sp, sp, #48	; 0x30
    a9e0:	e1a05000 	mov	r5, r0
    a9e4:	e3a0002c 	mov	r0, #44	; 0x2c
    a9e8:	ebfff45a 	bl	7b58 <syscall0>
    a9ec:	e2507000 	subs	r7, r0, #0
    a9f0:	ba00003d 	blt	aaec <ipc_serv_unreg+0x11c>
    a9f4:	eb000493 	bl	bc48 <get_proto_factor>
    a9f8:	e24b4034 	sub	r4, fp, #52	; 0x34
    a9fc:	e5903004 	ldr	r3, [r0, #4]
    aa00:	e1a00004 	mov	r0, r4
    aa04:	e1a0e00f 	mov	lr, pc
    aa08:	e12fff13 	bx	r3
    aa0c:	eb00048d 	bl	bc48 <get_proto_factor>
    aa10:	e24b604c 	sub	r6, fp, #76	; 0x4c
    aa14:	e5903004 	ldr	r3, [r0, #4]
    aa18:	e1a00006 	mov	r0, r6
    aa1c:	e1a0e00f 	mov	lr, pc
    aa20:	e12fff13 	bx	r3
    aa24:	e1a01005 	mov	r1, r5
    aa28:	e5903018 	ldr	r3, [r0, #24]
    aa2c:	e1a00006 	mov	r0, r6
    aa30:	e1a0e00f 	mov	lr, pc
    aa34:	e12fff13 	bx	r3
    aa38:	e3a00024 	mov	r0, #36	; 0x24
    aa3c:	e1a01007 	mov	r1, r7
    aa40:	e3a02001 	mov	r2, #1
    aa44:	e1a03006 	mov	r3, r6
    aa48:	ebfff43c 	bl	7b40 <syscall3>
    aa4c:	e3700001 	cmn	r0, #1
    aa50:	e1a05000 	mov	r5, r0
    aa54:	0afffff7 	beq	aa38 <ipc_serv_unreg+0x68>
    aa58:	e3500000 	cmp	r0, #0
    aa5c:	0a00000f 	beq	aaa0 <ipc_serv_unreg+0xd0>
    aa60:	eb000478 	bl	bc48 <get_proto_factor>
    aa64:	e590300c 	ldr	r3, [r0, #12]
    aa68:	e1a00004 	mov	r0, r4
    aa6c:	e1a0e00f 	mov	lr, pc
    aa70:	e12fff13 	bx	r3
    aa74:	ea000001 	b	aa80 <ipc_serv_unreg+0xb0>
    aa78:	e3a00000 	mov	r0, #0
    aa7c:	eb00091f 	bl	cf00 <sleep>
    aa80:	e3a00027 	mov	r0, #39	; 0x27
    aa84:	e1a01005 	mov	r1, r5
    aa88:	e1a02004 	mov	r2, r4
    aa8c:	ebfff42c 	bl	7b44 <syscall2>
    aa90:	e3700001 	cmn	r0, #1
    aa94:	0afffff7 	beq	aa78 <ipc_serv_unreg+0xa8>
    aa98:	e3500000 	cmp	r0, #0
    aa9c:	0a00000e 	beq	aadc <ipc_serv_unreg+0x10c>
    aaa0:	e3e05000 	mvn	r5, #0
    aaa4:	eb000467 	bl	bc48 <get_proto_factor>
    aaa8:	e590300c 	ldr	r3, [r0, #12]
    aaac:	e1a00006 	mov	r0, r6
    aab0:	e1a0e00f 	mov	lr, pc
    aab4:	e12fff13 	bx	r3
    aab8:	eb000462 	bl	bc48 <get_proto_factor>
    aabc:	e590300c 	ldr	r3, [r0, #12]
    aac0:	e1a00004 	mov	r0, r4
    aac4:	e1a0e00f 	mov	lr, pc
    aac8:	e12fff13 	bx	r3
    aacc:	e1a00005 	mov	r0, r5
    aad0:	e24bd01c 	sub	sp, fp, #28
    aad4:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    aad8:	e12fff1e 	bx	lr
    aadc:	e1a00004 	mov	r0, r4
    aae0:	eb000521 	bl	bf6c <proto_read_int>
    aae4:	e1a05000 	mov	r5, r0
    aae8:	eaffffed 	b	aaa4 <ipc_serv_unreg+0xd4>
    aaec:	e3e00000 	mvn	r0, #0
    aaf0:	eafffff6 	b	aad0 <ipc_serv_unreg+0x100>

0000aaf4 <ipc_serv_run>:
    aaf4:	e1a0c00d 	mov	ip, sp
    aaf8:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    aafc:	e1a05001 	mov	r5, r1
    ab00:	e1a04002 	mov	r4, r2
    ab04:	e59f3030 	ldr	r3, [pc, #48]	; ab3c <ipc_serv_run+0x48>
    ab08:	e24cb004 	sub	fp, ip, #4
    ab0c:	e08f3003 	add	r3, pc, r3
    ab10:	e5830000 	str	r0, [r3]
    ab14:	eb00079a 	bl	c984 <proc_ready_ping>
    ab18:	e59f1020 	ldr	r1, [pc, #32]	; ab40 <ipc_serv_run+0x4c>
    ab1c:	e1a02005 	mov	r2, r5
    ab20:	e1a03004 	mov	r3, r4
    ab24:	e08f1001 	add	r1, pc, r1
    ab28:	e3a00023 	mov	r0, #35	; 0x23
    ab2c:	ebfff403 	bl	7b40 <syscall3>
    ab30:	e24bd014 	sub	sp, fp, #20
    ab34:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    ab38:	e12fff1e 	bx	lr
    ab3c:	000428ac 	andeq	r2, r4, ip, lsr #17
    ab40:	fffff920 			; <UNDEFINED> instruction: 0xfffff920

0000ab44 <sys_sig_default>:
    ab44:	e1a0c00d 	mov	ip, sp
    ab48:	e3500000 	cmp	r0, #0
    ab4c:	e92dd800 	push	{fp, ip, lr, pc}
    ab50:	e24cb004 	sub	fp, ip, #4
    ab54:	0b0009ca 	bleq	d284 <exit>
    ab58:	e24bd00c 	sub	sp, fp, #12
    ab5c:	e89d6800 	ldm	sp, {fp, sp, lr}
    ab60:	e12fff1e 	bx	lr

0000ab64 <_do_signal>:
    ab64:	e1a0c00d 	mov	ip, sp
    ab68:	e3500001 	cmp	r0, #1
    ab6c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    ab70:	e24cb004 	sub	fp, ip, #4
    ab74:	e1a04000 	mov	r4, r0
    ab78:	9a000002 	bls	ab88 <_do_signal+0x24>
    ab7c:	e24bd014 	sub	sp, fp, #20
    ab80:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    ab84:	e12fff1e 	bx	lr
    ab88:	e59f5054 	ldr	r5, [pc, #84]	; abe4 <_do_signal+0x80>
    ab8c:	e08f5005 	add	r5, pc, r5
    ab90:	e7952180 	ldr	r2, [r5, r0, lsl #3]
    ab94:	e3520000 	cmp	r2, #0
    ab98:	0afffff7 	beq	ab7c <_do_signal+0x18>
    ab9c:	e3500001 	cmp	r0, #1
    aba0:	0a00000b 	beq	abd4 <_do_signal+0x70>
    aba4:	e59f303c 	ldr	r3, [pc, #60]	; abe8 <_do_signal+0x84>
    aba8:	e08f3003 	add	r3, pc, r3
    abac:	e0833184 	add	r3, r3, r4, lsl #3
    abb0:	e5931004 	ldr	r1, [r3, #4]
    abb4:	e1a00004 	mov	r0, r4
    abb8:	e1a0e00f 	mov	lr, pc
    abbc:	e12fff12 	bx	r2
    abc0:	e3a00011 	mov	r0, #17
    abc4:	ebfff3e3 	bl	7b58 <syscall0>
    abc8:	e24bd014 	sub	sp, fp, #20
    abcc:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    abd0:	e12fff1e 	bx	lr
    abd4:	e3a00000 	mov	r0, #0
    abd8:	eb0009a9 	bl	d284 <exit>
    abdc:	e5952008 	ldr	r2, [r5, #8]
    abe0:	eaffffef 	b	aba4 <_do_signal+0x40>
    abe4:	00042830 	andeq	r2, r4, r0, lsr r8
    abe8:	00042814 	andeq	r2, r4, r4, lsl r8

0000abec <sys_sig_ignore>:
    abec:	e12fff1e 	bx	lr

0000abf0 <sys_signal_init>:
    abf0:	e1a0c00d 	mov	ip, sp
    abf4:	e92dd800 	push	{fp, ip, lr, pc}
    abf8:	e24cb004 	sub	fp, ip, #4
    abfc:	e3a0c000 	mov	ip, #0
    ac00:	e59f103c 	ldr	r1, [pc, #60]	; ac44 <sys_signal_init+0x54>
    ac04:	e59f203c 	ldr	r2, [pc, #60]	; ac48 <sys_signal_init+0x58>
    ac08:	e08f1001 	add	r1, pc, r1
    ac0c:	e59f3038 	ldr	r3, [pc, #56]	; ac4c <sys_signal_init+0x5c>
    ac10:	e7912002 	ldr	r2, [r1, r2]
    ac14:	e59f1034 	ldr	r1, [pc, #52]	; ac50 <sys_signal_init+0x60>
    ac18:	e08f3003 	add	r3, pc, r3
    ac1c:	e08f1001 	add	r1, pc, r1
    ac20:	e5832000 	str	r2, [r3]
    ac24:	e5832008 	str	r2, [r3, #8]
    ac28:	e583c004 	str	ip, [r3, #4]
    ac2c:	e583c00c 	str	ip, [r3, #12]
    ac30:	e3a0000f 	mov	r0, #15
    ac34:	ebfff3c4 	bl	7b4c <syscall1>
    ac38:	e24bd00c 	sub	sp, fp, #12
    ac3c:	e89d6800 	ldm	sp, {fp, sp, lr}
    ac40:	e12fff1e 	bx	lr
    ac44:	0000cdf0 	strdeq	ip, [r0], -r0
    ac48:	00000044 	andeq	r0, r0, r4, asr #32
    ac4c:	000427a4 	andeq	r2, r4, r4, lsr #15
    ac50:	ffffff40 			; <UNDEFINED> instruction: 0xffffff40

0000ac54 <sys_signal>:
    ac54:	e3500001 	cmp	r0, #1
    ac58:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
    ac5c:	83a00000 	movhi	r0, #0
    ac60:	8a000006 	bhi	ac80 <sys_signal+0x2c>
    ac64:	e59fc01c 	ldr	ip, [pc, #28]	; ac88 <sys_signal+0x34>
    ac68:	e08fc00c 	add	ip, pc, ip
    ac6c:	e79c3180 	ldr	r3, [ip, r0, lsl #3]
    ac70:	e08ce180 	add	lr, ip, r0, lsl #3
    ac74:	e78c1180 	str	r1, [ip, r0, lsl #3]
    ac78:	e1a00003 	mov	r0, r3
    ac7c:	e58e2004 	str	r2, [lr, #4]
    ac80:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
    ac84:	e12fff1e 	bx	lr
    ac88:	00042754 	andeq	r2, r4, r4, asr r7

0000ac8c <sig_stop>:
    ac8c:	e3a03001 	mov	r3, #1
    ac90:	e5c13000 	strb	r3, [r1]
    ac94:	e12fff1e 	bx	lr

0000ac98 <handle>:
    ac98:	e1a0c00d 	mov	ip, sp
    ac9c:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    aca0:	e24cb004 	sub	fp, ip, #4
    aca4:	e24dd0fc 	sub	sp, sp, #252	; 0xfc
    aca8:	e59b4004 	ldr	r4, [fp, #4]
    acac:	e3540000 	cmp	r4, #0
    acb0:	e1a06000 	mov	r6, r0
    acb4:	e1a09002 	mov	r9, r2
    acb8:	e1a05003 	mov	r5, r3
    acbc:	0a00003d 	beq	adb8 <handle+0x120>
    acc0:	e2411001 	sub	r1, r1, #1
    acc4:	e5947044 	ldr	r7, [r4, #68]	; 0x44
    acc8:	e351000f 	cmp	r1, #15
    accc:	908ff101 	addls	pc, pc, r1, lsl #2
    acd0:	ea000038 	b	adb8 <handle+0x120>
    acd4:	ea000076 	b	aeb4 <handle+0x21c>
    acd8:	ea0000ad 	b	af94 <handle+0x2fc>
    acdc:	ea000096 	b	af3c <handle+0x2a4>
    ace0:	ea0000cc 	b	b018 <handle+0x380>
    ace4:	ea0000f1 	b	b0b0 <handle+0x418>
    ace8:	ea000009 	b	ad14 <handle+0x7c>
    acec:	ea00019e 	b	b36c <handle+0x6d4>
    acf0:	ea000030 	b	adb8 <handle+0x120>
    acf4:	ea0001b4 	b	b3cc <handle+0x734>
    acf8:	ea000120 	b	b180 <handle+0x4e8>
    acfc:	ea00002d 	b	adb8 <handle+0x120>
    ad00:	ea000161 	b	b28c <handle+0x5f4>
    ad04:	ea000171 	b	b2d0 <handle+0x638>
    ad08:	ea00017f 	b	b30c <handle+0x674>
    ad0c:	ea0001d4 	b	b464 <handle+0x7cc>
    ad10:	ea00002b 	b	adc4 <handle+0x12c>
    ad14:	e1a00002 	mov	r0, r2
    ad18:	eb000493 	bl	bf6c <proto_read_int>
    ad1c:	e1a08000 	mov	r8, r0
    ad20:	e1a00009 	mov	r0, r9
    ad24:	eb000490 	bl	bf6c <proto_read_int>
    ad28:	e1a0a000 	mov	sl, r0
    ad2c:	e1a00009 	mov	r0, r9
    ad30:	eb00048d 	bl	bf6c <proto_read_int>
    ad34:	e5943060 	ldr	r3, [r4, #96]	; 0x60
    ad38:	e3530000 	cmp	r3, #0
    ad3c:	e1a09000 	mov	r9, r0
    ad40:	0a0001cf 	beq	b484 <handle+0x7ec>
    ad44:	e3500000 	cmp	r0, #0
    ad48:	ba0001d4 	blt	b4a0 <handle+0x808>
    ad4c:	eb0006c6 	bl	c86c <shm_map>
    ad50:	e1a0c000 	mov	ip, r0
    ad54:	e35c0000 	cmp	ip, #0
    ad58:	0a0001c9 	beq	b484 <handle+0x7ec>
    ad5c:	e58d7000 	str	r7, [sp]
    ad60:	e1a00006 	mov	r0, r6
    ad64:	e1a02008 	mov	r2, r8
    ad68:	e1a0300a 	mov	r3, sl
    ad6c:	e1a0100c 	mov	r1, ip
    ad70:	e50bc108 	str	ip, [fp, #-264]	; 0x108
    ad74:	e594c060 	ldr	ip, [r4, #96]	; 0x60
    ad78:	e1a0e00f 	mov	lr, pc
    ad7c:	e12fff1c 	bx	ip
    ad80:	e1a04000 	mov	r4, r0
    ad84:	eb0003af 	bl	bc48 <get_proto_factor>
    ad88:	e1a01004 	mov	r1, r4
    ad8c:	e5903014 	ldr	r3, [r0, #20]
    ad90:	e1a00005 	mov	r0, r5
    ad94:	e1a0e00f 	mov	lr, pc
    ad98:	e12fff13 	bx	r3
    ad9c:	e3540000 	cmp	r4, #0
    ada0:	e51bc108 	ldr	ip, [fp, #-264]	; 0x108
    ada4:	da0001c9 	ble	b4d0 <handle+0x838>
    ada8:	e3590000 	cmp	r9, #0
    adac:	ba0001cc 	blt	b4e4 <handle+0x84c>
    adb0:	e1a00009 	mov	r0, r9
    adb4:	eb0006b5 	bl	c890 <shm_unmap>
    adb8:	e24bd028 	sub	sp, fp, #40	; 0x28
    adbc:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    adc0:	e12fff1e 	bx	lr
    adc4:	e1a00002 	mov	r0, r2
    adc8:	eb000467 	bl	bf6c <proto_read_int>
    adcc:	e24b10fc 	sub	r1, fp, #252	; 0xfc
    add0:	e50b0108 	str	r0, [fp, #-264]	; 0x108
    add4:	e1a00009 	mov	r0, r9
    add8:	eb0003d2 	bl	bd28 <proto_read>
    addc:	e1a09000 	mov	r9, r0
    ade0:	eb000398 	bl	bc48 <get_proto_factor>
    ade4:	e24ba0e4 	sub	sl, fp, #228	; 0xe4
    ade8:	e5903004 	ldr	r3, [r0, #4]
    adec:	e1a0000a 	mov	r0, sl
    adf0:	e1a0e00f 	mov	lr, pc
    adf4:	e12fff13 	bx	r3
    adf8:	eb000392 	bl	bc48 <get_proto_factor>
    adfc:	e24b8088 	sub	r8, fp, #136	; 0x88
    ae00:	e5903004 	ldr	r3, [r0, #4]
    ae04:	e1a00008 	mov	r0, r8
    ae08:	e1a0e00f 	mov	lr, pc
    ae0c:	e12fff13 	bx	r3
    ae10:	eb00038c 	bl	bc48 <get_proto_factor>
    ae14:	e3e01000 	mvn	r1, #0
    ae18:	e5903014 	ldr	r3, [r0, #20]
    ae1c:	e1a00005 	mov	r0, r5
    ae20:	e1a0e00f 	mov	lr, pc
    ae24:	e12fff13 	bx	r3
    ae28:	e3590000 	cmp	r9, #0
    ae2c:	0a000006 	beq	ae4c <handle+0x1b4>
    ae30:	eb000384 	bl	bc48 <get_proto_factor>
    ae34:	e1a01009 	mov	r1, r9
    ae38:	e5903008 	ldr	r3, [r0, #8]
    ae3c:	e51b20fc 	ldr	r2, [fp, #-252]	; 0xfc
    ae40:	e1a0000a 	mov	r0, sl
    ae44:	e1a0e00f 	mov	lr, pc
    ae48:	e12fff13 	bx	r3
    ae4c:	e594c048 	ldr	ip, [r4, #72]	; 0x48
    ae50:	e35c0000 	cmp	ip, #0
    ae54:	0a000104 	beq	b26c <handle+0x5d4>
    ae58:	e58d7000 	str	r7, [sp]
    ae5c:	e1a00006 	mov	r0, r6
    ae60:	e51b1108 	ldr	r1, [fp, #-264]	; 0x108
    ae64:	e1a03008 	mov	r3, r8
    ae68:	e1a0200a 	mov	r2, sl
    ae6c:	e1a0e00f 	mov	lr, pc
    ae70:	e12fff1c 	bx	ip
    ae74:	e2504000 	subs	r4, r0, #0
    ae78:	1a0000fb 	bne	b26c <handle+0x5d4>
    ae7c:	eb000371 	bl	bc48 <get_proto_factor>
    ae80:	e590300c 	ldr	r3, [r0, #12]
    ae84:	e1a00005 	mov	r0, r5
    ae88:	e1a0e00f 	mov	lr, pc
    ae8c:	e12fff13 	bx	r3
    ae90:	e1a01004 	mov	r1, r4
    ae94:	e5903014 	ldr	r3, [r0, #20]
    ae98:	e1a00005 	mov	r0, r5
    ae9c:	e1a0e00f 	mov	lr, pc
    aea0:	e12fff13 	bx	r3
    aea4:	e51b1084 	ldr	r1, [fp, #-132]	; 0x84
    aea8:	e5903010 	ldr	r3, [r0, #16]
    aeac:	e51b2080 	ldr	r2, [fp, #-128]	; 0x80
    aeb0:	ea0000ea 	b	b260 <handle+0x5c8>
    aeb4:	e1a00002 	mov	r0, r2
    aeb8:	eb00042b 	bl	bf6c <proto_read_int>
    aebc:	e24b8088 	sub	r8, fp, #136	; 0x88
    aec0:	e1a0a000 	mov	sl, r0
    aec4:	e1a01008 	mov	r1, r8
    aec8:	e3a0205c 	mov	r2, #92	; 0x5c
    aecc:	e1a00009 	mov	r0, r9
    aed0:	eb0003ba 	bl	bdc0 <proto_read_to>
    aed4:	e1a00009 	mov	r0, r9
    aed8:	eb000423 	bl	bf6c <proto_read_int>
    aedc:	e35a0000 	cmp	sl, #0
    aee0:	e1a03000 	mov	r3, r0
    aee4:	b3a04000 	movlt	r4, #0
    aee8:	ba00000a 	blt	af18 <handle+0x280>
    aeec:	e594404c 	ldr	r4, [r4, #76]	; 0x4c
    aef0:	e3540000 	cmp	r4, #0
    aef4:	0a000007 	beq	af18 <handle+0x280>
    aef8:	e58d7000 	str	r7, [sp]
    aefc:	e1a0000a 	mov	r0, sl
    af00:	e1a01006 	mov	r1, r6
    af04:	e1a02008 	mov	r2, r8
    af08:	e1a0e00f 	mov	lr, pc
    af0c:	e12fff14 	bx	r4
    af10:	e2904000 	adds	r4, r0, #0
    af14:	13e04000 	mvnne	r4, #0
    af18:	eb00034a 	bl	bc48 <get_proto_factor>
    af1c:	e1a01004 	mov	r1, r4
    af20:	e5903014 	ldr	r3, [r0, #20]
    af24:	e1a00005 	mov	r0, r5
    af28:	e1a0e00f 	mov	lr, pc
    af2c:	e12fff13 	bx	r3
    af30:	e24bd028 	sub	sp, fp, #40	; 0x28
    af34:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    af38:	e12fff1e 	bx	lr
    af3c:	e1a00002 	mov	r0, r2
    af40:	eb000409 	bl	bf6c <proto_read_int>
    af44:	e1a08000 	mov	r8, r0
    af48:	e1a00009 	mov	r0, r9
    af4c:	eb000406 	bl	bf6c <proto_read_int>
    af50:	e24b5088 	sub	r5, fp, #136	; 0x88
    af54:	e3500000 	cmp	r0, #0
    af58:	a1a06000 	movge	r6, r0
    af5c:	e1a01005 	mov	r1, r5
    af60:	e1a00009 	mov	r0, r9
    af64:	e3a0205c 	mov	r2, #92	; 0x5c
    af68:	eb000394 	bl	bdc0 <proto_read_to>
    af6c:	e594c054 	ldr	ip, [r4, #84]	; 0x54
    af70:	e35c0000 	cmp	ip, #0
    af74:	0affff8f 	beq	adb8 <handle+0x120>
    af78:	e1a00008 	mov	r0, r8
    af7c:	e1a01006 	mov	r1, r6
    af80:	e1a02005 	mov	r2, r5
    af84:	e1a03007 	mov	r3, r7
    af88:	e1a0e00f 	mov	lr, pc
    af8c:	e12fff1c 	bx	ip
    af90:	eaffff88 	b	adb8 <handle+0x120>
    af94:	e24b60e4 	sub	r6, fp, #228	; 0xe4
    af98:	e1a01006 	mov	r1, r6
    af9c:	e1a00002 	mov	r0, r2
    afa0:	e24b8088 	sub	r8, fp, #136	; 0x88
    afa4:	e3a0205c 	mov	r2, #92	; 0x5c
    afa8:	eb000384 	bl	bdc0 <proto_read_to>
    afac:	e1a00009 	mov	r0, r9
    afb0:	e1a01008 	mov	r1, r8
    afb4:	e3a0205c 	mov	r2, #92	; 0x5c
    afb8:	eb000380 	bl	bdc0 <proto_read_to>
    afbc:	e5943050 	ldr	r3, [r4, #80]	; 0x50
    afc0:	e3530000 	cmp	r3, #0
    afc4:	03e04000 	mvneq	r4, #0
    afc8:	0a000005 	beq	afe4 <handle+0x34c>
    afcc:	e1a00006 	mov	r0, r6
    afd0:	e1a02007 	mov	r2, r7
    afd4:	e1a01008 	mov	r1, r8
    afd8:	e1a0e00f 	mov	lr, pc
    afdc:	e12fff13 	bx	r3
    afe0:	e1a04000 	mov	r4, r0
    afe4:	eb000317 	bl	bc48 <get_proto_factor>
    afe8:	e1a01004 	mov	r1, r4
    afec:	e5903014 	ldr	r3, [r0, #20]
    aff0:	e1a00005 	mov	r0, r5
    aff4:	e1a0e00f 	mov	lr, pc
    aff8:	e12fff13 	bx	r3
    affc:	e1a01008 	mov	r1, r8
    b000:	e5903010 	ldr	r3, [r0, #16]
    b004:	e3a0205c 	mov	r2, #92	; 0x5c
    b008:	e1a00005 	mov	r0, r5
    b00c:	e1a0e00f 	mov	lr, pc
    b010:	e12fff13 	bx	r3
    b014:	eaffff67 	b	adb8 <handle+0x120>
    b018:	e1a00002 	mov	r0, r2
    b01c:	eb0003d2 	bl	bf6c <proto_read_int>
    b020:	e24b8088 	sub	r8, fp, #136	; 0x88
    b024:	e1a01008 	mov	r1, r8
    b028:	e3a0205c 	mov	r2, #92	; 0x5c
    b02c:	e50b0108 	str	r0, [fp, #-264]	; 0x108
    b030:	e1a00009 	mov	r0, r9
    b034:	eb000361 	bl	bdc0 <proto_read_to>
    b038:	e1a00009 	mov	r0, r9
    b03c:	eb0003ca 	bl	bf6c <proto_read_int>
    b040:	e1a0a000 	mov	sl, r0
    b044:	e1a00009 	mov	r0, r9
    b048:	eb0003c7 	bl	bf6c <proto_read_int>
    b04c:	e50b010c 	str	r0, [fp, #-268]	; 0x10c
    b050:	e1a00009 	mov	r0, r9
    b054:	eb0003c4 	bl	bf6c <proto_read_int>
    b058:	e5943058 	ldr	r3, [r4, #88]	; 0x58
    b05c:	e3530000 	cmp	r3, #0
    b060:	e1a09000 	mov	r9, r0
    b064:	0a000106 	beq	b484 <handle+0x7ec>
    b068:	e3500000 	cmp	r0, #0
    b06c:	ba000113 	blt	b4c0 <handle+0x828>
    b070:	eb0005fd 	bl	c86c <shm_map>
    b074:	e1a0c000 	mov	ip, r0
    b078:	e35c0000 	cmp	ip, #0
    b07c:	0a000100 	beq	b484 <handle+0x7ec>
    b080:	e51b310c 	ldr	r3, [fp, #-268]	; 0x10c
    b084:	e51b0108 	ldr	r0, [fp, #-264]	; 0x108
    b088:	e98d0088 	stmib	sp, {r3, r7}
    b08c:	e58da000 	str	sl, [sp]
    b090:	e1a01006 	mov	r1, r6
    b094:	e1a02008 	mov	r2, r8
    b098:	e1a0300c 	mov	r3, ip
    b09c:	e50bc108 	str	ip, [fp, #-264]	; 0x108
    b0a0:	e594c058 	ldr	ip, [r4, #88]	; 0x58
    b0a4:	e1a0e00f 	mov	lr, pc
    b0a8:	e12fff1c 	bx	ip
    b0ac:	eaffff33 	b	ad80 <handle+0xe8>
    b0b0:	e1a00002 	mov	r0, r2
    b0b4:	eb0003ac 	bl	bf6c <proto_read_int>
    b0b8:	e3a01000 	mov	r1, #0
    b0bc:	e50b0108 	str	r0, [fp, #-264]	; 0x108
    b0c0:	e1a00009 	mov	r0, r9
    b0c4:	eb000317 	bl	bd28 <proto_read>
    b0c8:	e24b8088 	sub	r8, fp, #136	; 0x88
    b0cc:	e1a01000 	mov	r1, r0
    b0d0:	e3a0205c 	mov	r2, #92	; 0x5c
    b0d4:	e1a00008 	mov	r0, r8
    b0d8:	eb000b86 	bl	def8 <memcpy>
    b0dc:	e1a00009 	mov	r0, r9
    b0e0:	eb0003a1 	bl	bf6c <proto_read_int>
    b0e4:	e50b010c 	str	r0, [fp, #-268]	; 0x10c
    b0e8:	e1a00009 	mov	r0, r9
    b0ec:	eb00039e 	bl	bf6c <proto_read_int>
    b0f0:	e594305c 	ldr	r3, [r4, #92]	; 0x5c
    b0f4:	e3530000 	cmp	r3, #0
    b0f8:	e1a0a000 	mov	sl, r0
    b0fc:	0a0000e0 	beq	b484 <handle+0x7ec>
    b100:	e35a0000 	cmp	sl, #0
    b104:	e1a00009 	mov	r0, r9
    b108:	ba0000e8 	blt	b4b0 <handle+0x818>
    b10c:	eb000396 	bl	bf6c <proto_read_int>
    b110:	e50b00e4 	str	r0, [fp, #-228]	; 0xe4
    b114:	e1a0000a 	mov	r0, sl
    b118:	eb0005d3 	bl	c86c <shm_map>
    b11c:	e1a03000 	mov	r3, r0
    b120:	e3530000 	cmp	r3, #0
    b124:	0a0000f8 	beq	b50c <handle+0x874>
    b128:	e51bc0e4 	ldr	ip, [fp, #-228]	; 0xe4
    b12c:	e51b210c 	ldr	r2, [fp, #-268]	; 0x10c
    b130:	e1a01006 	mov	r1, r6
    b134:	e98d0084 	stmib	sp, {r2, r7}
    b138:	e58dc000 	str	ip, [sp]
    b13c:	e1a02008 	mov	r2, r8
    b140:	e51b0108 	ldr	r0, [fp, #-264]	; 0x108
    b144:	e594c05c 	ldr	ip, [r4, #92]	; 0x5c
    b148:	e1a0e00f 	mov	lr, pc
    b14c:	e12fff1c 	bx	ip
    b150:	e50b00e4 	str	r0, [fp, #-228]	; 0xe4
    b154:	eb0002bb 	bl	bc48 <get_proto_factor>
    b158:	e51b10e4 	ldr	r1, [fp, #-228]	; 0xe4
    b15c:	e5903014 	ldr	r3, [r0, #20]
    b160:	e1a00005 	mov	r0, r5
    b164:	e1a0e00f 	mov	lr, pc
    b168:	e12fff13 	bx	r3
    b16c:	e35a0000 	cmp	sl, #0
    b170:	baffff10 	blt	adb8 <handle+0x120>
    b174:	e1a0000a 	mov	r0, sl
    b178:	eb0005c4 	bl	c890 <shm_unmap>
    b17c:	eaffff0d 	b	adb8 <handle+0x120>
    b180:	e1a00002 	mov	r0, r2
    b184:	eb000378 	bl	bf6c <proto_read_int>
    b188:	e24b8088 	sub	r8, fp, #136	; 0x88
    b18c:	e3a0205c 	mov	r2, #92	; 0x5c
    b190:	e1a01008 	mov	r1, r8
    b194:	e50b010c 	str	r0, [fp, #-268]	; 0x10c
    b198:	e1a00009 	mov	r0, r9
    b19c:	eb000307 	bl	bdc0 <proto_read_to>
    b1a0:	e1a00009 	mov	r0, r9
    b1a4:	eb000370 	bl	bf6c <proto_read_int>
    b1a8:	e50b0110 	str	r0, [fp, #-272]	; 0x110
    b1ac:	eb0002a5 	bl	bc48 <get_proto_factor>
    b1b0:	e24ba0e4 	sub	sl, fp, #228	; 0xe4
    b1b4:	e5903004 	ldr	r3, [r0, #4]
    b1b8:	e1a0000a 	mov	r0, sl
    b1bc:	e1a0e00f 	mov	lr, pc
    b1c0:	e12fff13 	bx	r3
    b1c4:	e24b1c01 	sub	r1, fp, #256	; 0x100
    b1c8:	e1a00009 	mov	r0, r9
    b1cc:	eb0002d5 	bl	bd28 <proto_read>
    b1d0:	e50b0108 	str	r0, [fp, #-264]	; 0x108
    b1d4:	eb00029b 	bl	bc48 <get_proto_factor>
    b1d8:	e24b90fc 	sub	r9, fp, #252	; 0xfc
    b1dc:	e5903000 	ldr	r3, [r0]
    b1e0:	e51b1108 	ldr	r1, [fp, #-264]	; 0x108
    b1e4:	e1a00009 	mov	r0, r9
    b1e8:	e51b2100 	ldr	r2, [fp, #-256]	; 0x100
    b1ec:	e1a0e00f 	mov	lr, pc
    b1f0:	e12fff13 	bx	r3
    b1f4:	e594c070 	ldr	ip, [r4, #112]	; 0x70
    b1f8:	e35c0000 	cmp	ip, #0
    b1fc:	03e04000 	mvneq	r4, #0
    b200:	0a000008 	beq	b228 <handle+0x590>
    b204:	e88d0600 	stm	sp, {r9, sl}
    b208:	e58d7008 	str	r7, [sp, #8]
    b20c:	e51b010c 	ldr	r0, [fp, #-268]	; 0x10c
    b210:	e1a01006 	mov	r1, r6
    b214:	e1a02008 	mov	r2, r8
    b218:	e51b3110 	ldr	r3, [fp, #-272]	; 0x110
    b21c:	e1a0e00f 	mov	lr, pc
    b220:	e12fff1c 	bx	ip
    b224:	e1a04000 	mov	r4, r0
    b228:	eb000286 	bl	bc48 <get_proto_factor>
    b22c:	e590300c 	ldr	r3, [r0, #12]
    b230:	e1a00009 	mov	r0, r9
    b234:	e1a0e00f 	mov	lr, pc
    b238:	e12fff13 	bx	r3
    b23c:	eb000281 	bl	bc48 <get_proto_factor>
    b240:	e1a01004 	mov	r1, r4
    b244:	e5903014 	ldr	r3, [r0, #20]
    b248:	e1a00005 	mov	r0, r5
    b24c:	e1a0e00f 	mov	lr, pc
    b250:	e12fff13 	bx	r3
    b254:	e51b10e0 	ldr	r1, [fp, #-224]	; 0xe0
    b258:	e5903010 	ldr	r3, [r0, #16]
    b25c:	e51b20dc 	ldr	r2, [fp, #-220]	; 0xdc
    b260:	e1a00005 	mov	r0, r5
    b264:	e1a0e00f 	mov	lr, pc
    b268:	e12fff13 	bx	r3
    b26c:	eb000275 	bl	bc48 <get_proto_factor>
    b270:	e590300c 	ldr	r3, [r0, #12]
    b274:	e1a0000a 	mov	r0, sl
    b278:	e1a0e00f 	mov	lr, pc
    b27c:	e12fff13 	bx	r3
    b280:	e24bd028 	sub	sp, fp, #40	; 0x28
    b284:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    b288:	e12fff1e 	bx	lr
    b28c:	e1a00002 	mov	r0, r2
    b290:	e24b10e4 	sub	r1, fp, #228	; 0xe4
    b294:	e3a0205c 	mov	r2, #92	; 0x5c
    b298:	eb0002c8 	bl	bdc0 <proto_read_to>
    b29c:	e1a00009 	mov	r0, r9
    b2a0:	eb000353 	bl	bff4 <proto_read_str>
    b2a4:	e594307c 	ldr	r3, [r4, #124]	; 0x7c
    b2a8:	e3530000 	cmp	r3, #0
    b2ac:	01a04003 	moveq	r4, r3
    b2b0:	0affff18 	beq	af18 <handle+0x280>
    b2b4:	e1a01000 	mov	r1, r0
    b2b8:	e1a02007 	mov	r2, r7
    b2bc:	e24b0088 	sub	r0, fp, #136	; 0x88
    b2c0:	e1a0e00f 	mov	lr, pc
    b2c4:	e12fff13 	bx	r3
    b2c8:	e1a04000 	mov	r4, r0
    b2cc:	eaffff11 	b	af18 <handle+0x280>
    b2d0:	e24b6088 	sub	r6, fp, #136	; 0x88
    b2d4:	e1a00002 	mov	r0, r2
    b2d8:	e1a01006 	mov	r1, r6
    b2dc:	e3a0205c 	mov	r2, #92	; 0x5c
    b2e0:	eb0002b6 	bl	bdc0 <proto_read_to>
    b2e4:	e5943080 	ldr	r3, [r4, #128]	; 0x80
    b2e8:	e3530000 	cmp	r3, #0
    b2ec:	03e04000 	mvneq	r4, #0
    b2f0:	0affff08 	beq	af18 <handle+0x280>
    b2f4:	e1a00006 	mov	r0, r6
    b2f8:	e1a01007 	mov	r1, r7
    b2fc:	e1a0e00f 	mov	lr, pc
    b300:	e12fff13 	bx	r3
    b304:	e1a04000 	mov	r4, r0
    b308:	eaffff02 	b	af18 <handle+0x280>
    b30c:	e1a00002 	mov	r0, r2
    b310:	eb000315 	bl	bf6c <proto_read_int>
    b314:	e3a01000 	mov	r1, #0
    b318:	e1a08000 	mov	r8, r0
    b31c:	e1a00009 	mov	r0, r9
    b320:	eb000280 	bl	bd28 <proto_read>
    b324:	e24b3088 	sub	r3, fp, #136	; 0x88
    b328:	e1a01000 	mov	r1, r0
    b32c:	e3a0205c 	mov	r2, #92	; 0x5c
    b330:	e1a00003 	mov	r0, r3
    b334:	eb000aef 	bl	def8 <memcpy>
    b338:	e594c06c 	ldr	ip, [r4, #108]	; 0x6c
    b33c:	e35c0000 	cmp	ip, #0
    b340:	e1a03000 	mov	r3, r0
    b344:	01a0400c 	moveq	r4, ip
    b348:	0afffef2 	beq	af18 <handle+0x280>
    b34c:	e1a02003 	mov	r2, r3
    b350:	e1a00008 	mov	r0, r8
    b354:	e1a01006 	mov	r1, r6
    b358:	e1a03007 	mov	r3, r7
    b35c:	e1a0e00f 	mov	lr, pc
    b360:	e12fff1c 	bx	ip
    b364:	e1a04000 	mov	r4, r0
    b368:	eafffeea 	b	af18 <handle+0x280>
    b36c:	e1a00002 	mov	r0, r2
    b370:	e24b1088 	sub	r1, fp, #136	; 0x88
    b374:	eb00026b 	bl	bd28 <proto_read>
    b378:	e1a08000 	mov	r8, r0
    b37c:	e1a00009 	mov	r0, r9
    b380:	eb0002f9 	bl	bf6c <proto_read_int>
    b384:	e594c064 	ldr	ip, [r4, #100]	; 0x64
    b388:	e35c0000 	cmp	ip, #0
    b38c:	e1a03000 	mov	r3, r0
    b390:	0a00003b 	beq	b484 <handle+0x7ec>
    b394:	e1a01008 	mov	r1, r8
    b398:	e51b2088 	ldr	r2, [fp, #-136]	; 0x88
    b39c:	e58d7000 	str	r7, [sp]
    b3a0:	e1a00006 	mov	r0, r6
    b3a4:	e1a0e00f 	mov	lr, pc
    b3a8:	e12fff1c 	bx	ip
    b3ac:	e50b0088 	str	r0, [fp, #-136]	; 0x88
    b3b0:	eb000224 	bl	bc48 <get_proto_factor>
    b3b4:	e51b1088 	ldr	r1, [fp, #-136]	; 0x88
    b3b8:	e5903014 	ldr	r3, [r0, #20]
    b3bc:	e1a00005 	mov	r0, r5
    b3c0:	e1a0e00f 	mov	lr, pc
    b3c4:	e12fff13 	bx	r3
    b3c8:	eafffe7a 	b	adb8 <handle+0x120>
    b3cc:	e1a00002 	mov	r0, r2
    b3d0:	eb0002e5 	bl	bf6c <proto_read_int>
    b3d4:	e3a01000 	mov	r1, #0
    b3d8:	e1a08000 	mov	r8, r0
    b3dc:	e1a00009 	mov	r0, r9
    b3e0:	eb000250 	bl	bd28 <proto_read>
    b3e4:	e24b3088 	sub	r3, fp, #136	; 0x88
    b3e8:	e1a01000 	mov	r1, r0
    b3ec:	e3a0205c 	mov	r2, #92	; 0x5c
    b3f0:	e1a00003 	mov	r0, r3
    b3f4:	eb000abf 	bl	def8 <memcpy>
    b3f8:	e3a02000 	mov	r2, #0
    b3fc:	e594c068 	ldr	ip, [r4, #104]	; 0x68
    b400:	e15c0002 	cmp	ip, r2
    b404:	e1a03000 	mov	r3, r0
    b408:	e50b20e4 	str	r2, [fp, #-228]	; 0xe4
    b40c:	03e04000 	mvneq	r4, #0
    b410:	0a000007 	beq	b434 <handle+0x79c>
    b414:	e1a02003 	mov	r2, r3
    b418:	e58d7000 	str	r7, [sp]
    b41c:	e1a00008 	mov	r0, r8
    b420:	e1a01006 	mov	r1, r6
    b424:	e24b30e4 	sub	r3, fp, #228	; 0xe4
    b428:	e1a0e00f 	mov	lr, pc
    b42c:	e12fff1c 	bx	ip
    b430:	e1a04000 	mov	r4, r0
    b434:	eb000203 	bl	bc48 <get_proto_factor>
    b438:	e1a01004 	mov	r1, r4
    b43c:	e5903014 	ldr	r3, [r0, #20]
    b440:	e1a00005 	mov	r0, r5
    b444:	e1a0e00f 	mov	lr, pc
    b448:	e12fff13 	bx	r3
    b44c:	e51b10e4 	ldr	r1, [fp, #-228]	; 0xe4
    b450:	e5903014 	ldr	r3, [r0, #20]
    b454:	e1a00005 	mov	r0, r5
    b458:	e1a0e00f 	mov	lr, pc
    b45c:	e12fff13 	bx	r3
    b460:	eafffe54 	b	adb8 <handle+0x120>
    b464:	e5943084 	ldr	r3, [r4, #132]	; 0x84
    b468:	e3530000 	cmp	r3, #0
    b46c:	0afffe51 	beq	adb8 <handle+0x120>
    b470:	e1a00002 	mov	r0, r2
    b474:	e1a01007 	mov	r1, r7
    b478:	e1a0e00f 	mov	lr, pc
    b47c:	e12fff13 	bx	r3
    b480:	eafffe4c 	b	adb8 <handle+0x120>
    b484:	eb0001ef 	bl	bc48 <get_proto_factor>
    b488:	e3e01000 	mvn	r1, #0
    b48c:	e5903014 	ldr	r3, [r0, #20]
    b490:	e1a00005 	mov	r0, r5
    b494:	e1a0e00f 	mov	lr, pc
    b498:	e12fff13 	bx	r3
    b49c:	eafffe45 	b	adb8 <handle+0x120>
    b4a0:	e1a00008 	mov	r0, r8
    b4a4:	eb000788 	bl	d2cc <malloc>
    b4a8:	e1a0c000 	mov	ip, r0
    b4ac:	eafffe28 	b	ad54 <handle+0xbc>
    b4b0:	e24b10e4 	sub	r1, fp, #228	; 0xe4
    b4b4:	eb00021b 	bl	bd28 <proto_read>
    b4b8:	e1a03000 	mov	r3, r0
    b4bc:	eaffff17 	b	b120 <handle+0x488>
    b4c0:	e1a0000a 	mov	r0, sl
    b4c4:	eb000780 	bl	d2cc <malloc>
    b4c8:	e1a0c000 	mov	ip, r0
    b4cc:	eafffee9 	b	b078 <handle+0x3e0>
    b4d0:	e3590000 	cmp	r9, #0
    b4d4:	aafffe35 	bge	adb0 <handle+0x118>
    b4d8:	e1a0000c 	mov	r0, ip
    b4dc:	eb000771 	bl	d2a8 <free>
    b4e0:	eafffe34 	b	adb8 <handle+0x120>
    b4e4:	eb0001d7 	bl	bc48 <get_proto_factor>
    b4e8:	e51bc108 	ldr	ip, [fp, #-264]	; 0x108
    b4ec:	e5903010 	ldr	r3, [r0, #16]
    b4f0:	e1a0100c 	mov	r1, ip
    b4f4:	e1a02004 	mov	r2, r4
    b4f8:	e1a00005 	mov	r0, r5
    b4fc:	e1a0e00f 	mov	lr, pc
    b500:	e12fff13 	bx	r3
    b504:	e51bc108 	ldr	ip, [fp, #-264]	; 0x108
    b508:	eafffff2 	b	b4d8 <handle+0x840>
    b50c:	eb0001cd 	bl	bc48 <get_proto_factor>
    b510:	e3e01000 	mvn	r1, #0
    b514:	e5903014 	ldr	r3, [r0, #20]
    b518:	e1a00005 	mov	r0, r5
    b51c:	e1a0e00f 	mov	lr, pc
    b520:	e12fff13 	bx	r3
    b524:	eaffff10 	b	b16c <handle+0x4d4>

0000b528 <device_run>:
    b528:	e1a0c00d 	mov	ip, sp
    b52c:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    b530:	e24cb004 	sub	fp, ip, #4
    b534:	e24dd0bc 	sub	sp, sp, #188	; 0xbc
    b538:	e2504000 	subs	r4, r0, #0
    b53c:	e1a07001 	mov	r7, r1
    b540:	e1a08002 	mov	r8, r2
    b544:	0a00005a 	beq	b6b4 <device_run+0x18c>
    b548:	e59f116c 	ldr	r1, [pc, #364]	; b6bc <device_run+0x194>
    b54c:	e3a00000 	mov	r0, #0
    b550:	e08f1001 	add	r1, pc, r1
    b554:	e1a02004 	mov	r2, r4
    b558:	ebfffdbd 	bl	ac54 <sys_signal>
    b55c:	e3570000 	cmp	r7, #0
    b560:	e24b50dc 	sub	r5, fp, #220	; 0xdc
    b564:	0a000021 	beq	b5f0 <device_run+0xc8>
    b568:	e1a01005 	mov	r1, r5
    b56c:	e1a00007 	mov	r0, r7
    b570:	ebfff528 	bl	8a18 <vfs_get>
    b574:	e3500000 	cmp	r0, #0
    b578:	1a000043 	bne	b68c <device_run+0x164>
    b57c:	e24b6080 	sub	r6, fp, #128	; 0x80
    b580:	e3a0205c 	mov	r2, #92	; 0x5c
    b584:	e3a01000 	mov	r1, #0
    b588:	e1a00006 	mov	r0, r6
    b58c:	eb000a95 	bl	dfe8 <memset>
    b590:	e24b10d8 	sub	r1, fp, #216	; 0xd8
    b594:	e24b007c 	sub	r0, fp, #124	; 0x7c
    b598:	eb000bb0 	bl	e460 <strcpy>
    b59c:	e1a00006 	mov	r0, r6
    b5a0:	e50b803c 	str	r8, [fp, #-60]	; 0x3c
    b5a4:	ebfff3db 	bl	8518 <vfs_new_node>
    b5a8:	e5943074 	ldr	r3, [r4, #116]	; 0x74
    b5ac:	e3530000 	cmp	r3, #0
    b5b0:	0a000005 	beq	b5cc <device_run+0xa4>
    b5b4:	e1a00006 	mov	r0, r6
    b5b8:	e5941044 	ldr	r1, [r4, #68]	; 0x44
    b5bc:	e1a0e00f 	mov	lr, pc
    b5c0:	e12fff13 	bx	r3
    b5c4:	e3500000 	cmp	r0, #0
    b5c8:	1a000035 	bne	b6a4 <device_run+0x17c>
    b5cc:	e1a00005 	mov	r0, r5
    b5d0:	e1a01006 	mov	r1, r6
    b5d4:	ebfff658 	bl	8f3c <vfs_mount>
    b5d8:	e3500000 	cmp	r0, #0
    b5dc:	1a000030 	bne	b6a4 <device_run+0x17c>
    b5e0:	e1a01006 	mov	r1, r6
    b5e4:	e1a00005 	mov	r0, r5
    b5e8:	e3a0205c 	mov	r2, #92	; 0x5c
    b5ec:	eb000a41 	bl	def8 <memcpy>
    b5f0:	e5942088 	ldr	r2, [r4, #136]	; 0x88
    b5f4:	e59f00c4 	ldr	r0, [pc, #196]	; b6c0 <device_run+0x198>
    b5f8:	e2922000 	adds	r2, r2, #0
    b5fc:	e08f0000 	add	r0, pc, r0
    b600:	13a02001 	movne	r2, #1
    b604:	e1a01004 	mov	r1, r4
    b608:	ebfffd39 	bl	aaf4 <ipc_serv_run>
    b60c:	e5d43000 	ldrb	r3, [r4]
    b610:	e3530000 	cmp	r3, #0
    b614:	1a00000a 	bne	b644 <device_run+0x11c>
    b618:	e5943088 	ldr	r3, [r4, #136]	; 0x88
    b61c:	e3530000 	cmp	r3, #0
    b620:	0a000015 	beq	b67c <device_run+0x154>
    b624:	e5940044 	ldr	r0, [r4, #68]	; 0x44
    b628:	e1a0e00f 	mov	lr, pc
    b62c:	e12fff13 	bx	r3
    b630:	e59f008c 	ldr	r0, [pc, #140]	; b6c4 <device_run+0x19c>
    b634:	eb00063d 	bl	cf30 <usleep>
    b638:	e5d43000 	ldrb	r3, [r4]
    b63c:	e3530000 	cmp	r3, #0
    b640:	0afffff4 	beq	b618 <device_run+0xf0>
    b644:	e3570000 	cmp	r7, #0
    b648:	0a000005 	beq	b664 <device_run+0x13c>
    b64c:	e5943078 	ldr	r3, [r4, #120]	; 0x78
    b650:	e3530000 	cmp	r3, #0
    b654:	15941044 	ldrne	r1, [r4, #68]	; 0x44
    b658:	11a00005 	movne	r0, r5
    b65c:	11a0e00f 	movne	lr, pc
    b660:	112fff13 	bxne	r3
    b664:	e1a00005 	mov	r0, r5
    b668:	ebfff66a 	bl	9018 <vfs_umount>
    b66c:	e3a00000 	mov	r0, #0
    b670:	e24bd020 	sub	sp, fp, #32
    b674:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    b678:	e12fff1e 	bx	lr
    b67c:	eb0005b2 	bl	cd4c <getpid>
    b680:	e1a01004 	mov	r1, r4
    b684:	eb0004d6 	bl	c9e4 <proc_block>
    b688:	eaffffdf 	b	b60c <device_run+0xe4>
    b68c:	e1a00007 	mov	r0, r7
    b690:	e1a01005 	mov	r1, r5
    b694:	e1a02008 	mov	r2, r8
    b698:	e3a03001 	mov	r3, #1
    b69c:	ebfff79e 	bl	951c <vfs_create>
    b6a0:	eaffffb5 	b	b57c <device_run+0x54>
    b6a4:	e1a00006 	mov	r0, r6
    b6a8:	ebfff5bd 	bl	8da4 <vfs_del>
    b6ac:	e3e00000 	mvn	r0, #0
    b6b0:	eaffffee 	b	b670 <device_run+0x148>
    b6b4:	e3e00000 	mvn	r0, #0
    b6b8:	eaffffec 	b	b670 <device_run+0x148>
    b6bc:	fffff734 			; <UNDEFINED> instruction: 0xfffff734
    b6c0:	fffff694 			; <UNDEFINED> instruction: 0xfffff694
    b6c4:	00000bb8 			; <UNDEFINED> instruction: 0x00000bb8

0000b6c8 <dev_cntl_by_pid>:
    b6c8:	e1a0c00d 	mov	ip, sp
    b6cc:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    b6d0:	e24cb004 	sub	fp, ip, #4
    b6d4:	e24dd03c 	sub	sp, sp, #60	; 0x3c
    b6d8:	e1a05002 	mov	r5, r2
    b6dc:	e1a07001 	mov	r7, r1
    b6e0:	e1a06003 	mov	r6, r3
    b6e4:	e1a08000 	mov	r8, r0
    b6e8:	eb000156 	bl	bc48 <get_proto_factor>
    b6ec:	e24b4054 	sub	r4, fp, #84	; 0x54
    b6f0:	e5903004 	ldr	r3, [r0, #4]
    b6f4:	e1a00004 	mov	r0, r4
    b6f8:	e1a0e00f 	mov	lr, pc
    b6fc:	e12fff13 	bx	r3
    b700:	e1a01007 	mov	r1, r7
    b704:	e5903014 	ldr	r3, [r0, #20]
    b708:	e1a00004 	mov	r0, r4
    b70c:	e1a0e00f 	mov	lr, pc
    b710:	e12fff13 	bx	r3
    b714:	e3550000 	cmp	r5, #0
    b718:	0a000006 	beq	b738 <dev_cntl_by_pid+0x70>
    b71c:	eb000149 	bl	bc48 <get_proto_factor>
    b720:	e5951004 	ldr	r1, [r5, #4]
    b724:	e5903010 	ldr	r3, [r0, #16]
    b728:	e5952008 	ldr	r2, [r5, #8]
    b72c:	e1a00004 	mov	r0, r4
    b730:	e1a0e00f 	mov	lr, pc
    b734:	e12fff13 	bx	r3
    b738:	e3560000 	cmp	r6, #0
    b73c:	0a00002c 	beq	b7f4 <dev_cntl_by_pid+0x12c>
    b740:	eb000140 	bl	bc48 <get_proto_factor>
    b744:	e24b503c 	sub	r5, fp, #60	; 0x3c
    b748:	e5903004 	ldr	r3, [r0, #4]
    b74c:	e1a00005 	mov	r0, r5
    b750:	e1a0e00f 	mov	lr, pc
    b754:	e12fff13 	bx	r3
    b758:	e1a03005 	mov	r3, r5
    b75c:	e3a01010 	mov	r1, #16
    b760:	e1a02004 	mov	r2, r4
    b764:	e1a00008 	mov	r0, r8
    b768:	ebfffb86 	bl	a588 <ipc_call>
    b76c:	e1a07000 	mov	r7, r0
    b770:	eb000134 	bl	bc48 <get_proto_factor>
    b774:	e590300c 	ldr	r3, [r0, #12]
    b778:	e1a00004 	mov	r0, r4
    b77c:	e1a0e00f 	mov	lr, pc
    b780:	e12fff13 	bx	r3
    b784:	e3570000 	cmp	r7, #0
    b788:	1a000028 	bne	b830 <dev_cntl_by_pid+0x168>
    b78c:	e1a00005 	mov	r0, r5
    b790:	eb0001f5 	bl	bf6c <proto_read_int>
    b794:	e2504000 	subs	r4, r0, #0
    b798:	13e04000 	mvnne	r4, #0
    b79c:	0a000008 	beq	b7c4 <dev_cntl_by_pid+0xfc>
    b7a0:	eb000128 	bl	bc48 <get_proto_factor>
    b7a4:	e590300c 	ldr	r3, [r0, #12]
    b7a8:	e1a00005 	mov	r0, r5
    b7ac:	e1a0e00f 	mov	lr, pc
    b7b0:	e12fff13 	bx	r3
    b7b4:	e1a00004 	mov	r0, r4
    b7b8:	e24bd020 	sub	sp, fp, #32
    b7bc:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    b7c0:	e12fff1e 	bx	lr
    b7c4:	e24b1058 	sub	r1, fp, #88	; 0x58
    b7c8:	e1a00005 	mov	r0, r5
    b7cc:	eb000155 	bl	bd28 <proto_read>
    b7d0:	e1a07000 	mov	r7, r0
    b7d4:	eb00011b 	bl	bc48 <get_proto_factor>
    b7d8:	e1a01007 	mov	r1, r7
    b7dc:	e5903008 	ldr	r3, [r0, #8]
    b7e0:	e51b2058 	ldr	r2, [fp, #-88]	; 0x58
    b7e4:	e1a00006 	mov	r0, r6
    b7e8:	e1a0e00f 	mov	lr, pc
    b7ec:	e12fff13 	bx	r3
    b7f0:	eaffffea 	b	b7a0 <dev_cntl_by_pid+0xd8>
    b7f4:	e1a03006 	mov	r3, r6
    b7f8:	e3a01010 	mov	r1, #16
    b7fc:	e1a02004 	mov	r2, r4
    b800:	e1a00008 	mov	r0, r8
    b804:	ebfffb5f 	bl	a588 <ipc_call>
    b808:	e1a05000 	mov	r5, r0
    b80c:	eb00010d 	bl	bc48 <get_proto_factor>
    b810:	e590300c 	ldr	r3, [r0, #12]
    b814:	e1a00004 	mov	r0, r4
    b818:	e1a0e00f 	mov	lr, pc
    b81c:	e12fff13 	bx	r3
    b820:	e1a00005 	mov	r0, r5
    b824:	e24bd020 	sub	sp, fp, #32
    b828:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    b82c:	e12fff1e 	bx	lr
    b830:	eb000104 	bl	bc48 <get_proto_factor>
    b834:	e590300c 	ldr	r3, [r0, #12]
    b838:	e1a00005 	mov	r0, r5
    b83c:	e1a0e00f 	mov	lr, pc
    b840:	e12fff13 	bx	r3
    b844:	e3e00000 	mvn	r0, #0
    b848:	eaffffda 	b	b7b8 <dev_cntl_by_pid+0xf0>

0000b84c <dev_get_pid>:
    b84c:	e1a0c00d 	mov	ip, sp
    b850:	e92dd800 	push	{fp, ip, lr, pc}
    b854:	e24cb004 	sub	fp, ip, #4
    b858:	e24dd060 	sub	sp, sp, #96	; 0x60
    b85c:	e24b1068 	sub	r1, fp, #104	; 0x68
    b860:	ebfff46c 	bl	8a18 <vfs_get>
    b864:	e3500000 	cmp	r0, #0
    b868:	13e00000 	mvnne	r0, #0
    b86c:	051b0014 	ldreq	r0, [fp, #-20]
    b870:	e24bd00c 	sub	sp, fp, #12
    b874:	e89d6800 	ldm	sp, {fp, sp, lr}
    b878:	e12fff1e 	bx	lr

0000b87c <dev_cntl>:
    b87c:	e1a0c00d 	mov	ip, sp
    b880:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    b884:	e24cb004 	sub	fp, ip, #4
    b888:	e1a06001 	mov	r6, r1
    b88c:	e1a05002 	mov	r5, r2
    b890:	e1a04003 	mov	r4, r3
    b894:	ebffffec 	bl	b84c <dev_get_pid>
    b898:	e3500000 	cmp	r0, #0
    b89c:	ba000005 	blt	b8b8 <dev_cntl+0x3c>
    b8a0:	e1a01006 	mov	r1, r6
    b8a4:	e1a02005 	mov	r2, r5
    b8a8:	e1a03004 	mov	r3, r4
    b8ac:	e24bd01c 	sub	sp, fp, #28
    b8b0:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    b8b4:	eaffff83 	b	b6c8 <dev_cntl_by_pid>
    b8b8:	e3e00000 	mvn	r0, #0
    b8bc:	e24bd01c 	sub	sp, fp, #28
    b8c0:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    b8c4:	e12fff1e 	bx	lr

0000b8c8 <proto_init_data>:
    b8c8:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
    b8cc:	e291e000 	adds	lr, r1, #0
    b8d0:	13a0e001 	movne	lr, #1
    b8d4:	e3e0c000 	mvn	ip, #0
    b8d8:	e3a03000 	mov	r3, #0
    b8dc:	e5c0e014 	strb	lr, [r0, #20]
    b8e0:	e5801004 	str	r1, [r0, #4]
    b8e4:	e5802008 	str	r2, [r0, #8]
    b8e8:	e580200c 	str	r2, [r0, #12]
    b8ec:	e580c000 	str	ip, [r0]
    b8f0:	e5803010 	str	r3, [r0, #16]
    b8f4:	e59f0008 	ldr	r0, [pc, #8]	; b904 <proto_init_data+0x3c>
    b8f8:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
    b8fc:	e08f0000 	add	r0, pc, r0
    b900:	e12fff1e 	bx	lr
    b904:	00041ad0 	ldrdeq	r1, [r4], -r0

0000b908 <proto_init>:
    b908:	e3a03000 	mov	r3, #0
    b90c:	e3e02000 	mvn	r2, #0
    b910:	e5803008 	str	r3, [r0, #8]
    b914:	e880000c 	stm	r0, {r2, r3}
    b918:	e580300c 	str	r3, [r0, #12]
    b91c:	e5803010 	str	r3, [r0, #16]
    b920:	e5c03014 	strb	r3, [r0, #20]
    b924:	e59f0004 	ldr	r0, [pc, #4]	; b930 <proto_init+0x28>
    b928:	e08f0000 	add	r0, pc, r0
    b92c:	e12fff1e 	bx	lr
    b930:	00041aa4 	andeq	r1, r4, r4, lsr #21

0000b934 <proto_add>:
    b934:	e1a0c00d 	mov	ip, sp
    b938:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    b93c:	e24cb004 	sub	fp, ip, #4
    b940:	e24dd008 	sub	sp, sp, #8
    b944:	e1a04000 	mov	r4, r0
    b948:	e2800008 	add	r0, r0, #8
    b94c:	e8901001 	ldm	r0, {r0, ip}
    b950:	e0823000 	add	r3, r2, r0
    b954:	e283e004 	add	lr, r3, #4
    b958:	e15e000c 	cmp	lr, ip
    b95c:	e1a05002 	mov	r5, r2
    b960:	e1a07001 	mov	r7, r1
    b964:	e50b2020 	str	r2, [fp, #-32]
    b968:	e5946004 	ldr	r6, [r4, #4]
    b96c:	9a00000e 	bls	b9ac <proto_add+0x78>
    b970:	e5d42014 	ldrb	r2, [r4, #20]
    b974:	e3520000 	cmp	r2, #0
    b978:	0a000004 	beq	b990 <proto_add+0x5c>
    b97c:	e59f0070 	ldr	r0, [pc, #112]	; b9f4 <proto_add+0xc0>
    b980:	e08f0000 	add	r0, pc, r0
    b984:	e24bd01c 	sub	sp, fp, #28
    b988:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    b98c:	e12fff1e 	bx	lr
    b990:	e2831084 	add	r1, r3, #132	; 0x84
    b994:	e1a00006 	mov	r0, r6
    b998:	e584100c 	str	r1, [r4, #12]
    b99c:	eb000653 	bl	d2f0 <realloc>
    b9a0:	e1a06000 	mov	r6, r0
    b9a4:	e5940008 	ldr	r0, [r4, #8]
    b9a8:	e5846004 	str	r6, [r4, #4]
    b9ac:	e0860000 	add	r0, r6, r0
    b9b0:	e24b1020 	sub	r1, fp, #32
    b9b4:	e3a02004 	mov	r2, #4
    b9b8:	eb00094e 	bl	def8 <memcpy>
    b9bc:	e3570000 	cmp	r7, #0
    b9c0:	13550000 	cmpne	r5, #0
    b9c4:	0a000005 	beq	b9e0 <proto_add+0xac>
    b9c8:	e5940008 	ldr	r0, [r4, #8]
    b9cc:	e2800004 	add	r0, r0, #4
    b9d0:	e1a01007 	mov	r1, r7
    b9d4:	e0860000 	add	r0, r6, r0
    b9d8:	e1a02005 	mov	r2, r5
    b9dc:	eb000945 	bl	def8 <memcpy>
    b9e0:	e5943008 	ldr	r3, [r4, #8]
    b9e4:	e0855003 	add	r5, r5, r3
    b9e8:	e2855004 	add	r5, r5, #4
    b9ec:	e5845008 	str	r5, [r4, #8]
    b9f0:	eaffffe1 	b	b97c <proto_add+0x48>
    b9f4:	00041a4c 	andeq	r1, r4, ip, asr #20

0000b9f8 <proto_copy>:
    b9f8:	e1a0c00d 	mov	ip, sp
    b9fc:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    ba00:	e5d03014 	ldrb	r3, [r0, #20]
    ba04:	e3530000 	cmp	r3, #0
    ba08:	e24cb004 	sub	fp, ip, #4
    ba0c:	e1a04000 	mov	r4, r0
    ba10:	e1a06001 	mov	r6, r1
    ba14:	e1a05002 	mov	r5, r2
    ba18:	1a000005 	bne	ba34 <proto_copy+0x3c>
    ba1c:	e590300c 	ldr	r3, [r0, #12]
    ba20:	e1530002 	cmp	r3, r2
    ba24:	e5900004 	ldr	r0, [r0, #4]
    ba28:	2a000005 	bcs	ba44 <proto_copy+0x4c>
    ba2c:	e3500000 	cmp	r0, #0
    ba30:	1b00061c 	blne	d2a8 <free>
    ba34:	e1a00005 	mov	r0, r5
    ba38:	eb000623 	bl	d2cc <malloc>
    ba3c:	e584500c 	str	r5, [r4, #12]
    ba40:	e5840004 	str	r0, [r4, #4]
    ba44:	e1a01006 	mov	r1, r6
    ba48:	e1a02005 	mov	r2, r5
    ba4c:	eb000929 	bl	def8 <memcpy>
    ba50:	e3a03000 	mov	r3, #0
    ba54:	e59f0018 	ldr	r0, [pc, #24]	; ba74 <proto_copy+0x7c>
    ba58:	e08f0000 	add	r0, pc, r0
    ba5c:	e5845008 	str	r5, [r4, #8]
    ba60:	e5843010 	str	r3, [r4, #16]
    ba64:	e5c43014 	strb	r3, [r4, #20]
    ba68:	e24bd01c 	sub	sp, fp, #28
    ba6c:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    ba70:	e12fff1e 	bx	lr
    ba74:	00041974 	andeq	r1, r4, r4, ror r9

0000ba78 <proto_clear>:
    ba78:	e1a0c00d 	mov	ip, sp
    ba7c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    ba80:	e5d03014 	ldrb	r3, [r0, #20]
    ba84:	e3530000 	cmp	r3, #0
    ba88:	e24cb004 	sub	fp, ip, #4
    ba8c:	e1a04000 	mov	r4, r0
    ba90:	1a000008 	bne	bab8 <proto_clear+0x40>
    ba94:	e5900004 	ldr	r0, [r0, #4]
    ba98:	e3500000 	cmp	r0, #0
    ba9c:	e5843008 	str	r3, [r4, #8]
    baa0:	e584300c 	str	r3, [r4, #12]
    baa4:	e5843010 	str	r3, [r4, #16]
    baa8:	e5c43014 	strb	r3, [r4, #20]
    baac:	1b0005fd 	blne	d2a8 <free>
    bab0:	e3a03000 	mov	r3, #0
    bab4:	e5843004 	str	r3, [r4, #4]
    bab8:	e59f000c 	ldr	r0, [pc, #12]	; bacc <proto_clear+0x54>
    babc:	e08f0000 	add	r0, pc, r0
    bac0:	e24bd014 	sub	sp, fp, #20
    bac4:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    bac8:	e12fff1e 	bx	lr
    bacc:	00041910 	andeq	r1, r4, r0, lsl r9

0000bad0 <proto_add_int>:
    bad0:	e1a0c00d 	mov	ip, sp
    bad4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    bad8:	e24cb004 	sub	fp, ip, #4
    badc:	e24dd010 	sub	sp, sp, #16
    bae0:	e1a04000 	mov	r4, r0
    bae4:	e3a03004 	mov	r3, #4
    bae8:	e2800008 	add	r0, r0, #8
    baec:	e8900005 	ldm	r0, {r0, r2}
    baf0:	e280c008 	add	ip, r0, #8
    baf4:	e15c0002 	cmp	ip, r2
    baf8:	e50b1020 	str	r1, [fp, #-32]
    bafc:	e5945004 	ldr	r5, [r4, #4]
    bb00:	e50b3018 	str	r3, [fp, #-24]
    bb04:	9a00000e 	bls	bb44 <proto_add_int+0x74>
    bb08:	e5d43014 	ldrb	r3, [r4, #20]
    bb0c:	e3530000 	cmp	r3, #0
    bb10:	0a000004 	beq	bb28 <proto_add_int+0x58>
    bb14:	e59f0060 	ldr	r0, [pc, #96]	; bb7c <proto_add_int+0xac>
    bb18:	e08f0000 	add	r0, pc, r0
    bb1c:	e24bd014 	sub	sp, fp, #20
    bb20:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    bb24:	e12fff1e 	bx	lr
    bb28:	e2801088 	add	r1, r0, #136	; 0x88
    bb2c:	e584100c 	str	r1, [r4, #12]
    bb30:	e1a00005 	mov	r0, r5
    bb34:	eb0005ed 	bl	d2f0 <realloc>
    bb38:	e1a05000 	mov	r5, r0
    bb3c:	e5940008 	ldr	r0, [r4, #8]
    bb40:	e5845004 	str	r5, [r4, #4]
    bb44:	e24b1018 	sub	r1, fp, #24
    bb48:	e3a02004 	mov	r2, #4
    bb4c:	e0850000 	add	r0, r5, r0
    bb50:	eb0008e8 	bl	def8 <memcpy>
    bb54:	e5940008 	ldr	r0, [r4, #8]
    bb58:	e2800004 	add	r0, r0, #4
    bb5c:	e0850000 	add	r0, r5, r0
    bb60:	e24b1020 	sub	r1, fp, #32
    bb64:	e3a02004 	mov	r2, #4
    bb68:	eb0008e2 	bl	def8 <memcpy>
    bb6c:	e5943008 	ldr	r3, [r4, #8]
    bb70:	e2833008 	add	r3, r3, #8
    bb74:	e5843008 	str	r3, [r4, #8]
    bb78:	eaffffe5 	b	bb14 <proto_add_int+0x44>
    bb7c:	000418b4 			; <UNDEFINED> instruction: 0x000418b4

0000bb80 <proto_add_str>:
    bb80:	e1a0c00d 	mov	ip, sp
    bb84:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    bb88:	e24cb004 	sub	fp, ip, #4
    bb8c:	e24dd008 	sub	sp, sp, #8
    bb90:	e1a04000 	mov	r4, r0
    bb94:	e1a00001 	mov	r0, r1
    bb98:	e1a07001 	mov	r7, r1
    bb9c:	eb000a6b 	bl	e550 <strlen>
    bba0:	e5943008 	ldr	r3, [r4, #8]
    bba4:	e2805001 	add	r5, r0, #1
    bba8:	e0851003 	add	r1, r5, r3
    bbac:	e594200c 	ldr	r2, [r4, #12]
    bbb0:	e2810004 	add	r0, r1, #4
    bbb4:	e1500002 	cmp	r0, r2
    bbb8:	e50b5020 	str	r5, [fp, #-32]
    bbbc:	e5946004 	ldr	r6, [r4, #4]
    bbc0:	9a00000e 	bls	bc00 <proto_add_str+0x80>
    bbc4:	e5d43014 	ldrb	r3, [r4, #20]
    bbc8:	e3530000 	cmp	r3, #0
    bbcc:	0a000004 	beq	bbe4 <proto_add_str+0x64>
    bbd0:	e59f006c 	ldr	r0, [pc, #108]	; bc44 <proto_add_str+0xc4>
    bbd4:	e08f0000 	add	r0, pc, r0
    bbd8:	e24bd01c 	sub	sp, fp, #28
    bbdc:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    bbe0:	e12fff1e 	bx	lr
    bbe4:	e2811084 	add	r1, r1, #132	; 0x84
    bbe8:	e1a00006 	mov	r0, r6
    bbec:	e584100c 	str	r1, [r4, #12]
    bbf0:	eb0005be 	bl	d2f0 <realloc>
    bbf4:	e1a06000 	mov	r6, r0
    bbf8:	e5943008 	ldr	r3, [r4, #8]
    bbfc:	e5840004 	str	r0, [r4, #4]
    bc00:	e0860003 	add	r0, r6, r3
    bc04:	e24b1020 	sub	r1, fp, #32
    bc08:	e3a02004 	mov	r2, #4
    bc0c:	eb0008b9 	bl	def8 <memcpy>
    bc10:	e3550000 	cmp	r5, #0
    bc14:	0a000005 	beq	bc30 <proto_add_str+0xb0>
    bc18:	e5940008 	ldr	r0, [r4, #8]
    bc1c:	e2800004 	add	r0, r0, #4
    bc20:	e1a01007 	mov	r1, r7
    bc24:	e0860000 	add	r0, r6, r0
    bc28:	e1a02005 	mov	r2, r5
    bc2c:	eb0008b1 	bl	def8 <memcpy>
    bc30:	e5943008 	ldr	r3, [r4, #8]
    bc34:	e2833004 	add	r3, r3, #4
    bc38:	e0835005 	add	r5, r3, r5
    bc3c:	e5845008 	str	r5, [r4, #8]
    bc40:	eaffffe2 	b	bbd0 <proto_add_str+0x50>
    bc44:	000417f8 	strdeq	r1, [r4], -r8

0000bc48 <get_proto_factor>:
    bc48:	e92d4030 	push	{r4, r5, lr}
    bc4c:	e59f0058 	ldr	r0, [pc, #88]	; bcac <get_proto_factor+0x64>
    bc50:	e59f5058 	ldr	r5, [pc, #88]	; bcb0 <get_proto_factor+0x68>
    bc54:	e59f4058 	ldr	r4, [pc, #88]	; bcb4 <get_proto_factor+0x6c>
    bc58:	e59fe058 	ldr	lr, [pc, #88]	; bcb8 <get_proto_factor+0x70>
    bc5c:	e59fc058 	ldr	ip, [pc, #88]	; bcbc <get_proto_factor+0x74>
    bc60:	e59f1058 	ldr	r1, [pc, #88]	; bcc0 <get_proto_factor+0x78>
    bc64:	e59f2058 	ldr	r2, [pc, #88]	; bcc4 <get_proto_factor+0x7c>
    bc68:	e59f3058 	ldr	r3, [pc, #88]	; bcc8 <get_proto_factor+0x80>
    bc6c:	e08f0000 	add	r0, pc, r0
    bc70:	e08f5005 	add	r5, pc, r5
    bc74:	e08f4004 	add	r4, pc, r4
    bc78:	e08fe00e 	add	lr, pc, lr
    bc7c:	e08fc00c 	add	ip, pc, ip
    bc80:	e08f1001 	add	r1, pc, r1
    bc84:	e08f2002 	add	r2, pc, r2
    bc88:	e08f3003 	add	r3, pc, r3
    bc8c:	e5805000 	str	r5, [r0]
    bc90:	e9804010 	stmib	r0, {r4, lr}
    bc94:	e580c00c 	str	ip, [r0, #12]
    bc98:	e5801010 	str	r1, [r0, #16]
    bc9c:	e5802014 	str	r2, [r0, #20]
    bca0:	e5803018 	str	r3, [r0, #24]
    bca4:	e8bd4030 	pop	{r4, r5, lr}
    bca8:	e12fff1e 	bx	lr
    bcac:	00041760 	andeq	r1, r4, r0, ror #14
    bcb0:	fffffc50 			; <UNDEFINED> instruction: 0xfffffc50
    bcb4:	fffffc8c 			; <UNDEFINED> instruction: 0xfffffc8c
    bcb8:	fffffd78 			; <UNDEFINED> instruction: 0xfffffd78
    bcbc:	fffffdf4 			; <UNDEFINED> instruction: 0xfffffdf4
    bcc0:	fffffcac 			; <UNDEFINED> instruction: 0xfffffcac
    bcc4:	fffffe44 			; <UNDEFINED> instruction: 0xfffffe44
    bcc8:	fffffef0 			; <UNDEFINED> instruction: 0xfffffef0

0000bccc <proto_new>:
    bccc:	e1a0c00d 	mov	ip, sp
    bcd0:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    bcd4:	e1a05000 	mov	r5, r0
    bcd8:	e24cb004 	sub	fp, ip, #4
    bcdc:	e3a00018 	mov	r0, #24
    bce0:	e1a04001 	mov	r4, r1
    bce4:	eb000578 	bl	d2cc <malloc>
    bce8:	e295c000 	adds	ip, r5, #0
    bcec:	13a0c001 	movne	ip, #1
    bcf0:	e3e01000 	mvn	r1, #0
    bcf4:	e3a02000 	mov	r2, #0
    bcf8:	e5c0c014 	strb	ip, [r0, #20]
    bcfc:	e5805004 	str	r5, [r0, #4]
    bd00:	e5804008 	str	r4, [r0, #8]
    bd04:	e580400c 	str	r4, [r0, #12]
    bd08:	e5801000 	str	r1, [r0]
    bd0c:	e5802010 	str	r2, [r0, #16]
    bd10:	e24bd014 	sub	sp, fp, #20
    bd14:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    bd18:	e12fff1e 	bx	lr

0000bd1c <proto_reset>:
    bd1c:	e3a03000 	mov	r3, #0
    bd20:	e5803010 	str	r3, [r0, #16]
    bd24:	e12fff1e 	bx	lr

0000bd28 <proto_read>:
    bd28:	e1a0c00d 	mov	ip, sp
    bd2c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    bd30:	e24cb004 	sub	fp, ip, #4
    bd34:	e24dd008 	sub	sp, sp, #8
    bd38:	e2517000 	subs	r7, r1, #0
    bd3c:	13a03000 	movne	r3, #0
    bd40:	e1a06000 	mov	r6, r0
    bd44:	e5900004 	ldr	r0, [r0, #4]
    bd48:	15873000 	strne	r3, [r7]
    bd4c:	e3500000 	cmp	r0, #0
    bd50:	0a000016 	beq	bdb0 <proto_read+0x88>
    bd54:	e5963008 	ldr	r3, [r6, #8]
    bd58:	e3530000 	cmp	r3, #0
    bd5c:	0a000013 	beq	bdb0 <proto_read+0x88>
    bd60:	e5965010 	ldr	r5, [r6, #16]
    bd64:	e1530005 	cmp	r3, r5
    bd68:	9a000010 	bls	bdb0 <proto_read+0x88>
    bd6c:	e0804005 	add	r4, r0, r5
    bd70:	e1a01004 	mov	r1, r4
    bd74:	e24b0020 	sub	r0, fp, #32
    bd78:	e3a02004 	mov	r2, #4
    bd7c:	eb00085d 	bl	def8 <memcpy>
    bd80:	e51b3020 	ldr	r3, [fp, #-32]
    bd84:	e2855004 	add	r5, r5, #4
    bd88:	e3570000 	cmp	r7, #0
    bd8c:	e0855003 	add	r5, r5, r3
    bd90:	e5865010 	str	r5, [r6, #16]
    bd94:	15873000 	strne	r3, [r7]
    bd98:	e3530000 	cmp	r3, #0
    bd9c:	0a000003 	beq	bdb0 <proto_read+0x88>
    bda0:	e2840004 	add	r0, r4, #4
    bda4:	e24bd01c 	sub	sp, fp, #28
    bda8:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    bdac:	e12fff1e 	bx	lr
    bdb0:	e3a00000 	mov	r0, #0
    bdb4:	e24bd01c 	sub	sp, fp, #28
    bdb8:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    bdbc:	e12fff1e 	bx	lr

0000bdc0 <proto_read_to>:
    bdc0:	e1a0c00d 	mov	ip, sp
    bdc4:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    bdc8:	e24cb004 	sub	fp, ip, #4
    bdcc:	e24dd00c 	sub	sp, sp, #12
    bdd0:	e1a07000 	mov	r7, r0
    bdd4:	e5900004 	ldr	r0, [r0, #4]
    bdd8:	e3500000 	cmp	r0, #0
    bddc:	e1a05001 	mov	r5, r1
    bde0:	e1a06002 	mov	r6, r2
    bde4:	0a000023 	beq	be78 <proto_read_to+0xb8>
    bde8:	e5973008 	ldr	r3, [r7, #8]
    bdec:	e3530000 	cmp	r3, #0
    bdf0:	0a000020 	beq	be78 <proto_read_to+0xb8>
    bdf4:	e5974010 	ldr	r4, [r7, #16]
    bdf8:	e1530004 	cmp	r3, r4
    bdfc:	9a00001d 	bls	be78 <proto_read_to+0xb8>
    be00:	e0808004 	add	r8, r0, r4
    be04:	e1a01008 	mov	r1, r8
    be08:	e24b0028 	sub	r0, fp, #40	; 0x28
    be0c:	e3a02004 	mov	r2, #4
    be10:	eb000838 	bl	def8 <memcpy>
    be14:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
    be18:	e2844004 	add	r4, r4, #4
    be1c:	e0844003 	add	r4, r4, r3
    be20:	e3530000 	cmp	r3, #0
    be24:	e5874010 	str	r4, [r7, #16]
    be28:	0a000012 	beq	be78 <proto_read_to+0xb8>
    be2c:	e1560003 	cmp	r6, r3
    be30:	a1a04003 	movge	r4, r3
    be34:	b1a04006 	movlt	r4, r6
    be38:	e3780004 	cmn	r8, #4
    be3c:	13550000 	cmpne	r5, #0
    be40:	03a03001 	moveq	r3, #1
    be44:	13a03000 	movne	r3, #0
    be48:	e3540000 	cmp	r4, #0
    be4c:	03833001 	orreq	r3, r3, #1
    be50:	e3530000 	cmp	r3, #0
    be54:	e2881004 	add	r1, r8, #4
    be58:	1a000006 	bne	be78 <proto_read_to+0xb8>
    be5c:	e1a00005 	mov	r0, r5
    be60:	e1a02004 	mov	r2, r4
    be64:	eb000823 	bl	def8 <memcpy>
    be68:	e1a00004 	mov	r0, r4
    be6c:	e24bd020 	sub	sp, fp, #32
    be70:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    be74:	e12fff1e 	bx	lr
    be78:	e3a00000 	mov	r0, #0
    be7c:	e24bd020 	sub	sp, fp, #32
    be80:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    be84:	e12fff1e 	bx	lr

0000be88 <proto_read_proto>:
    be88:	e1a0c00d 	mov	ip, sp
    be8c:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    be90:	e24cb004 	sub	fp, ip, #4
    be94:	e24dd00c 	sub	sp, sp, #12
    be98:	e5903004 	ldr	r3, [r0, #4]
    be9c:	e3530000 	cmp	r3, #0
    bea0:	e1a08000 	mov	r8, r0
    bea4:	e1a04001 	mov	r4, r1
    bea8:	0a00002b 	beq	bf5c <proto_read_proto+0xd4>
    beac:	e5902008 	ldr	r2, [r0, #8]
    beb0:	e3520000 	cmp	r2, #0
    beb4:	0a000028 	beq	bf5c <proto_read_proto+0xd4>
    beb8:	e5906010 	ldr	r6, [r0, #16]
    bebc:	e1520006 	cmp	r2, r6
    bec0:	9a000025 	bls	bf5c <proto_read_proto+0xd4>
    bec4:	e0835006 	add	r5, r3, r6
    bec8:	e1a01005 	mov	r1, r5
    becc:	e24b0028 	sub	r0, fp, #40	; 0x28
    bed0:	e3a02004 	mov	r2, #4
    bed4:	eb000807 	bl	def8 <memcpy>
    bed8:	e51b7028 	ldr	r7, [fp, #-40]	; 0x28
    bedc:	e2866004 	add	r6, r6, #4
    bee0:	e0866007 	add	r6, r6, r7
    bee4:	e3570000 	cmp	r7, #0
    bee8:	e5886010 	str	r6, [r8, #16]
    beec:	0a00001a 	beq	bf5c <proto_read_proto+0xd4>
    bef0:	e2955004 	adds	r5, r5, #4
    bef4:	0a000018 	beq	bf5c <proto_read_proto+0xd4>
    bef8:	e5d43014 	ldrb	r3, [r4, #20]
    befc:	e3530000 	cmp	r3, #0
    bf00:	0a00000d 	beq	bf3c <proto_read_proto+0xb4>
    bf04:	e1a00007 	mov	r0, r7
    bf08:	eb0004ef 	bl	d2cc <malloc>
    bf0c:	e584700c 	str	r7, [r4, #12]
    bf10:	e5840004 	str	r0, [r4, #4]
    bf14:	e1a01005 	mov	r1, r5
    bf18:	e1a02007 	mov	r2, r7
    bf1c:	eb0007f5 	bl	def8 <memcpy>
    bf20:	e3a00000 	mov	r0, #0
    bf24:	e5847008 	str	r7, [r4, #8]
    bf28:	e5840010 	str	r0, [r4, #16]
    bf2c:	e5c40014 	strb	r0, [r4, #20]
    bf30:	e24bd020 	sub	sp, fp, #32
    bf34:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    bf38:	e12fff1e 	bx	lr
    bf3c:	e594300c 	ldr	r3, [r4, #12]
    bf40:	e1570003 	cmp	r7, r3
    bf44:	e5940004 	ldr	r0, [r4, #4]
    bf48:	9afffff1 	bls	bf14 <proto_read_proto+0x8c>
    bf4c:	e3500000 	cmp	r0, #0
    bf50:	0affffeb 	beq	bf04 <proto_read_proto+0x7c>
    bf54:	eb0004d3 	bl	d2a8 <free>
    bf58:	eaffffe9 	b	bf04 <proto_read_proto+0x7c>
    bf5c:	e3e00000 	mvn	r0, #0
    bf60:	e24bd020 	sub	sp, fp, #32
    bf64:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    bf68:	e12fff1e 	bx	lr

0000bf6c <proto_read_int>:
    bf6c:	e1a0c00d 	mov	ip, sp
    bf70:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    bf74:	e24cb004 	sub	fp, ip, #4
    bf78:	e24dd00c 	sub	sp, sp, #12
    bf7c:	e5904004 	ldr	r4, [r0, #4]
    bf80:	e3540000 	cmp	r4, #0
    bf84:	e1a06000 	mov	r6, r0
    bf88:	0a000015 	beq	bfe4 <proto_read_int+0x78>
    bf8c:	e5903008 	ldr	r3, [r0, #8]
    bf90:	e3530000 	cmp	r3, #0
    bf94:	0a000012 	beq	bfe4 <proto_read_int+0x78>
    bf98:	e5905010 	ldr	r5, [r0, #16]
    bf9c:	e1530005 	cmp	r3, r5
    bfa0:	9a00000f 	bls	bfe4 <proto_read_int+0x78>
    bfa4:	e0844005 	add	r4, r4, r5
    bfa8:	e1a01004 	mov	r1, r4
    bfac:	e24b0020 	sub	r0, fp, #32
    bfb0:	e3a02004 	mov	r2, #4
    bfb4:	eb0007cf 	bl	def8 <memcpy>
    bfb8:	e51b3020 	ldr	r3, [fp, #-32]
    bfbc:	e2855004 	add	r5, r5, #4
    bfc0:	e0855003 	add	r5, r5, r3
    bfc4:	e3530000 	cmp	r3, #0
    bfc8:	e5865010 	str	r5, [r6, #16]
    bfcc:	0a000004 	beq	bfe4 <proto_read_int+0x78>
    bfd0:	e2940004 	adds	r0, r4, #4
    bfd4:	15940004 	ldrne	r0, [r4, #4]
    bfd8:	e24bd018 	sub	sp, fp, #24
    bfdc:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    bfe0:	e12fff1e 	bx	lr
    bfe4:	e3a00000 	mov	r0, #0
    bfe8:	e24bd018 	sub	sp, fp, #24
    bfec:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    bff0:	e12fff1e 	bx	lr

0000bff4 <proto_read_str>:
    bff4:	e1a0c00d 	mov	ip, sp
    bff8:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    bffc:	e24cb004 	sub	fp, ip, #4
    c000:	e24dd00c 	sub	sp, sp, #12
    c004:	e1a06000 	mov	r6, r0
    c008:	e5900004 	ldr	r0, [r0, #4]
    c00c:	e3500000 	cmp	r0, #0
    c010:	0a000014 	beq	c068 <proto_read_str+0x74>
    c014:	e5963008 	ldr	r3, [r6, #8]
    c018:	e3530000 	cmp	r3, #0
    c01c:	0a000011 	beq	c068 <proto_read_str+0x74>
    c020:	e5965010 	ldr	r5, [r6, #16]
    c024:	e1530005 	cmp	r3, r5
    c028:	9a00000e 	bls	c068 <proto_read_str+0x74>
    c02c:	e0804005 	add	r4, r0, r5
    c030:	e1a01004 	mov	r1, r4
    c034:	e24b0020 	sub	r0, fp, #32
    c038:	e3a02004 	mov	r2, #4
    c03c:	eb0007ad 	bl	def8 <memcpy>
    c040:	e51b3020 	ldr	r3, [fp, #-32]
    c044:	e2855004 	add	r5, r5, #4
    c048:	e0855003 	add	r5, r5, r3
    c04c:	e3530000 	cmp	r3, #0
    c050:	e5865010 	str	r5, [r6, #16]
    c054:	0a000003 	beq	c068 <proto_read_str+0x74>
    c058:	e2840004 	add	r0, r4, #4
    c05c:	e24bd018 	sub	sp, fp, #24
    c060:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    c064:	e12fff1e 	bx	lr
    c068:	e3a00000 	mov	r0, #0
    c06c:	e24bd018 	sub	sp, fp, #24
    c070:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    c074:	e12fff1e 	bx	lr

0000c078 <proto_free>:
    c078:	e1a0c00d 	mov	ip, sp
    c07c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    c080:	e5d03014 	ldrb	r3, [r0, #20]
    c084:	e3530000 	cmp	r3, #0
    c088:	e24cb004 	sub	fp, ip, #4
    c08c:	e1a04000 	mov	r4, r0
    c090:	0a000004 	beq	c0a8 <proto_free+0x30>
    c094:	e1a00004 	mov	r0, r4
    c098:	eb000482 	bl	d2a8 <free>
    c09c:	e24bd014 	sub	sp, fp, #20
    c0a0:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    c0a4:	e12fff1e 	bx	lr
    c0a8:	e5900004 	ldr	r0, [r0, #4]
    c0ac:	e3500000 	cmp	r0, #0
    c0b0:	e5843008 	str	r3, [r4, #8]
    c0b4:	e584300c 	str	r3, [r4, #12]
    c0b8:	e5843010 	str	r3, [r4, #16]
    c0bc:	e5c43014 	strb	r3, [r4, #20]
    c0c0:	0afffff3 	beq	c094 <proto_free+0x1c>
    c0c4:	eb000477 	bl	d2a8 <free>
    c0c8:	eafffff1 	b	c094 <proto_free+0x1c>

0000c0cc <div_u32>:
    c0cc:	e3510000 	cmp	r1, #0
    c0d0:	0a00001e 	beq	c150 <div_u32+0x84>
    c0d4:	e3510902 	cmp	r1, #32768	; 0x8000
    c0d8:	0a000087 	beq	c2fc <div_u32+0x230>
    c0dc:	9a00000d 	bls	c118 <div_u32+0x4c>
    c0e0:	e3510502 	cmp	r1, #8388608	; 0x800000
    c0e4:	0a000080 	beq	c2ec <div_u32+0x220>
    c0e8:	8a00001a 	bhi	c158 <div_u32+0x8c>
    c0ec:	e3510702 	cmp	r1, #524288	; 0x80000
    c0f0:	0a00006d 	beq	c2ac <div_u32+0x1e0>
    c0f4:	8a000035 	bhi	c1d0 <div_u32+0x104>
    c0f8:	e3510802 	cmp	r1, #131072	; 0x20000
    c0fc:	0a000052 	beq	c24c <div_u32+0x180>
    c100:	e3510701 	cmp	r1, #262144	; 0x40000
    c104:	0a00006e 	beq	c2c4 <div_u32+0x1f8>
    c108:	e3510801 	cmp	r1, #65536	; 0x10000
    c10c:	1a00003d 	bne	c208 <div_u32+0x13c>
    c110:	e1a00820 	lsr	r0, r0, #16
    c114:	e12fff1e 	bx	lr
    c118:	e3510080 	cmp	r1, #128	; 0x80
    c11c:	0a000074 	beq	c2f4 <div_u32+0x228>
    c120:	9a000017 	bls	c184 <div_u32+0xb8>
    c124:	e3510b02 	cmp	r1, #2048	; 0x800
    c128:	0a00006d 	beq	c2e4 <div_u32+0x218>
    c12c:	8a00001f 	bhi	c1b0 <div_u32+0xe4>
    c130:	e3510c02 	cmp	r1, #512	; 0x200
    c134:	0a000046 	beq	c254 <div_u32+0x188>
    c138:	e3510b01 	cmp	r1, #1024	; 0x400
    c13c:	0a000058 	beq	c2a4 <div_u32+0x1d8>
    c140:	e3510c01 	cmp	r1, #256	; 0x100
    c144:	1a00002f 	bne	c208 <div_u32+0x13c>
    c148:	e1a00420 	lsr	r0, r0, #8
    c14c:	e12fff1e 	bx	lr
    c150:	e1a00001 	mov	r0, r1
    c154:	e12fff1e 	bx	lr
    c158:	e3510302 	cmp	r1, #134217728	; 0x8000000
    c15c:	0a000054 	beq	c2b4 <div_u32+0x1e8>
    c160:	8a000022 	bhi	c1f0 <div_u32+0x124>
    c164:	e3510402 	cmp	r1, #33554432	; 0x2000000
    c168:	0a00003d 	beq	c264 <div_u32+0x198>
    c16c:	e3510301 	cmp	r1, #67108864	; 0x4000000
    c170:	0a000051 	beq	c2bc <div_u32+0x1f0>
    c174:	e3510401 	cmp	r1, #16777216	; 0x1000000
    c178:	1a000022 	bne	c208 <div_u32+0x13c>
    c17c:	e1a00c20 	lsr	r0, r0, #24
    c180:	e12fff1e 	bx	lr
    c184:	e3510008 	cmp	r1, #8
    c188:	0a00003d 	beq	c284 <div_u32+0x1b8>
    c18c:	9a000036 	bls	c26c <div_u32+0x1a0>
    c190:	e3510020 	cmp	r1, #32
    c194:	0a000030 	beq	c25c <div_u32+0x190>
    c198:	e3510040 	cmp	r1, #64	; 0x40
    c19c:	0a00003c 	beq	c294 <div_u32+0x1c8>
    c1a0:	e3510010 	cmp	r1, #16
    c1a4:	1a000017 	bne	c208 <div_u32+0x13c>
    c1a8:	e1a00220 	lsr	r0, r0, #4
    c1ac:	e12fff1e 	bx	lr
    c1b0:	e3510a02 	cmp	r1, #8192	; 0x2000
    c1b4:	0a000020 	beq	c23c <div_u32+0x170>
    c1b8:	e3510901 	cmp	r1, #16384	; 0x4000
    c1bc:	0a000036 	beq	c29c <div_u32+0x1d0>
    c1c0:	e3510a01 	cmp	r1, #4096	; 0x1000
    c1c4:	1a00000f 	bne	c208 <div_u32+0x13c>
    c1c8:	e1a00620 	lsr	r0, r0, #12
    c1cc:	e12fff1e 	bx	lr
    c1d0:	e3510602 	cmp	r1, #2097152	; 0x200000
    c1d4:	0a00001a 	beq	c244 <div_u32+0x178>
    c1d8:	e3510501 	cmp	r1, #4194304	; 0x400000
    c1dc:	0a00003a 	beq	c2cc <div_u32+0x200>
    c1e0:	e3510601 	cmp	r1, #1048576	; 0x100000
    c1e4:	1a000007 	bne	c208 <div_u32+0x13c>
    c1e8:	e1a00a20 	lsr	r0, r0, #20
    c1ec:	e12fff1e 	bx	lr
    c1f0:	e3510202 	cmp	r1, #536870912	; 0x20000000
    c1f4:	0a00000e 	beq	c234 <div_u32+0x168>
    c1f8:	e3510101 	cmp	r1, #1073741824	; 0x40000000
    c1fc:	0a000036 	beq	c2dc <div_u32+0x210>
    c200:	e3510201 	cmp	r1, #268435456	; 0x10000000
    c204:	0a000032 	beq	c2d4 <div_u32+0x208>
    c208:	e1510000 	cmp	r1, r0
    c20c:	83a02000 	movhi	r2, #0
    c210:	8a000005 	bhi	c22c <div_u32+0x160>
    c214:	e1a03001 	mov	r3, r1
    c218:	e3a02000 	mov	r2, #0
    c21c:	e0833001 	add	r3, r3, r1
    c220:	e1500003 	cmp	r0, r3
    c224:	e2822001 	add	r2, r2, #1
    c228:	2afffffb 	bcs	c21c <div_u32+0x150>
    c22c:	e1a00002 	mov	r0, r2
    c230:	e12fff1e 	bx	lr
    c234:	e1a00ea0 	lsr	r0, r0, #29
    c238:	e12fff1e 	bx	lr
    c23c:	e1a006a0 	lsr	r0, r0, #13
    c240:	e12fff1e 	bx	lr
    c244:	e1a00aa0 	lsr	r0, r0, #21
    c248:	e12fff1e 	bx	lr
    c24c:	e1a008a0 	lsr	r0, r0, #17
    c250:	e12fff1e 	bx	lr
    c254:	e1a004a0 	lsr	r0, r0, #9
    c258:	e12fff1e 	bx	lr
    c25c:	e1a002a0 	lsr	r0, r0, #5
    c260:	e12fff1e 	bx	lr
    c264:	e1a00ca0 	lsr	r0, r0, #25
    c268:	e12fff1e 	bx	lr
    c26c:	e3510002 	cmp	r1, #2
    c270:	0a000005 	beq	c28c <div_u32+0x1c0>
    c274:	e3510004 	cmp	r1, #4
    c278:	1affffe2 	bne	c208 <div_u32+0x13c>
    c27c:	e1a00120 	lsr	r0, r0, #2
    c280:	e12fff1e 	bx	lr
    c284:	e1a001a0 	lsr	r0, r0, #3
    c288:	e12fff1e 	bx	lr
    c28c:	e1a000a0 	lsr	r0, r0, #1
    c290:	e12fff1e 	bx	lr
    c294:	e1a00320 	lsr	r0, r0, #6
    c298:	e12fff1e 	bx	lr
    c29c:	e1a00720 	lsr	r0, r0, #14
    c2a0:	e12fff1e 	bx	lr
    c2a4:	e1a00520 	lsr	r0, r0, #10
    c2a8:	e12fff1e 	bx	lr
    c2ac:	e1a009a0 	lsr	r0, r0, #19
    c2b0:	e12fff1e 	bx	lr
    c2b4:	e1a00da0 	lsr	r0, r0, #27
    c2b8:	e12fff1e 	bx	lr
    c2bc:	e1a00d20 	lsr	r0, r0, #26
    c2c0:	e12fff1e 	bx	lr
    c2c4:	e1a00920 	lsr	r0, r0, #18
    c2c8:	e12fff1e 	bx	lr
    c2cc:	e1a00b20 	lsr	r0, r0, #22
    c2d0:	e12fff1e 	bx	lr
    c2d4:	e1a00e20 	lsr	r0, r0, #28
    c2d8:	e12fff1e 	bx	lr
    c2dc:	e1a00f20 	lsr	r0, r0, #30
    c2e0:	e12fff1e 	bx	lr
    c2e4:	e1a005a0 	lsr	r0, r0, #11
    c2e8:	e12fff1e 	bx	lr
    c2ec:	e1a00ba0 	lsr	r0, r0, #23
    c2f0:	e12fff1e 	bx	lr
    c2f4:	e1a003a0 	lsr	r0, r0, #7
    c2f8:	e12fff1e 	bx	lr
    c2fc:	e1a007a0 	lsr	r0, r0, #15
    c300:	e12fff1e 	bx	lr

0000c304 <mod_u32>:
    c304:	e3510000 	cmp	r1, #0
    c308:	0a000012 	beq	c358 <mod_u32+0x54>
    c30c:	e3510902 	cmp	r1, #32768	; 0x8000
    c310:	0a000086 	beq	c530 <mod_u32+0x22c>
    c314:	9a000011 	bls	c360 <mod_u32+0x5c>
    c318:	e3510502 	cmp	r1, #8388608	; 0x800000
    c31c:	0a00007e 	beq	c51c <mod_u32+0x218>
    c320:	8a00001e 	bhi	c3a0 <mod_u32+0x9c>
    c324:	e3510702 	cmp	r1, #524288	; 0x80000
    c328:	0a000075 	beq	c504 <mod_u32+0x200>
    c32c:	8a000042 	bhi	c43c <mod_u32+0x138>
    c330:	e3510802 	cmp	r1, #131072	; 0x20000
    c334:	0a000065 	beq	c4d0 <mod_u32+0x1cc>
    c338:	e3510701 	cmp	r1, #262144	; 0x40000
    c33c:	01a01920 	lsreq	r1, r0, #18
    c340:	01a01901 	lsleq	r1, r1, #18
    c344:	0a000003 	beq	c358 <mod_u32+0x54>
    c348:	e3510801 	cmp	r1, #65536	; 0x10000
    c34c:	1a00004e 	bne	c48c <mod_u32+0x188>
    c350:	e1a01820 	lsr	r1, r0, #16
    c354:	e1a01801 	lsl	r1, r1, #16
    c358:	e0610000 	rsb	r0, r1, r0
    c35c:	e12fff1e 	bx	lr
    c360:	e3510080 	cmp	r1, #128	; 0x80
    c364:	0a00006f 	beq	c528 <mod_u32+0x224>
    c368:	9a00001a 	bls	c3d8 <mod_u32+0xd4>
    c36c:	e3510b02 	cmp	r1, #2048	; 0x800
    c370:	0a000066 	beq	c510 <mod_u32+0x20c>
    c374:	8a000025 	bhi	c410 <mod_u32+0x10c>
    c378:	e3510c02 	cmp	r1, #512	; 0x200
    c37c:	0a000056 	beq	c4dc <mod_u32+0x1d8>
    c380:	e3510b01 	cmp	r1, #1024	; 0x400
    c384:	01a01520 	lsreq	r1, r0, #10
    c388:	01a01501 	lsleq	r1, r1, #10
    c38c:	0afffff1 	beq	c358 <mod_u32+0x54>
    c390:	e3510c01 	cmp	r1, #256	; 0x100
    c394:	1a00003c 	bne	c48c <mod_u32+0x188>
    c398:	e3c010ff 	bic	r1, r0, #255	; 0xff
    c39c:	eaffffed 	b	c358 <mod_u32+0x54>
    c3a0:	e3510302 	cmp	r1, #134217728	; 0x8000000
    c3a4:	0200133e 	andeq	r1, r0, #-134217728	; 0xf8000000
    c3a8:	0affffea 	beq	c358 <mod_u32+0x54>
    c3ac:	8a00002d 	bhi	c468 <mod_u32+0x164>
    c3b0:	e3510402 	cmp	r1, #33554432	; 0x2000000
    c3b4:	020014fe 	andeq	r1, r0, #-33554432	; 0xfe000000
    c3b8:	0affffe6 	beq	c358 <mod_u32+0x54>
    c3bc:	e3510301 	cmp	r1, #67108864	; 0x4000000
    c3c0:	0200133f 	andeq	r1, r0, #-67108864	; 0xfc000000
    c3c4:	0affffe3 	beq	c358 <mod_u32+0x54>
    c3c8:	e3510401 	cmp	r1, #16777216	; 0x1000000
    c3cc:	1a00002e 	bne	c48c <mod_u32+0x188>
    c3d0:	e20014ff 	and	r1, r0, #-16777216	; 0xff000000
    c3d4:	eaffffdf 	b	c358 <mod_u32+0x54>
    c3d8:	e3510008 	cmp	r1, #8
    c3dc:	03c01007 	biceq	r1, r0, #7
    c3e0:	0affffdc 	beq	c358 <mod_u32+0x54>
    c3e4:	9a00003f 	bls	c4e8 <mod_u32+0x1e4>
    c3e8:	e3510020 	cmp	r1, #32
    c3ec:	03c0101f 	biceq	r1, r0, #31
    c3f0:	0affffd8 	beq	c358 <mod_u32+0x54>
    c3f4:	e3510040 	cmp	r1, #64	; 0x40
    c3f8:	03c0103f 	biceq	r1, r0, #63	; 0x3f
    c3fc:	0affffd5 	beq	c358 <mod_u32+0x54>
    c400:	e3510010 	cmp	r1, #16
    c404:	1a000020 	bne	c48c <mod_u32+0x188>
    c408:	e3c0100f 	bic	r1, r0, #15
    c40c:	eaffffd1 	b	c358 <mod_u32+0x54>
    c410:	e3510a02 	cmp	r1, #8192	; 0x2000
    c414:	0a000027 	beq	c4b8 <mod_u32+0x1b4>
    c418:	e3510901 	cmp	r1, #16384	; 0x4000
    c41c:	01a01720 	lsreq	r1, r0, #14
    c420:	01a01701 	lsleq	r1, r1, #14
    c424:	0affffcb 	beq	c358 <mod_u32+0x54>
    c428:	e3510a01 	cmp	r1, #4096	; 0x1000
    c42c:	1a000016 	bne	c48c <mod_u32+0x188>
    c430:	e1a01620 	lsr	r1, r0, #12
    c434:	e1a01601 	lsl	r1, r1, #12
    c438:	eaffffc6 	b	c358 <mod_u32+0x54>
    c43c:	e3510602 	cmp	r1, #2097152	; 0x200000
    c440:	0a00001f 	beq	c4c4 <mod_u32+0x1c0>
    c444:	e3510501 	cmp	r1, #4194304	; 0x400000
    c448:	01a01b20 	lsreq	r1, r0, #22
    c44c:	01a01b01 	lsleq	r1, r1, #22
    c450:	0affffc0 	beq	c358 <mod_u32+0x54>
    c454:	e3510601 	cmp	r1, #1048576	; 0x100000
    c458:	1a00000b 	bne	c48c <mod_u32+0x188>
    c45c:	e1a01a20 	lsr	r1, r0, #20
    c460:	e1a01a01 	lsl	r1, r1, #20
    c464:	eaffffbb 	b	c358 <mod_u32+0x54>
    c468:	e3510202 	cmp	r1, #536870912	; 0x20000000
    c46c:	0200120e 	andeq	r1, r0, #-536870912	; 0xe0000000
    c470:	0affffb8 	beq	c358 <mod_u32+0x54>
    c474:	e3510101 	cmp	r1, #1073741824	; 0x40000000
    c478:	02001103 	andeq	r1, r0, #-1073741824	; 0xc0000000
    c47c:	0affffb5 	beq	c358 <mod_u32+0x54>
    c480:	e3510201 	cmp	r1, #268435456	; 0x10000000
    c484:	0200120f 	andeq	r1, r0, #-268435456	; 0xf0000000
    c488:	0affffb2 	beq	c358 <mod_u32+0x54>
    c48c:	e1500001 	cmp	r0, r1
    c490:	33a01000 	movcc	r1, #0
    c494:	3affffaf 	bcc	c358 <mod_u32+0x54>
    c498:	e1a03001 	mov	r3, r1
    c49c:	e3a02000 	mov	r2, #0
    c4a0:	e0833001 	add	r3, r3, r1
    c4a4:	e1500003 	cmp	r0, r3
    c4a8:	e2822001 	add	r2, r2, #1
    c4ac:	2afffffb 	bcs	c4a0 <mod_u32+0x19c>
    c4b0:	e0010192 	mul	r1, r2, r1
    c4b4:	eaffffa7 	b	c358 <mod_u32+0x54>
    c4b8:	e1a016a0 	lsr	r1, r0, #13
    c4bc:	e1a01681 	lsl	r1, r1, #13
    c4c0:	eaffffa4 	b	c358 <mod_u32+0x54>
    c4c4:	e1a01aa0 	lsr	r1, r0, #21
    c4c8:	e1a01a81 	lsl	r1, r1, #21
    c4cc:	eaffffa1 	b	c358 <mod_u32+0x54>
    c4d0:	e1a018a0 	lsr	r1, r0, #17
    c4d4:	e1a01881 	lsl	r1, r1, #17
    c4d8:	eaffff9e 	b	c358 <mod_u32+0x54>
    c4dc:	e1a014a0 	lsr	r1, r0, #9
    c4e0:	e1a01481 	lsl	r1, r1, #9
    c4e4:	eaffff9b 	b	c358 <mod_u32+0x54>
    c4e8:	e3510002 	cmp	r1, #2
    c4ec:	03c01001 	biceq	r1, r0, #1
    c4f0:	0affff98 	beq	c358 <mod_u32+0x54>
    c4f4:	e3510004 	cmp	r1, #4
    c4f8:	03c01003 	biceq	r1, r0, #3
    c4fc:	0affff95 	beq	c358 <mod_u32+0x54>
    c500:	eaffffe1 	b	c48c <mod_u32+0x188>
    c504:	e1a019a0 	lsr	r1, r0, #19
    c508:	e1a01981 	lsl	r1, r1, #19
    c50c:	eaffff91 	b	c358 <mod_u32+0x54>
    c510:	e1a015a0 	lsr	r1, r0, #11
    c514:	e1a01581 	lsl	r1, r1, #11
    c518:	eaffff8e 	b	c358 <mod_u32+0x54>
    c51c:	e1a01ba0 	lsr	r1, r0, #23
    c520:	e1a01b81 	lsl	r1, r1, #23
    c524:	eaffff8b 	b	c358 <mod_u32+0x54>
    c528:	e3c0107f 	bic	r1, r0, #127	; 0x7f
    c52c:	eaffff89 	b	c358 <mod_u32+0x54>
    c530:	e1a017a0 	lsr	r1, r0, #15
    c534:	e1a01781 	lsl	r1, r1, #15
    c538:	eaffff86 	b	c358 <mod_u32+0x54>

0000c53c <abs32>:
    c53c:	e3500000 	cmp	r0, #0
    c540:	b2600000 	rsblt	r0, r0, #0
    c544:	e12fff1e 	bx	lr

0000c548 <random_u32>:
    c548:	e1a0c00d 	mov	ip, sp
    c54c:	e3a0002e 	mov	r0, #46	; 0x2e
    c550:	e92dd800 	push	{fp, ip, lr, pc}
    c554:	e24cb004 	sub	fp, ip, #4
    c558:	ebffed7e 	bl	7b58 <syscall0>
    c55c:	e59f1034 	ldr	r1, [pc, #52]	; c598 <random_u32+0x50>
    c560:	e08f1001 	add	r1, pc, r1
    c564:	e5912000 	ldr	r2, [r1]
    c568:	e0000092 	mul	r0, r2, r0
    c56c:	e0823182 	add	r3, r2, r2, lsl #3
    c570:	e0633503 	rsb	r3, r3, r3, lsl #10
    c574:	e0623083 	rsb	r3, r2, r3, lsl #1
    c578:	e0633283 	rsb	r3, r3, r3, lsl #5
    c57c:	e0823083 	add	r3, r2, r3, lsl #1
    c580:	e0633183 	rsb	r3, r3, r3, lsl #3
    c584:	e28330e1 	add	r3, r3, #225	; 0xe1
    c588:	e5813000 	str	r3, [r1]
    c58c:	e24bd00c 	sub	sp, fp, #12
    c590:	e89d6800 	ldm	sp, {fp, sp, lr}
    c594:	e12fff1e 	bx	lr
    c598:	0000b4f4 	strdeq	fp, [r0], -r4

0000c59c <random_to>:
    c59c:	e1a0c00d 	mov	ip, sp
    c5a0:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    c5a4:	e24cb004 	sub	fp, ip, #4
    c5a8:	e1a04000 	mov	r4, r0
    c5ac:	e3a0002e 	mov	r0, #46	; 0x2e
    c5b0:	ebffed68 	bl	7b58 <syscall0>
    c5b4:	e59f1280 	ldr	r1, [pc, #640]	; c83c <random_to+0x2a0>
    c5b8:	e08f1001 	add	r1, pc, r1
    c5bc:	e5912000 	ldr	r2, [r1]
    c5c0:	e0823182 	add	r3, r2, r2, lsl #3
    c5c4:	e0633503 	rsb	r3, r3, r3, lsl #10
    c5c8:	e0623083 	rsb	r3, r2, r3, lsl #1
    c5cc:	e0633283 	rsb	r3, r3, r3, lsl #5
    c5d0:	e0823083 	add	r3, r2, r3, lsl #1
    c5d4:	e0633183 	rsb	r3, r3, r3, lsl #3
    c5d8:	e28330e1 	add	r3, r3, #225	; 0xe1
    c5dc:	e0000092 	mul	r0, r2, r0
    c5e0:	e5813000 	str	r3, [r1]
    c5e4:	ea000000 	b	c5ec <random_to+0x50>
    c5e8:	e1a00003 	mov	r0, r3
    c5ec:	e1a030a0 	lsr	r3, r0, #1
    c5f0:	e1530004 	cmp	r3, r4
    c5f4:	8afffffb 	bhi	c5e8 <random_to+0x4c>
    c5f8:	e3540000 	cmp	r4, #0
    c5fc:	0a000012 	beq	c64c <random_to+0xb0>
    c600:	e3540902 	cmp	r4, #32768	; 0x8000
    c604:	0a000089 	beq	c830 <random_to+0x294>
    c608:	9a000013 	bls	c65c <random_to+0xc0>
    c60c:	e3540502 	cmp	r4, #8388608	; 0x800000
    c610:	0a000081 	beq	c81c <random_to+0x280>
    c614:	8a000020 	bhi	c69c <random_to+0x100>
    c618:	e3540702 	cmp	r4, #524288	; 0x80000
    c61c:	0a000078 	beq	c804 <random_to+0x268>
    c620:	8a000044 	bhi	c738 <random_to+0x19c>
    c624:	e3540802 	cmp	r4, #131072	; 0x20000
    c628:	0a000069 	beq	c7d4 <random_to+0x238>
    c62c:	e3540701 	cmp	r4, #262144	; 0x40000
    c630:	01a04920 	lsreq	r4, r0, #18
    c634:	01a04904 	lsleq	r4, r4, #18
    c638:	0a000003 	beq	c64c <random_to+0xb0>
    c63c:	e3540801 	cmp	r4, #65536	; 0x10000
    c640:	1a00004f 	bne	c784 <random_to+0x1e8>
    c644:	e1a04820 	lsr	r4, r0, #16
    c648:	e1a04804 	lsl	r4, r4, #16
    c64c:	e0640000 	rsb	r0, r4, r0
    c650:	e24bd014 	sub	sp, fp, #20
    c654:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    c658:	e12fff1e 	bx	lr
    c65c:	e3540080 	cmp	r4, #128	; 0x80
    c660:	0a000070 	beq	c828 <random_to+0x28c>
    c664:	9a00001a 	bls	c6d4 <random_to+0x138>
    c668:	e3540b02 	cmp	r4, #2048	; 0x800
    c66c:	0a000067 	beq	c810 <random_to+0x274>
    c670:	8a000025 	bhi	c70c <random_to+0x170>
    c674:	e3540c02 	cmp	r4, #512	; 0x200
    c678:	0a000052 	beq	c7c8 <random_to+0x22c>
    c67c:	e3540b01 	cmp	r4, #1024	; 0x400
    c680:	01a04520 	lsreq	r4, r0, #10
    c684:	01a04504 	lsleq	r4, r4, #10
    c688:	0affffef 	beq	c64c <random_to+0xb0>
    c68c:	e3540c01 	cmp	r4, #256	; 0x100
    c690:	1a00003b 	bne	c784 <random_to+0x1e8>
    c694:	e3c040ff 	bic	r4, r0, #255	; 0xff
    c698:	eaffffeb 	b	c64c <random_to+0xb0>
    c69c:	e3540302 	cmp	r4, #134217728	; 0x8000000
    c6a0:	0200433e 	andeq	r4, r0, #-134217728	; 0xf8000000
    c6a4:	0affffe8 	beq	c64c <random_to+0xb0>
    c6a8:	8a00002d 	bhi	c764 <random_to+0x1c8>
    c6ac:	e3540402 	cmp	r4, #33554432	; 0x2000000
    c6b0:	020044fe 	andeq	r4, r0, #-33554432	; 0xfe000000
    c6b4:	0affffe4 	beq	c64c <random_to+0xb0>
    c6b8:	e3540301 	cmp	r4, #67108864	; 0x4000000
    c6bc:	0200433f 	andeq	r4, r0, #-67108864	; 0xfc000000
    c6c0:	0affffe1 	beq	c64c <random_to+0xb0>
    c6c4:	e3540401 	cmp	r4, #16777216	; 0x1000000
    c6c8:	1a00002d 	bne	c784 <random_to+0x1e8>
    c6cc:	e20044ff 	and	r4, r0, #-16777216	; 0xff000000
    c6d0:	eaffffdd 	b	c64c <random_to+0xb0>
    c6d4:	e3540008 	cmp	r4, #8
    c6d8:	03c04007 	biceq	r4, r0, #7
    c6dc:	0affffda 	beq	c64c <random_to+0xb0>
    c6e0:	9a00003e 	bls	c7e0 <random_to+0x244>
    c6e4:	e3540020 	cmp	r4, #32
    c6e8:	03c0401f 	biceq	r4, r0, #31
    c6ec:	0affffd6 	beq	c64c <random_to+0xb0>
    c6f0:	e3540040 	cmp	r4, #64	; 0x40
    c6f4:	03c0403f 	biceq	r4, r0, #63	; 0x3f
    c6f8:	0affffd3 	beq	c64c <random_to+0xb0>
    c6fc:	e3540010 	cmp	r4, #16
    c700:	1a00001f 	bne	c784 <random_to+0x1e8>
    c704:	e3c0400f 	bic	r4, r0, #15
    c708:	eaffffcf 	b	c64c <random_to+0xb0>
    c70c:	e3540a02 	cmp	r4, #8192	; 0x2000
    c710:	0a000026 	beq	c7b0 <random_to+0x214>
    c714:	e3540901 	cmp	r4, #16384	; 0x4000
    c718:	01a04720 	lsreq	r4, r0, #14
    c71c:	01a04704 	lsleq	r4, r4, #14
    c720:	0affffc9 	beq	c64c <random_to+0xb0>
    c724:	e3540a01 	cmp	r4, #4096	; 0x1000
    c728:	1a000015 	bne	c784 <random_to+0x1e8>
    c72c:	e1a04620 	lsr	r4, r0, #12
    c730:	e1a04604 	lsl	r4, r4, #12
    c734:	eaffffc4 	b	c64c <random_to+0xb0>
    c738:	e3540602 	cmp	r4, #2097152	; 0x200000
    c73c:	0a00001e 	beq	c7bc <random_to+0x220>
    c740:	e3540501 	cmp	r4, #4194304	; 0x400000
    c744:	01a04b20 	lsreq	r4, r0, #22
    c748:	01a04b04 	lsleq	r4, r4, #22
    c74c:	0affffbe 	beq	c64c <random_to+0xb0>
    c750:	e3540601 	cmp	r4, #1048576	; 0x100000
    c754:	1a00000a 	bne	c784 <random_to+0x1e8>
    c758:	e1a04a20 	lsr	r4, r0, #20
    c75c:	e1a04a04 	lsl	r4, r4, #20
    c760:	eaffffb9 	b	c64c <random_to+0xb0>
    c764:	e3540202 	cmp	r4, #536870912	; 0x20000000
    c768:	0200420e 	andeq	r4, r0, #-536870912	; 0xe0000000
    c76c:	0affffb6 	beq	c64c <random_to+0xb0>
    c770:	e3540101 	cmp	r4, #1073741824	; 0x40000000
    c774:	02004103 	andeq	r4, r0, #-1073741824	; 0xc0000000
    c778:	0affffb3 	beq	c64c <random_to+0xb0>
    c77c:	e3540201 	cmp	r4, #268435456	; 0x10000000
    c780:	0a00001d 	beq	c7fc <random_to+0x260>
    c784:	e1540000 	cmp	r4, r0
    c788:	83a04000 	movhi	r4, #0
    c78c:	8affffae 	bhi	c64c <random_to+0xb0>
    c790:	e1a02004 	mov	r2, r4
    c794:	e3a03000 	mov	r3, #0
    c798:	e0822004 	add	r2, r2, r4
    c79c:	e1520000 	cmp	r2, r0
    c7a0:	e2833001 	add	r3, r3, #1
    c7a4:	9afffffb 	bls	c798 <random_to+0x1fc>
    c7a8:	e0040493 	mul	r4, r3, r4
    c7ac:	eaffffa6 	b	c64c <random_to+0xb0>
    c7b0:	e1a046a0 	lsr	r4, r0, #13
    c7b4:	e1a04684 	lsl	r4, r4, #13
    c7b8:	eaffffa3 	b	c64c <random_to+0xb0>
    c7bc:	e1a04aa0 	lsr	r4, r0, #21
    c7c0:	e1a04a84 	lsl	r4, r4, #21
    c7c4:	eaffffa0 	b	c64c <random_to+0xb0>
    c7c8:	e1a044a0 	lsr	r4, r0, #9
    c7cc:	e1a04484 	lsl	r4, r4, #9
    c7d0:	eaffff9d 	b	c64c <random_to+0xb0>
    c7d4:	e1a048a0 	lsr	r4, r0, #17
    c7d8:	e1a04884 	lsl	r4, r4, #17
    c7dc:	eaffff9a 	b	c64c <random_to+0xb0>
    c7e0:	e3540002 	cmp	r4, #2
    c7e4:	01a04083 	lsleq	r4, r3, #1
    c7e8:	0affff97 	beq	c64c <random_to+0xb0>
    c7ec:	e3540004 	cmp	r4, #4
    c7f0:	03c04003 	biceq	r4, r0, #3
    c7f4:	0affff94 	beq	c64c <random_to+0xb0>
    c7f8:	eaffffe1 	b	c784 <random_to+0x1e8>
    c7fc:	e200420f 	and	r4, r0, #-268435456	; 0xf0000000
    c800:	eaffff91 	b	c64c <random_to+0xb0>
    c804:	e1a049a0 	lsr	r4, r0, #19
    c808:	e1a04984 	lsl	r4, r4, #19
    c80c:	eaffff8e 	b	c64c <random_to+0xb0>
    c810:	e1a045a0 	lsr	r4, r0, #11
    c814:	e1a04584 	lsl	r4, r4, #11
    c818:	eaffff8b 	b	c64c <random_to+0xb0>
    c81c:	e1a04ba0 	lsr	r4, r0, #23
    c820:	e1a04b84 	lsl	r4, r4, #23
    c824:	eaffff88 	b	c64c <random_to+0xb0>
    c828:	e3c0407f 	bic	r4, r0, #127	; 0x7f
    c82c:	eaffff86 	b	c64c <random_to+0xb0>
    c830:	e1a047a0 	lsr	r4, r0, #15
    c834:	e1a04784 	lsl	r4, r4, #15
    c838:	eaffff83 	b	c64c <random_to+0xb0>
    c83c:	0000b49c 	muleq	r0, ip, r4

0000c840 <shm_alloc>:
    c840:	e1a0c00d 	mov	ip, sp
    c844:	e1a03000 	mov	r3, r0
    c848:	e92dd800 	push	{fp, ip, lr, pc}
    c84c:	e1a02001 	mov	r2, r1
    c850:	e24cb004 	sub	fp, ip, #4
    c854:	e1a01003 	mov	r1, r3
    c858:	e3a0001a 	mov	r0, #26
    c85c:	ebffecb8 	bl	7b44 <syscall2>
    c860:	e24bd00c 	sub	sp, fp, #12
    c864:	e89d6800 	ldm	sp, {fp, sp, lr}
    c868:	e12fff1e 	bx	lr

0000c86c <shm_map>:
    c86c:	e1a0c00d 	mov	ip, sp
    c870:	e1a01000 	mov	r1, r0
    c874:	e92dd800 	push	{fp, ip, lr, pc}
    c878:	e3a0001b 	mov	r0, #27
    c87c:	e24cb004 	sub	fp, ip, #4
    c880:	ebffecb1 	bl	7b4c <syscall1>
    c884:	e24bd00c 	sub	sp, fp, #12
    c888:	e89d6800 	ldm	sp, {fp, sp, lr}
    c88c:	e12fff1e 	bx	lr

0000c890 <shm_unmap>:
    c890:	e1a0c00d 	mov	ip, sp
    c894:	e1a01000 	mov	r1, r0
    c898:	e92dd800 	push	{fp, ip, lr, pc}
    c89c:	e3a0001c 	mov	r0, #28
    c8a0:	e24cb004 	sub	fp, ip, #4
    c8a4:	ebffeca8 	bl	7b4c <syscall1>
    c8a8:	e24bd00c 	sub	sp, fp, #12
    c8ac:	e89d6800 	ldm	sp, {fp, sp, lr}
    c8b0:	e12fff1e 	bx	lr

0000c8b4 <proc_init>:
    c8b4:	e3e02000 	mvn	r2, #0
    c8b8:	e59f3008 	ldr	r3, [pc, #8]	; c8c8 <proc_init+0x14>
    c8bc:	e08f3003 	add	r3, pc, r3
    c8c0:	e5832000 	str	r2, [r3]
    c8c4:	e12fff1e 	bx	lr
    c8c8:	00040b2c 	andeq	r0, r4, ip, lsr #22

0000c8cc <get_vfsd_pid>:
    c8cc:	e1a0c00d 	mov	ip, sp
    c8d0:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    c8d4:	e59f4038 	ldr	r4, [pc, #56]	; c914 <get_vfsd_pid+0x48>
    c8d8:	e08f4004 	add	r4, pc, r4
    c8dc:	e5940000 	ldr	r0, [r4]
    c8e0:	e3500000 	cmp	r0, #0
    c8e4:	e24cb004 	sub	fp, ip, #4
    c8e8:	ba000002 	blt	c8f8 <get_vfsd_pid+0x2c>
    c8ec:	e24bd014 	sub	sp, fp, #20
    c8f0:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    c8f4:	e12fff1e 	bx	lr
    c8f8:	e59f0018 	ldr	r0, [pc, #24]	; c918 <get_vfsd_pid+0x4c>
    c8fc:	e08f0000 	add	r0, pc, r0
    c900:	ebfff7e9 	bl	a8ac <ipc_serv_get>
    c904:	e5840000 	str	r0, [r4]
    c908:	e24bd014 	sub	sp, fp, #20
    c90c:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    c910:	e12fff1e 	bx	lr
    c914:	00040b10 	andeq	r0, r4, r0, lsl fp
    c918:	00002b68 	andeq	r2, r0, r8, ror #22

0000c91c <proc_getpid>:
    c91c:	e1a0c00d 	mov	ip, sp
    c920:	e1a01000 	mov	r1, r0
    c924:	e92dd800 	push	{fp, ip, lr, pc}
    c928:	e3a00012 	mov	r0, #18
    c92c:	e24cb004 	sub	fp, ip, #4
    c930:	ebffec85 	bl	7b4c <syscall1>
    c934:	e24bd00c 	sub	sp, fp, #12
    c938:	e89d6800 	ldm	sp, {fp, sp, lr}
    c93c:	e12fff1e 	bx	lr

0000c940 <proc_detach>:
    c940:	e1a0c00d 	mov	ip, sp
    c944:	e3a0000c 	mov	r0, #12
    c948:	e92dd800 	push	{fp, ip, lr, pc}
    c94c:	e24cb004 	sub	fp, ip, #4
    c950:	ebffec80 	bl	7b58 <syscall0>
    c954:	e24bd00c 	sub	sp, fp, #12
    c958:	e89d6800 	ldm	sp, {fp, sp, lr}
    c95c:	e12fff1e 	bx	lr

0000c960 <proc_ping>:
    c960:	e1a0c00d 	mov	ip, sp
    c964:	e1a01000 	mov	r1, r0
    c968:	e92dd800 	push	{fp, ip, lr, pc}
    c96c:	e3a00014 	mov	r0, #20
    c970:	e24cb004 	sub	fp, ip, #4
    c974:	ebffec74 	bl	7b4c <syscall1>
    c978:	e24bd00c 	sub	sp, fp, #12
    c97c:	e89d6800 	ldm	sp, {fp, sp, lr}
    c980:	e12fff1e 	bx	lr

0000c984 <proc_ready_ping>:
    c984:	e1a0c00d 	mov	ip, sp
    c988:	e3a00015 	mov	r0, #21
    c98c:	e92dd800 	push	{fp, ip, lr, pc}
    c990:	e24cb004 	sub	fp, ip, #4
    c994:	ebffec6f 	bl	7b58 <syscall0>
    c998:	e24bd00c 	sub	sp, fp, #12
    c99c:	e89d6800 	ldm	sp, {fp, sp, lr}
    c9a0:	e12fff1e 	bx	lr

0000c9a4 <proc_wait_ready>:
    c9a4:	e1a0c00d 	mov	ip, sp
    c9a8:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    c9ac:	e24cb004 	sub	fp, ip, #4
    c9b0:	e1a04000 	mov	r4, r0
    c9b4:	ea000000 	b	c9bc <proc_wait_ready+0x18>
    c9b8:	eb00015c 	bl	cf30 <usleep>
    c9bc:	e3a00014 	mov	r0, #20
    c9c0:	e1a01004 	mov	r1, r4
    c9c4:	ebffec60 	bl	7b4c <syscall1>
    c9c8:	e3500000 	cmp	r0, #0
    c9cc:	e59f000c 	ldr	r0, [pc, #12]	; c9e0 <proc_wait_ready+0x3c>
    c9d0:	1afffff8 	bne	c9b8 <proc_wait_ready+0x14>
    c9d4:	e24bd014 	sub	sp, fp, #20
    c9d8:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    c9dc:	e12fff1e 	bx	lr
    c9e0:	00002710 	andeq	r2, r0, r0, lsl r7

0000c9e4 <proc_block>:
    c9e4:	e1a0c00d 	mov	ip, sp
    c9e8:	e1a03000 	mov	r3, r0
    c9ec:	e92dd800 	push	{fp, ip, lr, pc}
    c9f0:	e1a02001 	mov	r2, r1
    c9f4:	e24cb004 	sub	fp, ip, #4
    c9f8:	e1a01003 	mov	r1, r3
    c9fc:	e3a0000d 	mov	r0, #13
    ca00:	ebffec4f 	bl	7b44 <syscall2>
    ca04:	e24bd00c 	sub	sp, fp, #12
    ca08:	e89d6800 	ldm	sp, {fp, sp, lr}
    ca0c:	e12fff1e 	bx	lr

0000ca10 <proc_wakeup>:
    ca10:	e1a0c00d 	mov	ip, sp
    ca14:	e1a01000 	mov	r1, r0
    ca18:	e92dd800 	push	{fp, ip, lr, pc}
    ca1c:	e3a0000e 	mov	r0, #14
    ca20:	e24cb004 	sub	fp, ip, #4
    ca24:	ebffec48 	bl	7b4c <syscall1>
    ca28:	e24bd00c 	sub	sp, fp, #12
    ca2c:	e89d6800 	ldm	sp, {fp, sp, lr}
    ca30:	e12fff1e 	bx	lr

0000ca34 <proc_exec_elf>:
    ca34:	e1a0c00d 	mov	ip, sp
    ca38:	e92dd800 	push	{fp, ip, lr, pc}
    ca3c:	e24cb004 	sub	fp, ip, #4
    ca40:	e1a0c001 	mov	ip, r1
    ca44:	e1a03002 	mov	r3, r2
    ca48:	e1a01000 	mov	r1, r0
    ca4c:	e1a0200c 	mov	r2, ip
    ca50:	e3a00005 	mov	r0, #5
    ca54:	ebffec39 	bl	7b40 <syscall3>
    ca58:	e24bd00c 	sub	sp, fp, #12
    ca5c:	e89d6800 	ldm	sp, {fp, sp, lr}
    ca60:	e12fff1e 	bx	lr

0000ca64 <buffer_is_empty>:
    ca64:	e5903400 	ldr	r3, [r0, #1024]	; 0x400
    ca68:	e5900404 	ldr	r0, [r0, #1028]	; 0x404
    ca6c:	e0600003 	rsb	r0, r0, r3
    ca70:	e3500000 	cmp	r0, #0
    ca74:	c3a00000 	movgt	r0, #0
    ca78:	d3a00001 	movle	r0, #1
    ca7c:	e12fff1e 	bx	lr

0000ca80 <buffer_read>:
    ca80:	e1a0c00d 	mov	ip, sp
    ca84:	e2803b01 	add	r3, r0, #1024	; 0x400
    ca88:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    ca8c:	e24cb004 	sub	fp, ip, #4
    ca90:	e8931008 	ldm	r3, {r3, ip}
    ca94:	e06c3003 	rsb	r3, ip, r3
    ca98:	e1a05000 	mov	r5, r0
    ca9c:	e2430001 	sub	r0, r3, #1
    caa0:	e3500b01 	cmp	r0, #1024	; 0x400
    caa4:	23a00000 	movcs	r0, #0
    caa8:	2a000010 	bcs	caf0 <buffer_read+0x70>
    caac:	e1520003 	cmp	r2, r3
    cab0:	a1a04003 	movge	r4, r3
    cab4:	b1a04002 	movlt	r4, r2
    cab8:	e1a00001 	mov	r0, r1
    cabc:	e1a02004 	mov	r2, r4
    cac0:	e085100c 	add	r1, r5, ip
    cac4:	eb00050b 	bl	def8 <memcpy>
    cac8:	e2852b01 	add	r2, r5, #1024	; 0x400
    cacc:	e892000c 	ldm	r2, {r2, r3}
    cad0:	e0843003 	add	r3, r4, r3
    cad4:	e1530002 	cmp	r3, r2
    cad8:	e5853404 	str	r3, [r5, #1028]	; 0x404
    cadc:	03a03000 	moveq	r3, #0
    cae0:	01a00004 	moveq	r0, r4
    cae4:	11a00004 	movne	r0, r4
    cae8:	05853404 	streq	r3, [r5, #1028]	; 0x404
    caec:	05853400 	streq	r3, [r5, #1024]	; 0x400
    caf0:	e24bd014 	sub	sp, fp, #20
    caf4:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    caf8:	e12fff1e 	bx	lr

0000cafc <buffer_write>:
    cafc:	e1a0c00d 	mov	ip, sp
    cb00:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
    cb04:	e5906400 	ldr	r6, [r0, #1024]	; 0x400
    cb08:	e3560000 	cmp	r6, #0
    cb0c:	e1a05000 	mov	r5, r0
    cb10:	e24cb004 	sub	fp, ip, #4
    cb14:	13a00000 	movne	r0, #0
    cb18:	1a000007 	bne	cb3c <buffer_write+0x40>
    cb1c:	e3520b01 	cmp	r2, #1024	; 0x400
    cb20:	b1a04002 	movlt	r4, r2
    cb24:	a3a04b01 	movge	r4, #1024	; 0x400
    cb28:	e1a02004 	mov	r2, r4
    cb2c:	eb0004f1 	bl	def8 <memcpy>
    cb30:	e1a00004 	mov	r0, r4
    cb34:	e5854400 	str	r4, [r5, #1024]	; 0x400
    cb38:	e5856404 	str	r6, [r5, #1028]	; 0x404
    cb3c:	e24bd01c 	sub	sp, fp, #28
    cb40:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
    cb44:	e12fff1e 	bx	lr

0000cb48 <mmio_map>:
    cb48:	e1a0c00d 	mov	ip, sp
    cb4c:	e92dd810 	push	{r4, fp, ip, lr, pc}
    cb50:	e1a04000 	mov	r4, r0
    cb54:	e24cb004 	sub	fp, ip, #4
    cb58:	e24dd044 	sub	sp, sp, #68	; 0x44
    cb5c:	e24b1054 	sub	r1, fp, #84	; 0x54
    cb60:	e3a0001e 	mov	r0, #30
    cb64:	ebffebf8 	bl	7b4c <syscall1>
    cb68:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
    cb6c:	e3540000 	cmp	r4, #0
    cb70:	13833102 	orrne	r3, r3, #-2147483648	; 0x80000000
    cb74:	e51b1028 	ldr	r1, [fp, #-40]	; 0x28
    cb78:	e51b202c 	ldr	r2, [fp, #-44]	; 0x2c
    cb7c:	e3a00021 	mov	r0, #33	; 0x21
    cb80:	ebffebee 	bl	7b40 <syscall3>
    cb84:	e59f4018 	ldr	r4, [pc, #24]	; cba4 <mmio_map+0x5c>
    cb88:	e59f3018 	ldr	r3, [pc, #24]	; cba8 <mmio_map+0x60>
    cb8c:	e08f4004 	add	r4, pc, r4
    cb90:	e7943003 	ldr	r3, [r4, r3]
    cb94:	e5830000 	str	r0, [r3]
    cb98:	e24bd010 	sub	sp, fp, #16
    cb9c:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
    cba0:	e12fff1e 	bx	lr
    cba4:	0000ae6c 	andeq	sl, r0, ip, ror #28
    cba8:	00000024 	andeq	r0, r0, r4, lsr #32

0000cbac <dup2>:
    cbac:	e1a0c00d 	mov	ip, sp
    cbb0:	e92dd800 	push	{fp, ip, lr, pc}
    cbb4:	e24cb004 	sub	fp, ip, #4
    cbb8:	ebffef39 	bl	88a4 <vfs_dup2>
    cbbc:	e24bd00c 	sub	sp, fp, #12
    cbc0:	e89d6800 	ldm	sp, {fp, sp, lr}
    cbc4:	e12fff1e 	bx	lr

0000cbc8 <exec>:
    cbc8:	e1a0c00d 	mov	ip, sp
    cbcc:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    cbd0:	e1a06000 	mov	r6, r0
    cbd4:	e59f0090 	ldr	r0, [pc, #144]	; cc6c <exec+0xa4>
    cbd8:	e24cb004 	sub	fp, ip, #4
    cbdc:	e24dd00c 	sub	sp, sp, #12
    cbe0:	e08f0000 	add	r0, pc, r0
    cbe4:	ebffec28 	bl	7c8c <str_new>
    cbe8:	e5d61000 	ldrb	r1, [r6]
    cbec:	e31100df 	tst	r1, #223	; 0xdf
    cbf0:	e1a05000 	mov	r5, r0
    cbf4:	11a04006 	movne	r4, r6
    cbf8:	0a000004 	beq	cc10 <exec+0x48>
    cbfc:	e1a00005 	mov	r0, r5
    cc00:	ebffec6a 	bl	7db0 <str_addc>
    cc04:	e5f41001 	ldrb	r1, [r4, #1]!
    cc08:	e31100df 	tst	r1, #223	; 0xdf
    cc0c:	1afffffa 	bne	cbfc <exec+0x34>
    cc10:	e3a01000 	mov	r1, #0
    cc14:	e1a00005 	mov	r0, r5
    cc18:	ebffec64 	bl	7db0 <str_addc>
    cc1c:	e24b1020 	sub	r1, fp, #32
    cc20:	e5950000 	ldr	r0, [r5]
    cc24:	ebfff1c3 	bl	9338 <vfs_readfile>
    cc28:	e1a04000 	mov	r4, r0
    cc2c:	e1a00005 	mov	r0, r5
    cc30:	ebffec88 	bl	7e58 <str_free>
    cc34:	e3540000 	cmp	r4, #0
    cc38:	0a000009 	beq	cc64 <exec+0x9c>
    cc3c:	e1a00006 	mov	r0, r6
    cc40:	e1a01004 	mov	r1, r4
    cc44:	e51b2020 	ldr	r2, [fp, #-32]
    cc48:	ebffff79 	bl	ca34 <proc_exec_elf>
    cc4c:	e1a00004 	mov	r0, r4
    cc50:	eb000194 	bl	d2a8 <free>
    cc54:	e3a00000 	mov	r0, #0
    cc58:	e24bd018 	sub	sp, fp, #24
    cc5c:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    cc60:	e12fff1e 	bx	lr
    cc64:	e3e00000 	mvn	r0, #0
    cc68:	eafffffa 	b	cc58 <exec+0x90>
    cc6c:	0000290c 	andeq	r2, r0, ip, lsl #18

0000cc70 <fork>:
    cc70:	e1a0c00d 	mov	ip, sp
    cc74:	e3a00006 	mov	r0, #6
    cc78:	e92dd800 	push	{fp, ip, lr, pc}
    cc7c:	e24cb004 	sub	fp, ip, #4
    cc80:	ebffebb4 	bl	7b58 <syscall0>
    cc84:	e24bd00c 	sub	sp, fp, #12
    cc88:	e89d6800 	ldm	sp, {fp, sp, lr}
    cc8c:	e12fff1e 	bx	lr

0000cc90 <getcwd>:
    cc90:	e1a0c00d 	mov	ip, sp
    cc94:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
    cc98:	e24cb004 	sub	fp, ip, #4
    cc9c:	e24dd01c 	sub	sp, sp, #28
    cca0:	e1a06001 	mov	r6, r1
    cca4:	e1a05000 	mov	r5, r0
    cca8:	ebfffbe6 	bl	bc48 <get_proto_factor>
    ccac:	e24b4034 	sub	r4, fp, #52	; 0x34
    ccb0:	e5903004 	ldr	r3, [r0, #4]
    ccb4:	e1a00004 	mov	r0, r4
    ccb8:	e1a0e00f 	mov	lr, pc
    ccbc:	e12fff13 	bx	r3
    ccc0:	eb00042b 	bl	dd74 <get_cored_pid>
    ccc4:	e1a03004 	mov	r3, r4
    ccc8:	e3a01004 	mov	r1, #4
    cccc:	e3a02000 	mov	r2, #0
    ccd0:	ebfff62c 	bl	a588 <ipc_call>
    ccd4:	e3500000 	cmp	r0, #0
    ccd8:	0a000008 	beq	cd00 <getcwd+0x70>
    ccdc:	ebfffbd9 	bl	bc48 <get_proto_factor>
    cce0:	e590300c 	ldr	r3, [r0, #12]
    cce4:	e1a00004 	mov	r0, r4
    cce8:	e1a0e00f 	mov	lr, pc
    ccec:	e12fff13 	bx	r3
    ccf0:	e1a00005 	mov	r0, r5
    ccf4:	e24bd018 	sub	sp, fp, #24
    ccf8:	e89d6870 	ldm	sp, {r4, r5, r6, fp, sp, lr}
    ccfc:	e12fff1e 	bx	lr
    cd00:	e1a00004 	mov	r0, r4
    cd04:	ebfffc98 	bl	bf6c <proto_read_int>
    cd08:	e3500000 	cmp	r0, #0
    cd0c:	1afffff2 	bne	ccdc <getcwd+0x4c>
    cd10:	e1a00004 	mov	r0, r4
    cd14:	ebfffcb6 	bl	bff4 <proto_read_str>
    cd18:	e2462001 	sub	r2, r6, #1
    cd1c:	e1a01000 	mov	r1, r0
    cd20:	e1a00005 	mov	r0, r5
    cd24:	eb000667 	bl	e6c8 <strncpy>
    cd28:	eaffffeb 	b	ccdc <getcwd+0x4c>

0000cd2c <getuid>:
    cd2c:	e1a0c00d 	mov	ip, sp
    cd30:	e3a00018 	mov	r0, #24
    cd34:	e92dd800 	push	{fp, ip, lr, pc}
    cd38:	e24cb004 	sub	fp, ip, #4
    cd3c:	ebffeb85 	bl	7b58 <syscall0>
    cd40:	e24bd00c 	sub	sp, fp, #12
    cd44:	e89d6800 	ldm	sp, {fp, sp, lr}
    cd48:	e12fff1e 	bx	lr

0000cd4c <getpid>:
    cd4c:	e1a0c00d 	mov	ip, sp
    cd50:	e3e00000 	mvn	r0, #0
    cd54:	e92dd800 	push	{fp, ip, lr, pc}
    cd58:	e24cb004 	sub	fp, ip, #4
    cd5c:	ebfffeee 	bl	c91c <proc_getpid>
    cd60:	e24bd00c 	sub	sp, fp, #12
    cd64:	e89d6800 	ldm	sp, {fp, sp, lr}
    cd68:	e12fff1e 	bx	lr

0000cd6c <read>:
    cd6c:	e1a0c00d 	mov	ip, sp
    cd70:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    cd74:	e1a05001 	mov	r5, r1
    cd78:	e3a01000 	mov	r1, #0
    cd7c:	e24cb004 	sub	fp, ip, #4
    cd80:	e24dd064 	sub	sp, sp, #100	; 0x64
    cd84:	e1a04002 	mov	r4, r2
    cd88:	e1a02001 	mov	r2, r1
    cd8c:	e1a06000 	mov	r6, r0
    cd90:	eb0003a5 	bl	dc2c <fcntl>
    cd94:	e59f2138 	ldr	r2, [pc, #312]	; ced4 <read+0x168>
    cd98:	e3700001 	cmn	r0, #1
    cd9c:	e08f2002 	add	r2, pc, r2
    cda0:	0a000013 	beq	cdf4 <read+0x88>
    cda4:	e2103008 	ands	r3, r0, #8
    cda8:	0a000014 	beq	ce00 <read+0x94>
    cdac:	e3a0c000 	mov	ip, #0
    cdb0:	e59f3120 	ldr	r3, [pc, #288]	; ced8 <read+0x16c>
    cdb4:	e24b7080 	sub	r7, fp, #128	; 0x80
    cdb8:	e7923003 	ldr	r3, [r2, r3]
    cdbc:	e1a01007 	mov	r1, r7
    cdc0:	e1a00006 	mov	r0, r6
    cdc4:	e583c000 	str	ip, [r3]
    cdc8:	ebfff0c2 	bl	90d8 <vfs_get_by_fd>
    cdcc:	e2503000 	subs	r3, r0, #0
    cdd0:	1a00003d 	bne	cecc <read+0x160>
    cdd4:	e51b203c 	ldr	r2, [fp, #-60]	; 0x3c
    cdd8:	e3520003 	cmp	r2, #3
    cddc:	0a000033 	beq	ceb0 <read+0x144>
    cde0:	e1a00006 	mov	r0, r6
    cde4:	e1a01007 	mov	r1, r7
    cde8:	e1a02005 	mov	r2, r5
    cdec:	e1a03004 	mov	r3, r4
    cdf0:	ebfff42d 	bl	9eac <vfs_read>
    cdf4:	e24bd020 	sub	sp, fp, #32
    cdf8:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    cdfc:	e12fff1e 	bx	lr
    ce00:	e59f10d0 	ldr	r1, [pc, #208]	; ced8 <read+0x16c>
    ce04:	e24b7080 	sub	r7, fp, #128	; 0x80
    ce08:	e7928001 	ldr	r8, [r2, r1]
    ce0c:	e1a00006 	mov	r0, r6
    ce10:	e1a01007 	mov	r1, r7
    ce14:	e5883000 	str	r3, [r8]
    ce18:	ebfff0ae 	bl	90d8 <vfs_get_by_fd>
    ce1c:	e3500000 	cmp	r0, #0
    ce20:	1a000029 	bne	cecc <read+0x160>
    ce24:	e51b303c 	ldr	r3, [fp, #-60]	; 0x3c
    ce28:	e3530003 	cmp	r3, #3
    ce2c:	1a000008 	bne	ce54 <read+0xe8>
    ce30:	ea000016 	b	ce90 <read+0x124>
    ce34:	e5983000 	ldr	r3, [r8]
    ce38:	e3530001 	cmp	r3, #1
    ce3c:	1affffec 	bne	cdf4 <read+0x88>
    ce40:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    ce44:	e3a01000 	mov	r1, #0
    ce48:	ebfffee5 	bl	c9e4 <proc_block>
    ce4c:	e3a00000 	mov	r0, #0
    ce50:	eb00002a 	bl	cf00 <sleep>
    ce54:	e1a00006 	mov	r0, r6
    ce58:	e1a01007 	mov	r1, r7
    ce5c:	e1a02005 	mov	r2, r5
    ce60:	e1a03004 	mov	r3, r4
    ce64:	ebfff410 	bl	9eac <vfs_read>
    ce68:	e3500000 	cmp	r0, #0
    ce6c:	bafffff0 	blt	ce34 <read+0xc8>
    ce70:	e24bd020 	sub	sp, fp, #32
    ce74:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    ce78:	e12fff1e 	bx	lr
    ce7c:	e5983000 	ldr	r3, [r8]
    ce80:	e3530001 	cmp	r3, #1
    ce84:	1affffda 	bne	cdf4 <read+0x88>
    ce88:	e3a00000 	mov	r0, #0
    ce8c:	eb00001b 	bl	cf00 <sleep>
    ce90:	e1a00007 	mov	r0, r7
    ce94:	e1a01005 	mov	r1, r5
    ce98:	e1a02004 	mov	r2, r4
    ce9c:	e3a03001 	mov	r3, #1
    cea0:	ebfff3a4 	bl	9d38 <vfs_read_pipe>
    cea4:	e3500000 	cmp	r0, #0
    cea8:	bafffff3 	blt	ce7c <read+0x110>
    ceac:	eaffffd0 	b	cdf4 <read+0x88>
    ceb0:	e1a00007 	mov	r0, r7
    ceb4:	e1a01005 	mov	r1, r5
    ceb8:	e1a02004 	mov	r2, r4
    cebc:	ebfff39d 	bl	9d38 <vfs_read_pipe>
    cec0:	e24bd020 	sub	sp, fp, #32
    cec4:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    cec8:	e12fff1e 	bx	lr
    cecc:	e3e00000 	mvn	r0, #0
    ced0:	eaffffc7 	b	cdf4 <read+0x88>
    ced4:	0000ac5c 	andeq	sl, r0, ip, asr ip
    ced8:	00000018 	andeq	r0, r0, r8, lsl r0

0000cedc <setuid>:
    cedc:	e1a0c00d 	mov	ip, sp
    cee0:	e1a01000 	mov	r1, r0
    cee4:	e92dd800 	push	{fp, ip, lr, pc}
    cee8:	e3a00019 	mov	r0, #25
    ceec:	e24cb004 	sub	fp, ip, #4
    cef0:	ebffeb15 	bl	7b4c <syscall1>
    cef4:	e24bd00c 	sub	sp, fp, #12
    cef8:	e89d6800 	ldm	sp, {fp, sp, lr}
    cefc:	e12fff1e 	bx	lr

0000cf00 <sleep>:
    cf00:	e1a0c00d 	mov	ip, sp
    cf04:	e0603280 	rsb	r3, r0, r0, lsl #5
    cf08:	e0633303 	rsb	r3, r3, r3, lsl #6
    cf0c:	e0800183 	add	r0, r0, r3, lsl #3
    cf10:	e92dd800 	push	{fp, ip, lr, pc}
    cf14:	e1a00300 	lsl	r0, r0, #6
    cf18:	e24cb004 	sub	fp, ip, #4
    cf1c:	eb000003 	bl	cf30 <usleep>
    cf20:	e3a00000 	mov	r0, #0
    cf24:	e24bd00c 	sub	sp, fp, #12
    cf28:	e89d6800 	ldm	sp, {fp, sp, lr}
    cf2c:	e12fff1e 	bx	lr

0000cf30 <usleep>:
    cf30:	e1a0c00d 	mov	ip, sp
    cf34:	e2501000 	subs	r1, r0, #0
    cf38:	e92dd800 	push	{fp, ip, lr, pc}
    cf3c:	e24cb004 	sub	fp, ip, #4
    cf40:	0a000005 	beq	cf5c <usleep+0x2c>
    cf44:	e3a0000a 	mov	r0, #10
    cf48:	ebffeaff 	bl	7b4c <syscall1>
    cf4c:	e3a00000 	mov	r0, #0
    cf50:	e24bd00c 	sub	sp, fp, #12
    cf54:	e89d6800 	ldm	sp, {fp, sp, lr}
    cf58:	e12fff1e 	bx	lr
    cf5c:	e3a00008 	mov	r0, #8
    cf60:	ebffeafc 	bl	7b58 <syscall0>
    cf64:	e3a00000 	mov	r0, #0
    cf68:	e24bd00c 	sub	sp, fp, #12
    cf6c:	e89d6800 	ldm	sp, {fp, sp, lr}
    cf70:	e12fff1e 	bx	lr

0000cf74 <opendir>:
    cf74:	e1a0c00d 	mov	ip, sp
    cf78:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    cf7c:	e24cb004 	sub	fp, ip, #4
    cf80:	e24dd060 	sub	sp, sp, #96	; 0x60
    cf84:	e24b4078 	sub	r4, fp, #120	; 0x78
    cf88:	e1a01004 	mov	r1, r4
    cf8c:	ebffeea1 	bl	8a18 <vfs_get>
    cf90:	e2505000 	subs	r5, r0, #0
    cf94:	1a000012 	bne	cfe4 <opendir+0x70>
    cf98:	e24b101c 	sub	r1, fp, #28
    cf9c:	e5215060 	str	r5, [r1, #-96]!	; 0x60
    cfa0:	e1a00004 	mov	r0, r4
    cfa4:	ebffeed8 	bl	8b0c <vfs_kids>
    cfa8:	e2507000 	subs	r7, r0, #0
    cfac:	0a00000c 	beq	cfe4 <opendir+0x70>
    cfb0:	e51b607c 	ldr	r6, [fp, #-124]	; 0x7c
    cfb4:	e3560000 	cmp	r6, #0
    cfb8:	0a000009 	beq	cfe4 <opendir+0x70>
    cfbc:	e3a0000c 	mov	r0, #12
    cfc0:	eb0000c1 	bl	d2cc <malloc>
    cfc4:	e2504000 	subs	r4, r0, #0
    cfc8:	0a000009 	beq	cff4 <opendir+0x80>
    cfcc:	e1a00004 	mov	r0, r4
    cfd0:	e88400c0 	stm	r4, {r6, r7}
    cfd4:	e5845008 	str	r5, [r4, #8]
    cfd8:	e24bd01c 	sub	sp, fp, #28
    cfdc:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    cfe0:	e12fff1e 	bx	lr
    cfe4:	e3a00000 	mov	r0, #0
    cfe8:	e24bd01c 	sub	sp, fp, #28
    cfec:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    cff0:	e12fff1e 	bx	lr
    cff4:	e1a00007 	mov	r0, r7
    cff8:	eb0000aa 	bl	d2a8 <free>
    cffc:	e1a00004 	mov	r0, r4
    d000:	eafffff4 	b	cfd8 <opendir+0x64>

0000d004 <closedir>:
    d004:	e1a0c00d 	mov	ip, sp
    d008:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
    d00c:	e2504000 	subs	r4, r0, #0
    d010:	e24cb004 	sub	fp, ip, #4
    d014:	0a000008 	beq	d03c <closedir+0x38>
    d018:	e5940004 	ldr	r0, [r4, #4]
    d01c:	e3500000 	cmp	r0, #0
    d020:	1b0000a0 	blne	d2a8 <free>
    d024:	e1a00004 	mov	r0, r4
    d028:	eb00009e 	bl	d2a8 <free>
    d02c:	e3a00000 	mov	r0, #0
    d030:	e24bd014 	sub	sp, fp, #20
    d034:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
    d038:	e12fff1e 	bx	lr
    d03c:	e3e00000 	mvn	r0, #0
    d040:	eafffffa 	b	d030 <closedir+0x2c>

0000d044 <readdir>:
    d044:	e1a0c00d 	mov	ip, sp
    d048:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    d04c:	e2505000 	subs	r5, r0, #0
    d050:	e24cb004 	sub	fp, ip, #4
    d054:	0a00001e 	beq	d0d4 <readdir+0x90>
    d058:	e595c008 	ldr	ip, [r5, #8]
    d05c:	e5953000 	ldr	r3, [r5]
    d060:	e15c0003 	cmp	ip, r3
    d064:	2a00001a 	bcs	d0d4 <readdir+0x90>
    d068:	e08c308c 	add	r3, ip, ip, lsl #1
    d06c:	e5952004 	ldr	r2, [r5, #4]
    d070:	e06c3183 	rsb	r3, ip, r3, lsl #3
    d074:	e1a03103 	lsl	r3, r3, #2
    d078:	e0822003 	add	r2, r2, r3
    d07c:	e59f4060 	ldr	r4, [pc, #96]	; d0e4 <readdir+0xa0>
    d080:	e5921044 	ldr	r1, [r2, #68]	; 0x44
    d084:	e08f4004 	add	r4, pc, r4
    d088:	e5c4100a 	strb	r1, [r4, #10]
    d08c:	e5951004 	ldr	r1, [r5, #4]
    d090:	e0811003 	add	r1, r1, r3
    d094:	e5913048 	ldr	r3, [r1, #72]	; 0x48
    d098:	e592e058 	ldr	lr, [r2, #88]	; 0x58
    d09c:	e2811004 	add	r1, r1, #4
    d0a0:	e284000b 	add	r0, r4, #11
    d0a4:	e3a0203f 	mov	r2, #63	; 0x3f
    d0a8:	e1c430b8 	strh	r3, [r4, #8]
    d0ac:	e584e000 	str	lr, [r4]
    d0b0:	e584c004 	str	ip, [r4, #4]
    d0b4:	eb000583 	bl	e6c8 <strncpy>
    d0b8:	e1a00004 	mov	r0, r4
    d0bc:	e5953008 	ldr	r3, [r5, #8]
    d0c0:	e2833001 	add	r3, r3, #1
    d0c4:	e5853008 	str	r3, [r5, #8]
    d0c8:	e24bd014 	sub	sp, fp, #20
    d0cc:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    d0d0:	e12fff1e 	bx	lr
    d0d4:	e3a00000 	mov	r0, #0
    d0d8:	e24bd014 	sub	sp, fp, #20
    d0dc:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    d0e0:	e12fff1e 	bx	lr
    d0e4:	00040370 	andeq	r0, r4, r0, ror r3

0000d0e8 <atoi_base>:
    d0e8:	e92d4070 	push	{r4, r5, r6, lr}
    d0ec:	e5d03000 	ldrb	r3, [r0]
    d0f0:	e3530000 	cmp	r3, #0
    d0f4:	0a00004c 	beq	d22c <atoi_base+0x144>
    d0f8:	e3a06000 	mov	r6, #0
    d0fc:	e1a0e000 	mov	lr, r0
    d100:	e1a05006 	mov	r5, r6
    d104:	e1a00006 	mov	r0, r6
    d108:	e1a04006 	mov	r4, r6
    d10c:	ea00000a 	b	d13c <atoi_base+0x54>
    d110:	e3510008 	cmp	r1, #8
    d114:	0a00002f 	beq	d1d8 <atoi_base+0xf0>
    d118:	e351000a 	cmp	r1, #10
    d11c:	0a000010 	beq	d164 <atoi_base+0x7c>
    d120:	e3510010 	cmp	r1, #16
    d124:	0a000016 	beq	d184 <atoi_base+0x9c>
    d128:	e5fe3001 	ldrb	r3, [lr, #1]!
    d12c:	e3530000 	cmp	r3, #0
    d130:	e2844001 	add	r4, r4, #1
    d134:	e0800005 	add	r0, r0, r5
    d138:	0a000030 	beq	d200 <atoi_base+0x118>
    d13c:	e3510002 	cmp	r1, #2
    d140:	1afffff2 	bne	d110 <atoi_base+0x28>
    d144:	e2432030 	sub	r2, r3, #48	; 0x30
    d148:	e202c0ff 	and	ip, r2, #255	; 0xff
    d14c:	e35c0001 	cmp	ip, #1
    d150:	91a05002 	movls	r5, r2
    d154:	91a00080 	lslls	r0, r0, #1
    d158:	9afffff2 	bls	d128 <atoi_base+0x40>
    d15c:	e3a06001 	mov	r6, #1
    d160:	eafffff0 	b	d128 <atoi_base+0x40>
    d164:	e2432030 	sub	r2, r3, #48	; 0x30
    d168:	e20230ff 	and	r3, r2, #255	; 0xff
    d16c:	e3530009 	cmp	r3, #9
    d170:	8afffff9 	bhi	d15c <atoi_base+0x74>
    d174:	e0800100 	add	r0, r0, r0, lsl #2
    d178:	e1a00080 	lsl	r0, r0, #1
    d17c:	e1a05002 	mov	r5, r2
    d180:	eaffffe8 	b	d128 <atoi_base+0x40>
    d184:	e3540000 	cmp	r4, #0
    d188:	1a000020 	bne	d210 <atoi_base+0x128>
    d18c:	e3530030 	cmp	r3, #48	; 0x30
    d190:	0a000028 	beq	d238 <atoi_base+0x150>
    d194:	e2432030 	sub	r2, r3, #48	; 0x30
    d198:	e202c0ff 	and	ip, r2, #255	; 0xff
    d19c:	e35c0009 	cmp	ip, #9
    d1a0:	9a000003 	bls	d1b4 <atoi_base+0xcc>
    d1a4:	e3c3c020 	bic	ip, r3, #32
    d1a8:	e24cc041 	sub	ip, ip, #65	; 0x41
    d1ac:	e35c0005 	cmp	ip, #5
    d1b0:	8affffe9 	bhi	d15c <atoi_base+0x74>
    d1b4:	e243c041 	sub	ip, r3, #65	; 0x41
    d1b8:	e35c0005 	cmp	ip, #5
    d1bc:	81a05002 	movhi	r5, r2
    d1c0:	e2432061 	sub	r2, r3, #97	; 0x61
    d1c4:	92435037 	subls	r5, r3, #55	; 0x37
    d1c8:	e3520005 	cmp	r2, #5
    d1cc:	92435057 	subls	r5, r3, #87	; 0x57
    d1d0:	e1a00200 	lsl	r0, r0, #4
    d1d4:	eaffffd3 	b	d128 <atoi_base+0x40>
    d1d8:	e3540000 	cmp	r4, #0
    d1dc:	03530030 	cmpeq	r3, #48	; 0x30
    d1e0:	0a00000f 	beq	d224 <atoi_base+0x13c>
    d1e4:	e2432030 	sub	r2, r3, #48	; 0x30
    d1e8:	e20230ff 	and	r3, r2, #255	; 0xff
    d1ec:	e3530007 	cmp	r3, #7
    d1f0:	91a05002 	movls	r5, r2
    d1f4:	91a00180 	lslls	r0, r0, #3
    d1f8:	9affffca 	bls	d128 <atoi_base+0x40>
    d1fc:	eaffffd6 	b	d15c <atoi_base+0x74>
    d200:	e3560000 	cmp	r6, #0
    d204:	1a000008 	bne	d22c <atoi_base+0x144>
    d208:	e8bd4070 	pop	{r4, r5, r6, lr}
    d20c:	e12fff1e 	bx	lr
    d210:	e3540001 	cmp	r4, #1
    d214:	1affffde 	bne	d194 <atoi_base+0xac>
    d218:	e20320df 	and	r2, r3, #223	; 0xdf
    d21c:	e3520058 	cmp	r2, #88	; 0x58
    d220:	1affffdb 	bne	d194 <atoi_base+0xac>
    d224:	e3a05000 	mov	r5, #0
    d228:	eaffffbe 	b	d128 <atoi_base+0x40>
    d22c:	e1a00003 	mov	r0, r3
    d230:	e8bd4070 	pop	{r4, r5, r6, lr}
    d234:	e12fff1e 	bx	lr
    d238:	e1a05004 	mov	r5, r4
    d23c:	eaffffb9 	b	d128 <atoi_base+0x40>

0000d240 <atoi>:
    d240:	e3a0100a 	mov	r1, #10
    d244:	eaffffa7 	b	d0e8 <atoi_base>

0000d248 <calloc>:
    d248:	e1a0c00d 	mov	ip, sp
    d24c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    d250:	e0040091 	mul	r4, r1, r0
    d254:	e24cb004 	sub	fp, ip, #4
    d258:	e3a00002 	mov	r0, #2
    d25c:	e1a01004 	mov	r1, r4
    d260:	ebffea39 	bl	7b4c <syscall1>
    d264:	e2505000 	subs	r5, r0, #0
    d268:	11a02004 	movne	r2, r4
    d26c:	13a01000 	movne	r1, #0
    d270:	1b00035c 	blne	dfe8 <memset>
    d274:	e1a00005 	mov	r0, r5
    d278:	e24bd014 	sub	sp, fp, #20
    d27c:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    d280:	e12fff1e 	bx	lr

0000d284 <exit>:
    d284:	e1a0c00d 	mov	ip, sp
    d288:	e1a01000 	mov	r1, r0
    d28c:	e92dd800 	push	{fp, ip, lr, pc}
    d290:	e3a0000b 	mov	r0, #11
    d294:	e24cb004 	sub	fp, ip, #4
    d298:	ebffea2b 	bl	7b4c <syscall1>
    d29c:	e24bd00c 	sub	sp, fp, #12
    d2a0:	e89d6800 	ldm	sp, {fp, sp, lr}
    d2a4:	e12fff1e 	bx	lr

0000d2a8 <free>:
    d2a8:	e1a0c00d 	mov	ip, sp
    d2ac:	e1a01000 	mov	r1, r0
    d2b0:	e92dd800 	push	{fp, ip, lr, pc}
    d2b4:	e3a00004 	mov	r0, #4
    d2b8:	e24cb004 	sub	fp, ip, #4
    d2bc:	ebffea22 	bl	7b4c <syscall1>
    d2c0:	e24bd00c 	sub	sp, fp, #12
    d2c4:	e89d6800 	ldm	sp, {fp, sp, lr}
    d2c8:	e12fff1e 	bx	lr

0000d2cc <malloc>:
    d2cc:	e1a0c00d 	mov	ip, sp
    d2d0:	e1a01000 	mov	r1, r0
    d2d4:	e92dd800 	push	{fp, ip, lr, pc}
    d2d8:	e3a00002 	mov	r0, #2
    d2dc:	e24cb004 	sub	fp, ip, #4
    d2e0:	ebffea19 	bl	7b4c <syscall1>
    d2e4:	e24bd00c 	sub	sp, fp, #12
    d2e8:	e89d6800 	ldm	sp, {fp, sp, lr}
    d2ec:	e12fff1e 	bx	lr

0000d2f0 <realloc>:
    d2f0:	e1a0c00d 	mov	ip, sp
    d2f4:	e1a03000 	mov	r3, r0
    d2f8:	e92dd800 	push	{fp, ip, lr, pc}
    d2fc:	e1a02001 	mov	r2, r1
    d300:	e24cb004 	sub	fp, ip, #4
    d304:	e1a01003 	mov	r1, r3
    d308:	e3a00003 	mov	r0, #3
    d30c:	ebffea0c 	bl	7b44 <syscall2>
    d310:	e24bd00c 	sub	sp, fp, #12
    d314:	e89d6800 	ldm	sp, {fp, sp, lr}
    d318:	e12fff1e 	bx	lr

0000d31c <print_uint_in_base_raw>:
    d31c:	e1a0c00d 	mov	ip, sp
    d320:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    d324:	e3a0e000 	mov	lr, #0
    d328:	e1a05003 	mov	r5, r3
    d32c:	e1a06000 	mov	r6, r0
    d330:	e1a0400e 	mov	r4, lr
    d334:	e3a0301f 	mov	r3, #31
    d338:	e3a07001 	mov	r7, #1
    d33c:	e24cb004 	sub	fp, ip, #4
    d340:	e1a00317 	lsl	r0, r7, r3
    d344:	e1100001 	tst	r0, r1
    d348:	13a0c001 	movne	ip, #1
    d34c:	03a0c000 	moveq	ip, #0
    d350:	e18c4084 	orr	r4, ip, r4, lsl #1
    d354:	e1520004 	cmp	r2, r4
    d358:	e2433001 	sub	r3, r3, #1
    d35c:	90624004 	rsbls	r4, r2, r4
    d360:	918ee000 	orrls	lr, lr, r0
    d364:	e3730001 	cmn	r3, #1
    d368:	1afffff4 	bne	d340 <print_uint_in_base_raw+0x24>
    d36c:	e35e0000 	cmp	lr, #0
    d370:	1a00000f 	bne	d3b4 <print_uint_in_base_raw+0x98>
    d374:	e3550000 	cmp	r5, #0
    d378:	1a000006 	bne	d398 <print_uint_in_base_raw+0x7c>
    d37c:	e59f3044 	ldr	r3, [pc, #68]	; d3c8 <print_uint_in_base_raw+0xac>
    d380:	e08f3003 	add	r3, pc, r3
    d384:	e7d33004 	ldrb	r3, [r3, r4]
    d388:	e5c63000 	strb	r3, [r6]
    d38c:	e24bd01c 	sub	sp, fp, #28
    d390:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    d394:	e12fff1e 	bx	lr
    d398:	e59f302c 	ldr	r3, [pc, #44]	; d3cc <print_uint_in_base_raw+0xb0>
    d39c:	e08f3003 	add	r3, pc, r3
    d3a0:	e7d33004 	ldrb	r3, [r3, r4]
    d3a4:	e5c63000 	strb	r3, [r6]
    d3a8:	e24bd01c 	sub	sp, fp, #28
    d3ac:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    d3b0:	e12fff1e 	bx	lr
    d3b4:	e1a0100e 	mov	r1, lr
    d3b8:	e2860001 	add	r0, r6, #1
    d3bc:	e1a03005 	mov	r3, r5
    d3c0:	ebffffd5 	bl	d31c <print_uint_in_base_raw>
    d3c4:	eaffffea 	b	d374 <print_uint_in_base_raw+0x58>
    d3c8:	0000260c 	andeq	r2, r0, ip, lsl #12
    d3cc:	00002604 	andeq	r2, r0, r4, lsl #12

0000d3d0 <print_uint_in_base>:
    d3d0:	e1a0c00d 	mov	ip, sp
    d3d4:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    d3d8:	e24cb004 	sub	fp, ip, #4
    d3dc:	e24dd02c 	sub	sp, sp, #44	; 0x2c
    d3e0:	e1a0a003 	mov	sl, r3
    d3e4:	e24b804c 	sub	r8, fp, #76	; 0x4c
    d3e8:	e50b2050 	str	r2, [fp, #-80]	; 0x50
    d3ec:	e1a06000 	mov	r6, r0
    d3f0:	e3a02020 	mov	r2, #32
    d3f4:	e1a07001 	mov	r7, r1
    d3f8:	e1a00008 	mov	r0, r8
    d3fc:	e3a01000 	mov	r1, #0
    d400:	e5db500c 	ldrb	r5, [fp, #12]
    d404:	e5db4008 	ldrb	r4, [fp, #8]
    d408:	e59b9004 	ldr	r9, [fp, #4]
    d40c:	eb0002f5 	bl	dfe8 <memset>
    d410:	e51bc050 	ldr	ip, [fp, #-80]	; 0x50
    d414:	e1a00008 	mov	r0, r8
    d418:	e1a0100c 	mov	r1, ip
    d41c:	e1a0200a 	mov	r2, sl
    d420:	e1a03005 	mov	r3, r5
    d424:	ebffffbc 	bl	d31c <print_uint_in_base_raw>
    d428:	e1a00008 	mov	r0, r8
    d42c:	eb000447 	bl	e550 <strlen>
    d430:	e3540000 	cmp	r4, #0
    d434:	0a00000c 	beq	d46c <print_uint_in_base+0x9c>
    d438:	e0604009 	rsb	r4, r0, r9
    d43c:	e3540000 	cmp	r4, #0
    d440:	da000033 	ble	d514 <print_uint_in_base+0x144>
    d444:	e3a05000 	mov	r5, #0
    d448:	e2855001 	add	r5, r5, #1
    d44c:	e3a00030 	mov	r0, #48	; 0x30
    d450:	e1a01007 	mov	r1, r7
    d454:	e1a0e00f 	mov	lr, pc
    d458:	e12fff16 	bx	r6
    d45c:	e1550004 	cmp	r5, r4
    d460:	1afffff8 	bne	d448 <print_uint_in_base+0x78>
    d464:	e1a00008 	mov	r0, r8
    d468:	eb000438 	bl	e550 <strlen>
    d46c:	e2505001 	subs	r5, r0, #1
    d470:	4a00001b 	bmi	d4e4 <print_uint_in_base+0x114>
    d474:	e24b302c 	sub	r3, fp, #44	; 0x2c
    d478:	e0833005 	add	r3, r3, r5
    d47c:	e5530020 	ldrb	r0, [r3, #-32]
    d480:	e3500000 	cmp	r0, #0
    d484:	0a000016 	beq	d4e4 <print_uint_in_base+0x114>
    d488:	e3590000 	cmp	r9, #0
    d48c:	d3a0a000 	movle	sl, #0
    d490:	c3a0a001 	movgt	sl, #1
    d494:	e1540009 	cmp	r4, r9
    d498:	a3590000 	cmpge	r9, #0
    d49c:	ca000019 	bgt	d508 <print_uint_in_base+0x138>
    d4a0:	e0888005 	add	r8, r8, r5
    d4a4:	ea000007 	b	d4c8 <print_uint_in_base+0xf8>
    d4a8:	e5780001 	ldrb	r0, [r8, #-1]!
    d4ac:	e3500000 	cmp	r0, #0
    d4b0:	0a00000b 	beq	d4e4 <print_uint_in_base+0x114>
    d4b4:	e1590004 	cmp	r9, r4
    d4b8:	c3a03000 	movgt	r3, #0
    d4bc:	d20a3001 	andle	r3, sl, #1
    d4c0:	e3530000 	cmp	r3, #0
    d4c4:	1a00000f 	bne	d508 <print_uint_in_base+0x138>
    d4c8:	e2455001 	sub	r5, r5, #1
    d4cc:	e1a01007 	mov	r1, r7
    d4d0:	e1a0e00f 	mov	lr, pc
    d4d4:	e12fff16 	bx	r6
    d4d8:	e3750001 	cmn	r5, #1
    d4dc:	e2844001 	add	r4, r4, #1
    d4e0:	1afffff0 	bne	d4a8 <print_uint_in_base+0xd8>
    d4e4:	e1590004 	cmp	r9, r4
    d4e8:	da000006 	ble	d508 <print_uint_in_base+0x138>
    d4ec:	e2844001 	add	r4, r4, #1
    d4f0:	e3a00020 	mov	r0, #32
    d4f4:	e1a01007 	mov	r1, r7
    d4f8:	e1a0e00f 	mov	lr, pc
    d4fc:	e12fff16 	bx	r6
    d500:	e1540009 	cmp	r4, r9
    d504:	1afffff8 	bne	d4ec <print_uint_in_base+0x11c>
    d508:	e24bd028 	sub	sp, fp, #40	; 0x28
    d50c:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    d510:	e12fff1e 	bx	lr
    d514:	e3a04000 	mov	r4, #0
    d518:	eaffffd3 	b	d46c <print_uint_in_base+0x9c>

0000d51c <v_printf>:
    d51c:	e1a0c00d 	mov	ip, sp
    d520:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
    d524:	e24cb004 	sub	fp, ip, #4
    d528:	e24dd02c 	sub	sp, sp, #44	; 0x2c
    d52c:	e1a07002 	mov	r7, r2
    d530:	e5d22000 	ldrb	r2, [r2]
    d534:	e3520000 	cmp	r2, #0
    d538:	e1a08003 	mov	r8, r3
    d53c:	124b3034 	subne	r3, fp, #52	; 0x34
    d540:	e1a05000 	mov	r5, r0
    d544:	e1a06001 	mov	r6, r1
    d548:	13a04000 	movne	r4, #0
    d54c:	11a00002 	movne	r0, r2
    d550:	150b3038 	strne	r3, [fp, #-56]	; 0x38
    d554:	0a000077 	beq	d738 <v_printf+0x21c>
    d558:	e3500025 	cmp	r0, #37	; 0x25
    d55c:	0a00000a 	beq	d58c <v_printf+0x70>
    d560:	e0879004 	add	r9, r7, r4
    d564:	e1a01006 	mov	r1, r6
    d568:	e1a0e00f 	mov	lr, pc
    d56c:	e12fff15 	bx	r5
    d570:	e5f90001 	ldrb	r0, [r9, #1]!
    d574:	e3500000 	cmp	r0, #0
    d578:	13500025 	cmpne	r0, #37	; 0x25
    d57c:	e2844001 	add	r4, r4, #1
    d580:	1afffff7 	bne	d564 <v_printf+0x48>
    d584:	e3500000 	cmp	r0, #0
    d588:	0a00006a 	beq	d738 <v_printf+0x21c>
    d58c:	e0873004 	add	r3, r7, r4
    d590:	e5d33001 	ldrb	r3, [r3, #1]
    d594:	e3530000 	cmp	r3, #0
    d598:	0a000066 	beq	d738 <v_printf+0x21c>
    d59c:	e284e001 	add	lr, r4, #1
    d5a0:	e7d7300e 	ldrb	r3, [r7, lr]
    d5a4:	e3530030 	cmp	r3, #48	; 0x30
    d5a8:	0284e002 	addeq	lr, r4, #2
    d5ac:	07d7300e 	ldrbeq	r3, [r7, lr]
    d5b0:	03a09001 	moveq	r9, #1
    d5b4:	13a09000 	movne	r9, #0
    d5b8:	e353002d 	cmp	r3, #45	; 0x2d
    d5bc:	13a03000 	movne	r3, #0
    d5c0:	054b3034 	strbeq	r3, [fp, #-52]	; 0x34
    d5c4:	03a03001 	moveq	r3, #1
    d5c8:	e51b2038 	ldr	r2, [fp, #-56]	; 0x38
    d5cc:	e08ec003 	add	ip, lr, r3
    d5d0:	e24c0001 	sub	r0, ip, #1
    d5d4:	e2431001 	sub	r1, r3, #1
    d5d8:	e0870000 	add	r0, r7, r0
    d5dc:	e0821001 	add	r1, r2, r1
    d5e0:	ea000000 	b	d5e8 <v_printf+0xcc>
    d5e4:	e08ec003 	add	ip, lr, r3
    d5e8:	e5f02001 	ldrb	r2, [r0, #1]!
    d5ec:	e2424030 	sub	r4, r2, #48	; 0x30
    d5f0:	e3540009 	cmp	r4, #9
    d5f4:	8a00009d 	bhi	d870 <v_printf+0x354>
    d5f8:	e2833001 	add	r3, r3, #1
    d5fc:	e3530008 	cmp	r3, #8
    d600:	e5e12001 	strb	r2, [r1, #1]!
    d604:	1afffff6 	bne	d5e4 <v_printf+0xc8>
    d608:	e28ea008 	add	sl, lr, #8
    d60c:	e7d7200a 	ldrb	r2, [r7, sl]
    d610:	e24b102c 	sub	r1, fp, #44	; 0x2c
    d614:	e0813003 	add	r3, r1, r3
    d618:	e3a01000 	mov	r1, #0
    d61c:	e5431008 	strb	r1, [r3, #-8]
    d620:	e55b3034 	ldrb	r3, [fp, #-52]	; 0x34
    d624:	e353002d 	cmp	r3, #45	; 0x2d
    d628:	024b102c 	subeq	r1, fp, #44	; 0x2c
    d62c:	05713007 	ldrbeq	r3, [r1, #-7]!
    d630:	e2433030 	sub	r3, r3, #48	; 0x30
    d634:	e20300ff 	and	r0, r3, #255	; 0xff
    d638:	03e0e000 	mvneq	lr, #0
    d63c:	13a0e001 	movne	lr, #1
    d640:	151b1038 	ldrne	r1, [fp, #-56]	; 0x38
    d644:	e3500009 	cmp	r0, #9
    d648:	e3a04000 	mov	r4, #0
    d64c:	8a000007 	bhi	d670 <v_printf+0x154>
    d650:	e5f10001 	ldrb	r0, [r1, #1]!
    d654:	e0844104 	add	r4, r4, r4, lsl #2
    d658:	e0834084 	add	r4, r3, r4, lsl #1
    d65c:	e2403030 	sub	r3, r0, #48	; 0x30
    d660:	e20300ff 	and	r0, r3, #255	; 0xff
    d664:	e3500009 	cmp	r0, #9
    d668:	9afffff8 	bls	d650 <v_printf+0x134>
    d66c:	e004049e 	mul	r4, lr, r4
    d670:	e2422058 	sub	r2, r2, #88	; 0x58
    d674:	e3520020 	cmp	r2, #32
    d678:	908ff102 	addls	pc, pc, r2, lsl #2
    d67c:	ea000029 	b	d728 <v_printf+0x20c>
    d680:	ea000039 	b	d76c <v_printf+0x250>
    d684:	ea000027 	b	d728 <v_printf+0x20c>
    d688:	ea000026 	b	d728 <v_printf+0x20c>
    d68c:	ea000025 	b	d728 <v_printf+0x20c>
    d690:	ea000024 	b	d728 <v_printf+0x20c>
    d694:	ea000023 	b	d728 <v_printf+0x20c>
    d698:	ea000022 	b	d728 <v_printf+0x20c>
    d69c:	ea000021 	b	d728 <v_printf+0x20c>
    d6a0:	ea000020 	b	d728 <v_printf+0x20c>
    d6a4:	ea00001f 	b	d728 <v_printf+0x20c>
    d6a8:	ea00001e 	b	d728 <v_printf+0x20c>
    d6ac:	ea000047 	b	d7d0 <v_printf+0x2b4>
    d6b0:	ea000032 	b	d780 <v_printf+0x264>
    d6b4:	ea00001b 	b	d728 <v_printf+0x20c>
    d6b8:	ea00001a 	b	d728 <v_printf+0x20c>
    d6bc:	ea000019 	b	d728 <v_printf+0x20c>
    d6c0:	ea000018 	b	d728 <v_printf+0x20c>
    d6c4:	ea000017 	b	d728 <v_printf+0x20c>
    d6c8:	ea000016 	b	d728 <v_printf+0x20c>
    d6cc:	ea000015 	b	d728 <v_printf+0x20c>
    d6d0:	ea000014 	b	d728 <v_printf+0x20c>
    d6d4:	ea000013 	b	d728 <v_printf+0x20c>
    d6d8:	ea000012 	b	d728 <v_printf+0x20c>
    d6dc:	ea000011 	b	d728 <v_printf+0x20c>
    d6e0:	ea000010 	b	d728 <v_printf+0x20c>
    d6e4:	ea00000f 	b	d728 <v_printf+0x20c>
    d6e8:	ea00000e 	b	d728 <v_printf+0x20c>
    d6ec:	ea00003e 	b	d7ec <v_printf+0x2d0>
    d6f0:	ea00000c 	b	d728 <v_printf+0x20c>
    d6f4:	ea000012 	b	d744 <v_printf+0x228>
    d6f8:	ea00000a 	b	d728 <v_printf+0x20c>
    d6fc:	ea000009 	b	d728 <v_printf+0x20c>
    d700:	eaffffff 	b	d704 <v_printf+0x1e8>
    d704:	e3a03000 	mov	r3, #0
    d708:	e5982000 	ldr	r2, [r8]
    d70c:	e88d0210 	stm	sp, {r4, r9}
    d710:	e2888004 	add	r8, r8, #4
    d714:	e58d3008 	str	r3, [sp, #8]
    d718:	e1a00005 	mov	r0, r5
    d71c:	e1a01006 	mov	r1, r6
    d720:	e3a03010 	mov	r3, #16
    d724:	ebffff29 	bl	d3d0 <print_uint_in_base>
    d728:	e28a4001 	add	r4, sl, #1
    d72c:	e7d70004 	ldrb	r0, [r7, r4]
    d730:	e3500000 	cmp	r0, #0
    d734:	1affff87 	bne	d558 <v_printf+0x3c>
    d738:	e24bd028 	sub	sp, fp, #40	; 0x28
    d73c:	e89d6ff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
    d740:	e12fff1e 	bx	lr
    d744:	e5982000 	ldr	r2, [r8]
    d748:	e2888004 	add	r8, r8, #4
    d74c:	e3a03000 	mov	r3, #0
    d750:	e88d0210 	stm	sp, {r4, r9}
    d754:	e58d3008 	str	r3, [sp, #8]
    d758:	e1a00005 	mov	r0, r5
    d75c:	e1a01006 	mov	r1, r6
    d760:	e3a0300a 	mov	r3, #10
    d764:	ebffff19 	bl	d3d0 <print_uint_in_base>
    d768:	eaffffee 	b	d728 <v_printf+0x20c>
    d76c:	e5982000 	ldr	r2, [r8]
    d770:	e3a03001 	mov	r3, #1
    d774:	e2888004 	add	r8, r8, #4
    d778:	e88d0210 	stm	sp, {r4, r9}
    d77c:	eaffffe4 	b	d714 <v_printf+0x1f8>
    d780:	e5982000 	ldr	r2, [r8]
    d784:	e3520000 	cmp	r2, #0
    d788:	e2888004 	add	r8, r8, #4
    d78c:	aaffffee 	bge	d74c <v_printf+0x230>
    d790:	e50b203c 	str	r2, [fp, #-60]	; 0x3c
    d794:	e1a01006 	mov	r1, r6
    d798:	e3a0002d 	mov	r0, #45	; 0x2d
    d79c:	e1a0e00f 	mov	lr, pc
    d7a0:	e12fff15 	bx	r5
    d7a4:	e3a03000 	mov	r3, #0
    d7a8:	e51b203c 	ldr	r2, [fp, #-60]	; 0x3c
    d7ac:	e2444001 	sub	r4, r4, #1
    d7b0:	e58d3008 	str	r3, [sp, #8]
    d7b4:	e88d0210 	stm	sp, {r4, r9}
    d7b8:	e2622000 	rsb	r2, r2, #0
    d7bc:	e1a00005 	mov	r0, r5
    d7c0:	e1a01006 	mov	r1, r6
    d7c4:	e3a0300a 	mov	r3, #10
    d7c8:	ebffff00 	bl	d3d0 <print_uint_in_base>
    d7cc:	eaffffd5 	b	d728 <v_printf+0x20c>
    d7d0:	e2883004 	add	r3, r8, #4
    d7d4:	e5d80000 	ldrb	r0, [r8]
    d7d8:	e1a01006 	mov	r1, r6
    d7dc:	e1a08003 	mov	r8, r3
    d7e0:	e1a0e00f 	mov	lr, pc
    d7e4:	e12fff15 	bx	r5
    d7e8:	eaffffce 	b	d728 <v_printf+0x20c>
    d7ec:	e5983000 	ldr	r3, [r8]
    d7f0:	e1a00003 	mov	r0, r3
    d7f4:	e50b3040 	str	r3, [fp, #-64]	; 0x40
    d7f8:	eb000354 	bl	e550 <strlen>
    d7fc:	e2883004 	add	r3, r8, #4
    d800:	e3540000 	cmp	r4, #0
    d804:	e50b303c 	str	r3, [fp, #-60]	; 0x3c
    d808:	ba00001a 	blt	d878 <v_printf+0x35c>
    d80c:	e3a08000 	mov	r8, #0
    d810:	e51b3040 	ldr	r3, [fp, #-64]	; 0x40
    d814:	e2439001 	sub	r9, r3, #1
    d818:	ea000006 	b	d838 <v_printf+0x31c>
    d81c:	e2888001 	add	r8, r8, #1
    d820:	e1a01006 	mov	r1, r6
    d824:	e1a0e00f 	mov	lr, pc
    d828:	e12fff15 	bx	r5
    d82c:	e1580004 	cmp	r8, r4
    d830:	a3540000 	cmpge	r4, #0
    d834:	ca000002 	bgt	d844 <v_printf+0x328>
    d838:	e5f90001 	ldrb	r0, [r9, #1]!
    d83c:	e3500000 	cmp	r0, #0
    d840:	1afffff5 	bne	d81c <v_printf+0x300>
    d844:	e1580004 	cmp	r8, r4
    d848:	aa000006 	bge	d868 <v_printf+0x34c>
    d84c:	e2888001 	add	r8, r8, #1
    d850:	e3a00020 	mov	r0, #32
    d854:	e1a01006 	mov	r1, r6
    d858:	e1a0e00f 	mov	lr, pc
    d85c:	e12fff15 	bx	r5
    d860:	e1580004 	cmp	r8, r4
    d864:	1afffff8 	bne	d84c <v_printf+0x330>
    d868:	e51b803c 	ldr	r8, [fp, #-60]	; 0x3c
    d86c:	eaffffad 	b	d728 <v_printf+0x20c>
    d870:	e1a0a00c 	mov	sl, ip
    d874:	eaffff65 	b	d610 <v_printf+0xf4>
    d878:	e2644000 	rsb	r4, r4, #0
    d87c:	e0608004 	rsb	r8, r0, r4
    d880:	e3580000 	cmp	r8, #0
    d884:	daffffe0 	ble	d80c <v_printf+0x2f0>
    d888:	e3a09000 	mov	r9, #0
    d88c:	e2899001 	add	r9, r9, #1
    d890:	e3a00020 	mov	r0, #32
    d894:	e1a01006 	mov	r1, r6
    d898:	e1a0e00f 	mov	lr, pc
    d89c:	e12fff15 	bx	r5
    d8a0:	e1590008 	cmp	r9, r8
    d8a4:	1afffff8 	bne	d88c <v_printf+0x370>
    d8a8:	eaffffd8 	b	d810 <v_printf+0x2f4>

0000d8ac <outc_sn>:
    d8ac:	e5913004 	ldr	r3, [r1, #4]
    d8b0:	e5912008 	ldr	r2, [r1, #8]
    d8b4:	e1530002 	cmp	r3, r2
    d8b8:	35912000 	ldrcc	r2, [r1]
    d8bc:	37c20003 	strbcc	r0, [r2, r3]
    d8c0:	35913004 	ldrcc	r3, [r1, #4]
    d8c4:	32833001 	addcc	r3, r3, #1
    d8c8:	35813004 	strcc	r3, [r1, #4]
    d8cc:	e12fff1e 	bx	lr

0000d8d0 <snprintf>:
    d8d0:	e1a0c00d 	mov	ip, sp
    d8d4:	e92d000c 	push	{r2, r3}
    d8d8:	e92dd810 	push	{r4, fp, ip, lr, pc}
    d8dc:	e3a04000 	mov	r4, #0
    d8e0:	e24cb00c 	sub	fp, ip, #12
    d8e4:	e24dd014 	sub	sp, sp, #20
    d8e8:	e50b0020 	str	r0, [fp, #-32]
    d8ec:	e59f003c 	ldr	r0, [pc, #60]	; d930 <snprintf+0x60>
    d8f0:	e28bc008 	add	ip, fp, #8
    d8f4:	e50b1018 	str	r1, [fp, #-24]
    d8f8:	e1a0300c 	mov	r3, ip
    d8fc:	e59b2004 	ldr	r2, [fp, #4]
    d900:	e08f0000 	add	r0, pc, r0
    d904:	e24b1020 	sub	r1, fp, #32
    d908:	e50b401c 	str	r4, [fp, #-28]
    d90c:	e50bc024 	str	ip, [fp, #-36]	; 0x24
    d910:	ebffff01 	bl	d51c <v_printf>
    d914:	e24b2020 	sub	r2, fp, #32
    d918:	e892000c 	ldm	r2, {r2, r3}
    d91c:	e7c24003 	strb	r4, [r2, r3]
    d920:	e51b001c 	ldr	r0, [fp, #-28]
    d924:	e24bd010 	sub	sp, fp, #16
    d928:	e89d6810 	ldm	sp, {r4, fp, sp, lr}
    d92c:	e12fff1e 	bx	lr
    d930:	ffffffa4 			; <UNDEFINED> instruction: 0xffffffa4

0000d934 <outc>:
    d934:	e1a0c00d 	mov	ip, sp
    d938:	e1a03000 	mov	r3, r0
    d93c:	e92dd800 	push	{fp, ip, lr, pc}
    d940:	e1a00001 	mov	r0, r1
    d944:	e24cb004 	sub	fp, ip, #4
    d948:	e1a01003 	mov	r1, r3
    d94c:	ebffe917 	bl	7db0 <str_addc>
    d950:	e24bd00c 	sub	sp, fp, #12
    d954:	e89d6800 	ldm	sp, {fp, sp, lr}
    d958:	e12fff1e 	bx	lr

0000d95c <dprintf>:
    d95c:	e1a0c00d 	mov	ip, sp
    d960:	e92d000e 	push	{r1, r2, r3}
    d964:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
    d968:	e1a05000 	mov	r5, r0
    d96c:	e59f0054 	ldr	r0, [pc, #84]	; d9c8 <dprintf+0x6c>
    d970:	e24cb010 	sub	fp, ip, #16
    d974:	e24dd00c 	sub	sp, sp, #12
    d978:	e28b3008 	add	r3, fp, #8
    d97c:	e08f0000 	add	r0, pc, r0
    d980:	e50b301c 	str	r3, [fp, #-28]
    d984:	ebffe8c0 	bl	7c8c <str_new>
    d988:	e1a04000 	mov	r4, r0
    d98c:	e1a01000 	mov	r1, r0
    d990:	e59f0034 	ldr	r0, [pc, #52]	; d9cc <dprintf+0x70>
    d994:	e51b301c 	ldr	r3, [fp, #-28]
    d998:	e59b2004 	ldr	r2, [fp, #4]
    d99c:	e08f0000 	add	r0, pc, r0
    d9a0:	ebfffedd 	bl	d51c <v_printf>
    d9a4:	e5941000 	ldr	r1, [r4]
    d9a8:	e5942008 	ldr	r2, [r4, #8]
    d9ac:	e1a00005 	mov	r0, r5
    d9b0:	eb0000f7 	bl	dd94 <write>
    d9b4:	e1a00004 	mov	r0, r4
    d9b8:	ebffe926 	bl	7e58 <str_free>
    d9bc:	e24bd014 	sub	sp, fp, #20
    d9c0:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
    d9c4:	e12fff1e 	bx	lr
    d9c8:	00001b70 	andeq	r1, r0, r0, ror fp
    d9cc:	ffffff90 			; <UNDEFINED> instruction: 0xffffff90

0000d9d0 <open>:
    d9d0:	e1a0c00d 	mov	ip, sp
    d9d4:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
    d9d8:	e24cb004 	sub	fp, ip, #4
    d9dc:	e24dd090 	sub	sp, sp, #144	; 0x90
    d9e0:	e24b5080 	sub	r5, fp, #128	; 0x80
    d9e4:	e1a07001 	mov	r7, r1
    d9e8:	e1a01005 	mov	r1, r5
    d9ec:	e1a04000 	mov	r4, r0
    d9f0:	ebffec08 	bl	8a18 <vfs_get>
    d9f4:	e2506000 	subs	r6, r0, #0
    d9f8:	0a00004e 	beq	db38 <open+0x168>
    d9fc:	e3170004 	tst	r7, #4
    da00:	0a000048 	beq	db28 <open+0x158>
    da04:	e1a00004 	mov	r0, r4
    da08:	e1a01005 	mov	r1, r5
    da0c:	e3a02001 	mov	r2, #1
    da10:	e3a03000 	mov	r3, #0
    da14:	ebffeec0 	bl	951c <vfs_create>
    da18:	e3500000 	cmp	r0, #0
    da1c:	1a000041 	bne	db28 <open+0x158>
    da20:	e1a00005 	mov	r0, r5
    da24:	e1a01007 	mov	r1, r7
    da28:	ebffeb19 	bl	8694 <vfs_open>
    da2c:	e2504000 	subs	r4, r0, #0
    da30:	a3a09001 	movge	r9, #1
    da34:	ba000039 	blt	db20 <open+0x150>
    da38:	ebfff882 	bl	bc48 <get_proto_factor>
    da3c:	e24b8098 	sub	r8, fp, #152	; 0x98
    da40:	e5903004 	ldr	r3, [r0, #4]
    da44:	e1a00008 	mov	r0, r8
    da48:	e1a0e00f 	mov	lr, pc
    da4c:	e12fff13 	bx	r3
    da50:	ebfff87c 	bl	bc48 <get_proto_factor>
    da54:	e24b60b0 	sub	r6, fp, #176	; 0xb0
    da58:	e5903004 	ldr	r3, [r0, #4]
    da5c:	e1a00006 	mov	r0, r6
    da60:	e1a0e00f 	mov	lr, pc
    da64:	e12fff13 	bx	r3
    da68:	e1a01004 	mov	r1, r4
    da6c:	e5903014 	ldr	r3, [r0, #20]
    da70:	e1a00006 	mov	r0, r6
    da74:	e1a0e00f 	mov	lr, pc
    da78:	e12fff13 	bx	r3
    da7c:	e3a0205c 	mov	r2, #92	; 0x5c
    da80:	e5903010 	ldr	r3, [r0, #16]
    da84:	e1a01005 	mov	r1, r5
    da88:	e1a00006 	mov	r0, r6
    da8c:	e1a0e00f 	mov	lr, pc
    da90:	e12fff13 	bx	r3
    da94:	e1a01007 	mov	r1, r7
    da98:	e5903014 	ldr	r3, [r0, #20]
    da9c:	e1a00006 	mov	r0, r6
    daa0:	e1a0e00f 	mov	lr, pc
    daa4:	e12fff13 	bx	r3
    daa8:	e1a02006 	mov	r2, r6
    daac:	e1a03008 	mov	r3, r8
    dab0:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
    dab4:	e3a01001 	mov	r1, #1
    dab8:	ebfff2b2 	bl	a588 <ipc_call>
    dabc:	e3500000 	cmp	r0, #0
    dac0:	1a000003 	bne	dad4 <open+0x104>
    dac4:	e1a00008 	mov	r0, r8
    dac8:	ebfff927 	bl	bf6c <proto_read_int>
    dacc:	e3500000 	cmp	r0, #0
    dad0:	0a000004 	beq	dae8 <open+0x118>
    dad4:	e1a00004 	mov	r0, r4
    dad8:	ebffeb52 	bl	8828 <vfs_close>
    dadc:	e3590000 	cmp	r9, #0
    dae0:	1a00001c 	bne	db58 <open+0x188>
    dae4:	e3e04000 	mvn	r4, #0
    dae8:	ebfff856 	bl	bc48 <get_proto_factor>
    daec:	e590300c 	ldr	r3, [r0, #12]
    daf0:	e1a00006 	mov	r0, r6
    daf4:	e1a0e00f 	mov	lr, pc
    daf8:	e12fff13 	bx	r3
    dafc:	ebfff851 	bl	bc48 <get_proto_factor>
    db00:	e590300c 	ldr	r3, [r0, #12]
    db04:	e1a00008 	mov	r0, r8
    db08:	e1a0e00f 	mov	lr, pc
    db0c:	e12fff13 	bx	r3
    db10:	e1a00004 	mov	r0, r4
    db14:	e24bd024 	sub	sp, fp, #36	; 0x24
    db18:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    db1c:	e12fff1e 	bx	lr
    db20:	e1a00005 	mov	r0, r5
    db24:	ebffec9e 	bl	8da4 <vfs_del>
    db28:	e3e00000 	mvn	r0, #0
    db2c:	e24bd024 	sub	sp, fp, #36	; 0x24
    db30:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
    db34:	e12fff1e 	bx	lr
    db38:	e1a00005 	mov	r0, r5
    db3c:	e1a01007 	mov	r1, r7
    db40:	ebffead3 	bl	8694 <vfs_open>
    db44:	e2504000 	subs	r4, r0, #0
    db48:	a1a09006 	movge	r9, r6
    db4c:	aaffffb9 	bge	da38 <open+0x68>
    db50:	e3e00000 	mvn	r0, #0
    db54:	eafffff4 	b	db2c <open+0x15c>
    db58:	e1a00005 	mov	r0, r5
    db5c:	ebffec90 	bl	8da4 <vfs_del>
    db60:	eaffffdf 	b	dae4 <open+0x114>

0000db64 <close>:
    db64:	e1a0c00d 	mov	ip, sp
    db68:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
    db6c:	e24cb004 	sub	fp, ip, #4
    db70:	e24dd078 	sub	sp, sp, #120	; 0x78
    db74:	e24b4078 	sub	r4, fp, #120	; 0x78
    db78:	e1a01004 	mov	r1, r4
    db7c:	e1a06000 	mov	r6, r0
    db80:	ebffed54 	bl	90d8 <vfs_get_by_fd>
    db84:	e2507000 	subs	r7, r0, #0
    db88:	0a000002 	beq	db98 <close+0x34>
    db8c:	e24bd01c 	sub	sp, fp, #28
    db90:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    db94:	e12fff1e 	bx	lr
    db98:	ebfff82a 	bl	bc48 <get_proto_factor>
    db9c:	e24b5090 	sub	r5, fp, #144	; 0x90
    dba0:	e5903004 	ldr	r3, [r0, #4]
    dba4:	e1a00005 	mov	r0, r5
    dba8:	e1a0e00f 	mov	lr, pc
    dbac:	e12fff13 	bx	r3
    dbb0:	e1a01006 	mov	r1, r6
    dbb4:	e5903014 	ldr	r3, [r0, #20]
    dbb8:	e1a00005 	mov	r0, r5
    dbbc:	e1a0e00f 	mov	lr, pc
    dbc0:	e12fff13 	bx	r3
    dbc4:	e3e01000 	mvn	r1, #0
    dbc8:	e5903014 	ldr	r3, [r0, #20]
    dbcc:	e1a00005 	mov	r0, r5
    dbd0:	e1a0e00f 	mov	lr, pc
    dbd4:	e12fff13 	bx	r3
    dbd8:	e1a01004 	mov	r1, r4
    dbdc:	e5903010 	ldr	r3, [r0, #16]
    dbe0:	e3a0205c 	mov	r2, #92	; 0x5c
    dbe4:	e1a00005 	mov	r0, r5
    dbe8:	e1a0e00f 	mov	lr, pc
    dbec:	e12fff13 	bx	r3
    dbf0:	e1a02005 	mov	r2, r5
    dbf4:	e3a01003 	mov	r1, #3
    dbf8:	e1a03007 	mov	r3, r7
    dbfc:	e51b0024 	ldr	r0, [fp, #-36]	; 0x24
    dc00:	ebfff260 	bl	a588 <ipc_call>
    dc04:	ebfff80f 	bl	bc48 <get_proto_factor>
    dc08:	e590300c 	ldr	r3, [r0, #12]
    dc0c:	e1a00005 	mov	r0, r5
    dc10:	e1a0e00f 	mov	lr, pc
    dc14:	e12fff13 	bx	r3
    dc18:	e1a00006 	mov	r0, r6
    dc1c:	ebffeb01 	bl	8828 <vfs_close>
    dc20:	e24bd01c 	sub	sp, fp, #28
    dc24:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
    dc28:	e12fff1e 	bx	lr

0000dc2c <fcntl>:
    dc2c:	e1a0c00d 	mov	ip, sp
    dc30:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    dc34:	e24cb004 	sub	fp, ip, #4
    dc38:	e24dd034 	sub	sp, sp, #52	; 0x34
    dc3c:	e1a06001 	mov	r6, r1
    dc40:	e1a08002 	mov	r8, r2
    dc44:	e1a07000 	mov	r7, r0
    dc48:	ebfff7fe 	bl	bc48 <get_proto_factor>
    dc4c:	e24b4054 	sub	r4, fp, #84	; 0x54
    dc50:	e5903004 	ldr	r3, [r0, #4]
    dc54:	e1a00004 	mov	r0, r4
    dc58:	e1a0e00f 	mov	lr, pc
    dc5c:	e12fff13 	bx	r3
    dc60:	ebfff7f8 	bl	bc48 <get_proto_factor>
    dc64:	e24b503c 	sub	r5, fp, #60	; 0x3c
    dc68:	e5903004 	ldr	r3, [r0, #4]
    dc6c:	e1a00005 	mov	r0, r5
    dc70:	e1a0e00f 	mov	lr, pc
    dc74:	e12fff13 	bx	r3
    dc78:	e3560000 	cmp	r6, #0
    dc7c:	0a000025 	beq	dd18 <fcntl+0xec>
    dc80:	e3560001 	cmp	r6, #1
    dc84:	1a000038 	bne	dd6c <fcntl+0x140>
    dc88:	ebfff7ee 	bl	bc48 <get_proto_factor>
    dc8c:	e1a01007 	mov	r1, r7
    dc90:	e5903014 	ldr	r3, [r0, #20]
    dc94:	e1a00004 	mov	r0, r4
    dc98:	e1a0e00f 	mov	lr, pc
    dc9c:	e12fff13 	bx	r3
    dca0:	e1a01008 	mov	r1, r8
    dca4:	e5903014 	ldr	r3, [r0, #20]
    dca8:	e1a00004 	mov	r0, r4
    dcac:	e1a0e00f 	mov	lr, pc
    dcb0:	e12fff13 	bx	r3
    dcb4:	ebfffb04 	bl	c8cc <get_vfsd_pid>
    dcb8:	e3a01014 	mov	r1, #20
    dcbc:	e1a02004 	mov	r2, r4
    dcc0:	e1a03005 	mov	r3, r5
    dcc4:	ebfff22f 	bl	a588 <ipc_call>
    dcc8:	e3500000 	cmp	r0, #0
    dccc:	1a000026 	bne	dd6c <fcntl+0x140>
    dcd0:	e1a00005 	mov	r0, r5
    dcd4:	ebfff8a4 	bl	bf6c <proto_read_int>
    dcd8:	e2906000 	adds	r6, r0, #0
    dcdc:	13e06000 	mvnne	r6, #0
    dce0:	ebfff7d8 	bl	bc48 <get_proto_factor>
    dce4:	e590300c 	ldr	r3, [r0, #12]
    dce8:	e1a00005 	mov	r0, r5
    dcec:	e1a0e00f 	mov	lr, pc
    dcf0:	e12fff13 	bx	r3
    dcf4:	ebfff7d3 	bl	bc48 <get_proto_factor>
    dcf8:	e590300c 	ldr	r3, [r0, #12]
    dcfc:	e1a00004 	mov	r0, r4
    dd00:	e1a0e00f 	mov	lr, pc
    dd04:	e12fff13 	bx	r3
    dd08:	e1a00006 	mov	r0, r6
    dd0c:	e24bd020 	sub	sp, fp, #32
    dd10:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    dd14:	e12fff1e 	bx	lr
    dd18:	ebfff7ca 	bl	bc48 <get_proto_factor>
    dd1c:	e1a01007 	mov	r1, r7
    dd20:	e5903014 	ldr	r3, [r0, #20]
    dd24:	e1a00004 	mov	r0, r4
    dd28:	e1a0e00f 	mov	lr, pc
    dd2c:	e12fff13 	bx	r3
    dd30:	ebfffae5 	bl	c8cc <get_vfsd_pid>
    dd34:	e3a01013 	mov	r1, #19
    dd38:	e1a02004 	mov	r2, r4
    dd3c:	e1a03005 	mov	r3, r5
    dd40:	ebfff210 	bl	a588 <ipc_call>
    dd44:	e3500000 	cmp	r0, #0
    dd48:	1a000007 	bne	dd6c <fcntl+0x140>
    dd4c:	e1a00005 	mov	r0, r5
    dd50:	ebfff885 	bl	bf6c <proto_read_int>
    dd54:	e3500000 	cmp	r0, #0
    dd58:	1a000003 	bne	dd6c <fcntl+0x140>
    dd5c:	e1a00005 	mov	r0, r5
    dd60:	ebfff881 	bl	bf6c <proto_read_int>
    dd64:	e1a06000 	mov	r6, r0
    dd68:	eaffffdc 	b	dce0 <fcntl+0xb4>
    dd6c:	e3e06000 	mvn	r6, #0
    dd70:	eaffffda 	b	dce0 <fcntl+0xb4>

0000dd74 <get_cored_pid>:
    dd74:	e1a0c00d 	mov	ip, sp
    dd78:	e3a0002c 	mov	r0, #44	; 0x2c
    dd7c:	e92dd800 	push	{fp, ip, lr, pc}
    dd80:	e24cb004 	sub	fp, ip, #4
    dd84:	ebffe773 	bl	7b58 <syscall0>
    dd88:	e24bd00c 	sub	sp, fp, #12
    dd8c:	e89d6800 	ldm	sp, {fp, sp, lr}
    dd90:	e12fff1e 	bx	lr

0000dd94 <write>:
    dd94:	e1a0c00d 	mov	ip, sp
    dd98:	e92dd9f0 	push	{r4, r5, r6, r7, r8, fp, ip, lr, pc}
    dd9c:	e1a05001 	mov	r5, r1
    dda0:	e3a01000 	mov	r1, #0
    dda4:	e24cb004 	sub	fp, ip, #4
    dda8:	e24dd064 	sub	sp, sp, #100	; 0x64
    ddac:	e1a04002 	mov	r4, r2
    ddb0:	e1a02001 	mov	r2, r1
    ddb4:	e1a06000 	mov	r6, r0
    ddb8:	ebffff9b 	bl	dc2c <fcntl>
    ddbc:	e59f212c 	ldr	r2, [pc, #300]	; def0 <write+0x15c>
    ddc0:	e3700001 	cmn	r0, #1
    ddc4:	e08f2002 	add	r2, pc, r2
    ddc8:	0a000013 	beq	de1c <write+0x88>
    ddcc:	e2103008 	ands	r3, r0, #8
    ddd0:	0a000014 	beq	de28 <write+0x94>
    ddd4:	e3a0c000 	mov	ip, #0
    ddd8:	e59f3114 	ldr	r3, [pc, #276]	; def4 <write+0x160>
    dddc:	e24b7080 	sub	r7, fp, #128	; 0x80
    dde0:	e7923003 	ldr	r3, [r2, r3]
    dde4:	e1a01007 	mov	r1, r7
    dde8:	e1a00006 	mov	r0, r6
    ddec:	e583c000 	str	ip, [r3]
    ddf0:	ebffecb8 	bl	90d8 <vfs_get_by_fd>
    ddf4:	e2503000 	subs	r3, r0, #0
    ddf8:	1a00003a 	bne	dee8 <write+0x154>
    ddfc:	e51b203c 	ldr	r2, [fp, #-60]	; 0x3c
    de00:	e3520003 	cmp	r2, #3
    de04:	0a000030 	beq	decc <write+0x138>
    de08:	e1a00006 	mov	r0, r6
    de0c:	e1a01007 	mov	r1, r7
    de10:	e1a02005 	mov	r2, r5
    de14:	e1a03004 	mov	r3, r4
    de18:	ebfff100 	bl	a220 <vfs_write>
    de1c:	e24bd020 	sub	sp, fp, #32
    de20:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    de24:	e12fff1e 	bx	lr
    de28:	e59f10c4 	ldr	r1, [pc, #196]	; def4 <write+0x160>
    de2c:	e24b7080 	sub	r7, fp, #128	; 0x80
    de30:	e7928001 	ldr	r8, [r2, r1]
    de34:	e1a00006 	mov	r0, r6
    de38:	e1a01007 	mov	r1, r7
    de3c:	e5883000 	str	r3, [r8]
    de40:	ebffeca4 	bl	90d8 <vfs_get_by_fd>
    de44:	e3500000 	cmp	r0, #0
    de48:	1a000026 	bne	dee8 <write+0x154>
    de4c:	e51b303c 	ldr	r3, [fp, #-60]	; 0x3c
    de50:	e3530003 	cmp	r3, #3
    de54:	1a000005 	bne	de70 <write+0xdc>
    de58:	ea000013 	b	deac <write+0x118>
    de5c:	e5983000 	ldr	r3, [r8]
    de60:	e3530001 	cmp	r3, #1
    de64:	1affffec 	bne	de1c <write+0x88>
    de68:	e3a00000 	mov	r0, #0
    de6c:	ebfffc23 	bl	cf00 <sleep>
    de70:	e1a00006 	mov	r0, r6
    de74:	e1a01007 	mov	r1, r7
    de78:	e1a02005 	mov	r2, r5
    de7c:	e1a03004 	mov	r3, r4
    de80:	ebfff0e6 	bl	a220 <vfs_write>
    de84:	e3500000 	cmp	r0, #0
    de88:	bafffff3 	blt	de5c <write+0xc8>
    de8c:	e24bd020 	sub	sp, fp, #32
    de90:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    de94:	e12fff1e 	bx	lr
    de98:	e5983000 	ldr	r3, [r8]
    de9c:	e3530001 	cmp	r3, #1
    dea0:	1affffdd 	bne	de1c <write+0x88>
    dea4:	e3a00000 	mov	r0, #0
    dea8:	ebfffc14 	bl	cf00 <sleep>
    deac:	e1a00007 	mov	r0, r7
    deb0:	e1a01005 	mov	r1, r5
    deb4:	e1a02004 	mov	r2, r4
    deb8:	e3a03001 	mov	r3, #1
    debc:	ebfff07f 	bl	a0c0 <vfs_write_pipe>
    dec0:	e3500000 	cmp	r0, #0
    dec4:	bafffff3 	blt	de98 <write+0x104>
    dec8:	eaffffd3 	b	de1c <write+0x88>
    decc:	e1a00007 	mov	r0, r7
    ded0:	e1a01005 	mov	r1, r5
    ded4:	e1a02004 	mov	r2, r4
    ded8:	ebfff078 	bl	a0c0 <vfs_write_pipe>
    dedc:	e24bd020 	sub	sp, fp, #32
    dee0:	e89d69f0 	ldm	sp, {r4, r5, r6, r7, r8, fp, sp, lr}
    dee4:	e12fff1e 	bx	lr
    dee8:	e3e00000 	mvn	r0, #0
    deec:	eaffffca 	b	de1c <write+0x88>
    def0:	00009c34 	andeq	r9, r0, r4, lsr ip
    def4:	00000018 	andeq	r0, r0, r8, lsl r0

0000def8 <memcpy>:
    def8:	e352000f 	cmp	r2, #15
    defc:	e92d4070 	push	{r4, r5, r6, lr}
    df00:	9a000029 	bls	dfac <memcpy+0xb4>
    df04:	e1803001 	orr	r3, r0, r1
    df08:	e3130003 	tst	r3, #3
    df0c:	1a000031 	bne	dfd8 <memcpy+0xe0>
    df10:	e1a0e002 	mov	lr, r2
    df14:	e280c010 	add	ip, r0, #16
    df18:	e2813010 	add	r3, r1, #16
    df1c:	e5134010 	ldr	r4, [r3, #-16]
    df20:	e50c4010 	str	r4, [ip, #-16]
    df24:	e513400c 	ldr	r4, [r3, #-12]
    df28:	e50c400c 	str	r4, [ip, #-12]
    df2c:	e5134008 	ldr	r4, [r3, #-8]
    df30:	e50c4008 	str	r4, [ip, #-8]
    df34:	e24ee010 	sub	lr, lr, #16
    df38:	e5134004 	ldr	r4, [r3, #-4]
    df3c:	e35e000f 	cmp	lr, #15
    df40:	e50c4004 	str	r4, [ip, #-4]
    df44:	e2833010 	add	r3, r3, #16
    df48:	e28cc010 	add	ip, ip, #16
    df4c:	8afffff2 	bhi	df1c <memcpy+0x24>
    df50:	e2423010 	sub	r3, r2, #16
    df54:	e3c3300f 	bic	r3, r3, #15
    df58:	e202600f 	and	r6, r2, #15
    df5c:	e2833010 	add	r3, r3, #16
    df60:	e3560003 	cmp	r6, #3
    df64:	e0811003 	add	r1, r1, r3
    df68:	e0803003 	add	r3, r0, r3
    df6c:	9a00001b 	bls	dfe0 <memcpy+0xe8>
    df70:	e1a04001 	mov	r4, r1
    df74:	e1a0c006 	mov	ip, r6
    df78:	e243e004 	sub	lr, r3, #4
    df7c:	e24cc004 	sub	ip, ip, #4
    df80:	e4945004 	ldr	r5, [r4], #4
    df84:	e35c0003 	cmp	ip, #3
    df88:	e5ae5004 	str	r5, [lr, #4]!
    df8c:	8afffffa 	bhi	df7c <memcpy+0x84>
    df90:	e246c004 	sub	ip, r6, #4
    df94:	e3ccc003 	bic	ip, ip, #3
    df98:	e28cc004 	add	ip, ip, #4
    df9c:	e083300c 	add	r3, r3, ip
    dfa0:	e081100c 	add	r1, r1, ip
    dfa4:	e2022003 	and	r2, r2, #3
    dfa8:	ea000000 	b	dfb0 <memcpy+0xb8>
    dfac:	e1a03000 	mov	r3, r0
    dfb0:	e3520000 	cmp	r2, #0
    dfb4:	0a000005 	beq	dfd0 <memcpy+0xd8>
    dfb8:	e2433001 	sub	r3, r3, #1
    dfbc:	e0812002 	add	r2, r1, r2
    dfc0:	e4d1c001 	ldrb	ip, [r1], #1
    dfc4:	e1510002 	cmp	r1, r2
    dfc8:	e5e3c001 	strb	ip, [r3, #1]!
    dfcc:	1afffffb 	bne	dfc0 <memcpy+0xc8>
    dfd0:	e8bd4070 	pop	{r4, r5, r6, lr}
    dfd4:	e12fff1e 	bx	lr
    dfd8:	e1a03000 	mov	r3, r0
    dfdc:	eafffff5 	b	dfb8 <memcpy+0xc0>
    dfe0:	e1a02006 	mov	r2, r6
    dfe4:	eafffff1 	b	dfb0 <memcpy+0xb8>

0000dfe8 <memset>:
    dfe8:	e3100003 	tst	r0, #3
    dfec:	e92d4010 	push	{r4, lr}
    dff0:	0a000037 	beq	e0d4 <memset+0xec>
    dff4:	e3520000 	cmp	r2, #0
    dff8:	e2422001 	sub	r2, r2, #1
    dffc:	0a000032 	beq	e0cc <memset+0xe4>
    e000:	e201c0ff 	and	ip, r1, #255	; 0xff
    e004:	e1a03000 	mov	r3, r0
    e008:	ea000002 	b	e018 <memset+0x30>
    e00c:	e3520000 	cmp	r2, #0
    e010:	e2422001 	sub	r2, r2, #1
    e014:	0a00002c 	beq	e0cc <memset+0xe4>
    e018:	e4c3c001 	strb	ip, [r3], #1
    e01c:	e3130003 	tst	r3, #3
    e020:	1afffff9 	bne	e00c <memset+0x24>
    e024:	e3520003 	cmp	r2, #3
    e028:	9a000020 	bls	e0b0 <memset+0xc8>
    e02c:	e201e0ff 	and	lr, r1, #255	; 0xff
    e030:	e18ee40e 	orr	lr, lr, lr, lsl #8
    e034:	e352000f 	cmp	r2, #15
    e038:	e18ee80e 	orr	lr, lr, lr, lsl #16
    e03c:	9a000010 	bls	e084 <memset+0x9c>
    e040:	e1a04002 	mov	r4, r2
    e044:	e283c010 	add	ip, r3, #16
    e048:	e2444010 	sub	r4, r4, #16
    e04c:	e354000f 	cmp	r4, #15
    e050:	e50ce010 	str	lr, [ip, #-16]
    e054:	e50ce00c 	str	lr, [ip, #-12]
    e058:	e50ce008 	str	lr, [ip, #-8]
    e05c:	e50ce004 	str	lr, [ip, #-4]
    e060:	e28cc010 	add	ip, ip, #16
    e064:	8afffff7 	bhi	e048 <memset+0x60>
    e068:	e242c010 	sub	ip, r2, #16
    e06c:	e3ccc00f 	bic	ip, ip, #15
    e070:	e202200f 	and	r2, r2, #15
    e074:	e28cc010 	add	ip, ip, #16
    e078:	e3520003 	cmp	r2, #3
    e07c:	e083300c 	add	r3, r3, ip
    e080:	9a00000a 	bls	e0b0 <memset+0xc8>
    e084:	e1a04003 	mov	r4, r3
    e088:	e1a0c002 	mov	ip, r2
    e08c:	e24cc004 	sub	ip, ip, #4
    e090:	e35c0003 	cmp	ip, #3
    e094:	e484e004 	str	lr, [r4], #4
    e098:	8afffffb 	bhi	e08c <memset+0xa4>
    e09c:	e242c004 	sub	ip, r2, #4
    e0a0:	e3ccc003 	bic	ip, ip, #3
    e0a4:	e28cc004 	add	ip, ip, #4
    e0a8:	e083300c 	add	r3, r3, ip
    e0ac:	e2022003 	and	r2, r2, #3
    e0b0:	e3520000 	cmp	r2, #0
    e0b4:	0a000004 	beq	e0cc <memset+0xe4>
    e0b8:	e20110ff 	and	r1, r1, #255	; 0xff
    e0bc:	e0832002 	add	r2, r3, r2
    e0c0:	e4c31001 	strb	r1, [r3], #1
    e0c4:	e1530002 	cmp	r3, r2
    e0c8:	1afffffc 	bne	e0c0 <memset+0xd8>
    e0cc:	e8bd4010 	pop	{r4, lr}
    e0d0:	e12fff1e 	bx	lr
    e0d4:	e1a03000 	mov	r3, r0
    e0d8:	eaffffd1 	b	e024 <memset+0x3c>

0000e0dc <strchr>:
    e0dc:	e21110ff 	ands	r1, r1, #255	; 0xff
    e0e0:	e92d4010 	push	{r4, lr}
    e0e4:	0a00002c 	beq	e19c <strchr+0xc0>
    e0e8:	e3100003 	tst	r0, #3
    e0ec:	0a00000e 	beq	e12c <strchr+0x50>
    e0f0:	e5d03000 	ldrb	r3, [r0]
    e0f4:	e3530000 	cmp	r3, #0
    e0f8:	0a00004a 	beq	e228 <strchr+0x14c>
    e0fc:	e1510003 	cmp	r1, r3
    e100:	12803001 	addne	r3, r0, #1
    e104:	1a000005 	bne	e120 <strchr+0x44>
    e108:	ea000021 	b	e194 <strchr+0xb8>
    e10c:	e4d32001 	ldrb	r2, [r3], #1
    e110:	e3520000 	cmp	r2, #0
    e114:	0a000040 	beq	e21c <strchr+0x140>
    e118:	e1510002 	cmp	r1, r2
    e11c:	0a00001c 	beq	e194 <strchr+0xb8>
    e120:	e3130003 	tst	r3, #3
    e124:	e1a00003 	mov	r0, r3
    e128:	1afffff7 	bne	e10c <strchr+0x30>
    e12c:	e1814401 	orr	r4, r1, r1, lsl #8
    e130:	e590c000 	ldr	ip, [r0]
    e134:	e1844804 	orr	r4, r4, r4, lsl #16
    e138:	ea000000 	b	e140 <strchr+0x64>
    e13c:	e5b0c004 	ldr	ip, [r0, #4]!
    e140:	e59f20ec 	ldr	r2, [pc, #236]	; e234 <strchr+0x158>
    e144:	e1a03002 	mov	r3, r2
    e148:	e02ce004 	eor	lr, ip, r4
    e14c:	e08c3003 	add	r3, ip, r3
    e150:	e08e2002 	add	r2, lr, r2
    e154:	e1c3300c 	bic	r3, r3, ip
    e158:	e1c2200e 	bic	r2, r2, lr
    e15c:	e59fc0d4 	ldr	ip, [pc, #212]	; e238 <strchr+0x15c>
    e160:	e1823003 	orr	r3, r2, r3
    e164:	e00cc003 	and	ip, ip, r3
    e168:	e35c0000 	cmp	ip, #0
    e16c:	0afffff2 	beq	e13c <strchr+0x60>
    e170:	e5d03000 	ldrb	r3, [r0]
    e174:	e3530000 	cmp	r3, #0
    e178:	1a000003 	bne	e18c <strchr+0xb0>
    e17c:	ea000029 	b	e228 <strchr+0x14c>
    e180:	e5f03001 	ldrb	r3, [r0, #1]!
    e184:	e3530000 	cmp	r3, #0
    e188:	0a000026 	beq	e228 <strchr+0x14c>
    e18c:	e1510003 	cmp	r1, r3
    e190:	1afffffa 	bne	e180 <strchr+0xa4>
    e194:	e8bd4010 	pop	{r4, lr}
    e198:	e12fff1e 	bx	lr
    e19c:	e3100003 	tst	r0, #3
    e1a0:	0a00000b 	beq	e1d4 <strchr+0xf8>
    e1a4:	e5d03000 	ldrb	r3, [r0]
    e1a8:	e3530000 	cmp	r3, #0
    e1ac:	12803001 	addne	r3, r0, #1
    e1b0:	1a000004 	bne	e1c8 <strchr+0xec>
    e1b4:	eafffff6 	b	e194 <strchr+0xb8>
    e1b8:	e5d02000 	ldrb	r2, [r0]
    e1bc:	e3520000 	cmp	r2, #0
    e1c0:	e2833001 	add	r3, r3, #1
    e1c4:	0afffff2 	beq	e194 <strchr+0xb8>
    e1c8:	e3130003 	tst	r3, #3
    e1cc:	e1a00003 	mov	r0, r3
    e1d0:	1afffff8 	bne	e1b8 <strchr+0xdc>
    e1d4:	e5901000 	ldr	r1, [r0]
    e1d8:	ea000000 	b	e1e0 <strchr+0x104>
    e1dc:	e5b01004 	ldr	r1, [r0, #4]!
    e1e0:	e59f304c 	ldr	r3, [pc, #76]	; e234 <strchr+0x158>
    e1e4:	e59f204c 	ldr	r2, [pc, #76]	; e238 <strchr+0x15c>
    e1e8:	e0813003 	add	r3, r1, r3
    e1ec:	e1c33001 	bic	r3, r3, r1
    e1f0:	e0022003 	and	r2, r2, r3
    e1f4:	e3520000 	cmp	r2, #0
    e1f8:	0afffff7 	beq	e1dc <strchr+0x100>
    e1fc:	e5d03000 	ldrb	r3, [r0]
    e200:	e3530000 	cmp	r3, #0
    e204:	0affffe2 	beq	e194 <strchr+0xb8>
    e208:	e5f03001 	ldrb	r3, [r0, #1]!
    e20c:	e3530000 	cmp	r3, #0
    e210:	1afffffc 	bne	e208 <strchr+0x12c>
    e214:	e8bd4010 	pop	{r4, lr}
    e218:	e12fff1e 	bx	lr
    e21c:	e1a00002 	mov	r0, r2
    e220:	e8bd4010 	pop	{r4, lr}
    e224:	e12fff1e 	bx	lr
    e228:	e1a00003 	mov	r0, r3
    e22c:	e8bd4010 	pop	{r4, lr}
    e230:	e12fff1e 	bx	lr
    e234:	fefefeff 	mrc2	14, 7, pc, cr14, cr15, {7}
    e238:	80808080 	addhi	r8, r0, r0, lsl #1

0000e23c <strcmp>:
    e23c:	e020c001 	eor	ip, r0, r1
    e240:	e31c0003 	tst	ip, #3
    e244:	1a000021 	bne	e2d0 <strcmp+0x94>
    e248:	e210c003 	ands	ip, r0, #3
    e24c:	e3c00003 	bic	r0, r0, #3
    e250:	e3c11003 	bic	r1, r1, #3
    e254:	e4902004 	ldr	r2, [r0], #4
    e258:	04913004 	ldreq	r3, [r1], #4
    e25c:	0a000006 	beq	e27c <strcmp+0x40>
    e260:	e22cc003 	eor	ip, ip, #3
    e264:	e3e034ff 	mvn	r3, #-16777216	; 0xff000000
    e268:	e1a0c18c 	lsl	ip, ip, #3
    e26c:	e1a0cc33 	lsr	ip, r3, ip
    e270:	e4913004 	ldr	r3, [r1], #4
    e274:	e182200c 	orr	r2, r2, ip
    e278:	e183300c 	orr	r3, r3, ip
    e27c:	e52d4004 	push	{r4}		; (str r4, [sp, #-4]!)
    e280:	e3a04001 	mov	r4, #1
    e284:	e1844404 	orr	r4, r4, r4, lsl #8
    e288:	e1844804 	orr	r4, r4, r4, lsl #16
    e28c:	e042c004 	sub	ip, r2, r4
    e290:	e1520003 	cmp	r2, r3
    e294:	01ccc002 	biceq	ip, ip, r2
    e298:	011c0384 	tsteq	ip, r4, lsl #7
    e29c:	04902004 	ldreq	r2, [r0], #4
    e2a0:	04913004 	ldreq	r3, [r1], #4
    e2a4:	0afffff8 	beq	e28c <strcmp+0x50>
    e2a8:	e1a00c02 	lsl	r0, r2, #24
    e2ac:	e1a02422 	lsr	r2, r2, #8
    e2b0:	e3500001 	cmp	r0, #1
    e2b4:	21500c03 	cmpcs	r0, r3, lsl #24
    e2b8:	01a03423 	lsreq	r3, r3, #8
    e2bc:	0afffff9 	beq	e2a8 <strcmp+0x6c>
    e2c0:	e20330ff 	and	r3, r3, #255	; 0xff
    e2c4:	e0630c20 	rsb	r0, r3, r0, lsr #24
    e2c8:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
    e2cc:	e12fff1e 	bx	lr
    e2d0:	e3100003 	tst	r0, #3
    e2d4:	0a000006 	beq	e2f4 <strcmp+0xb8>
    e2d8:	e4d02001 	ldrb	r2, [r0], #1
    e2dc:	e4d13001 	ldrb	r3, [r1], #1
    e2e0:	e3520001 	cmp	r2, #1
    e2e4:	21520003 	cmpcs	r2, r3
    e2e8:	0afffff8 	beq	e2d0 <strcmp+0x94>
    e2ec:	e0420003 	sub	r0, r2, r3
    e2f0:	e12fff1e 	bx	lr
    e2f4:	e92d0030 	push	{r4, r5}
    e2f8:	e3a04001 	mov	r4, #1
    e2fc:	e1844404 	orr	r4, r4, r4, lsl #8
    e300:	e1844804 	orr	r4, r4, r4, lsl #16
    e304:	e4902004 	ldr	r2, [r0], #4
    e308:	e2015003 	and	r5, r1, #3
    e30c:	e3c11003 	bic	r1, r1, #3
    e310:	e4913004 	ldr	r3, [r1], #4
    e314:	e3550002 	cmp	r5, #2
    e318:	0a000017 	beq	e37c <strcmp+0x140>
    e31c:	8a00002d 	bhi	e3d8 <strcmp+0x19c>
    e320:	e3c254ff 	bic	r5, r2, #-16777216	; 0xff000000
    e324:	e1550423 	cmp	r5, r3, lsr #8
    e328:	e042c004 	sub	ip, r2, r4
    e32c:	e1ccc002 	bic	ip, ip, r2
    e330:	1a000007 	bne	e354 <strcmp+0x118>
    e334:	e01cc384 	ands	ip, ip, r4, lsl #7
    e338:	04913004 	ldreq	r3, [r1], #4
    e33c:	1a000006 	bne	e35c <strcmp+0x120>
    e340:	e0255002 	eor	r5, r5, r2
    e344:	e1550c03 	cmp	r5, r3, lsl #24
    e348:	1a000008 	bne	e370 <strcmp+0x134>
    e34c:	e4902004 	ldr	r2, [r0], #4
    e350:	eafffff2 	b	e320 <strcmp+0xe4>
    e354:	e1a03423 	lsr	r3, r3, #8
    e358:	ea000036 	b	e438 <strcmp+0x1fc>
    e35c:	e3dcc4ff 	bics	ip, ip, #-16777216	; 0xff000000
    e360:	1a000031 	bne	e42c <strcmp+0x1f0>
    e364:	e5d13000 	ldrb	r3, [r1]
    e368:	e1a05c22 	lsr	r5, r2, #24
    e36c:	ea000031 	b	e438 <strcmp+0x1fc>
    e370:	e1a05c22 	lsr	r5, r2, #24
    e374:	e20330ff 	and	r3, r3, #255	; 0xff
    e378:	ea00002e 	b	e438 <strcmp+0x1fc>
    e37c:	e1a05802 	lsl	r5, r2, #16
    e380:	e042c004 	sub	ip, r2, r4
    e384:	e1a05825 	lsr	r5, r5, #16
    e388:	e1ccc002 	bic	ip, ip, r2
    e38c:	e1550823 	cmp	r5, r3, lsr #16
    e390:	1a00000e 	bne	e3d0 <strcmp+0x194>
    e394:	e01cc384 	ands	ip, ip, r4, lsl #7
    e398:	04913004 	ldreq	r3, [r1], #4
    e39c:	1a000004 	bne	e3b4 <strcmp+0x178>
    e3a0:	e0255002 	eor	r5, r5, r2
    e3a4:	e1550803 	cmp	r5, r3, lsl #16
    e3a8:	1a000006 	bne	e3c8 <strcmp+0x18c>
    e3ac:	e4902004 	ldr	r2, [r0], #4
    e3b0:	eafffff1 	b	e37c <strcmp+0x140>
    e3b4:	e1b0c80c 	lsls	ip, ip, #16
    e3b8:	1a00001b 	bne	e42c <strcmp+0x1f0>
    e3bc:	e1d130b0 	ldrh	r3, [r1]
    e3c0:	e1a05822 	lsr	r5, r2, #16
    e3c4:	ea00001b 	b	e438 <strcmp+0x1fc>
    e3c8:	e1a03803 	lsl	r3, r3, #16
    e3cc:	e1a05822 	lsr	r5, r2, #16
    e3d0:	e1a03823 	lsr	r3, r3, #16
    e3d4:	ea000017 	b	e438 <strcmp+0x1fc>
    e3d8:	e20250ff 	and	r5, r2, #255	; 0xff
    e3dc:	e1550c23 	cmp	r5, r3, lsr #24
    e3e0:	e042c004 	sub	ip, r2, r4
    e3e4:	e1ccc002 	bic	ip, ip, r2
    e3e8:	1a000007 	bne	e40c <strcmp+0x1d0>
    e3ec:	e01cc384 	ands	ip, ip, r4, lsl #7
    e3f0:	04913004 	ldreq	r3, [r1], #4
    e3f4:	1a000006 	bne	e414 <strcmp+0x1d8>
    e3f8:	e0255002 	eor	r5, r5, r2
    e3fc:	e1550403 	cmp	r5, r3, lsl #8
    e400:	1a000006 	bne	e420 <strcmp+0x1e4>
    e404:	e4902004 	ldr	r2, [r0], #4
    e408:	eafffff2 	b	e3d8 <strcmp+0x19c>
    e40c:	e1a03c23 	lsr	r3, r3, #24
    e410:	ea000008 	b	e438 <strcmp+0x1fc>
    e414:	e31200ff 	tst	r2, #255	; 0xff
    e418:	0a000003 	beq	e42c <strcmp+0x1f0>
    e41c:	e4913004 	ldr	r3, [r1], #4
    e420:	e1a05422 	lsr	r5, r2, #8
    e424:	e3c334ff 	bic	r3, r3, #-16777216	; 0xff000000
    e428:	ea000002 	b	e438 <strcmp+0x1fc>
    e42c:	e3a00000 	mov	r0, #0
    e430:	e8bd0030 	pop	{r4, r5}
    e434:	e12fff1e 	bx	lr
    e438:	e20520ff 	and	r2, r5, #255	; 0xff
    e43c:	e20300ff 	and	r0, r3, #255	; 0xff
    e440:	e3500001 	cmp	r0, #1
    e444:	21500002 	cmpcs	r0, r2
    e448:	01a05425 	lsreq	r5, r5, #8
    e44c:	01a03423 	lsreq	r3, r3, #8
    e450:	0afffff8 	beq	e438 <strcmp+0x1fc>
    e454:	e0420000 	sub	r0, r2, r0
    e458:	e8bd0030 	pop	{r4, r5}
    e45c:	e12fff1e 	bx	lr

0000e460 <strcpy>:
    e460:	e0202001 	eor	r2, r0, r1
    e464:	e1a0c000 	mov	ip, r0
    e468:	e3120003 	tst	r2, #3
    e46c:	1a000032 	bne	e53c <strcpy+0xdc>
    e470:	e3110003 	tst	r1, #3
    e474:	1a000021 	bne	e500 <strcpy+0xa0>
    e478:	e52d5004 	push	{r5}		; (str r5, [sp, #-4]!)
    e47c:	e3a05001 	mov	r5, #1
    e480:	e1855405 	orr	r5, r5, r5, lsl #8
    e484:	e1855805 	orr	r5, r5, r5, lsl #16
    e488:	e52d4004 	push	{r4}		; (str r4, [sp, #-4]!)
    e48c:	e3110004 	tst	r1, #4
    e490:	e4913004 	ldr	r3, [r1], #4
    e494:	0a000005 	beq	e4b0 <strcpy+0x50>
    e498:	e0432005 	sub	r2, r3, r5
    e49c:	e1d22003 	bics	r2, r2, r3
    e4a0:	e1120385 	tst	r2, r5, lsl #7
    e4a4:	048c3004 	streq	r3, [ip], #4
    e4a8:	04913004 	ldreq	r3, [r1], #4
    e4ac:	1a00000c 	bne	e4e4 <strcpy+0x84>
    e4b0:	e4914004 	ldr	r4, [r1], #4
    e4b4:	e0432005 	sub	r2, r3, r5
    e4b8:	e1d22003 	bics	r2, r2, r3
    e4bc:	e1120385 	tst	r2, r5, lsl #7
    e4c0:	e0442005 	sub	r2, r4, r5
    e4c4:	1a000006 	bne	e4e4 <strcpy+0x84>
    e4c8:	e48c3004 	str	r3, [ip], #4
    e4cc:	e1d22004 	bics	r2, r2, r4
    e4d0:	e1120385 	tst	r2, r5, lsl #7
    e4d4:	04913004 	ldreq	r3, [r1], #4
    e4d8:	048c4004 	streq	r4, [ip], #4
    e4dc:	0afffff3 	beq	e4b0 <strcpy+0x50>
    e4e0:	e1a03004 	mov	r3, r4
    e4e4:	e4cc3001 	strb	r3, [ip], #1
    e4e8:	e31300ff 	tst	r3, #255	; 0xff
    e4ec:	e1a03463 	ror	r3, r3, #8
    e4f0:	1afffffb 	bne	e4e4 <strcpy+0x84>
    e4f4:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
    e4f8:	e49d5004 	pop	{r5}		; (ldr r5, [sp], #4)
    e4fc:	e12fff1e 	bx	lr
    e500:	e3110001 	tst	r1, #1
    e504:	0a000003 	beq	e518 <strcpy+0xb8>
    e508:	e4d12001 	ldrb	r2, [r1], #1
    e50c:	e4cc2001 	strb	r2, [ip], #1
    e510:	e3520000 	cmp	r2, #0
    e514:	012fff1e 	bxeq	lr
    e518:	e3110002 	tst	r1, #2
    e51c:	0affffd5 	beq	e478 <strcpy+0x18>
    e520:	e0d120b2 	ldrh	r2, [r1], #2
    e524:	e31200ff 	tst	r2, #255	; 0xff
    e528:	10cc20b2 	strhne	r2, [ip], #2
    e52c:	05cc2000 	strbeq	r2, [ip]
    e530:	13120cff 	tstne	r2, #65280	; 0xff00
    e534:	1affffcf 	bne	e478 <strcpy+0x18>
    e538:	e12fff1e 	bx	lr
    e53c:	e4d12001 	ldrb	r2, [r1], #1
    e540:	e4cc2001 	strb	r2, [ip], #1
    e544:	e3520000 	cmp	r2, #0
    e548:	1afffffb 	bne	e53c <strcpy+0xdc>
    e54c:	e12fff1e 	bx	lr

0000e550 <strlen>:
    e550:	e3c01003 	bic	r1, r0, #3
    e554:	e2100003 	ands	r0, r0, #3
    e558:	e2600000 	rsb	r0, r0, #0
    e55c:	e4913004 	ldr	r3, [r1], #4
    e560:	e280c004 	add	ip, r0, #4
    e564:	e1a0c18c 	lsl	ip, ip, #3
    e568:	e3e02000 	mvn	r2, #0
    e56c:	11833c32 	orrne	r3, r3, r2, lsr ip
    e570:	e3a0c001 	mov	ip, #1
    e574:	e18cc40c 	orr	ip, ip, ip, lsl #8
    e578:	e18cc80c 	orr	ip, ip, ip, lsl #16
    e57c:	e043200c 	sub	r2, r3, ip
    e580:	e1c22003 	bic	r2, r2, r3
    e584:	e012238c 	ands	r2, r2, ip, lsl #7
    e588:	04913004 	ldreq	r3, [r1], #4
    e58c:	02800004 	addeq	r0, r0, #4
    e590:	0afffff9 	beq	e57c <strlen+0x2c>
    e594:	e31300ff 	tst	r3, #255	; 0xff
    e598:	12800001 	addne	r0, r0, #1
    e59c:	13130cff 	tstne	r3, #65280	; 0xff00
    e5a0:	12800001 	addne	r0, r0, #1
    e5a4:	131308ff 	tstne	r3, #16711680	; 0xff0000
    e5a8:	12800001 	addne	r0, r0, #1
    e5ac:	e12fff1e 	bx	lr

0000e5b0 <strncmp>:
    e5b0:	e3520000 	cmp	r2, #0
    e5b4:	0a000038 	beq	e69c <strncmp+0xec>
    e5b8:	e1803001 	orr	r3, r0, r1
    e5bc:	e2133003 	ands	r3, r3, #3
    e5c0:	e92d4070 	push	{r4, r5, r6, lr}
    e5c4:	1a000023 	bne	e658 <strncmp+0xa8>
    e5c8:	e3520003 	cmp	r2, #3
    e5cc:	9a000021 	bls	e658 <strncmp+0xa8>
    e5d0:	e590e000 	ldr	lr, [r0]
    e5d4:	e591c000 	ldr	ip, [r1]
    e5d8:	e15e000c 	cmp	lr, ip
    e5dc:	1a00001d 	bne	e658 <strncmp+0xa8>
    e5e0:	e2522004 	subs	r2, r2, #4
    e5e4:	0a000031 	beq	e6b0 <strncmp+0x100>
    e5e8:	e59fc0d0 	ldr	ip, [pc, #208]	; e6c0 <strncmp+0x110>
    e5ec:	e59f40d0 	ldr	r4, [pc, #208]	; e6c4 <strncmp+0x114>
    e5f0:	e08ec00c 	add	ip, lr, ip
    e5f4:	e1ccc00e 	bic	ip, ip, lr
    e5f8:	e004400c 	and	r4, r4, ip
    e5fc:	e3540000 	cmp	r4, #0
    e600:	1a00002c 	bne	e6b8 <strncmp+0x108>
    e604:	e2805004 	add	r5, r0, #4
    e608:	e2814004 	add	r4, r1, #4
    e60c:	ea00000b 	b	e640 <strncmp+0x90>
    e610:	e495c004 	ldr	ip, [r5], #4
    e614:	e5946000 	ldr	r6, [r4]
    e618:	e08c3003 	add	r3, ip, r3
    e61c:	e1c3300c 	bic	r3, r3, ip
    e620:	e15c0006 	cmp	ip, r6
    e624:	e00ee003 	and	lr, lr, r3
    e628:	e2844004 	add	r4, r4, #4
    e62c:	1a000009 	bne	e658 <strncmp+0xa8>
    e630:	e2522004 	subs	r2, r2, #4
    e634:	0a00001d 	beq	e6b0 <strncmp+0x100>
    e638:	e35e0000 	cmp	lr, #0
    e63c:	1a000013 	bne	e690 <strncmp+0xe0>
    e640:	e3520003 	cmp	r2, #3
    e644:	e59f3074 	ldr	r3, [pc, #116]	; e6c0 <strncmp+0x110>
    e648:	e59fe074 	ldr	lr, [pc, #116]	; e6c4 <strncmp+0x114>
    e64c:	e1a00005 	mov	r0, r5
    e650:	e1a01004 	mov	r1, r4
    e654:	8affffed 	bhi	e610 <strncmp+0x60>
    e658:	e5d03000 	ldrb	r3, [r0]
    e65c:	e5d1c000 	ldrb	ip, [r1]
    e660:	e153000c 	cmp	r3, ip
    e664:	e2422001 	sub	r2, r2, #1
    e668:	0a000005 	beq	e684 <strncmp+0xd4>
    e66c:	ea00000c 	b	e6a4 <strncmp+0xf4>
    e670:	e5f03001 	ldrb	r3, [r0, #1]!
    e674:	e5f1c001 	ldrb	ip, [r1, #1]!
    e678:	e153000c 	cmp	r3, ip
    e67c:	e2422001 	sub	r2, r2, #1
    e680:	1a000007 	bne	e6a4 <strncmp+0xf4>
    e684:	e3520000 	cmp	r2, #0
    e688:	13530000 	cmpne	r3, #0
    e68c:	1afffff7 	bne	e670 <strncmp+0xc0>
    e690:	e3a00000 	mov	r0, #0
    e694:	e8bd4070 	pop	{r4, r5, r6, lr}
    e698:	e12fff1e 	bx	lr
    e69c:	e1a00002 	mov	r0, r2
    e6a0:	e12fff1e 	bx	lr
    e6a4:	e06c0003 	rsb	r0, ip, r3
    e6a8:	e8bd4070 	pop	{r4, r5, r6, lr}
    e6ac:	e12fff1e 	bx	lr
    e6b0:	e1a00002 	mov	r0, r2
    e6b4:	eafffffb 	b	e6a8 <strncmp+0xf8>
    e6b8:	e1a00003 	mov	r0, r3
    e6bc:	eafffff9 	b	e6a8 <strncmp+0xf8>
    e6c0:	fefefeff 	mrc2	14, 7, pc, cr14, cr15, {7}
    e6c4:	80808080 	addhi	r8, r0, r0, lsl #1

0000e6c8 <strncpy>:
    e6c8:	e1803001 	orr	r3, r0, r1
    e6cc:	e3130003 	tst	r3, #3
    e6d0:	03a03001 	moveq	r3, #1
    e6d4:	13a03000 	movne	r3, #0
    e6d8:	e3520003 	cmp	r2, #3
    e6dc:	93a03000 	movls	r3, #0
    e6e0:	82033001 	andhi	r3, r3, #1
    e6e4:	e3530000 	cmp	r3, #0
    e6e8:	e92d4030 	push	{r4, r5, lr}
    e6ec:	01a03000 	moveq	r3, r0
    e6f0:	0a00000f 	beq	e734 <strncpy+0x6c>
    e6f4:	e1a0e001 	mov	lr, r1
    e6f8:	e1a03000 	mov	r3, r0
    e6fc:	e1a0100e 	mov	r1, lr
    e700:	e59fc09c 	ldr	ip, [pc, #156]	; e7a4 <strncpy+0xdc>
    e704:	e49e5004 	ldr	r5, [lr], #4
    e708:	e59f4098 	ldr	r4, [pc, #152]	; e7a8 <strncpy+0xe0>
    e70c:	e085c00c 	add	ip, r5, ip
    e710:	e1ccc005 	bic	ip, ip, r5
    e714:	e004400c 	and	r4, r4, ip
    e718:	e3540000 	cmp	r4, #0
    e71c:	1a000004 	bne	e734 <strncpy+0x6c>
    e720:	e2422004 	sub	r2, r2, #4
    e724:	e3520003 	cmp	r2, #3
    e728:	e4835004 	str	r5, [r3], #4
    e72c:	e1a0100e 	mov	r1, lr
    e730:	8afffff1 	bhi	e6fc <strncpy+0x34>
    e734:	e3520000 	cmp	r2, #0
    e738:	0a00000e 	beq	e778 <strncpy+0xb0>
    e73c:	e5d1c000 	ldrb	ip, [r1]
    e740:	e35c0000 	cmp	ip, #0
    e744:	e5c3c000 	strb	ip, [r3]
    e748:	e2422001 	sub	r2, r2, #1
    e74c:	e2833001 	add	r3, r3, #1
    e750:	e2811001 	add	r1, r1, #1
    e754:	1a000005 	bne	e770 <strncpy+0xa8>
    e758:	ea000008 	b	e780 <strncpy+0xb8>
    e75c:	e4d1c001 	ldrb	ip, [r1], #1
    e760:	e35c0000 	cmp	ip, #0
    e764:	e2422001 	sub	r2, r2, #1
    e768:	e4c3c001 	strb	ip, [r3], #1
    e76c:	0a000003 	beq	e780 <strncpy+0xb8>
    e770:	e3520000 	cmp	r2, #0
    e774:	1afffff8 	bne	e75c <strncpy+0x94>
    e778:	e8bd4030 	pop	{r4, r5, lr}
    e77c:	e12fff1e 	bx	lr
    e780:	e3520000 	cmp	r2, #0
    e784:	13a01000 	movne	r1, #0
    e788:	10832002 	addne	r2, r3, r2
    e78c:	0afffff9 	beq	e778 <strncpy+0xb0>
    e790:	e4c31001 	strb	r1, [r3], #1
    e794:	e1530002 	cmp	r3, r2
    e798:	1afffffc 	bne	e790 <strncpy+0xc8>
    e79c:	e8bd4030 	pop	{r4, r5, lr}
    e7a0:	e12fff1e 	bx	lr
    e7a4:	fefefeff 	mrc2	14, 7, pc, cr14, cr15, {7}
    e7a8:	80808080 	addhi	r8, r0, r0, lsl #1

0000e7ac <critical_factorization>:
    e7ac:	e92d41f0 	push	{r4, r5, r6, r7, r8, lr}
    e7b0:	e3a06001 	mov	r6, #1
    e7b4:	e3a0e000 	mov	lr, #0
    e7b8:	e1a0c006 	mov	ip, r6
    e7bc:	e3e04000 	mvn	r4, #0
    e7c0:	e08c300e 	add	r3, ip, lr
    e7c4:	e1530001 	cmp	r3, r1
    e7c8:	e0805004 	add	r5, r0, r4
    e7cc:	2a00000a 	bcs	e7fc <critical_factorization+0x50>
    e7d0:	e7d5500c 	ldrb	r5, [r5, ip]
    e7d4:	e7d07003 	ldrb	r7, [r0, r3]
    e7d8:	e1570005 	cmp	r7, r5
    e7dc:	2a000021 	bcs	e868 <critical_factorization+0xbc>
    e7e0:	e1a0e003 	mov	lr, r3
    e7e4:	e3a0c001 	mov	ip, #1
    e7e8:	e0646003 	rsb	r6, r4, r3
    e7ec:	e08c300e 	add	r3, ip, lr
    e7f0:	e1530001 	cmp	r3, r1
    e7f4:	e0805004 	add	r5, r0, r4
    e7f8:	3afffff4 	bcc	e7d0 <critical_factorization+0x24>
    e7fc:	e3a08001 	mov	r8, #1
    e800:	e3a0e000 	mov	lr, #0
    e804:	e1a0c008 	mov	ip, r8
    e808:	e3e05000 	mvn	r5, #0
    e80c:	e5826000 	str	r6, [r2]
    e810:	e08c300e 	add	r3, ip, lr
    e814:	e1510003 	cmp	r1, r3
    e818:	e0806005 	add	r6, r0, r5
    e81c:	9a00000a 	bls	e84c <critical_factorization+0xa0>
    e820:	e7d6600c 	ldrb	r6, [r6, ip]
    e824:	e7d07003 	ldrb	r7, [r0, r3]
    e828:	e1570006 	cmp	r7, r6
    e82c:	9a000013 	bls	e880 <critical_factorization+0xd4>
    e830:	e1a0e003 	mov	lr, r3
    e834:	e3a0c001 	mov	ip, #1
    e838:	e0658003 	rsb	r8, r5, r3
    e83c:	e08c300e 	add	r3, ip, lr
    e840:	e1510003 	cmp	r1, r3
    e844:	e0806005 	add	r6, r0, r5
    e848:	8afffff4 	bhi	e820 <critical_factorization+0x74>
    e84c:	e2840001 	add	r0, r4, #1
    e850:	e2855001 	add	r5, r5, #1
    e854:	e1550000 	cmp	r5, r0
    e858:	21a00005 	movcs	r0, r5
    e85c:	25828000 	strcs	r8, [r2]
    e860:	e8bd41f0 	pop	{r4, r5, r6, r7, r8, lr}
    e864:	e12fff1e 	bx	lr
    e868:	0a00000a 	beq	e898 <critical_factorization+0xec>
    e86c:	e3a06001 	mov	r6, #1
    e870:	e1a0400e 	mov	r4, lr
    e874:	e1a0c006 	mov	ip, r6
    e878:	e08ee006 	add	lr, lr, r6
    e87c:	eaffffcf 	b	e7c0 <critical_factorization+0x14>
    e880:	0a000009 	beq	e8ac <critical_factorization+0x100>
    e884:	e3a08001 	mov	r8, #1
    e888:	e1a0500e 	mov	r5, lr
    e88c:	e1a0c008 	mov	ip, r8
    e890:	e08ee008 	add	lr, lr, r8
    e894:	eaffffdd 	b	e810 <critical_factorization+0x64>
    e898:	e15c0006 	cmp	ip, r6
    e89c:	128cc001 	addne	ip, ip, #1
    e8a0:	01a0e003 	moveq	lr, r3
    e8a4:	03a0c001 	moveq	ip, #1
    e8a8:	eaffffc4 	b	e7c0 <critical_factorization+0x14>
    e8ac:	e15c0008 	cmp	ip, r8
    e8b0:	128cc001 	addne	ip, ip, #1
    e8b4:	01a0e003 	moveq	lr, r3
    e8b8:	03a0c001 	moveq	ip, #1
    e8bc:	eaffffd3 	b	e810 <critical_factorization+0x64>

0000e8c0 <two_way_long_needle>:
    e8c0:	e92d4ff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    e8c4:	e1a08002 	mov	r8, r2
    e8c8:	e1a04003 	mov	r4, r3
    e8cc:	e24dde41 	sub	sp, sp, #1040	; 0x410
    e8d0:	e24dd00c 	sub	sp, sp, #12
    e8d4:	e28d3014 	add	r3, sp, #20
    e8d8:	e1a02003 	mov	r2, r3
    e8dc:	e1a05000 	mov	r5, r0
    e8e0:	e1a0b001 	mov	fp, r1
    e8e4:	e1a00008 	mov	r0, r8
    e8e8:	e1a01004 	mov	r1, r4
    e8ec:	e58d3004 	str	r3, [sp, #4]
    e8f0:	ebffffad 	bl	e7ac <critical_factorization>
    e8f4:	e28d2e41 	add	r2, sp, #1040	; 0x410
    e8f8:	e58d0000 	str	r0, [sp]
    e8fc:	e59d3004 	ldr	r3, [sp, #4]
    e900:	e2822004 	add	r2, r2, #4
    e904:	e5a34004 	str	r4, [r3, #4]!
    e908:	e1530002 	cmp	r3, r2
    e90c:	1afffffc 	bne	e904 <two_way_long_needle+0x44>
    e910:	e3540000 	cmp	r4, #0
    e914:	11a01008 	movne	r1, r8
    e918:	12442001 	subne	r2, r4, #1
    e91c:	10880004 	addne	r0, r8, r4
    e920:	0a000007 	beq	e944 <two_way_long_needle+0x84>
    e924:	e4d13001 	ldrb	r3, [r1], #1
    e928:	e28dce41 	add	ip, sp, #1040	; 0x410
    e92c:	e28cc008 	add	ip, ip, #8
    e930:	e08c3103 	add	r3, ip, r3, lsl #2
    e934:	e1510000 	cmp	r1, r0
    e938:	e5032400 	str	r2, [r3, #-1024]	; 0x400
    e93c:	e2422001 	sub	r2, r2, #1
    e940:	1afffff7 	bne	e924 <two_way_long_needle+0x64>
    e944:	e59d1014 	ldr	r1, [sp, #20]
    e948:	e1a00008 	mov	r0, r8
    e94c:	e0881001 	add	r1, r8, r1
    e950:	e59d2000 	ldr	r2, [sp]
    e954:	eb0001e7 	bl	f0f8 <memcmp>
    e958:	e3500000 	cmp	r0, #0
    e95c:	1a00005c 	bne	ead4 <two_way_long_needle+0x214>
    e960:	e59d2000 	ldr	r2, [sp]
    e964:	e2423001 	sub	r3, r2, #1
    e968:	e58d3004 	str	r3, [sp, #4]
    e96c:	e0883003 	add	r3, r8, r3
    e970:	e58d3008 	str	r3, [sp, #8]
    e974:	e2623001 	rsb	r3, r2, #1
    e978:	e1a09000 	mov	r9, r0
    e97c:	e1a0a000 	mov	sl, r0
    e980:	e2446001 	sub	r6, r4, #1
    e984:	e58d300c 	str	r3, [sp, #12]
    e988:	e1a0000b 	mov	r0, fp
    e98c:	ea000007 	b	e9b0 <two_way_long_needle+0xf0>
    e990:	e3590000 	cmp	r9, #0
    e994:	0a000002 	beq	e9a4 <two_way_long_needle+0xe4>
    e998:	e59d2014 	ldr	r2, [sp, #20]
    e99c:	e1530002 	cmp	r3, r2
    e9a0:	30623004 	rsbcc	r3, r2, r4
    e9a4:	e3a09000 	mov	r9, #0
    e9a8:	e08aa003 	add	sl, sl, r3
    e9ac:	e1a00007 	mov	r0, r7
    e9b0:	e08a7004 	add	r7, sl, r4
    e9b4:	e0602007 	rsb	r2, r0, r7
    e9b8:	e3a01000 	mov	r1, #0
    e9bc:	e0850000 	add	r0, r5, r0
    e9c0:	eb000186 	bl	efe0 <memchr>
    e9c4:	e2973000 	adds	r3, r7, #0
    e9c8:	13a03001 	movne	r3, #1
    e9cc:	e3500000 	cmp	r0, #0
    e9d0:	13a03000 	movne	r3, #0
    e9d4:	e3530000 	cmp	r3, #0
    e9d8:	0a000097 	beq	ec3c <two_way_long_needle+0x37c>
    e9dc:	e0853007 	add	r3, r5, r7
    e9e0:	e5533001 	ldrb	r3, [r3, #-1]
    e9e4:	e28d2e41 	add	r2, sp, #1040	; 0x410
    e9e8:	e2822008 	add	r2, r2, #8
    e9ec:	e0823103 	add	r3, r2, r3, lsl #2
    e9f0:	e5133400 	ldr	r3, [r3, #-1024]	; 0x400
    e9f4:	e3530000 	cmp	r3, #0
    e9f8:	1affffe4 	bne	e990 <two_way_long_needle+0xd0>
    e9fc:	e59d3000 	ldr	r3, [sp]
    ea00:	e1590003 	cmp	r9, r3
    ea04:	21a03009 	movcs	r3, r9
    ea08:	e1530006 	cmp	r3, r6
    ea0c:	2a00000e 	bcs	ea4c <two_way_long_needle+0x18c>
    ea10:	e083200a 	add	r2, r3, sl
    ea14:	e7d51002 	ldrb	r1, [r5, r2]
    ea18:	e7d80003 	ldrb	r0, [r8, r3]
    ea1c:	e1500001 	cmp	r0, r1
    ea20:	e0852002 	add	r2, r5, r2
    ea24:	e0881003 	add	r1, r8, r3
    ea28:	0a000004 	beq	ea40 <two_way_long_needle+0x180>
    ea2c:	ea000025 	b	eac8 <two_way_long_needle+0x208>
    ea30:	e5f1c001 	ldrb	ip, [r1, #1]!
    ea34:	e5f20001 	ldrb	r0, [r2, #1]!
    ea38:	e15c0000 	cmp	ip, r0
    ea3c:	1a000021 	bne	eac8 <two_way_long_needle+0x208>
    ea40:	e2833001 	add	r3, r3, #1
    ea44:	e1530006 	cmp	r3, r6
    ea48:	3afffff8 	bcc	ea30 <two_way_long_needle+0x170>
    ea4c:	e59d3000 	ldr	r3, [sp]
    ea50:	e1590003 	cmp	r9, r3
    ea54:	e59d1004 	ldr	r1, [sp, #4]
    ea58:	21a01003 	movcs	r1, r3
    ea5c:	2a000012 	bcs	eaac <two_way_long_needle+0x1ec>
    ea60:	e59d3004 	ldr	r3, [sp, #4]
    ea64:	e59dc008 	ldr	ip, [sp, #8]
    ea68:	e08a2003 	add	r2, sl, r3
    ea6c:	e7d53002 	ldrb	r3, [r5, r2]
    ea70:	e5dc0000 	ldrb	r0, [ip]
    ea74:	e1500003 	cmp	r0, r3
    ea78:	e0852002 	add	r2, r5, r2
    ea7c:	1a000073 	bne	ec50 <two_way_long_needle+0x390>
    ea80:	e1a0300c 	mov	r3, ip
    ea84:	e088b009 	add	fp, r8, r9
    ea88:	ea000004 	b	eaa0 <two_way_long_needle+0x1e0>
    ea8c:	e573c001 	ldrb	ip, [r3, #-1]!
    ea90:	e5720001 	ldrb	r0, [r2, #-1]!
    ea94:	e15c0000 	cmp	ip, r0
    ea98:	1a000003 	bne	eaac <two_way_long_needle+0x1ec>
    ea9c:	e1a0100e 	mov	r1, lr
    eaa0:	e153000b 	cmp	r3, fp
    eaa4:	e241e001 	sub	lr, r1, #1
    eaa8:	1afffff7 	bne	ea8c <two_way_long_needle+0x1cc>
    eaac:	e2899001 	add	r9, r9, #1
    eab0:	e1590001 	cmp	r9, r1
    eab4:	8a000067 	bhi	ec58 <two_way_long_needle+0x398>
    eab8:	e59d9014 	ldr	r9, [sp, #20]
    eabc:	e08aa009 	add	sl, sl, r9
    eac0:	e0699004 	rsb	r9, r9, r4
    eac4:	eaffffb8 	b	e9ac <two_way_long_needle+0xec>
    eac8:	e59d200c 	ldr	r2, [sp, #12]
    eacc:	e082a00a 	add	sl, r2, sl
    ead0:	eaffffb3 	b	e9a4 <two_way_long_needle+0xe4>
    ead4:	e59d2000 	ldr	r2, [sp]
    ead8:	e0623004 	rsb	r3, r2, r4
    eadc:	e1530002 	cmp	r3, r2
    eae0:	31a03002 	movcc	r3, r2
    eae4:	e2421001 	sub	r1, r2, #1
    eae8:	e2833001 	add	r3, r3, #1
    eaec:	e0887002 	add	r7, r8, r2
    eaf0:	e58d3014 	str	r3, [sp, #20]
    eaf4:	e2446001 	sub	r6, r4, #1
    eaf8:	e0883001 	add	r3, r8, r1
    eafc:	e1a0000b 	mov	r0, fp
    eb00:	e3a09000 	mov	r9, #0
    eb04:	e1a0b008 	mov	fp, r8
    eb08:	e1a08007 	mov	r8, r7
    eb0c:	e1a07004 	mov	r7, r4
    eb10:	e1a04002 	mov	r4, r2
    eb14:	e58d3004 	str	r3, [sp, #4]
    eb18:	e2623001 	rsb	r3, r2, #1
    eb1c:	e58d1000 	str	r1, [sp]
    eb20:	e58d3008 	str	r3, [sp, #8]
    eb24:	e089a007 	add	sl, r9, r7
    eb28:	e060200a 	rsb	r2, r0, sl
    eb2c:	e3a01000 	mov	r1, #0
    eb30:	e0850000 	add	r0, r5, r0
    eb34:	eb000129 	bl	efe0 <memchr>
    eb38:	e29a3000 	adds	r3, sl, #0
    eb3c:	13a03001 	movne	r3, #1
    eb40:	e3500000 	cmp	r0, #0
    eb44:	13a03000 	movne	r3, #0
    eb48:	e3530000 	cmp	r3, #0
    eb4c:	0a00003a 	beq	ec3c <two_way_long_needle+0x37c>
    eb50:	e085300a 	add	r3, r5, sl
    eb54:	e5533001 	ldrb	r3, [r3, #-1]
    eb58:	e28d2e41 	add	r2, sp, #1040	; 0x410
    eb5c:	e2822008 	add	r2, r2, #8
    eb60:	e0823103 	add	r3, r2, r3, lsl #2
    eb64:	e5133400 	ldr	r3, [r3, #-1024]	; 0x400
    eb68:	e3530000 	cmp	r3, #0
    eb6c:	1a00002b 	bne	ec20 <two_way_long_needle+0x360>
    eb70:	e1540006 	cmp	r4, r6
    eb74:	2a000010 	bcs	ebbc <two_way_long_needle+0x2fc>
    eb78:	e0892004 	add	r2, r9, r4
    eb7c:	e7d53002 	ldrb	r3, [r5, r2]
    eb80:	e5d81000 	ldrb	r1, [r8]
    eb84:	e1510003 	cmp	r1, r3
    eb88:	e0852002 	add	r2, r5, r2
    eb8c:	11a03004 	movne	r3, r4
    eb90:	1a000025 	bne	ec2c <two_way_long_needle+0x36c>
    eb94:	e1a01008 	mov	r1, r8
    eb98:	e1a03004 	mov	r3, r4
    eb9c:	ea000003 	b	ebb0 <two_way_long_needle+0x2f0>
    eba0:	e5f1c001 	ldrb	ip, [r1, #1]!
    eba4:	e5f20001 	ldrb	r0, [r2, #1]!
    eba8:	e15c0000 	cmp	ip, r0
    ebac:	1a00001e 	bne	ec2c <two_way_long_needle+0x36c>
    ebb0:	e2833001 	add	r3, r3, #1
    ebb4:	e1530006 	cmp	r3, r6
    ebb8:	3afffff8 	bcc	eba0 <two_way_long_needle+0x2e0>
    ebbc:	e59d3000 	ldr	r3, [sp]
    ebc0:	e3730001 	cmn	r3, #1
    ebc4:	0a00000f 	beq	ec08 <two_way_long_needle+0x348>
    ebc8:	e59d3000 	ldr	r3, [sp]
    ebcc:	e59d0004 	ldr	r0, [sp, #4]
    ebd0:	e0892003 	add	r2, r9, r3
    ebd4:	e7d53002 	ldrb	r3, [r5, r2]
    ebd8:	e5d01000 	ldrb	r1, [r0]
    ebdc:	e1510003 	cmp	r1, r3
    ebe0:	e0852002 	add	r2, r5, r2
    ebe4:	1a00000c 	bne	ec1c <two_way_long_needle+0x35c>
    ebe8:	e1a03000 	mov	r3, r0
    ebec:	ea000003 	b	ec00 <two_way_long_needle+0x340>
    ebf0:	e5730001 	ldrb	r0, [r3, #-1]!
    ebf4:	e5721001 	ldrb	r1, [r2, #-1]!
    ebf8:	e1500001 	cmp	r0, r1
    ebfc:	1a000006 	bne	ec1c <two_way_long_needle+0x35c>
    ec00:	e153000b 	cmp	r3, fp
    ec04:	1afffff9 	bne	ebf0 <two_way_long_needle+0x330>
    ec08:	e0850009 	add	r0, r5, r9
    ec0c:	e28dde41 	add	sp, sp, #1040	; 0x410
    ec10:	e28dd00c 	add	sp, sp, #12
    ec14:	e8bd4ff0 	pop	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ec18:	e12fff1e 	bx	lr
    ec1c:	e59d3014 	ldr	r3, [sp, #20]
    ec20:	e0899003 	add	r9, r9, r3
    ec24:	e1a0000a 	mov	r0, sl
    ec28:	eaffffbd 	b	eb24 <two_way_long_needle+0x264>
    ec2c:	e59d2008 	ldr	r2, [sp, #8]
    ec30:	e0829009 	add	r9, r2, r9
    ec34:	e0899003 	add	r9, r9, r3
    ec38:	eafffff9 	b	ec24 <two_way_long_needle+0x364>
    ec3c:	e3a00000 	mov	r0, #0
    ec40:	e28dde41 	add	sp, sp, #1040	; 0x410
    ec44:	e28dd00c 	add	sp, sp, #12
    ec48:	e8bd4ff0 	pop	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ec4c:	e12fff1e 	bx	lr
    ec50:	e59d1000 	ldr	r1, [sp]
    ec54:	eaffff94 	b	eaac <two_way_long_needle+0x1ec>
    ec58:	e085000a 	add	r0, r5, sl
    ec5c:	eaffffea 	b	ec0c <two_way_long_needle+0x34c>

0000ec60 <strstr>:
    ec60:	e92d4ff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ec64:	e5d02000 	ldrb	r2, [r0]
    ec68:	e3520000 	cmp	r2, #0
    ec6c:	e24dd01c 	sub	sp, sp, #28
    ec70:	0a0000d5 	beq	efcc <strstr+0x36c>
    ec74:	e5d1c000 	ldrb	ip, [r1]
    ec78:	e35c0000 	cmp	ip, #0
    ec7c:	0a000012 	beq	eccc <strstr+0x6c>
    ec80:	e3a08001 	mov	r8, #1
    ec84:	e1a09000 	mov	r9, r0
    ec88:	e081e008 	add	lr, r1, r8
    ec8c:	ea000002 	b	ec9c <strstr+0x3c>
    ec90:	e4dec001 	ldrb	ip, [lr], #1
    ec94:	e35c0000 	cmp	ip, #0
    ec98:	0a000009 	beq	ecc4 <strstr+0x64>
    ec9c:	e152000c 	cmp	r2, ip
    eca0:	13a08000 	movne	r8, #0
    eca4:	02088001 	andeq	r8, r8, #1
    eca8:	e5f92001 	ldrb	r2, [r9, #1]!
    ecac:	e3520000 	cmp	r2, #0
    ecb0:	e1a0400e 	mov	r4, lr
    ecb4:	1afffff5 	bne	ec90 <strstr+0x30>
    ecb8:	e5d43000 	ldrb	r3, [r4]
    ecbc:	e3530000 	cmp	r3, #0
    ecc0:	1a00001c 	bne	ed38 <strstr+0xd8>
    ecc4:	e3580000 	cmp	r8, #0
    ecc8:	0a000002 	beq	ecd8 <strstr+0x78>
    eccc:	e28dd01c 	add	sp, sp, #28
    ecd0:	e8bd4ff0 	pop	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ecd4:	e12fff1e 	bx	lr
    ecd8:	e1a06001 	mov	r6, r1
    ecdc:	e1a05000 	mov	r5, r0
    ece0:	e5d11000 	ldrb	r1, [r1]
    ece4:	e2800001 	add	r0, r0, #1
    ece8:	ebfffcfb 	bl	e0dc <strchr>
    ecec:	e0664004 	rsb	r4, r6, r4
    ecf0:	e3540001 	cmp	r4, #1
    ecf4:	13500000 	cmpne	r0, #0
    ecf8:	e1a07000 	mov	r7, r0
    ecfc:	0afffff2 	beq	eccc <strstr+0x6c>
    ed00:	e0850004 	add	r0, r5, r4
    ed04:	e1570000 	cmp	r7, r0
    ed08:	90673000 	rsbls	r3, r7, r0
    ed0c:	83a03001 	movhi	r3, #1
    ed10:	e354001f 	cmp	r4, #31
    ed14:	9a00000b 	bls	ed48 <strstr+0xe8>
    ed18:	e1a01003 	mov	r1, r3
    ed1c:	e1a00007 	mov	r0, r7
    ed20:	e1a02006 	mov	r2, r6
    ed24:	e1a03004 	mov	r3, r4
    ed28:	ebfffee4 	bl	e8c0 <two_way_long_needle>
    ed2c:	e28dd01c 	add	sp, sp, #28
    ed30:	e8bd4ff0 	pop	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ed34:	e12fff1e 	bx	lr
    ed38:	e3a00000 	mov	r0, #0
    ed3c:	e28dd01c 	add	sp, sp, #28
    ed40:	e8bd4ff0 	pop	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ed44:	e12fff1e 	bx	lr
    ed48:	e1a01004 	mov	r1, r4
    ed4c:	e28d2014 	add	r2, sp, #20
    ed50:	e1a00006 	mov	r0, r6
    ed54:	e58d3004 	str	r3, [sp, #4]
    ed58:	ebfffe93 	bl	e7ac <critical_factorization>
    ed5c:	e59d1014 	ldr	r1, [sp, #20]
    ed60:	e1a02000 	mov	r2, r0
    ed64:	e1a0b000 	mov	fp, r0
    ed68:	e0861001 	add	r1, r6, r1
    ed6c:	e1a00006 	mov	r0, r6
    ed70:	eb0000e0 	bl	f0f8 <memcmp>
    ed74:	e3500000 	cmp	r0, #0
    ed78:	e59d3004 	ldr	r3, [sp, #4]
    ed7c:	0a00003e 	beq	ee7c <strstr+0x21c>
    ed80:	e06b2004 	rsb	r2, fp, r4
    ed84:	e152000b 	cmp	r2, fp
    ed88:	31a0200b 	movcc	r2, fp
    ed8c:	e3a05000 	mov	r5, #0
    ed90:	e1a00003 	mov	r0, r3
    ed94:	e2822001 	add	r2, r2, #1
    ed98:	e24b9001 	sub	r9, fp, #1
    ed9c:	e58d2014 	str	r2, [sp, #20]
    eda0:	e0862009 	add	r2, r6, r9
    eda4:	e58d2008 	str	r2, [sp, #8]
    eda8:	e26b2001 	rsb	r2, fp, #1
    edac:	e58d2004 	str	r2, [sp, #4]
    edb0:	e086800b 	add	r8, r6, fp
    edb4:	e085a004 	add	sl, r5, r4
    edb8:	e060200a 	rsb	r2, r0, sl
    edbc:	e3a01000 	mov	r1, #0
    edc0:	e0870000 	add	r0, r7, r0
    edc4:	eb000085 	bl	efe0 <memchr>
    edc8:	e29a3000 	adds	r3, sl, #0
    edcc:	13a03001 	movne	r3, #1
    edd0:	e3500000 	cmp	r0, #0
    edd4:	13a03000 	movne	r3, #0
    edd8:	e3530000 	cmp	r3, #0
    eddc:	0affffd5 	beq	ed38 <strstr+0xd8>
    ede0:	e154000b 	cmp	r4, fp
    ede4:	9a00000f 	bls	ee28 <strstr+0x1c8>
    ede8:	e085200b 	add	r2, r5, fp
    edec:	e7d73002 	ldrb	r3, [r7, r2]
    edf0:	e5d81000 	ldrb	r1, [r8]
    edf4:	e1510003 	cmp	r1, r3
    edf8:	e0872002 	add	r2, r7, r2
    edfc:	1a000069 	bne	efa8 <strstr+0x348>
    ee00:	e1a01008 	mov	r1, r8
    ee04:	e1a0300b 	mov	r3, fp
    ee08:	ea000003 	b	ee1c <strstr+0x1bc>
    ee0c:	e5f1c001 	ldrb	ip, [r1, #1]!
    ee10:	e5f20001 	ldrb	r0, [r2, #1]!
    ee14:	e15c0000 	cmp	ip, r0
    ee18:	1a000063 	bne	efac <strstr+0x34c>
    ee1c:	e2833001 	add	r3, r3, #1
    ee20:	e1540003 	cmp	r4, r3
    ee24:	8afffff8 	bhi	ee0c <strstr+0x1ac>
    ee28:	e3790001 	cmn	r9, #1
    ee2c:	0a00000e 	beq	ee6c <strstr+0x20c>
    ee30:	e59d0008 	ldr	r0, [sp, #8]
    ee34:	e0852009 	add	r2, r5, r9
    ee38:	e7d73002 	ldrb	r3, [r7, r2]
    ee3c:	e5d01000 	ldrb	r1, [r0]
    ee40:	e1510003 	cmp	r1, r3
    ee44:	e0872002 	add	r2, r7, r2
    ee48:	1a00005c 	bne	efc0 <strstr+0x360>
    ee4c:	e1a03000 	mov	r3, r0
    ee50:	ea000003 	b	ee64 <strstr+0x204>
    ee54:	e5730001 	ldrb	r0, [r3, #-1]!
    ee58:	e5721001 	ldrb	r1, [r2, #-1]!
    ee5c:	e1500001 	cmp	r0, r1
    ee60:	1a000056 	bne	efc0 <strstr+0x360>
    ee64:	e1530006 	cmp	r3, r6
    ee68:	1afffff9 	bne	ee54 <strstr+0x1f4>
    ee6c:	e0870005 	add	r0, r7, r5
    ee70:	e28dd01c 	add	sp, sp, #28
    ee74:	e8bd4ff0 	pop	{r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ee78:	e12fff1e 	bx	lr
    ee7c:	e1a08000 	mov	r8, r0
    ee80:	e1a05000 	mov	r5, r0
    ee84:	e1a00003 	mov	r0, r3
    ee88:	e24b2001 	sub	r2, fp, #1
    ee8c:	e58d2004 	str	r2, [sp, #4]
    ee90:	e0862002 	add	r2, r6, r2
    ee94:	e58d200c 	str	r2, [sp, #12]
    ee98:	e26b2001 	rsb	r2, fp, #1
    ee9c:	e58d2008 	str	r2, [sp, #8]
    eea0:	e0859004 	add	r9, r5, r4
    eea4:	e0602009 	rsb	r2, r0, r9
    eea8:	e3a01000 	mov	r1, #0
    eeac:	e0870000 	add	r0, r7, r0
    eeb0:	eb00004a 	bl	efe0 <memchr>
    eeb4:	e2993000 	adds	r3, r9, #0
    eeb8:	13a03001 	movne	r3, #1
    eebc:	e3500000 	cmp	r0, #0
    eec0:	13a03000 	movne	r3, #0
    eec4:	e3530000 	cmp	r3, #0
    eec8:	0affff9a 	beq	ed38 <strstr+0xd8>
    eecc:	e158000b 	cmp	r8, fp
    eed0:	21a03008 	movcs	r3, r8
    eed4:	31a0300b 	movcc	r3, fp
    eed8:	e1540003 	cmp	r4, r3
    eedc:	9a00000e 	bls	ef1c <strstr+0x2bc>
    eee0:	e0832005 	add	r2, r3, r5
    eee4:	e7d71002 	ldrb	r1, [r7, r2]
    eee8:	e7d60003 	ldrb	r0, [r6, r3]
    eeec:	e1500001 	cmp	r0, r1
    eef0:	e0872002 	add	r2, r7, r2
    eef4:	e0861003 	add	r1, r6, r3
    eef8:	0a000004 	beq	ef10 <strstr+0x2b0>
    eefc:	ea000024 	b	ef94 <strstr+0x334>
    ef00:	e5f1c001 	ldrb	ip, [r1, #1]!
    ef04:	e5f20001 	ldrb	r0, [r2, #1]!
    ef08:	e15c0000 	cmp	ip, r0
    ef0c:	1a000020 	bne	ef94 <strstr+0x334>
    ef10:	e2833001 	add	r3, r3, #1
    ef14:	e1540003 	cmp	r4, r3
    ef18:	8afffff8 	bhi	ef00 <strstr+0x2a0>
    ef1c:	e59d3004 	ldr	r3, [sp, #4]
    ef20:	e15b0008 	cmp	fp, r8
    ef24:	e1a01003 	mov	r1, r3
    ef28:	9a00002a 	bls	efd8 <strstr+0x378>
    ef2c:	e59dc00c 	ldr	ip, [sp, #12]
    ef30:	e0852003 	add	r2, r5, r3
    ef34:	e7d73002 	ldrb	r3, [r7, r2]
    ef38:	e5dc0000 	ldrb	r0, [ip]
    ef3c:	e1500003 	cmp	r0, r3
    ef40:	e0872002 	add	r2, r7, r2
    ef44:	1a000023 	bne	efd8 <strstr+0x378>
    ef48:	e1a0300c 	mov	r3, ip
    ef4c:	e086a008 	add	sl, r6, r8
    ef50:	ea000004 	b	ef68 <strstr+0x308>
    ef54:	e573e001 	ldrb	lr, [r3, #-1]!
    ef58:	e572c001 	ldrb	ip, [r2, #-1]!
    ef5c:	e15e000c 	cmp	lr, ip
    ef60:	1a000003 	bne	ef74 <strstr+0x314>
    ef64:	e1a01000 	mov	r1, r0
    ef68:	e153000a 	cmp	r3, sl
    ef6c:	e2410001 	sub	r0, r1, #1
    ef70:	1afffff7 	bne	ef54 <strstr+0x2f4>
    ef74:	e2888001 	add	r8, r8, #1
    ef78:	e1580001 	cmp	r8, r1
    ef7c:	8affffba 	bhi	ee6c <strstr+0x20c>
    ef80:	e59d8014 	ldr	r8, [sp, #20]
    ef84:	e0855008 	add	r5, r5, r8
    ef88:	e0688004 	rsb	r8, r8, r4
    ef8c:	e1a00009 	mov	r0, r9
    ef90:	eaffffc2 	b	eea0 <strstr+0x240>
    ef94:	e59d2008 	ldr	r2, [sp, #8]
    ef98:	e0825005 	add	r5, r2, r5
    ef9c:	e0855003 	add	r5, r5, r3
    efa0:	e3a08000 	mov	r8, #0
    efa4:	eafffff8 	b	ef8c <strstr+0x32c>
    efa8:	e1a0300b 	mov	r3, fp
    efac:	e59d2004 	ldr	r2, [sp, #4]
    efb0:	e0825005 	add	r5, r2, r5
    efb4:	e0855003 	add	r5, r5, r3
    efb8:	e1a0000a 	mov	r0, sl
    efbc:	eaffff7c 	b	edb4 <strstr+0x154>
    efc0:	e59d3014 	ldr	r3, [sp, #20]
    efc4:	e0855003 	add	r5, r5, r3
    efc8:	eafffffa 	b	efb8 <strstr+0x358>
    efcc:	e1a04001 	mov	r4, r1
    efd0:	e3a08001 	mov	r8, #1
    efd4:	eaffff37 	b	ecb8 <strstr+0x58>
    efd8:	e1a0100b 	mov	r1, fp
    efdc:	eaffffe4 	b	ef74 <strstr+0x314>

0000efe0 <memchr>:
    efe0:	e3100003 	tst	r0, #3
    efe4:	e92d4030 	push	{r4, r5, lr}
    efe8:	e20110ff 	and	r1, r1, #255	; 0xff
    efec:	0a00003b 	beq	f0e0 <memchr+0x100>
    eff0:	e3520000 	cmp	r2, #0
    eff4:	e242c001 	sub	ip, r2, #1
    eff8:	0a00003a 	beq	f0e8 <memchr+0x108>
    effc:	e5d03000 	ldrb	r3, [r0]
    f000:	e1530001 	cmp	r3, r1
    f004:	12803001 	addne	r3, r0, #1
    f008:	1a000007 	bne	f02c <memchr+0x4c>
    f00c:	ea00001b 	b	f080 <memchr+0xa0>
    f010:	e35c0000 	cmp	ip, #0
    f014:	0a00001b 	beq	f088 <memchr+0xa8>
    f018:	e5d02000 	ldrb	r2, [r0]
    f01c:	e1520001 	cmp	r2, r1
    f020:	e2833001 	add	r3, r3, #1
    f024:	e24cc001 	sub	ip, ip, #1
    f028:	0a000014 	beq	f080 <memchr+0xa0>
    f02c:	e3130003 	tst	r3, #3
    f030:	e1a00003 	mov	r0, r3
    f034:	1afffff5 	bne	f010 <memchr+0x30>
    f038:	e35c0003 	cmp	ip, #3
    f03c:	8a000014 	bhi	f094 <memchr+0xb4>
    f040:	e35c0000 	cmp	ip, #0
    f044:	0a00000f 	beq	f088 <memchr+0xa8>
    f048:	e5d03000 	ldrb	r3, [r0]
    f04c:	e1530001 	cmp	r3, r1
    f050:	1080c00c 	addne	ip, r0, ip
    f054:	12803001 	addne	r3, r0, #1
    f058:	1a000004 	bne	f070 <memchr+0x90>
    f05c:	ea000007 	b	f080 <memchr+0xa0>
    f060:	e5d02000 	ldrb	r2, [r0]
    f064:	e1520001 	cmp	r2, r1
    f068:	e2833001 	add	r3, r3, #1
    f06c:	0a000003 	beq	f080 <memchr+0xa0>
    f070:	e153000c 	cmp	r3, ip
    f074:	e1a00003 	mov	r0, r3
    f078:	1afffff8 	bne	f060 <memchr+0x80>
    f07c:	e3a00000 	mov	r0, #0
    f080:	e8bd4030 	pop	{r4, r5, lr}
    f084:	e12fff1e 	bx	lr
    f088:	e1a0000c 	mov	r0, ip
    f08c:	e8bd4030 	pop	{r4, r5, lr}
    f090:	e12fff1e 	bx	lr
    f094:	e1a0e000 	mov	lr, r0
    f098:	e1815401 	orr	r5, r1, r1, lsl #8
    f09c:	e1855805 	orr	r5, r5, r5, lsl #16
    f0a0:	e59e2000 	ldr	r2, [lr]
    f0a4:	e59f3044 	ldr	r3, [pc, #68]	; f0f0 <memchr+0x110>
    f0a8:	e0222005 	eor	r2, r2, r5
    f0ac:	e0823003 	add	r3, r2, r3
    f0b0:	e59f403c 	ldr	r4, [pc, #60]	; f0f4 <memchr+0x114>
    f0b4:	e1c33002 	bic	r3, r3, r2
    f0b8:	e0044003 	and	r4, r4, r3
    f0bc:	e3540000 	cmp	r4, #0
    f0c0:	e1a0000e 	mov	r0, lr
    f0c4:	e28ee004 	add	lr, lr, #4
    f0c8:	1affffdc 	bne	f040 <memchr+0x60>
    f0cc:	e24cc004 	sub	ip, ip, #4
    f0d0:	e35c0003 	cmp	ip, #3
    f0d4:	e1a0000e 	mov	r0, lr
    f0d8:	8afffff0 	bhi	f0a0 <memchr+0xc0>
    f0dc:	eaffffd7 	b	f040 <memchr+0x60>
    f0e0:	e1a0c002 	mov	ip, r2
    f0e4:	eaffffd3 	b	f038 <memchr+0x58>
    f0e8:	e1a00002 	mov	r0, r2
    f0ec:	eaffffe3 	b	f080 <memchr+0xa0>
    f0f0:	fefefeff 	mrc2	14, 7, pc, cr14, cr15, {7}
    f0f4:	80808080 	addhi	r8, r0, r0, lsl #1

0000f0f8 <memcmp>:
    f0f8:	e3520003 	cmp	r2, #3
    f0fc:	e92d4010 	push	{r4, lr}
    f100:	9a000011 	bls	f14c <memcmp+0x54>
    f104:	e1803001 	orr	r3, r0, r1
    f108:	e3130003 	tst	r3, #3
    f10c:	1a000010 	bne	f154 <memcmp+0x5c>
    f110:	e1a0c001 	mov	ip, r1
    f114:	e1a03000 	mov	r3, r0
    f118:	e5934000 	ldr	r4, [r3]
    f11c:	e59ce000 	ldr	lr, [ip]
    f120:	e154000e 	cmp	r4, lr
    f124:	e1a00003 	mov	r0, r3
    f128:	e1a0100c 	mov	r1, ip
    f12c:	e2833004 	add	r3, r3, #4
    f130:	e28cc004 	add	ip, ip, #4
    f134:	1a000004 	bne	f14c <memcmp+0x54>
    f138:	e2422004 	sub	r2, r2, #4
    f13c:	e3520003 	cmp	r2, #3
    f140:	e1a00003 	mov	r0, r3
    f144:	e1a0100c 	mov	r1, ip
    f148:	8afffff2 	bhi	f118 <memcmp+0x20>
    f14c:	e3520000 	cmp	r2, #0
    f150:	0a000012 	beq	f1a0 <memcmp+0xa8>
    f154:	e5d0c000 	ldrb	ip, [r0]
    f158:	e5d1e000 	ldrb	lr, [r1]
    f15c:	e15c000e 	cmp	ip, lr
    f160:	00802002 	addeq	r2, r0, r2
    f164:	02803001 	addeq	r3, r0, #1
    f168:	0a000004 	beq	f180 <memcmp+0x88>
    f16c:	ea000008 	b	f194 <memcmp+0x9c>
    f170:	e4d3c001 	ldrb	ip, [r3], #1
    f174:	e5f1e001 	ldrb	lr, [r1, #1]!
    f178:	e15c000e 	cmp	ip, lr
    f17c:	1a000004 	bne	f194 <memcmp+0x9c>
    f180:	e1530002 	cmp	r3, r2
    f184:	1afffff9 	bne	f170 <memcmp+0x78>
    f188:	e3a00000 	mov	r0, #0
    f18c:	e8bd4010 	pop	{r4, lr}
    f190:	e12fff1e 	bx	lr
    f194:	e06e000c 	rsb	r0, lr, ip
    f198:	e8bd4010 	pop	{r4, lr}
    f19c:	e12fff1e 	bx	lr
    f1a0:	e1a00002 	mov	r0, r2
    f1a4:	eafffff8 	b	f18c <memcmp+0x94>

Disassembly of section .rodata:

0000f1a8 <crc32_tab-0x388>:
    f1a8:	00007325 	andeq	r7, r0, r5, lsr #6
    f1ac:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f1b0:	6966203a 	stmdbvs	r6!, {r1, r3, r4, r5, sp}^
    f1b4:	6e20656c 	cfsh64vs	mvdx6, mvdx0, #60
    f1b8:	6620746f 	strtvs	r7, [r0], -pc, ror #8
    f1bc:	646e756f 	strbtvs	r7, [lr], #-1391	; 0x56f
    f1c0:	255b2021 	ldrbcs	r2, [fp, #-33]	; 0x21
    f1c4:	000a5d73 	andeq	r5, sl, r3, ror sp
    f1c8:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f1cc:	7325203a 			; <UNDEFINED> instruction: 0x7325203a
    f1d0:	00000020 	andeq	r0, r0, r0, lsr #32
    f1d4:	7272655b 	rsbsvc	r6, r2, #381681664	; 0x16c00000
    f1d8:	5d21726f 	sfmpl	f7, 4, [r1, #-444]!	; 0xfffffe44
    f1dc:	0000000a 	andeq	r0, r0, sl
    f1e0:	5d6b6f5b 	stclpl	15, cr6, [fp, #-364]!	; 0xfffffe94
    f1e4:	0000000a 	andeq	r0, r0, sl
    f1e8:	6374652f 	cmnvs	r4, #197132288	; 0xbc00000
    f1ec:	7665642f 	strbtvc	r6, [r5], -pc, lsr #8
    f1f0:	6372612f 	cmnvs	r2, #-1073741813	; 0xc000000b
    f1f4:	73252f68 			; <UNDEFINED> instruction: 0x73252f68
    f1f8:	696e692f 	stmdbvs	lr!, {r0, r1, r2, r3, r5, r8, fp, sp, lr}^
    f1fc:	65642e74 	strbvs	r2, [r4, #-3700]!	; 0xe74
    f200:	00000076 	andeq	r0, r0, r6, ror r0
    f204:	6374652f 	cmnvs	r4, #197132288	; 0xbc00000
    f208:	7665642f 	strbtvc	r6, [r5], -pc, lsr #8
    f20c:	7478652f 	ldrbtvc	r6, [r8], #-1327	; 0x52f
    f210:	00006172 	andeq	r6, r0, r2, ror r1
    f214:	252f7325 	strcs	r7, [pc, #-805]!	; eef7 <strstr+0x297>
    f218:	00000073 	andeq	r0, r0, r3, ror r0
    f21c:	636f7270 	cmnvs	pc, #112, 4
    f220:	20737365 	rsbscs	r7, r3, r5, ror #6
    f224:	696e6927 	stmdbvs	lr!, {r0, r1, r2, r5, r8, fp, sp, lr}^
    f228:	63202774 			; <UNDEFINED> instruction: 0x63202774
    f22c:	6f206e61 	svcvs	0x00206e61
    f230:	20796c6e 	rsbscs	r6, r9, lr, ror #24
    f234:	64616f6c 	strbtvs	r6, [r1], #-3948	; 0xf6c
    f238:	62206465 	eorvs	r6, r0, #1694498816	; 0x65000000
    f23c:	656b2079 	strbvs	r2, [fp, #-121]!	; 0x79
    f240:	6c656e72 	stclvs	14, cr6, [r5], #-456	; 0xfffffe38
    f244:	00000a21 	andeq	r0, r0, r1, lsr #20
    f248:	6e695b0a 	vmulvs.f64	d21, d9, d10
    f24c:	70207469 	eorvc	r7, r0, r9, ror #8
    f250:	65636f72 	strbvs	r6, [r3, #-3954]!	; 0xf72
    f254:	73207373 			; <UNDEFINED> instruction: 0x73207373
    f258:	74726174 	ldrbtvc	r6, [r2], #-372	; 0x174
    f25c:	0a5d6465 	beq	17683f8 <_stack+0x16e83f8>
    f260:	00000000 	andeq	r0, r0, r0
    f264:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f268:	00000000 	andeq	r0, r0, r0
    f26c:	206e7572 	rsbcs	r7, lr, r2, ror r5
    f270:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f274:	726f632d 	rsbvc	r6, pc, #-1275068416	; 0xb4000000
    f278:	20202065 	eorcs	r2, r0, r5, rrx
    f27c:	00000020 	andeq	r0, r0, r0, lsr #32
    f280:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f284:	726f632d 	rsbvc	r6, pc, #-1275068416	; 0xb4000000
    f288:	00000065 	andeq	r0, r0, r5, rrx
    f28c:	206e7572 	rsbcs	r7, lr, r2, ror r5
    f290:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f294:	7366762d 	cmnvc	r6, #47185920	; 0x2d00000
    f298:	20202064 	eorcs	r2, r0, r4, rrx
    f29c:	00000020 	andeq	r0, r0, r0, lsr #32
    f2a0:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f2a4:	7366762d 	cmnvc	r6, #47185920	; 0x2d00000
    f2a8:	00000064 	andeq	r0, r0, r4, rrx
    f2ac:	206e7572 	rsbcs	r7, lr, r2, ror r5
    f2b0:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f2b4:	6f6f722d 	svcvs	0x006f722d
    f2b8:	64736674 	ldrbtvs	r6, [r3], #-1652	; 0x674
    f2bc:	20202020 	eorcs	r2, r0, r0, lsr #32
    f2c0:	00000000 	andeq	r0, r0, r0
    f2c4:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f2c8:	6664732d 	strbtvs	r7, [r4], -sp, lsr #6
    f2cc:	00006473 	andeq	r6, r0, r3, ror r4
    f2d0:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f2d4:	6d6f722d 	sfmvs	f7, 2, [pc, #-180]!	; f228 <memcmp+0x130>
    f2d8:	00647366 	rsbeq	r7, r4, r6, ror #6
    f2dc:	6374652f 	cmnvs	r4, #197132288	; 0xbc00000
    f2e0:	7665642f 	strbtvc	r6, [r5], -pc, lsr #8
    f2e4:	7379732f 	cmnvc	r9, #-1140850688	; 0xbc000000
    f2e8:	696e695f 	stmdbvs	lr!, {r0, r1, r2, r3, r4, r6, r8, fp, sp, lr}^
    f2ec:	65642e74 	strbvs	r2, [r4, #-3700]!	; 0xe74
    f2f0:	00000076 	andeq	r0, r0, r6, ror r0
    f2f4:	7665642f 	strbtvc	r6, [r5], -pc, lsr #8
    f2f8:	6e6f632f 	cdpvs	3, 6, cr6, cr15, cr15, {1}
    f2fc:	656c6f73 	strbvs	r6, [ip, #-3955]!	; 0xf73
    f300:	00000030 	andeq	r0, r0, r0, lsr r0
    f304:	5f5f5f20 	svcpl	0x005f5f20
    f308:	205f5f5f 	subscs	r5, pc, pc, asr pc	; <UNPREDICTABLE>
    f30c:	20202020 	eorcs	r2, r0, r0, lsr #32
    f310:	20202020 	eorcs	r2, r0, r0, lsr #32
    f314:	5f5f2020 	svcpl	0x005f2020
    f318:	5f5f5f5f 	svcpl	0x005f5f5f
    f31c:	205f2020 	subscs	r2, pc, r0, lsr #32
    f320:	5f202020 	svcpl	0x00202020
    f324:	5f202020 	svcpl	0x00202020
    f328:	5f5f5f5f 	svcpl	0x005f5f5f
    f32c:	5f20205f 	svcpl	0x0020205f
    f330:	5f5f5f5f 	svcpl	0x005f5f5f
    f334:	280a205f 	stmdacs	sl, {r0, r1, r2, r3, r4, r6, sp}
    f338:	5f5f2020 	svcpl	0x005f2020
    f33c:	7c5c205f 	mrrcvc	0, 5, r2, ip, cr15
    f340:	2020205c 	eorcs	r2, r0, ip, asr r0
    f344:	7c2f2020 	stcvc	0, cr2, [pc], #-128	; f2cc <memcmp+0x1d4>
    f348:	5f202028 	svcpl	0x00202028
    f34c:	2920205f 	stmdbcs	r0!, {r0, r1, r2, r3, r4, r6, sp}
    f350:	205c207c 	subscs	r2, ip, ip, ror r0
    f354:	5c202f20 	stcpl	15, cr2, [r0], #-128	; 0xffffff80
    f358:	20202820 	eorcs	r2, r0, r0, lsr #16
    f35c:	20205f5f 	eorcs	r5, r0, pc, asr pc
    f360:	20202829 	eorcs	r2, r0, r9, lsr #16
    f364:	205f5f5f 	subscs	r5, pc, pc, asr pc	; <UNPREDICTABLE>
    f368:	207c0a5c 	rsbscs	r0, ip, ip, asr sl
    f36c:	205f5f28 	subscs	r5, pc, r8, lsr #30
    f370:	207c2020 	rsbscs	r2, ip, r0, lsr #32
    f374:	205f207c 	subscs	r2, pc, ip, ror r0	; <UNPREDICTABLE>
    f378:	7c7c207c 	ldclvc	0, cr2, [ip], #-496	; 0xfffffe10
    f37c:	20207c20 	eorcs	r7, r0, r0, lsr #24
    f380:	7c7c207c 	ldclvc	0, cr2, [ip], #-496	; 0xfffffe10
    f384:	2f5f2820 	svccs	0x005f2820
    f388:	202f2020 	eorcs	r2, pc, r0, lsr #32
    f38c:	207c207c 	rsbscs	r2, ip, ip, ror r0
    f390:	7c207c20 	stcvc	12, cr7, [r0], #-128	; 0xffffff80
    f394:	5f28207c 	svcpl	0x0028207c
    f398:	0a5f5f5f 	beq	17e711c <_stack+0x176711c>
    f39c:	5f20207c 	svcpl	0x0020207c
    f3a0:	2020295f 	eorcs	r2, r0, pc, asr r9
    f3a4:	287c207c 	ldmdacs	ip!, {r2, r3, r4, r5, r6, sp}^
    f3a8:	207c2920 	rsbscs	r2, ip, r0, lsr #18
    f3ac:	7c207c7c 	stcvc	12, cr7, [r0], #-496	; 0xfffffe10
    f3b0:	207c2020 	rsbscs	r2, ip, r0, lsr #32
    f3b4:	20207c7c 	eorcs	r7, r0, ip, ror ip
    f3b8:	2820205f 	stmdacs	r0!, {r0, r1, r2, r3, r4, r6, sp}
    f3bc:	207c2020 	rsbscs	r2, ip, r0, lsr #32
    f3c0:	7c20207c 	stcvc	0, cr2, [r0], #-496	; 0xfffffe10
    f3c4:	5f287c20 	svcpl	0x00287c20
    f3c8:	205f5f5f 	subscs	r5, pc, pc, asr pc	; <UNPREDICTABLE>
    f3cc:	7c0a2920 	stcvc	9, cr2, [sl], {32}
    f3d0:	5f5f2820 	svcpl	0x005f2820
    f3d4:	7c20205f 	stcvc	0, cr2, [r0], #-380	; 0xfffffe84
    f3d8:	207c7c20 	rsbscs	r7, ip, r0, lsr #24
    f3dc:	7c207c7c 	stcvc	12, cr7, [r0], #-496	; 0xfffffe10
    f3e0:	5f7c207c 	svcpl	0x007c207c
    f3e4:	7c207c5f 	stcvc	12, cr7, [r0], #-380	; 0xfffffe84
    f3e8:	2028207c 	eorcs	r2, r8, ip, ror r0
    f3ec:	5c20205c 	stcpl	0, cr2, [r0], #-368	; 0xfffffe90
    f3f0:	7c207c20 	stcvc	12, cr7, [r0], #-128	; 0xffffff80
    f3f4:	207c5f5f 	rsbscs	r5, ip, pc, asr pc
    f3f8:	5f20207c 	svcpl	0x0020207c
    f3fc:	20295f5f 	eorcs	r5, r9, pc, asr pc
    f400:	5f280a7c 	svcpl	0x00280a7c
    f404:	5f5f5f5f 	svcpl	0x005f5f5f
    f408:	5f282f5f 	svcpl	0x00282f5f
    f40c:	5f5f5f5f 	svcpl	0x005f5f5f
    f410:	28295f5f 	stmdacs	r9!, {r0, r1, r2, r3, r4, r6, r8, r9, sl, fp, ip, lr}
    f414:	5f5f5f5f 	svcpl	0x005f5f5f
    f418:	7c295f5f 	stcvc	15, cr5, [r9], #-380	; 0xfffffe84
    f41c:	20202f5f 	eorcs	r2, r0, pc, asr pc
    f420:	202f5f5c 	eorcs	r5, pc, ip, asr pc	; <UNPREDICTABLE>
    f424:	5f5f5f28 	svcpl	0x005f5f28
    f428:	295f5f5f 	ldmdbcs	pc, {r0, r1, r2, r3, r4, r6, r8, r9, sl, fp, ip, lr}^	; <UNPREDICTABLE>
    f42c:	5f5f5f5c 	svcpl	0x005f5f5c
    f430:	295f5f5f 	ldmdbcs	pc, {r0, r1, r2, r3, r4, r6, r8, r9, sl, fp, ip, lr}^	; <UNPREDICTABLE>
    f434:	00000a0a 	andeq	r0, r0, sl, lsl #20
    f438:	7665642f 	strbtvc	r6, [r5], -pc, lsr #8
    f43c:	7974742f 	ldmdbvc	r4!, {r0, r1, r2, r3, r5, sl, ip, sp, lr}^
    f440:	00000030 	andeq	r0, r0, r0, lsr r0
    f444:	6374652f 	cmnvs	r4, #197132288	; 0xbc00000
    f448:	7665642f 	strbtvc	r6, [r5], -pc, lsr #8
    f44c:	7379732f 	cmnvc	r9, #-1140850688	; 0xbc000000
    f450:	7665642e 	strbtvc	r6, [r5], -lr, lsr #8
    f454:	00000000 	andeq	r0, r0, r0
    f458:	6374652f 	cmnvs	r4, #197132288	; 0xbc00000
    f45c:	696e692f 	stmdbvs	lr!, {r0, r1, r2, r3, r5, r8, fp, sp, lr}^
    f460:	64722e74 	ldrbtvs	r2, [r2], #-3700	; 0xe74
    f464:	00000000 	andeq	r0, r0, r0
    f468:	0000002f 	andeq	r0, r0, pc, lsr #32
    f46c:	5f637069 	svcpl	0x00637069
    f470:	76726573 			; <UNDEFINED> instruction: 0x76726573
    f474:	7366762e 	cmnvc	r6, #48234496	; 0x2e00000
    f478:	00000000 	andeq	r0, r0, r0
    f47c:	20676572 	rsbcs	r6, r7, r2, ror r5
    f480:	20736676 	rsbscs	r6, r3, r6, ror r6
    f484:	5f637069 	svcpl	0x00637069
    f488:	76726573 			; <UNDEFINED> instruction: 0x76726573
    f48c:	72726520 	rsbsvc	r6, r2, #32, 10	; 0x8000000
    f490:	0a21726f 	beq	86be54 <_stack+0x7ebe54>
    f494:	00000000 	andeq	r0, r0, r0
    f498:	746f6f72 	strbtvc	r6, [pc], #-3954	; f4a0 <memcmp+0x3a8>
    f49c:	6d287366 	stcvs	3, cr7, [r8, #-408]!	; 0xfffffe68
    f4a0:	00296d65 	eoreq	r6, r9, r5, ror #26
    f4a4:	0000002e 	andeq	r0, r0, lr, lsr #32
    f4a8:	00002e2e 	andeq	r2, r0, lr, lsr #28
    f4ac:	73696874 	cmnvc	r9, #116, 16	; 0x740000
    f4b0:	6f727020 	svcvs	0x00727020
    f4b4:	73736563 	cmnvc	r3, #415236096	; 0x18c00000
    f4b8:	6e616320 	cdpvs	3, 6, cr6, cr1, cr0, {1}
    f4bc:	6c6e6f20 	stclvs	15, cr6, [lr], #-128	; 0xffffff80
    f4c0:	6f6c2079 	svcvs	0x006c2079
    f4c4:	64656461 	strbtvs	r6, [r5], #-1121	; 0x461
    f4c8:	20796220 	rsbscs	r6, r9, r0, lsr #4
    f4cc:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
    f4d0:	0a216c65 	beq	86a66c <_stack+0x7ea66c>
    f4d4:	00000000 	andeq	r0, r0, r0
    f4d8:	2020200a 	eorcs	r2, r0, sl
    f4dc:	696e6920 	stmdbvs	lr!, {r5, r8, fp, sp, lr}^
    f4e0:	64732074 	ldrbtvs	r2, [r3], #-116	; 0x74
    f4e4:	2e2e2063 	cdpcs	0, 2, cr2, cr14, cr3, {3}
    f4e8:	0000202e 	andeq	r2, r0, lr, lsr #32
    f4ec:	6c696166 	stfvse	f6, [r9], #-408	; 0xfffffe68
    f4f0:	0a216465 	beq	86868c <_stack+0x7e868c>
    f4f4:	00000000 	andeq	r0, r0, r0
    f4f8:	0a2e6b6f 	beq	baa2bc <_stack+0xb2a2bc>
    f4fc:	00000000 	andeq	r0, r0, r0
    f500:	20202020 	eorcs	r2, r0, r0, lsr #32
    f504:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
    f508:	74786520 	ldrbtvc	r6, [r8], #-1312	; 0x520
    f50c:	73662032 	cmnvc	r6, #50	; 0x32
    f510:	2e2e2e20 	cdpcs	14, 2, cr2, cr14, cr0, {1}
    f514:	00000020 	andeq	r0, r0, r0, lsr #32
    f518:	746f6f72 	strbtvc	r6, [pc], #-3954	; f520 <memcmp+0x428>
    f51c:	65287366 	strvs	r7, [r8, #-870]!	; 0x366
    f520:	29327478 	ldmdbcs	r2!, {r3, r4, r5, r6, sl, ip, sp, lr}
    f524:	00000000 	andeq	r0, r0, r0
    f528:	70736172 	rsbsvc	r6, r3, r2, ror r1
    f52c:	00000069 	andeq	r0, r0, r9, rrx

0000f530 <crc32_tab>:
    f530:	00000000 	andeq	r0, r0, r0
    f534:	77073096 			; <UNDEFINED> instruction: 0x77073096
    f538:	ee0e612c 	adfep	f6, f6, #4.0
    f53c:	990951ba 	stmdbls	r9, {r1, r3, r4, r5, r7, r8, ip, lr}
    f540:	076dc419 			; <UNDEFINED> instruction: 0x076dc419
    f544:	706af48f 	rsbvc	pc, sl, pc, lsl #9
    f548:	e963a535 	stmdb	r3!, {r0, r2, r4, r5, r8, sl, sp, pc}^
    f54c:	9e6495a3 	cdpls	5, 6, cr9, cr4, cr3, {5}
    f550:	0edb8832 	mrceq	8, 6, r8, cr11, cr2, {1}
    f554:	79dcb8a4 	ldmibvc	ip, {r2, r5, r7, fp, ip, sp, pc}^
    f558:	e0d5e91e 	sbcs	lr, r5, lr, lsl r9
    f55c:	97d2d988 	ldrbls	sp, [r2, r8, lsl #19]
    f560:	09b64c2b 	ldmibeq	r6!, {r0, r1, r3, r5, sl, fp, lr}
    f564:	7eb17cbd 	mrcvc	12, 5, r7, cr1, cr13, {5}
    f568:	e7b82d07 	ldr	r2, [r8, r7, lsl #26]!
    f56c:	90bf1d91 	umlalsls	r1, pc, r1, sp	; <UNPREDICTABLE>
    f570:	1db71064 	ldcne	0, cr1, [r7, #400]!	; 0x190
    f574:	6ab020f2 	bvs	fec17944 <_stack+0xfeb97944>
    f578:	f3b97148 	vceq.i32	<illegal reg q3.5>, q4, #0
    f57c:	84be41de 	ldrthi	r4, [lr], #478	; 0x1de
    f580:	1adad47d 	bne	ff6c477c <_stack+0xff64477c>
    f584:	6ddde4eb 	cfldrdvs	mvd14, [sp, #940]	; 0x3ac
    f588:	f4d4b551 			; <UNDEFINED> instruction: 0xf4d4b551
    f58c:	83d385c7 	bicshi	r8, r3, #834666496	; 0x31c00000
    f590:	136c9856 	cmnne	ip, #5636096	; 0x560000
    f594:	646ba8c0 	strbtvs	sl, [fp], #-2240	; 0x8c0
    f598:	fd62f97a 	stc2l	9, cr15, [r2, #-488]!	; 0xfffffe18
    f59c:	8a65c9ec 	bhi	1981d54 <_stack+0x1901d54>
    f5a0:	14015c4f 	strne	r5, [r1], #-3151	; 0xc4f
    f5a4:	63066cd9 	movwvs	r6, #27865	; 0x6cd9
    f5a8:	fa0f3d63 	blx	3deb3c <_stack+0x35eb3c>
    f5ac:	8d080df5 	stchi	13, cr0, [r8, #-980]	; 0xfffffc2c
    f5b0:	3b6e20c8 	blcc	1b978d8 <_stack+0x1b178d8>
    f5b4:	4c69105e 	stclmi	0, cr1, [r9], #-376	; 0xfffffe88
    f5b8:	d56041e4 	strble	r4, [r0, #-484]!	; 0x1e4
    f5bc:	a2677172 	rsbge	r7, r7, #-2147483620	; 0x8000001c
    f5c0:	3c03e4d1 	cfstrscc	mvf14, [r3], {209}	; 0xd1
    f5c4:	4b04d447 	blmi	1446e8 <_stack+0xc46e8>
    f5c8:	d20d85fd 	andle	r8, sp, #1061158912	; 0x3f400000
    f5cc:	a50ab56b 	strge	fp, [sl, #-1387]	; 0x56b
    f5d0:	35b5a8fa 	ldrcc	sl, [r5, #2298]!	; 0x8fa
    f5d4:	42b2986c 	adcsmi	r9, r2, #108, 16	; 0x6c0000
    f5d8:	dbbbc9d6 	blle	fef01d38 <_stack+0xfee81d38>
    f5dc:	acbcf940 	ldcge	9, cr15, [ip], #256	; 0x100
    f5e0:	32d86ce3 	sbcscc	r6, r8, #58112	; 0xe300
    f5e4:	45df5c75 	ldrbmi	r5, [pc, #3189]	; 10261 <crc32_tab+0xd31>
    f5e8:	dcd60dcf 	ldclle	13, cr0, [r6], {207}	; 0xcf
    f5ec:	abd13d59 	blge	ff45eb58 <_stack+0xff3deb58>
    f5f0:	26d930ac 	ldrbcs	r3, [r9], ip, lsr #1
    f5f4:	51de003a 	bicspl	r0, lr, sl, lsr r0
    f5f8:	c8d75180 	ldmgt	r7, {r7, r8, ip, lr}^
    f5fc:	bfd06116 	svclt	0x00d06116
    f600:	21b4f4b5 			; <UNDEFINED> instruction: 0x21b4f4b5
    f604:	56b3c423 	ldrtpl	ip, [r3], r3, lsr #8
    f608:	cfba9599 	svcgt	0x00ba9599
    f60c:	b8bda50f 	poplt	{r0, r1, r2, r3, r8, sl, sp, pc}
    f610:	2802b89e 	stmdacs	r2, {r1, r2, r3, r4, r7, fp, ip, sp, pc}
    f614:	5f058808 	svcpl	0x00058808
    f618:	c60cd9b2 			; <UNDEFINED> instruction: 0xc60cd9b2
    f61c:	b10be924 	tstlt	fp, r4, lsr #18
    f620:	2f6f7c87 	svccs	0x006f7c87
    f624:	58684c11 	stmdapl	r8!, {r0, r4, sl, fp, lr}^
    f628:	c1611dab 	cmngt	r1, fp, lsr #27
    f62c:	b6662d3d 			; <UNDEFINED> instruction: 0xb6662d3d
    f630:	76dc4190 			; <UNDEFINED> instruction: 0x76dc4190
    f634:	01db7106 	bicseq	r7, fp, r6, lsl #2
    f638:	98d220bc 	ldmls	r2, {r2, r3, r4, r5, r7, sp}^
    f63c:	efd5102a 	svc	0x00d5102a
    f640:	71b18589 			; <UNDEFINED> instruction: 0x71b18589
    f644:	06b6b51f 	ssateq	fp, #23, pc, lsl #10	; <UNPREDICTABLE>
    f648:	9fbfe4a5 	svcls	0x00bfe4a5
    f64c:	e8b8d433 	ldm	r8!, {r0, r1, r4, r5, sl, ip, lr, pc}
    f650:	7807c9a2 	stmdavc	r7, {r1, r5, r7, r8, fp, lr, pc}
    f654:	0f00f934 	svceq	0x0000f934
    f658:	9609a88e 	strls	sl, [r9], -lr, lsl #17
    f65c:	e10e9818 	tst	lr, r8, lsl r8
    f660:	7f6a0dbb 	svcvc	0x006a0dbb
    f664:	086d3d2d 	stmdaeq	sp!, {r0, r2, r3, r5, r8, sl, fp, ip, sp}^
    f668:	91646c97 			; <UNDEFINED> instruction: 0x91646c97
    f66c:	e6635c01 	strbt	r5, [r3], -r1, lsl #24
    f670:	6b6b51f4 	blvs	1ae3e48 <_stack+0x1a63e48>
    f674:	1c6c6162 	stfnee	f6, [ip], #-392	; 0xfffffe78
    f678:	856530d8 	strbhi	r3, [r5, #-216]!	; 0xd8
    f67c:	f262004e 	vhadd.s32	q8, q1, q7
    f680:	6c0695ed 	cfstr32vs	mvfx9, [r6], {237}	; 0xed
    f684:	1b01a57b 	blne	78c78 <__bss_end__+0x2b818>
    f688:	8208f4c1 	andhi	pc, r8, #-1056964608	; 0xc1000000
    f68c:	f50fc457 			; <UNDEFINED> instruction: 0xf50fc457
    f690:	65b0d9c6 	ldrvs	sp, [r0, #2502]!	; 0x9c6
    f694:	12b7e950 	adcsne	lr, r7, #80, 18	; 0x140000
    f698:	8bbeb8ea 	blhi	fefbda48 <_stack+0xfef3da48>
    f69c:	fcb9887c 	ldc2	8, cr8, [r9], #496	; 0x1f0
    f6a0:	62dd1ddf 	sbcsvs	r1, sp, #14272	; 0x37c0
    f6a4:	15da2d49 	ldrbne	r2, [sl, #3401]	; 0xd49
    f6a8:	8cd37cf3 	ldclhi	12, cr7, [r3], {243}	; 0xf3
    f6ac:	fbd44c65 	blx	ff52284a <_stack+0xff4a284a>
    f6b0:	4db26158 	ldfmis	f6, [r2, #352]!	; 0x160
    f6b4:	3ab551ce 	bcc	fed63df4 <_stack+0xfece3df4>
    f6b8:	a3bc0074 			; <UNDEFINED> instruction: 0xa3bc0074
    f6bc:	d4bb30e2 	ldrtle	r3, [fp], #226	; 0xe2
    f6c0:	4adfa541 	bmi	ff7f8bcc <_stack+0xff778bcc>
    f6c4:	3dd895d7 	cfldr64cc	mvdx9, [r8, #860]	; 0x35c
    f6c8:	a4d1c46d 	ldrbge	ip, [r1], #1133	; 0x46d
    f6cc:	d3d6f4fb 	bicsle	pc, r6, #-83886080	; 0xfb000000
    f6d0:	4369e96a 	cmnmi	r9, #1736704	; 0x1a8000
    f6d4:	346ed9fc 	strbtcc	sp, [lr], #-2556	; 0x9fc
    f6d8:	ad678846 	stclge	8, cr8, [r7, #-280]!	; 0xfffffee8
    f6dc:	da60b8d0 	ble	183da24 <_stack+0x17bda24>
    f6e0:	44042d73 	strmi	r2, [r4], #-3443	; 0xd73
    f6e4:	33031de5 	movwcc	r1, #15845	; 0x3de5
    f6e8:	aa0a4c5f 	bge	2a286c <_stack+0x22286c>
    f6ec:	dd0d7cc9 	stcle	12, cr7, [sp, #-804]	; 0xfffffcdc
    f6f0:	5005713c 	andpl	r7, r5, ip, lsr r1
    f6f4:	270241aa 	strcs	r4, [r2, -sl, lsr #3]
    f6f8:	be0b1010 	mcrlt	0, 0, r1, cr11, cr0, {0}
    f6fc:	c90c2086 	stmdbgt	ip, {r1, r2, r7, sp}
    f700:	5768b525 	strbpl	fp, [r8, -r5, lsr #10]!
    f704:	206f85b3 	strhtcs	r8, [pc], #-83
    f708:	b966d409 	stmdblt	r6!, {r0, r3, sl, ip, lr, pc}^
    f70c:	ce61e49f 	mcrgt	4, 3, lr, cr1, cr15, {4}
    f710:	5edef90e 	cdppl	9, 13, cr15, cr14, cr14, {0}
    f714:	29d9c998 	ldmibcs	r9, {r3, r4, r7, r8, fp, lr, pc}^
    f718:	b0d09822 	sbcslt	r9, r0, r2, lsr #16
    f71c:	c7d7a8b4 			; <UNDEFINED> instruction: 0xc7d7a8b4
    f720:	59b33d17 	ldmibpl	r3!, {r0, r1, r2, r4, r8, sl, fp, ip, sp}
    f724:	2eb40d81 	cdpcs	13, 11, cr0, cr4, cr1, {4}
    f728:	b7bd5c3b 			; <UNDEFINED> instruction: 0xb7bd5c3b
    f72c:	c0ba6cad 	adcsgt	r6, sl, sp, lsr #25
    f730:	edb88320 	ldc	3, cr8, [r8, #128]!	; 0x80
    f734:	9abfb3b6 	bls	feffc614 <_stack+0xfef7c614>
    f738:	03b6e20c 			; <UNDEFINED> instruction: 0x03b6e20c
    f73c:	74b1d29a 	ldrtvc	sp, [r1], #666	; 0x29a
    f740:	ead54739 	b	ff56142c <_stack+0xff4e142c>
    f744:	9dd277af 	ldclls	7, cr7, [r2, #700]	; 0x2bc
    f748:	04db2615 	ldrbeq	r2, [fp], #1557	; 0x615
    f74c:	73dc1683 	bicsvc	r1, ip, #137363456	; 0x8300000
    f750:	e3630b12 	cmn	r3, #18432	; 0x4800
    f754:	94643b84 	strbtls	r3, [r4], #-2948	; 0xb84
    f758:	0d6d6a3e 	vpusheq	{s13-s74}
    f75c:	7a6a5aa8 	bvc	1aa6204 <_stack+0x1a26204>
    f760:	e40ecf0b 	str	ip, [lr], #-3851	; 0xf0b
    f764:	9309ff9d 	movwls	pc, #40861	; 0x9f9d	; <UNPREDICTABLE>
    f768:	0a00ae27 	beq	3b00c <_proc_fds_table+0x230a8>
    f76c:	7d079eb1 	stcvc	14, cr9, [r7, #-708]	; 0xfffffd3c
    f770:	f00f9344 			; <UNDEFINED> instruction: 0xf00f9344
    f774:	8708a3d2 			; <UNDEFINED> instruction: 0x8708a3d2
    f778:	1e01f268 	cdpne	2, 0, cr15, cr1, cr8, {3}
    f77c:	6906c2fe 	stmdbvs	r6, {r1, r2, r3, r4, r5, r6, r7, r9, lr, pc}
    f780:	f762575d 			; <UNDEFINED> instruction: 0xf762575d
    f784:	806567cb 	rsbhi	r6, r5, fp, asr #15
    f788:	196c3671 	stmdbne	ip!, {r0, r4, r5, r6, r9, sl, ip, sp}^
    f78c:	6e6b06e7 	cdpvs	6, 6, cr0, cr11, cr7, {7}
    f790:	fed41b76 	mrc2	11, 6, r1, cr4, cr6, {3}
    f794:	89d32be0 	ldmibhi	r3, {r5, r6, r7, r8, r9, fp, sp}^
    f798:	10da7a5a 	sbcsne	r7, sl, sl, asr sl
    f79c:	67dd4acc 	ldrbvs	r4, [sp, ip, asr #21]
    f7a0:	f9b9df6f 			; <UNDEFINED> instruction: 0xf9b9df6f
    f7a4:	8ebeeff9 	mrchi	15, 5, lr, cr14, cr9, {7}
    f7a8:	17b7be43 	ldrne	fp, [r7, r3, asr #28]!
    f7ac:	60b08ed5 	ldrsbtvs	r8, [r0], r5
    f7b0:	d6d6a3e8 	ldrble	sl, [r6], r8, ror #7
    f7b4:	a1d1937e 	bicsge	r9, r1, lr, ror r3
    f7b8:	38d8c2c4 	ldmcc	r8, {r2, r6, r7, r9, lr, pc}^
    f7bc:	4fdff252 	svcmi	0x00dff252
    f7c0:	d1bb67f1 			; <UNDEFINED> instruction: 0xd1bb67f1
    f7c4:	a6bc5767 	ldrtge	r5, [ip], r7, ror #14
    f7c8:	3fb506dd 	svccc	0x00b506dd
    f7cc:	48b2364b 	ldmmi	r2!, {r0, r1, r3, r6, r9, sl, ip, sp}
    f7d0:	d80d2bda 	stmdale	sp, {r1, r3, r4, r6, r7, r8, r9, fp, sp}
    f7d4:	af0a1b4c 	svcge	0x000a1b4c
    f7d8:	36034af6 			; <UNDEFINED> instruction: 0x36034af6
    f7dc:	41047a60 	tstmi	r4, r0, ror #20
    f7e0:	df60efc3 	svcle	0x0060efc3
    f7e4:	a867df55 	stmdage	r7!, {r0, r2, r4, r6, r8, r9, sl, fp, ip, lr, pc}^
    f7e8:	316e8eef 	cmncc	lr, pc, ror #29
    f7ec:	4669be79 			; <UNDEFINED> instruction: 0x4669be79
    f7f0:	cb61b38c 	blgt	187c628 <_stack+0x17fc628>
    f7f4:	bc66831a 	stcllt	3, cr8, [r6], #-104	; 0xffffff98
    f7f8:	256fd2a0 	strbcs	sp, [pc, #-672]!	; f560 <crc32_tab+0x30>
    f7fc:	5268e236 	rsbpl	lr, r8, #1610612739	; 0x60000003
    f800:	cc0c7795 	stcgt	7, cr7, [ip], {149}	; 0x95
    f804:	bb0b4703 	bllt	2e1418 <_stack+0x261418>
    f808:	220216b9 	andcs	r1, r2, #193986560	; 0xb900000
    f80c:	5505262f 	strpl	r2, [r5, #-1583]	; 0x62f
    f810:	c5ba3bbe 	ldrgt	r3, [sl, #3006]!	; 0xbbe
    f814:	b2bd0b28 	adcslt	r0, sp, #40, 22	; 0xa000
    f818:	2bb45a92 	blcs	fed26268 <_stack+0xfeca6268>
    f81c:	5cb36a04 	vldmiapl	r3!, {s12-s15}
    f820:	c2d7ffa7 	sbcsgt	pc, r7, #668	; 0x29c
    f824:	b5d0cf31 	ldrblt	ip, [r0, #3889]	; 0xf31
    f828:	2cd99e8b 	ldclcs	14, cr9, [r9], {139}	; 0x8b
    f82c:	5bdeae1d 	blpl	ff7bb0a8 <_stack+0xff73b0a8>
    f830:	9b64c2b0 	blls	19402f8 <_stack+0x18c02f8>
    f834:	ec63f226 	sfm	f7, 3, [r3], #-152	; 0xffffff68
    f838:	756aa39c 	strbvc	sl, [sl, #-924]!	; 0x39c
    f83c:	026d930a 	rsbeq	r9, sp, #671088640	; 0x28000000
    f840:	9c0906a9 	stcls	6, cr0, [r9], {169}	; 0xa9
    f844:	eb0e363f 	bl	39d148 <_stack+0x31d148>
    f848:	72076785 	andvc	r6, r7, #34865152	; 0x2140000
    f84c:	05005713 	streq	r5, [r0, #-1811]	; 0x713
    f850:	95bf4a82 	ldrls	r4, [pc, #2690]!	; 102da <crc32_tab+0xdaa>
    f854:	e2b87a14 	adcs	r7, r8, #20, 20	; 0x14000
    f858:	7bb12bae 	blvc	fec5a718 <_stack+0xfebda718>
    f85c:	0cb61b38 	vldmiaeq	r6!, {d1-d28}
    f860:	92d28e9b 	sbcsls	r8, r2, #2480	; 0x9b0
    f864:	e5d5be0d 	ldrb	fp, [r5, #3597]	; 0xe0d
    f868:	7cdcefb7 	ldclvc	15, cr14, [ip], {183}	; 0xb7
    f86c:	0bdbdf21 	bleq	ff7074f8 <_stack+0xff6874f8>
    f870:	86d3d2d4 			; <UNDEFINED> instruction: 0x86d3d2d4
    f874:	f1d4e242 			; <UNDEFINED> instruction: 0xf1d4e242
    f878:	68ddb3f8 	ldmvs	sp, {r3, r4, r5, r6, r7, r8, r9, ip, sp, pc}^
    f87c:	1fda836e 	svcne	0x00da836e
    f880:	81be16cd 			; <UNDEFINED> instruction: 0x81be16cd
    f884:	f6b9265b 			; <UNDEFINED> instruction: 0xf6b9265b
    f888:	6fb077e1 	svcvs	0x00b077e1
    f88c:	18b74777 	ldmne	r7!, {r0, r1, r2, r4, r5, r6, r8, r9, sl, lr}
    f890:	88085ae6 	stmdahi	r8, {r1, r2, r5, r6, r7, r9, fp, ip, lr}
    f894:	ff0f6a70 			; <UNDEFINED> instruction: 0xff0f6a70
    f898:	66063bca 	strvs	r3, [r6], -sl, asr #23
    f89c:	11010b5c 	tstne	r1, ip, asr fp
    f8a0:	8f659eff 	svchi	0x00659eff
    f8a4:	f862ae69 			; <UNDEFINED> instruction: 0xf862ae69
    f8a8:	616bffd3 	ldrdvs	pc, [fp, #-243]!	; 0xffffff0d
    f8ac:	166ccf45 	strbtne	ip, [ip], -r5, asr #30
    f8b0:	a00ae278 	andge	lr, sl, r8, ror r2
    f8b4:	d70dd2ee 	strle	sp, [sp, -lr, ror #5]
    f8b8:	4e048354 	mcrmi	3, 0, r8, cr4, cr4, {2}
    f8bc:	3903b3c2 	stmdbcc	r3, {r1, r6, r7, r8, r9, ip, sp, pc}
    f8c0:	a7672661 	strbge	r2, [r7, -r1, ror #12]!
    f8c4:	d06016f7 	strdle	r1, [r0], #-103	; 0xffffff99	; <UNPREDICTABLE>
    f8c8:	4969474d 	stmdbmi	r9!, {r0, r2, r3, r6, r8, r9, sl, lr}^
    f8cc:	3e6e77db 	mcrcc	7, 3, r7, cr14, cr11, {6}
    f8d0:	aed16a4a 	vfnmage.f32	s13, s2, s20
    f8d4:	d9d65adc 	ldmible	r6, {r2, r3, r4, r6, r7, r9, fp, ip, lr}^
    f8d8:	40df0b66 	sbcsmi	r0, pc, r6, ror #22
    f8dc:	37d83bf0 			; <UNDEFINED> instruction: 0x37d83bf0
    f8e0:	a9bcae53 	ldmibge	ip!, {r0, r1, r4, r6, r9, sl, fp, sp, pc}
    f8e4:	debb9ec5 	cdple	14, 11, cr9, cr11, cr5, {6}
    f8e8:	47b2cf7f 			; <UNDEFINED> instruction: 0x47b2cf7f
    f8ec:	30b5ffe9 	adcscc	pc, r5, r9, ror #31
    f8f0:	bdbdf21c 	lfmlt	f7, 1, [sp, #112]!	; 0x70
    f8f4:	cabac28a 	bgt	feec0324 <_stack+0xfee40324>
    f8f8:	53b39330 			; <UNDEFINED> instruction: 0x53b39330
    f8fc:	24b4a3a6 	ldrtcs	sl, [r4], #934	; 0x3a6
    f900:	bad03605 	blt	ff41d11c <_stack+0xff39d11c>
    f904:	cdd70693 	ldclgt	6, cr0, [r7, #588]	; 0x24c
    f908:	54de5729 	ldrbpl	r5, [lr], #1833	; 0x729
    f90c:	23d967bf 	bicscs	r6, r9, #50069504	; 0x2fc0000
    f910:	b3667a2e 	cmnlt	r6, #188416	; 0x2e000
    f914:	c4614ab8 	strbtgt	r4, [r1], #-2744	; 0xab8
    f918:	5d681b02 	vstmdbpl	r8!, {d17}
    f91c:	2a6f2b94 	bcs	1bda774 <_stack+0x1b5a774>
    f920:	b40bbe37 	strlt	fp, [fp], #-3639	; 0xe37
    f924:	c30c8ea1 	movwgt	r8, #52897	; 0xcea1
    f928:	5a05df1b 	bpl	18759c <_stack+0x10759c>
    f92c:	2d02ef8d 	stccs	15, cr14, [r2, #-564]	; 0xfffffdcc
    f930:	7778797a 			; <UNDEFINED> instruction: 0x7778797a
    f934:	73747576 	cmnvc	r4, #494927872	; 0x1d800000
    f938:	6f707172 	svcvs	0x00707172
    f93c:	6b6c6d6e 	blvs	1b2aefc <_stack+0x1aaaefc>
    f940:	6768696a 	strbvs	r6, [r8, -sl, ror #18]!
    f944:	63646566 	cmnvs	r4, #427819008	; 0x19800000
    f948:	38396162 	ldmdacc	r9!, {r1, r5, r6, r8, sp, lr}
    f94c:	34353637 	ldrtcc	r3, [r5], #-1591	; 0x637
    f950:	30313233 	eorscc	r3, r1, r3, lsr r2
    f954:	34333231 	ldrtcc	r3, [r3], #-561	; 0x231
    f958:	38373635 	ldmdacc	r7!, {r0, r2, r4, r5, r9, sl, ip, sp}
    f95c:	63626139 	cmnvs	r2, #1073741838	; 0x4000000e
    f960:	67666564 	strbvs	r6, [r6, -r4, ror #10]!
    f964:	6b6a6968 	blvs	1aa9f0c <_stack+0x1a29f0c>
    f968:	6f6e6d6c 	svcvs	0x006e6d6c
    f96c:	73727170 	cmnvc	r2, #112, 2
    f970:	77767574 			; <UNDEFINED> instruction: 0x77767574
    f974:	007a7978 	rsbseq	r7, sl, r8, ror r9
    f978:	65757274 	ldrbvs	r7, [r5, #-628]!	; 0x274
    f97c:	00000000 	andeq	r0, r0, r0
    f980:	736c6166 	cmnvc	ip, #-2147483623	; 0x80000019
    f984:	00000065 	andeq	r0, r0, r5, rrx
    f988:	00007830 	andeq	r7, r0, r0, lsr r8
    f98c:	45555254 	ldrbmi	r5, [r5, #-596]	; 0x254
    f990:	00000000 	andeq	r0, r0, r0
    f994:	33323130 	teqcc	r2, #48, 2
    f998:	37363534 			; <UNDEFINED> instruction: 0x37363534
    f99c:	62613938 	rsbvs	r3, r1, #56, 18	; 0xe0000
    f9a0:	66656463 	strbtvs	r6, [r5], -r3, ror #8
    f9a4:	00000000 	andeq	r0, r0, r0
    f9a8:	33323130 	teqcc	r2, #48, 2
    f9ac:	37363534 			; <UNDEFINED> instruction: 0x37363534
    f9b0:	42413938 	submi	r3, r1, #56, 18	; 0xe0000
    f9b4:	46454443 	strbmi	r4, [r5], -r3, asr #8
    f9b8:	00000000 	andeq	r0, r0, r0

Disassembly of section .eh_frame:

0000f9bc <.eh_frame>:
    f9bc:	00000010 	andeq	r0, r0, r0, lsl r0
    f9c0:	00000000 	andeq	r0, r0, r0
    f9c4:	00527a01 	subseq	r7, r2, r1, lsl #20
    f9c8:	010e7c02 	tsteq	lr, r2, lsl #24
    f9cc:	000d0c1b 	andeq	r0, sp, fp, lsl ip
    f9d0:	0000002c 	andeq	r0, r0, ip, lsr #32
    f9d4:	00000018 	andeq	r0, r0, r8, lsl r0
    f9d8:	ffffe864 			; <UNDEFINED> instruction: 0xffffe864
    f9dc:	00000224 	andeq	r0, r0, r4, lsr #4
    f9e0:	040e6200 	streq	r6, [lr], #-512	; 0x200
    f9e4:	c4660184 	strbtgt	r0, [r6], #-388	; 0x184
    f9e8:	0e56000e 	cdpeq	0, 5, cr0, cr6, cr14, {0}
    f9ec:	85028408 	strhi	r8, [r2, #-1032]	; 0x408
    f9f0:	0a9c0201 	beq	fe7101fc <_stack+0xfe6901fc>
    f9f4:	0ec5c442 	cdpeq	4, 12, cr12, cr5, cr2, {2}
    f9f8:	520b4200 	andpl	r4, fp, #0, 4
    f9fc:	000ec5c4 	andeq	ip, lr, r4, asr #11

Disassembly of section .got:

00017a00 <_GLOBAL_OFFSET_TABLE_>:
	...
   17a0c:	0004d1b8 			; <UNDEFINED> instruction: 0x0004d1b8
   17a10:	000033d0 	ldrdeq	r3, [r0], -r0
   17a14:	00006a74 	andeq	r6, r0, r4, ror sl
   17a18:	0004d3f8 	strdeq	sp, [r4], -r8
   17a1c:	00007374 	andeq	r7, r0, r4, ror r3
   17a20:	00007454 	andeq	r7, r0, r4, asr r4
   17a24:	0004d3f4 	strdeq	sp, [r4], -r4
   17a28:	0004d44c 	andeq	sp, r4, ip, asr #8
   17a2c:	0004d450 	andeq	sp, r4, r0, asr r4
   17a30:	0004d1bc 			; <UNDEFINED> instruction: 0x0004d1bc
   17a34:	0004d454 	andeq	sp, r4, r4, asr r4
   17a38:	000071f4 	strdeq	r7, [r0], -r4
   17a3c:	00000100 	andeq	r0, r0, r0, lsl #2
   17a40:	000055b0 			; <UNDEFINED> instruction: 0x000055b0
   17a44:	0000ab44 	andeq	sl, r0, r4, asr #22
   17a48:	0000342c 	andeq	r3, r0, ip, lsr #8
   17a4c:	0004d1b4 			; <UNDEFINED> instruction: 0x0004d1b4
   17a50:	0004d45c 	andeq	sp, r4, ip, asr r4
   17a54:	000065b8 			; <UNDEFINED> instruction: 0x000065b8

Disassembly of section .data:

00017a58 <__data_start>:
   17a58:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff

00017a5c <_r_mask>:
   17a5c:	13579abc 	cmpne	r7, #188, 20	; 0xbc000

Disassembly of section .bss:

00017a60 <__bss_start>:
	...

00017b60 <_ipc_servs>:
   17b60:	00000000 	andeq	r0, r0, r0

00017b64 <_proc_info_table>:
	...

00017f64 <_proc_fds_table>:
	...

00047f64 <_vfs_mounts>:
	...

0004c064 <_event_tail>:
   4c064:	00000000 	andeq	r0, r0, r0

0004c068 <_event_head>:
   4c068:	00000000 	andeq	r0, r0, r0

0004c06c <_vfs_root>:
   4c06c:	00000000 	andeq	r0, r0, r0

0004c070 <ret.5002>:
	...

0004c0cc <ret.4903>:
	...

0004c2cc <_partition>:
	...

0004c2dc <_sector_buf>:
   4c2dc:	00000000 	andeq	r0, r0, r0

0004c2e0 <_sector_buf_num>:
   4c2e0:	00000000 	andeq	r0, r0, r0

0004c2e4 <sd_read_sector_arch>:
   4c2e4:	00000000 	andeq	r0, r0, r0

0004c2e8 <sd_write_sector_arch>:
   4c2e8:	00000000 	andeq	r0, r0, r0

0004c2ec <_partitions>:
	...

0004c32c <sd_init_arch>:
   4c32c:	00000000 	andeq	r0, r0, r0

0004c330 <buf.4958>:
	...

0004c730 <_sdc>:
	...

0004c93c <_sdc>:
	...

0004cd54 <_str_result>:
	...

0004cd98 <_stdin>:
	...

0004cda0 <_stdout>:
	...

0004cda8 <_stderr>:
	...

0004cdb0 <_cmd>:
	...

0004d1b0 <_off_cmd>:
   4d1b0:	00000000 	andeq	r0, r0, r0

0004d1b4 <stderr>:
   4d1b4:	00000000 	andeq	r0, r0, r0

0004d1b8 <stdout>:
   4d1b8:	00000000 	andeq	r0, r0, r0

0004d1bc <stdin>:
   4d1bc:	00000000 	andeq	r0, r0, r0

0004d1c0 <ret.4889>:
	...

0004d3c0 <_ipc_serv_handle>:
   4d3c0:	00000000 	andeq	r0, r0, r0

0004d3c4 <_signals>:
	...

0004d3d4 <_proto_factor>:
	...

0004d3f0 <_vfsd_pid>:
   4d3f0:	00000000 	andeq	r0, r0, r0

0004d3f4 <_mmio_base>:
   4d3f4:	00000000 	andeq	r0, r0, r0

0004d3f8 <errno>:
   4d3f8:	00000000 	andeq	r0, r0, r0

0004d3fc <ret.4358>:
	...

0004d448 <sd_ocr>:
   4d448:	00000000 	andeq	r0, r0, r0

0004d44c <sd_err>:
   4d44c:	00000000 	andeq	r0, r0, r0

0004d450 <sd_rca>:
   4d450:	00000000 	andeq	r0, r0, r0

0004d454 <sd_scr>:
	...

0004d45c <sd_hv>:
   4d45c:	00000000 	andeq	r0, r0, r0

Disassembly of section .comment:

00000000 <.comment>:
   0:	3a434347 	bcc	10d0d24 <_stack+0x1050d24>
   4:	4e472820 	cdpmi	8, 4, cr2, cr7, cr0, {1}
   8:	6f542055 	svcvs	0x00542055
   c:	20736c6f 	rsbscs	r6, r3, pc, ror #24
  10:	20726f66 	rsbscs	r6, r2, r6, ror #30
  14:	204d5241 	subcs	r5, sp, r1, asr #4
  18:	65626d45 	strbvs	r6, [r2, #-3397]!	; 0xd45
  1c:	64656464 	strbtvs	r6, [r5], #-1124	; 0x464
  20:	6f725020 	svcvs	0x00725020
  24:	73736563 	cmnvc	r3, #415236096	; 0x18c00000
  28:	2973726f 	ldmdbcs	r3!, {r0, r1, r2, r3, r5, r6, r9, ip, sp, lr}^
  2c:	392e3420 	stmdbcc	lr!, {r5, sl, ip, sp}
  30:	3220332e 	eorcc	r3, r0, #-1207959552	; 0xb8000000
  34:	30353130 	eorscc	r3, r5, r0, lsr r1
  38:	20393235 	eorscs	r3, r9, r5, lsr r2
  3c:	6c657228 	sfmvs	f7, 2, [r5], #-160	; 0xffffff60
  40:	65736165 	ldrbvs	r6, [r3, #-357]!	; 0x165
  44:	415b2029 	cmpmi	fp, r9, lsr #32
  48:	652f4d52 	strvs	r4, [pc, #-3410]!	; fffff2fe <_stack+0xfff7f2fe>
  4c:	6465626d 	strbtvs	r6, [r5], #-621	; 0x26d
  50:	2d646564 	cfstr64cs	mvdx6, [r4, #-400]!	; 0xfffffe70
  54:	2d395f34 	ldccs	15, cr5, [r9, #-208]!	; 0xffffff30
  58:	6e617262 	cdpvs	2, 6, cr7, cr1, cr2, {3}
  5c:	72206863 	eorvc	r6, r0, #6488064	; 0x630000
  60:	73697665 	cmnvc	r9, #105906176	; 0x6500000
  64:	206e6f69 	rsbcs	r6, lr, r9, ror #30
  68:	39373232 	ldmdbcc	r7!, {r1, r4, r5, r9, ip, sp}
  6c:	005d3737 	subseq	r3, sp, r7, lsr r7

Disassembly of section .debug_frame:

00000000 <.debug_frame>:
   0:	0000000c 	andeq	r0, r0, ip
   4:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
   8:	7c020001 	stcvc	0, cr0, [r2], {1}
   c:	000d0c0e 	andeq	r0, sp, lr, lsl #24
  10:	00000024 	andeq	r0, r0, r4, lsr #32
  14:	00000000 	andeq	r0, r0, r0
  18:	0000def8 	strdeq	sp, [r0], -r8
  1c:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
  20:	84100e44 	ldrhi	r0, [r0], #-3652	; 0xe44
  24:	86038504 	strhi	r8, [r3], -r4, lsl #10
  28:	02018e02 	andeq	r8, r1, #2, 28
  2c:	c6ce0a6a 	strbgt	r0, [lr], sl, ror #20
  30:	000ec4c5 	andeq	ip, lr, r5, asr #9
  34:	00000b42 	andeq	r0, r0, r2, asr #22
  38:	0000000c 	andeq	r0, r0, ip
  3c:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
  40:	7c020001 	stcvc	0, cr0, [r2], {1}
  44:	000d0c0e 	andeq	r0, sp, lr, lsl #24
  48:	0000001c 	andeq	r0, r0, ip, lsl r0
  4c:	00000038 	andeq	r0, r0, r8, lsr r0
  50:	0000dfe8 	andeq	sp, r0, r8, ror #31
  54:	000000f4 	strdeq	r0, [r0], -r4
  58:	84080e44 	strhi	r0, [r8], #-3652	; 0xe44
  5c:	02018e02 	andeq	r8, r1, #2, 28
  60:	c4ce0a70 	strbgt	r0, [lr], #2672	; 0xa70
  64:	0b42000e 	bleq	10800a4 <_stack+0x10000a4>
  68:	0000000c 	andeq	r0, r0, ip
  6c:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
  70:	7c020001 	stcvc	0, cr0, [r2], {1}
  74:	000d0c0e 	andeq	r0, sp, lr, lsl #24
  78:	00000034 	andeq	r0, r0, r4, lsr r0
  7c:	00000068 	andeq	r0, r0, r8, rrx
  80:	0000e0dc 	ldrdeq	lr, [r0], -ip
  84:	00000160 	andeq	r0, r0, r0, ror #2
  88:	84080e44 	strhi	r0, [r8], #-3652	; 0xe44
  8c:	02018e02 	andeq	r8, r1, #2, 28
  90:	c4ce0a5a 	strbgt	r0, [lr], #2650	; 0xa5a
  94:	0b42000e 	bleq	10800d4 <_stack+0x10000d4>
  98:	cec40a7e 	mcrgt	10, 6, r0, cr4, cr14, {3}
  9c:	0b42000e 	bleq	10800dc <_stack+0x10000dc>
  a0:	cec40a44 	cdpgt	10, 12, cr0, cr4, cr4, {2}
  a4:	0b42000e 	bleq	10800e4 <_stack+0x10000e4>
  a8:	0ecec444 	cdpeq	4, 12, cr12, cr14, cr4, {2}
  ac:	00000000 	andeq	r0, r0, r0
  b0:	0000000c 	andeq	r0, r0, ip
  b4:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
  b8:	7c020001 	stcvc	0, cr0, [r2], {1}
  bc:	000d0c0e 	andeq	r0, sp, lr, lsl #24
  c0:	0000000c 	andeq	r0, r0, ip
  c4:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
  c8:	0000e460 	andeq	lr, r0, r0, ror #8
  cc:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
  d0:	0000000c 	andeq	r0, r0, ip
  d4:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
  d8:	7c020001 	stcvc	0, cr0, [r2], {1}
  dc:	000d0c0e 	andeq	r0, sp, lr, lsl #24
  e0:	0000000c 	andeq	r0, r0, ip
  e4:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
  e8:	0000e550 	andeq	lr, r0, r0, asr r5
  ec:	00000060 	andeq	r0, r0, r0, rrx
  f0:	0000000c 	andeq	r0, r0, ip
  f4:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
  f8:	7c020001 	stcvc	0, cr0, [r2], {1}
  fc:	000d0c0e 	andeq	r0, sp, lr, lsl #24
 100:	00000034 	andeq	r0, r0, r4, lsr r0
 104:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
 108:	0000e5b0 			; <UNDEFINED> instruction: 0x0000e5b0
 10c:	00000118 	andeq	r0, r0, r8, lsl r1
 110:	84100e4a 	ldrhi	r0, [r0], #-3658	; 0xe4a
 114:	86038504 	strhi	r8, [r3], -r4, lsl #10
 118:	02018e02 	andeq	r8, r1, #2, 28
 11c:	c6c5c46a 	strbgt	ip, [r5], sl, ror #8
 120:	46000ece 	strmi	r0, [r0], -lr, asr #29
 124:	0484100e 	streq	r1, [r4], #14
 128:	02860385 	addeq	r0, r6, #335544322	; 0x14000002
 12c:	0a44018e 	beq	110076c <_stack+0x108076c>
 130:	c4c5c6ce 	strbgt	ip, [r5], #1742	; 0x6ce
 134:	0b42000e 	bleq	1080174 <_stack+0x1000174>
 138:	0000000c 	andeq	r0, r0, ip
 13c:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
 140:	7c020001 	stcvc	0, cr0, [r2], {1}
 144:	000d0c0e 	andeq	r0, sp, lr, lsl #24
 148:	00000028 	andeq	r0, r0, r8, lsr #32
 14c:	00000138 	andeq	r0, r0, r8, lsr r1
 150:	0000e6c8 	andeq	lr, r0, r8, asr #13
 154:	000000e4 	andeq	r0, r0, r4, ror #1
 158:	840c0e52 	strhi	r0, [ip], #-3666	; 0xe52
 15c:	8e028503 	cfsh32hi	mvfx8, mvfx2, #3
 160:	0a480201 	beq	120096c <_stack+0x118096c>
 164:	0ec4c5ce 	cdpeq	5, 12, cr12, cr4, cr14, {6}
 168:	500b4200 	andpl	r4, fp, r0, lsl #4
 16c:	0ecec5c4 	cdpeq	5, 12, cr12, cr14, cr4, {6}
 170:	00000000 	andeq	r0, r0, r0
 174:	0000000c 	andeq	r0, r0, ip
 178:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
 17c:	7c020001 	stcvc	0, cr0, [r2], {1}
 180:	000d0c0e 	andeq	r0, sp, lr, lsl #24
 184:	00000028 	andeq	r0, r0, r8, lsr #32
 188:	00000174 	andeq	r0, r0, r4, ror r1
 18c:	0000e7ac 	andeq	lr, r0, ip, lsr #15
 190:	00000114 	andeq	r0, r0, r4, lsl r1
 194:	84180e42 	ldrhi	r0, [r8], #-3650	; 0xe42
 198:	86058506 	strhi	r8, [r5], -r6, lsl #10
 19c:	88038704 	stmdahi	r3, {r2, r8, r9, sl, pc}
 1a0:	02018e02 	andeq	r8, r1, #2, 28
 1a4:	c8ce0a5a 	stmiagt	lr, {r1, r3, r4, r6, r9, fp}^
 1a8:	c4c5c6c7 	strbgt	ip, [r5], #1735	; 0x6c7
 1ac:	0b42000e 	bleq	10801ec <_stack+0x10001ec>
 1b0:	00000050 	andeq	r0, r0, r0, asr r0
 1b4:	00000174 	andeq	r0, r0, r4, ror r1
 1b8:	0000e8c0 	andeq	lr, r0, r0, asr #17
 1bc:	000003a0 	andeq	r0, r0, r0, lsr #7
 1c0:	84240e42 	strthi	r0, [r4], #-3650	; 0xe42
 1c4:	86088509 	strhi	r8, [r8], -r9, lsl #10
 1c8:	88068707 	stmdahi	r6, {r0, r1, r2, r8, r9, sl, pc}
 1cc:	8a048905 	bhi	1225e8 <_stack+0xa25e8>
 1d0:	8e028b03 	vmlahi.f64	d8, d2, d3
 1d4:	b40e4601 	strlt	r4, [lr], #-1537	; 0x601
 1d8:	c00e4208 	andgt	r4, lr, r8, lsl #4
 1dc:	01a00308 	lsleq	r0, r8, #6
 1e0:	42240e0a 	eormi	r0, r4, #10, 28	; 0xa0
 1e4:	c9cacbce 	stmibgt	sl, {r1, r2, r3, r6, r7, r8, r9, fp, lr, pc}^
 1e8:	c5c6c7c8 	strbgt	ip, [r6, #1992]	; 0x7c8
 1ec:	42000ec4 	andmi	r0, r0, #196, 28	; 0xc40
 1f0:	0e0a560b 	cfmadd32eq	mvax0, mvfx5, mvfx10, mvfx11
 1f4:	c5c44224 	strbgt	r4, [r4, #548]	; 0x224
 1f8:	c9c8c7c6 	stmibgt	r8, {r1, r2, r6, r7, r8, r9, sl, lr, pc}^
 1fc:	0ececbca 	cdpeq	11, 12, cr12, cr14, cr10, {6}
 200:	000b4200 	andeq	r4, fp, r0, lsl #4
 204:	00000070 	andeq	r0, r0, r0, ror r0
 208:	00000174 	andeq	r0, r0, r4, ror r1
 20c:	0000ec60 	andeq	lr, r0, r0, ror #24
 210:	00000380 	andeq	r0, r0, r0, lsl #7
 214:	84240e42 	strthi	r0, [r4], #-3650	; 0xe42
 218:	86088509 	strhi	r8, [r8], -r9, lsl #10
 21c:	88068707 	stmdahi	r6, {r0, r1, r2, r8, r9, sl, pc}
 220:	8a048905 	bhi	12263c <_stack+0xa263c>
 224:	8e028b03 	vmlahi.f64	d8, d2, d3
 228:	400e4601 	andmi	r4, lr, r1, lsl #12
 22c:	240e0a70 	strcs	r0, [lr], #-2672	; 0xa70
 230:	cacbce42 	bgt	ff2f3b40 <_stack+0xff273b40>
 234:	c6c7c8c9 	strbgt	ip, [r7], r9, asr #17
 238:	000ec4c5 	andeq	ip, lr, r5, asr #9
 23c:	0a6c0b42 	beq	1b02f4c <_stack+0x1a82f4c>
 240:	c442240e 	strbgt	r2, [r2], #-1038	; 0x40e
 244:	c8c7c6c5 	stmiagt	r7, {r0, r2, r6, r7, r9, sl, lr, pc}^
 248:	cecbcac9 	cdpgt	10, 12, cr12, cr11, cr9, {6}
 24c:	0b42000e 	bleq	108028c <_stack+0x100028c>
 250:	240e0a44 	strcs	r0, [lr], #-2628	; 0xa44
 254:	c6c5c442 	strbgt	ip, [r5], r2, asr #8
 258:	cac9c8c7 	bgt	ff27257c <_stack+0xff1f257c>
 25c:	000ececb 	andeq	ip, lr, fp, asr #29
 260:	96020b42 	strls	r0, [r2], -r2, asr #22
 264:	42240e0a 	eormi	r0, r4, #10, 28	; 0xa0
 268:	c7c6c5c4 	strbgt	ip, [r6, r4, asr #11]
 26c:	cbcac9c8 	blgt	ff2b2994 <_stack+0xff232994>
 270:	42000ece 	andmi	r0, r0, #3296	; 0xce0
 274:	0000000b 	andeq	r0, r0, fp
 278:	0000000c 	andeq	r0, r0, ip
 27c:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
 280:	7c020001 	stcvc	0, cr0, [r2], {1}
 284:	000d0c0e 	andeq	r0, sp, lr, lsl #24
 288:	00000028 	andeq	r0, r0, r8, lsr #32
 28c:	00000278 	andeq	r0, r0, r8, ror r2
 290:	0000efe0 	andeq	lr, r0, r0, ror #31
 294:	00000118 	andeq	r0, r0, r8, lsl r1
 298:	840c0e44 	strhi	r0, [ip], #-3652	; 0xe44
 29c:	8e028503 	cfsh32hi	mvfx8, mvfx2, #3
 2a0:	0a4e0201 	beq	1380aac <_stack+0x1300aac>
 2a4:	0ec4c5ce 	cdpeq	5, 12, cr12, cr4, cr14, {6}
 2a8:	440b4200 	strmi	r4, [fp], #-512	; 0x200
 2ac:	cec5c40a 	cdpgt	4, 12, cr12, cr5, cr10, {0}
 2b0:	0b42000e 	bleq	10802f0 <_stack+0x10002f0>
 2b4:	0000000c 	andeq	r0, r0, ip
 2b8:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
 2bc:	7c020001 	stcvc	0, cr0, [r2], {1}
 2c0:	000d0c0e 	andeq	r0, sp, lr, lsl #24
 2c4:	00000024 	andeq	r0, r0, r4, lsr #32
 2c8:	000002b4 			; <UNDEFINED> instruction: 0x000002b4
 2cc:	0000f0f8 	strdeq	pc, [r0], -r8
 2d0:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
 2d4:	84080e44 	strhi	r0, [r8], #-3652	; 0xe44
 2d8:	02018e02 	andeq	r8, r1, #2, 28
 2dc:	c4ce0a48 	strbgt	r0, [lr], #2632	; 0xa48
 2e0:	0b42000e 	bleq	1080320 <_stack+0x1000320>
 2e4:	cec40a44 	cdpgt	10, 12, cr0, cr4, cr4, {2}
 2e8:	0b42000e 	bleq	1080328 <_stack+0x1000328>

Disassembly of section .ARM.attributes:

00000000 <_stack-0x80000>:
   0:	00002d41 	andeq	r2, r0, r1, asr #26
   4:	61656100 	cmnvs	r5, r0, lsl #2
   8:	01006962 	tsteq	r0, r2, ror #18
   c:	00000023 	andeq	r0, r0, r3, lsr #32
  10:	4d524105 	ldfmie	f4, [r2, #-20]	; 0xffffffec
  14:	4d445437 	cfstrdmi	mvd5, [r4, #-220]	; 0xffffff24
  18:	02060049 	andeq	r0, r6, #73	; 0x49
  1c:	01090108 	tsteq	r9, r8, lsl #2
  20:	01140412 	tsteq	r4, r2, lsl r4
  24:	03170115 	tsteq	r7, #1073741829	; 0x40000005
  28:	011a0118 	tsteq	sl, r8, lsl r1
  2c:	Address 0x000000000000002c is out of bounds.

