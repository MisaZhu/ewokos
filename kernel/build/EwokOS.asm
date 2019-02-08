
build/EwokOS.elf:     file format elf32-littlearm


Disassembly of section .text:

80010000 <__entry>:
80010000:	e59fd10c 	ldr	sp, [pc, #268]	; 80010114 <__halt+0x4>
80010004:	e28dda01 	add	sp, sp, #4096	; 0x1000
80010008:	eb000004 	bl	80010020 <enablePaging>
8001000c:	eb00000d 	bl	80010048 <copyInterruptTable>
80010010:	eb000057 	bl	80010174 <__useHighInterrupts>
80010014:	eb00004d 	bl	80010150 <__enableInterrupts>
80010018:	eb00004a 	bl	80010148 <__jump2HighMem>
8001001c:	eb0002f9 	bl	80010c08 <kernelEntry>

80010020 <enablePaging>:
80010020:	e1a0200e 	mov	r2, lr
80010024:	e3a00001 	mov	r0, #1
80010028:	eb000042 	bl	80010138 <__setDomainAccessControl>
8001002c:	e59f00e4 	ldr	r0, [pc, #228]	; 80010118 <__halt+0x8>
80010030:	e2400102 	sub	r0, r0, #-2147483648	; 0x80000000
80010034:	eb000041 	bl	80010140 <__setTranslationTableBase>
80010038:	eb00003a 	bl	80010128 <__readControlRegister>
8001003c:	e3800001 	orr	r0, r0, #1
80010040:	eb00003a 	bl	80010130 <__setControlRegister>
80010044:	e1a0f002 	mov	pc, r2

80010048 <copyInterruptTable>:
80010048:	e3a00000 	mov	r0, #0
8001004c:	e59f10c8 	ldr	r1, [pc, #200]	; 8001011c <__halt+0xc>
80010050:	e59f30c8 	ldr	r3, [pc, #200]	; 80010120 <__halt+0x10>

80010054 <copyLoopStart>:
80010054:	e5912000 	ldr	r2, [r1]
80010058:	e5802000 	str	r2, [r0]
8001005c:	e2800004 	add	r0, r0, #4
80010060:	e2811004 	add	r1, r1, #4
80010064:	e1510003 	cmp	r1, r3
80010068:	1afffff9 	bne	80010054 <copyLoopStart>
8001006c:	e1a0f00e 	mov	pc, lr

80010070 <interruptTableStart>:
80010070:	e320f000 	nop	{0}
80010074:	e59ff018 	ldr	pc, [pc, #24]	; 80010094 <abortEntryAddress>
80010078:	e59ff00c 	ldr	pc, [pc, #12]	; 8001008c <syscallEntryAddress>
8001007c:	e59ff010 	ldr	pc, [pc, #16]	; 80010094 <abortEntryAddress>
80010080:	e59ff00c 	ldr	pc, [pc, #12]	; 80010094 <abortEntryAddress>
80010084:	e320f000 	nop	{0}
80010088:	e59ff000 	ldr	pc, [pc]	; 80010090 <irqEntryAddress>

8001008c <syscallEntryAddress>:
8001008c:	80010098 	mulhi	r1, r8, r0

80010090 <irqEntryAddress>:
80010090:	800100c8 	andhi	r0, r1, r8, asr #1

80010094 <abortEntryAddress>:
80010094:	800100fc 	strdhi	r0, [r1], -ip

80010098 <interruptTableEnd>:
80010098:	e59fd074 	ldr	sp, [pc, #116]	; 80010114 <__halt+0x4>
8001009c:	e28dda01 	add	sp, sp, #4096	; 0x1000
800100a0:	e92d4001 	push	{r0, lr}
800100a4:	e1a0000e 	mov	r0, lr
800100a8:	eb00003c 	bl	800101a0 <__saveContext1>
800100ac:	e8bd4001 	pop	{r0, lr}
800100b0:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
800100b4:	eb000049 	bl	800101e0 <__saveContext2>
800100b8:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
800100bc:	e92d5ffe 	push	{r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800100c0:	eb0007cf 	bl	80012004 <handleSyscall>
800100c4:	e8fd9ffe 	ldm	sp!, {r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, pc}^

800100c8 <irqEntry>:
800100c8:	e24ee004 	sub	lr, lr, #4
800100cc:	e59fd050 	ldr	sp, [pc, #80]	; 80010124 <__halt+0x14>
800100d0:	e28dda01 	add	sp, sp, #4096	; 0x1000
800100d4:	e92d4001 	push	{r0, lr}
800100d8:	e1a0000e 	mov	r0, lr
800100dc:	eb00002f 	bl	800101a0 <__saveContext1>
800100e0:	e8bd4001 	pop	{r0, lr}
800100e4:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
800100e8:	eb00003c 	bl	800101e0 <__saveContext2>
800100ec:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
800100f0:	e92d5fff 	push	{r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800100f4:	eb000342 	bl	80010e04 <handleIRQ>
800100f8:	e8fd9fff 	ldm	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, pc}^

800100fc <abortEntry>:
800100fc:	e10f2000 	mrs	r2, CPSR
80010100:	e3c2200f 	bic	r2, r2, #15
80010104:	e3822003 	orr	r2, r2, #3
80010108:	e129f002 	msr	CPSR_fc, r2
8001010c:	eb000f79 	bl	80013ef8 <_abortEntry>

80010110 <__halt>:
80010110:	eafffffe 	b	80010110 <__halt>
80010114:	80025000 	andhi	r5, r2, r0
80010118:	8001c000 	andhi	ip, r1, r0
8001011c:	80010070 	andhi	r0, r1, r0, ror r0
80010120:	80010098 	mulhi	r1, r8, r0
80010124:	80026000 	andhi	r6, r2, r0

80010128 <__readControlRegister>:
80010128:	ee110f10 	mrc	15, 0, r0, cr1, cr0, {0}
8001012c:	e1a0f00e 	mov	pc, lr

80010130 <__setControlRegister>:
80010130:	ee010f10 	mcr	15, 0, r0, cr1, cr0, {0}
80010134:	e1a0f00e 	mov	pc, lr

80010138 <__setDomainAccessControl>:
80010138:	ee030f10 	mcr	15, 0, r0, cr3, cr0, {0}
8001013c:	e1a0f00e 	mov	pc, lr

80010140 <__setTranslationTableBase>:
80010140:	ee020f10 	mcr	15, 0, r0, cr2, cr0, {0}
80010144:	e1a0f00e 	mov	pc, lr

80010148 <__jump2HighMem>:
80010148:	e28ee102 	add	lr, lr, #-2147483648	; 0x80000000
8001014c:	e1a0f00e 	mov	pc, lr

80010150 <__enableInterrupts>:
80010150:	e10f1000 	mrs	r1, CPSR
80010154:	e3c11080 	bic	r1, r1, #128	; 0x80
80010158:	e121f001 	msr	CPSR_c, r1
8001015c:	e1a0f00e 	mov	pc, lr
80010160:	e12fff1e 	bx	lr

80010164 <__disableInterrupts>:
80010164:	e10f1000 	mrs	r1, CPSR
80010168:	e3c11000 	bic	r1, r1, #0
8001016c:	e121f001 	msr	CPSR_c, r1
80010170:	e1a0f00e 	mov	pc, lr

80010174 <__useHighInterrupts>:
80010174:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
80010178:	ebffffea 	bl	80010128 <__readControlRegister>
8001017c:	e3800a02 	orr	r0, r0, #8192	; 0x2000
80010180:	ebffffea 	bl	80010130 <__setControlRegister>
80010184:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
80010188:	e1a0f00e 	mov	pc, lr

8001018c <__switchToContext>:
8001018c:	e8b05000 	ldm	r0!, {ip, lr}
80010190:	e16ff00c 	msr	SPSR_fsxc, ip
80010194:	e8d07fff 	ldm	r0, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
80010198:	e320f000 	nop	{0}
8001019c:	e1b0f00e 	movs	pc, lr

800101a0 <__saveContext1>:
800101a0:	e92d5fff 	push	{r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800101a4:	e1a0e000 	mov	lr, r0
800101a8:	e92d5ffe 	push	{r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800101ac:	eb000d40 	bl	800136b4 <getCurrentContext>
800101b0:	e8bd5ffe 	pop	{r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800101b4:	03500000 	cmpeq	r0, #0
800101b8:	0a000011 	beq	80010204 <saveContextReturn>
800101bc:	e52d1004 	push	{r1}		; (str r1, [sp, #-4]!)
800101c0:	e14f1000 	mrs	r1, SPSR
800101c4:	e5801000 	str	r1, [r0]
800101c8:	e49d1004 	pop	{r1}		; (ldr r1, [sp], #4)
800101cc:	e2800004 	add	r0, r0, #4
800101d0:	e580e000 	str	lr, [r0]
800101d4:	e2800008 	add	r0, r0, #8
800101d8:	e8c07ffe 	stmia	r0, {r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
800101dc:	ea000008 	b	80010204 <saveContextReturn>

800101e0 <__saveContext2>:
800101e0:	e92d5fff 	push	{r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800101e4:	e1a01000 	mov	r1, r0
800101e8:	e92d5ffe 	push	{r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800101ec:	eb000d30 	bl	800136b4 <getCurrentContext>
800101f0:	e8bd5ffe 	pop	{r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
800101f4:	03500000 	cmpeq	r0, #0
800101f8:	0a000001 	beq	80010204 <saveContextReturn>
800101fc:	e2800008 	add	r0, r0, #8
80010200:	e5801000 	str	r1, [r0]

80010204 <saveContextReturn>:
80010204:	e8bd5fff 	pop	{r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
80010208:	e1a0f00e 	mov	pc, lr

8001020c <routing_core0cntv_to_core0irq>:
8001020c:	e1a0c00d 	mov	ip, sp
80010210:	e92dd800 	push	{fp, ip, lr, pc}
80010214:	e24cb004 	sub	fp, ip, #4
80010218:	e24dd008 	sub	sp, sp, #8
8001021c:	eb000140 	bl	80010724 <getMMIOBasePhy>
80010220:	e1a03000 	mov	r3, r0
80010224:	e2633040 	rsb	r3, r3, #64	; 0x40
80010228:	e50b3010 	str	r3, [fp, #-16]
8001022c:	e51b3010 	ldr	r3, [fp, #-16]
80010230:	e3a02008 	mov	r2, #8
80010234:	e5832000 	str	r2, [r3]
80010238:	e24bd00c 	sub	sp, fp, #12
8001023c:	e89da800 	ldm	sp, {fp, sp, pc}

80010240 <read_core0timer_pending>:
80010240:	e1a0c00d 	mov	ip, sp
80010244:	e92dd800 	push	{fp, ip, lr, pc}
80010248:	e24cb004 	sub	fp, ip, #4
8001024c:	e24dd008 	sub	sp, sp, #8
80010250:	eb000133 	bl	80010724 <getMMIOBasePhy>
80010254:	e1a03000 	mov	r3, r0
80010258:	e2633060 	rsb	r3, r3, #96	; 0x60
8001025c:	e50b3010 	str	r3, [fp, #-16]
80010260:	e51b3010 	ldr	r3, [fp, #-16]
80010264:	e5933000 	ldr	r3, [r3]
80010268:	e50b3014 	str	r3, [fp, #-20]
8001026c:	e51b3014 	ldr	r3, [fp, #-20]
80010270:	e1a00003 	mov	r0, r3
80010274:	e24bd00c 	sub	sp, fp, #12
80010278:	e89da800 	ldm	sp, {fp, sp, pc}

8001027c <enable_cntv>:
8001027c:	e1a0c00d 	mov	ip, sp
80010280:	e92dd800 	push	{fp, ip, lr, pc}
80010284:	e24cb004 	sub	fp, ip, #4
80010288:	e24dd008 	sub	sp, sp, #8
8001028c:	e3a03001 	mov	r3, #1
80010290:	e50b3010 	str	r3, [fp, #-16]
80010294:	e51b3010 	ldr	r3, [fp, #-16]
80010298:	ee0e3f33 	mcr	15, 0, r3, cr14, cr3, {1}
8001029c:	e24bd00c 	sub	sp, fp, #12
800102a0:	e89da800 	ldm	sp, {fp, sp, pc}

800102a4 <disable_cntv>:
800102a4:	e1a0c00d 	mov	ip, sp
800102a8:	e92dd800 	push	{fp, ip, lr, pc}
800102ac:	e24cb004 	sub	fp, ip, #4
800102b0:	e24dd008 	sub	sp, sp, #8
800102b4:	e3a03000 	mov	r3, #0
800102b8:	e50b3010 	str	r3, [fp, #-16]
800102bc:	e51b3010 	ldr	r3, [fp, #-16]
800102c0:	ee0e3f33 	mcr	15, 0, r3, cr14, cr3, {1}
800102c4:	e24bd00c 	sub	sp, fp, #12
800102c8:	e89da800 	ldm	sp, {fp, sp, pc}

800102cc <write_cntv_tval>:
800102cc:	e1a0c00d 	mov	ip, sp
800102d0:	e92dd800 	push	{fp, ip, lr, pc}
800102d4:	e24cb004 	sub	fp, ip, #4
800102d8:	e24dd008 	sub	sp, sp, #8
800102dc:	e50b0010 	str	r0, [fp, #-16]
800102e0:	e51b3010 	ldr	r3, [fp, #-16]
800102e4:	ee0e3f13 	mcr	15, 0, r3, cr14, cr3, {0}
800102e8:	e1a00000 	nop			; (mov r0, r0)
800102ec:	e24bd00c 	sub	sp, fp, #12
800102f0:	e89da800 	ldm	sp, {fp, sp, pc}

800102f4 <irqInit>:
800102f4:	e1a0c00d 	mov	ip, sp
800102f8:	e92dd800 	push	{fp, ip, lr, pc}
800102fc:	e24cb004 	sub	fp, ip, #4
80010300:	e59f3018 	ldr	r3, [pc, #24]	; 80010320 <irqInit+0x2c>
80010304:	e08f3003 	add	r3, pc, r3
80010308:	e5933000 	ldr	r3, [r3]
8001030c:	e1a00003 	mov	r0, r3
80010310:	ebffffed 	bl	800102cc <write_cntv_tval>
80010314:	ebffffbc 	bl	8001020c <routing_core0cntv_to_core0irq>
80010318:	ebffffd7 	bl	8001027c <enable_cntv>
8001031c:	e89da800 	ldm	sp, {fp, sp, pc}
80010320:	00007cf4 	strdeq	r7, [r0], -r4

80010324 <enableIRQ>:
80010324:	e1a0c00d 	mov	ip, sp
80010328:	e92dd800 	push	{fp, ip, lr, pc}
8001032c:	e24cb004 	sub	fp, ip, #4
80010330:	e24dd010 	sub	sp, sp, #16
80010334:	e50b0018 	str	r0, [fp, #-24]
80010338:	e3a03cb2 	mov	r3, #45568	; 0xb200
8001033c:	e34c3000 	movt	r3, #49152	; 0xc000
80010340:	e50b3010 	str	r3, [fp, #-16]
80010344:	e51b3010 	ldr	r3, [fp, #-16]
80010348:	e2833018 	add	r3, r3, #24
8001034c:	e51b2018 	ldr	r2, [fp, #-24]
80010350:	e3a01001 	mov	r1, #1
80010354:	e1a02211 	lsl	r2, r1, r2
80010358:	e5832000 	str	r2, [r3]
8001035c:	e24bd00c 	sub	sp, fp, #12
80010360:	e89da800 	ldm	sp, {fp, sp, pc}

80010364 <disableIRQ>:
80010364:	e1a0c00d 	mov	ip, sp
80010368:	e92dd800 	push	{fp, ip, lr, pc}
8001036c:	e24cb004 	sub	fp, ip, #4
80010370:	e24dd010 	sub	sp, sp, #16
80010374:	e50b0018 	str	r0, [fp, #-24]
80010378:	e3a03cb2 	mov	r3, #45568	; 0xb200
8001037c:	e34c3000 	movt	r3, #49152	; 0xc000
80010380:	e50b3010 	str	r3, [fp, #-16]
80010384:	e51b3010 	ldr	r3, [fp, #-16]
80010388:	e2833024 	add	r3, r3, #36	; 0x24
8001038c:	e51b2018 	ldr	r2, [fp, #-24]
80010390:	e3a01001 	mov	r1, #1
80010394:	e1a02211 	lsl	r2, r1, r2
80010398:	e5832000 	str	r2, [r3]
8001039c:	e24bd00c 	sub	sp, fp, #12
800103a0:	e89da800 	ldm	sp, {fp, sp, pc}

800103a4 <getPendingIRQs>:
800103a4:	e1a0c00d 	mov	ip, sp
800103a8:	e92dd800 	push	{fp, ip, lr, pc}
800103ac:	e24cb004 	sub	fp, ip, #4
800103b0:	e24dd018 	sub	sp, sp, #24
800103b4:	e50b0020 	str	r0, [fp, #-32]
800103b8:	e3a03cb2 	mov	r3, #45568	; 0xb200
800103bc:	e34c3000 	movt	r3, #49152	; 0xc000
800103c0:	e50b3014 	str	r3, [fp, #-20]
800103c4:	e51b3014 	ldr	r3, [fp, #-20]
800103c8:	e5933000 	ldr	r3, [r3]
800103cc:	e50b3018 	str	r3, [fp, #-24]
800103d0:	e51b0020 	ldr	r0, [fp, #-32]
800103d4:	e3a01000 	mov	r1, #0
800103d8:	e3a02080 	mov	r2, #128	; 0x80
800103dc:	eb001335 	bl	800150b8 <memset>
800103e0:	ebffff96 	bl	80010240 <read_core0timer_pending>
800103e4:	e1a03000 	mov	r3, r0
800103e8:	e2033008 	and	r3, r3, #8
800103ec:	e3530000 	cmp	r3, #0
800103f0:	0a00000c 	beq	80010428 <getPendingIRQs+0x84>
800103f4:	e59f3088 	ldr	r3, [pc, #136]	; 80010484 <getPendingIRQs+0xe0>
800103f8:	e08f3003 	add	r3, pc, r3
800103fc:	e5933000 	ldr	r3, [r3]
80010400:	e1a00003 	mov	r0, r3
80010404:	ebffffb0 	bl	800102cc <write_cntv_tval>
80010408:	eb0000d7 	bl	8001076c <getTimerIrq>
8001040c:	e1a03000 	mov	r3, r0
80010410:	e1a03103 	lsl	r3, r3, #2
80010414:	e51b2020 	ldr	r2, [fp, #-32]
80010418:	e0823003 	add	r3, r2, r3
8001041c:	e3a02001 	mov	r2, #1
80010420:	e5832000 	str	r2, [r3]
80010424:	ea000014 	b	8001047c <getPendingIRQs+0xd8>
80010428:	e3a03000 	mov	r3, #0
8001042c:	e50b3010 	str	r3, [fp, #-16]
80010430:	ea00000e 	b	80010470 <getPendingIRQs+0xcc>
80010434:	e51b2018 	ldr	r2, [fp, #-24]
80010438:	e51b3010 	ldr	r3, [fp, #-16]
8001043c:	e1a03332 	lsr	r3, r2, r3
80010440:	e2033001 	and	r3, r3, #1
80010444:	e3530000 	cmp	r3, #0
80010448:	0a000005 	beq	80010464 <getPendingIRQs+0xc0>
8001044c:	e51b3010 	ldr	r3, [fp, #-16]
80010450:	e1a03103 	lsl	r3, r3, #2
80010454:	e51b2020 	ldr	r2, [fp, #-32]
80010458:	e0823003 	add	r3, r2, r3
8001045c:	e3a02001 	mov	r2, #1
80010460:	e5832000 	str	r2, [r3]
80010464:	e51b3010 	ldr	r3, [fp, #-16]
80010468:	e2833001 	add	r3, r3, #1
8001046c:	e50b3010 	str	r3, [fp, #-16]
80010470:	e51b3010 	ldr	r3, [fp, #-16]
80010474:	e353001f 	cmp	r3, #31
80010478:	9affffed 	bls	80010434 <getPendingIRQs+0x90>
8001047c:	e24bd00c 	sub	sp, fp, #12
80010480:	e89da800 	ldm	sp, {fp, sp, pc}
80010484:	00007c00 	andeq	r7, r0, r0, lsl #24

80010488 <delay>:
80010488:	e1a0c00d 	mov	ip, sp
8001048c:	e92dd800 	push	{fp, ip, lr, pc}
80010490:	e24cb004 	sub	fp, ip, #4
80010494:	e24dd008 	sub	sp, sp, #8
80010498:	e50b0010 	str	r0, [fp, #-16]
8001049c:	e51b3010 	ldr	r3, [fp, #-16]

800104a0 <__delay_12>:
800104a0:	e2533001 	subs	r3, r3, #1
800104a4:	1afffffd 	bne	800104a0 <__delay_12>
800104a8:	e24bd00c 	sub	sp, fp, #12
800104ac:	e89da800 	ldm	sp, {fp, sp, pc}

800104b0 <uartDevInit>:
800104b0:	e1a0c00d 	mov	ip, sp
800104b4:	e92dd800 	push	{fp, ip, lr, pc}
800104b8:	e24cb004 	sub	fp, ip, #4
800104bc:	e3013030 	movw	r3, #4144	; 0x1030
800104c0:	e34c3020 	movt	r3, #49184	; 0xc020
800104c4:	e3a02000 	mov	r2, #0
800104c8:	e5832000 	str	r2, [r3]
800104cc:	e3a03094 	mov	r3, #148	; 0x94
800104d0:	e34c3020 	movt	r3, #49184	; 0xc020
800104d4:	e3a02000 	mov	r2, #0
800104d8:	e5832000 	str	r2, [r3]
800104dc:	e3a00096 	mov	r0, #150	; 0x96
800104e0:	ebffffe8 	bl	80010488 <delay>
800104e4:	e3a03098 	mov	r3, #152	; 0x98
800104e8:	e34c3020 	movt	r3, #49184	; 0xc020
800104ec:	e3a02903 	mov	r2, #49152	; 0xc000
800104f0:	e5832000 	str	r2, [r3]
800104f4:	e3a00096 	mov	r0, #150	; 0x96
800104f8:	ebffffe2 	bl	80010488 <delay>
800104fc:	e3a03098 	mov	r3, #152	; 0x98
80010500:	e34c3020 	movt	r3, #49184	; 0xc020
80010504:	e3a02000 	mov	r2, #0
80010508:	e5832000 	str	r2, [r3]
8001050c:	e3013044 	movw	r3, #4164	; 0x1044
80010510:	e34c3020 	movt	r3, #49184	; 0xc020
80010514:	e30027ff 	movw	r2, #2047	; 0x7ff
80010518:	e5832000 	str	r2, [r3]
8001051c:	e3013024 	movw	r3, #4132	; 0x1024
80010520:	e34c3020 	movt	r3, #49184	; 0xc020
80010524:	e3a02001 	mov	r2, #1
80010528:	e5832000 	str	r2, [r3]
8001052c:	e3013028 	movw	r3, #4136	; 0x1028
80010530:	e34c3020 	movt	r3, #49184	; 0xc020
80010534:	e3a02028 	mov	r2, #40	; 0x28
80010538:	e5832000 	str	r2, [r3]
8001053c:	e301302c 	movw	r3, #4140	; 0x102c
80010540:	e34c3020 	movt	r3, #49184	; 0xc020
80010544:	e3a02070 	mov	r2, #112	; 0x70
80010548:	e5832000 	str	r2, [r3]
8001054c:	e3013038 	movw	r3, #4152	; 0x1038
80010550:	e34c3020 	movt	r3, #49184	; 0xc020
80010554:	e30027f2 	movw	r2, #2034	; 0x7f2
80010558:	e5832000 	str	r2, [r3]
8001055c:	e3013030 	movw	r3, #4144	; 0x1030
80010560:	e34c3020 	movt	r3, #49184	; 0xc020
80010564:	e3002301 	movw	r2, #769	; 0x301
80010568:	e5832000 	str	r2, [r3]
8001056c:	e89da800 	ldm	sp, {fp, sp, pc}

80010570 <uartTransmit>:
80010570:	e1a0c00d 	mov	ip, sp
80010574:	e92dd800 	push	{fp, ip, lr, pc}
80010578:	e24cb004 	sub	fp, ip, #4
8001057c:	e24dd008 	sub	sp, sp, #8
80010580:	e1a03000 	mov	r3, r0
80010584:	e54b300d 	strb	r3, [fp, #-13]
80010588:	e1a00000 	nop			; (mov r0, r0)
8001058c:	e3013018 	movw	r3, #4120	; 0x1018
80010590:	e34c3020 	movt	r3, #49184	; 0xc020
80010594:	e5933000 	ldr	r3, [r3]
80010598:	e2033020 	and	r3, r3, #32
8001059c:	e3530000 	cmp	r3, #0
800105a0:	1afffff9 	bne	8001058c <uartTransmit+0x1c>
800105a4:	e3a03a01 	mov	r3, #4096	; 0x1000
800105a8:	e34c3020 	movt	r3, #49184	; 0xc020
800105ac:	e55b200d 	ldrb	r2, [fp, #-13]
800105b0:	e5832000 	str	r2, [r3]
800105b4:	e24bd00c 	sub	sp, fp, #12
800105b8:	e89da800 	ldm	sp, {fp, sp, pc}

800105bc <uartReceive>:
800105bc:	e1a0c00d 	mov	ip, sp
800105c0:	e92dd800 	push	{fp, ip, lr, pc}
800105c4:	e24cb004 	sub	fp, ip, #4
800105c8:	e3a03a01 	mov	r3, #4096	; 0x1000
800105cc:	e34c3020 	movt	r3, #49184	; 0xc020
800105d0:	e5933000 	ldr	r3, [r3]
800105d4:	e1a00003 	mov	r0, r3
800105d8:	e89da800 	ldm	sp, {fp, sp, pc}

800105dc <uartReadyToRecv>:
800105dc:	e1a0c00d 	mov	ip, sp
800105e0:	e92dd800 	push	{fp, ip, lr, pc}
800105e4:	e24cb004 	sub	fp, ip, #4
800105e8:	e3013018 	movw	r3, #4120	; 0x1018
800105ec:	e34c3020 	movt	r3, #49184	; 0xc020
800105f0:	e5933000 	ldr	r3, [r3]
800105f4:	e2033010 	and	r3, r3, #16
800105f8:	e3530000 	cmp	r3, #0
800105fc:	03a03001 	moveq	r3, #1
80010600:	13a03000 	movne	r3, #0
80010604:	e6ef3073 	uxtb	r3, r3
80010608:	e1a00003 	mov	r0, r3
8001060c:	e89da800 	ldm	sp, {fp, sp, pc}

80010610 <timerSetInterval>:
80010610:	e1a0c00d 	mov	ip, sp
80010614:	e92dd800 	push	{fp, ip, lr, pc}
80010618:	e24cb004 	sub	fp, ip, #4
8001061c:	e24dd010 	sub	sp, sp, #16
80010620:	e50b0018 	str	r0, [fp, #-24]
80010624:	e3a03b2d 	mov	r3, #46080	; 0xb400
80010628:	e34c3000 	movt	r3, #49152	; 0xc000
8001062c:	e50b3010 	str	r3, [fp, #-16]
80010630:	e51b3010 	ldr	r3, [fp, #-16]
80010634:	e2833008 	add	r3, r3, #8
80010638:	e3a02000 	mov	r2, #0
8001063c:	e5832000 	str	r2, [r3]
80010640:	e51b3010 	ldr	r3, [fp, #-16]
80010644:	e2833018 	add	r3, r3, #24
80010648:	e3a02000 	mov	r2, #0
8001064c:	e5832000 	str	r2, [r3]
80010650:	e51b3010 	ldr	r3, [fp, #-16]
80010654:	e51b2018 	ldr	r2, [fp, #-24]
80010658:	e5832000 	str	r2, [r3]
8001065c:	e51b3010 	ldr	r3, [fp, #-16]
80010660:	e2832008 	add	r2, r3, #8
80010664:	e3a030a2 	mov	r3, #162	; 0xa2
80010668:	e340303e 	movt	r3, #62	; 0x3e
8001066c:	e5823000 	str	r3, [r2]
80010670:	e24bd00c 	sub	sp, fp, #12
80010674:	e89da800 	ldm	sp, {fp, sp, pc}

80010678 <timerClearInterrupt>:
80010678:	e1a0c00d 	mov	ip, sp
8001067c:	e92dd800 	push	{fp, ip, lr, pc}
80010680:	e24cb004 	sub	fp, ip, #4
80010684:	e24dd008 	sub	sp, sp, #8
80010688:	e3a03b2d 	mov	r3, #46080	; 0xb400
8001068c:	e34c3000 	movt	r3, #49152	; 0xc000
80010690:	e50b3010 	str	r3, [fp, #-16]
80010694:	e51b3010 	ldr	r3, [fp, #-16]
80010698:	e283300c 	add	r3, r3, #12
8001069c:	e3a02000 	mov	r2, #0
800106a0:	e5832000 	str	r2, [r3]
800106a4:	e24bd00c 	sub	sp, fp, #12
800106a8:	e89da800 	ldm	sp, {fp, sp, pc}

800106ac <timerInit>:
800106ac:	e1a0c00d 	mov	ip, sp
800106b0:	e92dd800 	push	{fp, ip, lr, pc}
800106b4:	e24cb004 	sub	fp, ip, #4
800106b8:	e24dd008 	sub	sp, sp, #8
800106bc:	e3a03b2d 	mov	r3, #46080	; 0xb400
800106c0:	e34c3000 	movt	r3, #49152	; 0xc000
800106c4:	e50b3010 	str	r3, [fp, #-16]
800106c8:	e51b3010 	ldr	r3, [fp, #-16]
800106cc:	e3a02a01 	mov	r2, #4096	; 0x1000
800106d0:	e5832000 	str	r2, [r3]
800106d4:	e51b3010 	ldr	r3, [fp, #-16]
800106d8:	e2833008 	add	r3, r3, #8
800106dc:	e3a0283e 	mov	r2, #4063232	; 0x3e0000
800106e0:	e5832000 	str	r2, [r3]
800106e4:	e51b3010 	ldr	r3, [fp, #-16]
800106e8:	e283300c 	add	r3, r3, #12
800106ec:	e3a02000 	mov	r2, #0
800106f0:	e5832000 	str	r2, [r3]
800106f4:	e3a00000 	mov	r0, #0
800106f8:	ebffff09 	bl	80010324 <enableIRQ>
800106fc:	e3a00001 	mov	r0, #1
80010700:	ebffff07 	bl	80010324 <enableIRQ>
80010704:	e24bd00c 	sub	sp, fp, #12
80010708:	e89da800 	ldm	sp, {fp, sp, pc}

8001070c <getPhyRamSize>:
8001070c:	e1a0c00d 	mov	ip, sp
80010710:	e92dd800 	push	{fp, ip, lr, pc}
80010714:	e24cb004 	sub	fp, ip, #4
80010718:	e3a03201 	mov	r3, #268435456	; 0x10000000
8001071c:	e1a00003 	mov	r0, r3
80010720:	e89da800 	ldm	sp, {fp, sp, pc}

80010724 <getMMIOBasePhy>:
80010724:	e1a0c00d 	mov	ip, sp
80010728:	e92dd800 	push	{fp, ip, lr, pc}
8001072c:	e24cb004 	sub	fp, ip, #4
80010730:	e3a0343f 	mov	r3, #1056964608	; 0x3f000000
80010734:	e1a00003 	mov	r0, r3
80010738:	e89da800 	ldm	sp, {fp, sp, pc}

8001073c <getMMIOMemSize>:
8001073c:	e1a0c00d 	mov	ip, sp
80010740:	e92dd800 	push	{fp, ip, lr, pc}
80010744:	e24cb004 	sub	fp, ip, #4
80010748:	e3a03402 	mov	r3, #33554432	; 0x2000000
8001074c:	e1a00003 	mov	r0, r3
80010750:	e89da800 	ldm	sp, {fp, sp, pc}

80010754 <getUartIrq>:
80010754:	e1a0c00d 	mov	ip, sp
80010758:	e92dd800 	push	{fp, ip, lr, pc}
8001075c:	e24cb004 	sub	fp, ip, #4
80010760:	e3a0301d 	mov	r3, #29
80010764:	e1a00003 	mov	r0, r3
80010768:	e89da800 	ldm	sp, {fp, sp, pc}

8001076c <getTimerIrq>:
8001076c:	e1a0c00d 	mov	ip, sp
80010770:	e92dd800 	push	{fp, ip, lr, pc}
80010774:	e24cb004 	sub	fp, ip, #4
80010778:	e3a03000 	mov	r3, #0
8001077c:	e1a00003 	mov	r0, r3
80010780:	e89da800 	ldm	sp, {fp, sp, pc}

80010784 <uartInit>:
80010784:	e1a0c00d 	mov	ip, sp
80010788:	e92dd800 	push	{fp, ip, lr, pc}
8001078c:	e24cb004 	sub	fp, ip, #4
80010790:	e59f303c 	ldr	r3, [pc, #60]	; 800107d4 <uartInit+0x50>
80010794:	e08f3003 	add	r3, pc, r3
80010798:	e3a02000 	mov	r2, #0
8001079c:	e5832000 	str	r2, [r3]
800107a0:	e59f3030 	ldr	r3, [pc, #48]	; 800107d8 <uartInit+0x54>
800107a4:	e08f3003 	add	r3, pc, r3
800107a8:	e3a02000 	mov	r2, #0
800107ac:	e5832000 	str	r2, [r3]
800107b0:	ebffff3e 	bl	800104b0 <uartDevInit>
800107b4:	ebffffe6 	bl	80010754 <getUartIrq>
800107b8:	e1a03000 	mov	r3, r0
800107bc:	e1a00003 	mov	r0, r3
800107c0:	e59f3014 	ldr	r3, [pc, #20]	; 800107dc <uartInit+0x58>
800107c4:	e08f3003 	add	r3, pc, r3
800107c8:	e1a01003 	mov	r1, r3
800107cc:	eb00017c 	bl	80010dc4 <registerInterruptHandler>
800107d0:	e89da800 	ldm	sp, {fp, sp, pc}
800107d4:	00013874 	andeq	r3, r1, r4, ror r8
800107d8:	00013868 	andeq	r3, r1, r8, ror #16
800107dc:	000001a4 	andeq	r0, r0, r4, lsr #3

800107e0 <uartPuts>:
800107e0:	e1a0c00d 	mov	ip, sp
800107e4:	e92dd800 	push	{fp, ip, lr, pc}
800107e8:	e24cb004 	sub	fp, ip, #4
800107ec:	e24dd010 	sub	sp, sp, #16
800107f0:	e50b0018 	str	r0, [fp, #-24]
800107f4:	e3a03000 	mov	r3, #0
800107f8:	e50b3010 	str	r3, [fp, #-16]
800107fc:	e51b3010 	ldr	r3, [fp, #-16]
80010800:	e2832001 	add	r2, r3, #1
80010804:	e50b2010 	str	r2, [fp, #-16]
80010808:	e1a02003 	mov	r2, r3
8001080c:	e51b3018 	ldr	r3, [fp, #-24]
80010810:	e0833002 	add	r3, r3, r2
80010814:	e5d33000 	ldrb	r3, [r3]
80010818:	e50b3014 	str	r3, [fp, #-20]
8001081c:	e51b3014 	ldr	r3, [fp, #-20]
80010820:	e3530000 	cmp	r3, #0
80010824:	1a000000 	bne	8001082c <uartPuts+0x4c>
80010828:	ea000002 	b	80010838 <uartPuts+0x58>
8001082c:	e51b0014 	ldr	r0, [fp, #-20]
80010830:	eb000002 	bl	80010840 <uartPutch>
80010834:	eafffff0 	b	800107fc <uartPuts+0x1c>
80010838:	e24bd00c 	sub	sp, fp, #12
8001083c:	e89da800 	ldm	sp, {fp, sp, pc}

80010840 <uartPutch>:
80010840:	e1a0c00d 	mov	ip, sp
80010844:	e92dd800 	push	{fp, ip, lr, pc}
80010848:	e24cb004 	sub	fp, ip, #4
8001084c:	e24dd008 	sub	sp, sp, #8
80010850:	e50b0010 	str	r0, [fp, #-16]
80010854:	e51b3010 	ldr	r3, [fp, #-16]
80010858:	e353000d 	cmp	r3, #13
8001085c:	1a000001 	bne	80010868 <uartPutch+0x28>
80010860:	e3a0300a 	mov	r3, #10
80010864:	e50b3010 	str	r3, [fp, #-16]
80010868:	e51b3010 	ldr	r3, [fp, #-16]
8001086c:	e6ef3073 	uxtb	r3, r3
80010870:	e1a00003 	mov	r0, r3
80010874:	ebffff3d 	bl	80010570 <uartTransmit>
80010878:	e24bd00c 	sub	sp, fp, #12
8001087c:	e89da800 	ldm	sp, {fp, sp, pc}

80010880 <uartGetch>:
80010880:	e1a0c00d 	mov	ip, sp
80010884:	e92dd800 	push	{fp, ip, lr, pc}
80010888:	e24cb004 	sub	fp, ip, #4
8001088c:	e24dd008 	sub	sp, sp, #8
80010890:	e3a03000 	mov	r3, #0
80010894:	e50b3010 	str	r3, [fp, #-16]
80010898:	e59f306c 	ldr	r3, [pc, #108]	; 8001090c <uartGetch+0x8c>
8001089c:	e08f3003 	add	r3, pc, r3
800108a0:	e5932000 	ldr	r2, [r3]
800108a4:	e59f3064 	ldr	r3, [pc, #100]	; 80010910 <uartGetch+0x90>
800108a8:	e08f3003 	add	r3, pc, r3
800108ac:	e5933000 	ldr	r3, [r3]
800108b0:	e1520003 	cmp	r2, r3
800108b4:	0a000010 	beq	800108fc <uartGetch+0x7c>
800108b8:	e59f3054 	ldr	r3, [pc, #84]	; 80010914 <uartGetch+0x94>
800108bc:	e08f3003 	add	r3, pc, r3
800108c0:	e5933000 	ldr	r3, [r3]
800108c4:	e59f204c 	ldr	r2, [pc, #76]	; 80010918 <uartGetch+0x98>
800108c8:	e08f2002 	add	r2, pc, r2
800108cc:	e7d23003 	ldrb	r3, [r2, r3]
800108d0:	e50b3010 	str	r3, [fp, #-16]
800108d4:	e59f3040 	ldr	r3, [pc, #64]	; 8001091c <uartGetch+0x9c>
800108d8:	e08f3003 	add	r3, pc, r3
800108dc:	e5933000 	ldr	r3, [r3]
800108e0:	e1a00003 	mov	r0, r3
800108e4:	e3a01010 	mov	r1, #16
800108e8:	eb00000d 	bl	80010924 <circularInc>
800108ec:	e1a02000 	mov	r2, r0
800108f0:	e59f3028 	ldr	r3, [pc, #40]	; 80010920 <uartGetch+0xa0>
800108f4:	e08f3003 	add	r3, pc, r3
800108f8:	e5832000 	str	r2, [r3]
800108fc:	e51b3010 	ldr	r3, [fp, #-16]
80010900:	e1a00003 	mov	r0, r3
80010904:	e24bd00c 	sub	sp, fp, #12
80010908:	e89da800 	ldm	sp, {fp, sp, pc}
8001090c:	0001376c 	andeq	r3, r1, ip, ror #14
80010910:	00013764 	andeq	r3, r1, r4, ror #14
80010914:	00013750 	andeq	r3, r1, r0, asr r7
80010918:	00013730 	andeq	r3, r1, r0, lsr r7
8001091c:	00013734 	andeq	r3, r1, r4, lsr r7
80010920:	00013718 	andeq	r3, r1, r8, lsl r7

80010924 <circularInc>:
80010924:	e1a0c00d 	mov	ip, sp
80010928:	e92dd800 	push	{fp, ip, lr, pc}
8001092c:	e24cb004 	sub	fp, ip, #4
80010930:	e24dd008 	sub	sp, sp, #8
80010934:	e50b0010 	str	r0, [fp, #-16]
80010938:	e50b1014 	str	r1, [fp, #-20]
8001093c:	e51b3010 	ldr	r3, [fp, #-16]
80010940:	e2833001 	add	r3, r3, #1
80010944:	e50b3010 	str	r3, [fp, #-16]
80010948:	e51b2010 	ldr	r2, [fp, #-16]
8001094c:	e51b3014 	ldr	r3, [fp, #-20]
80010950:	e1520003 	cmp	r2, r3
80010954:	1a000001 	bne	80010960 <circularInc+0x3c>
80010958:	e3a03000 	mov	r3, #0
8001095c:	e50b3010 	str	r3, [fp, #-16]
80010960:	e51b3010 	ldr	r3, [fp, #-16]
80010964:	e1a00003 	mov	r0, r3
80010968:	e24bd00c 	sub	sp, fp, #12
8001096c:	e89da800 	ldm	sp, {fp, sp, pc}

80010970 <uartInterruptHandler>:
80010970:	e1a0c00d 	mov	ip, sp
80010974:	e92dd800 	push	{fp, ip, lr, pc}
80010978:	e24cb004 	sub	fp, ip, #4
8001097c:	e24dd008 	sub	sp, sp, #8
80010980:	ea00001c 	b	800109f8 <uartInterruptHandler+0x88>
80010984:	e3a03000 	mov	r3, #0
80010988:	e50b3010 	str	r3, [fp, #-16]
8001098c:	ebffff0a 	bl	800105bc <uartReceive>
80010990:	e50b0014 	str	r0, [fp, #-20]
80010994:	e59f3074 	ldr	r3, [pc, #116]	; 80010a10 <uartInterruptHandler+0xa0>
80010998:	e08f3003 	add	r3, pc, r3
8001099c:	e5933000 	ldr	r3, [r3]
800109a0:	e1a00003 	mov	r0, r3
800109a4:	e3a01010 	mov	r1, #16
800109a8:	ebffffdd 	bl	80010924 <circularInc>
800109ac:	e50b0010 	str	r0, [fp, #-16]
800109b0:	e59f305c 	ldr	r3, [pc, #92]	; 80010a14 <uartInterruptHandler+0xa4>
800109b4:	e08f3003 	add	r3, pc, r3
800109b8:	e5933000 	ldr	r3, [r3]
800109bc:	e51b2010 	ldr	r2, [fp, #-16]
800109c0:	e1520003 	cmp	r2, r3
800109c4:	0a00000b 	beq	800109f8 <uartInterruptHandler+0x88>
800109c8:	e59f3048 	ldr	r3, [pc, #72]	; 80010a18 <uartInterruptHandler+0xa8>
800109cc:	e08f3003 	add	r3, pc, r3
800109d0:	e5933000 	ldr	r3, [r3]
800109d4:	e51b2014 	ldr	r2, [fp, #-20]
800109d8:	e6ef1072 	uxtb	r1, r2
800109dc:	e59f2038 	ldr	r2, [pc, #56]	; 80010a1c <uartInterruptHandler+0xac>
800109e0:	e08f2002 	add	r2, pc, r2
800109e4:	e7c21003 	strb	r1, [r2, r3]
800109e8:	e59f3030 	ldr	r3, [pc, #48]	; 80010a20 <uartInterruptHandler+0xb0>
800109ec:	e08f3003 	add	r3, pc, r3
800109f0:	e51b2010 	ldr	r2, [fp, #-16]
800109f4:	e5832000 	str	r2, [r3]
800109f8:	ebfffef7 	bl	800105dc <uartReadyToRecv>
800109fc:	e1a03000 	mov	r3, r0
80010a00:	e3530000 	cmp	r3, #0
80010a04:	1affffde 	bne	80010984 <uartInterruptHandler+0x14>
80010a08:	e24bd00c 	sub	sp, fp, #12
80010a0c:	e89da800 	ldm	sp, {fp, sp, pc}
80010a10:	00013670 	andeq	r3, r1, r0, ror r6
80010a14:	00013658 	andeq	r3, r1, r8, asr r6
80010a18:	0001363c 	andeq	r3, r1, ip, lsr r6
80010a1c:	00013618 	andeq	r3, r1, r8, lsl r6
80010a20:	0001361c 	andeq	r3, r1, ip, lsl r6

80010a24 <initKernelVM>:
80010a24:	e1a0c00d 	mov	ip, sp
80010a28:	e92dd810 	push	{r4, fp, ip, lr, pc}
80010a2c:	e24cb004 	sub	fp, ip, #4
80010a30:	e24dd00c 	sub	sp, sp, #12
80010a34:	e59f4064 	ldr	r4, [pc, #100]	; 80010aa0 <initKernelVM+0x7c>
80010a38:	e08f4004 	add	r4, pc, r4
80010a3c:	e59f3060 	ldr	r3, [pc, #96]	; 80010aa4 <initKernelVM+0x80>
80010a40:	e7943003 	ldr	r3, [r4, r3]
80010a44:	e50b3018 	str	r3, [fp, #-24]
80010a48:	e51b3018 	ldr	r3, [fp, #-24]
80010a4c:	e2833dff 	add	r3, r3, #16320	; 0x3fc0
80010a50:	e283303f 	add	r3, r3, #63	; 0x3f
80010a54:	e3c33dff 	bic	r3, r3, #16320	; 0x3fc0
80010a58:	e3c3303f 	bic	r3, r3, #63	; 0x3f
80010a5c:	e1a02003 	mov	r2, r3
80010a60:	e59f3040 	ldr	r3, [pc, #64]	; 80010aa8 <initKernelVM+0x84>
80010a64:	e7943003 	ldr	r3, [r4, r3]
80010a68:	e5832000 	str	r2, [r3]
80010a6c:	e59f3034 	ldr	r3, [pc, #52]	; 80010aa8 <initKernelVM+0x84>
80010a70:	e7943003 	ldr	r3, [r4, r3]
80010a74:	e5933000 	ldr	r3, [r3]
80010a78:	e1a00003 	mov	r0, r3
80010a7c:	eb00000a 	bl	80010aac <setKernelVM>
80010a80:	e59f3020 	ldr	r3, [pc, #32]	; 80010aa8 <initKernelVM+0x84>
80010a84:	e7943003 	ldr	r3, [r4, r3]
80010a88:	e5933000 	ldr	r3, [r3]
80010a8c:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80010a90:	e1a00003 	mov	r0, r3
80010a94:	ebfffda9 	bl	80010140 <__setTranslationTableBase>
80010a98:	e24bd010 	sub	sp, fp, #16
80010a9c:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80010aa0:	0000f5c4 	andeq	pc, r0, r4, asr #11
80010aa4:	00000018 	andeq	r0, r0, r8, lsl r0
80010aa8:	0000000c 	andeq	r0, r0, ip

80010aac <setKernelVM>:
80010aac:	e1a0c00d 	mov	ip, sp
80010ab0:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
80010ab4:	e24cb004 	sub	fp, ip, #4
80010ab8:	e24dd014 	sub	sp, sp, #20
80010abc:	e50b0020 	str	r0, [fp, #-32]
80010ac0:	e59f4138 	ldr	r4, [pc, #312]	; 80010c00 <setKernelVM+0x154>
80010ac4:	e08f4004 	add	r4, pc, r4
80010ac8:	e51b0020 	ldr	r0, [fp, #-32]
80010acc:	e3a01000 	mov	r1, #0
80010ad0:	e3a02901 	mov	r2, #16384	; 0x4000
80010ad4:	eb001177 	bl	800150b8 <memset>
80010ad8:	e3a03055 	mov	r3, #85	; 0x55
80010adc:	e58d3000 	str	r3, [sp]
80010ae0:	e51b0020 	ldr	r0, [fp, #-32]
80010ae4:	e3a01000 	mov	r1, #0
80010ae8:	e34f1fff 	movt	r1, #65535	; 0xffff
80010aec:	e3a02000 	mov	r2, #0
80010af0:	e3a03a01 	mov	r3, #4096	; 0x1000
80010af4:	eb000557 	bl	80012058 <mapPages>
80010af8:	e59f3104 	ldr	r3, [pc, #260]	; 80010c04 <setKernelVM+0x158>
80010afc:	e7943003 	ldr	r3, [r4, r3]
80010b00:	e283c102 	add	ip, r3, #-2147483648	; 0x80000000
80010b04:	e3a03055 	mov	r3, #85	; 0x55
80010b08:	e58d3000 	str	r3, [sp]
80010b0c:	e51b0020 	ldr	r0, [fp, #-32]
80010b10:	e3a01a01 	mov	r1, #4096	; 0x1000
80010b14:	e3481000 	movt	r1, #32768	; 0x8000
80010b18:	e3a02a01 	mov	r2, #4096	; 0x1000
80010b1c:	e1a0300c 	mov	r3, ip
80010b20:	eb00054c 	bl	80012058 <mapPages>
80010b24:	e3a03055 	mov	r3, #85	; 0x55
80010b28:	e58d3000 	str	r3, [sp]
80010b2c:	e51b0020 	ldr	r0, [fp, #-32]
80010b30:	e3a01302 	mov	r1, #134217728	; 0x8000000
80010b34:	e3a02302 	mov	r2, #134217728	; 0x8000000
80010b38:	e3a03681 	mov	r3, #135266304	; 0x8100000
80010b3c:	eb000545 	bl	80012058 <mapPages>
80010b40:	ebfffef7 	bl	80010724 <getMMIOBasePhy>
80010b44:	e1a06000 	mov	r6, r0
80010b48:	ebfffef5 	bl	80010724 <getMMIOBasePhy>
80010b4c:	e1a05000 	mov	r5, r0
80010b50:	ebfffef9 	bl	8001073c <getMMIOMemSize>
80010b54:	e1a03000 	mov	r3, r0
80010b58:	e085c003 	add	ip, r5, r3
80010b5c:	e3a03055 	mov	r3, #85	; 0x55
80010b60:	e58d3000 	str	r3, [sp]
80010b64:	e51b0020 	ldr	r0, [fp, #-32]
80010b68:	e3a01103 	mov	r1, #-1073741824	; 0xc0000000
80010b6c:	e1a02006 	mov	r2, r6
80010b70:	e1a0300c 	mov	r3, ip
80010b74:	eb000537 	bl	80012058 <mapPages>
80010b78:	e59f3084 	ldr	r3, [pc, #132]	; 80010c04 <setKernelVM+0x158>
80010b7c:	e7943003 	ldr	r3, [r4, r3]
80010b80:	e283c902 	add	ip, r3, #32768	; 0x8000
80010b84:	e59f3078 	ldr	r3, [pc, #120]	; 80010c04 <setKernelVM+0x158>
80010b88:	e7943003 	ldr	r3, [r4, r3]
80010b8c:	e2832102 	add	r2, r3, #-2147483648	; 0x80000000
80010b90:	e2822902 	add	r2, r2, #32768	; 0x8000
80010b94:	e59f3068 	ldr	r3, [pc, #104]	; 80010c04 <setKernelVM+0x158>
80010b98:	e7943003 	ldr	r3, [r4, r3]
80010b9c:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80010ba0:	e2833982 	add	r3, r3, #2129920	; 0x208000
80010ba4:	e3a01055 	mov	r1, #85	; 0x55
80010ba8:	e58d1000 	str	r1, [sp]
80010bac:	e51b0020 	ldr	r0, [fp, #-32]
80010bb0:	e1a0100c 	mov	r1, ip
80010bb4:	eb000527 	bl	80012058 <mapPages>
80010bb8:	e59f3044 	ldr	r3, [pc, #68]	; 80010c04 <setKernelVM+0x158>
80010bbc:	e7943003 	ldr	r3, [r4, r3]
80010bc0:	e2835982 	add	r5, r3, #2129920	; 0x208000
80010bc4:	e59f3038 	ldr	r3, [pc, #56]	; 80010c04 <setKernelVM+0x158>
80010bc8:	e7943003 	ldr	r3, [r4, r3]
80010bcc:	e2834102 	add	r4, r3, #-2147483648	; 0x80000000
80010bd0:	e2844982 	add	r4, r4, #2129920	; 0x208000
80010bd4:	ebfffecc 	bl	8001070c <getPhyRamSize>
80010bd8:	e1a0c000 	mov	ip, r0
80010bdc:	e3a03055 	mov	r3, #85	; 0x55
80010be0:	e58d3000 	str	r3, [sp]
80010be4:	e51b0020 	ldr	r0, [fp, #-32]
80010be8:	e1a01005 	mov	r1, r5
80010bec:	e1a02004 	mov	r2, r4
80010bf0:	e1a0300c 	mov	r3, ip
80010bf4:	eb000517 	bl	80012058 <mapPages>
80010bf8:	e24bd018 	sub	sp, fp, #24
80010bfc:	e89da870 	ldm	sp, {r4, r5, r6, fp, sp, pc}
80010c00:	0000f538 	andeq	pc, r0, r8, lsr r5	; <UNPREDICTABLE>
80010c04:	00000018 	andeq	r0, r0, r8, lsl r0

80010c08 <kernelEntry>:
80010c08:	e1a0c00d 	mov	ip, sp
80010c0c:	e92dd810 	push	{r4, fp, ip, lr, pc}
80010c10:	e24cb004 	sub	fp, ip, #4
80010c14:	e24dd00c 	sub	sp, sp, #12
80010c18:	e59f4174 	ldr	r4, [pc, #372]	; 80010d94 <kernelEntry+0x18c>
80010c1c:	e08f4004 	add	r4, pc, r4
80010c20:	e59f3170 	ldr	r3, [pc, #368]	; 80010d98 <kernelEntry+0x190>
80010c24:	e7943003 	ldr	r3, [r4, r3]
80010c28:	e2833982 	add	r3, r3, #2129920	; 0x208000
80010c2c:	e1a00003 	mov	r0, r3
80010c30:	e3a01000 	mov	r1, #0
80010c34:	e3481080 	movt	r1, #32896	; 0x8080
80010c38:	eb00061b 	bl	800124ac <kallocInit>
80010c3c:	ebffff78 	bl	80010a24 <initKernelVM>
80010c40:	eb0008e5 	bl	80012fdc <kmInit>
80010c44:	e3a00601 	mov	r0, #1048576	; 0x100000
80010c48:	eb0008fc 	bl	80013040 <kmalloc>
80010c4c:	e1a02000 	mov	r2, r0
80010c50:	e59f3144 	ldr	r3, [pc, #324]	; 80010d9c <kernelEntry+0x194>
80010c54:	e7943003 	ldr	r3, [r4, r3]
80010c58:	e5832000 	str	r2, [r3]
80010c5c:	e59f3138 	ldr	r3, [pc, #312]	; 80010d9c <kernelEntry+0x194>
80010c60:	e7943003 	ldr	r3, [r4, r3]
80010c64:	e5933000 	ldr	r3, [r3]
80010c68:	e1a00003 	mov	r0, r3
80010c6c:	e3a01302 	mov	r1, #134217728	; 0x8000000
80010c70:	e3a02601 	mov	r2, #1048576	; 0x100000
80010c74:	eb000fbc 	bl	80014b6c <memcpy>
80010c78:	ebfffea3 	bl	8001070c <getPhyRamSize>
80010c7c:	e1a03000 	mov	r3, r0
80010c80:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80010c84:	e3a00000 	mov	r0, #0
80010c88:	e3480080 	movt	r0, #32896	; 0x8080
80010c8c:	e1a01003 	mov	r1, r3
80010c90:	eb000605 	bl	800124ac <kallocInit>
80010c94:	eb000905 	bl	800130b0 <procInit>
80010c98:	ebfffeb9 	bl	80010784 <uartInit>
80010c9c:	e59f30fc 	ldr	r3, [pc, #252]	; 80010da0 <kernelEntry+0x198>
80010ca0:	e08f3003 	add	r3, pc, r3
80010ca4:	e1a00003 	mov	r0, r3
80010ca8:	ebfffecc 	bl	800107e0 <uartPuts>
80010cac:	eb000992 	bl	800132fc <procCreate>
80010cb0:	e1a02000 	mov	r2, r0
80010cb4:	e59f30e8 	ldr	r3, [pc, #232]	; 80010da4 <kernelEntry+0x19c>
80010cb8:	e08f3003 	add	r3, pc, r3
80010cbc:	e5832000 	str	r2, [r3]
80010cc0:	e59f30d4 	ldr	r3, [pc, #212]	; 80010d9c <kernelEntry+0x194>
80010cc4:	e7943003 	ldr	r3, [r4, r3]
80010cc8:	e5933000 	ldr	r3, [r3]
80010ccc:	e1a00003 	mov	r0, r3
80010cd0:	e59f30d0 	ldr	r3, [pc, #208]	; 80010da8 <kernelEntry+0x1a0>
80010cd4:	e7943003 	ldr	r3, [r4, r3]
80010cd8:	e1a01003 	mov	r1, r3
80010cdc:	e59f30c8 	ldr	r3, [pc, #200]	; 80010dac <kernelEntry+0x1a4>
80010ce0:	e7943003 	ldr	r3, [r4, r3]
80010ce4:	e1a02003 	mov	r2, r3
80010ce8:	eb000e47 	bl	8001460c <ramdiskOpen>
80010cec:	e3a03000 	mov	r3, #0
80010cf0:	e50b301c 	str	r3, [fp, #-28]
80010cf4:	e24b201c 	sub	r2, fp, #28
80010cf8:	e59f30a8 	ldr	r3, [pc, #168]	; 80010da8 <kernelEntry+0x1a0>
80010cfc:	e7943003 	ldr	r3, [r4, r3]
80010d00:	e1a00003 	mov	r0, r3
80010d04:	e59f30a4 	ldr	r3, [pc, #164]	; 80010db0 <kernelEntry+0x1a8>
80010d08:	e08f3003 	add	r3, pc, r3
80010d0c:	e1a01003 	mov	r1, r3
80010d10:	eb000e85 	bl	8001472c <ramdiskRead>
80010d14:	e50b0018 	str	r0, [fp, #-24]
80010d18:	e51b3018 	ldr	r3, [fp, #-24]
80010d1c:	e3530000 	cmp	r3, #0
80010d20:	1a000004 	bne	80010d38 <kernelEntry+0x130>
80010d24:	e59f3088 	ldr	r3, [pc, #136]	; 80010db4 <kernelEntry+0x1ac>
80010d28:	e08f3003 	add	r3, pc, r3
80010d2c:	e1a00003 	mov	r0, r3
80010d30:	ebfffeaa 	bl	800107e0 <uartPuts>
80010d34:	ea000014 	b	80010d8c <kernelEntry+0x184>
80010d38:	e59f3078 	ldr	r3, [pc, #120]	; 80010db8 <kernelEntry+0x1b0>
80010d3c:	e08f3003 	add	r3, pc, r3
80010d40:	e5933000 	ldr	r3, [r3]
80010d44:	e1a00003 	mov	r0, r3
80010d48:	e51b1018 	ldr	r1, [fp, #-24]
80010d4c:	eb000aa3 	bl	800137e0 <procLoad>
80010d50:	e59f3064 	ldr	r3, [pc, #100]	; 80010dbc <kernelEntry+0x1b4>
80010d54:	e08f3003 	add	r3, pc, r3
80010d58:	e5933000 	ldr	r3, [r3]
80010d5c:	e2833010 	add	r3, r3, #16
80010d60:	e1a00003 	mov	r0, r3
80010d64:	e59f3054 	ldr	r3, [pc, #84]	; 80010dc0 <kernelEntry+0x1b8>
80010d68:	e08f3003 	add	r3, pc, r3
80010d6c:	e1a01003 	mov	r1, r3
80010d70:	e3a02080 	mov	r2, #128	; 0x80
80010d74:	eb000fbf 	bl	80014c78 <strncpy>
80010d78:	ebfffd5d 	bl	800102f4 <irqInit>
80010d7c:	ebfffe4a 	bl	800106ac <timerInit>
80010d80:	eb000f62 	bl	80014b10 <schedulerInit>
80010d84:	eb000f1e 	bl	80014a04 <schedule>
80010d88:	eafffffe 	b	80010d88 <kernelEntry+0x180>
80010d8c:	e24bd010 	sub	sp, fp, #16
80010d90:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80010d94:	0000f3e0 	andeq	pc, r0, r0, ror #7
80010d98:	00000018 	andeq	r0, r0, r8, lsl r0
80010d9c:	0000001c 	andeq	r0, r0, ip, lsl r0
80010da0:	000044f0 	strdeq	r4, [r0], -r0
80010da4:	00013360 	andeq	r3, r1, r0, ror #6
80010da8:	00000000 	andeq	r0, r0, r0
80010dac:	00000020 	andeq	r0, r0, r0, lsr #32
80010db0:	00004504 	andeq	r4, r0, r4, lsl #10
80010db4:	000044ec 	andeq	r4, r0, ip, ror #9
80010db8:	000132dc 	ldrdeq	r3, [r1], -ip
80010dbc:	000132c4 	andeq	r3, r1, r4, asr #5
80010dc0:	000044a4 	andeq	r4, r0, r4, lsr #9

80010dc4 <registerInterruptHandler>:
80010dc4:	e1a0c00d 	mov	ip, sp
80010dc8:	e92dd800 	push	{fp, ip, lr, pc}
80010dcc:	e24cb004 	sub	fp, ip, #4
80010dd0:	e24dd008 	sub	sp, sp, #8
80010dd4:	e50b0010 	str	r0, [fp, #-16]
80010dd8:	e50b1014 	str	r1, [fp, #-20]
80010ddc:	e51b0010 	ldr	r0, [fp, #-16]
80010de0:	ebfffd4f 	bl	80010324 <enableIRQ>
80010de4:	e59f3014 	ldr	r3, [pc, #20]	; 80010e00 <registerInterruptHandler+0x3c>
80010de8:	e08f3003 	add	r3, pc, r3
80010dec:	e51b2010 	ldr	r2, [fp, #-16]
80010df0:	e51b1014 	ldr	r1, [fp, #-20]
80010df4:	e7831102 	str	r1, [r3, r2, lsl #2]
80010df8:	e24bd00c 	sub	sp, fp, #12
80010dfc:	e89da800 	ldm	sp, {fp, sp, pc}
80010e00:	0001323c 	andeq	r3, r1, ip, lsr r2

80010e04 <handleIRQ>:
80010e04:	e1a0c00d 	mov	ip, sp
80010e08:	e92dd800 	push	{fp, ip, lr, pc}
80010e0c:	e24cb004 	sub	fp, ip, #4
80010e10:	e24dd088 	sub	sp, sp, #136	; 0x88
80010e14:	e3a03000 	mov	r3, #0
80010e18:	e50b3014 	str	r3, [fp, #-20]
80010e1c:	e24b3094 	sub	r3, fp, #148	; 0x94
80010e20:	e1a00003 	mov	r0, r3
80010e24:	ebfffd5e 	bl	800103a4 <getPendingIRQs>
80010e28:	e3a03000 	mov	r3, #0
80010e2c:	e50b3010 	str	r3, [fp, #-16]
80010e30:	ea000013 	b	80010e84 <handleIRQ+0x80>
80010e34:	e51b3010 	ldr	r3, [fp, #-16]
80010e38:	e1a03103 	lsl	r3, r3, #2
80010e3c:	e24b200c 	sub	r2, fp, #12
80010e40:	e0823003 	add	r3, r2, r3
80010e44:	e5133088 	ldr	r3, [r3, #-136]	; 0x88
80010e48:	e3530000 	cmp	r3, #0
80010e4c:	0a000009 	beq	80010e78 <handleIRQ+0x74>
80010e50:	e59f3040 	ldr	r3, [pc, #64]	; 80010e98 <handleIRQ+0x94>
80010e54:	e08f3003 	add	r3, pc, r3
80010e58:	e51b2010 	ldr	r2, [fp, #-16]
80010e5c:	e7933102 	ldr	r3, [r3, r2, lsl #2]
80010e60:	e50b3014 	str	r3, [fp, #-20]
80010e64:	e51b3014 	ldr	r3, [fp, #-20]
80010e68:	e3530000 	cmp	r3, #0
80010e6c:	0a000001 	beq	80010e78 <handleIRQ+0x74>
80010e70:	e51b3014 	ldr	r3, [fp, #-20]
80010e74:	e12fff33 	blx	r3
80010e78:	e51b3010 	ldr	r3, [fp, #-16]
80010e7c:	e2833001 	add	r3, r3, #1
80010e80:	e50b3010 	str	r3, [fp, #-16]
80010e84:	e51b3010 	ldr	r3, [fp, #-16]
80010e88:	e353001f 	cmp	r3, #31
80010e8c:	9affffe8 	bls	80010e34 <handleIRQ+0x30>
80010e90:	e24bd00c 	sub	sp, fp, #12
80010e94:	e89da800 	ldm	sp, {fp, sp, pc}
80010e98:	000131d0 	ldrdeq	r3, [r1], -r0

80010e9c <syscall_uartPutch>:
80010e9c:	e1a0c00d 	mov	ip, sp
80010ea0:	e92dd800 	push	{fp, ip, lr, pc}
80010ea4:	e24cb004 	sub	fp, ip, #4
80010ea8:	e24dd008 	sub	sp, sp, #8
80010eac:	e50b0010 	str	r0, [fp, #-16]
80010eb0:	e51b0010 	ldr	r0, [fp, #-16]
80010eb4:	ebfffe61 	bl	80010840 <uartPutch>
80010eb8:	e3a03000 	mov	r3, #0
80010ebc:	e1a00003 	mov	r0, r3
80010ec0:	e24bd00c 	sub	sp, fp, #12
80010ec4:	e89da800 	ldm	sp, {fp, sp, pc}

80010ec8 <syscall_uartGetch>:
80010ec8:	e1a0c00d 	mov	ip, sp
80010ecc:	e92dd800 	push	{fp, ip, lr, pc}
80010ed0:	e24cb004 	sub	fp, ip, #4
80010ed4:	e24dd008 	sub	sp, sp, #8
80010ed8:	ebfffe68 	bl	80010880 <uartGetch>
80010edc:	e50b0010 	str	r0, [fp, #-16]
80010ee0:	e51b3010 	ldr	r3, [fp, #-16]
80010ee4:	e1a00003 	mov	r0, r3
80010ee8:	e24bd00c 	sub	sp, fp, #12
80010eec:	e89da800 	ldm	sp, {fp, sp, pc}

80010ef0 <syscall_execElf>:
80010ef0:	e1a0c00d 	mov	ip, sp
80010ef4:	e92dd810 	push	{r4, fp, ip, lr, pc}
80010ef8:	e24cb004 	sub	fp, ip, #4
80010efc:	e24dd014 	sub	sp, sp, #20
80010f00:	e50b0020 	str	r0, [fp, #-32]
80010f04:	e50b1024 	str	r1, [fp, #-36]	; 0x24
80010f08:	e59f40a0 	ldr	r4, [pc, #160]	; 80010fb0 <syscall_execElf+0xc0>
80010f0c:	e08f4004 	add	r4, pc, r4
80010f10:	e51b3020 	ldr	r3, [fp, #-32]
80010f14:	e50b3018 	str	r3, [fp, #-24]
80010f18:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80010f1c:	e50b301c 	str	r3, [fp, #-28]
80010f20:	e51b301c 	ldr	r3, [fp, #-28]
80010f24:	e3530000 	cmp	r3, #0
80010f28:	0a000002 	beq	80010f38 <syscall_execElf+0x48>
80010f2c:	e51b3018 	ldr	r3, [fp, #-24]
80010f30:	e3530000 	cmp	r3, #0
80010f34:	1a000001 	bne	80010f40 <syscall_execElf+0x50>
80010f38:	e3e03000 	mvn	r3, #0
80010f3c:	ea000018 	b	80010fa4 <syscall_execElf+0xb4>
80010f40:	e59f306c 	ldr	r3, [pc, #108]	; 80010fb4 <syscall_execElf+0xc4>
80010f44:	e7943003 	ldr	r3, [r4, r3]
80010f48:	e5933000 	ldr	r3, [r3]
80010f4c:	e2833010 	add	r3, r3, #16
80010f50:	e1a00003 	mov	r0, r3
80010f54:	e51b1018 	ldr	r1, [fp, #-24]
80010f58:	e3a02080 	mov	r2, #128	; 0x80
80010f5c:	eb000f45 	bl	80014c78 <strncpy>
80010f60:	e59f304c 	ldr	r3, [pc, #76]	; 80010fb4 <syscall_execElf+0xc4>
80010f64:	e7943003 	ldr	r3, [r4, r3]
80010f68:	e5933000 	ldr	r3, [r3]
80010f6c:	e1a00003 	mov	r0, r3
80010f70:	e51b101c 	ldr	r1, [fp, #-28]
80010f74:	eb000a19 	bl	800137e0 <procLoad>
80010f78:	e1a03000 	mov	r3, r0
80010f7c:	e3530000 	cmp	r3, #0
80010f80:	1a000001 	bne	80010f8c <syscall_execElf+0x9c>
80010f84:	e3e03000 	mvn	r3, #0
80010f88:	ea000005 	b	80010fa4 <syscall_execElf+0xb4>
80010f8c:	e59f3020 	ldr	r3, [pc, #32]	; 80010fb4 <syscall_execElf+0xc4>
80010f90:	e7943003 	ldr	r3, [r4, r3]
80010f94:	e5933000 	ldr	r3, [r3]
80010f98:	e1a00003 	mov	r0, r3
80010f9c:	eb000a97 	bl	80013a00 <procStart>
80010fa0:	e3a03000 	mov	r3, #0
80010fa4:	e1a00003 	mov	r0, r3
80010fa8:	e24bd010 	sub	sp, fp, #16
80010fac:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80010fb0:	0000f0f0 	strdeq	pc, [r0], -r0
80010fb4:	00000014 	andeq	r0, r0, r4, lsl r0

80010fb8 <syscall_fork>:
80010fb8:	e1a0c00d 	mov	ip, sp
80010fbc:	e92dd800 	push	{fp, ip, lr, pc}
80010fc0:	e24cb004 	sub	fp, ip, #4
80010fc4:	eb000ac1 	bl	80013ad0 <kfork>
80010fc8:	e1a03000 	mov	r3, r0
80010fcc:	e1a00003 	mov	r0, r3
80010fd0:	e89da800 	ldm	sp, {fp, sp, pc}

80010fd4 <syscall_getpid>:
80010fd4:	e1a0c00d 	mov	ip, sp
80010fd8:	e92dd800 	push	{fp, ip, lr, pc}
80010fdc:	e24cb004 	sub	fp, ip, #4
80010fe0:	e59f2018 	ldr	r2, [pc, #24]	; 80011000 <syscall_getpid+0x2c>
80010fe4:	e08f2002 	add	r2, pc, r2
80010fe8:	e59f3014 	ldr	r3, [pc, #20]	; 80011004 <syscall_getpid+0x30>
80010fec:	e7923003 	ldr	r3, [r2, r3]
80010ff0:	e5933000 	ldr	r3, [r3]
80010ff4:	e5933004 	ldr	r3, [r3, #4]
80010ff8:	e1a00003 	mov	r0, r3
80010ffc:	e89da800 	ldm	sp, {fp, sp, pc}
80011000:	0000f018 	andeq	pc, r0, r8, lsl r0	; <UNPREDICTABLE>
80011004:	00000014 	andeq	r0, r0, r4, lsl r0

80011008 <syscall_exit>:
80011008:	e1a0c00d 	mov	ip, sp
8001100c:	e92dd800 	push	{fp, ip, lr, pc}
80011010:	e24cb004 	sub	fp, ip, #4
80011014:	e24dd008 	sub	sp, sp, #8
80011018:	e50b0010 	str	r0, [fp, #-16]
8001101c:	eb000b6e 	bl	80013ddc <procExit>
80011020:	e3a03000 	mov	r3, #0
80011024:	e1a00003 	mov	r0, r3
80011028:	e24bd00c 	sub	sp, fp, #12
8001102c:	e89da800 	ldm	sp, {fp, sp, pc}

80011030 <syscall_wait>:
80011030:	e1a0c00d 	mov	ip, sp
80011034:	e92dd810 	push	{r4, fp, ip, lr, pc}
80011038:	e24cb004 	sub	fp, ip, #4
8001103c:	e24dd014 	sub	sp, sp, #20
80011040:	e50b0020 	str	r0, [fp, #-32]
80011044:	e59f4054 	ldr	r4, [pc, #84]	; 800110a0 <syscall_wait+0x70>
80011048:	e08f4004 	add	r4, pc, r4
8001104c:	e51b0020 	ldr	r0, [fp, #-32]
80011050:	eb000a84 	bl	80013a68 <procGet>
80011054:	e50b0018 	str	r0, [fp, #-24]
80011058:	e51b3018 	ldr	r3, [fp, #-24]
8001105c:	e3530000 	cmp	r3, #0
80011060:	0a00000a 	beq	80011090 <syscall_wait+0x60>
80011064:	e59f3038 	ldr	r3, [pc, #56]	; 800110a4 <syscall_wait+0x74>
80011068:	e7943003 	ldr	r3, [r4, r3]
8001106c:	e5933000 	ldr	r3, [r3]
80011070:	e51b2020 	ldr	r2, [fp, #-32]
80011074:	e58321e8 	str	r2, [r3, #488]	; 0x1e8
80011078:	e59f3024 	ldr	r3, [pc, #36]	; 800110a4 <syscall_wait+0x74>
8001107c:	e7943003 	ldr	r3, [r4, r3]
80011080:	e5933000 	ldr	r3, [r3]
80011084:	e3a02002 	mov	r2, #2
80011088:	e5c32000 	strb	r2, [r3]
8001108c:	eb000e5c 	bl	80014a04 <schedule>
80011090:	e3a03000 	mov	r3, #0
80011094:	e1a00003 	mov	r0, r3
80011098:	e24bd010 	sub	sp, fp, #16
8001109c:	e89da810 	ldm	sp, {r4, fp, sp, pc}
800110a0:	0000efb4 			; <UNDEFINED> instruction: 0x0000efb4
800110a4:	00000014 	andeq	r0, r0, r4, lsl r0

800110a8 <syscall_yield>:
800110a8:	e1a0c00d 	mov	ip, sp
800110ac:	e92dd800 	push	{fp, ip, lr, pc}
800110b0:	e24cb004 	sub	fp, ip, #4
800110b4:	eb000e52 	bl	80014a04 <schedule>
800110b8:	e3a03000 	mov	r3, #0
800110bc:	e1a00003 	mov	r0, r3
800110c0:	e89da800 	ldm	sp, {fp, sp, pc}

800110c4 <syscall_pmalloc>:
800110c4:	e1a0c00d 	mov	ip, sp
800110c8:	e92dd800 	push	{fp, ip, lr, pc}
800110cc:	e24cb004 	sub	fp, ip, #4
800110d0:	e24dd010 	sub	sp, sp, #16
800110d4:	e50b0018 	str	r0, [fp, #-24]
800110d8:	e59f2034 	ldr	r2, [pc, #52]	; 80011114 <syscall_pmalloc+0x50>
800110dc:	e08f2002 	add	r2, pc, r2
800110e0:	e59f3030 	ldr	r3, [pc, #48]	; 80011118 <syscall_pmalloc+0x54>
800110e4:	e7923003 	ldr	r3, [r2, r3]
800110e8:	e5933000 	ldr	r3, [r3]
800110ec:	e2832e1f 	add	r2, r3, #496	; 0x1f0
800110f0:	e51b3018 	ldr	r3, [fp, #-24]
800110f4:	e1a00002 	mov	r0, r2
800110f8:	e1a01003 	mov	r1, r3
800110fc:	eb000638 	bl	800129e4 <trunkMalloc>
80011100:	e50b0010 	str	r0, [fp, #-16]
80011104:	e51b3010 	ldr	r3, [fp, #-16]
80011108:	e1a00003 	mov	r0, r3
8001110c:	e24bd00c 	sub	sp, fp, #12
80011110:	e89da800 	ldm	sp, {fp, sp, pc}
80011114:	0000ef20 	andeq	lr, r0, r0, lsr #30
80011118:	00000014 	andeq	r0, r0, r4, lsl r0

8001111c <syscall_pfree>:
8001111c:	e1a0c00d 	mov	ip, sp
80011120:	e92dd800 	push	{fp, ip, lr, pc}
80011124:	e24cb004 	sub	fp, ip, #4
80011128:	e24dd010 	sub	sp, sp, #16
8001112c:	e50b0018 	str	r0, [fp, #-24]
80011130:	e59f2034 	ldr	r2, [pc, #52]	; 8001116c <syscall_pfree+0x50>
80011134:	e08f2002 	add	r2, pc, r2
80011138:	e51b3018 	ldr	r3, [fp, #-24]
8001113c:	e50b3010 	str	r3, [fp, #-16]
80011140:	e59f3028 	ldr	r3, [pc, #40]	; 80011170 <syscall_pfree+0x54>
80011144:	e7923003 	ldr	r3, [r2, r3]
80011148:	e5933000 	ldr	r3, [r3]
8001114c:	e2833e1f 	add	r3, r3, #496	; 0x1f0
80011150:	e1a00003 	mov	r0, r3
80011154:	e51b1010 	ldr	r1, [fp, #-16]
80011158:	eb00073d 	bl	80012e54 <trunkFree>
8001115c:	e3a03000 	mov	r3, #0
80011160:	e1a00003 	mov	r0, r3
80011164:	e24bd00c 	sub	sp, fp, #12
80011168:	e89da800 	ldm	sp, {fp, sp, pc}
8001116c:	0000eec8 	andeq	lr, r0, r8, asr #29
80011170:	00000014 	andeq	r0, r0, r4, lsl r0

80011174 <syscall_sendMessage>:
80011174:	e1a0c00d 	mov	ip, sp
80011178:	e92dd800 	push	{fp, ip, lr, pc}
8001117c:	e24cb004 	sub	fp, ip, #4
80011180:	e24dd018 	sub	sp, sp, #24
80011184:	e50b0018 	str	r0, [fp, #-24]
80011188:	e50b101c 	str	r1, [fp, #-28]
8001118c:	e50b2020 	str	r2, [fp, #-32]
80011190:	e51b3020 	ldr	r3, [fp, #-32]
80011194:	e50b3010 	str	r3, [fp, #-16]
80011198:	e51b0018 	ldr	r0, [fp, #-24]
8001119c:	e51b101c 	ldr	r1, [fp, #-28]
800111a0:	e51b2010 	ldr	r2, [fp, #-16]
800111a4:	eb000b69 	bl	80013f50 <ksend>
800111a8:	e1a03000 	mov	r3, r0
800111ac:	e1a00003 	mov	r0, r3
800111b0:	e24bd00c 	sub	sp, fp, #12
800111b4:	e89da800 	ldm	sp, {fp, sp, pc}

800111b8 <syscall_readMessage>:
800111b8:	e1a0c00d 	mov	ip, sp
800111bc:	e92dd800 	push	{fp, ip, lr, pc}
800111c0:	e24cb004 	sub	fp, ip, #4
800111c4:	e24dd010 	sub	sp, sp, #16
800111c8:	e50b0018 	str	r0, [fp, #-24]
800111cc:	e51b0018 	ldr	r0, [fp, #-24]
800111d0:	eb000bce 	bl	80014110 <krecv>
800111d4:	e50b0010 	str	r0, [fp, #-16]
800111d8:	e51b3010 	ldr	r3, [fp, #-16]
800111dc:	e3530000 	cmp	r3, #0
800111e0:	1a000001 	bne	800111ec <syscall_readMessage+0x34>
800111e4:	e3a03000 	mov	r3, #0
800111e8:	ea000000 	b	800111f0 <syscall_readMessage+0x38>
800111ec:	e51b3010 	ldr	r3, [fp, #-16]
800111f0:	e1a00003 	mov	r0, r3
800111f4:	e24bd00c 	sub	sp, fp, #12
800111f8:	e89da800 	ldm	sp, {fp, sp, pc}

800111fc <syscall_readInitRD>:
800111fc:	e1a0c00d 	mov	ip, sp
80011200:	e92dd810 	push	{r4, fp, ip, lr, pc}
80011204:	e24cb004 	sub	fp, ip, #4
80011208:	e24dd02c 	sub	sp, sp, #44	; 0x2c
8001120c:	e50b0030 	str	r0, [fp, #-48]	; 0x30
80011210:	e50b1034 	str	r1, [fp, #-52]	; 0x34
80011214:	e50b2038 	str	r2, [fp, #-56]	; 0x38
80011218:	e59f4120 	ldr	r4, [pc, #288]	; 80011340 <syscall_readInitRD+0x144>
8001121c:	e08f4004 	add	r4, pc, r4
80011220:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
80011224:	e50b301c 	str	r3, [fp, #-28]
80011228:	e3a03000 	mov	r3, #0
8001122c:	e50b302c 	str	r3, [fp, #-44]	; 0x2c
80011230:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
80011234:	e5933000 	ldr	r3, [r3]
80011238:	e50b3018 	str	r3, [fp, #-24]
8001123c:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
80011240:	e3a02000 	mov	r2, #0
80011244:	e5832000 	str	r2, [r3]
80011248:	e24b202c 	sub	r2, fp, #44	; 0x2c
8001124c:	e59f30f0 	ldr	r3, [pc, #240]	; 80011344 <syscall_readInitRD+0x148>
80011250:	e7943003 	ldr	r3, [r4, r3]
80011254:	e1a00003 	mov	r0, r3
80011258:	e51b101c 	ldr	r1, [fp, #-28]
8001125c:	eb000d32 	bl	8001472c <ramdiskRead>
80011260:	e50b0020 	str	r0, [fp, #-32]
80011264:	e51b3020 	ldr	r3, [fp, #-32]
80011268:	e3530000 	cmp	r3, #0
8001126c:	0a000002 	beq	8001127c <syscall_readInitRD+0x80>
80011270:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
80011274:	e3530000 	cmp	r3, #0
80011278:	1a000001 	bne	80011284 <syscall_readInitRD+0x88>
8001127c:	e3a03000 	mov	r3, #0
80011280:	ea00002b 	b	80011334 <syscall_readInitRD+0x138>
80011284:	e51b202c 	ldr	r2, [fp, #-44]	; 0x2c
80011288:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
8001128c:	e0633002 	rsb	r3, r3, r2
80011290:	e50b3024 	str	r3, [fp, #-36]	; 0x24
80011294:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80011298:	e3530000 	cmp	r3, #0
8001129c:	ca000001 	bgt	800112a8 <syscall_readInitRD+0xac>
800112a0:	e3a03000 	mov	r3, #0
800112a4:	ea000022 	b	80011334 <syscall_readInitRD+0x138>
800112a8:	e51b3018 	ldr	r3, [fp, #-24]
800112ac:	e3530000 	cmp	r3, #0
800112b0:	da000003 	ble	800112c4 <syscall_readInitRD+0xc8>
800112b4:	e51b2018 	ldr	r2, [fp, #-24]
800112b8:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800112bc:	e1520003 	cmp	r2, r3
800112c0:	da000001 	ble	800112cc <syscall_readInitRD+0xd0>
800112c4:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800112c8:	e50b3018 	str	r3, [fp, #-24]
800112cc:	e59f3074 	ldr	r3, [pc, #116]	; 80011348 <syscall_readInitRD+0x14c>
800112d0:	e7943003 	ldr	r3, [r4, r3]
800112d4:	e5933000 	ldr	r3, [r3]
800112d8:	e2832e1f 	add	r2, r3, #496	; 0x1f0
800112dc:	e51b3018 	ldr	r3, [fp, #-24]
800112e0:	e1a00002 	mov	r0, r2
800112e4:	e1a01003 	mov	r1, r3
800112e8:	eb0005bd 	bl	800129e4 <trunkMalloc>
800112ec:	e50b0028 	str	r0, [fp, #-40]	; 0x28
800112f0:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
800112f4:	e3530000 	cmp	r3, #0
800112f8:	1a000001 	bne	80011304 <syscall_readInitRD+0x108>
800112fc:	e3a03000 	mov	r3, #0
80011300:	ea00000b 	b	80011334 <syscall_readInitRD+0x138>
80011304:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
80011308:	e51b2020 	ldr	r2, [fp, #-32]
8001130c:	e0822003 	add	r2, r2, r3
80011310:	e51b3018 	ldr	r3, [fp, #-24]
80011314:	e51b0028 	ldr	r0, [fp, #-40]	; 0x28
80011318:	e1a01002 	mov	r1, r2
8001131c:	e1a02003 	mov	r2, r3
80011320:	eb000e11 	bl	80014b6c <memcpy>
80011324:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
80011328:	e51b2018 	ldr	r2, [fp, #-24]
8001132c:	e5832000 	str	r2, [r3]
80011330:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80011334:	e1a00003 	mov	r0, r3
80011338:	e24bd010 	sub	sp, fp, #16
8001133c:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80011340:	0000ede0 	andeq	lr, r0, r0, ror #27
80011344:	00000000 	andeq	r0, r0, r0
80011348:	00000014 	andeq	r0, r0, r4, lsl r0

8001134c <syscall_filesInitRD>:
8001134c:	e1a0c00d 	mov	ip, sp
80011350:	e92dd800 	push	{fp, ip, lr, pc}
80011354:	e24cb004 	sub	fp, ip, #4
80011358:	e24dd018 	sub	sp, sp, #24
8001135c:	e59f3148 	ldr	r3, [pc, #328]	; 800114ac <syscall_filesInitRD+0x160>
80011360:	e08f3003 	add	r3, pc, r3
80011364:	e59f2144 	ldr	r2, [pc, #324]	; 800114b0 <syscall_filesInitRD+0x164>
80011368:	e7932002 	ldr	r2, [r3, r2]
8001136c:	e5922000 	ldr	r2, [r2]
80011370:	e50b2010 	str	r2, [fp, #-16]
80011374:	e59f2138 	ldr	r2, [pc, #312]	; 800114b4 <syscall_filesInitRD+0x168>
80011378:	e7933002 	ldr	r3, [r3, r2]
8001137c:	e5933000 	ldr	r3, [r3]
80011380:	e2833e1f 	add	r3, r3, #496	; 0x1f0
80011384:	e1a00003 	mov	r0, r3
80011388:	e3001401 	movw	r1, #1025	; 0x401
8001138c:	eb000594 	bl	800129e4 <trunkMalloc>
80011390:	e50b0020 	str	r0, [fp, #-32]
80011394:	e51b3020 	ldr	r3, [fp, #-32]
80011398:	e3530000 	cmp	r3, #0
8001139c:	1a000001 	bne	800113a8 <syscall_filesInitRD+0x5c>
800113a0:	e3a03000 	mov	r3, #0
800113a4:	ea00003d 	b	800114a0 <syscall_filesInitRD+0x154>
800113a8:	e3a03000 	mov	r3, #0
800113ac:	e50b3014 	str	r3, [fp, #-20]
800113b0:	ea000031 	b	8001147c <syscall_filesInitRD+0x130>
800113b4:	e3a03000 	mov	r3, #0
800113b8:	e50b3018 	str	r3, [fp, #-24]
800113bc:	e51b2010 	ldr	r2, [fp, #-16]
800113c0:	e51b3018 	ldr	r3, [fp, #-24]
800113c4:	e0823003 	add	r3, r2, r3
800113c8:	e5d33004 	ldrb	r3, [r3, #4]
800113cc:	e54b3019 	strb	r3, [fp, #-25]
800113d0:	ea000019 	b	8001143c <syscall_filesInitRD+0xf0>
800113d4:	e51b3014 	ldr	r3, [fp, #-20]
800113d8:	e51b2020 	ldr	r2, [fp, #-32]
800113dc:	e0823003 	add	r3, r2, r3
800113e0:	e55b2019 	ldrb	r2, [fp, #-25]
800113e4:	e5c32000 	strb	r2, [r3]
800113e8:	e51b3014 	ldr	r3, [fp, #-20]
800113ec:	e2833001 	add	r3, r3, #1
800113f0:	e50b3014 	str	r3, [fp, #-20]
800113f4:	e51b3018 	ldr	r3, [fp, #-24]
800113f8:	e2833001 	add	r3, r3, #1
800113fc:	e50b3018 	str	r3, [fp, #-24]
80011400:	e51b3014 	ldr	r3, [fp, #-20]
80011404:	e3530b01 	cmp	r3, #1024	; 0x400
80011408:	ba000006 	blt	80011428 <syscall_filesInitRD+0xdc>
8001140c:	e51b3014 	ldr	r3, [fp, #-20]
80011410:	e51b2020 	ldr	r2, [fp, #-32]
80011414:	e0823003 	add	r3, r2, r3
80011418:	e3a02000 	mov	r2, #0
8001141c:	e5c32000 	strb	r2, [r3]
80011420:	e51b3020 	ldr	r3, [fp, #-32]
80011424:	ea00001d 	b	800114a0 <syscall_filesInitRD+0x154>
80011428:	e51b2010 	ldr	r2, [fp, #-16]
8001142c:	e51b3018 	ldr	r3, [fp, #-24]
80011430:	e0823003 	add	r3, r2, r3
80011434:	e5d33004 	ldrb	r3, [r3, #4]
80011438:	e54b3019 	strb	r3, [fp, #-25]
8001143c:	e55b3019 	ldrb	r3, [fp, #-25]
80011440:	e3530000 	cmp	r3, #0
80011444:	1affffe2 	bne	800113d4 <syscall_filesInitRD+0x88>
80011448:	e51b3010 	ldr	r3, [fp, #-16]
8001144c:	e5933000 	ldr	r3, [r3]
80011450:	e50b3010 	str	r3, [fp, #-16]
80011454:	e51b3014 	ldr	r3, [fp, #-20]
80011458:	e2832001 	add	r2, r3, #1
8001145c:	e50b2014 	str	r2, [fp, #-20]
80011460:	e1a02003 	mov	r2, r3
80011464:	e51b3020 	ldr	r3, [fp, #-32]
80011468:	e0833002 	add	r3, r3, r2
8001146c:	e3a0200a 	mov	r2, #10
80011470:	e5c32000 	strb	r2, [r3]
80011474:	e3a03000 	mov	r3, #0
80011478:	e50b3018 	str	r3, [fp, #-24]
8001147c:	e51b3010 	ldr	r3, [fp, #-16]
80011480:	e3530000 	cmp	r3, #0
80011484:	1affffca 	bne	800113b4 <syscall_filesInitRD+0x68>
80011488:	e51b3014 	ldr	r3, [fp, #-20]
8001148c:	e51b2020 	ldr	r2, [fp, #-32]
80011490:	e0823003 	add	r3, r2, r3
80011494:	e3a02000 	mov	r2, #0
80011498:	e5c32000 	strb	r2, [r3]
8001149c:	e51b3020 	ldr	r3, [fp, #-32]
800114a0:	e1a00003 	mov	r0, r3
800114a4:	e24bd00c 	sub	sp, fp, #12
800114a8:	e89da800 	ldm	sp, {fp, sp, pc}
800114ac:	0000ec9c 	muleq	r0, ip, ip
800114b0:	00000000 	andeq	r0, r0, r0
800114b4:	00000014 	andeq	r0, r0, r4, lsl r0

800114b8 <syscall_infoInitRD>:
800114b8:	e1a0c00d 	mov	ip, sp
800114bc:	e92dd800 	push	{fp, ip, lr, pc}
800114c0:	e24cb004 	sub	fp, ip, #4
800114c4:	e24dd018 	sub	sp, sp, #24
800114c8:	e50b0020 	str	r0, [fp, #-32]
800114cc:	e50b1024 	str	r1, [fp, #-36]	; 0x24
800114d0:	e59f2078 	ldr	r2, [pc, #120]	; 80011550 <syscall_infoInitRD+0x98>
800114d4:	e08f2002 	add	r2, pc, r2
800114d8:	e51b3020 	ldr	r3, [fp, #-32]
800114dc:	e50b3010 	str	r3, [fp, #-16]
800114e0:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800114e4:	e50b3014 	str	r3, [fp, #-20]
800114e8:	e3a03000 	mov	r3, #0
800114ec:	e50b301c 	str	r3, [fp, #-28]
800114f0:	e24bc01c 	sub	ip, fp, #28
800114f4:	e59f3058 	ldr	r3, [pc, #88]	; 80011554 <syscall_infoInitRD+0x9c>
800114f8:	e7923003 	ldr	r3, [r2, r3]
800114fc:	e1a00003 	mov	r0, r3
80011500:	e51b1010 	ldr	r1, [fp, #-16]
80011504:	e1a0200c 	mov	r2, ip
80011508:	eb000c87 	bl	8001472c <ramdiskRead>
8001150c:	e50b0018 	str	r0, [fp, #-24]
80011510:	e51b3018 	ldr	r3, [fp, #-24]
80011514:	e3530000 	cmp	r3, #0
80011518:	1a000001 	bne	80011524 <syscall_infoInitRD+0x6c>
8001151c:	e3e03000 	mvn	r3, #0
80011520:	ea000007 	b	80011544 <syscall_infoInitRD+0x8c>
80011524:	e51b3014 	ldr	r3, [fp, #-20]
80011528:	e3a02001 	mov	r2, #1
8001152c:	e5832004 	str	r2, [r3, #4]
80011530:	e51b301c 	ldr	r3, [fp, #-28]
80011534:	e1a02003 	mov	r2, r3
80011538:	e51b3014 	ldr	r3, [fp, #-20]
8001153c:	e5832000 	str	r2, [r3]
80011540:	e3a03000 	mov	r3, #0
80011544:	e1a00003 	mov	r0, r3
80011548:	e24bd00c 	sub	sp, fp, #12
8001154c:	e89da800 	ldm	sp, {fp, sp, pc}
80011550:	0000eb28 	andeq	lr, r0, r8, lsr #22
80011554:	00000000 	andeq	r0, r0, r0

80011558 <syscall_kdb>:
80011558:	e1a0c00d 	mov	ip, sp
8001155c:	e92dd800 	push	{fp, ip, lr, pc}
80011560:	e24cb004 	sub	fp, ip, #4
80011564:	e24dd008 	sub	sp, sp, #8
80011568:	e50b0010 	str	r0, [fp, #-16]
8001156c:	e51b3010 	ldr	r3, [fp, #-16]
80011570:	e1a00003 	mov	r0, r3
80011574:	e24bd00c 	sub	sp, fp, #12
80011578:	e89da800 	ldm	sp, {fp, sp, pc}

8001157c <syscall_pfOpen>:
8001157c:	e1a0c00d 	mov	ip, sp
80011580:	e92dd800 	push	{fp, ip, lr, pc}
80011584:	e24cb004 	sub	fp, ip, #4
80011588:	e24dd020 	sub	sp, sp, #32
8001158c:	e50b0020 	str	r0, [fp, #-32]
80011590:	e50b1024 	str	r1, [fp, #-36]	; 0x24
80011594:	e50b2028 	str	r2, [fp, #-40]	; 0x28
80011598:	e51b0020 	ldr	r0, [fp, #-32]
8001159c:	eb000931 	bl	80013a68 <procGet>
800115a0:	e50b0014 	str	r0, [fp, #-20]
800115a4:	e51b3014 	ldr	r3, [fp, #-20]
800115a8:	e3530000 	cmp	r3, #0
800115ac:	1a000001 	bne	800115b8 <syscall_pfOpen+0x3c>
800115b0:	e3e03000 	mvn	r3, #0
800115b4:	ea00003d 	b	800116b0 <syscall_pfOpen+0x134>
800115b8:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800115bc:	e1a00003 	mov	r0, r3
800115c0:	eb000b55 	bl	8001431c <kfOpen>
800115c4:	e50b0018 	str	r0, [fp, #-24]
800115c8:	e51b3018 	ldr	r3, [fp, #-24]
800115cc:	e3530000 	cmp	r3, #0
800115d0:	1a000001 	bne	800115dc <syscall_pfOpen+0x60>
800115d4:	e3e03000 	mvn	r3, #0
800115d8:	ea000034 	b	800116b0 <syscall_pfOpen+0x134>
800115dc:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
800115e0:	e51b0018 	ldr	r0, [fp, #-24]
800115e4:	e1a01003 	mov	r1, r3
800115e8:	eb000bce 	bl	80014528 <kfRef>
800115ec:	e3a03000 	mov	r3, #0
800115f0:	e50b3010 	str	r3, [fp, #-16]
800115f4:	ea000025 	b	80011690 <syscall_pfOpen+0x114>
800115f8:	e51b2014 	ldr	r2, [fp, #-20]
800115fc:	e51b3010 	ldr	r3, [fp, #-16]
80011600:	e3a0100c 	mov	r1, #12
80011604:	e0030391 	mul	r3, r1, r3
80011608:	e0823003 	add	r3, r2, r3
8001160c:	e2833e21 	add	r3, r3, #528	; 0x210
80011610:	e5933000 	ldr	r3, [r3]
80011614:	e3530000 	cmp	r3, #0
80011618:	1a000019 	bne	80011684 <syscall_pfOpen+0x108>
8001161c:	e51b2014 	ldr	r2, [fp, #-20]
80011620:	e51b3010 	ldr	r3, [fp, #-16]
80011624:	e3a0100c 	mov	r1, #12
80011628:	e0030391 	mul	r3, r1, r3
8001162c:	e0823003 	add	r3, r2, r3
80011630:	e2833e21 	add	r3, r3, #528	; 0x210
80011634:	e51b2018 	ldr	r2, [fp, #-24]
80011638:	e5832000 	str	r2, [r3]
8001163c:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80011640:	e51b1014 	ldr	r1, [fp, #-20]
80011644:	e51b3010 	ldr	r3, [fp, #-16]
80011648:	e3a0000c 	mov	r0, #12
8001164c:	e0030390 	mul	r3, r0, r3
80011650:	e0813003 	add	r3, r1, r3
80011654:	e2833e21 	add	r3, r3, #528	; 0x210
80011658:	e5832004 	str	r2, [r3, #4]
8001165c:	e51b2014 	ldr	r2, [fp, #-20]
80011660:	e51b3010 	ldr	r3, [fp, #-16]
80011664:	e3a0100c 	mov	r1, #12
80011668:	e0030391 	mul	r3, r1, r3
8001166c:	e0823003 	add	r3, r2, r3
80011670:	e2833f86 	add	r3, r3, #536	; 0x218
80011674:	e3a02000 	mov	r2, #0
80011678:	e5832000 	str	r2, [r3]
8001167c:	e51b3010 	ldr	r3, [fp, #-16]
80011680:	ea00000a 	b	800116b0 <syscall_pfOpen+0x134>
80011684:	e51b3010 	ldr	r3, [fp, #-16]
80011688:	e2833001 	add	r3, r3, #1
8001168c:	e50b3010 	str	r3, [fp, #-16]
80011690:	e51b3010 	ldr	r3, [fp, #-16]
80011694:	e353001f 	cmp	r3, #31
80011698:	daffffd6 	ble	800115f8 <syscall_pfOpen+0x7c>
8001169c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
800116a0:	e51b0018 	ldr	r0, [fp, #-24]
800116a4:	e1a01003 	mov	r1, r3
800116a8:	eb000b50 	bl	800143f0 <kfUnref>
800116ac:	e3e03000 	mvn	r3, #0
800116b0:	e1a00003 	mov	r0, r3
800116b4:	e24bd00c 	sub	sp, fp, #12
800116b8:	e89da800 	ldm	sp, {fp, sp, pc}

800116bc <syscall_pfClose>:
800116bc:	e1a0c00d 	mov	ip, sp
800116c0:	e92dd800 	push	{fp, ip, lr, pc}
800116c4:	e24cb004 	sub	fp, ip, #4
800116c8:	e24dd010 	sub	sp, sp, #16
800116cc:	e50b0018 	str	r0, [fp, #-24]
800116d0:	e50b101c 	str	r1, [fp, #-28]
800116d4:	e51b0018 	ldr	r0, [fp, #-24]
800116d8:	eb0008e2 	bl	80013a68 <procGet>
800116dc:	e50b0010 	str	r0, [fp, #-16]
800116e0:	e51b3010 	ldr	r3, [fp, #-16]
800116e4:	e3530000 	cmp	r3, #0
800116e8:	1a000001 	bne	800116f4 <syscall_pfClose+0x38>
800116ec:	e3e03000 	mvn	r3, #0
800116f0:	ea000032 	b	800117c0 <syscall_pfClose+0x104>
800116f4:	e51b301c 	ldr	r3, [fp, #-28]
800116f8:	e50b3014 	str	r3, [fp, #-20]
800116fc:	e51b3014 	ldr	r3, [fp, #-20]
80011700:	e3530000 	cmp	r3, #0
80011704:	ba000002 	blt	80011714 <syscall_pfClose+0x58>
80011708:	e51b3014 	ldr	r3, [fp, #-20]
8001170c:	e353001f 	cmp	r3, #31
80011710:	da000001 	ble	8001171c <syscall_pfClose+0x60>
80011714:	e3e03000 	mvn	r3, #0
80011718:	ea000028 	b	800117c0 <syscall_pfClose+0x104>
8001171c:	e51b2010 	ldr	r2, [fp, #-16]
80011720:	e51b3014 	ldr	r3, [fp, #-20]
80011724:	e3a0100c 	mov	r1, #12
80011728:	e0030391 	mul	r3, r1, r3
8001172c:	e0823003 	add	r3, r2, r3
80011730:	e2833e21 	add	r3, r3, #528	; 0x210
80011734:	e5930000 	ldr	r0, [r3]
80011738:	e51b2010 	ldr	r2, [fp, #-16]
8001173c:	e51b3014 	ldr	r3, [fp, #-20]
80011740:	e3a0100c 	mov	r1, #12
80011744:	e0030391 	mul	r3, r1, r3
80011748:	e0823003 	add	r3, r2, r3
8001174c:	e2833e21 	add	r3, r3, #528	; 0x210
80011750:	e5933004 	ldr	r3, [r3, #4]
80011754:	e1a01003 	mov	r1, r3
80011758:	eb000b24 	bl	800143f0 <kfUnref>
8001175c:	e51b2010 	ldr	r2, [fp, #-16]
80011760:	e51b3014 	ldr	r3, [fp, #-20]
80011764:	e3a0100c 	mov	r1, #12
80011768:	e0030391 	mul	r3, r1, r3
8001176c:	e0823003 	add	r3, r2, r3
80011770:	e2833e21 	add	r3, r3, #528	; 0x210
80011774:	e3a02000 	mov	r2, #0
80011778:	e5832000 	str	r2, [r3]
8001177c:	e51b2010 	ldr	r2, [fp, #-16]
80011780:	e51b3014 	ldr	r3, [fp, #-20]
80011784:	e3a0100c 	mov	r1, #12
80011788:	e0030391 	mul	r3, r1, r3
8001178c:	e0823003 	add	r3, r2, r3
80011790:	e2833e21 	add	r3, r3, #528	; 0x210
80011794:	e3a02000 	mov	r2, #0
80011798:	e5832004 	str	r2, [r3, #4]
8001179c:	e51b2010 	ldr	r2, [fp, #-16]
800117a0:	e51b3014 	ldr	r3, [fp, #-20]
800117a4:	e3a0100c 	mov	r1, #12
800117a8:	e0030391 	mul	r3, r1, r3
800117ac:	e0823003 	add	r3, r2, r3
800117b0:	e2833f86 	add	r3, r3, #536	; 0x218
800117b4:	e3a02000 	mov	r2, #0
800117b8:	e5832000 	str	r2, [r3]
800117bc:	e3a03000 	mov	r3, #0
800117c0:	e1a00003 	mov	r0, r3
800117c4:	e24bd00c 	sub	sp, fp, #12
800117c8:	e89da800 	ldm	sp, {fp, sp, pc}

800117cc <syscall_pfNode>:
800117cc:	e1a0c00d 	mov	ip, sp
800117d0:	e92dd800 	push	{fp, ip, lr, pc}
800117d4:	e24cb004 	sub	fp, ip, #4
800117d8:	e24dd018 	sub	sp, sp, #24
800117dc:	e50b0020 	str	r0, [fp, #-32]
800117e0:	e50b1024 	str	r1, [fp, #-36]	; 0x24
800117e4:	e51b0020 	ldr	r0, [fp, #-32]
800117e8:	eb00089e 	bl	80013a68 <procGet>
800117ec:	e50b0010 	str	r0, [fp, #-16]
800117f0:	e51b3010 	ldr	r3, [fp, #-16]
800117f4:	e3530000 	cmp	r3, #0
800117f8:	1a000001 	bne	80011804 <syscall_pfNode+0x38>
800117fc:	e3e03000 	mvn	r3, #0
80011800:	ea000018 	b	80011868 <syscall_pfNode+0x9c>
80011804:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80011808:	e50b3014 	str	r3, [fp, #-20]
8001180c:	e51b3014 	ldr	r3, [fp, #-20]
80011810:	e3530000 	cmp	r3, #0
80011814:	ba000002 	blt	80011824 <syscall_pfNode+0x58>
80011818:	e51b3014 	ldr	r3, [fp, #-20]
8001181c:	e353001f 	cmp	r3, #31
80011820:	da000001 	ble	8001182c <syscall_pfNode+0x60>
80011824:	e3a03000 	mov	r3, #0
80011828:	ea00000e 	b	80011868 <syscall_pfNode+0x9c>
8001182c:	e51b2010 	ldr	r2, [fp, #-16]
80011830:	e51b3014 	ldr	r3, [fp, #-20]
80011834:	e3a0100c 	mov	r1, #12
80011838:	e0030391 	mul	r3, r1, r3
8001183c:	e0823003 	add	r3, r2, r3
80011840:	e2833e21 	add	r3, r3, #528	; 0x210
80011844:	e5933000 	ldr	r3, [r3]
80011848:	e50b3018 	str	r3, [fp, #-24]
8001184c:	e51b3018 	ldr	r3, [fp, #-24]
80011850:	e3530000 	cmp	r3, #0
80011854:	1a000001 	bne	80011860 <syscall_pfNode+0x94>
80011858:	e3a03000 	mov	r3, #0
8001185c:	ea000001 	b	80011868 <syscall_pfNode+0x9c>
80011860:	e51b3018 	ldr	r3, [fp, #-24]
80011864:	e5933008 	ldr	r3, [r3, #8]
80011868:	e1a00003 	mov	r0, r3
8001186c:	e24bd00c 	sub	sp, fp, #12
80011870:	e89da800 	ldm	sp, {fp, sp, pc}

80011874 <syscall_pfSeek>:
80011874:	e1a0c00d 	mov	ip, sp
80011878:	e92dd800 	push	{fp, ip, lr, pc}
8001187c:	e24cb004 	sub	fp, ip, #4
80011880:	e24dd020 	sub	sp, sp, #32
80011884:	e50b0020 	str	r0, [fp, #-32]
80011888:	e50b1024 	str	r1, [fp, #-36]	; 0x24
8001188c:	e50b2028 	str	r2, [fp, #-40]	; 0x28
80011890:	e51b0020 	ldr	r0, [fp, #-32]
80011894:	eb000873 	bl	80013a68 <procGet>
80011898:	e50b0010 	str	r0, [fp, #-16]
8001189c:	e51b3010 	ldr	r3, [fp, #-16]
800118a0:	e3530000 	cmp	r3, #0
800118a4:	1a000001 	bne	800118b0 <syscall_pfSeek+0x3c>
800118a8:	e3e03000 	mvn	r3, #0
800118ac:	ea000023 	b	80011940 <syscall_pfSeek+0xcc>
800118b0:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800118b4:	e50b3014 	str	r3, [fp, #-20]
800118b8:	e51b3014 	ldr	r3, [fp, #-20]
800118bc:	e3530000 	cmp	r3, #0
800118c0:	ba000002 	blt	800118d0 <syscall_pfSeek+0x5c>
800118c4:	e51b3014 	ldr	r3, [fp, #-20]
800118c8:	e353001f 	cmp	r3, #31
800118cc:	da000001 	ble	800118d8 <syscall_pfSeek+0x64>
800118d0:	e3e03000 	mvn	r3, #0
800118d4:	ea000019 	b	80011940 <syscall_pfSeek+0xcc>
800118d8:	e51b2010 	ldr	r2, [fp, #-16]
800118dc:	e51b3014 	ldr	r3, [fp, #-20]
800118e0:	e3a0100c 	mov	r1, #12
800118e4:	e0030391 	mul	r3, r1, r3
800118e8:	e0823003 	add	r3, r2, r3
800118ec:	e2833e21 	add	r3, r3, #528	; 0x210
800118f0:	e5933000 	ldr	r3, [r3]
800118f4:	e50b3018 	str	r3, [fp, #-24]
800118f8:	e51b3018 	ldr	r3, [fp, #-24]
800118fc:	e3530000 	cmp	r3, #0
80011900:	0a000003 	beq	80011914 <syscall_pfSeek+0xa0>
80011904:	e51b3018 	ldr	r3, [fp, #-24]
80011908:	e5933008 	ldr	r3, [r3, #8]
8001190c:	e3530000 	cmp	r3, #0
80011910:	1a000001 	bne	8001191c <syscall_pfSeek+0xa8>
80011914:	e3e03000 	mvn	r3, #0
80011918:	ea000008 	b	80011940 <syscall_pfSeek+0xcc>
8001191c:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80011920:	e51b1010 	ldr	r1, [fp, #-16]
80011924:	e51b3014 	ldr	r3, [fp, #-20]
80011928:	e3a0000c 	mov	r0, #12
8001192c:	e0030390 	mul	r3, r0, r3
80011930:	e0813003 	add	r3, r1, r3
80011934:	e2833f86 	add	r3, r3, #536	; 0x218
80011938:	e5832000 	str	r2, [r3]
8001193c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80011940:	e1a00003 	mov	r0, r3
80011944:	e24bd00c 	sub	sp, fp, #12
80011948:	e89da800 	ldm	sp, {fp, sp, pc}

8001194c <syscall_pfGetSeek>:
8001194c:	e1a0c00d 	mov	ip, sp
80011950:	e92dd800 	push	{fp, ip, lr, pc}
80011954:	e24cb004 	sub	fp, ip, #4
80011958:	e24dd018 	sub	sp, sp, #24
8001195c:	e50b0020 	str	r0, [fp, #-32]
80011960:	e50b1024 	str	r1, [fp, #-36]	; 0x24
80011964:	e51b0020 	ldr	r0, [fp, #-32]
80011968:	eb00083e 	bl	80013a68 <procGet>
8001196c:	e50b0010 	str	r0, [fp, #-16]
80011970:	e51b3010 	ldr	r3, [fp, #-16]
80011974:	e3530000 	cmp	r3, #0
80011978:	1a000001 	bne	80011984 <syscall_pfGetSeek+0x38>
8001197c:	e3e03000 	mvn	r3, #0
80011980:	ea000021 	b	80011a0c <syscall_pfGetSeek+0xc0>
80011984:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80011988:	e50b3014 	str	r3, [fp, #-20]
8001198c:	e51b3014 	ldr	r3, [fp, #-20]
80011990:	e3530000 	cmp	r3, #0
80011994:	ba000002 	blt	800119a4 <syscall_pfGetSeek+0x58>
80011998:	e51b3014 	ldr	r3, [fp, #-20]
8001199c:	e353001f 	cmp	r3, #31
800119a0:	da000001 	ble	800119ac <syscall_pfGetSeek+0x60>
800119a4:	e3e03000 	mvn	r3, #0
800119a8:	ea000017 	b	80011a0c <syscall_pfGetSeek+0xc0>
800119ac:	e51b2010 	ldr	r2, [fp, #-16]
800119b0:	e51b3014 	ldr	r3, [fp, #-20]
800119b4:	e3a0100c 	mov	r1, #12
800119b8:	e0030391 	mul	r3, r1, r3
800119bc:	e0823003 	add	r3, r2, r3
800119c0:	e2833e21 	add	r3, r3, #528	; 0x210
800119c4:	e5933000 	ldr	r3, [r3]
800119c8:	e50b3018 	str	r3, [fp, #-24]
800119cc:	e51b3018 	ldr	r3, [fp, #-24]
800119d0:	e3530000 	cmp	r3, #0
800119d4:	0a000003 	beq	800119e8 <syscall_pfGetSeek+0x9c>
800119d8:	e51b3018 	ldr	r3, [fp, #-24]
800119dc:	e5933008 	ldr	r3, [r3, #8]
800119e0:	e3530000 	cmp	r3, #0
800119e4:	1a000001 	bne	800119f0 <syscall_pfGetSeek+0xa4>
800119e8:	e3e03000 	mvn	r3, #0
800119ec:	ea000006 	b	80011a0c <syscall_pfGetSeek+0xc0>
800119f0:	e51b2010 	ldr	r2, [fp, #-16]
800119f4:	e51b3014 	ldr	r3, [fp, #-20]
800119f8:	e3a0100c 	mov	r1, #12
800119fc:	e0030391 	mul	r3, r1, r3
80011a00:	e0823003 	add	r3, r2, r3
80011a04:	e2833f86 	add	r3, r3, #536	; 0x218
80011a08:	e5933000 	ldr	r3, [r3]
80011a0c:	e1a00003 	mov	r0, r3
80011a10:	e24bd00c 	sub	sp, fp, #12
80011a14:	e89da800 	ldm	sp, {fp, sp, pc}

80011a18 <syscall_kservReg>:
80011a18:	e1a0c00d 	mov	ip, sp
80011a1c:	e92dd800 	push	{fp, ip, lr, pc}
80011a20:	e24cb004 	sub	fp, ip, #4
80011a24:	e24dd008 	sub	sp, sp, #8
80011a28:	e50b0010 	str	r0, [fp, #-16]
80011a2c:	e51b3010 	ldr	r3, [fp, #-16]
80011a30:	e1a00003 	mov	r0, r3
80011a34:	eb000b65 	bl	800147d0 <kservReg>
80011a38:	e1a03000 	mov	r3, r0
80011a3c:	e1a00003 	mov	r0, r3
80011a40:	e24bd00c 	sub	sp, fp, #12
80011a44:	e89da800 	ldm	sp, {fp, sp, pc}

80011a48 <syscall_kservGet>:
80011a48:	e1a0c00d 	mov	ip, sp
80011a4c:	e92dd800 	push	{fp, ip, lr, pc}
80011a50:	e24cb004 	sub	fp, ip, #4
80011a54:	e24dd008 	sub	sp, sp, #8
80011a58:	e50b0010 	str	r0, [fp, #-16]
80011a5c:	e51b3010 	ldr	r3, [fp, #-16]
80011a60:	e1a00003 	mov	r0, r3
80011a64:	eb000bb2 	bl	80014934 <kservGet>
80011a68:	e1a03000 	mov	r3, r0
80011a6c:	e1a00003 	mov	r0, r3
80011a70:	e24bd00c 	sub	sp, fp, #12
80011a74:	e89da800 	ldm	sp, {fp, sp, pc}

80011a78 <getProcs>:
80011a78:	e1a0c00d 	mov	ip, sp
80011a7c:	e92dd800 	push	{fp, ip, lr, pc}
80011a80:	e24cb004 	sub	fp, ip, #4
80011a84:	e24dd010 	sub	sp, sp, #16
80011a88:	e50b0018 	str	r0, [fp, #-24]
80011a8c:	e59f30c8 	ldr	r3, [pc, #200]	; 80011b5c <getProcs+0xe4>
80011a90:	e08f3003 	add	r3, pc, r3
80011a94:	e3a02000 	mov	r2, #0
80011a98:	e50b2010 	str	r2, [fp, #-16]
80011a9c:	e3a02000 	mov	r2, #0
80011aa0:	e50b2014 	str	r2, [fp, #-20]
80011aa4:	ea000025 	b	80011b40 <getProcs+0xc8>
80011aa8:	e59f20b0 	ldr	r2, [pc, #176]	; 80011b60 <getProcs+0xe8>
80011aac:	e7931002 	ldr	r1, [r3, r2]
80011ab0:	e51b2014 	ldr	r2, [fp, #-20]
80011ab4:	e3a00e39 	mov	r0, #912	; 0x390
80011ab8:	e0020290 	mul	r2, r0, r2
80011abc:	e0812002 	add	r2, r1, r2
80011ac0:	e5d22000 	ldrb	r2, [r2]
80011ac4:	e3520000 	cmp	r2, #0
80011ac8:	0a000019 	beq	80011b34 <getProcs+0xbc>
80011acc:	e59f2090 	ldr	r2, [pc, #144]	; 80011b64 <getProcs+0xec>
80011ad0:	e7932002 	ldr	r2, [r3, r2]
80011ad4:	e5922000 	ldr	r2, [r2]
80011ad8:	e592200c 	ldr	r2, [r2, #12]
80011adc:	e3520000 	cmp	r2, #0
80011ae0:	0a000010 	beq	80011b28 <getProcs+0xb0>
80011ae4:	e51b2018 	ldr	r2, [fp, #-24]
80011ae8:	e3520001 	cmp	r2, #1
80011aec:	1a00000d 	bne	80011b28 <getProcs+0xb0>
80011af0:	e59f2068 	ldr	r2, [pc, #104]	; 80011b60 <getProcs+0xe8>
80011af4:	e7931002 	ldr	r1, [r3, r2]
80011af8:	e51b2014 	ldr	r2, [fp, #-20]
80011afc:	e3a00e39 	mov	r0, #912	; 0x390
80011b00:	e0020290 	mul	r2, r0, r2
80011b04:	e0812002 	add	r2, r1, r2
80011b08:	e2822008 	add	r2, r2, #8
80011b0c:	e5921004 	ldr	r1, [r2, #4]
80011b10:	e59f204c 	ldr	r2, [pc, #76]	; 80011b64 <getProcs+0xec>
80011b14:	e7932002 	ldr	r2, [r3, r2]
80011b18:	e5922000 	ldr	r2, [r2]
80011b1c:	e592200c 	ldr	r2, [r2, #12]
80011b20:	e1510002 	cmp	r1, r2
80011b24:	1a000002 	bne	80011b34 <getProcs+0xbc>
80011b28:	e51b2010 	ldr	r2, [fp, #-16]
80011b2c:	e2822001 	add	r2, r2, #1
80011b30:	e50b2010 	str	r2, [fp, #-16]
80011b34:	e51b2014 	ldr	r2, [fp, #-20]
80011b38:	e2822001 	add	r2, r2, #1
80011b3c:	e50b2014 	str	r2, [fp, #-20]
80011b40:	e51b2014 	ldr	r2, [fp, #-20]
80011b44:	e352007f 	cmp	r2, #127	; 0x7f
80011b48:	daffffd6 	ble	80011aa8 <getProcs+0x30>
80011b4c:	e51b3010 	ldr	r3, [fp, #-16]
80011b50:	e1a00003 	mov	r0, r3
80011b54:	e24bd00c 	sub	sp, fp, #12
80011b58:	e89da800 	ldm	sp, {fp, sp, pc}
80011b5c:	0000e56c 	andeq	lr, r0, ip, ror #10
80011b60:	00000024 	andeq	r0, r0, r4, lsr #32
80011b64:	00000014 	andeq	r0, r0, r4, lsl r0

80011b68 <syscall_getProcs>:
80011b68:	e1a0c00d 	mov	ip, sp
80011b6c:	e92dd810 	push	{r4, fp, ip, lr, pc}
80011b70:	e24cb004 	sub	fp, ip, #4
80011b74:	e24dd024 	sub	sp, sp, #36	; 0x24
80011b78:	e50b0030 	str	r0, [fp, #-48]	; 0x30
80011b7c:	e50b1034 	str	r1, [fp, #-52]	; 0x34
80011b80:	e59f4268 	ldr	r4, [pc, #616]	; 80011df0 <syscall_getProcs+0x288>
80011b84:	e08f4004 	add	r4, pc, r4
80011b88:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
80011b8c:	e50b3020 	str	r3, [fp, #-32]
80011b90:	e51b0020 	ldr	r0, [fp, #-32]
80011b94:	ebffffb7 	bl	80011a78 <getProcs>
80011b98:	e50b0024 	str	r0, [fp, #-36]	; 0x24
80011b9c:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80011ba0:	e3530000 	cmp	r3, #0
80011ba4:	1a000001 	bne	80011bb0 <syscall_getProcs+0x48>
80011ba8:	e3a03000 	mov	r3, #0
80011bac:	ea00008c 	b	80011de4 <syscall_getProcs+0x27c>
80011bb0:	e59f323c 	ldr	r3, [pc, #572]	; 80011df4 <syscall_getProcs+0x28c>
80011bb4:	e7943003 	ldr	r3, [r4, r3]
80011bb8:	e5933000 	ldr	r3, [r3]
80011bbc:	e2831e1f 	add	r1, r3, #496	; 0x1f0
80011bc0:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80011bc4:	e3a02090 	mov	r2, #144	; 0x90
80011bc8:	e0030392 	mul	r3, r2, r3
80011bcc:	e1a00001 	mov	r0, r1
80011bd0:	e1a01003 	mov	r1, r3
80011bd4:	eb000382 	bl	800129e4 <trunkMalloc>
80011bd8:	e50b0028 	str	r0, [fp, #-40]	; 0x28
80011bdc:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80011be0:	e3530000 	cmp	r3, #0
80011be4:	1a000001 	bne	80011bf0 <syscall_getProcs+0x88>
80011be8:	e3a03000 	mov	r3, #0
80011bec:	ea00007c 	b	80011de4 <syscall_getProcs+0x27c>
80011bf0:	e3a03000 	mov	r3, #0
80011bf4:	e50b3018 	str	r3, [fp, #-24]
80011bf8:	e3a03000 	mov	r3, #0
80011bfc:	e50b301c 	str	r3, [fp, #-28]
80011c00:	ea00006c 	b	80011db8 <syscall_getProcs+0x250>
80011c04:	e59f31ec 	ldr	r3, [pc, #492]	; 80011df8 <syscall_getProcs+0x290>
80011c08:	e7942003 	ldr	r2, [r4, r3]
80011c0c:	e51b301c 	ldr	r3, [fp, #-28]
80011c10:	e3a01e39 	mov	r1, #912	; 0x390
80011c14:	e0030391 	mul	r3, r1, r3
80011c18:	e0823003 	add	r3, r2, r3
80011c1c:	e5d33000 	ldrb	r3, [r3]
80011c20:	e3530000 	cmp	r3, #0
80011c24:	0a000060 	beq	80011dac <syscall_getProcs+0x244>
80011c28:	e59f31c4 	ldr	r3, [pc, #452]	; 80011df4 <syscall_getProcs+0x28c>
80011c2c:	e7943003 	ldr	r3, [r4, r3]
80011c30:	e5933000 	ldr	r3, [r3]
80011c34:	e593300c 	ldr	r3, [r3, #12]
80011c38:	e3530000 	cmp	r3, #0
80011c3c:	0a000010 	beq	80011c84 <syscall_getProcs+0x11c>
80011c40:	e51b3020 	ldr	r3, [fp, #-32]
80011c44:	e3530001 	cmp	r3, #1
80011c48:	1a00000d 	bne	80011c84 <syscall_getProcs+0x11c>
80011c4c:	e59f31a4 	ldr	r3, [pc, #420]	; 80011df8 <syscall_getProcs+0x290>
80011c50:	e7942003 	ldr	r2, [r4, r3]
80011c54:	e51b301c 	ldr	r3, [fp, #-28]
80011c58:	e3a01e39 	mov	r1, #912	; 0x390
80011c5c:	e0030391 	mul	r3, r1, r3
80011c60:	e0823003 	add	r3, r2, r3
80011c64:	e2833008 	add	r3, r3, #8
80011c68:	e5932004 	ldr	r2, [r3, #4]
80011c6c:	e59f3180 	ldr	r3, [pc, #384]	; 80011df4 <syscall_getProcs+0x28c>
80011c70:	e7943003 	ldr	r3, [r4, r3]
80011c74:	e5933000 	ldr	r3, [r3]
80011c78:	e593300c 	ldr	r3, [r3, #12]
80011c7c:	e1520003 	cmp	r2, r3
80011c80:	1a000049 	bne	80011dac <syscall_getProcs+0x244>
80011c84:	e51b3018 	ldr	r3, [fp, #-24]
80011c88:	e3a02090 	mov	r2, #144	; 0x90
80011c8c:	e0030392 	mul	r3, r2, r3
80011c90:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80011c94:	e0823003 	add	r3, r2, r3
80011c98:	e59f2158 	ldr	r2, [pc, #344]	; 80011df8 <syscall_getProcs+0x290>
80011c9c:	e7941002 	ldr	r1, [r4, r2]
80011ca0:	e51b201c 	ldr	r2, [fp, #-28]
80011ca4:	e3a00e39 	mov	r0, #912	; 0x390
80011ca8:	e0020290 	mul	r2, r0, r2
80011cac:	e0812002 	add	r2, r1, r2
80011cb0:	e5922004 	ldr	r2, [r2, #4]
80011cb4:	e5832000 	str	r2, [r3]
80011cb8:	e51b3018 	ldr	r3, [fp, #-24]
80011cbc:	e3a02090 	mov	r2, #144	; 0x90
80011cc0:	e0030392 	mul	r3, r2, r3
80011cc4:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80011cc8:	e0823003 	add	r3, r2, r3
80011ccc:	e59f2124 	ldr	r2, [pc, #292]	; 80011df8 <syscall_getProcs+0x290>
80011cd0:	e7941002 	ldr	r1, [r4, r2]
80011cd4:	e51b201c 	ldr	r2, [fp, #-28]
80011cd8:	e3a00e39 	mov	r0, #912	; 0x390
80011cdc:	e0020290 	mul	r2, r0, r2
80011ce0:	e0812002 	add	r2, r1, r2
80011ce4:	e2822008 	add	r2, r2, #8
80011ce8:	e5922000 	ldr	r2, [r2]
80011cec:	e5832004 	str	r2, [r3, #4]
80011cf0:	e51b3018 	ldr	r3, [fp, #-24]
80011cf4:	e3a02090 	mov	r2, #144	; 0x90
80011cf8:	e0030392 	mul	r3, r2, r3
80011cfc:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80011d00:	e0823003 	add	r3, r2, r3
80011d04:	e59f20ec 	ldr	r2, [pc, #236]	; 80011df8 <syscall_getProcs+0x290>
80011d08:	e7941002 	ldr	r1, [r4, r2]
80011d0c:	e51b201c 	ldr	r2, [fp, #-28]
80011d10:	e3a00e39 	mov	r0, #912	; 0x390
80011d14:	e0020290 	mul	r2, r0, r2
80011d18:	e0812002 	add	r2, r1, r2
80011d1c:	e2822008 	add	r2, r2, #8
80011d20:	e5922004 	ldr	r2, [r2, #4]
80011d24:	e5832008 	str	r2, [r3, #8]
80011d28:	e51b3018 	ldr	r3, [fp, #-24]
80011d2c:	e3a02090 	mov	r2, #144	; 0x90
80011d30:	e0030392 	mul	r3, r2, r3
80011d34:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80011d38:	e0823003 	add	r3, r2, r3
80011d3c:	e59f20b4 	ldr	r2, [pc, #180]	; 80011df8 <syscall_getProcs+0x290>
80011d40:	e7941002 	ldr	r1, [r4, r2]
80011d44:	e51b201c 	ldr	r2, [fp, #-28]
80011d48:	e3a00e39 	mov	r0, #912	; 0x390
80011d4c:	e0020290 	mul	r2, r0, r2
80011d50:	e0812002 	add	r2, r1, r2
80011d54:	e2822f66 	add	r2, r2, #408	; 0x198
80011d58:	e5922000 	ldr	r2, [r2]
80011d5c:	e583200c 	str	r2, [r3, #12]
80011d60:	e51b3018 	ldr	r3, [fp, #-24]
80011d64:	e3a02090 	mov	r2, #144	; 0x90
80011d68:	e0030392 	mul	r3, r2, r3
80011d6c:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80011d70:	e0823003 	add	r3, r2, r3
80011d74:	e2831010 	add	r1, r3, #16
80011d78:	e51b301c 	ldr	r3, [fp, #-28]
80011d7c:	e3a02e39 	mov	r2, #912	; 0x390
80011d80:	e0030392 	mul	r3, r2, r3
80011d84:	e2833010 	add	r3, r3, #16
80011d88:	e59f2068 	ldr	r2, [pc, #104]	; 80011df8 <syscall_getProcs+0x290>
80011d8c:	e7942002 	ldr	r2, [r4, r2]
80011d90:	e0833002 	add	r3, r3, r2
80011d94:	e1a00001 	mov	r0, r1
80011d98:	e1a01003 	mov	r1, r3
80011d9c:	eb000b96 	bl	80014bfc <strcpy>
80011da0:	e51b3018 	ldr	r3, [fp, #-24]
80011da4:	e2833001 	add	r3, r3, #1
80011da8:	e50b3018 	str	r3, [fp, #-24]
80011dac:	e51b301c 	ldr	r3, [fp, #-28]
80011db0:	e2833001 	add	r3, r3, #1
80011db4:	e50b301c 	str	r3, [fp, #-28]
80011db8:	e51b301c 	ldr	r3, [fp, #-28]
80011dbc:	e353007f 	cmp	r3, #127	; 0x7f
80011dc0:	ca000003 	bgt	80011dd4 <syscall_getProcs+0x26c>
80011dc4:	e51b2018 	ldr	r2, [fp, #-24]
80011dc8:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80011dcc:	e1520003 	cmp	r2, r3
80011dd0:	baffff8b 	blt	80011c04 <syscall_getProcs+0x9c>
80011dd4:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
80011dd8:	e51b2018 	ldr	r2, [fp, #-24]
80011ddc:	e5832000 	str	r2, [r3]
80011de0:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80011de4:	e1a00003 	mov	r0, r3
80011de8:	e24bd010 	sub	sp, fp, #16
80011dec:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80011df0:	0000e478 	andeq	lr, r0, r8, ror r4
80011df4:	00000014 	andeq	r0, r0, r4, lsl r0
80011df8:	00000024 	andeq	r0, r0, r4, lsr #32

80011dfc <syscall_getCWD>:
80011dfc:	e1a0c00d 	mov	ip, sp
80011e00:	e92dd800 	push	{fp, ip, lr, pc}
80011e04:	e24cb004 	sub	fp, ip, #4
80011e08:	e24dd010 	sub	sp, sp, #16
80011e0c:	e50b0018 	str	r0, [fp, #-24]
80011e10:	e50b101c 	str	r1, [fp, #-28]
80011e14:	e59f2048 	ldr	r2, [pc, #72]	; 80011e64 <syscall_getCWD+0x68>
80011e18:	e08f2002 	add	r2, pc, r2
80011e1c:	e51b3018 	ldr	r3, [fp, #-24]
80011e20:	e50b3010 	str	r3, [fp, #-16]
80011e24:	e59f303c 	ldr	r3, [pc, #60]	; 80011e68 <syscall_getCWD+0x6c>
80011e28:	e7923003 	ldr	r3, [r2, r3]
80011e2c:	e5933000 	ldr	r3, [r3]
80011e30:	e2832090 	add	r2, r3, #144	; 0x90
80011e34:	e51b301c 	ldr	r3, [fp, #-28]
80011e38:	e3530c01 	cmp	r3, #256	; 0x100
80011e3c:	b1a03003 	movlt	r3, r3
80011e40:	a3a03c01 	movge	r3, #256	; 0x100
80011e44:	e51b0010 	ldr	r0, [fp, #-16]
80011e48:	e1a01002 	mov	r1, r2
80011e4c:	e1a02003 	mov	r2, r3
80011e50:	eb000b88 	bl	80014c78 <strncpy>
80011e54:	e51b3010 	ldr	r3, [fp, #-16]
80011e58:	e1a00003 	mov	r0, r3
80011e5c:	e24bd00c 	sub	sp, fp, #12
80011e60:	e89da800 	ldm	sp, {fp, sp, pc}
80011e64:	0000e1e4 	andeq	lr, r0, r4, ror #3
80011e68:	00000014 	andeq	r0, r0, r4, lsl r0

80011e6c <syscall_setCWD>:
80011e6c:	e1a0c00d 	mov	ip, sp
80011e70:	e92dd800 	push	{fp, ip, lr, pc}
80011e74:	e24cb004 	sub	fp, ip, #4
80011e78:	e24dd010 	sub	sp, sp, #16
80011e7c:	e50b0018 	str	r0, [fp, #-24]
80011e80:	e59f2038 	ldr	r2, [pc, #56]	; 80011ec0 <syscall_setCWD+0x54>
80011e84:	e08f2002 	add	r2, pc, r2
80011e88:	e51b3018 	ldr	r3, [fp, #-24]
80011e8c:	e50b3010 	str	r3, [fp, #-16]
80011e90:	e59f302c 	ldr	r3, [pc, #44]	; 80011ec4 <syscall_setCWD+0x58>
80011e94:	e7923003 	ldr	r3, [r2, r3]
80011e98:	e5933000 	ldr	r3, [r3]
80011e9c:	e2833090 	add	r3, r3, #144	; 0x90
80011ea0:	e1a00003 	mov	r0, r3
80011ea4:	e51b1010 	ldr	r1, [fp, #-16]
80011ea8:	e3a02c01 	mov	r2, #256	; 0x100
80011eac:	eb000b71 	bl	80014c78 <strncpy>
80011eb0:	e3a03000 	mov	r3, #0
80011eb4:	e1a00003 	mov	r0, r3
80011eb8:	e24bd00c 	sub	sp, fp, #12
80011ebc:	e89da800 	ldm	sp, {fp, sp, pc}
80011ec0:	0000e178 	andeq	lr, r0, r8, ror r1
80011ec4:	00000014 	andeq	r0, r0, r4, lsl r0

80011ec8 <syscall_getCmd>:
80011ec8:	e1a0c00d 	mov	ip, sp
80011ecc:	e92dd800 	push	{fp, ip, lr, pc}
80011ed0:	e24cb004 	sub	fp, ip, #4
80011ed4:	e24dd010 	sub	sp, sp, #16
80011ed8:	e50b0018 	str	r0, [fp, #-24]
80011edc:	e50b101c 	str	r1, [fp, #-28]
80011ee0:	e59f203c 	ldr	r2, [pc, #60]	; 80011f24 <syscall_getCmd+0x5c>
80011ee4:	e08f2002 	add	r2, pc, r2
80011ee8:	e51b3018 	ldr	r3, [fp, #-24]
80011eec:	e50b3010 	str	r3, [fp, #-16]
80011ef0:	e59f3030 	ldr	r3, [pc, #48]	; 80011f28 <syscall_getCmd+0x60>
80011ef4:	e7923003 	ldr	r3, [r2, r3]
80011ef8:	e5933000 	ldr	r3, [r3]
80011efc:	e2832010 	add	r2, r3, #16
80011f00:	e51b301c 	ldr	r3, [fp, #-28]
80011f04:	e51b0010 	ldr	r0, [fp, #-16]
80011f08:	e1a01002 	mov	r1, r2
80011f0c:	e1a02003 	mov	r2, r3
80011f10:	eb000b58 	bl	80014c78 <strncpy>
80011f14:	e51b3018 	ldr	r3, [fp, #-24]
80011f18:	e1a00003 	mov	r0, r3
80011f1c:	e24bd00c 	sub	sp, fp, #12
80011f20:	e89da800 	ldm	sp, {fp, sp, pc}
80011f24:	0000e118 	andeq	lr, r0, r8, lsl r1
80011f28:	00000014 	andeq	r0, r0, r4, lsl r0

80011f2c <syscall_setUID>:
80011f2c:	e1a0c00d 	mov	ip, sp
80011f30:	e92dd800 	push	{fp, ip, lr, pc}
80011f34:	e24cb004 	sub	fp, ip, #4
80011f38:	e24dd010 	sub	sp, sp, #16
80011f3c:	e50b0018 	str	r0, [fp, #-24]
80011f40:	e50b101c 	str	r1, [fp, #-28]
80011f44:	e59f2068 	ldr	r2, [pc, #104]	; 80011fb4 <syscall_setUID+0x88>
80011f48:	e08f2002 	add	r2, pc, r2
80011f4c:	e51b301c 	ldr	r3, [fp, #-28]
80011f50:	e3530000 	cmp	r3, #0
80011f54:	ba000005 	blt	80011f70 <syscall_setUID+0x44>
80011f58:	e59f3058 	ldr	r3, [pc, #88]	; 80011fb8 <syscall_setUID+0x8c>
80011f5c:	e7923003 	ldr	r3, [r2, r3]
80011f60:	e5933000 	ldr	r3, [r3]
80011f64:	e593300c 	ldr	r3, [r3, #12]
80011f68:	e3530000 	cmp	r3, #0
80011f6c:	ba000001 	blt	80011f78 <syscall_setUID+0x4c>
80011f70:	e3e03000 	mvn	r3, #0
80011f74:	ea00000b 	b	80011fa8 <syscall_setUID+0x7c>
80011f78:	e51b0018 	ldr	r0, [fp, #-24]
80011f7c:	eb0006b9 	bl	80013a68 <procGet>
80011f80:	e50b0010 	str	r0, [fp, #-16]
80011f84:	e51b3010 	ldr	r3, [fp, #-16]
80011f88:	e3530000 	cmp	r3, #0
80011f8c:	1a000001 	bne	80011f98 <syscall_setUID+0x6c>
80011f90:	e3e03000 	mvn	r3, #0
80011f94:	ea000003 	b	80011fa8 <syscall_setUID+0x7c>
80011f98:	e51b3010 	ldr	r3, [fp, #-16]
80011f9c:	e51b201c 	ldr	r2, [fp, #-28]
80011fa0:	e583200c 	str	r2, [r3, #12]
80011fa4:	e3a03000 	mov	r3, #0
80011fa8:	e1a00003 	mov	r0, r3
80011fac:	e24bd00c 	sub	sp, fp, #12
80011fb0:	e89da800 	ldm	sp, {fp, sp, pc}
80011fb4:	0000e0b4 	strheq	lr, [r0], -r4
80011fb8:	00000014 	andeq	r0, r0, r4, lsl r0

80011fbc <syscall_getUID>:
80011fbc:	e1a0c00d 	mov	ip, sp
80011fc0:	e92dd800 	push	{fp, ip, lr, pc}
80011fc4:	e24cb004 	sub	fp, ip, #4
80011fc8:	e24dd010 	sub	sp, sp, #16
80011fcc:	e50b0018 	str	r0, [fp, #-24]
80011fd0:	e51b0018 	ldr	r0, [fp, #-24]
80011fd4:	eb0006a3 	bl	80013a68 <procGet>
80011fd8:	e50b0010 	str	r0, [fp, #-16]
80011fdc:	e51b3010 	ldr	r3, [fp, #-16]
80011fe0:	e3530000 	cmp	r3, #0
80011fe4:	1a000001 	bne	80011ff0 <syscall_getUID+0x34>
80011fe8:	e3e03000 	mvn	r3, #0
80011fec:	ea000001 	b	80011ff8 <syscall_getUID+0x3c>
80011ff0:	e51b3010 	ldr	r3, [fp, #-16]
80011ff4:	e593300c 	ldr	r3, [r3, #12]
80011ff8:	e1a00003 	mov	r0, r3
80011ffc:	e24bd00c 	sub	sp, fp, #12
80012000:	e89da800 	ldm	sp, {fp, sp, pc}

80012004 <handleSyscall>:
80012004:	e1a0c00d 	mov	ip, sp
80012008:	e92dd800 	push	{fp, ip, lr, pc}
8001200c:	e24cb004 	sub	fp, ip, #4
80012010:	e24dd010 	sub	sp, sp, #16
80012014:	e50b0010 	str	r0, [fp, #-16]
80012018:	e50b1014 	str	r1, [fp, #-20]
8001201c:	e50b2018 	str	r2, [fp, #-24]
80012020:	e50b301c 	str	r3, [fp, #-28]
80012024:	e59f3028 	ldr	r3, [pc, #40]	; 80012054 <handleSyscall+0x50>
80012028:	e08f3003 	add	r3, pc, r3
8001202c:	e51b2010 	ldr	r2, [fp, #-16]
80012030:	e7933102 	ldr	r3, [r3, r2, lsl #2]
80012034:	e51b0014 	ldr	r0, [fp, #-20]
80012038:	e51b1018 	ldr	r1, [fp, #-24]
8001203c:	e51b201c 	ldr	r2, [fp, #-28]
80012040:	e12fff33 	blx	r3
80012044:	e1a03000 	mov	r3, r0
80012048:	e1a00003 	mov	r0, r3
8001204c:	e24bd00c 	sub	sp, fp, #12
80012050:	e89da800 	ldm	sp, {fp, sp, pc}
80012054:	0000e008 	andeq	lr, r0, r8

80012058 <mapPages>:
80012058:	e1a0c00d 	mov	ip, sp
8001205c:	e92dd800 	push	{fp, ip, lr, pc}
80012060:	e24cb004 	sub	fp, ip, #4
80012064:	e24dd028 	sub	sp, sp, #40	; 0x28
80012068:	e50b0028 	str	r0, [fp, #-40]	; 0x28
8001206c:	e50b102c 	str	r1, [fp, #-44]	; 0x2c
80012070:	e50b2030 	str	r2, [fp, #-48]	; 0x30
80012074:	e50b3034 	str	r3, [fp, #-52]	; 0x34
80012078:	e3a03000 	mov	r3, #0
8001207c:	e50b3010 	str	r3, [fp, #-16]
80012080:	e3a03000 	mov	r3, #0
80012084:	e50b3014 	str	r3, [fp, #-20]
80012088:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
8001208c:	e3c33eff 	bic	r3, r3, #4080	; 0xff0
80012090:	e3c3300f 	bic	r3, r3, #15
80012094:	e50b3018 	str	r3, [fp, #-24]
80012098:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
8001209c:	e3c33eff 	bic	r3, r3, #4080	; 0xff0
800120a0:	e3c3300f 	bic	r3, r3, #15
800120a4:	e50b301c 	str	r3, [fp, #-28]
800120a8:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
800120ac:	e2833eff 	add	r3, r3, #4080	; 0xff0
800120b0:	e283300f 	add	r3, r3, #15
800120b4:	e3c33eff 	bic	r3, r3, #4080	; 0xff0
800120b8:	e3c3300f 	bic	r3, r3, #15
800120bc:	e50b3020 	str	r3, [fp, #-32]
800120c0:	e51b3018 	ldr	r3, [fp, #-24]
800120c4:	e50b3014 	str	r3, [fp, #-20]
800120c8:	e51b301c 	ldr	r3, [fp, #-28]
800120cc:	e50b3010 	str	r3, [fp, #-16]
800120d0:	ea00000a 	b	80012100 <mapPages+0xa8>
800120d4:	e51b0028 	ldr	r0, [fp, #-40]	; 0x28
800120d8:	e51b1014 	ldr	r1, [fp, #-20]
800120dc:	e51b2010 	ldr	r2, [fp, #-16]
800120e0:	e59b3004 	ldr	r3, [fp, #4]
800120e4:	eb00000d 	bl	80012120 <mapPage>
800120e8:	e51b3014 	ldr	r3, [fp, #-20]
800120ec:	e2833a01 	add	r3, r3, #4096	; 0x1000
800120f0:	e50b3014 	str	r3, [fp, #-20]
800120f4:	e51b3010 	ldr	r3, [fp, #-16]
800120f8:	e2833a01 	add	r3, r3, #4096	; 0x1000
800120fc:	e50b3010 	str	r3, [fp, #-16]
80012100:	e51b2010 	ldr	r2, [fp, #-16]
80012104:	e51b3020 	ldr	r3, [fp, #-32]
80012108:	e1520003 	cmp	r2, r3
8001210c:	1afffff0 	bne	800120d4 <mapPages+0x7c>
80012110:	e3a03000 	mov	r3, #0
80012114:	e50b3018 	str	r3, [fp, #-24]
80012118:	e24bd00c 	sub	sp, fp, #12
8001211c:	e89da800 	ldm	sp, {fp, sp, pc}

80012120 <mapPage>:
80012120:	e1a0c00d 	mov	ip, sp
80012124:	e92dd800 	push	{fp, ip, lr, pc}
80012128:	e24cb004 	sub	fp, ip, #4
8001212c:	e24dd020 	sub	sp, sp, #32
80012130:	e50b0020 	str	r0, [fp, #-32]
80012134:	e50b1024 	str	r1, [fp, #-36]	; 0x24
80012138:	e50b2028 	str	r2, [fp, #-40]	; 0x28
8001213c:	e50b302c 	str	r3, [fp, #-44]	; 0x2c
80012140:	e3a03000 	mov	r3, #0
80012144:	e50b3010 	str	r3, [fp, #-16]
80012148:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8001214c:	e1a03a23 	lsr	r3, r3, #20
80012150:	e50b3014 	str	r3, [fp, #-20]
80012154:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80012158:	e1a03623 	lsr	r3, r3, #12
8001215c:	e6ef3073 	uxtb	r3, r3
80012160:	e50b3018 	str	r3, [fp, #-24]
80012164:	e51b3014 	ldr	r3, [fp, #-20]
80012168:	e1a03103 	lsl	r3, r3, #2
8001216c:	e51b2020 	ldr	r2, [fp, #-32]
80012170:	e0823003 	add	r3, r2, r3
80012174:	e5d33000 	ldrb	r3, [r3]
80012178:	e2033003 	and	r3, r3, #3
8001217c:	e6ef3073 	uxtb	r3, r3
80012180:	e3530000 	cmp	r3, #0
80012184:	1a000020 	bne	8001220c <mapPage+0xec>
80012188:	eb000122 	bl	80012618 <kalloc1k>
8001218c:	e50b0010 	str	r0, [fp, #-16]
80012190:	e51b0010 	ldr	r0, [fp, #-16]
80012194:	e3a01000 	mov	r1, #0
80012198:	e3a02b01 	mov	r2, #1024	; 0x400
8001219c:	eb000bc5 	bl	800150b8 <memset>
800121a0:	e51b3014 	ldr	r3, [fp, #-20]
800121a4:	e1a03103 	lsl	r3, r3, #2
800121a8:	e51b2020 	ldr	r2, [fp, #-32]
800121ac:	e0822003 	add	r2, r2, r3
800121b0:	e51b3010 	ldr	r3, [fp, #-16]
800121b4:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
800121b8:	e1a03523 	lsr	r3, r3, #10
800121bc:	e7f51053 	ubfx	r1, r3, #0, #22
800121c0:	e5923000 	ldr	r3, [r2]
800121c4:	e7df3511 	bfi	r3, r1, #10, #22
800121c8:	e5823000 	str	r3, [r2]
800121cc:	e51b3014 	ldr	r3, [fp, #-20]
800121d0:	e1a03103 	lsl	r3, r3, #2
800121d4:	e51b2020 	ldr	r2, [fp, #-32]
800121d8:	e0822003 	add	r2, r2, r3
800121dc:	e5d23000 	ldrb	r3, [r2]
800121e0:	e3a01001 	mov	r1, #1
800121e4:	e7c13011 	bfi	r3, r1, #0, #2
800121e8:	e5c23000 	strb	r3, [r2]
800121ec:	e51b3014 	ldr	r3, [fp, #-20]
800121f0:	e1a03103 	lsl	r3, r3, #2
800121f4:	e51b2020 	ldr	r2, [fp, #-32]
800121f8:	e0822003 	add	r2, r2, r3
800121fc:	e1d230b0 	ldrh	r3, [r2]
80012200:	e7c8329f 	bfc	r3, #5, #4
80012204:	e1c230b0 	strh	r3, [r2]
80012208:	ea000008 	b	80012230 <mapPage+0x110>
8001220c:	e51b3014 	ldr	r3, [fp, #-20]
80012210:	e1a03103 	lsl	r3, r3, #2
80012214:	e51b2020 	ldr	r2, [fp, #-32]
80012218:	e0823003 	add	r3, r2, r3
8001221c:	e5933000 	ldr	r3, [r3]
80012220:	e7f53553 	ubfx	r3, r3, #10, #22
80012224:	e1a03503 	lsl	r3, r3, #10
80012228:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
8001222c:	e50b3010 	str	r3, [fp, #-16]
80012230:	e51b3018 	ldr	r3, [fp, #-24]
80012234:	e1a03103 	lsl	r3, r3, #2
80012238:	e51b2010 	ldr	r2, [fp, #-16]
8001223c:	e0822003 	add	r2, r2, r3
80012240:	e5d23000 	ldrb	r3, [r2]
80012244:	e3a01002 	mov	r1, #2
80012248:	e7c13011 	bfi	r3, r1, #0, #2
8001224c:	e5c23000 	strb	r3, [r2]
80012250:	e51b3018 	ldr	r3, [fp, #-24]
80012254:	e1a03103 	lsl	r3, r3, #2
80012258:	e51b2010 	ldr	r2, [fp, #-16]
8001225c:	e0822003 	add	r2, r2, r3
80012260:	e5d23000 	ldrb	r3, [r2]
80012264:	e7c2311f 	bfc	r3, #2, #1
80012268:	e5c23000 	strb	r3, [r2]
8001226c:	e51b3018 	ldr	r3, [fp, #-24]
80012270:	e1a03103 	lsl	r3, r3, #2
80012274:	e51b2010 	ldr	r2, [fp, #-16]
80012278:	e0822003 	add	r2, r2, r3
8001227c:	e5d23000 	ldrb	r3, [r2]
80012280:	e7c3319f 	bfc	r3, #3, #1
80012284:	e5c23000 	strb	r3, [r2]
80012288:	e51b3018 	ldr	r3, [fp, #-24]
8001228c:	e1a03103 	lsl	r3, r3, #2
80012290:	e51b2010 	ldr	r2, [fp, #-16]
80012294:	e0822003 	add	r2, r2, r3
80012298:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
8001229c:	e6ef1073 	uxtb	r1, r3
800122a0:	e1d230b0 	ldrh	r3, [r2]
800122a4:	e7cb3211 	bfi	r3, r1, #4, #8
800122a8:	e1c230b0 	strh	r3, [r2]
800122ac:	e51b3018 	ldr	r3, [fp, #-24]
800122b0:	e1a03103 	lsl	r3, r3, #2
800122b4:	e51b2010 	ldr	r2, [fp, #-16]
800122b8:	e0822003 	add	r2, r2, r3
800122bc:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
800122c0:	e1a03623 	lsr	r3, r3, #12
800122c4:	e7f31053 	ubfx	r1, r3, #0, #20
800122c8:	e5923000 	ldr	r3, [r2]
800122cc:	e7df3611 	bfi	r3, r1, #12, #20
800122d0:	e5823000 	str	r3, [r2]
800122d4:	e24bd00c 	sub	sp, fp, #12
800122d8:	e89da800 	ldm	sp, {fp, sp, pc}

800122dc <unmapPage>:
800122dc:	e1a0c00d 	mov	ip, sp
800122e0:	e92dd800 	push	{fp, ip, lr, pc}
800122e4:	e24cb004 	sub	fp, ip, #4
800122e8:	e24dd018 	sub	sp, sp, #24
800122ec:	e50b0020 	str	r0, [fp, #-32]
800122f0:	e50b1024 	str	r1, [fp, #-36]	; 0x24
800122f4:	e3a03000 	mov	r3, #0
800122f8:	e50b3010 	str	r3, [fp, #-16]
800122fc:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80012300:	e1a03a23 	lsr	r3, r3, #20
80012304:	e50b3014 	str	r3, [fp, #-20]
80012308:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8001230c:	e1a03623 	lsr	r3, r3, #12
80012310:	e6ef3073 	uxtb	r3, r3
80012314:	e50b3018 	str	r3, [fp, #-24]
80012318:	e51b3014 	ldr	r3, [fp, #-20]
8001231c:	e1a03103 	lsl	r3, r3, #2
80012320:	e51b2020 	ldr	r2, [fp, #-32]
80012324:	e0823003 	add	r3, r2, r3
80012328:	e5933000 	ldr	r3, [r3]
8001232c:	e7f53553 	ubfx	r3, r3, #10, #22
80012330:	e1a03503 	lsl	r3, r3, #10
80012334:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80012338:	e50b3010 	str	r3, [fp, #-16]
8001233c:	e51b3018 	ldr	r3, [fp, #-24]
80012340:	e1a03103 	lsl	r3, r3, #2
80012344:	e51b2010 	ldr	r2, [fp, #-16]
80012348:	e0822003 	add	r2, r2, r3
8001234c:	e5d23000 	ldrb	r3, [r2]
80012350:	e7c1301f 	bfc	r3, #0, #2
80012354:	e5c23000 	strb	r3, [r2]
80012358:	e24bd00c 	sub	sp, fp, #12
8001235c:	e89da800 	ldm	sp, {fp, sp, pc}

80012360 <resolvePhyAddress>:
80012360:	e1a0c00d 	mov	ip, sp
80012364:	e92dd800 	push	{fp, ip, lr, pc}
80012368:	e24cb004 	sub	fp, ip, #4
8001236c:	e24dd018 	sub	sp, sp, #24
80012370:	e50b0020 	str	r0, [fp, #-32]
80012374:	e50b1024 	str	r1, [fp, #-36]	; 0x24
80012378:	e3a03000 	mov	r3, #0
8001237c:	e50b3010 	str	r3, [fp, #-16]
80012380:	e3a03000 	mov	r3, #0
80012384:	e50b3014 	str	r3, [fp, #-20]
80012388:	e3a03000 	mov	r3, #0
8001238c:	e50b3018 	str	r3, [fp, #-24]
80012390:	e3a03000 	mov	r3, #0
80012394:	e50b301c 	str	r3, [fp, #-28]
80012398:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8001239c:	e1a03a23 	lsr	r3, r3, #20
800123a0:	e1a02103 	lsl	r2, r3, #2
800123a4:	e51b3020 	ldr	r3, [fp, #-32]
800123a8:	e1823003 	orr	r3, r2, r3
800123ac:	e50b3010 	str	r3, [fp, #-16]
800123b0:	e51b3010 	ldr	r3, [fp, #-16]
800123b4:	e5933000 	ldr	r3, [r3]
800123b8:	e7f53553 	ubfx	r3, r3, #10, #22
800123bc:	e1a03503 	lsl	r3, r3, #10
800123c0:	e50b301c 	str	r3, [fp, #-28]
800123c4:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800123c8:	e1a03523 	lsr	r3, r3, #10
800123cc:	e2032fff 	and	r2, r3, #1020	; 0x3fc
800123d0:	e51b301c 	ldr	r3, [fp, #-28]
800123d4:	e1823003 	orr	r3, r2, r3
800123d8:	e50b3014 	str	r3, [fp, #-20]
800123dc:	e51b3014 	ldr	r3, [fp, #-20]
800123e0:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
800123e4:	e50b3014 	str	r3, [fp, #-20]
800123e8:	e51b3014 	ldr	r3, [fp, #-20]
800123ec:	e5933000 	ldr	r3, [r3]
800123f0:	e7f33653 	ubfx	r3, r3, #12, #20
800123f4:	e1a03603 	lsl	r3, r3, #12
800123f8:	e1a02003 	mov	r2, r3
800123fc:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80012400:	e7eb3053 	ubfx	r3, r3, #0, #12
80012404:	e1823003 	orr	r3, r2, r3
80012408:	e50b3018 	str	r3, [fp, #-24]
8001240c:	e51b3018 	ldr	r3, [fp, #-24]
80012410:	e1a00003 	mov	r0, r3
80012414:	e24bd00c 	sub	sp, fp, #12
80012418:	e89da800 	ldm	sp, {fp, sp, pc}

8001241c <freePageTables>:
8001241c:	e1a0c00d 	mov	ip, sp
80012420:	e92dd800 	push	{fp, ip, lr, pc}
80012424:	e24cb004 	sub	fp, ip, #4
80012428:	e24dd010 	sub	sp, sp, #16
8001242c:	e50b0018 	str	r0, [fp, #-24]
80012430:	e3a03000 	mov	r3, #0
80012434:	e50b3010 	str	r3, [fp, #-16]
80012438:	ea000016 	b	80012498 <freePageTables+0x7c>
8001243c:	e51b3010 	ldr	r3, [fp, #-16]
80012440:	e1a03103 	lsl	r3, r3, #2
80012444:	e51b2018 	ldr	r2, [fp, #-24]
80012448:	e0823003 	add	r3, r2, r3
8001244c:	e5d33000 	ldrb	r3, [r3]
80012450:	e2033003 	and	r3, r3, #3
80012454:	e6ef3073 	uxtb	r3, r3
80012458:	e3530000 	cmp	r3, #0
8001245c:	0a00000a 	beq	8001248c <freePageTables+0x70>
80012460:	e51b3010 	ldr	r3, [fp, #-16]
80012464:	e1a03103 	lsl	r3, r3, #2
80012468:	e51b2018 	ldr	r2, [fp, #-24]
8001246c:	e0823003 	add	r3, r2, r3
80012470:	e5933000 	ldr	r3, [r3]
80012474:	e7f53553 	ubfx	r3, r3, #10, #22
80012478:	e1a03503 	lsl	r3, r3, #10
8001247c:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80012480:	e50b3014 	str	r3, [fp, #-20]
80012484:	e51b0014 	ldr	r0, [fp, #-20]
80012488:	eb000099 	bl	800126f4 <kfree1k>
8001248c:	e51b3010 	ldr	r3, [fp, #-16]
80012490:	e2833001 	add	r3, r3, #1
80012494:	e50b3010 	str	r3, [fp, #-16]
80012498:	e51b3010 	ldr	r3, [fp, #-16]
8001249c:	e3530a01 	cmp	r3, #4096	; 0x1000
800124a0:	baffffe5 	blt	8001243c <freePageTables+0x20>
800124a4:	e24bd00c 	sub	sp, fp, #12
800124a8:	e89da800 	ldm	sp, {fp, sp, pc}

800124ac <kallocInit>:
800124ac:	e1a0c00d 	mov	ip, sp
800124b0:	e92dd800 	push	{fp, ip, lr, pc}
800124b4:	e24cb004 	sub	fp, ip, #4
800124b8:	e24dd018 	sub	sp, sp, #24
800124bc:	e50b0020 	str	r0, [fp, #-32]
800124c0:	e50b1024 	str	r1, [fp, #-36]	; 0x24
800124c4:	e51b3020 	ldr	r3, [fp, #-32]
800124c8:	e2833eff 	add	r3, r3, #4080	; 0xff0
800124cc:	e283300f 	add	r3, r3, #15
800124d0:	e3c33eff 	bic	r3, r3, #4080	; 0xff0
800124d4:	e3c3300f 	bic	r3, r3, #15
800124d8:	e50b3014 	str	r3, [fp, #-20]
800124dc:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800124e0:	e3c33eff 	bic	r3, r3, #4080	; 0xff0
800124e4:	e3c3300f 	bic	r3, r3, #15
800124e8:	e50b3018 	str	r3, [fp, #-24]
800124ec:	e3a03000 	mov	r3, #0
800124f0:	e50b3010 	str	r3, [fp, #-16]
800124f4:	e51b3014 	ldr	r3, [fp, #-20]
800124f8:	e50b3010 	str	r3, [fp, #-16]
800124fc:	ea00000c 	b	80012534 <kallocInit+0x88>
80012500:	e59f3044 	ldr	r3, [pc, #68]	; 8001254c <kallocInit+0xa0>
80012504:	e08f3003 	add	r3, pc, r3
80012508:	e5933000 	ldr	r3, [r3]
8001250c:	e1a00003 	mov	r0, r3
80012510:	e51b1010 	ldr	r1, [fp, #-16]
80012514:	eb0000b3 	bl	800127e8 <pageListPrepend>
80012518:	e1a02000 	mov	r2, r0
8001251c:	e59f302c 	ldr	r3, [pc, #44]	; 80012550 <kallocInit+0xa4>
80012520:	e08f3003 	add	r3, pc, r3
80012524:	e5832000 	str	r2, [r3]
80012528:	e51b3010 	ldr	r3, [fp, #-16]
8001252c:	e2833a01 	add	r3, r3, #4096	; 0x1000
80012530:	e50b3010 	str	r3, [fp, #-16]
80012534:	e51b2010 	ldr	r2, [fp, #-16]
80012538:	e51b3018 	ldr	r3, [fp, #-24]
8001253c:	e1520003 	cmp	r2, r3
80012540:	1affffee 	bne	80012500 <kallocInit+0x54>
80012544:	e24bd00c 	sub	sp, fp, #12
80012548:	e89da800 	ldm	sp, {fp, sp, pc}
8001254c:	00014af4 	strdeq	r4, [r1], -r4	; <UNPREDICTABLE>
80012550:	00014ad8 	ldrdeq	r4, [r1], -r8

80012554 <kalloc>:
80012554:	e1a0c00d 	mov	ip, sp
80012558:	e92dd800 	push	{fp, ip, lr, pc}
8001255c:	e24cb004 	sub	fp, ip, #4
80012560:	e24dd008 	sub	sp, sp, #8
80012564:	e3a03000 	mov	r3, #0
80012568:	e50b3010 	str	r3, [fp, #-16]
8001256c:	e59f3048 	ldr	r3, [pc, #72]	; 800125bc <kalloc+0x68>
80012570:	e08f3003 	add	r3, pc, r3
80012574:	e5933000 	ldr	r3, [r3]
80012578:	e3530000 	cmp	r3, #0
8001257c:	0a00000a 	beq	800125ac <kalloc+0x58>
80012580:	e59f3038 	ldr	r3, [pc, #56]	; 800125c0 <kalloc+0x6c>
80012584:	e08f3003 	add	r3, pc, r3
80012588:	e5933000 	ldr	r3, [r3]
8001258c:	e50b3010 	str	r3, [fp, #-16]
80012590:	e59f302c 	ldr	r3, [pc, #44]	; 800125c4 <kalloc+0x70>
80012594:	e08f3003 	add	r3, pc, r3
80012598:	e5933000 	ldr	r3, [r3]
8001259c:	e5932000 	ldr	r2, [r3]
800125a0:	e59f3020 	ldr	r3, [pc, #32]	; 800125c8 <kalloc+0x74>
800125a4:	e08f3003 	add	r3, pc, r3
800125a8:	e5832000 	str	r2, [r3]
800125ac:	e51b3010 	ldr	r3, [fp, #-16]
800125b0:	e1a00003 	mov	r0, r3
800125b4:	e24bd00c 	sub	sp, fp, #12
800125b8:	e89da800 	ldm	sp, {fp, sp, pc}
800125bc:	00014a88 	andeq	r4, r1, r8, lsl #21
800125c0:	00014a74 	andeq	r4, r1, r4, ror sl
800125c4:	00014a64 	andeq	r4, r1, r4, ror #20
800125c8:	00014a54 	andeq	r4, r1, r4, asr sl

800125cc <kfree>:
800125cc:	e1a0c00d 	mov	ip, sp
800125d0:	e92dd800 	push	{fp, ip, lr, pc}
800125d4:	e24cb004 	sub	fp, ip, #4
800125d8:	e24dd008 	sub	sp, sp, #8
800125dc:	e50b0010 	str	r0, [fp, #-16]
800125e0:	e59f3028 	ldr	r3, [pc, #40]	; 80012610 <kfree+0x44>
800125e4:	e08f3003 	add	r3, pc, r3
800125e8:	e5933000 	ldr	r3, [r3]
800125ec:	e1a00003 	mov	r0, r3
800125f0:	e51b1010 	ldr	r1, [fp, #-16]
800125f4:	eb00007b 	bl	800127e8 <pageListPrepend>
800125f8:	e1a02000 	mov	r2, r0
800125fc:	e59f3010 	ldr	r3, [pc, #16]	; 80012614 <kfree+0x48>
80012600:	e08f3003 	add	r3, pc, r3
80012604:	e5832000 	str	r2, [r3]
80012608:	e24bd00c 	sub	sp, fp, #12
8001260c:	e89da800 	ldm	sp, {fp, sp, pc}
80012610:	00014a14 	andeq	r4, r1, r4, lsl sl
80012614:	000149f8 	strdeq	r4, [r1], -r8

80012618 <kalloc1k>:
80012618:	e1a0c00d 	mov	ip, sp
8001261c:	e92dd800 	push	{fp, ip, lr, pc}
80012620:	e24cb004 	sub	fp, ip, #4
80012624:	e24dd008 	sub	sp, sp, #8
80012628:	e3a03000 	mov	r3, #0
8001262c:	e50b3010 	str	r3, [fp, #-16]
80012630:	e59f30a8 	ldr	r3, [pc, #168]	; 800126e0 <kalloc1k+0xc8>
80012634:	e08f3003 	add	r3, pc, r3
80012638:	e5933000 	ldr	r3, [r3]
8001263c:	e3530000 	cmp	r3, #0
80012640:	1a000012 	bne	80012690 <kalloc1k+0x78>
80012644:	ebffffc2 	bl	80012554 <kalloc>
80012648:	e50b0014 	str	r0, [fp, #-20]
8001264c:	e51b3014 	ldr	r3, [fp, #-20]
80012650:	e3530000 	cmp	r3, #0
80012654:	0a00000d 	beq	80012690 <kalloc1k+0x78>
80012658:	e51b0014 	ldr	r0, [fp, #-20]
8001265c:	eb000024 	bl	800126f4 <kfree1k>
80012660:	e51b3014 	ldr	r3, [fp, #-20]
80012664:	e2833b01 	add	r3, r3, #1024	; 0x400
80012668:	e1a00003 	mov	r0, r3
8001266c:	eb000020 	bl	800126f4 <kfree1k>
80012670:	e51b3014 	ldr	r3, [fp, #-20]
80012674:	e2833b02 	add	r3, r3, #2048	; 0x800
80012678:	e1a00003 	mov	r0, r3
8001267c:	eb00001c 	bl	800126f4 <kfree1k>
80012680:	e51b3014 	ldr	r3, [fp, #-20]
80012684:	e2833b03 	add	r3, r3, #3072	; 0xc00
80012688:	e1a00003 	mov	r0, r3
8001268c:	eb000018 	bl	800126f4 <kfree1k>
80012690:	e59f304c 	ldr	r3, [pc, #76]	; 800126e4 <kalloc1k+0xcc>
80012694:	e08f3003 	add	r3, pc, r3
80012698:	e5933000 	ldr	r3, [r3]
8001269c:	e3530000 	cmp	r3, #0
800126a0:	0a00000a 	beq	800126d0 <kalloc1k+0xb8>
800126a4:	e59f303c 	ldr	r3, [pc, #60]	; 800126e8 <kalloc1k+0xd0>
800126a8:	e08f3003 	add	r3, pc, r3
800126ac:	e5933000 	ldr	r3, [r3]
800126b0:	e50b3010 	str	r3, [fp, #-16]
800126b4:	e59f3030 	ldr	r3, [pc, #48]	; 800126ec <kalloc1k+0xd4>
800126b8:	e08f3003 	add	r3, pc, r3
800126bc:	e5933000 	ldr	r3, [r3]
800126c0:	e5932000 	ldr	r2, [r3]
800126c4:	e59f3024 	ldr	r3, [pc, #36]	; 800126f0 <kalloc1k+0xd8>
800126c8:	e08f3003 	add	r3, pc, r3
800126cc:	e5832000 	str	r2, [r3]
800126d0:	e51b3010 	ldr	r3, [fp, #-16]
800126d4:	e1a00003 	mov	r0, r3
800126d8:	e24bd00c 	sub	sp, fp, #12
800126dc:	e89da800 	ldm	sp, {fp, sp, pc}
800126e0:	000149c8 	andeq	r4, r1, r8, asr #19
800126e4:	00014968 	andeq	r4, r1, r8, ror #18
800126e8:	00014954 	andeq	r4, r1, r4, asr r9
800126ec:	00014944 	andeq	r4, r1, r4, asr #18
800126f0:	00014934 	andeq	r4, r1, r4, lsr r9

800126f4 <kfree1k>:
800126f4:	e1a0c00d 	mov	ip, sp
800126f8:	e92dd800 	push	{fp, ip, lr, pc}
800126fc:	e24cb004 	sub	fp, ip, #4
80012700:	e24dd008 	sub	sp, sp, #8
80012704:	e50b0010 	str	r0, [fp, #-16]
80012708:	e59f3028 	ldr	r3, [pc, #40]	; 80012738 <kfree1k+0x44>
8001270c:	e08f3003 	add	r3, pc, r3
80012710:	e5933000 	ldr	r3, [r3]
80012714:	e1a00003 	mov	r0, r3
80012718:	e51b1010 	ldr	r1, [fp, #-16]
8001271c:	eb000031 	bl	800127e8 <pageListPrepend>
80012720:	e1a02000 	mov	r2, r0
80012724:	e59f3010 	ldr	r3, [pc, #16]	; 8001273c <kfree1k+0x48>
80012728:	e08f3003 	add	r3, pc, r3
8001272c:	e5832000 	str	r2, [r3]
80012730:	e24bd00c 	sub	sp, fp, #12
80012734:	e89da800 	ldm	sp, {fp, sp, pc}
80012738:	000148f0 	strdeq	r4, [r1], -r0
8001273c:	000148d4 	ldrdeq	r4, [r1], -r4	; <UNPREDICTABLE>

80012740 <getFreeMemorySize>:
80012740:	e1a0c00d 	mov	ip, sp
80012744:	e92dd800 	push	{fp, ip, lr, pc}
80012748:	e24cb004 	sub	fp, ip, #4
8001274c:	e24dd008 	sub	sp, sp, #8
80012750:	e3a03000 	mov	r3, #0
80012754:	e50b3010 	str	r3, [fp, #-16]
80012758:	e3a03000 	mov	r3, #0
8001275c:	e50b3014 	str	r3, [fp, #-20]
80012760:	e59f3078 	ldr	r3, [pc, #120]	; 800127e0 <getFreeMemorySize+0xa0>
80012764:	e08f3003 	add	r3, pc, r3
80012768:	e5933000 	ldr	r3, [r3]
8001276c:	e50b3014 	str	r3, [fp, #-20]
80012770:	ea000005 	b	8001278c <getFreeMemorySize+0x4c>
80012774:	e51b3010 	ldr	r3, [fp, #-16]
80012778:	e2833b01 	add	r3, r3, #1024	; 0x400
8001277c:	e50b3010 	str	r3, [fp, #-16]
80012780:	e51b3014 	ldr	r3, [fp, #-20]
80012784:	e5933000 	ldr	r3, [r3]
80012788:	e50b3014 	str	r3, [fp, #-20]
8001278c:	e51b3014 	ldr	r3, [fp, #-20]
80012790:	e3530000 	cmp	r3, #0
80012794:	1afffff6 	bne	80012774 <getFreeMemorySize+0x34>
80012798:	e59f3044 	ldr	r3, [pc, #68]	; 800127e4 <getFreeMemorySize+0xa4>
8001279c:	e08f3003 	add	r3, pc, r3
800127a0:	e5933000 	ldr	r3, [r3]
800127a4:	e50b3014 	str	r3, [fp, #-20]
800127a8:	ea000005 	b	800127c4 <getFreeMemorySize+0x84>
800127ac:	e51b3010 	ldr	r3, [fp, #-16]
800127b0:	e2833a01 	add	r3, r3, #4096	; 0x1000
800127b4:	e50b3010 	str	r3, [fp, #-16]
800127b8:	e51b3014 	ldr	r3, [fp, #-20]
800127bc:	e5933000 	ldr	r3, [r3]
800127c0:	e50b3014 	str	r3, [fp, #-20]
800127c4:	e51b3014 	ldr	r3, [fp, #-20]
800127c8:	e3530000 	cmp	r3, #0
800127cc:	1afffff6 	bne	800127ac <getFreeMemorySize+0x6c>
800127d0:	e51b3010 	ldr	r3, [fp, #-16]
800127d4:	e1a00003 	mov	r0, r3
800127d8:	e24bd00c 	sub	sp, fp, #12
800127dc:	e89da800 	ldm	sp, {fp, sp, pc}
800127e0:	00014898 	muleq	r1, r8, r8
800127e4:	0001485c 	andeq	r4, r1, ip, asr r8

800127e8 <pageListPrepend>:
800127e8:	e1a0c00d 	mov	ip, sp
800127ec:	e92dd800 	push	{fp, ip, lr, pc}
800127f0:	e24cb004 	sub	fp, ip, #4
800127f4:	e24dd010 	sub	sp, sp, #16
800127f8:	e50b0018 	str	r0, [fp, #-24]
800127fc:	e50b101c 	str	r1, [fp, #-28]
80012800:	e51b301c 	ldr	r3, [fp, #-28]
80012804:	e50b3010 	str	r3, [fp, #-16]
80012808:	e51b3010 	ldr	r3, [fp, #-16]
8001280c:	e51b2018 	ldr	r2, [fp, #-24]
80012810:	e5832000 	str	r2, [r3]
80012814:	e51b3010 	ldr	r3, [fp, #-16]
80012818:	e1a00003 	mov	r0, r3
8001281c:	e24bd00c 	sub	sp, fp, #12
80012820:	e89da800 	ldm	sp, {fp, sp, pc}

80012824 <genBlock>:
80012824:	e1a0c00d 	mov	ip, sp
80012828:	e92dd800 	push	{fp, ip, lr, pc}
8001282c:	e24cb004 	sub	fp, ip, #4
80012830:	e24dd010 	sub	sp, sp, #16
80012834:	e50b0018 	str	r0, [fp, #-24]
80012838:	e50b101c 	str	r1, [fp, #-28]
8001283c:	e3a03010 	mov	r3, #16
80012840:	e50b3010 	str	r3, [fp, #-16]
80012844:	e51b3018 	ldr	r3, [fp, #-24]
80012848:	e50b3014 	str	r3, [fp, #-20]
8001284c:	e51b3014 	ldr	r3, [fp, #-20]
80012850:	e3a02000 	mov	r2, #0
80012854:	e5832004 	str	r2, [r3, #4]
80012858:	e51b3014 	ldr	r3, [fp, #-20]
8001285c:	e5932004 	ldr	r2, [r3, #4]
80012860:	e51b3014 	ldr	r3, [fp, #-20]
80012864:	e5832000 	str	r2, [r3]
80012868:	e51b2018 	ldr	r2, [fp, #-24]
8001286c:	e51b3010 	ldr	r3, [fp, #-16]
80012870:	e0822003 	add	r2, r2, r3
80012874:	e51b3014 	ldr	r3, [fp, #-20]
80012878:	e583200c 	str	r2, [r3, #12]
8001287c:	e51b201c 	ldr	r2, [fp, #-28]
80012880:	e51b3010 	ldr	r3, [fp, #-16]
80012884:	e0633002 	rsb	r3, r3, r2
80012888:	e3c31102 	bic	r1, r3, #-2147483648	; 0x80000000
8001288c:	e51b2014 	ldr	r2, [fp, #-20]
80012890:	e5923008 	ldr	r3, [r2, #8]
80012894:	e7de3011 	bfi	r3, r1, #0, #31
80012898:	e5823008 	str	r3, [r2, #8]
8001289c:	e51b3014 	ldr	r3, [fp, #-20]
800128a0:	e1a00003 	mov	r0, r3
800128a4:	e24bd00c 	sub	sp, fp, #12
800128a8:	e89da800 	ldm	sp, {fp, sp, pc}

800128ac <tryBreak>:
800128ac:	e1a0c00d 	mov	ip, sp
800128b0:	e92dd800 	push	{fp, ip, lr, pc}
800128b4:	e24cb004 	sub	fp, ip, #4
800128b8:	e24dd020 	sub	sp, sp, #32
800128bc:	e50b0020 	str	r0, [fp, #-32]
800128c0:	e50b1024 	str	r1, [fp, #-36]	; 0x24
800128c4:	e50b2028 	str	r2, [fp, #-40]	; 0x28
800128c8:	e3a03010 	mov	r3, #16
800128cc:	e50b3010 	str	r3, [fp, #-16]
800128d0:	e51b2010 	ldr	r2, [fp, #-16]
800128d4:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
800128d8:	e0822003 	add	r2, r2, r3
800128dc:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800128e0:	e5933008 	ldr	r3, [r3, #8]
800128e4:	e7fe3053 	ubfx	r3, r3, #0, #31
800128e8:	e1a01fa3 	lsr	r1, r3, #31
800128ec:	e0813003 	add	r3, r1, r3
800128f0:	e1a030c3 	asr	r3, r3, #1
800128f4:	e1520003 	cmp	r2, r3
800128f8:	9a000000 	bls	80012900 <tryBreak+0x54>
800128fc:	ea000036 	b	800129dc <tryBreak+0x130>
80012900:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012904:	e2833003 	add	r3, r3, #3
80012908:	e3c33003 	bic	r3, r3, #3
8001290c:	e50b3028 	str	r3, [fp, #-40]	; 0x28
80012910:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80012914:	e593200c 	ldr	r2, [r3, #12]
80012918:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
8001291c:	e0823003 	add	r3, r2, r3
80012920:	e50b3014 	str	r3, [fp, #-20]
80012924:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80012928:	e5933008 	ldr	r3, [r3, #8]
8001292c:	e7fe3053 	ubfx	r3, r3, #0, #31
80012930:	e1a02003 	mov	r2, r3
80012934:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012938:	e0633002 	rsb	r3, r3, r2
8001293c:	e51b0014 	ldr	r0, [fp, #-20]
80012940:	e1a01003 	mov	r1, r3
80012944:	ebffffb6 	bl	80012824 <genBlock>
80012948:	e50b0018 	str	r0, [fp, #-24]
8001294c:	e51b2018 	ldr	r2, [fp, #-24]
80012950:	e5d2300b 	ldrb	r3, [r2, #11]
80012954:	e7c7339f 	bfc	r3, #7, #1
80012958:	e5c2300b 	strb	r3, [r2, #11]
8001295c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012960:	e3c31102 	bic	r1, r3, #-2147483648	; 0x80000000
80012964:	e51b2024 	ldr	r2, [fp, #-36]	; 0x24
80012968:	e5923008 	ldr	r3, [r2, #8]
8001296c:	e7de3011 	bfi	r3, r1, #0, #31
80012970:	e5823008 	str	r3, [r2, #8]
80012974:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80012978:	e5932000 	ldr	r2, [r3]
8001297c:	e51b3018 	ldr	r3, [fp, #-24]
80012980:	e5832000 	str	r2, [r3]
80012984:	e51b3018 	ldr	r3, [fp, #-24]
80012988:	e5933000 	ldr	r3, [r3]
8001298c:	e3530000 	cmp	r3, #0
80012990:	0a000003 	beq	800129a4 <tryBreak+0xf8>
80012994:	e51b3018 	ldr	r3, [fp, #-24]
80012998:	e5933000 	ldr	r3, [r3]
8001299c:	e51b2018 	ldr	r2, [fp, #-24]
800129a0:	e5832004 	str	r2, [r3, #4]
800129a4:	e51b3018 	ldr	r3, [fp, #-24]
800129a8:	e51b2024 	ldr	r2, [fp, #-36]	; 0x24
800129ac:	e5832004 	str	r2, [r3, #4]
800129b0:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800129b4:	e51b2018 	ldr	r2, [fp, #-24]
800129b8:	e5832000 	str	r2, [r3]
800129bc:	e51b3020 	ldr	r3, [fp, #-32]
800129c0:	e5932014 	ldr	r2, [r3, #20]
800129c4:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800129c8:	e1520003 	cmp	r2, r3
800129cc:	1a000002 	bne	800129dc <tryBreak+0x130>
800129d0:	e51b3020 	ldr	r3, [fp, #-32]
800129d4:	e51b2018 	ldr	r2, [fp, #-24]
800129d8:	e5832014 	str	r2, [r3, #20]
800129dc:	e24bd00c 	sub	sp, fp, #12
800129e0:	e89da800 	ldm	sp, {fp, sp, pc}

800129e4 <trunkMalloc>:
800129e4:	e1a0c00d 	mov	ip, sp
800129e8:	e92dd800 	push	{fp, ip, lr, pc}
800129ec:	e24cb004 	sub	fp, ip, #4
800129f0:	e24dd020 	sub	sp, sp, #32
800129f4:	e50b0028 	str	r0, [fp, #-40]	; 0x28
800129f8:	e50b102c 	str	r1, [fp, #-44]	; 0x2c
800129fc:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012a00:	e5933010 	ldr	r3, [r3, #16]
80012a04:	e50b3010 	str	r3, [fp, #-16]
80012a08:	ea00001b 	b	80012a7c <trunkMalloc+0x98>
80012a0c:	e51b3010 	ldr	r3, [fp, #-16]
80012a10:	e5d3300b 	ldrb	r3, [r3, #11]
80012a14:	e3c3307f 	bic	r3, r3, #127	; 0x7f
80012a18:	e6ef3073 	uxtb	r3, r3
80012a1c:	e3530000 	cmp	r3, #0
80012a20:	1a000006 	bne	80012a40 <trunkMalloc+0x5c>
80012a24:	e51b3010 	ldr	r3, [fp, #-16]
80012a28:	e5933008 	ldr	r3, [r3, #8]
80012a2c:	e7fe3053 	ubfx	r3, r3, #0, #31
80012a30:	e1a02003 	mov	r2, r3
80012a34:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
80012a38:	e1520003 	cmp	r2, r3
80012a3c:	2a000003 	bcs	80012a50 <trunkMalloc+0x6c>
80012a40:	e51b3010 	ldr	r3, [fp, #-16]
80012a44:	e5933000 	ldr	r3, [r3]
80012a48:	e50b3010 	str	r3, [fp, #-16]
80012a4c:	ea00000a 	b	80012a7c <trunkMalloc+0x98>
80012a50:	e51b2010 	ldr	r2, [fp, #-16]
80012a54:	e5d2300b 	ldrb	r3, [r2, #11]
80012a58:	e3833080 	orr	r3, r3, #128	; 0x80
80012a5c:	e5c2300b 	strb	r3, [r2, #11]
80012a60:	e51b0028 	ldr	r0, [fp, #-40]	; 0x28
80012a64:	e51b1010 	ldr	r1, [fp, #-16]
80012a68:	e51b202c 	ldr	r2, [fp, #-44]	; 0x2c
80012a6c:	ebffff8e 	bl	800128ac <tryBreak>
80012a70:	e51b3010 	ldr	r3, [fp, #-16]
80012a74:	e593300c 	ldr	r3, [r3, #12]
80012a78:	ea000050 	b	80012bc0 <trunkMalloc+0x1dc>
80012a7c:	e51b3010 	ldr	r3, [fp, #-16]
80012a80:	e3530000 	cmp	r3, #0
80012a84:	1affffe0 	bne	80012a0c <trunkMalloc+0x28>
80012a88:	e3a03010 	mov	r3, #16
80012a8c:	e50b3018 	str	r3, [fp, #-24]
80012a90:	e51b202c 	ldr	r2, [fp, #-44]	; 0x2c
80012a94:	e51b3018 	ldr	r3, [fp, #-24]
80012a98:	e0823003 	add	r3, r2, r3
80012a9c:	e50b301c 	str	r3, [fp, #-28]
80012aa0:	e51b301c 	ldr	r3, [fp, #-28]
80012aa4:	e1a03623 	lsr	r3, r3, #12
80012aa8:	e50b3014 	str	r3, [fp, #-20]
80012aac:	e51b301c 	ldr	r3, [fp, #-28]
80012ab0:	e7eb3053 	ubfx	r3, r3, #0, #12
80012ab4:	e3530000 	cmp	r3, #0
80012ab8:	0a000002 	beq	80012ac8 <trunkMalloc+0xe4>
80012abc:	e51b3014 	ldr	r3, [fp, #-20]
80012ac0:	e2833001 	add	r3, r3, #1
80012ac4:	e50b3014 	str	r3, [fp, #-20]
80012ac8:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012acc:	e593300c 	ldr	r3, [r3, #12]
80012ad0:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80012ad4:	e5922000 	ldr	r2, [r2]
80012ad8:	e1a00002 	mov	r0, r2
80012adc:	e12fff33 	blx	r3
80012ae0:	e50b0020 	str	r0, [fp, #-32]
80012ae4:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012ae8:	e5933004 	ldr	r3, [r3, #4]
80012aec:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80012af0:	e5921000 	ldr	r1, [r2]
80012af4:	e51b2014 	ldr	r2, [fp, #-20]
80012af8:	e1a00001 	mov	r0, r1
80012afc:	e1a01002 	mov	r1, r2
80012b00:	e12fff33 	blx	r3
80012b04:	e1a03000 	mov	r3, r0
80012b08:	e3530000 	cmp	r3, #0
80012b0c:	1a000001 	bne	80012b18 <trunkMalloc+0x134>
80012b10:	e3a03000 	mov	r3, #0
80012b14:	ea000029 	b	80012bc0 <trunkMalloc+0x1dc>
80012b18:	e51b3014 	ldr	r3, [fp, #-20]
80012b1c:	e1a03603 	lsl	r3, r3, #12
80012b20:	e51b0020 	ldr	r0, [fp, #-32]
80012b24:	e1a01003 	mov	r1, r3
80012b28:	ebffff3d 	bl	80012824 <genBlock>
80012b2c:	e50b0010 	str	r0, [fp, #-16]
80012b30:	e51b2010 	ldr	r2, [fp, #-16]
80012b34:	e5d2300b 	ldrb	r3, [r2, #11]
80012b38:	e3833080 	orr	r3, r3, #128	; 0x80
80012b3c:	e5c2300b 	strb	r3, [r2, #11]
80012b40:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012b44:	e5933010 	ldr	r3, [r3, #16]
80012b48:	e3530000 	cmp	r3, #0
80012b4c:	1a000002 	bne	80012b5c <trunkMalloc+0x178>
80012b50:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012b54:	e51b2010 	ldr	r2, [fp, #-16]
80012b58:	e5832010 	str	r2, [r3, #16]
80012b5c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012b60:	e5933014 	ldr	r3, [r3, #20]
80012b64:	e3530000 	cmp	r3, #0
80012b68:	1a000003 	bne	80012b7c <trunkMalloc+0x198>
80012b6c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012b70:	e51b2010 	ldr	r2, [fp, #-16]
80012b74:	e5832014 	str	r2, [r3, #20]
80012b78:	ea00000a 	b	80012ba8 <trunkMalloc+0x1c4>
80012b7c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012b80:	e5933014 	ldr	r3, [r3, #20]
80012b84:	e51b2010 	ldr	r2, [fp, #-16]
80012b88:	e5832000 	str	r2, [r3]
80012b8c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012b90:	e5932014 	ldr	r2, [r3, #20]
80012b94:	e51b3010 	ldr	r3, [fp, #-16]
80012b98:	e5832004 	str	r2, [r3, #4]
80012b9c:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80012ba0:	e51b2010 	ldr	r2, [fp, #-16]
80012ba4:	e5832014 	str	r2, [r3, #20]
80012ba8:	e51b0028 	ldr	r0, [fp, #-40]	; 0x28
80012bac:	e51b1010 	ldr	r1, [fp, #-16]
80012bb0:	e51b202c 	ldr	r2, [fp, #-44]	; 0x2c
80012bb4:	ebffff3c 	bl	800128ac <tryBreak>
80012bb8:	e51b3010 	ldr	r3, [fp, #-16]
80012bbc:	e593300c 	ldr	r3, [r3, #12]
80012bc0:	e1a00003 	mov	r0, r3
80012bc4:	e24bd00c 	sub	sp, fp, #12
80012bc8:	e89da800 	ldm	sp, {fp, sp, pc}

80012bcc <tryMerge>:
80012bcc:	e1a0c00d 	mov	ip, sp
80012bd0:	e92dd800 	push	{fp, ip, lr, pc}
80012bd4:	e24cb004 	sub	fp, ip, #4
80012bd8:	e24dd010 	sub	sp, sp, #16
80012bdc:	e50b0018 	str	r0, [fp, #-24]
80012be0:	e50b101c 	str	r1, [fp, #-28]
80012be4:	e3a03010 	mov	r3, #16
80012be8:	e50b3010 	str	r3, [fp, #-16]
80012bec:	e51b301c 	ldr	r3, [fp, #-28]
80012bf0:	e5933000 	ldr	r3, [r3]
80012bf4:	e50b3014 	str	r3, [fp, #-20]
80012bf8:	e51b3014 	ldr	r3, [fp, #-20]
80012bfc:	e3530000 	cmp	r3, #0
80012c00:	0a000029 	beq	80012cac <tryMerge+0xe0>
80012c04:	e51b3014 	ldr	r3, [fp, #-20]
80012c08:	e5d3300b 	ldrb	r3, [r3, #11]
80012c0c:	e3c3307f 	bic	r3, r3, #127	; 0x7f
80012c10:	e6ef3073 	uxtb	r3, r3
80012c14:	e3530000 	cmp	r3, #0
80012c18:	1a000023 	bne	80012cac <tryMerge+0xe0>
80012c1c:	e51b301c 	ldr	r3, [fp, #-28]
80012c20:	e5933008 	ldr	r3, [r3, #8]
80012c24:	e7fe3053 	ubfx	r3, r3, #0, #31
80012c28:	e1a02003 	mov	r2, r3
80012c2c:	e51b3014 	ldr	r3, [fp, #-20]
80012c30:	e5933008 	ldr	r3, [r3, #8]
80012c34:	e7fe3053 	ubfx	r3, r3, #0, #31
80012c38:	e1a01003 	mov	r1, r3
80012c3c:	e51b3010 	ldr	r3, [fp, #-16]
80012c40:	e0813003 	add	r3, r1, r3
80012c44:	e0823003 	add	r3, r2, r3
80012c48:	e3c31102 	bic	r1, r3, #-2147483648	; 0x80000000
80012c4c:	e51b201c 	ldr	r2, [fp, #-28]
80012c50:	e5923008 	ldr	r3, [r2, #8]
80012c54:	e7de3011 	bfi	r3, r1, #0, #31
80012c58:	e5823008 	str	r3, [r2, #8]
80012c5c:	e51b3014 	ldr	r3, [fp, #-20]
80012c60:	e5932000 	ldr	r2, [r3]
80012c64:	e51b301c 	ldr	r3, [fp, #-28]
80012c68:	e5832000 	str	r2, [r3]
80012c6c:	e51b301c 	ldr	r3, [fp, #-28]
80012c70:	e5933000 	ldr	r3, [r3]
80012c74:	e3530000 	cmp	r3, #0
80012c78:	0a000003 	beq	80012c8c <tryMerge+0xc0>
80012c7c:	e51b301c 	ldr	r3, [fp, #-28]
80012c80:	e5933000 	ldr	r3, [r3]
80012c84:	e51b201c 	ldr	r2, [fp, #-28]
80012c88:	e5832004 	str	r2, [r3, #4]
80012c8c:	e51b3018 	ldr	r3, [fp, #-24]
80012c90:	e5932014 	ldr	r2, [r3, #20]
80012c94:	e51b3014 	ldr	r3, [fp, #-20]
80012c98:	e1520003 	cmp	r2, r3
80012c9c:	1a000002 	bne	80012cac <tryMerge+0xe0>
80012ca0:	e51b3018 	ldr	r3, [fp, #-24]
80012ca4:	e51b201c 	ldr	r2, [fp, #-28]
80012ca8:	e5832014 	str	r2, [r3, #20]
80012cac:	e51b301c 	ldr	r3, [fp, #-28]
80012cb0:	e5933004 	ldr	r3, [r3, #4]
80012cb4:	e50b3014 	str	r3, [fp, #-20]
80012cb8:	e51b3014 	ldr	r3, [fp, #-20]
80012cbc:	e3530000 	cmp	r3, #0
80012cc0:	0a000029 	beq	80012d6c <tryMerge+0x1a0>
80012cc4:	e51b3014 	ldr	r3, [fp, #-20]
80012cc8:	e5d3300b 	ldrb	r3, [r3, #11]
80012ccc:	e3c3307f 	bic	r3, r3, #127	; 0x7f
80012cd0:	e6ef3073 	uxtb	r3, r3
80012cd4:	e3530000 	cmp	r3, #0
80012cd8:	1a000023 	bne	80012d6c <tryMerge+0x1a0>
80012cdc:	e51b3014 	ldr	r3, [fp, #-20]
80012ce0:	e5933008 	ldr	r3, [r3, #8]
80012ce4:	e7fe3053 	ubfx	r3, r3, #0, #31
80012ce8:	e1a02003 	mov	r2, r3
80012cec:	e51b301c 	ldr	r3, [fp, #-28]
80012cf0:	e5933008 	ldr	r3, [r3, #8]
80012cf4:	e7fe3053 	ubfx	r3, r3, #0, #31
80012cf8:	e1a01003 	mov	r1, r3
80012cfc:	e51b3010 	ldr	r3, [fp, #-16]
80012d00:	e0813003 	add	r3, r1, r3
80012d04:	e0823003 	add	r3, r2, r3
80012d08:	e3c31102 	bic	r1, r3, #-2147483648	; 0x80000000
80012d0c:	e51b2014 	ldr	r2, [fp, #-20]
80012d10:	e5923008 	ldr	r3, [r2, #8]
80012d14:	e7de3011 	bfi	r3, r1, #0, #31
80012d18:	e5823008 	str	r3, [r2, #8]
80012d1c:	e51b301c 	ldr	r3, [fp, #-28]
80012d20:	e5932000 	ldr	r2, [r3]
80012d24:	e51b3014 	ldr	r3, [fp, #-20]
80012d28:	e5832000 	str	r2, [r3]
80012d2c:	e51b3014 	ldr	r3, [fp, #-20]
80012d30:	e5933000 	ldr	r3, [r3]
80012d34:	e3530000 	cmp	r3, #0
80012d38:	0a000003 	beq	80012d4c <tryMerge+0x180>
80012d3c:	e51b3014 	ldr	r3, [fp, #-20]
80012d40:	e5933000 	ldr	r3, [r3]
80012d44:	e51b2014 	ldr	r2, [fp, #-20]
80012d48:	e5832004 	str	r2, [r3, #4]
80012d4c:	e51b3018 	ldr	r3, [fp, #-24]
80012d50:	e5932014 	ldr	r2, [r3, #20]
80012d54:	e51b301c 	ldr	r3, [fp, #-28]
80012d58:	e1520003 	cmp	r2, r3
80012d5c:	1a000002 	bne	80012d6c <tryMerge+0x1a0>
80012d60:	e51b3018 	ldr	r3, [fp, #-24]
80012d64:	e51b2014 	ldr	r2, [fp, #-20]
80012d68:	e5832014 	str	r2, [r3, #20]
80012d6c:	e24bd00c 	sub	sp, fp, #12
80012d70:	e89da800 	ldm	sp, {fp, sp, pc}

80012d74 <tryShrink>:
80012d74:	e1a0c00d 	mov	ip, sp
80012d78:	e92dd800 	push	{fp, ip, lr, pc}
80012d7c:	e24cb004 	sub	fp, ip, #4
80012d80:	e24dd018 	sub	sp, sp, #24
80012d84:	e50b0020 	str	r0, [fp, #-32]
80012d88:	e3a03010 	mov	r3, #16
80012d8c:	e50b3010 	str	r3, [fp, #-16]
80012d90:	e51b3020 	ldr	r3, [fp, #-32]
80012d94:	e5933014 	ldr	r3, [r3, #20]
80012d98:	e50b3014 	str	r3, [fp, #-20]
80012d9c:	e51b3020 	ldr	r3, [fp, #-32]
80012da0:	e5933014 	ldr	r3, [r3, #20]
80012da4:	e5d3300b 	ldrb	r3, [r3, #11]
80012da8:	e3c3307f 	bic	r3, r3, #127	; 0x7f
80012dac:	e6ef3073 	uxtb	r3, r3
80012db0:	e3530000 	cmp	r3, #0
80012db4:	1a000003 	bne	80012dc8 <tryShrink+0x54>
80012db8:	e51b3014 	ldr	r3, [fp, #-20]
80012dbc:	e7eb3053 	ubfx	r3, r3, #0, #12
80012dc0:	e3530000 	cmp	r3, #0
80012dc4:	0a000000 	beq	80012dcc <tryShrink+0x58>
80012dc8:	ea00001f 	b	80012e4c <tryShrink+0xd8>
80012dcc:	e51b3020 	ldr	r3, [fp, #-32]
80012dd0:	e5933014 	ldr	r3, [r3, #20]
80012dd4:	e5933008 	ldr	r3, [r3, #8]
80012dd8:	e7fe3053 	ubfx	r3, r3, #0, #31
80012ddc:	e1a02003 	mov	r2, r3
80012de0:	e51b3010 	ldr	r3, [fp, #-16]
80012de4:	e0823003 	add	r3, r2, r3
80012de8:	e1a03623 	lsr	r3, r3, #12
80012dec:	e50b3018 	str	r3, [fp, #-24]
80012df0:	e51b3020 	ldr	r3, [fp, #-32]
80012df4:	e5933014 	ldr	r3, [r3, #20]
80012df8:	e5932004 	ldr	r2, [r3, #4]
80012dfc:	e51b3020 	ldr	r3, [fp, #-32]
80012e00:	e5832014 	str	r2, [r3, #20]
80012e04:	e51b3020 	ldr	r3, [fp, #-32]
80012e08:	e5933014 	ldr	r3, [r3, #20]
80012e0c:	e3a02000 	mov	r2, #0
80012e10:	e5832000 	str	r2, [r3]
80012e14:	e51b3020 	ldr	r3, [fp, #-32]
80012e18:	e5933014 	ldr	r3, [r3, #20]
80012e1c:	e3530000 	cmp	r3, #0
80012e20:	1a000002 	bne	80012e30 <tryShrink+0xbc>
80012e24:	e51b3020 	ldr	r3, [fp, #-32]
80012e28:	e3a02000 	mov	r2, #0
80012e2c:	e5832010 	str	r2, [r3, #16]
80012e30:	e51b3020 	ldr	r3, [fp, #-32]
80012e34:	e5933008 	ldr	r3, [r3, #8]
80012e38:	e51b2020 	ldr	r2, [fp, #-32]
80012e3c:	e5922000 	ldr	r2, [r2]
80012e40:	e1a00002 	mov	r0, r2
80012e44:	e51b1018 	ldr	r1, [fp, #-24]
80012e48:	e12fff33 	blx	r3
80012e4c:	e24bd00c 	sub	sp, fp, #12
80012e50:	e89da800 	ldm	sp, {fp, sp, pc}

80012e54 <trunkFree>:
80012e54:	e1a0c00d 	mov	ip, sp
80012e58:	e92dd800 	push	{fp, ip, lr, pc}
80012e5c:	e24cb004 	sub	fp, ip, #4
80012e60:	e24dd010 	sub	sp, sp, #16
80012e64:	e50b0018 	str	r0, [fp, #-24]
80012e68:	e50b101c 	str	r1, [fp, #-28]
80012e6c:	e3a03010 	mov	r3, #16
80012e70:	e50b3010 	str	r3, [fp, #-16]
80012e74:	e51b201c 	ldr	r2, [fp, #-28]
80012e78:	e51b3010 	ldr	r3, [fp, #-16]
80012e7c:	e1520003 	cmp	r2, r3
80012e80:	2a000000 	bcs	80012e88 <trunkFree+0x34>
80012e84:	ea00000d 	b	80012ec0 <trunkFree+0x6c>
80012e88:	e51b3010 	ldr	r3, [fp, #-16]
80012e8c:	e2633000 	rsb	r3, r3, #0
80012e90:	e51b201c 	ldr	r2, [fp, #-28]
80012e94:	e0823003 	add	r3, r2, r3
80012e98:	e50b3014 	str	r3, [fp, #-20]
80012e9c:	e51b2014 	ldr	r2, [fp, #-20]
80012ea0:	e5d2300b 	ldrb	r3, [r2, #11]
80012ea4:	e7c7339f 	bfc	r3, #7, #1
80012ea8:	e5c2300b 	strb	r3, [r2, #11]
80012eac:	e51b0018 	ldr	r0, [fp, #-24]
80012eb0:	e51b1014 	ldr	r1, [fp, #-20]
80012eb4:	ebffff44 	bl	80012bcc <tryMerge>
80012eb8:	e51b0018 	ldr	r0, [fp, #-24]
80012ebc:	ebffffac 	bl	80012d74 <tryShrink>
80012ec0:	e24bd00c 	sub	sp, fp, #12
80012ec4:	e89da800 	ldm	sp, {fp, sp, pc}

80012ec8 <kmShrink>:
80012ec8:	e1a0c00d 	mov	ip, sp
80012ecc:	e92dd800 	push	{fp, ip, lr, pc}
80012ed0:	e24cb004 	sub	fp, ip, #4
80012ed4:	e24dd008 	sub	sp, sp, #8
80012ed8:	e50b0010 	str	r0, [fp, #-16]
80012edc:	e50b1014 	str	r1, [fp, #-20]
80012ee0:	e51b3014 	ldr	r3, [fp, #-20]
80012ee4:	e1a03603 	lsl	r3, r3, #12
80012ee8:	e2633000 	rsb	r3, r3, #0
80012eec:	e1a02003 	mov	r2, r3
80012ef0:	e59f301c 	ldr	r3, [pc, #28]	; 80012f14 <kmShrink+0x4c>
80012ef4:	e08f3003 	add	r3, pc, r3
80012ef8:	e5933000 	ldr	r3, [r3]
80012efc:	e0822003 	add	r2, r2, r3
80012f00:	e59f3010 	ldr	r3, [pc, #16]	; 80012f18 <kmShrink+0x50>
80012f04:	e08f3003 	add	r3, pc, r3
80012f08:	e5832000 	str	r2, [r3]
80012f0c:	e24bd00c 	sub	sp, fp, #12
80012f10:	e89da800 	ldm	sp, {fp, sp, pc}
80012f14:	0000d1b0 			; <UNDEFINED> instruction: 0x0000d1b0
80012f18:	0000d1a0 	andeq	sp, r0, r0, lsr #3

80012f1c <kmExpand>:
80012f1c:	e1a0c00d 	mov	ip, sp
80012f20:	e92dd800 	push	{fp, ip, lr, pc}
80012f24:	e24cb004 	sub	fp, ip, #4
80012f28:	e24dd010 	sub	sp, sp, #16
80012f2c:	e50b0018 	str	r0, [fp, #-24]
80012f30:	e50b101c 	str	r1, [fp, #-28]
80012f34:	e59f2060 	ldr	r2, [pc, #96]	; 80012f9c <kmExpand+0x80>
80012f38:	e08f2002 	add	r2, pc, r2
80012f3c:	e51b301c 	ldr	r3, [fp, #-28]
80012f40:	e1a03603 	lsl	r3, r3, #12
80012f44:	e1a01003 	mov	r1, r3
80012f48:	e59f3050 	ldr	r3, [pc, #80]	; 80012fa0 <kmExpand+0x84>
80012f4c:	e08f3003 	add	r3, pc, r3
80012f50:	e5933000 	ldr	r3, [r3]
80012f54:	e0813003 	add	r3, r1, r3
80012f58:	e50b3010 	str	r3, [fp, #-16]
80012f5c:	e59f3040 	ldr	r3, [pc, #64]	; 80012fa4 <kmExpand+0x88>
80012f60:	e7923003 	ldr	r3, [r2, r3]
80012f64:	e2833982 	add	r3, r3, #2129920	; 0x208000
80012f68:	e51b2010 	ldr	r2, [fp, #-16]
80012f6c:	e1520003 	cmp	r2, r3
80012f70:	9a000001 	bls	80012f7c <kmExpand+0x60>
80012f74:	e3a03000 	mov	r3, #0
80012f78:	ea000004 	b	80012f90 <kmExpand+0x74>
80012f7c:	e59f3024 	ldr	r3, [pc, #36]	; 80012fa8 <kmExpand+0x8c>
80012f80:	e08f3003 	add	r3, pc, r3
80012f84:	e51b2010 	ldr	r2, [fp, #-16]
80012f88:	e5832000 	str	r2, [r3]
80012f8c:	e3a03001 	mov	r3, #1
80012f90:	e1a00003 	mov	r0, r3
80012f94:	e24bd00c 	sub	sp, fp, #12
80012f98:	e89da800 	ldm	sp, {fp, sp, pc}
80012f9c:	0000d0c4 	andeq	sp, r0, r4, asr #1
80012fa0:	0000d158 	andeq	sp, r0, r8, asr r1
80012fa4:	00000018 	andeq	r0, r0, r8, lsl r0
80012fa8:	0000d124 	andeq	sp, r0, r4, lsr #2

80012fac <kmGetMemTail>:
80012fac:	e1a0c00d 	mov	ip, sp
80012fb0:	e92dd800 	push	{fp, ip, lr, pc}
80012fb4:	e24cb004 	sub	fp, ip, #4
80012fb8:	e24dd008 	sub	sp, sp, #8
80012fbc:	e50b0010 	str	r0, [fp, #-16]
80012fc0:	e59f3010 	ldr	r3, [pc, #16]	; 80012fd8 <kmGetMemTail+0x2c>
80012fc4:	e08f3003 	add	r3, pc, r3
80012fc8:	e5933000 	ldr	r3, [r3]
80012fcc:	e1a00003 	mov	r0, r3
80012fd0:	e24bd00c 	sub	sp, fp, #12
80012fd4:	e89da800 	ldm	sp, {fp, sp, pc}
80012fd8:	0000d0e0 	andeq	sp, r0, r0, ror #1

80012fdc <kmInit>:
80012fdc:	e1a0c00d 	mov	ip, sp
80012fe0:	e92dd800 	push	{fp, ip, lr, pc}
80012fe4:	e24cb004 	sub	fp, ip, #4
80012fe8:	e59f3038 	ldr	r3, [pc, #56]	; 80013028 <kmInit+0x4c>
80012fec:	e08f3003 	add	r3, pc, r3
80012ff0:	e59f2034 	ldr	r2, [pc, #52]	; 8001302c <kmInit+0x50>
80012ff4:	e08f2002 	add	r2, pc, r2
80012ff8:	e5832004 	str	r2, [r3, #4]
80012ffc:	e59f302c 	ldr	r3, [pc, #44]	; 80013030 <kmInit+0x54>
80013000:	e08f3003 	add	r3, pc, r3
80013004:	e59f2028 	ldr	r2, [pc, #40]	; 80013034 <kmInit+0x58>
80013008:	e08f2002 	add	r2, pc, r2
8001300c:	e5832008 	str	r2, [r3, #8]
80013010:	e59f3020 	ldr	r3, [pc, #32]	; 80013038 <kmInit+0x5c>
80013014:	e08f3003 	add	r3, pc, r3
80013018:	e59f201c 	ldr	r2, [pc, #28]	; 8001303c <kmInit+0x60>
8001301c:	e08f2002 	add	r2, pc, r2
80013020:	e583200c 	str	r2, [r3, #12]
80013024:	e89da800 	ldm	sp, {fp, sp, pc}
80013028:	00014014 	andeq	r4, r1, r4, lsl r0
8001302c:	ffffff20 			; <UNDEFINED> instruction: 0xffffff20
80013030:	00014000 	andeq	r4, r1, r0
80013034:	fffffeb8 			; <UNDEFINED> instruction: 0xfffffeb8
80013038:	00013fec 	andeq	r3, r1, ip, ror #31
8001303c:	ffffff88 			; <UNDEFINED> instruction: 0xffffff88

80013040 <kmalloc>:
80013040:	e1a0c00d 	mov	ip, sp
80013044:	e92dd800 	push	{fp, ip, lr, pc}
80013048:	e24cb004 	sub	fp, ip, #4
8001304c:	e24dd008 	sub	sp, sp, #8
80013050:	e50b0010 	str	r0, [fp, #-16]
80013054:	e59f301c 	ldr	r3, [pc, #28]	; 80013078 <kmalloc+0x38>
80013058:	e08f3003 	add	r3, pc, r3
8001305c:	e1a00003 	mov	r0, r3
80013060:	e51b1010 	ldr	r1, [fp, #-16]
80013064:	ebfffe5e 	bl	800129e4 <trunkMalloc>
80013068:	e1a03000 	mov	r3, r0
8001306c:	e1a00003 	mov	r0, r3
80013070:	e24bd00c 	sub	sp, fp, #12
80013074:	e89da800 	ldm	sp, {fp, sp, pc}
80013078:	00013fa8 	andeq	r3, r1, r8, lsr #31

8001307c <kmfree>:
8001307c:	e1a0c00d 	mov	ip, sp
80013080:	e92dd800 	push	{fp, ip, lr, pc}
80013084:	e24cb004 	sub	fp, ip, #4
80013088:	e24dd008 	sub	sp, sp, #8
8001308c:	e50b0010 	str	r0, [fp, #-16]
80013090:	e59f3014 	ldr	r3, [pc, #20]	; 800130ac <kmfree+0x30>
80013094:	e08f3003 	add	r3, pc, r3
80013098:	e1a00003 	mov	r0, r3
8001309c:	e51b1010 	ldr	r1, [fp, #-16]
800130a0:	ebffff6b 	bl	80012e54 <trunkFree>
800130a4:	e24bd00c 	sub	sp, fp, #12
800130a8:	e89da800 	ldm	sp, {fp, sp, pc}
800130ac:	00013f6c 	andeq	r3, r1, ip, ror #30

800130b0 <procInit>:
800130b0:	e1a0c00d 	mov	ip, sp
800130b4:	e92dd800 	push	{fp, ip, lr, pc}
800130b8:	e24cb004 	sub	fp, ip, #4
800130bc:	e24dd008 	sub	sp, sp, #8
800130c0:	e59f305c 	ldr	r3, [pc, #92]	; 80013124 <procInit+0x74>
800130c4:	e08f3003 	add	r3, pc, r3
800130c8:	e3a02000 	mov	r2, #0
800130cc:	e50b2010 	str	r2, [fp, #-16]
800130d0:	ea00000a 	b	80013100 <procInit+0x50>
800130d4:	e59f204c 	ldr	r2, [pc, #76]	; 80013128 <procInit+0x78>
800130d8:	e7931002 	ldr	r1, [r3, r2]
800130dc:	e51b2010 	ldr	r2, [fp, #-16]
800130e0:	e3a00e39 	mov	r0, #912	; 0x390
800130e4:	e0020290 	mul	r2, r0, r2
800130e8:	e0812002 	add	r2, r1, r2
800130ec:	e3a01000 	mov	r1, #0
800130f0:	e5c21000 	strb	r1, [r2]
800130f4:	e51b2010 	ldr	r2, [fp, #-16]
800130f8:	e2822001 	add	r2, r2, #1
800130fc:	e50b2010 	str	r2, [fp, #-16]
80013100:	e51b2010 	ldr	r2, [fp, #-16]
80013104:	e352007f 	cmp	r2, #127	; 0x7f
80013108:	dafffff1 	ble	800130d4 <procInit+0x24>
8001310c:	e59f2018 	ldr	r2, [pc, #24]	; 8001312c <procInit+0x7c>
80013110:	e7933002 	ldr	r3, [r3, r2]
80013114:	e3a02000 	mov	r2, #0
80013118:	e5832000 	str	r2, [r3]
8001311c:	e24bd00c 	sub	sp, fp, #12
80013120:	e89da800 	ldm	sp, {fp, sp, pc}
80013124:	0000cf38 	andeq	ip, r0, r8, lsr pc
80013128:	00000024 	andeq	r0, r0, r4, lsr #32
8001312c:	00000014 	andeq	r0, r0, r4, lsl r0

80013130 <procExpandMemory>:
80013130:	e1a0c00d 	mov	ip, sp
80013134:	e92dd800 	push	{fp, ip, lr, pc}
80013138:	e24cb004 	sub	fp, ip, #4
8001313c:	e24dd018 	sub	sp, sp, #24
80013140:	e50b0020 	str	r0, [fp, #-32]
80013144:	e50b1024 	str	r1, [fp, #-36]	; 0x24
80013148:	e51b3020 	ldr	r3, [fp, #-32]
8001314c:	e50b3014 	str	r3, [fp, #-20]
80013150:	e3a03000 	mov	r3, #0
80013154:	e50b3010 	str	r3, [fp, #-16]
80013158:	ea000020 	b	800131e0 <procExpandMemory+0xb0>
8001315c:	ebfffcfc 	bl	80012554 <kalloc>
80013160:	e50b0018 	str	r0, [fp, #-24]
80013164:	e51b3018 	ldr	r3, [fp, #-24]
80013168:	e3530000 	cmp	r3, #0
8001316c:	1a000004 	bne	80013184 <procExpandMemory+0x54>
80013170:	e51b0014 	ldr	r0, [fp, #-20]
80013174:	e51b1010 	ldr	r1, [fp, #-16]
80013178:	eb000020 	bl	80013200 <procShrinkMemory>
8001317c:	e3a03000 	mov	r3, #0
80013180:	ea00001b 	b	800131f4 <procExpandMemory+0xc4>
80013184:	e51b0018 	ldr	r0, [fp, #-24]
80013188:	e3a01000 	mov	r1, #0
8001318c:	e3a02a01 	mov	r2, #4096	; 0x1000
80013190:	eb0007c8 	bl	800150b8 <memset>
80013194:	e51b3014 	ldr	r3, [fp, #-20]
80013198:	e5931194 	ldr	r1, [r3, #404]	; 0x194
8001319c:	e51b3014 	ldr	r3, [fp, #-20]
800131a0:	e5932198 	ldr	r2, [r3, #408]	; 0x198
800131a4:	e51b3018 	ldr	r3, [fp, #-24]
800131a8:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
800131ac:	e1a00001 	mov	r0, r1
800131b0:	e1a01002 	mov	r1, r2
800131b4:	e1a02003 	mov	r2, r3
800131b8:	e3a030ff 	mov	r3, #255	; 0xff
800131bc:	ebfffbd7 	bl	80012120 <mapPage>
800131c0:	e51b3014 	ldr	r3, [fp, #-20]
800131c4:	e5933198 	ldr	r3, [r3, #408]	; 0x198
800131c8:	e2832a01 	add	r2, r3, #4096	; 0x1000
800131cc:	e51b3014 	ldr	r3, [fp, #-20]
800131d0:	e5832198 	str	r2, [r3, #408]	; 0x198
800131d4:	e51b3010 	ldr	r3, [fp, #-16]
800131d8:	e2833001 	add	r3, r3, #1
800131dc:	e50b3010 	str	r3, [fp, #-16]
800131e0:	e51b2010 	ldr	r2, [fp, #-16]
800131e4:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800131e8:	e1520003 	cmp	r2, r3
800131ec:	baffffda 	blt	8001315c <procExpandMemory+0x2c>
800131f0:	e3a03001 	mov	r3, #1
800131f4:	e1a00003 	mov	r0, r3
800131f8:	e24bd00c 	sub	sp, fp, #12
800131fc:	e89da800 	ldm	sp, {fp, sp, pc}

80013200 <procShrinkMemory>:
80013200:	e1a0c00d 	mov	ip, sp
80013204:	e92dd800 	push	{fp, ip, lr, pc}
80013208:	e24cb004 	sub	fp, ip, #4
8001320c:	e24dd020 	sub	sp, sp, #32
80013210:	e50b0028 	str	r0, [fp, #-40]	; 0x28
80013214:	e50b102c 	str	r1, [fp, #-44]	; 0x2c
80013218:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
8001321c:	e50b3014 	str	r3, [fp, #-20]
80013220:	e3a03000 	mov	r3, #0
80013224:	e50b3010 	str	r3, [fp, #-16]
80013228:	ea000021 	b	800132b4 <procShrinkMemory+0xb4>
8001322c:	e51b3014 	ldr	r3, [fp, #-20]
80013230:	e5933198 	ldr	r3, [r3, #408]	; 0x198
80013234:	e2433a01 	sub	r3, r3, #4096	; 0x1000
80013238:	e50b3018 	str	r3, [fp, #-24]
8001323c:	e51b3014 	ldr	r3, [fp, #-20]
80013240:	e5933194 	ldr	r3, [r3, #404]	; 0x194
80013244:	e1a00003 	mov	r0, r3
80013248:	e51b1018 	ldr	r1, [fp, #-24]
8001324c:	ebfffc43 	bl	80012360 <resolvePhyAddress>
80013250:	e50b001c 	str	r0, [fp, #-28]
80013254:	e51b301c 	ldr	r3, [fp, #-28]
80013258:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
8001325c:	e50b3020 	str	r3, [fp, #-32]
80013260:	e51b3020 	ldr	r3, [fp, #-32]
80013264:	e1a00003 	mov	r0, r3
80013268:	ebfffcd7 	bl	800125cc <kfree>
8001326c:	e51b3014 	ldr	r3, [fp, #-20]
80013270:	e5933194 	ldr	r3, [r3, #404]	; 0x194
80013274:	e1a00003 	mov	r0, r3
80013278:	e51b1018 	ldr	r1, [fp, #-24]
8001327c:	ebfffc16 	bl	800122dc <unmapPage>
80013280:	e51b3014 	ldr	r3, [fp, #-20]
80013284:	e5933198 	ldr	r3, [r3, #408]	; 0x198
80013288:	e2432a01 	sub	r2, r3, #4096	; 0x1000
8001328c:	e51b3014 	ldr	r3, [fp, #-20]
80013290:	e5832198 	str	r2, [r3, #408]	; 0x198
80013294:	e51b3014 	ldr	r3, [fp, #-20]
80013298:	e5933198 	ldr	r3, [r3, #408]	; 0x198
8001329c:	e3530000 	cmp	r3, #0
800132a0:	1a000000 	bne	800132a8 <procShrinkMemory+0xa8>
800132a4:	ea000006 	b	800132c4 <procShrinkMemory+0xc4>
800132a8:	e51b3010 	ldr	r3, [fp, #-16]
800132ac:	e2833001 	add	r3, r3, #1
800132b0:	e50b3010 	str	r3, [fp, #-16]
800132b4:	e51b2010 	ldr	r2, [fp, #-16]
800132b8:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
800132bc:	e1520003 	cmp	r2, r3
800132c0:	baffffd9 	blt	8001322c <procShrinkMemory+0x2c>
800132c4:	e24bd00c 	sub	sp, fp, #12
800132c8:	e89da800 	ldm	sp, {fp, sp, pc}

800132cc <procGetMemTail>:
800132cc:	e1a0c00d 	mov	ip, sp
800132d0:	e92dd800 	push	{fp, ip, lr, pc}
800132d4:	e24cb004 	sub	fp, ip, #4
800132d8:	e24dd010 	sub	sp, sp, #16
800132dc:	e50b0018 	str	r0, [fp, #-24]
800132e0:	e51b3018 	ldr	r3, [fp, #-24]
800132e4:	e50b3010 	str	r3, [fp, #-16]
800132e8:	e51b3010 	ldr	r3, [fp, #-16]
800132ec:	e5933198 	ldr	r3, [r3, #408]	; 0x198
800132f0:	e1a00003 	mov	r0, r3
800132f4:	e24bd00c 	sub	sp, fp, #12
800132f8:	e89da800 	ldm	sp, {fp, sp, pc}

800132fc <procCreate>:
800132fc:	e1a0c00d 	mov	ip, sp
80013300:	e92dd810 	push	{r4, fp, ip, lr, pc}
80013304:	e24cb004 	sub	fp, ip, #4
80013308:	e24dd014 	sub	sp, sp, #20
8001330c:	e59f4314 	ldr	r4, [pc, #788]	; 80013628 <procCreate+0x32c>
80013310:	e08f4004 	add	r4, pc, r4
80013314:	e3a03000 	mov	r3, #0
80013318:	e50b301c 	str	r3, [fp, #-28]
8001331c:	e3a03000 	mov	r3, #0
80013320:	e50b3020 	str	r3, [fp, #-32]
80013324:	e3a03000 	mov	r3, #0
80013328:	e50b3024 	str	r3, [fp, #-36]	; 0x24
8001332c:	e3a03000 	mov	r3, #0
80013330:	e50b3018 	str	r3, [fp, #-24]
80013334:	ea000010 	b	8001337c <procCreate+0x80>
80013338:	e59f32ec 	ldr	r3, [pc, #748]	; 8001362c <procCreate+0x330>
8001333c:	e7942003 	ldr	r2, [r4, r3]
80013340:	e51b3018 	ldr	r3, [fp, #-24]
80013344:	e3a01e39 	mov	r1, #912	; 0x390
80013348:	e0030391 	mul	r3, r1, r3
8001334c:	e0823003 	add	r3, r2, r3
80013350:	e5d33000 	ldrb	r3, [r3]
80013354:	e3530000 	cmp	r3, #0
80013358:	1a000004 	bne	80013370 <procCreate+0x74>
8001335c:	e59f32cc 	ldr	r3, [pc, #716]	; 80013630 <procCreate+0x334>
80013360:	e08f3003 	add	r3, pc, r3
80013364:	e51b2018 	ldr	r2, [fp, #-24]
80013368:	e5832000 	str	r2, [r3]
8001336c:	ea000005 	b	80013388 <procCreate+0x8c>
80013370:	e51b3018 	ldr	r3, [fp, #-24]
80013374:	e2833001 	add	r3, r3, #1
80013378:	e50b3018 	str	r3, [fp, #-24]
8001337c:	e51b3018 	ldr	r3, [fp, #-24]
80013380:	e353007f 	cmp	r3, #127	; 0x7f
80013384:	daffffeb 	ble	80013338 <procCreate+0x3c>
80013388:	e59f32a4 	ldr	r3, [pc, #676]	; 80013634 <procCreate+0x338>
8001338c:	e08f3003 	add	r3, pc, r3
80013390:	e5933000 	ldr	r3, [r3]
80013394:	e3530000 	cmp	r3, #0
80013398:	aa000001 	bge	800133a4 <procCreate+0xa8>
8001339c:	e3a03000 	mov	r3, #0
800133a0:	ea00009d 	b	8001361c <procCreate+0x320>
800133a4:	e59f328c 	ldr	r3, [pc, #652]	; 80013638 <procCreate+0x33c>
800133a8:	e08f3003 	add	r3, pc, r3
800133ac:	e5933000 	ldr	r3, [r3]
800133b0:	e3a02e39 	mov	r2, #912	; 0x390
800133b4:	e0030392 	mul	r3, r2, r3
800133b8:	e59f226c 	ldr	r2, [pc, #620]	; 8001362c <procCreate+0x330>
800133bc:	e7942002 	ldr	r2, [r4, r2]
800133c0:	e0832002 	add	r2, r3, r2
800133c4:	e59f3270 	ldr	r3, [pc, #624]	; 8001363c <procCreate+0x340>
800133c8:	e08f3003 	add	r3, pc, r3
800133cc:	e5832000 	str	r2, [r3]
800133d0:	ebfffc5f 	bl	80012554 <kalloc>
800133d4:	e50b0020 	str	r0, [fp, #-32]
800133d8:	ebfffc5d 	bl	80012554 <kalloc>
800133dc:	e50b0024 	str	r0, [fp, #-36]	; 0x24
800133e0:	e59f3258 	ldr	r3, [pc, #600]	; 80013640 <procCreate+0x344>
800133e4:	e08f3003 	add	r3, pc, r3
800133e8:	e5933000 	ldr	r3, [r3]
800133ec:	e1a03703 	lsl	r3, r3, #14
800133f0:	e59f224c 	ldr	r2, [pc, #588]	; 80013644 <procCreate+0x348>
800133f4:	e08f2002 	add	r2, pc, r2
800133f8:	e0833002 	add	r3, r3, r2
800133fc:	e50b301c 	str	r3, [fp, #-28]
80013400:	e51b001c 	ldr	r0, [fp, #-28]
80013404:	ebfff5a8 	bl	80010aac <setKernelVM>
80013408:	e51b3020 	ldr	r3, [fp, #-32]
8001340c:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80013410:	e51b001c 	ldr	r0, [fp, #-28]
80013414:	e3a01a0e 	mov	r1, #57344	; 0xe000
80013418:	e3471fff 	movt	r1, #32767	; 0x7fff
8001341c:	e1a02003 	mov	r2, r3
80013420:	e3a030aa 	mov	r3, #170	; 0xaa
80013424:	ebfffb3d 	bl	80012120 <mapPage>
80013428:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8001342c:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80013430:	e51b001c 	ldr	r0, [fp, #-28]
80013434:	e3a01a0d 	mov	r1, #53248	; 0xd000
80013438:	e3471fff 	movt	r1, #32767	; 0x7fff
8001343c:	e1a02003 	mov	r2, r3
80013440:	e3a030ff 	mov	r3, #255	; 0xff
80013444:	ebfffb35 	bl	80012120 <mapPage>
80013448:	e59f31f8 	ldr	r3, [pc, #504]	; 80013648 <procCreate+0x34c>
8001344c:	e08f3003 	add	r3, pc, r3
80013450:	e5933000 	ldr	r3, [r3]
80013454:	e59f21f0 	ldr	r2, [pc, #496]	; 8001364c <procCreate+0x350>
80013458:	e08f2002 	add	r2, pc, r2
8001345c:	e5922000 	ldr	r2, [r2]
80013460:	e5832004 	str	r2, [r3, #4]
80013464:	e59f31e4 	ldr	r3, [pc, #484]	; 80013650 <procCreate+0x354>
80013468:	e08f3003 	add	r3, pc, r3
8001346c:	e5933000 	ldr	r3, [r3]
80013470:	e3a02000 	mov	r2, #0
80013474:	e5832008 	str	r2, [r3, #8]
80013478:	e59f31d4 	ldr	r3, [pc, #468]	; 80013654 <procCreate+0x358>
8001347c:	e08f3003 	add	r3, pc, r3
80013480:	e5933000 	ldr	r3, [r3]
80013484:	e3e02000 	mvn	r2, #0
80013488:	e583200c 	str	r2, [r3, #12]
8001348c:	e59f31c4 	ldr	r3, [pc, #452]	; 80013658 <procCreate+0x35c>
80013490:	e08f3003 	add	r3, pc, r3
80013494:	e5933000 	ldr	r3, [r3]
80013498:	e3a02000 	mov	r2, #0
8001349c:	e5c32010 	strb	r2, [r3, #16]
800134a0:	e59f31b4 	ldr	r3, [pc, #436]	; 8001365c <procCreate+0x360>
800134a4:	e08f3003 	add	r3, pc, r3
800134a8:	e5933000 	ldr	r3, [r3]
800134ac:	e2833090 	add	r3, r3, #144	; 0x90
800134b0:	e1a00003 	mov	r0, r3
800134b4:	e59f31a4 	ldr	r3, [pc, #420]	; 80013660 <procCreate+0x364>
800134b8:	e08f3003 	add	r3, pc, r3
800134bc:	e1a01003 	mov	r1, r3
800134c0:	eb0005cd 	bl	80014bfc <strcpy>
800134c4:	e59f3198 	ldr	r3, [pc, #408]	; 80013664 <procCreate+0x368>
800134c8:	e08f3003 	add	r3, pc, r3
800134cc:	e5933000 	ldr	r3, [r3]
800134d0:	e3a02001 	mov	r2, #1
800134d4:	e5c32000 	strb	r2, [r3]
800134d8:	e59f3188 	ldr	r3, [pc, #392]	; 80013668 <procCreate+0x36c>
800134dc:	e08f3003 	add	r3, pc, r3
800134e0:	e5933000 	ldr	r3, [r3]
800134e4:	e51b201c 	ldr	r2, [fp, #-28]
800134e8:	e5832194 	str	r2, [r3, #404]	; 0x194
800134ec:	e59f3178 	ldr	r3, [pc, #376]	; 8001366c <procCreate+0x370>
800134f0:	e08f3003 	add	r3, pc, r3
800134f4:	e5933000 	ldr	r3, [r3]
800134f8:	e3a02000 	mov	r2, #0
800134fc:	e5832198 	str	r2, [r3, #408]	; 0x198
80013500:	e59f3168 	ldr	r3, [pc, #360]	; 80013670 <procCreate+0x374>
80013504:	e08f3003 	add	r3, pc, r3
80013508:	e5933000 	ldr	r3, [r3]
8001350c:	e51b2020 	ldr	r2, [fp, #-32]
80013510:	e58321a0 	str	r2, [r3, #416]	; 0x1a0
80013514:	e59f3158 	ldr	r3, [pc, #344]	; 80013674 <procCreate+0x378>
80013518:	e08f3003 	add	r3, pc, r3
8001351c:	e5933000 	ldr	r3, [r3]
80013520:	e51b2024 	ldr	r2, [fp, #-36]	; 0x24
80013524:	e583219c 	str	r2, [r3, #412]	; 0x19c
80013528:	e59f3148 	ldr	r3, [pc, #328]	; 80013678 <procCreate+0x37c>
8001352c:	e08f3003 	add	r3, pc, r3
80013530:	e5933000 	ldr	r3, [r3]
80013534:	e3e02000 	mvn	r2, #0
80013538:	e58321e8 	str	r2, [r3, #488]	; 0x1e8
8001353c:	e59f3138 	ldr	r3, [pc, #312]	; 8001367c <procCreate+0x380>
80013540:	e08f3003 	add	r3, pc, r3
80013544:	e5933000 	ldr	r3, [r3]
80013548:	e59f2130 	ldr	r2, [pc, #304]	; 80013680 <procCreate+0x384>
8001354c:	e08f2002 	add	r2, pc, r2
80013550:	e5922000 	ldr	r2, [r2]
80013554:	e58321f0 	str	r2, [r3, #496]	; 0x1f0
80013558:	e59f3124 	ldr	r3, [pc, #292]	; 80013684 <procCreate+0x388>
8001355c:	e08f3003 	add	r3, pc, r3
80013560:	e5933000 	ldr	r3, [r3]
80013564:	e3a02000 	mov	r2, #0
80013568:	e5832200 	str	r2, [r3, #512]	; 0x200
8001356c:	e59f3114 	ldr	r3, [pc, #276]	; 80013688 <procCreate+0x38c>
80013570:	e08f3003 	add	r3, pc, r3
80013574:	e5933000 	ldr	r3, [r3]
80013578:	e3a02000 	mov	r2, #0
8001357c:	e5832204 	str	r2, [r3, #516]	; 0x204
80013580:	e59f3104 	ldr	r3, [pc, #260]	; 8001368c <procCreate+0x390>
80013584:	e08f3003 	add	r3, pc, r3
80013588:	e5933000 	ldr	r3, [r3]
8001358c:	e59f20fc 	ldr	r2, [pc, #252]	; 80013690 <procCreate+0x394>
80013590:	e7942002 	ldr	r2, [r4, r2]
80013594:	e58321f4 	str	r2, [r3, #500]	; 0x1f4
80013598:	e59f30f4 	ldr	r3, [pc, #244]	; 80013694 <procCreate+0x398>
8001359c:	e08f3003 	add	r3, pc, r3
800135a0:	e5933000 	ldr	r3, [r3]
800135a4:	e59f20ec 	ldr	r2, [pc, #236]	; 80013698 <procCreate+0x39c>
800135a8:	e7942002 	ldr	r2, [r4, r2]
800135ac:	e58321f8 	str	r2, [r3, #504]	; 0x1f8
800135b0:	e59f30e4 	ldr	r3, [pc, #228]	; 8001369c <procCreate+0x3a0>
800135b4:	e08f3003 	add	r3, pc, r3
800135b8:	e5933000 	ldr	r3, [r3]
800135bc:	e59f20dc 	ldr	r2, [pc, #220]	; 800136a0 <procCreate+0x3a4>
800135c0:	e08f2002 	add	r2, pc, r2
800135c4:	e58321fc 	str	r2, [r3, #508]	; 0x1fc
800135c8:	e59f30d4 	ldr	r3, [pc, #212]	; 800136a4 <procCreate+0x3a8>
800135cc:	e08f3003 	add	r3, pc, r3
800135d0:	e5933000 	ldr	r3, [r3]
800135d4:	e3a02000 	mov	r2, #0
800135d8:	e5832208 	str	r2, [r3, #520]	; 0x208
800135dc:	e59f30c4 	ldr	r3, [pc, #196]	; 800136a8 <procCreate+0x3ac>
800135e0:	e08f3003 	add	r3, pc, r3
800135e4:	e5933000 	ldr	r3, [r3]
800135e8:	e3a02000 	mov	r2, #0
800135ec:	e583220c 	str	r2, [r3, #524]	; 0x20c
800135f0:	e59f30b4 	ldr	r3, [pc, #180]	; 800136ac <procCreate+0x3b0>
800135f4:	e08f3003 	add	r3, pc, r3
800135f8:	e5933000 	ldr	r3, [r3]
800135fc:	e2833e21 	add	r3, r3, #528	; 0x210
80013600:	e1a00003 	mov	r0, r3
80013604:	e3a01000 	mov	r1, #0
80013608:	e3a02d06 	mov	r2, #384	; 0x180
8001360c:	eb0006a9 	bl	800150b8 <memset>
80013610:	e59f3098 	ldr	r3, [pc, #152]	; 800136b0 <procCreate+0x3b4>
80013614:	e08f3003 	add	r3, pc, r3
80013618:	e5933000 	ldr	r3, [r3]
8001361c:	e1a00003 	mov	r0, r3
80013620:	e24bd010 	sub	sp, fp, #16
80013624:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80013628:	0000ccec 	andeq	ip, r0, ip, ror #25
8001362c:	00000024 	andeq	r0, r0, r4, lsr #32
80013630:	0000cc98 	muleq	r0, r8, ip
80013634:	0000cc6c 	andeq	ip, r0, ip, ror #24
80013638:	0000cc50 	andeq	ip, r0, r0, asr ip
8001363c:	00214c34 	eoreq	r4, r1, r4, lsr ip
80013640:	0000cc14 	andeq	ip, r0, r4, lsl ip
80013644:	00014c04 	andeq	r4, r1, r4, lsl #24
80013648:	00214bb0 	strhteq	r4, [r1], -r0
8001364c:	0000cba0 	andeq	ip, r0, r0, lsr #23
80013650:	00214b94 	mlaeq	r1, r4, fp, r4
80013654:	00214b80 	eoreq	r4, r1, r0, lsl #23
80013658:	00214b6c 	eoreq	r4, r1, ip, ror #22
8001365c:	00214b58 	eoreq	r4, r1, r8, asr fp
80013660:	00001d80 	andeq	r1, r0, r0, lsl #27
80013664:	00214b34 	eoreq	r4, r1, r4, lsr fp
80013668:	00214b20 	eoreq	r4, r1, r0, lsr #22
8001366c:	00214b0c 	eoreq	r4, r1, ip, lsl #22
80013670:	00214af8 	strdeq	r4, [r1], -r8	; <UNPREDICTABLE>
80013674:	00214ae4 	eoreq	r4, r1, r4, ror #21
80013678:	00214ad0 	ldrdeq	r4, [r1], -r0	; <UNPREDICTABLE>
8001367c:	00214abc 	strhteq	r4, [r1], -ip
80013680:	00214ab0 	strhteq	r4, [r1], -r0
80013684:	00214aa0 	eoreq	r4, r1, r0, lsr #21
80013688:	00214a8c 	eoreq	r4, r1, ip, lsl #21
8001368c:	00214a78 	eoreq	r4, r1, r8, ror sl
80013690:	00000008 	andeq	r0, r0, r8
80013694:	00214a60 	eoreq	r4, r1, r0, ror #20
80013698:	00000010 	andeq	r0, r0, r0, lsl r0
8001369c:	00214a48 	eoreq	r4, r1, r8, asr #20
800136a0:	fffffd04 			; <UNDEFINED> instruction: 0xfffffd04
800136a4:	00214a30 	eoreq	r4, r1, r0, lsr sl
800136a8:	00214a1c 	eoreq	r4, r1, ip, lsl sl
800136ac:	00214a08 	eoreq	r4, r1, r8, lsl #20
800136b0:	002149e8 	eoreq	r4, r1, r8, ror #19

800136b4 <getCurrentContext>:
800136b4:	e1a0c00d 	mov	ip, sp
800136b8:	e92dd800 	push	{fp, ip, lr, pc}
800136bc:	e24cb004 	sub	fp, ip, #4
800136c0:	e59f2018 	ldr	r2, [pc, #24]	; 800136e0 <getCurrentContext+0x2c>
800136c4:	e08f2002 	add	r2, pc, r2
800136c8:	e59f3014 	ldr	r3, [pc, #20]	; 800136e4 <getCurrentContext+0x30>
800136cc:	e7923003 	ldr	r3, [r2, r3]
800136d0:	e5933000 	ldr	r3, [r3]
800136d4:	e2833f69 	add	r3, r3, #420	; 0x1a4
800136d8:	e1a00003 	mov	r0, r3
800136dc:	e89da800 	ldm	sp, {fp, sp, pc}
800136e0:	0000c938 	andeq	ip, r0, r8, lsr r9
800136e4:	00000014 	andeq	r0, r0, r4, lsl r0

800136e8 <procFree>:
800136e8:	e1a0c00d 	mov	ip, sp
800136ec:	e92dd800 	push	{fp, ip, lr, pc}
800136f0:	e24cb004 	sub	fp, ip, #4
800136f4:	e24dd010 	sub	sp, sp, #16
800136f8:	e50b0018 	str	r0, [fp, #-24]
800136fc:	e3a03000 	mov	r3, #0
80013700:	e50b3010 	str	r3, [fp, #-16]
80013704:	ea000017 	b	80013768 <procFree+0x80>
80013708:	e51b2018 	ldr	r2, [fp, #-24]
8001370c:	e51b3010 	ldr	r3, [fp, #-16]
80013710:	e3a0100c 	mov	r1, #12
80013714:	e0030391 	mul	r3, r1, r3
80013718:	e0823003 	add	r3, r2, r3
8001371c:	e2833e21 	add	r3, r3, #528	; 0x210
80013720:	e5933000 	ldr	r3, [r3]
80013724:	e50b3014 	str	r3, [fp, #-20]
80013728:	e51b3014 	ldr	r3, [fp, #-20]
8001372c:	e3530000 	cmp	r3, #0
80013730:	0a000009 	beq	8001375c <procFree+0x74>
80013734:	e51b2018 	ldr	r2, [fp, #-24]
80013738:	e51b3010 	ldr	r3, [fp, #-16]
8001373c:	e3a0100c 	mov	r1, #12
80013740:	e0030391 	mul	r3, r1, r3
80013744:	e0823003 	add	r3, r2, r3
80013748:	e2833e21 	add	r3, r3, #528	; 0x210
8001374c:	e5933004 	ldr	r3, [r3, #4]
80013750:	e51b0014 	ldr	r0, [fp, #-20]
80013754:	e1a01003 	mov	r1, r3
80013758:	eb000324 	bl	800143f0 <kfUnref>
8001375c:	e51b3010 	ldr	r3, [fp, #-16]
80013760:	e2833001 	add	r3, r3, #1
80013764:	e50b3010 	str	r3, [fp, #-16]
80013768:	e51b3010 	ldr	r3, [fp, #-16]
8001376c:	e353001f 	cmp	r3, #31
80013770:	daffffe4 	ble	80013708 <procFree+0x20>
80013774:	e51b3018 	ldr	r3, [fp, #-24]
80013778:	e2833f82 	add	r3, r3, #520	; 0x208
8001377c:	e1a00003 	mov	r0, r3
80013780:	eb0002c6 	bl	800142a0 <clearMessageQueue>
80013784:	e51b3018 	ldr	r3, [fp, #-24]
80013788:	e59331a0 	ldr	r3, [r3, #416]	; 0x1a0
8001378c:	e1a00003 	mov	r0, r3
80013790:	ebfffb8d 	bl	800125cc <kfree>
80013794:	e51b3018 	ldr	r3, [fp, #-24]
80013798:	e593319c 	ldr	r3, [r3, #412]	; 0x19c
8001379c:	e1a00003 	mov	r0, r3
800137a0:	ebfffb89 	bl	800125cc <kfree>
800137a4:	e51b3018 	ldr	r3, [fp, #-24]
800137a8:	e5933198 	ldr	r3, [r3, #408]	; 0x198
800137ac:	e1a03623 	lsr	r3, r3, #12
800137b0:	e51b0018 	ldr	r0, [fp, #-24]
800137b4:	e1a01003 	mov	r1, r3
800137b8:	ebfffe90 	bl	80013200 <procShrinkMemory>
800137bc:	e51b3018 	ldr	r3, [fp, #-24]
800137c0:	e5933194 	ldr	r3, [r3, #404]	; 0x194
800137c4:	e1a00003 	mov	r0, r3
800137c8:	ebfffb13 	bl	8001241c <freePageTables>
800137cc:	e51b3018 	ldr	r3, [fp, #-24]
800137d0:	e3a02000 	mov	r2, #0
800137d4:	e5c32000 	strb	r2, [r3]
800137d8:	e24bd00c 	sub	sp, fp, #12
800137dc:	e89da800 	ldm	sp, {fp, sp, pc}

800137e0 <procLoad>:
800137e0:	e1a0c00d 	mov	ip, sp
800137e4:	e92dd800 	push	{fp, ip, lr, pc}
800137e8:	e24cb004 	sub	fp, ip, #4
800137ec:	e24dd030 	sub	sp, sp, #48	; 0x30
800137f0:	e50b0038 	str	r0, [fp, #-56]	; 0x38
800137f4:	e50b103c 	str	r1, [fp, #-60]	; 0x3c
800137f8:	e3a03000 	mov	r3, #0
800137fc:	e50b3010 	str	r3, [fp, #-16]
80013800:	e3a03000 	mov	r3, #0
80013804:	e50b301c 	str	r3, [fp, #-28]
80013808:	e3a03000 	mov	r3, #0
8001380c:	e50b3014 	str	r3, [fp, #-20]
80013810:	e51b303c 	ldr	r3, [fp, #-60]	; 0x3c
80013814:	e50b3020 	str	r3, [fp, #-32]
80013818:	e51b3020 	ldr	r3, [fp, #-32]
8001381c:	e1d331b0 	ldrh	r3, [r3, #16]
80013820:	e3530002 	cmp	r3, #2
80013824:	0a000001 	beq	80013830 <procLoad+0x50>
80013828:	e3a03000 	mov	r3, #0
8001382c:	ea00006f 	b	800139f0 <procLoad+0x210>
80013830:	e51b3020 	ldr	r3, [fp, #-32]
80013834:	e593301c 	ldr	r3, [r3, #28]
80013838:	e50b3010 	str	r3, [fp, #-16]
8001383c:	e51b3020 	ldr	r3, [fp, #-32]
80013840:	e1d332bc 	ldrh	r3, [r3, #44]	; 0x2c
80013844:	e50b301c 	str	r3, [fp, #-28]
80013848:	e3a03000 	mov	r3, #0
8001384c:	e50b3014 	str	r3, [fp, #-20]
80013850:	ea000047 	b	80013974 <procLoad+0x194>
80013854:	e3a03000 	mov	r3, #0
80013858:	e50b3018 	str	r3, [fp, #-24]
8001385c:	e51b3010 	ldr	r3, [fp, #-16]
80013860:	e51b203c 	ldr	r2, [fp, #-60]	; 0x3c
80013864:	e0823003 	add	r3, r2, r3
80013868:	e50b3024 	str	r3, [fp, #-36]	; 0x24
8001386c:	ea00000b 	b	800138a0 <procLoad+0xc0>
80013870:	e51b0038 	ldr	r0, [fp, #-56]	; 0x38
80013874:	e3a01001 	mov	r1, #1
80013878:	ebfffe2c 	bl	80013130 <procExpandMemory>
8001387c:	e1a03000 	mov	r3, r0
80013880:	e3530000 	cmp	r3, #0
80013884:	1a000005 	bne	800138a0 <procLoad+0xc0>
80013888:	e59f316c 	ldr	r3, [pc, #364]	; 800139fc <procLoad+0x21c>
8001388c:	e08f3003 	add	r3, pc, r3
80013890:	e1a00003 	mov	r0, r3
80013894:	ebfff3d1 	bl	800107e0 <uartPuts>
80013898:	e3a03000 	mov	r3, #0
8001389c:	ea000053 	b	800139f0 <procLoad+0x210>
800138a0:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
800138a4:	e5932198 	ldr	r2, [r3, #408]	; 0x198
800138a8:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800138ac:	e5931008 	ldr	r1, [r3, #8]
800138b0:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800138b4:	e5933014 	ldr	r3, [r3, #20]
800138b8:	e0813003 	add	r3, r1, r3
800138bc:	e1520003 	cmp	r2, r3
800138c0:	3affffea 	bcc	80013870 <procLoad+0x90>
800138c4:	e3a03000 	mov	r3, #0
800138c8:	e50b3018 	str	r3, [fp, #-24]
800138cc:	ea00001d 	b	80013948 <procLoad+0x168>
800138d0:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800138d4:	e5932008 	ldr	r2, [r3, #8]
800138d8:	e51b3018 	ldr	r3, [fp, #-24]
800138dc:	e0823003 	add	r3, r2, r3
800138e0:	e50b3028 	str	r3, [fp, #-40]	; 0x28
800138e4:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
800138e8:	e5932194 	ldr	r2, [r3, #404]	; 0x194
800138ec:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
800138f0:	e1a00002 	mov	r0, r2
800138f4:	e1a01003 	mov	r1, r3
800138f8:	ebfffa98 	bl	80012360 <resolvePhyAddress>
800138fc:	e1a03000 	mov	r3, r0
80013900:	e50b302c 	str	r3, [fp, #-44]	; 0x2c
80013904:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
80013908:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
8001390c:	e50b3030 	str	r3, [fp, #-48]	; 0x30
80013910:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80013914:	e5932004 	ldr	r2, [r3, #4]
80013918:	e51b3018 	ldr	r3, [fp, #-24]
8001391c:	e0823003 	add	r3, r2, r3
80013920:	e50b3034 	str	r3, [fp, #-52]	; 0x34
80013924:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
80013928:	e51b203c 	ldr	r2, [fp, #-60]	; 0x3c
8001392c:	e0823003 	add	r3, r2, r3
80013930:	e5d32000 	ldrb	r2, [r3]
80013934:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
80013938:	e5c32000 	strb	r2, [r3]
8001393c:	e51b3018 	ldr	r3, [fp, #-24]
80013940:	e2833001 	add	r3, r3, #1
80013944:	e50b3018 	str	r3, [fp, #-24]
80013948:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8001394c:	e5932014 	ldr	r2, [r3, #20]
80013950:	e51b3018 	ldr	r3, [fp, #-24]
80013954:	e1520003 	cmp	r2, r3
80013958:	8affffdc 	bhi	800138d0 <procLoad+0xf0>
8001395c:	e51b3010 	ldr	r3, [fp, #-16]
80013960:	e2833020 	add	r3, r3, #32
80013964:	e50b3010 	str	r3, [fp, #-16]
80013968:	e51b3014 	ldr	r3, [fp, #-20]
8001396c:	e2833001 	add	r3, r3, #1
80013970:	e50b3014 	str	r3, [fp, #-20]
80013974:	e51b2014 	ldr	r2, [fp, #-20]
80013978:	e51b301c 	ldr	r3, [fp, #-28]
8001397c:	e1520003 	cmp	r2, r3
80013980:	baffffb3 	blt	80013854 <procLoad+0x74>
80013984:	e51b3020 	ldr	r3, [fp, #-32]
80013988:	e5933018 	ldr	r3, [r3, #24]
8001398c:	e1a02003 	mov	r2, r3
80013990:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
80013994:	e5832190 	str	r2, [r3, #400]	; 0x190
80013998:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
8001399c:	e3a02003 	mov	r2, #3
800139a0:	e5c32000 	strb	r2, [r3]
800139a4:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
800139a8:	e2833f69 	add	r3, r3, #420	; 0x1a4
800139ac:	e1a00003 	mov	r0, r3
800139b0:	e3a01000 	mov	r1, #0
800139b4:	e3a02044 	mov	r2, #68	; 0x44
800139b8:	eb0005be 	bl	800150b8 <memset>
800139bc:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
800139c0:	e3a02010 	mov	r2, #16
800139c4:	e58321a4 	str	r2, [r3, #420]	; 0x1a4
800139c8:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
800139cc:	e5933190 	ldr	r3, [r3, #400]	; 0x190
800139d0:	e1a02003 	mov	r2, r3
800139d4:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
800139d8:	e58321a8 	str	r2, [r3, #424]	; 0x1a8
800139dc:	e51b2038 	ldr	r2, [fp, #-56]	; 0x38
800139e0:	e3a03a0e 	mov	r3, #57344	; 0xe000
800139e4:	e3473fff 	movt	r3, #32767	; 0x7fff
800139e8:	e58231e0 	str	r3, [r2, #480]	; 0x1e0
800139ec:	e3a03001 	mov	r3, #1
800139f0:	e1a00003 	mov	r0, r3
800139f4:	e24bd00c 	sub	sp, fp, #12
800139f8:	e89da800 	ldm	sp, {fp, sp, pc}
800139fc:	000019b0 			; <UNDEFINED> instruction: 0x000019b0

80013a00 <procStart>:
80013a00:	e1a0c00d 	mov	ip, sp
80013a04:	e92dd800 	push	{fp, ip, lr, pc}
80013a08:	e24cb004 	sub	fp, ip, #4
80013a0c:	e24dd008 	sub	sp, sp, #8
80013a10:	e50b0010 	str	r0, [fp, #-16]
80013a14:	e59f2044 	ldr	r2, [pc, #68]	; 80013a60 <procStart+0x60>
80013a18:	e08f2002 	add	r2, pc, r2
80013a1c:	e59f3040 	ldr	r3, [pc, #64]	; 80013a64 <procStart+0x64>
80013a20:	e7923003 	ldr	r3, [r2, r3]
80013a24:	e51b2010 	ldr	r2, [fp, #-16]
80013a28:	e5832000 	str	r2, [r3]
80013a2c:	e51b3010 	ldr	r3, [fp, #-16]
80013a30:	e5933194 	ldr	r3, [r3, #404]	; 0x194
80013a34:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80013a38:	e1a00003 	mov	r0, r3
80013a3c:	ebfff1bf 	bl	80010140 <__setTranslationTableBase>
80013a40:	e3a04000 	mov	r4, #0
80013a44:	ee084f17 	mcr	15, 0, r4, cr8, cr7, {0}
80013a48:	e51b3010 	ldr	r3, [fp, #-16]
80013a4c:	e2833f69 	add	r3, r3, #420	; 0x1a4
80013a50:	e1a00003 	mov	r0, r3
80013a54:	ebfff1cc 	bl	8001018c <__switchToContext>
80013a58:	e24bd00c 	sub	sp, fp, #12
80013a5c:	e89da800 	ldm	sp, {fp, sp, pc}
80013a60:	0000c5e4 	andeq	ip, r0, r4, ror #11
80013a64:	00000014 	andeq	r0, r0, r4, lsl r0

80013a68 <procGet>:
80013a68:	e1a0c00d 	mov	ip, sp
80013a6c:	e92dd800 	push	{fp, ip, lr, pc}
80013a70:	e24cb004 	sub	fp, ip, #4
80013a74:	e24dd008 	sub	sp, sp, #8
80013a78:	e50b0010 	str	r0, [fp, #-16]
80013a7c:	e59f1044 	ldr	r1, [pc, #68]	; 80013ac8 <procGet+0x60>
80013a80:	e08f1001 	add	r1, pc, r1
80013a84:	e51b3010 	ldr	r3, [fp, #-16]
80013a88:	e3530000 	cmp	r3, #0
80013a8c:	ba000002 	blt	80013a9c <procGet+0x34>
80013a90:	e51b3010 	ldr	r3, [fp, #-16]
80013a94:	e353007f 	cmp	r3, #127	; 0x7f
80013a98:	da000001 	ble	80013aa4 <procGet+0x3c>
80013a9c:	e3a03000 	mov	r3, #0
80013aa0:	ea000005 	b	80013abc <procGet+0x54>
80013aa4:	e51b3010 	ldr	r3, [fp, #-16]
80013aa8:	e3a02e39 	mov	r2, #912	; 0x390
80013aac:	e0030392 	mul	r3, r2, r3
80013ab0:	e59f2014 	ldr	r2, [pc, #20]	; 80013acc <procGet+0x64>
80013ab4:	e7912002 	ldr	r2, [r1, r2]
80013ab8:	e0833002 	add	r3, r3, r2
80013abc:	e1a00003 	mov	r0, r3
80013ac0:	e24bd00c 	sub	sp, fp, #12
80013ac4:	e89da800 	ldm	sp, {fp, sp, pc}
80013ac8:	0000c57c 	andeq	ip, r0, ip, ror r5
80013acc:	00000024 	andeq	r0, r0, r4, lsr #32

80013ad0 <kfork>:
80013ad0:	e1a0c00d 	mov	ip, sp
80013ad4:	e92dd800 	push	{fp, ip, lr, pc}
80013ad8:	e24cb004 	sub	fp, ip, #4
80013adc:	e24dd020 	sub	sp, sp, #32
80013ae0:	e59f22e8 	ldr	r2, [pc, #744]	; 80013dd0 <kfork+0x300>
80013ae4:	e08f2002 	add	r2, pc, r2
80013ae8:	e3a03000 	mov	r3, #0
80013aec:	e50b3014 	str	r3, [fp, #-20]
80013af0:	e59f32dc 	ldr	r3, [pc, #732]	; 80013dd4 <kfork+0x304>
80013af4:	e7923003 	ldr	r3, [r2, r3]
80013af8:	e5933000 	ldr	r3, [r3]
80013afc:	e50b3018 	str	r3, [fp, #-24]
80013b00:	e3a03000 	mov	r3, #0
80013b04:	e50b3010 	str	r3, [fp, #-16]
80013b08:	ebfffdfb 	bl	800132fc <procCreate>
80013b0c:	e50b0014 	str	r0, [fp, #-20]
80013b10:	e51b3018 	ldr	r3, [fp, #-24]
80013b14:	e5933198 	ldr	r3, [r3, #408]	; 0x198
80013b18:	e1a03623 	lsr	r3, r3, #12
80013b1c:	e51b0014 	ldr	r0, [fp, #-20]
80013b20:	e1a01003 	mov	r1, r3
80013b24:	ebfffd81 	bl	80013130 <procExpandMemory>
80013b28:	e1a03000 	mov	r3, r0
80013b2c:	e3530000 	cmp	r3, #0
80013b30:	1a000005 	bne	80013b4c <kfork+0x7c>
80013b34:	e59f329c 	ldr	r3, [pc, #668]	; 80013dd8 <kfork+0x308>
80013b38:	e08f3003 	add	r3, pc, r3
80013b3c:	e1a00003 	mov	r0, r3
80013b40:	ebfff326 	bl	800107e0 <uartPuts>
80013b44:	e3e03000 	mvn	r3, #0
80013b48:	ea00009d 	b	80013dc4 <kfork+0x2f4>
80013b4c:	e3a03000 	mov	r3, #0
80013b50:	e50b3010 	str	r3, [fp, #-16]
80013b54:	ea000012 	b	80013ba4 <kfork+0xd4>
80013b58:	e51b3014 	ldr	r3, [fp, #-20]
80013b5c:	e5933194 	ldr	r3, [r3, #404]	; 0x194
80013b60:	e1a00003 	mov	r0, r3
80013b64:	e51b1010 	ldr	r1, [fp, #-16]
80013b68:	ebfff9fc 	bl	80012360 <resolvePhyAddress>
80013b6c:	e1a03000 	mov	r3, r0
80013b70:	e50b301c 	str	r3, [fp, #-28]
80013b74:	e51b301c 	ldr	r3, [fp, #-28]
80013b78:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80013b7c:	e50b3020 	str	r3, [fp, #-32]
80013b80:	e51b3010 	ldr	r3, [fp, #-16]
80013b84:	e50b3024 	str	r3, [fp, #-36]	; 0x24
80013b88:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80013b8c:	e5d32000 	ldrb	r2, [r3]
80013b90:	e51b3020 	ldr	r3, [fp, #-32]
80013b94:	e5c32000 	strb	r2, [r3]
80013b98:	e51b3010 	ldr	r3, [fp, #-16]
80013b9c:	e2833001 	add	r3, r3, #1
80013ba0:	e50b3010 	str	r3, [fp, #-16]
80013ba4:	e51b3018 	ldr	r3, [fp, #-24]
80013ba8:	e5932198 	ldr	r2, [r3, #408]	; 0x198
80013bac:	e51b3010 	ldr	r3, [fp, #-16]
80013bb0:	e1520003 	cmp	r2, r3
80013bb4:	8affffe7 	bhi	80013b58 <kfork+0x88>
80013bb8:	e51b3014 	ldr	r3, [fp, #-20]
80013bbc:	e59321a0 	ldr	r2, [r3, #416]	; 0x1a0
80013bc0:	e51b3018 	ldr	r3, [fp, #-24]
80013bc4:	e59331a0 	ldr	r3, [r3, #416]	; 0x1a0
80013bc8:	e1a00002 	mov	r0, r2
80013bcc:	e1a01003 	mov	r1, r3
80013bd0:	e3a02a01 	mov	r2, #4096	; 0x1000
80013bd4:	eb0003e4 	bl	80014b6c <memcpy>
80013bd8:	e51b3014 	ldr	r3, [fp, #-20]
80013bdc:	e593219c 	ldr	r2, [r3, #412]	; 0x19c
80013be0:	e51b3018 	ldr	r3, [fp, #-24]
80013be4:	e593319c 	ldr	r3, [r3, #412]	; 0x19c
80013be8:	e1a00002 	mov	r0, r2
80013bec:	e1a01003 	mov	r1, r3
80013bf0:	e3a02a01 	mov	r2, #4096	; 0x1000
80013bf4:	eb0003dc 	bl	80014b6c <memcpy>
80013bf8:	e51b3014 	ldr	r3, [fp, #-20]
80013bfc:	e2832f69 	add	r2, r3, #420	; 0x1a4
80013c00:	e51b3018 	ldr	r3, [fp, #-24]
80013c04:	e2833f69 	add	r3, r3, #420	; 0x1a4
80013c08:	e1a00002 	mov	r0, r2
80013c0c:	e1a01003 	mov	r1, r3
80013c10:	e3a02044 	mov	r2, #68	; 0x44
80013c14:	eb0003d4 	bl	80014b6c <memcpy>
80013c18:	e51b2014 	ldr	r2, [fp, #-20]
80013c1c:	e51b3018 	ldr	r3, [fp, #-24]
80013c20:	e282ce1f 	add	ip, r2, #496	; 0x1f0
80013c24:	e283ee1f 	add	lr, r3, #496	; 0x1f0
80013c28:	e8be000f 	ldm	lr!, {r0, r1, r2, r3}
80013c2c:	e8ac000f 	stmia	ip!, {r0, r1, r2, r3}
80013c30:	e89e0003 	ldm	lr, {r0, r1}
80013c34:	e88c0003 	stm	ip, {r0, r1}
80013c38:	e51b3014 	ldr	r3, [fp, #-20]
80013c3c:	e51b2014 	ldr	r2, [fp, #-20]
80013c40:	e58321f0 	str	r2, [r3, #496]	; 0x1f0
80013c44:	e51b3014 	ldr	r3, [fp, #-20]
80013c48:	e3a02000 	mov	r2, #0
80013c4c:	e58321ac 	str	r2, [r3, #428]	; 0x1ac
80013c50:	e51b3014 	ldr	r3, [fp, #-20]
80013c54:	e3a02003 	mov	r2, #3
80013c58:	e5c32000 	strb	r2, [r3]
80013c5c:	e51b3018 	ldr	r3, [fp, #-24]
80013c60:	e5932004 	ldr	r2, [r3, #4]
80013c64:	e51b3014 	ldr	r3, [fp, #-20]
80013c68:	e5832008 	str	r2, [r3, #8]
80013c6c:	e51b3018 	ldr	r3, [fp, #-24]
80013c70:	e593200c 	ldr	r2, [r3, #12]
80013c74:	e51b3014 	ldr	r3, [fp, #-20]
80013c78:	e583200c 	str	r2, [r3, #12]
80013c7c:	e51b3014 	ldr	r3, [fp, #-20]
80013c80:	e2832010 	add	r2, r3, #16
80013c84:	e51b3018 	ldr	r3, [fp, #-24]
80013c88:	e2833010 	add	r3, r3, #16
80013c8c:	e1a00002 	mov	r0, r2
80013c90:	e1a01003 	mov	r1, r3
80013c94:	eb0003d8 	bl	80014bfc <strcpy>
80013c98:	e51b3014 	ldr	r3, [fp, #-20]
80013c9c:	e2832090 	add	r2, r3, #144	; 0x90
80013ca0:	e51b3018 	ldr	r3, [fp, #-24]
80013ca4:	e2833090 	add	r3, r3, #144	; 0x90
80013ca8:	e1a00002 	mov	r0, r2
80013cac:	e1a01003 	mov	r1, r3
80013cb0:	eb0003d1 	bl	80014bfc <strcpy>
80013cb4:	e3a03000 	mov	r3, #0
80013cb8:	e50b3010 	str	r3, [fp, #-16]
80013cbc:	ea00003b 	b	80013db0 <kfork+0x2e0>
80013cc0:	e51b2018 	ldr	r2, [fp, #-24]
80013cc4:	e51b3010 	ldr	r3, [fp, #-16]
80013cc8:	e3a0100c 	mov	r1, #12
80013ccc:	e0030391 	mul	r3, r1, r3
80013cd0:	e0823003 	add	r3, r2, r3
80013cd4:	e2833e21 	add	r3, r3, #528	; 0x210
80013cd8:	e5933000 	ldr	r3, [r3]
80013cdc:	e50b3028 	str	r3, [fp, #-40]	; 0x28
80013ce0:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80013ce4:	e3530000 	cmp	r3, #0
80013ce8:	0a00002d 	beq	80013da4 <kfork+0x2d4>
80013cec:	e51b2014 	ldr	r2, [fp, #-20]
80013cf0:	e51b3010 	ldr	r3, [fp, #-16]
80013cf4:	e3a0100c 	mov	r1, #12
80013cf8:	e0030391 	mul	r3, r1, r3
80013cfc:	e0823003 	add	r3, r2, r3
80013d00:	e2833e21 	add	r3, r3, #528	; 0x210
80013d04:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80013d08:	e5832000 	str	r2, [r3]
80013d0c:	e51b2018 	ldr	r2, [fp, #-24]
80013d10:	e51b3010 	ldr	r3, [fp, #-16]
80013d14:	e3a0100c 	mov	r1, #12
80013d18:	e0030391 	mul	r3, r1, r3
80013d1c:	e0823003 	add	r3, r2, r3
80013d20:	e2833e21 	add	r3, r3, #528	; 0x210
80013d24:	e5932004 	ldr	r2, [r3, #4]
80013d28:	e51b1014 	ldr	r1, [fp, #-20]
80013d2c:	e51b3010 	ldr	r3, [fp, #-16]
80013d30:	e3a0000c 	mov	r0, #12
80013d34:	e0030390 	mul	r3, r0, r3
80013d38:	e0813003 	add	r3, r1, r3
80013d3c:	e2833e21 	add	r3, r3, #528	; 0x210
80013d40:	e5832004 	str	r2, [r3, #4]
80013d44:	e51b2018 	ldr	r2, [fp, #-24]
80013d48:	e51b3010 	ldr	r3, [fp, #-16]
80013d4c:	e3a0100c 	mov	r1, #12
80013d50:	e0030391 	mul	r3, r1, r3
80013d54:	e0823003 	add	r3, r2, r3
80013d58:	e2833f86 	add	r3, r3, #536	; 0x218
80013d5c:	e5932000 	ldr	r2, [r3]
80013d60:	e51b1014 	ldr	r1, [fp, #-20]
80013d64:	e51b3010 	ldr	r3, [fp, #-16]
80013d68:	e3a0000c 	mov	r0, #12
80013d6c:	e0030390 	mul	r3, r0, r3
80013d70:	e0813003 	add	r3, r1, r3
80013d74:	e2833f86 	add	r3, r3, #536	; 0x218
80013d78:	e5832000 	str	r2, [r3]
80013d7c:	e51b2014 	ldr	r2, [fp, #-20]
80013d80:	e51b3010 	ldr	r3, [fp, #-16]
80013d84:	e3a0100c 	mov	r1, #12
80013d88:	e0030391 	mul	r3, r1, r3
80013d8c:	e0823003 	add	r3, r2, r3
80013d90:	e2833e21 	add	r3, r3, #528	; 0x210
80013d94:	e5933004 	ldr	r3, [r3, #4]
80013d98:	e51b0028 	ldr	r0, [fp, #-40]	; 0x28
80013d9c:	e1a01003 	mov	r1, r3
80013da0:	eb0001e0 	bl	80014528 <kfRef>
80013da4:	e51b3010 	ldr	r3, [fp, #-16]
80013da8:	e2833001 	add	r3, r3, #1
80013dac:	e50b3010 	str	r3, [fp, #-16]
80013db0:	e51b3010 	ldr	r3, [fp, #-16]
80013db4:	e353001f 	cmp	r3, #31
80013db8:	9affffc0 	bls	80013cc0 <kfork+0x1f0>
80013dbc:	e51b3014 	ldr	r3, [fp, #-20]
80013dc0:	e5933004 	ldr	r3, [r3, #4]
80013dc4:	e1a00003 	mov	r0, r3
80013dc8:	e24bd00c 	sub	sp, fp, #12
80013dcc:	e89da800 	ldm	sp, {fp, sp, pc}
80013dd0:	0000c518 	andeq	ip, r0, r8, lsl r5
80013dd4:	00000014 	andeq	r0, r0, r4, lsl r0
80013dd8:	00001704 	andeq	r1, r0, r4, lsl #14

80013ddc <procExit>:
80013ddc:	e1a0c00d 	mov	ip, sp
80013de0:	e92dd810 	push	{r4, fp, ip, lr, pc}
80013de4:	e24cb004 	sub	fp, ip, #4
80013de8:	e24dd00c 	sub	sp, sp, #12
80013dec:	e59f40f4 	ldr	r4, [pc, #244]	; 80013ee8 <procExit+0x10c>
80013df0:	e08f4004 	add	r4, pc, r4
80013df4:	e59f30f0 	ldr	r3, [pc, #240]	; 80013eec <procExit+0x110>
80013df8:	e7943003 	ldr	r3, [r4, r3]
80013dfc:	e5933000 	ldr	r3, [r3]
80013e00:	e3530000 	cmp	r3, #0
80013e04:	1a000000 	bne	80013e0c <procExit+0x30>
80013e08:	ea000034 	b	80013ee0 <procExit+0x104>
80013e0c:	e59f30dc 	ldr	r3, [pc, #220]	; 80013ef0 <procExit+0x114>
80013e10:	e7943003 	ldr	r3, [r4, r3]
80013e14:	e5933000 	ldr	r3, [r3]
80013e18:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
80013e1c:	e1a00003 	mov	r0, r3
80013e20:	ebfff0c6 	bl	80010140 <__setTranslationTableBase>
80013e24:	e59fd0e0 	ldr	sp, [pc, #224]	; 80013f0c <_abortEntry+0x14>
80013e28:	e28dda01 	add	sp, sp, #4096	; 0x1000
80013e2c:	e3a03000 	mov	r3, #0
80013e30:	e50b3018 	str	r3, [fp, #-24]
80013e34:	ea00001b 	b	80013ea8 <procExit+0xcc>
80013e38:	e51b3018 	ldr	r3, [fp, #-24]
80013e3c:	e3a02e39 	mov	r2, #912	; 0x390
80013e40:	e0030392 	mul	r3, r2, r3
80013e44:	e59f20a8 	ldr	r2, [pc, #168]	; 80013ef4 <procExit+0x118>
80013e48:	e7942002 	ldr	r2, [r4, r2]
80013e4c:	e0833002 	add	r3, r3, r2
80013e50:	e50b301c 	str	r3, [fp, #-28]
80013e54:	e51b301c 	ldr	r3, [fp, #-28]
80013e58:	e5d33000 	ldrb	r3, [r3]
80013e5c:	e3530002 	cmp	r3, #2
80013e60:	1a00000d 	bne	80013e9c <procExit+0xc0>
80013e64:	e51b301c 	ldr	r3, [fp, #-28]
80013e68:	e59321e8 	ldr	r2, [r3, #488]	; 0x1e8
80013e6c:	e59f3078 	ldr	r3, [pc, #120]	; 80013eec <procExit+0x110>
80013e70:	e7943003 	ldr	r3, [r4, r3]
80013e74:	e5933000 	ldr	r3, [r3]
80013e78:	e5933004 	ldr	r3, [r3, #4]
80013e7c:	e1520003 	cmp	r2, r3
80013e80:	1a000005 	bne	80013e9c <procExit+0xc0>
80013e84:	e51b301c 	ldr	r3, [fp, #-28]
80013e88:	e3e02000 	mvn	r2, #0
80013e8c:	e58321e8 	str	r2, [r3, #488]	; 0x1e8
80013e90:	e51b301c 	ldr	r3, [fp, #-28]
80013e94:	e3a02003 	mov	r2, #3
80013e98:	e5c32000 	strb	r2, [r3]
80013e9c:	e51b3018 	ldr	r3, [fp, #-24]
80013ea0:	e2833001 	add	r3, r3, #1
80013ea4:	e50b3018 	str	r3, [fp, #-24]
80013ea8:	e51b3018 	ldr	r3, [fp, #-24]
80013eac:	e353007f 	cmp	r3, #127	; 0x7f
80013eb0:	daffffe0 	ble	80013e38 <procExit+0x5c>
80013eb4:	e59f3030 	ldr	r3, [pc, #48]	; 80013eec <procExit+0x110>
80013eb8:	e7943003 	ldr	r3, [r4, r3]
80013ebc:	e5933000 	ldr	r3, [r3]
80013ec0:	e1a00003 	mov	r0, r3
80013ec4:	ebfffe07 	bl	800136e8 <procFree>
80013ec8:	e59f301c 	ldr	r3, [pc, #28]	; 80013eec <procExit+0x110>
80013ecc:	e7943003 	ldr	r3, [r4, r3]
80013ed0:	e3a02000 	mov	r2, #0
80013ed4:	e5832000 	str	r2, [r3]
80013ed8:	eb0002c9 	bl	80014a04 <schedule>
80013edc:	e1a00000 	nop			; (mov r0, r0)
80013ee0:	e24bd010 	sub	sp, fp, #16
80013ee4:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80013ee8:	0000c20c 	andeq	ip, r0, ip, lsl #4
80013eec:	00000014 	andeq	r0, r0, r4, lsl r0
80013ef0:	0000000c 	andeq	r0, r0, ip
80013ef4:	00000024 	andeq	r0, r0, r4, lsr #32

80013ef8 <_abortEntry>:
80013ef8:	e1a0c00d 	mov	ip, sp
80013efc:	e92dd800 	push	{fp, ip, lr, pc}
80013f00:	e24cb004 	sub	fp, ip, #4
80013f04:	ebffffb4 	bl	80013ddc <procExit>
80013f08:	e89da800 	ldm	sp, {fp, sp, pc}
80013f0c:	80025000 	andhi	r5, r2, r0

80013f10 <getPackageSize>:
80013f10:	e1a0c00d 	mov	ip, sp
80013f14:	e92dd800 	push	{fp, ip, lr, pc}
80013f18:	e24cb004 	sub	fp, ip, #4
80013f1c:	e24dd008 	sub	sp, sp, #8
80013f20:	e50b0010 	str	r0, [fp, #-16]
80013f24:	e51b3010 	ldr	r3, [fp, #-16]
80013f28:	e3530000 	cmp	r3, #0
80013f2c:	1a000001 	bne	80013f38 <getPackageSize+0x28>
80013f30:	e3a03000 	mov	r3, #0
80013f34:	ea000002 	b	80013f44 <getPackageSize+0x34>
80013f38:	e51b3010 	ldr	r3, [fp, #-16]
80013f3c:	e593300c 	ldr	r3, [r3, #12]
80013f40:	e2833010 	add	r3, r3, #16
80013f44:	e1a00003 	mov	r0, r3
80013f48:	e24bd00c 	sub	sp, fp, #12
80013f4c:	e89da800 	ldm	sp, {fp, sp, pc}

80013f50 <ksend>:
80013f50:	e1a0c00d 	mov	ip, sp
80013f54:	e92dd810 	push	{r4, fp, ip, lr, pc}
80013f58:	e24cb004 	sub	fp, ip, #4
80013f5c:	e24dd024 	sub	sp, sp, #36	; 0x24
80013f60:	e50b0028 	str	r0, [fp, #-40]	; 0x28
80013f64:	e50b102c 	str	r1, [fp, #-44]	; 0x2c
80013f68:	e50b2030 	str	r2, [fp, #-48]	; 0x30
80013f6c:	e59f418c 	ldr	r4, [pc, #396]	; 80014100 <ksend+0x1b0>
80013f70:	e08f4004 	add	r4, pc, r4
80013f74:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
80013f78:	e3530000 	cmp	r3, #0
80013f7c:	1a000001 	bne	80013f88 <ksend+0x38>
80013f80:	e3e03000 	mvn	r3, #0
80013f84:	ea00005a 	b	800140f4 <ksend+0x1a4>
80013f88:	e51b002c 	ldr	r0, [fp, #-44]	; 0x2c
80013f8c:	ebfffeb5 	bl	80013a68 <procGet>
80013f90:	e50b0018 	str	r0, [fp, #-24]
80013f94:	e51b3018 	ldr	r3, [fp, #-24]
80013f98:	e3530000 	cmp	r3, #0
80013f9c:	1a000001 	bne	80013fa8 <ksend+0x58>
80013fa0:	e3e03000 	mvn	r3, #0
80013fa4:	ea000052 	b	800140f4 <ksend+0x1a4>
80013fa8:	e3a0000c 	mov	r0, #12
80013fac:	ebfffc23 	bl	80013040 <kmalloc>
80013fb0:	e50b001c 	str	r0, [fp, #-28]
80013fb4:	e51b301c 	ldr	r3, [fp, #-28]
80013fb8:	e3530000 	cmp	r3, #0
80013fbc:	1a000001 	bne	80013fc8 <ksend+0x78>
80013fc0:	e3e03000 	mvn	r3, #0
80013fc4:	ea00004a 	b	800140f4 <ksend+0x1a4>
80013fc8:	e51b0030 	ldr	r0, [fp, #-48]	; 0x30
80013fcc:	ebffffcf 	bl	80013f10 <getPackageSize>
80013fd0:	e50b0020 	str	r0, [fp, #-32]
80013fd4:	e51b0020 	ldr	r0, [fp, #-32]
80013fd8:	ebfffc18 	bl	80013040 <kmalloc>
80013fdc:	e1a02000 	mov	r2, r0
80013fe0:	e51b301c 	ldr	r3, [fp, #-28]
80013fe4:	e5832008 	str	r2, [r3, #8]
80013fe8:	e51b301c 	ldr	r3, [fp, #-28]
80013fec:	e5933008 	ldr	r3, [r3, #8]
80013ff0:	e3530000 	cmp	r3, #0
80013ff4:	1a000003 	bne	80014008 <ksend+0xb8>
80013ff8:	e51b001c 	ldr	r0, [fp, #-28]
80013ffc:	ebfffc1e 	bl	8001307c <kmfree>
80014000:	e3e03000 	mvn	r3, #0
80014004:	ea00003a 	b	800140f4 <ksend+0x1a4>
80014008:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
8001400c:	e3530000 	cmp	r3, #0
80014010:	aa000009 	bge	8001403c <ksend+0xec>
80014014:	e59f30e8 	ldr	r3, [pc, #232]	; 80014104 <ksend+0x1b4>
80014018:	e08f3003 	add	r3, pc, r3
8001401c:	e5933000 	ldr	r3, [r3]
80014020:	e2831001 	add	r1, r3, #1
80014024:	e59f20dc 	ldr	r2, [pc, #220]	; 80014108 <ksend+0x1b8>
80014028:	e08f2002 	add	r2, pc, r2
8001402c:	e5821000 	str	r1, [r2]
80014030:	e51b2030 	ldr	r2, [fp, #-48]	; 0x30
80014034:	e5823000 	str	r3, [r2]
80014038:	ea000002 	b	80014048 <ksend+0xf8>
8001403c:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
80014040:	e51b2028 	ldr	r2, [fp, #-40]	; 0x28
80014044:	e5832000 	str	r2, [r3]
80014048:	e59f30bc 	ldr	r3, [pc, #188]	; 8001410c <ksend+0x1bc>
8001404c:	e7943003 	ldr	r3, [r4, r3]
80014050:	e5933000 	ldr	r3, [r3]
80014054:	e5932004 	ldr	r2, [r3, #4]
80014058:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
8001405c:	e5832004 	str	r2, [r3, #4]
80014060:	e51b301c 	ldr	r3, [fp, #-28]
80014064:	e5933008 	ldr	r3, [r3, #8]
80014068:	e1a00003 	mov	r0, r3
8001406c:	e51b1030 	ldr	r1, [fp, #-48]	; 0x30
80014070:	e51b2020 	ldr	r2, [fp, #-32]
80014074:	eb0002bc 	bl	80014b6c <memcpy>
80014078:	e51b301c 	ldr	r3, [fp, #-28]
8001407c:	e3a02000 	mov	r2, #0
80014080:	e5832000 	str	r2, [r3]
80014084:	e51b301c 	ldr	r3, [fp, #-28]
80014088:	e3a02000 	mov	r2, #0
8001408c:	e5832004 	str	r2, [r3, #4]
80014090:	e51b3018 	ldr	r3, [fp, #-24]
80014094:	e5933208 	ldr	r3, [r3, #520]	; 0x208
80014098:	e3530000 	cmp	r3, #0
8001409c:	1a000007 	bne	800140c0 <ksend+0x170>
800140a0:	e51b3018 	ldr	r3, [fp, #-24]
800140a4:	e51b201c 	ldr	r2, [fp, #-28]
800140a8:	e583220c 	str	r2, [r3, #524]	; 0x20c
800140ac:	e51b3018 	ldr	r3, [fp, #-24]
800140b0:	e593220c 	ldr	r2, [r3, #524]	; 0x20c
800140b4:	e51b3018 	ldr	r3, [fp, #-24]
800140b8:	e5832208 	str	r2, [r3, #520]	; 0x208
800140bc:	ea00000a 	b	800140ec <ksend+0x19c>
800140c0:	e51b3018 	ldr	r3, [fp, #-24]
800140c4:	e593320c 	ldr	r3, [r3, #524]	; 0x20c
800140c8:	e51b201c 	ldr	r2, [fp, #-28]
800140cc:	e5832000 	str	r2, [r3]
800140d0:	e51b3018 	ldr	r3, [fp, #-24]
800140d4:	e593220c 	ldr	r2, [r3, #524]	; 0x20c
800140d8:	e51b301c 	ldr	r3, [fp, #-28]
800140dc:	e5832004 	str	r2, [r3, #4]
800140e0:	e51b3018 	ldr	r3, [fp, #-24]
800140e4:	e51b201c 	ldr	r2, [fp, #-28]
800140e8:	e583220c 	str	r2, [r3, #524]	; 0x20c
800140ec:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
800140f0:	e5933000 	ldr	r3, [r3]
800140f4:	e1a00003 	mov	r0, r3
800140f8:	e24bd010 	sub	sp, fp, #16
800140fc:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80014100:	0000c08c 	andeq	ip, r0, ip, lsl #1
80014104:	002307e8 	eoreq	r0, r3, r8, ror #15
80014108:	002307d8 	ldrdeq	r0, [r3], -r8	; <UNPREDICTABLE>
8001410c:	00000014 	andeq	r0, r0, r4, lsl r0

80014110 <krecv>:
80014110:	e1a0c00d 	mov	ip, sp
80014114:	e92dd810 	push	{r4, fp, ip, lr, pc}
80014118:	e24cb004 	sub	fp, ip, #4
8001411c:	e24dd01c 	sub	sp, sp, #28
80014120:	e50b0028 	str	r0, [fp, #-40]	; 0x28
80014124:	e59f416c 	ldr	r4, [pc, #364]	; 80014298 <krecv+0x188>
80014128:	e08f4004 	add	r4, pc, r4
8001412c:	e59f3168 	ldr	r3, [pc, #360]	; 8001429c <krecv+0x18c>
80014130:	e7943003 	ldr	r3, [r4, r3]
80014134:	e5933000 	ldr	r3, [r3]
80014138:	e2833f82 	add	r3, r3, #520	; 0x208
8001413c:	e50b301c 	str	r3, [fp, #-28]
80014140:	e51b301c 	ldr	r3, [fp, #-28]
80014144:	e5933000 	ldr	r3, [r3]
80014148:	e50b3018 	str	r3, [fp, #-24]
8001414c:	ea00000b 	b	80014180 <krecv+0x70>
80014150:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80014154:	e3530000 	cmp	r3, #0
80014158:	ba00000b 	blt	8001418c <krecv+0x7c>
8001415c:	e51b3018 	ldr	r3, [fp, #-24]
80014160:	e5933008 	ldr	r3, [r3, #8]
80014164:	e5932000 	ldr	r2, [r3]
80014168:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
8001416c:	e1520003 	cmp	r2, r3
80014170:	0a000005 	beq	8001418c <krecv+0x7c>
80014174:	e51b3018 	ldr	r3, [fp, #-24]
80014178:	e5933000 	ldr	r3, [r3]
8001417c:	e50b3018 	str	r3, [fp, #-24]
80014180:	e51b3018 	ldr	r3, [fp, #-24]
80014184:	e3530000 	cmp	r3, #0
80014188:	1afffff0 	bne	80014150 <krecv+0x40>
8001418c:	e51b3018 	ldr	r3, [fp, #-24]
80014190:	e3530000 	cmp	r3, #0
80014194:	1a000001 	bne	800141a0 <krecv+0x90>
80014198:	e3a03000 	mov	r3, #0
8001419c:	ea00003a 	b	8001428c <krecv+0x17c>
800141a0:	e51b3018 	ldr	r3, [fp, #-24]
800141a4:	e5933008 	ldr	r3, [r3, #8]
800141a8:	e1a00003 	mov	r0, r3
800141ac:	ebffff57 	bl	80013f10 <getPackageSize>
800141b0:	e50b0020 	str	r0, [fp, #-32]
800141b4:	e59f30e0 	ldr	r3, [pc, #224]	; 8001429c <krecv+0x18c>
800141b8:	e7943003 	ldr	r3, [r4, r3]
800141bc:	e5933000 	ldr	r3, [r3]
800141c0:	e2833e1f 	add	r3, r3, #496	; 0x1f0
800141c4:	e1a00003 	mov	r0, r3
800141c8:	e51b1020 	ldr	r1, [fp, #-32]
800141cc:	ebfffa04 	bl	800129e4 <trunkMalloc>
800141d0:	e50b0024 	str	r0, [fp, #-36]	; 0x24
800141d4:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
800141d8:	e3530000 	cmp	r3, #0
800141dc:	1a000001 	bne	800141e8 <krecv+0xd8>
800141e0:	e3a03000 	mov	r3, #0
800141e4:	ea000028 	b	8001428c <krecv+0x17c>
800141e8:	e51b3018 	ldr	r3, [fp, #-24]
800141ec:	e5933008 	ldr	r3, [r3, #8]
800141f0:	e51b0024 	ldr	r0, [fp, #-36]	; 0x24
800141f4:	e1a01003 	mov	r1, r3
800141f8:	e51b2020 	ldr	r2, [fp, #-32]
800141fc:	eb00025a 	bl	80014b6c <memcpy>
80014200:	e51b3018 	ldr	r3, [fp, #-24]
80014204:	e5933004 	ldr	r3, [r3, #4]
80014208:	e3530000 	cmp	r3, #0
8001420c:	0a000005 	beq	80014228 <krecv+0x118>
80014210:	e51b3018 	ldr	r3, [fp, #-24]
80014214:	e5933004 	ldr	r3, [r3, #4]
80014218:	e51b2018 	ldr	r2, [fp, #-24]
8001421c:	e5922000 	ldr	r2, [r2]
80014220:	e5832000 	str	r2, [r3]
80014224:	ea000003 	b	80014238 <krecv+0x128>
80014228:	e51b3018 	ldr	r3, [fp, #-24]
8001422c:	e5932000 	ldr	r2, [r3]
80014230:	e51b301c 	ldr	r3, [fp, #-28]
80014234:	e5832000 	str	r2, [r3]
80014238:	e51b3018 	ldr	r3, [fp, #-24]
8001423c:	e5933000 	ldr	r3, [r3]
80014240:	e3530000 	cmp	r3, #0
80014244:	0a000005 	beq	80014260 <krecv+0x150>
80014248:	e51b3018 	ldr	r3, [fp, #-24]
8001424c:	e5933000 	ldr	r3, [r3]
80014250:	e51b2018 	ldr	r2, [fp, #-24]
80014254:	e5922004 	ldr	r2, [r2, #4]
80014258:	e5832004 	str	r2, [r3, #4]
8001425c:	ea000003 	b	80014270 <krecv+0x160>
80014260:	e51b3018 	ldr	r3, [fp, #-24]
80014264:	e5932004 	ldr	r2, [r3, #4]
80014268:	e51b301c 	ldr	r3, [fp, #-28]
8001426c:	e5832004 	str	r2, [r3, #4]
80014270:	e51b3018 	ldr	r3, [fp, #-24]
80014274:	e5933008 	ldr	r3, [r3, #8]
80014278:	e1a00003 	mov	r0, r3
8001427c:	ebfffb7e 	bl	8001307c <kmfree>
80014280:	e51b0018 	ldr	r0, [fp, #-24]
80014284:	ebfffb7c 	bl	8001307c <kmfree>
80014288:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8001428c:	e1a00003 	mov	r0, r3
80014290:	e24bd010 	sub	sp, fp, #16
80014294:	e89da810 	ldm	sp, {r4, fp, sp, pc}
80014298:	0000bed4 	ldrdeq	fp, [r0], -r4
8001429c:	00000014 	andeq	r0, r0, r4, lsl r0

800142a0 <clearMessageQueue>:
800142a0:	e1a0c00d 	mov	ip, sp
800142a4:	e92dd800 	push	{fp, ip, lr, pc}
800142a8:	e24cb004 	sub	fp, ip, #4
800142ac:	e24dd010 	sub	sp, sp, #16
800142b0:	e50b0018 	str	r0, [fp, #-24]
800142b4:	e51b3018 	ldr	r3, [fp, #-24]
800142b8:	e5933000 	ldr	r3, [r3]
800142bc:	e50b3010 	str	r3, [fp, #-16]
800142c0:	ea00000a 	b	800142f0 <clearMessageQueue+0x50>
800142c4:	e51b3010 	ldr	r3, [fp, #-16]
800142c8:	e50b3014 	str	r3, [fp, #-20]
800142cc:	e51b3010 	ldr	r3, [fp, #-16]
800142d0:	e5933000 	ldr	r3, [r3]
800142d4:	e50b3010 	str	r3, [fp, #-16]
800142d8:	e51b3014 	ldr	r3, [fp, #-20]
800142dc:	e5933008 	ldr	r3, [r3, #8]
800142e0:	e1a00003 	mov	r0, r3
800142e4:	ebfffb64 	bl	8001307c <kmfree>
800142e8:	e51b0014 	ldr	r0, [fp, #-20]
800142ec:	ebfffb62 	bl	8001307c <kmfree>
800142f0:	e51b3010 	ldr	r3, [fp, #-16]
800142f4:	e3530000 	cmp	r3, #0
800142f8:	1afffff1 	bne	800142c4 <clearMessageQueue+0x24>
800142fc:	e51b3018 	ldr	r3, [fp, #-24]
80014300:	e3a02000 	mov	r2, #0
80014304:	e5832000 	str	r2, [r3]
80014308:	e51b3018 	ldr	r3, [fp, #-24]
8001430c:	e3a02000 	mov	r2, #0
80014310:	e5832004 	str	r2, [r3, #4]
80014314:	e24bd00c 	sub	sp, fp, #12
80014318:	e89da800 	ldm	sp, {fp, sp, pc}

8001431c <kfOpen>:
8001431c:	e1a0c00d 	mov	ip, sp
80014320:	e92dd800 	push	{fp, ip, lr, pc}
80014324:	e24cb004 	sub	fp, ip, #4
80014328:	e24dd010 	sub	sp, sp, #16
8001432c:	e50b0018 	str	r0, [fp, #-24]
80014330:	e3a00010 	mov	r0, #16
80014334:	ebfffb41 	bl	80013040 <kmalloc>
80014338:	e50b0010 	str	r0, [fp, #-16]
8001433c:	e51b3010 	ldr	r3, [fp, #-16]
80014340:	e3530000 	cmp	r3, #0
80014344:	1a000001 	bne	80014350 <kfOpen+0x34>
80014348:	e3a03000 	mov	r3, #0
8001434c:	ea00001f 	b	800143d0 <kfOpen+0xb4>
80014350:	e51b0010 	ldr	r0, [fp, #-16]
80014354:	e3a01000 	mov	r1, #0
80014358:	e3a02010 	mov	r2, #16
8001435c:	eb000355 	bl	800150b8 <memset>
80014360:	e59f3074 	ldr	r3, [pc, #116]	; 800143dc <kfOpen+0xc0>
80014364:	e08f3003 	add	r3, pc, r3
80014368:	e5933000 	ldr	r3, [r3]
8001436c:	e3530000 	cmp	r3, #0
80014370:	1a000004 	bne	80014388 <kfOpen+0x6c>
80014374:	e59f3064 	ldr	r3, [pc, #100]	; 800143e0 <kfOpen+0xc4>
80014378:	e08f3003 	add	r3, pc, r3
8001437c:	e51b2010 	ldr	r2, [fp, #-16]
80014380:	e5832000 	str	r2, [r3]
80014384:	ea00000d 	b	800143c0 <kfOpen+0xa4>
80014388:	e59f3054 	ldr	r3, [pc, #84]	; 800143e4 <kfOpen+0xc8>
8001438c:	e08f3003 	add	r3, pc, r3
80014390:	e5932000 	ldr	r2, [r3]
80014394:	e51b3010 	ldr	r3, [fp, #-16]
80014398:	e5832000 	str	r2, [r3]
8001439c:	e59f3044 	ldr	r3, [pc, #68]	; 800143e8 <kfOpen+0xcc>
800143a0:	e08f3003 	add	r3, pc, r3
800143a4:	e5933000 	ldr	r3, [r3]
800143a8:	e51b2010 	ldr	r2, [fp, #-16]
800143ac:	e5832004 	str	r2, [r3, #4]
800143b0:	e59f3034 	ldr	r3, [pc, #52]	; 800143ec <kfOpen+0xd0>
800143b4:	e08f3003 	add	r3, pc, r3
800143b8:	e51b2010 	ldr	r2, [fp, #-16]
800143bc:	e5832000 	str	r2, [r3]
800143c0:	e51b3010 	ldr	r3, [fp, #-16]
800143c4:	e51b2018 	ldr	r2, [fp, #-24]
800143c8:	e5832008 	str	r2, [r3, #8]
800143cc:	e51b3010 	ldr	r3, [fp, #-16]
800143d0:	e1a00003 	mov	r0, r3
800143d4:	e24bd00c 	sub	sp, fp, #12
800143d8:	e89da800 	ldm	sp, {fp, sp, pc}
800143dc:	002304a0 	eoreq	r0, r3, r0, lsr #9
800143e0:	0023048c 	eoreq	r0, r3, ip, lsl #9
800143e4:	00230478 	eoreq	r0, r3, r8, ror r4
800143e8:	00230464 	eoreq	r0, r3, r4, ror #8
800143ec:	00230450 	eoreq	r0, r3, r0, asr r4

800143f0 <kfUnref>:
800143f0:	e1a0c00d 	mov	ip, sp
800143f4:	e92dd800 	push	{fp, ip, lr, pc}
800143f8:	e24cb004 	sub	fp, ip, #4
800143fc:	e24dd008 	sub	sp, sp, #8
80014400:	e50b0010 	str	r0, [fp, #-16]
80014404:	e50b1014 	str	r1, [fp, #-20]
80014408:	e51b3010 	ldr	r3, [fp, #-16]
8001440c:	e3530000 	cmp	r3, #0
80014410:	1a000000 	bne	80014418 <kfUnref+0x28>
80014414:	ea00003f 	b	80014518 <kfUnref+0x128>
80014418:	e51b3014 	ldr	r3, [fp, #-20]
8001441c:	e2033001 	and	r3, r3, #1
80014420:	e3530000 	cmp	r3, #0
80014424:	1a00000a 	bne	80014454 <kfUnref+0x64>
80014428:	e51b3010 	ldr	r3, [fp, #-16]
8001442c:	e1d330bc 	ldrh	r3, [r3, #12]
80014430:	e3530000 	cmp	r3, #0
80014434:	0a000010 	beq	8001447c <kfUnref+0x8c>
80014438:	e51b3010 	ldr	r3, [fp, #-16]
8001443c:	e1d330bc 	ldrh	r3, [r3, #12]
80014440:	e2433001 	sub	r3, r3, #1
80014444:	e6ff2073 	uxth	r2, r3
80014448:	e51b3010 	ldr	r3, [fp, #-16]
8001444c:	e1c320bc 	strh	r2, [r3, #12]
80014450:	ea000009 	b	8001447c <kfUnref+0x8c>
80014454:	e51b3010 	ldr	r3, [fp, #-16]
80014458:	e1d330be 	ldrh	r3, [r3, #14]
8001445c:	e3530000 	cmp	r3, #0
80014460:	0a000005 	beq	8001447c <kfUnref+0x8c>
80014464:	e51b3010 	ldr	r3, [fp, #-16]
80014468:	e1d330be 	ldrh	r3, [r3, #14]
8001446c:	e2433001 	sub	r3, r3, #1
80014470:	e6ff2073 	uxth	r2, r3
80014474:	e51b3010 	ldr	r3, [fp, #-16]
80014478:	e1c320be 	strh	r2, [r3, #14]
8001447c:	e51b3010 	ldr	r3, [fp, #-16]
80014480:	e1d330be 	ldrh	r3, [r3, #14]
80014484:	e1a02003 	mov	r2, r3
80014488:	e51b3010 	ldr	r3, [fp, #-16]
8001448c:	e1d330bc 	ldrh	r3, [r3, #12]
80014490:	e0823003 	add	r3, r2, r3
80014494:	e3530000 	cmp	r3, #0
80014498:	1a00001e 	bne	80014518 <kfUnref+0x128>
8001449c:	e51b3010 	ldr	r3, [fp, #-16]
800144a0:	e5933004 	ldr	r3, [r3, #4]
800144a4:	e3530000 	cmp	r3, #0
800144a8:	0a000004 	beq	800144c0 <kfUnref+0xd0>
800144ac:	e51b3010 	ldr	r3, [fp, #-16]
800144b0:	e5933004 	ldr	r3, [r3, #4]
800144b4:	e51b2010 	ldr	r2, [fp, #-16]
800144b8:	e5922000 	ldr	r2, [r2]
800144bc:	e5832000 	str	r2, [r3]
800144c0:	e51b3010 	ldr	r3, [fp, #-16]
800144c4:	e5933000 	ldr	r3, [r3]
800144c8:	e3530000 	cmp	r3, #0
800144cc:	0a000004 	beq	800144e4 <kfUnref+0xf4>
800144d0:	e51b3010 	ldr	r3, [fp, #-16]
800144d4:	e5933000 	ldr	r3, [r3]
800144d8:	e51b2010 	ldr	r2, [fp, #-16]
800144dc:	e5922004 	ldr	r2, [r2, #4]
800144e0:	e5832004 	str	r2, [r3, #4]
800144e4:	e59f3034 	ldr	r3, [pc, #52]	; 80014520 <kfUnref+0x130>
800144e8:	e08f3003 	add	r3, pc, r3
800144ec:	e5933000 	ldr	r3, [r3]
800144f0:	e51b2010 	ldr	r2, [fp, #-16]
800144f4:	e1520003 	cmp	r2, r3
800144f8:	1a000004 	bne	80014510 <kfUnref+0x120>
800144fc:	e51b3010 	ldr	r3, [fp, #-16]
80014500:	e5932000 	ldr	r2, [r3]
80014504:	e59f3018 	ldr	r3, [pc, #24]	; 80014524 <kfUnref+0x134>
80014508:	e08f3003 	add	r3, pc, r3
8001450c:	e5832000 	str	r2, [r3]
80014510:	e51b0010 	ldr	r0, [fp, #-16]
80014514:	ebfffad8 	bl	8001307c <kmfree>
80014518:	e24bd00c 	sub	sp, fp, #12
8001451c:	e89da800 	ldm	sp, {fp, sp, pc}
80014520:	0023031c 	eoreq	r0, r3, ip, lsl r3
80014524:	002302fc 	strdeq	r0, [r3], -ip	; <UNPREDICTABLE>

80014528 <kfRef>:
80014528:	e1a0c00d 	mov	ip, sp
8001452c:	e92dd800 	push	{fp, ip, lr, pc}
80014530:	e24cb004 	sub	fp, ip, #4
80014534:	e24dd008 	sub	sp, sp, #8
80014538:	e50b0010 	str	r0, [fp, #-16]
8001453c:	e50b1014 	str	r1, [fp, #-20]
80014540:	e51b3010 	ldr	r3, [fp, #-16]
80014544:	e3530000 	cmp	r3, #0
80014548:	1a000000 	bne	80014550 <kfRef+0x28>
8001454c:	ea000010 	b	80014594 <kfRef+0x6c>
80014550:	e51b3014 	ldr	r3, [fp, #-20]
80014554:	e2033001 	and	r3, r3, #1
80014558:	e3530000 	cmp	r3, #0
8001455c:	1a000006 	bne	8001457c <kfRef+0x54>
80014560:	e51b3010 	ldr	r3, [fp, #-16]
80014564:	e1d330bc 	ldrh	r3, [r3, #12]
80014568:	e2833001 	add	r3, r3, #1
8001456c:	e6ff2073 	uxth	r2, r3
80014570:	e51b3010 	ldr	r3, [fp, #-16]
80014574:	e1c320bc 	strh	r2, [r3, #12]
80014578:	ea000005 	b	80014594 <kfRef+0x6c>
8001457c:	e51b3010 	ldr	r3, [fp, #-16]
80014580:	e1d330be 	ldrh	r3, [r3, #14]
80014584:	e2833001 	add	r3, r3, #1
80014588:	e6ff2073 	uxth	r2, r3
8001458c:	e51b3010 	ldr	r3, [fp, #-16]
80014590:	e1c320be 	strh	r2, [r3, #14]
80014594:	e24bd00c 	sub	sp, fp, #12
80014598:	e89da800 	ldm	sp, {fp, sp, pc}

8001459c <ramdiskClose>:
8001459c:	e1a0c00d 	mov	ip, sp
800145a0:	e92dd800 	push	{fp, ip, lr, pc}
800145a4:	e24cb004 	sub	fp, ip, #4
800145a8:	e24dd010 	sub	sp, sp, #16
800145ac:	e50b0018 	str	r0, [fp, #-24]
800145b0:	e50b101c 	str	r1, [fp, #-28]
800145b4:	e51b3018 	ldr	r3, [fp, #-24]
800145b8:	e5933000 	ldr	r3, [r3]
800145bc:	e50b3010 	str	r3, [fp, #-16]
800145c0:	ea000009 	b	800145ec <ramdiskClose+0x50>
800145c4:	e51b3010 	ldr	r3, [fp, #-16]
800145c8:	e5932000 	ldr	r2, [r3]
800145cc:	e51b3018 	ldr	r3, [fp, #-24]
800145d0:	e5832000 	str	r2, [r3]
800145d4:	e51b301c 	ldr	r3, [fp, #-28]
800145d8:	e51b0010 	ldr	r0, [fp, #-16]
800145dc:	e12fff33 	blx	r3
800145e0:	e51b3018 	ldr	r3, [fp, #-24]
800145e4:	e5933000 	ldr	r3, [r3]
800145e8:	e50b3010 	str	r3, [fp, #-16]
800145ec:	e51b3010 	ldr	r3, [fp, #-16]
800145f0:	e3530000 	cmp	r3, #0
800145f4:	1afffff2 	bne	800145c4 <ramdiskClose+0x28>
800145f8:	e51b3018 	ldr	r3, [fp, #-24]
800145fc:	e3a02000 	mov	r2, #0
80014600:	e5832000 	str	r2, [r3]
80014604:	e24bd00c 	sub	sp, fp, #12
80014608:	e89da800 	ldm	sp, {fp, sp, pc}

8001460c <ramdiskOpen>:
8001460c:	e1a0c00d 	mov	ip, sp
80014610:	e92dd800 	push	{fp, ip, lr, pc}
80014614:	e24cb004 	sub	fp, ip, #4
80014618:	e24dd018 	sub	sp, sp, #24
8001461c:	e50b0018 	str	r0, [fp, #-24]
80014620:	e50b101c 	str	r1, [fp, #-28]
80014624:	e50b2020 	str	r2, [fp, #-32]
80014628:	e51b301c 	ldr	r3, [fp, #-28]
8001462c:	e51b2018 	ldr	r2, [fp, #-24]
80014630:	e5832004 	str	r2, [r3, #4]
80014634:	e51b301c 	ldr	r3, [fp, #-28]
80014638:	e3a02000 	mov	r2, #0
8001463c:	e5832000 	str	r2, [r3]
80014640:	e24b3014 	sub	r3, fp, #20
80014644:	e1a00003 	mov	r0, r3
80014648:	e51b1018 	ldr	r1, [fp, #-24]
8001464c:	e3a02004 	mov	r2, #4
80014650:	eb000145 	bl	80014b6c <memcpy>
80014654:	e51b3014 	ldr	r3, [fp, #-20]
80014658:	e3530000 	cmp	r3, #0
8001465c:	0a000030 	beq	80014724 <ramdiskOpen+0x118>
80014660:	e51b3018 	ldr	r3, [fp, #-24]
80014664:	e2833004 	add	r3, r3, #4
80014668:	e50b3018 	str	r3, [fp, #-24]
8001466c:	e51b3020 	ldr	r3, [fp, #-32]
80014670:	e3a00f43 	mov	r0, #268	; 0x10c
80014674:	e12fff33 	blx	r3
80014678:	e50b0010 	str	r0, [fp, #-16]
8001467c:	e51b3010 	ldr	r3, [fp, #-16]
80014680:	e2833004 	add	r3, r3, #4
80014684:	e51b2014 	ldr	r2, [fp, #-20]
80014688:	e1a00003 	mov	r0, r3
8001468c:	e51b1018 	ldr	r1, [fp, #-24]
80014690:	eb000135 	bl	80014b6c <memcpy>
80014694:	e51b3014 	ldr	r3, [fp, #-20]
80014698:	e51b2010 	ldr	r2, [fp, #-16]
8001469c:	e0823003 	add	r3, r2, r3
800146a0:	e3a02000 	mov	r2, #0
800146a4:	e5c32004 	strb	r2, [r3, #4]
800146a8:	e51b3014 	ldr	r3, [fp, #-20]
800146ac:	e1a02003 	mov	r2, r3
800146b0:	e51b3018 	ldr	r3, [fp, #-24]
800146b4:	e0833002 	add	r3, r3, r2
800146b8:	e50b3018 	str	r3, [fp, #-24]
800146bc:	e51b3010 	ldr	r3, [fp, #-16]
800146c0:	e2833f42 	add	r3, r3, #264	; 0x108
800146c4:	e1a00003 	mov	r0, r3
800146c8:	e51b1018 	ldr	r1, [fp, #-24]
800146cc:	e3a02004 	mov	r2, #4
800146d0:	eb000125 	bl	80014b6c <memcpy>
800146d4:	e51b3018 	ldr	r3, [fp, #-24]
800146d8:	e2833004 	add	r3, r3, #4
800146dc:	e50b3018 	str	r3, [fp, #-24]
800146e0:	e51b3010 	ldr	r3, [fp, #-16]
800146e4:	e51b2018 	ldr	r2, [fp, #-24]
800146e8:	e5832104 	str	r2, [r3, #260]	; 0x104
800146ec:	e51b3010 	ldr	r3, [fp, #-16]
800146f0:	e5933108 	ldr	r3, [r3, #264]	; 0x108
800146f4:	e1a02003 	mov	r2, r3
800146f8:	e51b3018 	ldr	r3, [fp, #-24]
800146fc:	e0833002 	add	r3, r3, r2
80014700:	e50b3018 	str	r3, [fp, #-24]
80014704:	e51b301c 	ldr	r3, [fp, #-28]
80014708:	e5932000 	ldr	r2, [r3]
8001470c:	e51b3010 	ldr	r3, [fp, #-16]
80014710:	e5832000 	str	r2, [r3]
80014714:	e51b301c 	ldr	r3, [fp, #-28]
80014718:	e51b2010 	ldr	r2, [fp, #-16]
8001471c:	e5832000 	str	r2, [r3]
80014720:	eaffffc6 	b	80014640 <ramdiskOpen+0x34>
80014724:	e24bd00c 	sub	sp, fp, #12
80014728:	e89da800 	ldm	sp, {fp, sp, pc}

8001472c <ramdiskRead>:
8001472c:	e1a0c00d 	mov	ip, sp
80014730:	e92dd800 	push	{fp, ip, lr, pc}
80014734:	e24cb004 	sub	fp, ip, #4
80014738:	e24dd018 	sub	sp, sp, #24
8001473c:	e50b0018 	str	r0, [fp, #-24]
80014740:	e50b101c 	str	r1, [fp, #-28]
80014744:	e50b2020 	str	r2, [fp, #-32]
80014748:	e51b3018 	ldr	r3, [fp, #-24]
8001474c:	e3530000 	cmp	r3, #0
80014750:	1a000001 	bne	8001475c <ramdiskRead+0x30>
80014754:	e3a03000 	mov	r3, #0
80014758:	ea000019 	b	800147c4 <ramdiskRead+0x98>
8001475c:	e51b3018 	ldr	r3, [fp, #-24]
80014760:	e5933000 	ldr	r3, [r3]
80014764:	e50b3010 	str	r3, [fp, #-16]
80014768:	ea000011 	b	800147b4 <ramdiskRead+0x88>
8001476c:	e51b3010 	ldr	r3, [fp, #-16]
80014770:	e2833004 	add	r3, r3, #4
80014774:	e51b001c 	ldr	r0, [fp, #-28]
80014778:	e1a01003 	mov	r1, r3
8001477c:	eb00016b 	bl	80014d30 <strcmp>
80014780:	e1a03000 	mov	r3, r0
80014784:	e3530000 	cmp	r3, #0
80014788:	1a000006 	bne	800147a8 <ramdiskRead+0x7c>
8001478c:	e51b3010 	ldr	r3, [fp, #-16]
80014790:	e5932108 	ldr	r2, [r3, #264]	; 0x108
80014794:	e51b3020 	ldr	r3, [fp, #-32]
80014798:	e5832000 	str	r2, [r3]
8001479c:	e51b3010 	ldr	r3, [fp, #-16]
800147a0:	e5933104 	ldr	r3, [r3, #260]	; 0x104
800147a4:	ea000006 	b	800147c4 <ramdiskRead+0x98>
800147a8:	e51b3010 	ldr	r3, [fp, #-16]
800147ac:	e5933000 	ldr	r3, [r3]
800147b0:	e50b3010 	str	r3, [fp, #-16]
800147b4:	e51b3010 	ldr	r3, [fp, #-16]
800147b8:	e3530000 	cmp	r3, #0
800147bc:	1affffea 	bne	8001476c <ramdiskRead+0x40>
800147c0:	e3a03000 	mov	r3, #0
800147c4:	e1a00003 	mov	r0, r3
800147c8:	e24bd00c 	sub	sp, fp, #12
800147cc:	e89da800 	ldm	sp, {fp, sp, pc}

800147d0 <kservReg>:
800147d0:	e1a0c00d 	mov	ip, sp
800147d4:	e92dd800 	push	{fp, ip, lr, pc}
800147d8:	e24cb004 	sub	fp, ip, #4
800147dc:	e24dd010 	sub	sp, sp, #16
800147e0:	e50b0018 	str	r0, [fp, #-24]
800147e4:	e59f212c 	ldr	r2, [pc, #300]	; 80014918 <kservReg+0x148>
800147e8:	e08f2002 	add	r2, pc, r2
800147ec:	e59f3128 	ldr	r3, [pc, #296]	; 8001491c <kservReg+0x14c>
800147f0:	e7923003 	ldr	r3, [r2, r3]
800147f4:	e5933000 	ldr	r3, [r3]
800147f8:	e5933004 	ldr	r3, [r3, #4]
800147fc:	e50b3014 	str	r3, [fp, #-20]
80014800:	e3a03000 	mov	r3, #0
80014804:	e50b3010 	str	r3, [fp, #-16]
80014808:	ea000023 	b	8001489c <kservReg+0xcc>
8001480c:	e59f210c 	ldr	r2, [pc, #268]	; 80014920 <kservReg+0x150>
80014810:	e08f2002 	add	r2, pc, r2
80014814:	e51b3010 	ldr	r3, [fp, #-16]
80014818:	e3a01018 	mov	r1, #24
8001481c:	e0030391 	mul	r3, r1, r3
80014820:	e0823003 	add	r3, r2, r3
80014824:	e5d33000 	ldrb	r3, [r3]
80014828:	e3530000 	cmp	r3, #0
8001482c:	1a000000 	bne	80014834 <kservReg+0x64>
80014830:	ea00001c 	b	800148a8 <kservReg+0xd8>
80014834:	e51b3010 	ldr	r3, [fp, #-16]
80014838:	e3a02018 	mov	r2, #24
8001483c:	e0030392 	mul	r3, r2, r3
80014840:	e59f20dc 	ldr	r2, [pc, #220]	; 80014924 <kservReg+0x154>
80014844:	e08f2002 	add	r2, pc, r2
80014848:	e0833002 	add	r3, r3, r2
8001484c:	e1a00003 	mov	r0, r3
80014850:	e51b1018 	ldr	r1, [fp, #-24]
80014854:	eb000135 	bl	80014d30 <strcmp>
80014858:	e1a03000 	mov	r3, r0
8001485c:	e3530000 	cmp	r3, #0
80014860:	1a00000a 	bne	80014890 <kservReg+0xc0>
80014864:	e59f20bc 	ldr	r2, [pc, #188]	; 80014928 <kservReg+0x158>
80014868:	e08f2002 	add	r2, pc, r2
8001486c:	e51b3010 	ldr	r3, [fp, #-16]
80014870:	e3a01018 	mov	r1, #24
80014874:	e0030391 	mul	r3, r1, r3
80014878:	e0823003 	add	r3, r2, r3
8001487c:	e2833010 	add	r3, r3, #16
80014880:	e51b2014 	ldr	r2, [fp, #-20]
80014884:	e5832004 	str	r2, [r3, #4]
80014888:	e3a03000 	mov	r3, #0
8001488c:	ea00001e 	b	8001490c <kservReg+0x13c>
80014890:	e51b3010 	ldr	r3, [fp, #-16]
80014894:	e2833001 	add	r3, r3, #1
80014898:	e50b3010 	str	r3, [fp, #-16]
8001489c:	e51b3010 	ldr	r3, [fp, #-16]
800148a0:	e353001f 	cmp	r3, #31
800148a4:	daffffd8 	ble	8001480c <kservReg+0x3c>
800148a8:	e51b3010 	ldr	r3, [fp, #-16]
800148ac:	e353001f 	cmp	r3, #31
800148b0:	da000001 	ble	800148bc <kservReg+0xec>
800148b4:	e3e03000 	mvn	r3, #0
800148b8:	ea000013 	b	8001490c <kservReg+0x13c>
800148bc:	e51b3010 	ldr	r3, [fp, #-16]
800148c0:	e3a02018 	mov	r2, #24
800148c4:	e0030392 	mul	r3, r2, r3
800148c8:	e59f205c 	ldr	r2, [pc, #92]	; 8001492c <kservReg+0x15c>
800148cc:	e08f2002 	add	r2, pc, r2
800148d0:	e0833002 	add	r3, r3, r2
800148d4:	e1a00003 	mov	r0, r3
800148d8:	e51b1018 	ldr	r1, [fp, #-24]
800148dc:	e3a02010 	mov	r2, #16
800148e0:	eb0000e4 	bl	80014c78 <strncpy>
800148e4:	e59f2044 	ldr	r2, [pc, #68]	; 80014930 <kservReg+0x160>
800148e8:	e08f2002 	add	r2, pc, r2
800148ec:	e51b3010 	ldr	r3, [fp, #-16]
800148f0:	e3a01018 	mov	r1, #24
800148f4:	e0030391 	mul	r3, r1, r3
800148f8:	e0823003 	add	r3, r2, r3
800148fc:	e2833010 	add	r3, r3, #16
80014900:	e51b2014 	ldr	r2, [fp, #-20]
80014904:	e5832004 	str	r2, [r3, #4]
80014908:	e3a03000 	mov	r3, #0
8001490c:	e1a00003 	mov	r0, r3
80014910:	e24bd00c 	sub	sp, fp, #12
80014914:	e89da800 	ldm	sp, {fp, sp, pc}
80014918:	0000b814 	andeq	fp, r0, r4, lsl r8
8001491c:	00000014 	andeq	r0, r0, r4, lsl r0
80014920:	0022fff8 	strdeq	pc, [r2], -r8	; <UNPREDICTABLE>
80014924:	0022ffc4 	eoreq	pc, r2, r4, asr #31
80014928:	0022ffa0 	eoreq	pc, r2, r0, lsr #31
8001492c:	0022ff3c 	eoreq	pc, r2, ip, lsr pc	; <UNPREDICTABLE>
80014930:	0022ff20 	eoreq	pc, r2, r0, lsr #30

80014934 <kservGet>:
80014934:	e1a0c00d 	mov	ip, sp
80014938:	e92dd800 	push	{fp, ip, lr, pc}
8001493c:	e24cb004 	sub	fp, ip, #4
80014940:	e24dd010 	sub	sp, sp, #16
80014944:	e50b0018 	str	r0, [fp, #-24]
80014948:	e3a03000 	mov	r3, #0
8001494c:	e50b3010 	str	r3, [fp, #-16]
80014950:	ea000021 	b	800149dc <kservGet+0xa8>
80014954:	e59f209c 	ldr	r2, [pc, #156]	; 800149f8 <kservGet+0xc4>
80014958:	e08f2002 	add	r2, pc, r2
8001495c:	e51b3010 	ldr	r3, [fp, #-16]
80014960:	e3a01018 	mov	r1, #24
80014964:	e0030391 	mul	r3, r1, r3
80014968:	e0823003 	add	r3, r2, r3
8001496c:	e5d33000 	ldrb	r3, [r3]
80014970:	e3530000 	cmp	r3, #0
80014974:	1a000000 	bne	8001497c <kservGet+0x48>
80014978:	ea00001a 	b	800149e8 <kservGet+0xb4>
8001497c:	e51b3010 	ldr	r3, [fp, #-16]
80014980:	e3a02018 	mov	r2, #24
80014984:	e0030392 	mul	r3, r2, r3
80014988:	e59f206c 	ldr	r2, [pc, #108]	; 800149fc <kservGet+0xc8>
8001498c:	e08f2002 	add	r2, pc, r2
80014990:	e0833002 	add	r3, r3, r2
80014994:	e1a00003 	mov	r0, r3
80014998:	e51b1018 	ldr	r1, [fp, #-24]
8001499c:	eb0000e3 	bl	80014d30 <strcmp>
800149a0:	e1a03000 	mov	r3, r0
800149a4:	e3530000 	cmp	r3, #0
800149a8:	1a000008 	bne	800149d0 <kservGet+0x9c>
800149ac:	e59f204c 	ldr	r2, [pc, #76]	; 80014a00 <kservGet+0xcc>
800149b0:	e08f2002 	add	r2, pc, r2
800149b4:	e51b3010 	ldr	r3, [fp, #-16]
800149b8:	e3a01018 	mov	r1, #24
800149bc:	e0030391 	mul	r3, r1, r3
800149c0:	e0823003 	add	r3, r2, r3
800149c4:	e2833010 	add	r3, r3, #16
800149c8:	e5933004 	ldr	r3, [r3, #4]
800149cc:	ea000006 	b	800149ec <kservGet+0xb8>
800149d0:	e51b3010 	ldr	r3, [fp, #-16]
800149d4:	e2833001 	add	r3, r3, #1
800149d8:	e50b3010 	str	r3, [fp, #-16]
800149dc:	e51b3010 	ldr	r3, [fp, #-16]
800149e0:	e353001f 	cmp	r3, #31
800149e4:	daffffda 	ble	80014954 <kservGet+0x20>
800149e8:	e3e03000 	mvn	r3, #0
800149ec:	e1a00003 	mov	r0, r3
800149f0:	e24bd00c 	sub	sp, fp, #12
800149f4:	e89da800 	ldm	sp, {fp, sp, pc}
800149f8:	0022feb0 	strhteq	pc, [r2], -r0	; <UNPREDICTABLE>
800149fc:	0022fe7c 	eoreq	pc, r2, ip, ror lr	; <UNPREDICTABLE>
80014a00:	0022fe58 	eoreq	pc, r2, r8, asr lr	; <UNPREDICTABLE>

80014a04 <schedule>:
80014a04:	e1a0c00d 	mov	ip, sp
80014a08:	e92dd810 	push	{r4, fp, ip, lr, pc}
80014a0c:	e24cb004 	sub	fp, ip, #4
80014a10:	e24dd00c 	sub	sp, sp, #12
80014a14:	e59f40bc 	ldr	r4, [pc, #188]	; 80014ad8 <schedule+0xd4>
80014a18:	e08f4004 	add	r4, pc, r4
80014a1c:	e3a03000 	mov	r3, #0
80014a20:	e50b3018 	str	r3, [fp, #-24]
80014a24:	ea000027 	b	80014ac8 <schedule+0xc4>
80014a28:	e3a03000 	mov	r3, #0
80014a2c:	e50b301c 	str	r3, [fp, #-28]
80014a30:	e59f30a4 	ldr	r3, [pc, #164]	; 80014adc <schedule+0xd8>
80014a34:	e08f3003 	add	r3, pc, r3
80014a38:	e5933000 	ldr	r3, [r3]
80014a3c:	e2832001 	add	r2, r3, #1
80014a40:	e59f3098 	ldr	r3, [pc, #152]	; 80014ae0 <schedule+0xdc>
80014a44:	e08f3003 	add	r3, pc, r3
80014a48:	e5832000 	str	r2, [r3]
80014a4c:	e59f3090 	ldr	r3, [pc, #144]	; 80014ae4 <schedule+0xe0>
80014a50:	e08f3003 	add	r3, pc, r3
80014a54:	e5933000 	ldr	r3, [r3]
80014a58:	e3530080 	cmp	r3, #128	; 0x80
80014a5c:	1a000003 	bne	80014a70 <schedule+0x6c>
80014a60:	e59f3080 	ldr	r3, [pc, #128]	; 80014ae8 <schedule+0xe4>
80014a64:	e08f3003 	add	r3, pc, r3
80014a68:	e3a02000 	mov	r2, #0
80014a6c:	e5832000 	str	r2, [r3]
80014a70:	e59f3074 	ldr	r3, [pc, #116]	; 80014aec <schedule+0xe8>
80014a74:	e08f3003 	add	r3, pc, r3
80014a78:	e5933000 	ldr	r3, [r3]
80014a7c:	e3a02e39 	mov	r2, #912	; 0x390
80014a80:	e0030392 	mul	r3, r2, r3
80014a84:	e59f2064 	ldr	r2, [pc, #100]	; 80014af0 <schedule+0xec>
80014a88:	e7942002 	ldr	r2, [r4, r2]
80014a8c:	e0833002 	add	r3, r3, r2
80014a90:	e50b301c 	str	r3, [fp, #-28]
80014a94:	e51b301c 	ldr	r3, [fp, #-28]
80014a98:	e5d33000 	ldrb	r3, [r3]
80014a9c:	e3530003 	cmp	r3, #3
80014aa0:	1a000005 	bne	80014abc <schedule+0xb8>
80014aa4:	e59f3048 	ldr	r3, [pc, #72]	; 80014af4 <schedule+0xf0>
80014aa8:	e7943003 	ldr	r3, [r4, r3]
80014aac:	e51b201c 	ldr	r2, [fp, #-28]
80014ab0:	e5832000 	str	r2, [r3]
80014ab4:	e51b001c 	ldr	r0, [fp, #-28]
80014ab8:	ebfffbd0 	bl	80013a00 <procStart>
80014abc:	e51b3018 	ldr	r3, [fp, #-24]
80014ac0:	e2833001 	add	r3, r3, #1
80014ac4:	e50b3018 	str	r3, [fp, #-24]
80014ac8:	e51b3018 	ldr	r3, [fp, #-24]
80014acc:	e353007f 	cmp	r3, #127	; 0x7f
80014ad0:	daffffd4 	ble	80014a28 <schedule+0x24>
80014ad4:	eafffffe 	b	80014ad4 <schedule+0xd0>
80014ad8:	0000b5e4 	andeq	fp, r0, r4, ror #11
80014adc:	002300d4 	ldrdeq	r0, [r3], -r4	; <UNPREDICTABLE>
80014ae0:	002300c4 	eoreq	r0, r3, r4, asr #1
80014ae4:	002300b8 	strhteq	r0, [r3], -r8
80014ae8:	002300a4 	eoreq	r0, r3, r4, lsr #1
80014aec:	00230094 	mlaeq	r3, r4, r0, r0
80014af0:	00000024 	andeq	r0, r0, r4, lsr #32
80014af4:	00000014 	andeq	r0, r0, r4, lsl r0

80014af8 <handleTimer>:
80014af8:	e1a0c00d 	mov	ip, sp
80014afc:	e92dd800 	push	{fp, ip, lr, pc}
80014b00:	e24cb004 	sub	fp, ip, #4
80014b04:	ebffeedb 	bl	80010678 <timerClearInterrupt>
80014b08:	ebffffbd 	bl	80014a04 <schedule>
80014b0c:	e89da800 	ldm	sp, {fp, sp, pc}

80014b10 <schedulerInit>:
80014b10:	e1a0c00d 	mov	ip, sp
80014b14:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80014b18:	e24cb004 	sub	fp, ip, #4
80014b1c:	e59f403c 	ldr	r4, [pc, #60]	; 80014b60 <schedulerInit+0x50>
80014b20:	e08f4004 	add	r4, pc, r4
80014b24:	e59f3038 	ldr	r3, [pc, #56]	; 80014b64 <schedulerInit+0x54>
80014b28:	e08f3003 	add	r3, pc, r3
80014b2c:	e3a02000 	mov	r2, #0
80014b30:	e5832000 	str	r2, [r3]
80014b34:	e30806a0 	movw	r0, #34464	; 0x86a0
80014b38:	e3400001 	movt	r0, #1
80014b3c:	ebffeeb3 	bl	80010610 <timerSetInterval>
80014b40:	ebffef09 	bl	8001076c <getTimerIrq>
80014b44:	e1a03000 	mov	r3, r0
80014b48:	e1a00003 	mov	r0, r3
80014b4c:	e59f3014 	ldr	r3, [pc, #20]	; 80014b68 <schedulerInit+0x58>
80014b50:	e7943003 	ldr	r3, [r4, r3]
80014b54:	e1a01003 	mov	r1, r3
80014b58:	ebfff099 	bl	80010dc4 <registerInterruptHandler>
80014b5c:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
80014b60:	0000b4dc 	ldrdeq	fp, [r0], -ip
80014b64:	0022ffe0 	eoreq	pc, r2, r0, ror #31
80014b68:	00000004 	andeq	r0, r0, r4

80014b6c <memcpy>:
80014b6c:	e1a0c00d 	mov	ip, sp
80014b70:	e92dd800 	push	{fp, ip, lr, pc}
80014b74:	e24cb004 	sub	fp, ip, #4
80014b78:	e24dd020 	sub	sp, sp, #32
80014b7c:	e50b0020 	str	r0, [fp, #-32]
80014b80:	e50b1024 	str	r1, [fp, #-36]	; 0x24
80014b84:	e50b2028 	str	r2, [fp, #-40]	; 0x28
80014b88:	e51b3020 	ldr	r3, [fp, #-32]
80014b8c:	e50b3014 	str	r3, [fp, #-20]
80014b90:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
80014b94:	e50b3018 	str	r3, [fp, #-24]
80014b98:	e3a03000 	mov	r3, #0
80014b9c:	e50b3010 	str	r3, [fp, #-16]
80014ba0:	e3a03000 	mov	r3, #0
80014ba4:	e50b3010 	str	r3, [fp, #-16]
80014ba8:	ea00000a 	b	80014bd8 <memcpy+0x6c>
80014bac:	e51b2014 	ldr	r2, [fp, #-20]
80014bb0:	e51b3010 	ldr	r3, [fp, #-16]
80014bb4:	e0823003 	add	r3, r2, r3
80014bb8:	e51b1018 	ldr	r1, [fp, #-24]
80014bbc:	e51b2010 	ldr	r2, [fp, #-16]
80014bc0:	e0812002 	add	r2, r1, r2
80014bc4:	e5d22000 	ldrb	r2, [r2]
80014bc8:	e5c32000 	strb	r2, [r3]
80014bcc:	e51b3010 	ldr	r3, [fp, #-16]
80014bd0:	e2833001 	add	r3, r3, #1
80014bd4:	e50b3010 	str	r3, [fp, #-16]
80014bd8:	e51b2010 	ldr	r2, [fp, #-16]
80014bdc:	e51b3028 	ldr	r3, [fp, #-40]	; 0x28
80014be0:	e1520003 	cmp	r2, r3
80014be4:	3afffff0 	bcc	80014bac <memcpy+0x40>
80014be8:	e51b3020 	ldr	r3, [fp, #-32]
80014bec:	e1a00003 	mov	r0, r3
80014bf0:	e24bd00c 	sub	sp, fp, #12
80014bf4:	e89d6800 	ldm	sp, {fp, sp, lr}
80014bf8:	e12fff1e 	bx	lr

80014bfc <strcpy>:
80014bfc:	e1a0c00d 	mov	ip, sp
80014c00:	e92dd800 	push	{fp, ip, lr, pc}
80014c04:	e24cb004 	sub	fp, ip, #4
80014c08:	e24dd010 	sub	sp, sp, #16
80014c0c:	e50b0018 	str	r0, [fp, #-24]
80014c10:	e50b101c 	str	r1, [fp, #-28]
80014c14:	e51b3018 	ldr	r3, [fp, #-24]
80014c18:	e50b3010 	str	r3, [fp, #-16]
80014c1c:	ea000009 	b	80014c48 <strcpy+0x4c>
80014c20:	e51b301c 	ldr	r3, [fp, #-28]
80014c24:	e5d32000 	ldrb	r2, [r3]
80014c28:	e51b3018 	ldr	r3, [fp, #-24]
80014c2c:	e5c32000 	strb	r2, [r3]
80014c30:	e51b3018 	ldr	r3, [fp, #-24]
80014c34:	e2833001 	add	r3, r3, #1
80014c38:	e50b3018 	str	r3, [fp, #-24]
80014c3c:	e51b301c 	ldr	r3, [fp, #-28]
80014c40:	e2833001 	add	r3, r3, #1
80014c44:	e50b301c 	str	r3, [fp, #-28]
80014c48:	e51b301c 	ldr	r3, [fp, #-28]
80014c4c:	e5d33000 	ldrb	r3, [r3]
80014c50:	e3530000 	cmp	r3, #0
80014c54:	1afffff1 	bne	80014c20 <strcpy+0x24>
80014c58:	e51b3018 	ldr	r3, [fp, #-24]
80014c5c:	e3a02000 	mov	r2, #0
80014c60:	e5c32000 	strb	r2, [r3]
80014c64:	e51b3010 	ldr	r3, [fp, #-16]
80014c68:	e1a00003 	mov	r0, r3
80014c6c:	e24bd00c 	sub	sp, fp, #12
80014c70:	e89d6800 	ldm	sp, {fp, sp, lr}
80014c74:	e12fff1e 	bx	lr

80014c78 <strncpy>:
80014c78:	e1a0c00d 	mov	ip, sp
80014c7c:	e92dd800 	push	{fp, ip, lr, pc}
80014c80:	e24cb004 	sub	fp, ip, #4
80014c84:	e24dd018 	sub	sp, sp, #24
80014c88:	e50b0018 	str	r0, [fp, #-24]
80014c8c:	e50b101c 	str	r1, [fp, #-28]
80014c90:	e50b2020 	str	r2, [fp, #-32]
80014c94:	e3a03000 	mov	r3, #0
80014c98:	e50b3014 	str	r3, [fp, #-20]
80014c9c:	e3a03000 	mov	r3, #0
80014ca0:	e50b3010 	str	r3, [fp, #-16]
80014ca4:	ea00000a 	b	80014cd4 <strncpy+0x5c>
80014ca8:	e51b2018 	ldr	r2, [fp, #-24]
80014cac:	e51b3010 	ldr	r3, [fp, #-16]
80014cb0:	e0823003 	add	r3, r2, r3
80014cb4:	e51b101c 	ldr	r1, [fp, #-28]
80014cb8:	e51b2010 	ldr	r2, [fp, #-16]
80014cbc:	e0812002 	add	r2, r1, r2
80014cc0:	e5d22000 	ldrb	r2, [r2]
80014cc4:	e5c32000 	strb	r2, [r3]
80014cc8:	e51b3010 	ldr	r3, [fp, #-16]
80014ccc:	e2833001 	add	r3, r3, #1
80014cd0:	e50b3010 	str	r3, [fp, #-16]
80014cd4:	e51b2010 	ldr	r2, [fp, #-16]
80014cd8:	e51b3020 	ldr	r3, [fp, #-32]
80014cdc:	e1520003 	cmp	r2, r3
80014ce0:	2a000005 	bcs	80014cfc <strncpy+0x84>
80014ce4:	e51b201c 	ldr	r2, [fp, #-28]
80014ce8:	e51b3010 	ldr	r3, [fp, #-16]
80014cec:	e0823003 	add	r3, r2, r3
80014cf0:	e5d33000 	ldrb	r3, [r3]
80014cf4:	e3530000 	cmp	r3, #0
80014cf8:	1affffea 	bne	80014ca8 <strncpy+0x30>
80014cfc:	e51b2018 	ldr	r2, [fp, #-24]
80014d00:	e51b3010 	ldr	r3, [fp, #-16]
80014d04:	e0823003 	add	r3, r2, r3
80014d08:	e3a02000 	mov	r2, #0
80014d0c:	e5c32000 	strb	r2, [r3]
80014d10:	e51b001c 	ldr	r0, [fp, #-28]
80014d14:	eb000107 	bl	80015138 <strlen>
80014d18:	e50b0014 	str	r0, [fp, #-20]
80014d1c:	e51b3014 	ldr	r3, [fp, #-20]
80014d20:	e1a00003 	mov	r0, r3
80014d24:	e24bd00c 	sub	sp, fp, #12
80014d28:	e89d6800 	ldm	sp, {fp, sp, lr}
80014d2c:	e12fff1e 	bx	lr

80014d30 <strcmp>:
80014d30:	e1a0c00d 	mov	ip, sp
80014d34:	e92dd800 	push	{fp, ip, lr, pc}
80014d38:	e24cb004 	sub	fp, ip, #4
80014d3c:	e24dd008 	sub	sp, sp, #8
80014d40:	e50b0010 	str	r0, [fp, #-16]
80014d44:	e50b1014 	str	r1, [fp, #-20]
80014d48:	ea000005 	b	80014d64 <strcmp+0x34>
80014d4c:	e51b3010 	ldr	r3, [fp, #-16]
80014d50:	e2833001 	add	r3, r3, #1
80014d54:	e50b3010 	str	r3, [fp, #-16]
80014d58:	e51b3014 	ldr	r3, [fp, #-20]
80014d5c:	e2833001 	add	r3, r3, #1
80014d60:	e50b3014 	str	r3, [fp, #-20]
80014d64:	e51b3010 	ldr	r3, [fp, #-16]
80014d68:	e5d32000 	ldrb	r2, [r3]
80014d6c:	e51b3014 	ldr	r3, [fp, #-20]
80014d70:	e5d33000 	ldrb	r3, [r3]
80014d74:	e1520003 	cmp	r2, r3
80014d78:	1a000007 	bne	80014d9c <strcmp+0x6c>
80014d7c:	e51b3010 	ldr	r3, [fp, #-16]
80014d80:	e5d33000 	ldrb	r3, [r3]
80014d84:	e3530000 	cmp	r3, #0
80014d88:	0a000003 	beq	80014d9c <strcmp+0x6c>
80014d8c:	e51b3014 	ldr	r3, [fp, #-20]
80014d90:	e5d33000 	ldrb	r3, [r3]
80014d94:	e3530000 	cmp	r3, #0
80014d98:	1affffeb 	bne	80014d4c <strcmp+0x1c>
80014d9c:	e51b3010 	ldr	r3, [fp, #-16]
80014da0:	e5d33000 	ldrb	r3, [r3]
80014da4:	e1a02003 	mov	r2, r3
80014da8:	e51b3014 	ldr	r3, [fp, #-20]
80014dac:	e5d33000 	ldrb	r3, [r3]
80014db0:	e0633002 	rsb	r3, r3, r2
80014db4:	e1a00003 	mov	r0, r3
80014db8:	e24bd00c 	sub	sp, fp, #12
80014dbc:	e89d6800 	ldm	sp, {fp, sp, lr}
80014dc0:	e12fff1e 	bx	lr

80014dc4 <strncmp>:
80014dc4:	e1a0c00d 	mov	ip, sp
80014dc8:	e92dd800 	push	{fp, ip, lr, pc}
80014dcc:	e24cb004 	sub	fp, ip, #4
80014dd0:	e24dd018 	sub	sp, sp, #24
80014dd4:	e50b0018 	str	r0, [fp, #-24]
80014dd8:	e50b101c 	str	r1, [fp, #-28]
80014ddc:	e50b2020 	str	r2, [fp, #-32]
80014de0:	e3a03000 	mov	r3, #0
80014de4:	e50b3010 	str	r3, [fp, #-16]
80014de8:	e51b3020 	ldr	r3, [fp, #-32]
80014dec:	e3530000 	cmp	r3, #0
80014df0:	1a000001 	bne	80014dfc <strncmp+0x38>
80014df4:	e3a03000 	mov	r3, #0
80014df8:	ea000022 	b	80014e88 <strncmp+0xc4>
80014dfc:	ea000008 	b	80014e24 <strncmp+0x60>
80014e00:	e51b3018 	ldr	r3, [fp, #-24]
80014e04:	e2833001 	add	r3, r3, #1
80014e08:	e50b3018 	str	r3, [fp, #-24]
80014e0c:	e51b301c 	ldr	r3, [fp, #-28]
80014e10:	e2833001 	add	r3, r3, #1
80014e14:	e50b301c 	str	r3, [fp, #-28]
80014e18:	e51b3010 	ldr	r3, [fp, #-16]
80014e1c:	e2833001 	add	r3, r3, #1
80014e20:	e50b3010 	str	r3, [fp, #-16]
80014e24:	e51b3018 	ldr	r3, [fp, #-24]
80014e28:	e5d32000 	ldrb	r2, [r3]
80014e2c:	e51b301c 	ldr	r3, [fp, #-28]
80014e30:	e5d33000 	ldrb	r3, [r3]
80014e34:	e1520003 	cmp	r2, r3
80014e38:	1a00000c 	bne	80014e70 <strncmp+0xac>
80014e3c:	e51b3018 	ldr	r3, [fp, #-24]
80014e40:	e5d33000 	ldrb	r3, [r3]
80014e44:	e3530000 	cmp	r3, #0
80014e48:	0a000008 	beq	80014e70 <strncmp+0xac>
80014e4c:	e51b301c 	ldr	r3, [fp, #-28]
80014e50:	e5d33000 	ldrb	r3, [r3]
80014e54:	e3530000 	cmp	r3, #0
80014e58:	0a000004 	beq	80014e70 <strncmp+0xac>
80014e5c:	e51b3020 	ldr	r3, [fp, #-32]
80014e60:	e2432001 	sub	r2, r3, #1
80014e64:	e51b3010 	ldr	r3, [fp, #-16]
80014e68:	e1520003 	cmp	r2, r3
80014e6c:	8affffe3 	bhi	80014e00 <strncmp+0x3c>
80014e70:	e51b3018 	ldr	r3, [fp, #-24]
80014e74:	e5d33000 	ldrb	r3, [r3]
80014e78:	e1a02003 	mov	r2, r3
80014e7c:	e51b301c 	ldr	r3, [fp, #-28]
80014e80:	e5d33000 	ldrb	r3, [r3]
80014e84:	e0633002 	rsb	r3, r3, r2
80014e88:	e1a00003 	mov	r0, r3
80014e8c:	e24bd00c 	sub	sp, fp, #12
80014e90:	e89d6800 	ldm	sp, {fp, sp, lr}
80014e94:	e12fff1e 	bx	lr

80014e98 <strchr>:
80014e98:	e1a0c00d 	mov	ip, sp
80014e9c:	e92dd800 	push	{fp, ip, lr, pc}
80014ea0:	e24cb004 	sub	fp, ip, #4
80014ea4:	e24dd008 	sub	sp, sp, #8
80014ea8:	e50b0010 	str	r0, [fp, #-16]
80014eac:	e50b1014 	str	r1, [fp, #-20]
80014eb0:	ea000002 	b	80014ec0 <strchr+0x28>
80014eb4:	e51b3010 	ldr	r3, [fp, #-16]
80014eb8:	e2833001 	add	r3, r3, #1
80014ebc:	e50b3010 	str	r3, [fp, #-16]
80014ec0:	e51b3010 	ldr	r3, [fp, #-16]
80014ec4:	e5d33000 	ldrb	r3, [r3]
80014ec8:	e3530000 	cmp	r3, #0
80014ecc:	0a000005 	beq	80014ee8 <strchr+0x50>
80014ed0:	e51b3010 	ldr	r3, [fp, #-16]
80014ed4:	e5d33000 	ldrb	r3, [r3]
80014ed8:	e1a02003 	mov	r2, r3
80014edc:	e51b3014 	ldr	r3, [fp, #-20]
80014ee0:	e1520003 	cmp	r2, r3
80014ee4:	1afffff2 	bne	80014eb4 <strchr+0x1c>
80014ee8:	e51b3010 	ldr	r3, [fp, #-16]
80014eec:	e5d33000 	ldrb	r3, [r3]
80014ef0:	e1a02003 	mov	r2, r3
80014ef4:	e51b3014 	ldr	r3, [fp, #-20]
80014ef8:	e1520003 	cmp	r2, r3
80014efc:	1a000001 	bne	80014f08 <strchr+0x70>
80014f00:	e51b3010 	ldr	r3, [fp, #-16]
80014f04:	ea000000 	b	80014f0c <strchr+0x74>
80014f08:	e3a03000 	mov	r3, #0
80014f0c:	e1a00003 	mov	r0, r3
80014f10:	e24bd00c 	sub	sp, fp, #12
80014f14:	e89d6800 	ldm	sp, {fp, sp, lr}
80014f18:	e12fff1e 	bx	lr

80014f1c <strtok>:
80014f1c:	e1a0c00d 	mov	ip, sp
80014f20:	e92dd800 	push	{fp, ip, lr, pc}
80014f24:	e24cb004 	sub	fp, ip, #4
80014f28:	e24dd010 	sub	sp, sp, #16
80014f2c:	e50b0018 	str	r0, [fp, #-24]
80014f30:	e50b101c 	str	r1, [fp, #-28]
80014f34:	e3a03000 	mov	r3, #0
80014f38:	e50b3010 	str	r3, [fp, #-16]
80014f3c:	e51b3018 	ldr	r3, [fp, #-24]
80014f40:	e3530000 	cmp	r3, #0
80014f44:	0a000003 	beq	80014f58 <strtok+0x3c>
80014f48:	e59f313c 	ldr	r3, [pc, #316]	; 8001508c <strtok+0x170>
80014f4c:	e08f3003 	add	r3, pc, r3
80014f50:	e51b2018 	ldr	r2, [fp, #-24]
80014f54:	e5832000 	str	r2, [r3]
80014f58:	e59f3130 	ldr	r3, [pc, #304]	; 80015090 <strtok+0x174>
80014f5c:	e08f3003 	add	r3, pc, r3
80014f60:	e5933000 	ldr	r3, [r3]
80014f64:	e50b3010 	str	r3, [fp, #-16]
80014f68:	ea000002 	b	80014f78 <strtok+0x5c>
80014f6c:	e51b3010 	ldr	r3, [fp, #-16]
80014f70:	e2833001 	add	r3, r3, #1
80014f74:	e50b3010 	str	r3, [fp, #-16]
80014f78:	e51b3010 	ldr	r3, [fp, #-16]
80014f7c:	e5d33000 	ldrb	r3, [r3]
80014f80:	e3530000 	cmp	r3, #0
80014f84:	0a000007 	beq	80014fa8 <strtok+0x8c>
80014f88:	e51b3010 	ldr	r3, [fp, #-16]
80014f8c:	e5d33000 	ldrb	r3, [r3]
80014f90:	e51b001c 	ldr	r0, [fp, #-28]
80014f94:	e1a01003 	mov	r1, r3
80014f98:	ebffffbe 	bl	80014e98 <strchr>
80014f9c:	e1a03000 	mov	r3, r0
80014fa0:	e3530000 	cmp	r3, #0
80014fa4:	1afffff0 	bne	80014f6c <strtok+0x50>
80014fa8:	e51b3010 	ldr	r3, [fp, #-16]
80014fac:	e5d33000 	ldrb	r3, [r3]
80014fb0:	e3530000 	cmp	r3, #0
80014fb4:	1a000005 	bne	80014fd0 <strtok+0xb4>
80014fb8:	e59f30d4 	ldr	r3, [pc, #212]	; 80015094 <strtok+0x178>
80014fbc:	e08f3003 	add	r3, pc, r3
80014fc0:	e3a02000 	mov	r2, #0
80014fc4:	e5832000 	str	r2, [r3]
80014fc8:	e3a03000 	mov	r3, #0
80014fcc:	ea00002a 	b	8001507c <strtok+0x160>
80014fd0:	ea000006 	b	80014ff0 <strtok+0xd4>
80014fd4:	e59f30bc 	ldr	r3, [pc, #188]	; 80015098 <strtok+0x17c>
80014fd8:	e08f3003 	add	r3, pc, r3
80014fdc:	e5933000 	ldr	r3, [r3]
80014fe0:	e2832001 	add	r2, r3, #1
80014fe4:	e59f30b0 	ldr	r3, [pc, #176]	; 8001509c <strtok+0x180>
80014fe8:	e08f3003 	add	r3, pc, r3
80014fec:	e5832000 	str	r2, [r3]
80014ff0:	e59f30a8 	ldr	r3, [pc, #168]	; 800150a0 <strtok+0x184>
80014ff4:	e08f3003 	add	r3, pc, r3
80014ff8:	e5933000 	ldr	r3, [r3]
80014ffc:	e5d33000 	ldrb	r3, [r3]
80015000:	e3530000 	cmp	r3, #0
80015004:	0a000009 	beq	80015030 <strtok+0x114>
80015008:	e59f3094 	ldr	r3, [pc, #148]	; 800150a4 <strtok+0x188>
8001500c:	e08f3003 	add	r3, pc, r3
80015010:	e5933000 	ldr	r3, [r3]
80015014:	e5d33000 	ldrb	r3, [r3]
80015018:	e51b001c 	ldr	r0, [fp, #-28]
8001501c:	e1a01003 	mov	r1, r3
80015020:	ebffff9c 	bl	80014e98 <strchr>
80015024:	e1a03000 	mov	r3, r0
80015028:	e3530000 	cmp	r3, #0
8001502c:	0affffe8 	beq	80014fd4 <strtok+0xb8>
80015030:	e59f3070 	ldr	r3, [pc, #112]	; 800150a8 <strtok+0x18c>
80015034:	e08f3003 	add	r3, pc, r3
80015038:	e5933000 	ldr	r3, [r3]
8001503c:	e5d33000 	ldrb	r3, [r3]
80015040:	e3530000 	cmp	r3, #0
80015044:	0a00000b 	beq	80015078 <strtok+0x15c>
80015048:	e59f305c 	ldr	r3, [pc, #92]	; 800150ac <strtok+0x190>
8001504c:	e08f3003 	add	r3, pc, r3
80015050:	e5933000 	ldr	r3, [r3]
80015054:	e3a02000 	mov	r2, #0
80015058:	e5c32000 	strb	r2, [r3]
8001505c:	e59f304c 	ldr	r3, [pc, #76]	; 800150b0 <strtok+0x194>
80015060:	e08f3003 	add	r3, pc, r3
80015064:	e5933000 	ldr	r3, [r3]
80015068:	e2832001 	add	r2, r3, #1
8001506c:	e59f3040 	ldr	r3, [pc, #64]	; 800150b4 <strtok+0x198>
80015070:	e08f3003 	add	r3, pc, r3
80015074:	e5832000 	str	r2, [r3]
80015078:	e51b3010 	ldr	r3, [fp, #-16]
8001507c:	e1a00003 	mov	r0, r3
80015080:	e24bd00c 	sub	sp, fp, #12
80015084:	e89d6800 	ldm	sp, {fp, sp, lr}
80015088:	e12fff1e 	bx	lr
8001508c:	0022fbc0 	eoreq	pc, r2, r0, asr #23
80015090:	0022fbb0 	strhteq	pc, [r2], -r0	; <UNPREDICTABLE>
80015094:	0022fb50 	eoreq	pc, r2, r0, asr fp	; <UNPREDICTABLE>
80015098:	0022fb34 	eoreq	pc, r2, r4, lsr fp	; <UNPREDICTABLE>
8001509c:	0022fb24 	eoreq	pc, r2, r4, lsr #22
800150a0:	0022fb18 	eoreq	pc, r2, r8, lsl fp	; <UNPREDICTABLE>
800150a4:	0022fb00 	eoreq	pc, r2, r0, lsl #22
800150a8:	0022fad8 	ldrdeq	pc, [r2], -r8	; <UNPREDICTABLE>
800150ac:	0022fac0 	eoreq	pc, r2, r0, asr #21
800150b0:	0022faac 	eoreq	pc, r2, ip, lsr #21
800150b4:	0022fa9c 	mlaeq	r2, ip, sl, pc	; <UNPREDICTABLE>

800150b8 <memset>:
800150b8:	e1a0c00d 	mov	ip, sp
800150bc:	e92dd800 	push	{fp, ip, lr, pc}
800150c0:	e24cb004 	sub	fp, ip, #4
800150c4:	e24dd018 	sub	sp, sp, #24
800150c8:	e50b0018 	str	r0, [fp, #-24]
800150cc:	e50b101c 	str	r1, [fp, #-28]
800150d0:	e50b2020 	str	r2, [fp, #-32]
800150d4:	e51b3018 	ldr	r3, [fp, #-24]
800150d8:	e50b3014 	str	r3, [fp, #-20]
800150dc:	e3a03000 	mov	r3, #0
800150e0:	e50b3010 	str	r3, [fp, #-16]
800150e4:	e3a03000 	mov	r3, #0
800150e8:	e50b3010 	str	r3, [fp, #-16]
800150ec:	ea000008 	b	80015114 <memset+0x5c>
800150f0:	e51b2014 	ldr	r2, [fp, #-20]
800150f4:	e51b3010 	ldr	r3, [fp, #-16]
800150f8:	e0823003 	add	r3, r2, r3
800150fc:	e51b201c 	ldr	r2, [fp, #-28]
80015100:	e20220ff 	and	r2, r2, #255	; 0xff
80015104:	e5c32000 	strb	r2, [r3]
80015108:	e51b3010 	ldr	r3, [fp, #-16]
8001510c:	e2833001 	add	r3, r3, #1
80015110:	e50b3010 	str	r3, [fp, #-16]
80015114:	e51b2010 	ldr	r2, [fp, #-16]
80015118:	e51b3020 	ldr	r3, [fp, #-32]
8001511c:	e1520003 	cmp	r2, r3
80015120:	3afffff2 	bcc	800150f0 <memset+0x38>
80015124:	e51b3018 	ldr	r3, [fp, #-24]
80015128:	e1a00003 	mov	r0, r3
8001512c:	e24bd00c 	sub	sp, fp, #12
80015130:	e89d6800 	ldm	sp, {fp, sp, lr}
80015134:	e12fff1e 	bx	lr

80015138 <strlen>:
80015138:	e1a0c00d 	mov	ip, sp
8001513c:	e92dd800 	push	{fp, ip, lr, pc}
80015140:	e24cb004 	sub	fp, ip, #4
80015144:	e24dd010 	sub	sp, sp, #16
80015148:	e50b0018 	str	r0, [fp, #-24]
8001514c:	e3a03000 	mov	r3, #0
80015150:	e50b3010 	str	r3, [fp, #-16]
80015154:	ea000005 	b	80015170 <strlen+0x38>
80015158:	e51b3018 	ldr	r3, [fp, #-24]
8001515c:	e2833001 	add	r3, r3, #1
80015160:	e50b3018 	str	r3, [fp, #-24]
80015164:	e51b3010 	ldr	r3, [fp, #-16]
80015168:	e2833001 	add	r3, r3, #1
8001516c:	e50b3010 	str	r3, [fp, #-16]
80015170:	e51b3018 	ldr	r3, [fp, #-24]
80015174:	e5d33000 	ldrb	r3, [r3]
80015178:	e3530000 	cmp	r3, #0
8001517c:	1afffff5 	bne	80015158 <strlen+0x20>
80015180:	e51b3010 	ldr	r3, [fp, #-16]
80015184:	e1a00003 	mov	r0, r3
80015188:	e24bd00c 	sub	sp, fp, #12
8001518c:	e89d6800 	ldm	sp, {fp, sp, lr}
80015190:	e12fff1e 	bx	lr

Disassembly of section .rodata:

80015194 <PKG_TYPE_ERR>:
80015194:	0fffffff 	svceq	0x00ffffff
80015198:	3d3d0a0a 	vldmdbcc	sp!, {s0-s9}
8001519c:	3d3d3d3d 	ldccc	13, cr3, [sp, #-244]!	; 0xffffff0c
800151a0:	3d3d3d3d 	ldccc	13, cr3, [sp, #-244]!	; 0xffffff0c
800151a4:	3d3d3d3d 	ldccc	13, cr3, [sp, #-244]!	; 0xffffff0c
800151a8:	0a3d3d3d 	beq	80f646a4 <_kernelEnd+0xd1fb8c>
800151ac:	6b6f7745 	blvs	81bf2ec8 <_kernelEnd+0x19ae3b0>
800151b0:	2820534f 	stmdacs	r0!, {r0, r1, r2, r3, r6, r8, r9, ip, lr}
800151b4:	4d207962 	stcmi	9, cr7, [r0, #-392]!	; 0xfffffe78
800151b8:	2e617369 	cdpcs	3, 6, cr7, cr1, cr9, {3}
800151bc:	3d0a295a 	stccc	9, cr2, [sl, #-360]	; 0xfffffe98
800151c0:	3d3d3d3d 	ldccc	13, cr3, [sp, #-244]!	; 0xffffff0c
800151c4:	3d3d3d3d 	ldccc	13, cr3, [sp, #-244]!	; 0xffffff0c
800151c8:	3d3d3d3d 	ldccc	13, cr3, [sp, #-244]!	; 0xffffff0c
800151cc:	3d3d3d3d 	ldccc	13, cr3, [sp, #-244]!	; 0xffffff0c
800151d0:	72654b0a 	rsbvc	r4, r5, #10240	; 0x2800
800151d4:	206c656e 	rsbcs	r6, ip, lr, ror #10
800151d8:	20746f67 	rsbscs	r6, r4, r7, ror #30
800151dc:	64616572 	strbtvs	r6, [r1], #-1394	; 0x572
800151e0:	4d4d2879 	stclmi	8, cr2, [sp, #-484]	; 0xfffffe1c
800151e4:	6e612055 	mcrvs	0, 3, r2, cr1, cr5, {2}
800151e8:	72502064 	subsvc	r2, r0, #100	; 0x64
800151ec:	614d636f 	cmpvs	sp, pc, ror #6
800151f0:	0a2e296e 	beq	80b9f7b0 <_kernelEnd+0x95ac98>
800151f4:	64616f4c 	strbtvs	r6, [r1], #-3916	; 0xf4c
800151f8:	20676e69 	rsbcs	r6, r7, r9, ror #28
800151fc:	20656874 	rsbcs	r6, r5, r4, ror r8
80015200:	73726966 	cmnvc	r2, #1671168	; 0x198000
80015204:	72702074 	rsbsvc	r2, r0, #116	; 0x74
80015208:	7365636f 	cmnvc	r5, #-1140850687	; 0xbc000001
8001520c:	2e2e2e73 	mcrcs	14, 1, r2, cr14, cr3, {3}
80015210:	00000a0a 	andeq	r0, r0, sl, lsl #20
80015214:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
80015218:	00000000 	andeq	r0, r0, r0
8001521c:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
80015220:	6f727020 	svcvs	0x00727020
80015224:	73736563 	cmnvc	r3, #415236096	; 0x18c00000
80015228:	616f6c20 	cmnvs	pc, r0, lsr #24
8001522c:	61662064 	cmnvs	r6, r4, rrx
80015230:	64656c69 	strbtvs	r6, [r5], #-3177	; 0xc69
80015234:	00000a21 	andeq	r0, r0, r1, lsr #20

80015238 <PKG_TYPE_ERR>:
80015238:	0fffffff 	svceq	0x00ffffff

8001523c <PKG_TYPE_ERR>:
8001523c:	0fffffff 	svceq	0x00ffffff
80015240:	0000002f 	andeq	r0, r0, pc, lsr #32
80015244:	696e6150 	stmdbvs	lr!, {r4, r6, r8, sp, lr}^
80015248:	70203a63 	eorvc	r3, r0, r3, ror #20
8001524c:	20636f72 	rsbcs	r6, r3, r2, ror pc
80015250:	61707865 	cmnvs	r0, r5, ror #16
80015254:	6d20646e 	cfstrsvs	mvf6, [r0, #-440]!	; 0xfffffe48
80015258:	726f6d65 	rsbvc	r6, pc, #6464	; 0x1940
8001525c:	61662079 	smcvs	25097	; 0x6209
80015260:	64656c69 	strbtvs	r6, [r5], #-3177	; 0xc69
80015264:	000a2121 	andeq	r2, sl, r1, lsr #2

80015268 <PKG_TYPE_ERR>:
80015268:	0fffffff 	svceq	0x00ffffff

8001526c <PKG_TYPE_ERR>:
8001526c:	0fffffff 	svceq	0x00ffffff

80015270 <PKG_TYPE_ERR>:
80015270:	0fffffff 	svceq	0x00ffffff

Disassembly of section .data:

80018000 <cntfrq>:
80018000:	00004000 	andeq	r4, r0, r0
	...

8001c000 <_startupPageDir>:
8001c000:	00000802 	andeq	r0, r0, r2, lsl #16
8001c004:	00100802 	andseq	r0, r0, r2, lsl #16
8001c008:	00200802 	eoreq	r0, r0, r2, lsl #16
8001c00c:	00300802 	eorseq	r0, r0, r2, lsl #16
8001c010:	00400802 	subeq	r0, r0, r2, lsl #16
8001c014:	00500802 	subseq	r0, r0, r2, lsl #16
8001c018:	00600802 	rsbeq	r0, r0, r2, lsl #16
8001c01c:	00700802 	rsbseq	r0, r0, r2, lsl #16
	...
8001e000:	00000802 	andeq	r0, r0, r2, lsl #16
8001e004:	00100802 	andseq	r0, r0, r2, lsl #16
8001e008:	00200802 	eoreq	r0, r0, r2, lsl #16
8001e00c:	00300802 	eorseq	r0, r0, r2, lsl #16
8001e010:	00400802 	subeq	r0, r0, r2, lsl #16
8001e014:	00500802 	subseq	r0, r0, r2, lsl #16
8001e018:	00600802 	rsbeq	r0, r0, r2, lsl #16
8001e01c:	00700802 	rsbseq	r0, r0, r2, lsl #16
	...

80020000 <index.4300>:
80020000:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff

Disassembly of section .got:

80020004 <.got>:
80020004:	80024024 	andhi	r4, r2, r4, lsr #32
80020008:	80014af8 	strdhi	r4, [r1], -r8
8002000c:	80013130 	andhi	r3, r1, r0, lsr r1
80020010:	80024018 	andhi	r4, r2, r8, lsl r0
80020014:	80013200 	andhi	r3, r1, r0, lsl #4
80020018:	80228000 	eorhi	r8, r2, r0
8002001c:	80244b18 	eorhi	r4, r4, r8, lsl fp
80020020:	8002401c 	andhi	r4, r2, ip, lsl r0
80020024:	80013040 	andhi	r3, r1, r0, asr #32
80020028:	80228008 	eorhi	r8, r2, r8

Disassembly of section .got.plt:

8002002c <_GLOBAL_OFFSET_TABLE_>:
	...

Disassembly of section .data.rel.ro:

80020038 <_syscallHandler>:
80020038:	80011558 	andhi	r1, r1, r8, asr r5
8002003c:	80010e9c 	mulhi	r1, ip, lr
80020040:	80010ec8 	andhi	r0, r1, r8, asr #29
80020044:	80010fb8 			; <UNDEFINED> instruction: 0x80010fb8
80020048:	80010fd4 	ldrdhi	r0, [r1], -r4
8002004c:	80010ef0 	strdhi	r0, [r1], -r0	; <UNPREDICTABLE>
80020050:	80011030 	andhi	r1, r1, r0, lsr r0
80020054:	800110a8 	andhi	r1, r1, r8, lsr #1
80020058:	80011008 	andhi	r1, r1, r8
8002005c:	800110c4 	andhi	r1, r1, r4, asr #1
80020060:	8001111c 	andhi	r1, r1, ip, lsl r1
80020064:	80011ec8 	andhi	r1, r1, r8, asr #29
80020068:	80011174 	andhi	r1, r1, r4, ror r1
8002006c:	800111b8 			; <UNDEFINED> instruction: 0x800111b8
80020070:	800111fc 	strdhi	r1, [r1], -ip
80020074:	8001134c 	andhi	r1, r1, ip, asr #6
80020078:	800114b8 			; <UNDEFINED> instruction: 0x800114b8
8002007c:	800117cc 	andhi	r1, r1, ip, asr #15
80020080:	8001194c 	andhi	r1, r1, ip, asr #18
80020084:	80011874 	andhi	r1, r1, r4, ror r8
80020088:	8001157c 	andhi	r1, r1, ip, ror r5
8002008c:	800116bc 			; <UNDEFINED> instruction: 0x800116bc
80020090:	80011a18 	andhi	r1, r1, r8, lsl sl
80020094:	80011a48 	andhi	r1, r1, r8, asr #20
80020098:	80011b68 	andhi	r1, r1, r8, ror #22
8002009c:	80011dfc 	strdhi	r1, [r1], -ip
800200a0:	80011e6c 	andhi	r1, r1, ip, ror #28
800200a4:	80011f2c 	andhi	r1, r1, ip, lsr #30
800200a8:	80011fbc 			; <UNDEFINED> instruction: 0x80011fbc

Disassembly of section .data.rel:

800200ac <_kMallocMemTail>:
800200ac:	8024cb18 	eorhi	ip, r4, r8, lsl fp

Disassembly of section .bss:

80024000 <receiveBuffer>:
	...

80024010 <receiveBufferHead>:
80024010:	00000000 	andeq	r0, r0, r0

80024014 <receiveBufferTail>:
80024014:	00000000 	andeq	r0, r0, r0

80024018 <_kernelVM>:
80024018:	00000000 	andeq	r0, r0, r0

8002401c <_initRamDiskBase>:
8002401c:	00000000 	andeq	r0, r0, r0

80024020 <proc.4272>:
80024020:	00000000 	andeq	r0, r0, r0

80024024 <_initRamDisk>:
	...

8002402c <_interruptHandlers>:
	...

80025000 <_initStack>:
	...

80026000 <_irqStack>:
	...

80027000 <freeList4k>:
80027000:	00000000 	andeq	r0, r0, r0

80027004 <freeList1k>:
80027004:	00000000 	andeq	r0, r0, r0

80027008 <_kMalloc>:
	...

80028000 <_processVM>:
	...

80228000 <_currentProcess>:
80228000:	00000000 	andeq	r0, r0, r0

80228004 <proc.4299>:
80228004:	00000000 	andeq	r0, r0, r0

80228008 <_processTable>:
	...

80244808 <_pkgIDCount>:
80244808:	00000000 	andeq	r0, r0, r0

8024480c <_kernelFiles>:
8024480c:	00000000 	andeq	r0, r0, r0

80244810 <_kernelServices>:
	...

80244b10 <roundRobinIndex>:
80244b10:	00000000 	andeq	r0, r0, r0

80244b14 <last.4036>:
80244b14:	00000000 	andeq	r0, r0, r0

Disassembly of section .ARM.attributes:

00000000 <.ARM.attributes>:
   0:	00003641 	andeq	r3, r0, r1, asr #12
   4:	61656100 	cmnvs	r5, r0, lsl #2
   8:	01006962 	tsteq	r0, r2, ror #18
   c:	0000002c 	andeq	r0, r0, ip, lsr #32
  10:	726f4305 	rsbvc	r4, pc, #335544320	; 0x14000000
  14:	2d786574 	cfldr64cs	mvdx6, [r8, #-464]!	; 0xfffffe30
  18:	06003741 	streq	r3, [r0], -r1, asr #14
  1c:	0841070a 	stmdaeq	r1, {r1, r3, r8, r9, sl}^
  20:	12020901 	andne	r0, r2, #16384	; 0x4000
  24:	15011404 	strne	r1, [r1, #-1028]	; 0x404
  28:	18031701 	stmdane	r3, {r0, r8, r9, sl, ip}
  2c:	22011a01 	andcs	r1, r1, #4096	; 0x1000
  30:	2c012a01 	stccs	10, cr2, [r1], {1}
  34:	Address 0x0000000000000034 is out of bounds.


Disassembly of section .comment:

00000000 <.comment>:
   0:	3a434347 	bcc	10d0d24 <start_address+0x10c0d24>
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
  48:	652f4d52 	strvs	r4, [pc, #-3410]!	; fffff2fe <_kernelEnd+0x7fdba7e6>
  4c:	6465626d 	strbtvs	r6, [r5], #-621	; 0x26d
  50:	2d646564 	cfstr64cs	mvdx6, [r4, #-400]!	; 0xfffffe70
  54:	2d395f34 	ldccs	15, cr5, [r9, #-208]!	; 0xffffff30
  58:	6e617262 	cdpvs	2, 6, cr7, cr1, cr2, {3}
  5c:	72206863 	eorvc	r6, r0, #6488064	; 0x630000
  60:	73697665 	cmnvc	r9, #105906176	; 0x6500000
  64:	206e6f69 	rsbcs	r6, lr, r9, ror #30
  68:	39373232 	ldmdbcc	r7!, {r1, r4, r5, r9, ip, sp}
  6c:	005d3737 	subseq	r3, sp, r7, lsr r7

Disassembly of section .debug_line:

00000000 <.debug_line>:
   0:	00000079 	andeq	r0, r0, r9, ror r0
   4:	00210002 	eoreq	r0, r1, r2
   8:	01020000 	mrseq	r0, (UNDEF: 2)
   c:	000d0efb 	strdeq	r0, [sp], -fp
  10:	01010101 	tsteq	r1, r1, lsl #2
  14:	01000000 	mrseq	r0, (UNDEF: 0)
  18:	61010000 	mrsvs	r0, (UNDEF: 1)
  1c:	00006d73 	andeq	r6, r0, r3, ror sp
  20:	746f6f62 	strbtvc	r6, [pc], #-3938	; 28 <start_address-0xffd8>
  24:	0100532e 	tsteq	r0, lr, lsr #6
  28:	00000000 	andeq	r0, r0, r0
  2c:	00000205 	andeq	r0, r0, r5, lsl #4
  30:	0c038001 	stceq	0, cr8, [r3], {1}
  34:	2f302f01 	svccs	0x00302f01
  38:	302f2f2f 	eorcc	r2, pc, pc, lsr #30
  3c:	302f3032 	eorcc	r3, pc, r2, lsr r0	; <UNPREDICTABLE>
  40:	2f302f2f 	svccs	0x00302f2f
  44:	2f35302f 	svccs	0x0035302f
  48:	2f2f312f 	svccs	0x002f312f
  4c:	302f2f2f 	eorcc	r2, pc, pc, lsr #30
  50:	2f2f2f31 	svccs	0x002f2f31
  54:	8a2f2f2f 	bhi	bcbd18 <start_address+0xbbbd18>
  58:	30d8302f 	sbcscc	r3, r8, pc, lsr #32
  5c:	2f2f3130 	svccs	0x002f3130
  60:	3030d830 	eorscc	sp, r0, r0, lsr r8
  64:	2f2f2f33 	svccs	0x002f2f33
  68:	9f03322f 	svcls	0x0003322f
  6c:	12032e7f 	andne	r2, r3, #2032	; 0x7f0
  70:	2e10032e 	cdpcs	3, 1, cr0, cr0, cr14, {1}
  74:	2e28032f 	cdpcs	3, 2, cr0, cr8, cr15, {1}
  78:	01000202 	tsteq	r0, r2, lsl #4
  7c:	00005301 	andeq	r5, r0, r1, lsl #6
  80:	23000200 	movwcs	r0, #512	; 0x200
  84:	02000000 	andeq	r0, r0, #0
  88:	0d0efb01 	vstreq	d15, [lr, #-4]
  8c:	01010100 	mrseq	r0, (UNDEF: 17)
  90:	00000001 	andeq	r0, r0, r1
  94:	01000001 	tsteq	r0, r1
  98:	006d7361 	rsbeq	r7, sp, r1, ror #6
  9c:	73797300 	cmnvc	r9, #0, 6
  a0:	2e6d6574 	mcrcs	5, 3, r6, cr13, cr4, {3}
  a4:	00010053 	andeq	r0, r1, r3, asr r0
  a8:	05000000 	streq	r0, [r0, #-0]
  ac:	01012802 	tsteq	r1, r2, lsl #16
  b0:	322f1680 	eorcc	r1, pc, #128, 12	; 0x8000000
  b4:	322f322f 	eorcc	r3, pc, #-268435454	; 0xf0000002
  b8:	322f322f 	eorcc	r3, pc, #-268435454	; 0xf0000002
  bc:	2f2f2f2f 	svccs	0x002f2f2f
  c0:	302f2f32 	eorcc	r2, pc, r2, lsr pc	; <UNPREDICTABLE>
  c4:	2f2f2f32 	svccs	0x002f2f32
  c8:	2f32302f 	svccs	0x0032302f
  cc:	022f2f2f 	eoreq	r2, pc, #47, 30	; 0xbc
  d0:	01010002 	tsteq	r1, r2
  d4:	00000051 	andeq	r0, r0, r1, asr r0
  d8:	00240002 	eoreq	r0, r4, r2
  dc:	01020000 	mrseq	r0, (UNDEF: 2)
  e0:	000d0efb 	strdeq	r0, [sp], -fp
  e4:	01010101 	tsteq	r1, r1, lsl #2
  e8:	01000000 	mrseq	r0, (UNDEF: 0)
  ec:	61010000 	mrsvs	r0, (UNDEF: 1)
  f0:	00006d73 	andeq	r6, r0, r3, ror sp
  f4:	746e6f63 	strbtvc	r6, [lr], #-3939	; 0xf63
  f8:	2e747865 	cdpcs	8, 7, cr7, cr4, cr5, {3}
  fc:	00010053 	andeq	r0, r1, r3, asr r0
 100:	05000000 	streq	r0, [r0, #-0]
 104:	0101a002 	tsteq	r1, r2
 108:	30311580 	eorscc	r1, r1, r0, lsl #11
 10c:	2f312f2f 	svccs	0x00312f2f
 110:	2f2f2f31 	svccs	0x002f2f31
 114:	2f312f31 	svccs	0x00312f31
 118:	31303330 	teqcc	r0, r0, lsr r3
 11c:	2f312f2f 	svccs	0x00312f2f
 120:	2f312f31 	svccs	0x00312f31
 124:	01000202 	tsteq	r0, r2, lsl #4
 128:	Address 0x0000000000000128 is out of bounds.


Disassembly of section .debug_info:

00000000 <.debug_info>:
   0:	0000004e 	andeq	r0, r0, lr, asr #32
   4:	00000002 	andeq	r0, r0, r2
   8:	01040000 	mrseq	r0, (UNDEF: 4)
   c:	00000000 	andeq	r0, r0, r0
  10:	80010000 	andhi	r0, r1, r0
  14:	80010128 	andhi	r0, r1, r8, lsr #2
  18:	2f6d7361 	svccs	0x006d7361
  1c:	746f6f62 	strbtvc	r6, [pc], #-3938	; 24 <start_address-0xffdc>
  20:	2f00532e 	svccs	0x0000532e
  24:	72657355 	rsbvc	r7, r5, #1409286145	; 0x54000001
  28:	696d2f73 	stmdbvs	sp!, {r0, r1, r4, r5, r6, r8, r9, sl, fp, sp}^
  2c:	772f6173 			; <UNDEFINED> instruction: 0x772f6173
  30:	2f6b726f 	svccs	0x006b726f
  34:	6b6f7745 	blvs	1bddd50 <start_address+0x1bcdd50>
  38:	6b2f534f 	blvs	bd4d7c <start_address+0xbc4d7c>
  3c:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
  40:	4e47006c 	cdpmi	0, 4, cr0, cr7, cr12, {3}
  44:	53412055 	movtpl	r2, #4181	; 0x1055
  48:	322e3220 	eorcc	r3, lr, #32, 4
  4c:	00302e34 	eorseq	r2, r0, r4, lsr lr
  50:	00508001 	subseq	r8, r0, r1
  54:	00020000 	andeq	r0, r2, r0
  58:	00000014 	andeq	r0, r0, r4, lsl r0
  5c:	007d0104 	rsbseq	r0, sp, r4, lsl #2
  60:	01280000 			; <UNDEFINED> instruction: 0x01280000
  64:	01a08001 	moveq	r8, r1
  68:	73618001 	cmnvc	r1, #1
  6c:	79732f6d 	ldmdbvc	r3!, {r0, r2, r3, r5, r6, r8, r9, sl, fp, sp}^
  70:	6d657473 	cfstrdvs	mvd7, [r5, #-460]!	; 0xfffffe34
  74:	2f00532e 	svccs	0x0000532e
  78:	72657355 	rsbvc	r7, r5, #1409286145	; 0x54000001
  7c:	696d2f73 	stmdbvs	sp!, {r0, r1, r4, r5, r6, r8, r9, sl, fp, sp}^
  80:	772f6173 			; <UNDEFINED> instruction: 0x772f6173
  84:	2f6b726f 	svccs	0x006b726f
  88:	6b6f7745 	blvs	1bddda4 <start_address+0x1bcdda4>
  8c:	6b2f534f 	blvs	bd4dd0 <start_address+0xbc4dd0>
  90:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
  94:	4e47006c 	cdpmi	0, 4, cr0, cr7, cr12, {3}
  98:	53412055 	movtpl	r2, #4181	; 0x1055
  9c:	322e3220 	eorcc	r3, lr, #32, 4
  a0:	00302e34 	eorseq	r2, r0, r4, lsr lr
  a4:	00518001 	subseq	r8, r1, r1
  a8:	00020000 	andeq	r0, r2, r0
  ac:	00000028 	andeq	r0, r0, r8, lsr #32
  b0:	00d40104 	sbcseq	r0, r4, r4, lsl #2
  b4:	01a00000 	moveq	r0, r0
  b8:	020c8001 	andeq	r8, ip, #1
  bc:	73618001 	cmnvc	r1, #1
  c0:	6f632f6d 	svcvs	0x00632f6d
  c4:	7865746e 	stmdavc	r5!, {r1, r2, r3, r5, r6, sl, ip, sp, lr}^
  c8:	00532e74 	subseq	r2, r3, r4, ror lr
  cc:	6573552f 	ldrbvs	r5, [r3, #-1327]!	; 0x52f
  d0:	6d2f7372 	stcvs	3, cr7, [pc, #-456]!	; ffffff10 <_kernelEnd+0x7fdbb3f8>
  d4:	2f617369 	svccs	0x00617369
  d8:	6b726f77 	blvs	1c9bebc <start_address+0x1c8bebc>
  dc:	6f77452f 	svcvs	0x0077452f
  e0:	2f534f6b 	svccs	0x00534f6b
  e4:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
  e8:	47006c65 	strmi	r6, [r0, -r5, ror #24]
  ec:	4120554e 			; <UNDEFINED> instruction: 0x4120554e
  f0:	2e322053 	mrccs	0, 1, r2, cr2, cr3, {2}
  f4:	302e3432 	eorcc	r3, lr, r2, lsr r4
  f8:	Address 0x00000000000000f8 is out of bounds.


Disassembly of section .debug_abbrev:

00000000 <.debug_abbrev>:
   0:	10001101 	andne	r1, r0, r1, lsl #2
   4:	12011106 	andne	r1, r1, #-2147483647	; 0x80000001
   8:	1b080301 	blne	200c14 <start_address+0x1f0c14>
   c:	13082508 	movwne	r2, #34056	; 0x8508
  10:	00000005 	andeq	r0, r0, r5
  14:	10001101 	andne	r1, r0, r1, lsl #2
  18:	12011106 	andne	r1, r1, #-2147483647	; 0x80000001
  1c:	1b080301 	blne	200c28 <start_address+0x1f0c28>
  20:	13082508 	movwne	r2, #34056	; 0x8508
  24:	00000005 	andeq	r0, r0, r5
  28:	10001101 	andne	r1, r0, r1, lsl #2
  2c:	12011106 	andne	r1, r1, #-2147483647	; 0x80000001
  30:	1b080301 	blne	200c3c <start_address+0x1f0c3c>
  34:	13082508 	movwne	r2, #34056	; 0x8508
  38:	00000005 	andeq	r0, r0, r5

Disassembly of section .debug_aranges:

00000000 <.debug_aranges>:
   0:	0000001c 	andeq	r0, r0, ip, lsl r0
   4:	00000002 	andeq	r0, r0, r2
   8:	00040000 	andeq	r0, r4, r0
   c:	00000000 	andeq	r0, r0, r0
  10:	80010000 	andhi	r0, r1, r0
  14:	00000128 	andeq	r0, r0, r8, lsr #2
	...
  20:	0000001c 	andeq	r0, r0, ip, lsl r0
  24:	00520002 	subseq	r0, r2, r2
  28:	00040000 	andeq	r0, r4, r0
  2c:	00000000 	andeq	r0, r0, r0
  30:	80010128 	andhi	r0, r1, r8, lsr #2
  34:	00000078 	andeq	r0, r0, r8, ror r0
	...
  40:	0000001c 	andeq	r0, r0, ip, lsl r0
  44:	00a60002 	adceq	r0, r6, r2
  48:	00040000 	andeq	r0, r4, r0
  4c:	00000000 	andeq	r0, r0, r0
  50:	800101a0 	andhi	r0, r1, r0, lsr #3
  54:	0000006c 	andeq	r0, r0, ip, rrx
	...

Disassembly of section .stab:

00000000 <.stab>:
       0:	00000001 	andeq	r0, r0, r1
       4:	0d7a0000 	ldcleq	0, cr0, [sl, #-0]
       8:	00003a5b 	andeq	r3, r0, fp, asr sl
       c:	00000007 	andeq	r0, r0, r7
      10:	00020064 	andeq	r0, r2, r4, rrx
      14:	8001020c 	andhi	r0, r1, ip, lsl #4
      18:	00000027 	andeq	r0, r0, r7, lsr #32
      1c:	00020064 	andeq	r0, r2, r4, rrx
      20:	8001020c 	andhi	r0, r1, ip, lsl #4
      24:	0000003d 	andeq	r0, r0, sp, lsr r0
      28:	0000003c 	andeq	r0, r0, ip, lsr r0
      2c:	00000000 	andeq	r0, r0, r0
      30:	0000004c 	andeq	r0, r0, ip, asr #32
      34:	00000080 	andeq	r0, r0, r0, lsl #1
      38:	00000000 	andeq	r0, r0, r0
      3c:	00000076 	andeq	r0, r0, r6, ror r0
      40:	00000080 	andeq	r0, r0, r0, lsl #1
      44:	00000000 	andeq	r0, r0, r0
      48:	00000094 	muleq	r0, r4, r0
      4c:	00000080 	andeq	r0, r0, r0, lsl #1
      50:	00000000 	andeq	r0, r0, r0
      54:	000000c3 	andeq	r0, r0, r3, asr #1
      58:	00000080 	andeq	r0, r0, r0, lsl #1
      5c:	00000000 	andeq	r0, r0, r0
      60:	000000ee 	andeq	r0, r0, lr, ror #1
      64:	00000080 	andeq	r0, r0, r0, lsl #1
      68:	00000000 	andeq	r0, r0, r0
      6c:	0000011e 	andeq	r0, r0, lr, lsl r1
      70:	00000080 	andeq	r0, r0, r0, lsl #1
      74:	00000000 	andeq	r0, r0, r0
      78:	0000016f 	andeq	r0, r0, pc, ror #2
      7c:	00000080 	andeq	r0, r0, r0, lsl #1
      80:	00000000 	andeq	r0, r0, r0
      84:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
      88:	00000080 	andeq	r0, r0, r0, lsl #1
      8c:	00000000 	andeq	r0, r0, r0
      90:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
      94:	00000080 	andeq	r0, r0, r0, lsl #1
      98:	00000000 	andeq	r0, r0, r0
      9c:	0000020e 	andeq	r0, r0, lr, lsl #4
      a0:	00000080 	andeq	r0, r0, r0, lsl #1
      a4:	00000000 	andeq	r0, r0, r0
      a8:	00000238 	andeq	r0, r0, r8, lsr r2
      ac:	00000080 	andeq	r0, r0, r0, lsl #1
      b0:	00000000 	andeq	r0, r0, r0
      b4:	00000261 	andeq	r0, r0, r1, ror #4
      b8:	00000080 	andeq	r0, r0, r0, lsl #1
      bc:	00000000 	andeq	r0, r0, r0
      c0:	0000027b 	andeq	r0, r0, fp, ror r2
      c4:	00000080 	andeq	r0, r0, r0, lsl #1
      c8:	00000000 	andeq	r0, r0, r0
      cc:	00000296 	muleq	r0, r6, r2
      d0:	00000080 	andeq	r0, r0, r0, lsl #1
      d4:	00000000 	andeq	r0, r0, r0
      d8:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
      dc:	00000080 	andeq	r0, r0, r0, lsl #1
      e0:	00000000 	andeq	r0, r0, r0
      e4:	000002d7 	ldrdeq	r0, [r0], -r7
      e8:	00000080 	andeq	r0, r0, r0, lsl #1
      ec:	00000000 	andeq	r0, r0, r0
      f0:	000002f7 	strdeq	r0, [r0], -r7
      f4:	00000080 	andeq	r0, r0, r0, lsl #1
      f8:	00000000 	andeq	r0, r0, r0
      fc:	0000031c 	andeq	r0, r0, ip, lsl r3
     100:	00000080 	andeq	r0, r0, r0, lsl #1
     104:	00000000 	andeq	r0, r0, r0
     108:	00000346 	andeq	r0, r0, r6, asr #6
     10c:	00000080 	andeq	r0, r0, r0, lsl #1
     110:	00000000 	andeq	r0, r0, r0
     114:	0000036a 	andeq	r0, r0, sl, ror #6
     118:	00000080 	andeq	r0, r0, r0, lsl #1
     11c:	00000000 	andeq	r0, r0, r0
     120:	00000393 	muleq	r0, r3, r3
     124:	00000080 	andeq	r0, r0, r0, lsl #1
     128:	00000000 	andeq	r0, r0, r0
     12c:	000003c1 	andeq	r0, r0, r1, asr #7
     130:	00000080 	andeq	r0, r0, r0, lsl #1
     134:	00000000 	andeq	r0, r0, r0
     138:	000003e7 	andeq	r0, r0, r7, ror #7
     13c:	00000080 	andeq	r0, r0, r0, lsl #1
     140:	00000000 	andeq	r0, r0, r0
     144:	00000407 	andeq	r0, r0, r7, lsl #8
     148:	00000080 	andeq	r0, r0, r0, lsl #1
     14c:	00000000 	andeq	r0, r0, r0
     150:	0000042c 	andeq	r0, r0, ip, lsr #8
     154:	00000080 	andeq	r0, r0, r0, lsl #1
     158:	00000000 	andeq	r0, r0, r0
     15c:	00000456 	andeq	r0, r0, r6, asr r4
     160:	00000080 	andeq	r0, r0, r0, lsl #1
     164:	00000000 	andeq	r0, r0, r0
     168:	00000485 	andeq	r0, r0, r5, lsl #9
     16c:	00000080 	andeq	r0, r0, r0, lsl #1
     170:	00000000 	andeq	r0, r0, r0
     174:	000004ae 	andeq	r0, r0, lr, lsr #9
     178:	00000080 	andeq	r0, r0, r0, lsl #1
     17c:	00000000 	andeq	r0, r0, r0
     180:	000004dc 	ldrdeq	r0, [r0], -ip
     184:	00000080 	andeq	r0, r0, r0, lsl #1
     188:	00000000 	andeq	r0, r0, r0
     18c:	0000050f 	andeq	r0, r0, pc, lsl #10
     190:	00000080 	andeq	r0, r0, r0, lsl #1
     194:	00000000 	andeq	r0, r0, r0
     198:	00000530 	andeq	r0, r0, r0, lsr r5
     19c:	00000080 	andeq	r0, r0, r0, lsl #1
     1a0:	00000000 	andeq	r0, r0, r0
     1a4:	00000550 	andeq	r0, r0, r0, asr r5
     1a8:	00000080 	andeq	r0, r0, r0, lsl #1
     1ac:	00000000 	andeq	r0, r0, r0
     1b0:	00000575 	andeq	r0, r0, r5, ror r5
     1b4:	00000080 	andeq	r0, r0, r0, lsl #1
     1b8:	00000000 	andeq	r0, r0, r0
     1bc:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
     1c0:	00000080 	andeq	r0, r0, r0, lsl #1
     1c4:	00000000 	andeq	r0, r0, r0
     1c8:	000005c3 	andeq	r0, r0, r3, asr #11
     1cc:	00000080 	andeq	r0, r0, r0, lsl #1
     1d0:	00000000 	andeq	r0, r0, r0
     1d4:	000005ec 	andeq	r0, r0, ip, ror #11
     1d8:	00000080 	andeq	r0, r0, r0, lsl #1
     1dc:	00000000 	andeq	r0, r0, r0
     1e0:	0000061a 	andeq	r0, r0, sl, lsl r6
     1e4:	00000080 	andeq	r0, r0, r0, lsl #1
     1e8:	00000000 	andeq	r0, r0, r0
     1ec:	00000640 	andeq	r0, r0, r0, asr #12
     1f0:	00000080 	andeq	r0, r0, r0, lsl #1
     1f4:	00000000 	andeq	r0, r0, r0
     1f8:	00000660 	andeq	r0, r0, r0, ror #12
     1fc:	00000080 	andeq	r0, r0, r0, lsl #1
     200:	00000000 	andeq	r0, r0, r0
     204:	00000685 	andeq	r0, r0, r5, lsl #13
     208:	00000080 	andeq	r0, r0, r0, lsl #1
     20c:	00000000 	andeq	r0, r0, r0
     210:	000006af 	andeq	r0, r0, pc, lsr #13
     214:	00000080 	andeq	r0, r0, r0, lsl #1
     218:	00000000 	andeq	r0, r0, r0
     21c:	000006de 	ldrdeq	r0, [r0], -lr
     220:	00000080 	andeq	r0, r0, r0, lsl #1
     224:	00000000 	andeq	r0, r0, r0
     228:	00000707 	andeq	r0, r0, r7, lsl #14
     22c:	00000080 	andeq	r0, r0, r0, lsl #1
     230:	00000000 	andeq	r0, r0, r0
     234:	00000735 	andeq	r0, r0, r5, lsr r7
     238:	00000080 	andeq	r0, r0, r0, lsl #1
     23c:	00000000 	andeq	r0, r0, r0
     240:	00000768 	andeq	r0, r0, r8, ror #14
     244:	00000080 	andeq	r0, r0, r0, lsl #1
     248:	00000000 	andeq	r0, r0, r0
     24c:	0000077c 	andeq	r0, r0, ip, ror r7
     250:	00000082 	andeq	r0, r0, r2, lsl #1
     254:	00000bc7 	andeq	r0, r0, r7, asr #23
     258:	0000078a 	andeq	r0, r0, sl, lsl #15
     25c:	00000082 	andeq	r0, r0, r2, lsl #1
     260:	00002d60 	andeq	r2, r0, r0, ror #26
     264:	0000079a 	muleq	r0, sl, r7
     268:	000c0080 	andeq	r0, ip, r0, lsl #1
     26c:	00000000 	andeq	r0, r0, r0
     270:	000007ac 	andeq	r0, r0, ip, lsr #15
     274:	00100080 	andseq	r0, r0, r0, lsl #1
     278:	00000000 	andeq	r0, r0, r0
     27c:	000007c1 	andeq	r0, r0, r1, asr #15
     280:	00110080 	andseq	r0, r1, r0, lsl #1
     284:	00000000 	andeq	r0, r0, r0
     288:	000007d7 	ldrdeq	r0, [r0], -r7
     28c:	00120080 	andseq	r0, r2, r0, lsl #1
     290:	00000000 	andeq	r0, r0, r0
     294:	000007ec 	andeq	r0, r0, ip, ror #15
     298:	00130080 	andseq	r0, r3, r0, lsl #1
     29c:	00000000 	andeq	r0, r0, r0
     2a0:	00000802 	andeq	r0, r0, r2, lsl #16
     2a4:	00140080 	andseq	r0, r4, r0, lsl #1
     2a8:	00000000 	andeq	r0, r0, r0
     2ac:	00000817 	andeq	r0, r0, r7, lsl r8
     2b0:	00150080 	andseq	r0, r5, r0, lsl #1
     2b4:	00000000 	andeq	r0, r0, r0
     2b8:	0000082d 	andeq	r0, r0, sp, lsr #16
     2bc:	00160080 	andseq	r0, r6, r0, lsl #1
     2c0:	00000000 	andeq	r0, r0, r0
     2c4:	00000844 	andeq	r0, r0, r4, asr #16
     2c8:	00190080 	andseq	r0, r9, r0, lsl #1
	...
     2d4:	000000a2 	andeq	r0, r0, r2, lsr #1
     2d8:	00000000 	andeq	r0, r0, r0
     2dc:	00000858 	andeq	r0, r0, r8, asr r8
     2e0:	00090080 	andeq	r0, r9, r0, lsl #1
	...
     2ec:	000000a2 	andeq	r0, r0, r2, lsr #1
     2f0:	00000000 	andeq	r0, r0, r0
     2f4:	00000886 	andeq	r0, r0, r6, lsl #17
     2f8:	00000082 	andeq	r0, r0, r2, lsl #1
     2fc:	00003ac8 	andeq	r3, r0, r8, asr #21
     300:	00000897 	muleq	r0, r7, r8
     304:	00400080 	subeq	r0, r0, r0, lsl #1
     308:	00000000 	andeq	r0, r0, r0
     30c:	000008fd 	strdeq	r0, [r0], -sp
     310:	00490080 	subeq	r0, r9, r0, lsl #1
	...
     31c:	000000a2 	andeq	r0, r0, r2, lsr #1
     320:	00000000 	andeq	r0, r0, r0
     324:	0000097d 	andeq	r0, r0, sp, ror r9
     328:	00120024 	andseq	r0, r2, r4, lsr #32
     32c:	8001020c 	andhi	r0, r1, ip, lsl #4
     330:	00000000 	andeq	r0, r0, r0
     334:	0000002e 	andeq	r0, r0, lr, lsr #32
     338:	8001020c 	andhi	r0, r1, ip, lsl #4
     33c:	00000000 	andeq	r0, r0, r0
     340:	00130044 	andseq	r0, r3, r4, asr #32
	...
     34c:	00140044 	andseq	r0, r4, r4, asr #32
     350:	00000010 	andeq	r0, r0, r0, lsl r0
     354:	00000000 	andeq	r0, r0, r0
     358:	00150044 	andseq	r0, r5, r4, asr #32
     35c:	00000020 	andeq	r0, r0, r0, lsr #32
     360:	00000000 	andeq	r0, r0, r0
     364:	00160044 	andseq	r0, r6, r4, asr #32
     368:	0000002c 	andeq	r0, r0, ip, lsr #32
     36c:	000009a3 	andeq	r0, r0, r3, lsr #19
     370:	00140080 	andseq	r0, r4, r0, lsl #1
     374:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     378:	00000000 	andeq	r0, r0, r0
     37c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
     388:	000000e0 	andeq	r0, r0, r0, ror #1
     38c:	00000034 	andeq	r0, r0, r4, lsr r0
     390:	00000000 	andeq	r0, r0, r0
     394:	00000024 	andeq	r0, r0, r4, lsr #32
     398:	00000034 	andeq	r0, r0, r4, lsr r0
     39c:	00000000 	andeq	r0, r0, r0
     3a0:	0000004e 	andeq	r0, r0, lr, asr #32
     3a4:	80010240 	andhi	r0, r1, r0, asr #4
     3a8:	000009ae 	andeq	r0, r0, lr, lsr #19
     3ac:	00190024 	andseq	r0, r9, r4, lsr #32
     3b0:	80010240 	andhi	r0, r1, r0, asr #4
     3b4:	00000000 	andeq	r0, r0, r0
     3b8:	0000002e 	andeq	r0, r0, lr, lsr #32
     3bc:	80010240 	andhi	r0, r1, r0, asr #4
     3c0:	00000000 	andeq	r0, r0, r0
     3c4:	001a0044 	andseq	r0, sl, r4, asr #32
	...
     3d0:	001c0044 	andseq	r0, ip, r4, asr #32
     3d4:	00000010 	andeq	r0, r0, r0, lsl r0
     3d8:	00000000 	andeq	r0, r0, r0
     3dc:	001d0044 	andseq	r0, sp, r4, asr #32
     3e0:	00000020 	andeq	r0, r0, r0, lsr #32
     3e4:	00000000 	andeq	r0, r0, r0
     3e8:	001e0044 	andseq	r0, lr, r4, asr #32
     3ec:	0000002c 	andeq	r0, r0, ip, lsr #32
     3f0:	00000000 	andeq	r0, r0, r0
     3f4:	001f0044 	andseq	r0, pc, r4, asr #32
     3f8:	00000030 	andeq	r0, r0, r0, lsr r0
     3fc:	000009cd 	andeq	r0, r0, sp, asr #19
     400:	001b0080 	andseq	r0, fp, r0, lsl #1
     404:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
     408:	000009a3 	andeq	r0, r0, r3, lsr #19
     40c:	001c0080 	andseq	r0, ip, r0, lsl #1
     410:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     414:	00000000 	andeq	r0, r0, r0
     418:	000000c0 	andeq	r0, r0, r0, asr #1
	...
     424:	000000e0 	andeq	r0, r0, r0, ror #1
     428:	0000003c 	andeq	r0, r0, ip, lsr r0
     42c:	00000000 	andeq	r0, r0, r0
     430:	00000024 	andeq	r0, r0, r4, lsr #32
     434:	0000003c 	andeq	r0, r0, ip, lsr r0
     438:	00000000 	andeq	r0, r0, r0
     43c:	0000004e 	andeq	r0, r0, lr, asr #32
     440:	8001027c 	andhi	r0, r1, ip, ror r2
     444:	000009d7 	ldrdeq	r0, [r0], -r7
     448:	00230024 	eoreq	r0, r3, r4, lsr #32
     44c:	8001027c 	andhi	r0, r1, ip, ror r2
     450:	00000000 	andeq	r0, r0, r0
     454:	0000002e 	andeq	r0, r0, lr, lsr #32
     458:	8001027c 	andhi	r0, r1, ip, ror r2
     45c:	00000000 	andeq	r0, r0, r0
     460:	00240044 	eoreq	r0, r4, r4, asr #32
	...
     46c:	00260044 	eoreq	r0, r6, r4, asr #32
     470:	00000010 	andeq	r0, r0, r0, lsl r0
     474:	00000000 	andeq	r0, r0, r0
     478:	00270044 	eoreq	r0, r7, r4, asr #32
     47c:	00000018 	andeq	r0, r0, r8, lsl r0
     480:	00000000 	andeq	r0, r0, r0
     484:	00280044 	eoreq	r0, r8, r4, asr #32
     488:	00000020 	andeq	r0, r0, r0, lsr #32
     48c:	000009eb 	andeq	r0, r0, fp, ror #19
     490:	00250080 	eoreq	r0, r5, r0, lsl #1
     494:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     498:	00000000 	andeq	r0, r0, r0
     49c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
     4a8:	000000e0 	andeq	r0, r0, r0, ror #1
     4ac:	00000028 	andeq	r0, r0, r8, lsr #32
     4b0:	00000000 	andeq	r0, r0, r0
     4b4:	00000024 	andeq	r0, r0, r4, lsr #32
     4b8:	00000028 	andeq	r0, r0, r8, lsr #32
     4bc:	00000000 	andeq	r0, r0, r0
     4c0:	0000004e 	andeq	r0, r0, lr, asr #32
     4c4:	800102a4 	andhi	r0, r1, r4, lsr #5
     4c8:	000009fa 	strdeq	r0, [r0], -sl
     4cc:	002a0024 	eoreq	r0, sl, r4, lsr #32
     4d0:	800102a4 	andhi	r0, r1, r4, lsr #5
     4d4:	00000000 	andeq	r0, r0, r0
     4d8:	0000002e 	andeq	r0, r0, lr, lsr #32
     4dc:	800102a4 	andhi	r0, r1, r4, lsr #5
     4e0:	00000000 	andeq	r0, r0, r0
     4e4:	002b0044 	eoreq	r0, fp, r4, asr #32
	...
     4f0:	002d0044 	eoreq	r0, sp, r4, asr #32
     4f4:	00000010 	andeq	r0, r0, r0, lsl r0
     4f8:	00000000 	andeq	r0, r0, r0
     4fc:	002e0044 	eoreq	r0, lr, r4, asr #32
     500:	00000018 	andeq	r0, r0, r8, lsl r0
     504:	00000000 	andeq	r0, r0, r0
     508:	002f0044 	eoreq	r0, pc, r4, asr #32
     50c:	00000020 	andeq	r0, r0, r0, lsr #32
     510:	000009eb 	andeq	r0, r0, fp, ror #19
     514:	002c0080 	eoreq	r0, ip, r0, lsl #1
     518:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     51c:	00000000 	andeq	r0, r0, r0
     520:	000000c0 	andeq	r0, r0, r0, asr #1
	...
     52c:	000000e0 	andeq	r0, r0, r0, ror #1
     530:	00000028 	andeq	r0, r0, r8, lsr #32
     534:	00000000 	andeq	r0, r0, r0
     538:	00000024 	andeq	r0, r0, r4, lsr #32
     53c:	00000028 	andeq	r0, r0, r8, lsr #32
     540:	00000000 	andeq	r0, r0, r0
     544:	0000004e 	andeq	r0, r0, lr, asr #32
     548:	800102cc 	andhi	r0, r1, ip, asr #5
     54c:	00000a0f 	andeq	r0, r0, pc, lsl #20
     550:	00400024 	subeq	r0, r0, r4, lsr #32
     554:	800102cc 	andhi	r0, r1, ip, asr #5
     558:	00000a27 	andeq	r0, r0, r7, lsr #20
     55c:	004000a0 	subeq	r0, r0, r0, lsr #1
     560:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     564:	00000000 	andeq	r0, r0, r0
     568:	0000002e 	andeq	r0, r0, lr, lsr #32
     56c:	800102cc 	andhi	r0, r1, ip, asr #5
     570:	00000000 	andeq	r0, r0, r0
     574:	00410044 	subeq	r0, r1, r4, asr #32
	...
     580:	00420044 	subeq	r0, r2, r4, asr #32
     584:	00000014 	andeq	r0, r0, r4, lsl r0
     588:	00000000 	andeq	r0, r0, r0
     58c:	00430044 	subeq	r0, r3, r4, asr #32
     590:	0000001c 	andeq	r0, r0, ip, lsl r0
     594:	00000000 	andeq	r0, r0, r0
     598:	00440044 	subeq	r0, r4, r4, asr #32
     59c:	00000020 	andeq	r0, r0, r0, lsr #32
     5a0:	00000000 	andeq	r0, r0, r0
     5a4:	00000024 	andeq	r0, r0, r4, lsr #32
     5a8:	00000028 	andeq	r0, r0, r8, lsr #32
     5ac:	00000000 	andeq	r0, r0, r0
     5b0:	0000004e 	andeq	r0, r0, lr, asr #32
     5b4:	800102f4 	strdhi	r0, [r1], -r4
     5b8:	00000a32 	andeq	r0, r0, r2, lsr sl
     5bc:	00460024 	subeq	r0, r6, r4, lsr #32
     5c0:	800102f4 	strdhi	r0, [r1], -r4
     5c4:	00000000 	andeq	r0, r0, r0
     5c8:	0000002e 	andeq	r0, r0, lr, lsr #32
     5cc:	800102f4 	strdhi	r0, [r1], -r4
     5d0:	00000000 	andeq	r0, r0, r0
     5d4:	00460044 	subeq	r0, r6, r4, asr #32
	...
     5e0:	00480044 	subeq	r0, r8, r4, asr #32
     5e4:	0000000c 	andeq	r0, r0, ip
     5e8:	00000000 	andeq	r0, r0, r0
     5ec:	00490044 	subeq	r0, r9, r4, asr #32
     5f0:	00000020 	andeq	r0, r0, r0, lsr #32
     5f4:	00000000 	andeq	r0, r0, r0
     5f8:	004a0044 	subeq	r0, sl, r4, asr #32
     5fc:	00000024 	andeq	r0, r0, r4, lsr #32
     600:	00000000 	andeq	r0, r0, r0
     604:	004b0044 	subeq	r0, fp, r4, asr #32
     608:	00000028 	andeq	r0, r0, r8, lsr #32
     60c:	00000000 	andeq	r0, r0, r0
     610:	00000024 	andeq	r0, r0, r4, lsr #32
     614:	00000030 	andeq	r0, r0, r0, lsr r0
     618:	00000000 	andeq	r0, r0, r0
     61c:	0000004e 	andeq	r0, r0, lr, asr #32
     620:	80010324 	andhi	r0, r1, r4, lsr #6
     624:	00000a42 	andeq	r0, r0, r2, asr #20
     628:	004d0024 	subeq	r0, sp, r4, lsr #32
     62c:	80010324 	andhi	r0, r1, r4, lsr #6
     630:	00000a54 	andeq	r0, r0, r4, asr sl
     634:	004d00a0 	subeq	r0, sp, r0, lsr #1
     638:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
     63c:	00000000 	andeq	r0, r0, r0
     640:	0000002e 	andeq	r0, r0, lr, lsr #32
     644:	80010324 	andhi	r0, r1, r4, lsr #6
     648:	00000000 	andeq	r0, r0, r0
     64c:	004d0044 	subeq	r0, sp, r4, asr #32
	...
     658:	004e0044 	subeq	r0, lr, r4, asr #32
     65c:	00000014 	andeq	r0, r0, r4, lsl r0
     660:	00000000 	andeq	r0, r0, r0
     664:	004f0044 	subeq	r0, pc, r4, asr #32
     668:	00000020 	andeq	r0, r0, r0, lsr #32
     66c:	00000000 	andeq	r0, r0, r0
     670:	00500044 	subseq	r0, r0, r4, asr #32
     674:	00000038 	andeq	r0, r0, r8, lsr r0
     678:	00000a60 	andeq	r0, r0, r0, ror #20
     67c:	004e0080 	subeq	r0, lr, r0, lsl #1
     680:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     684:	00000000 	andeq	r0, r0, r0
     688:	000000c0 	andeq	r0, r0, r0, asr #1
	...
     694:	000000e0 	andeq	r0, r0, r0, ror #1
     698:	00000040 	andeq	r0, r0, r0, asr #32
     69c:	00000000 	andeq	r0, r0, r0
     6a0:	00000024 	andeq	r0, r0, r4, lsr #32
     6a4:	00000040 	andeq	r0, r0, r0, asr #32
     6a8:	00000000 	andeq	r0, r0, r0
     6ac:	0000004e 	andeq	r0, r0, lr, asr #32
     6b0:	80010364 	andhi	r0, r1, r4, ror #6
     6b4:	00000a7b 	andeq	r0, r0, fp, ror sl
     6b8:	00520024 	subseq	r0, r2, r4, lsr #32
     6bc:	80010364 	andhi	r0, r1, r4, ror #6
     6c0:	00000a54 	andeq	r0, r0, r4, asr sl
     6c4:	005200a0 	subseq	r0, r2, r0, lsr #1
     6c8:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
     6cc:	00000000 	andeq	r0, r0, r0
     6d0:	0000002e 	andeq	r0, r0, lr, lsr #32
     6d4:	80010364 	andhi	r0, r1, r4, ror #6
     6d8:	00000000 	andeq	r0, r0, r0
     6dc:	00520044 	subseq	r0, r2, r4, asr #32
	...
     6e8:	00530044 	subseq	r0, r3, r4, asr #32
     6ec:	00000014 	andeq	r0, r0, r4, lsl r0
     6f0:	00000000 	andeq	r0, r0, r0
     6f4:	00540044 	subseq	r0, r4, r4, asr #32
     6f8:	00000020 	andeq	r0, r0, r0, lsr #32
     6fc:	00000000 	andeq	r0, r0, r0
     700:	00550044 	subseq	r0, r5, r4, asr #32
     704:	00000038 	andeq	r0, r0, r8, lsr r0
     708:	00000a8e 	andeq	r0, r0, lr, lsl #21
     70c:	00530080 	subseq	r0, r3, r0, lsl #1
     710:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     714:	00000000 	andeq	r0, r0, r0
     718:	000000c0 	andeq	r0, r0, r0, asr #1
	...
     724:	000000e0 	andeq	r0, r0, r0, ror #1
     728:	00000040 	andeq	r0, r0, r0, asr #32
     72c:	00000000 	andeq	r0, r0, r0
     730:	00000024 	andeq	r0, r0, r4, lsr #32
     734:	00000040 	andeq	r0, r0, r0, asr #32
     738:	00000000 	andeq	r0, r0, r0
     73c:	0000004e 	andeq	r0, r0, lr, asr #32
     740:	800103a4 	andhi	r0, r1, r4, lsr #7
     744:	00000a9a 	muleq	r0, sl, sl
     748:	00570024 	subseq	r0, r7, r4, lsr #32
     74c:	800103a4 	andhi	r0, r1, r4, lsr #7
     750:	00000ab1 			; <UNDEFINED> instruction: 0x00000ab1
     754:	005700a0 	subseq	r0, r7, r0, lsr #1
     758:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
     75c:	00000000 	andeq	r0, r0, r0
     760:	0000002e 	andeq	r0, r0, lr, lsr #32
     764:	800103a4 	andhi	r0, r1, r4, lsr #7
     768:	00000000 	andeq	r0, r0, r0
     76c:	00570044 	subseq	r0, r7, r4, asr #32
	...
     778:	00580044 	subseq	r0, r8, r4, asr #32
     77c:	00000014 	andeq	r0, r0, r4, lsl r0
     780:	00000000 	andeq	r0, r0, r0
     784:	00590044 	subseq	r0, r9, r4, asr #32
     788:	00000020 	andeq	r0, r0, r0, lsr #32
     78c:	00000000 	andeq	r0, r0, r0
     790:	005a0044 	subseq	r0, sl, r4, asr #32
     794:	0000002c 	andeq	r0, r0, ip, lsr #32
     798:	00000000 	andeq	r0, r0, r0
     79c:	005c0044 	subseq	r0, ip, r4, asr #32
     7a0:	0000003c 	andeq	r0, r0, ip, lsr r0
     7a4:	00000000 	andeq	r0, r0, r0
     7a8:	005d0044 	subseq	r0, sp, r4, asr #32
     7ac:	00000050 	andeq	r0, r0, r0, asr r0
     7b0:	00000000 	andeq	r0, r0, r0
     7b4:	005e0044 	subseq	r0, lr, r4, asr #32
     7b8:	00000064 	andeq	r0, r0, r4, rrx
     7bc:	00000000 	andeq	r0, r0, r0
     7c0:	005f0044 	subseq	r0, pc, r4, asr #32
     7c4:	00000080 	andeq	r0, r0, r0, lsl #1
     7c8:	00000000 	andeq	r0, r0, r0
     7cc:	00620044 	rsbeq	r0, r2, r4, asr #32
     7d0:	00000084 	andeq	r0, r0, r4, lsl #1
     7d4:	00000000 	andeq	r0, r0, r0
     7d8:	00630044 	rsbeq	r0, r3, r4, asr #32
     7dc:	00000090 	muleq	r0, r0, r0
     7e0:	00000000 	andeq	r0, r0, r0
     7e4:	00640044 	rsbeq	r0, r4, r4, asr #32
     7e8:	000000a8 	andeq	r0, r0, r8, lsr #1
     7ec:	00000000 	andeq	r0, r0, r0
     7f0:	00620044 	rsbeq	r0, r2, r4, asr #32
     7f4:	000000c0 	andeq	r0, r0, r0, asr #1
     7f8:	00000000 	andeq	r0, r0, r0
     7fc:	00620044 	rsbeq	r0, r2, r4, asr #32
     800:	000000cc 	andeq	r0, r0, ip, asr #1
     804:	00000000 	andeq	r0, r0, r0
     808:	00650044 	rsbeq	r0, r5, r4, asr #32
     80c:	000000d8 	ldrdeq	r0, [r0], -r8
     810:	00000a8e 	andeq	r0, r0, lr, lsl #21
     814:	00580080 	subseq	r0, r8, r0, lsl #1
     818:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
     81c:	00000ac7 	andeq	r0, r0, r7, asr #21
     820:	00590080 	subseq	r0, r9, r0, lsl #1
     824:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
     828:	00000000 	andeq	r0, r0, r0
     82c:	000000c0 	andeq	r0, r0, r0, asr #1
     830:	00000000 	andeq	r0, r0, r0
     834:	00000ad8 	ldrdeq	r0, [r0], -r8
     838:	00620080 	rsbeq	r0, r2, r0, lsl #1
     83c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     840:	00000000 	andeq	r0, r0, r0
     844:	000000c0 	andeq	r0, r0, r0, asr #1
     848:	00000084 	andeq	r0, r0, r4, lsl #1
     84c:	00000000 	andeq	r0, r0, r0
     850:	000000e0 	andeq	r0, r0, r0, ror #1
     854:	000000d8 	ldrdeq	r0, [r0], -r8
     858:	00000000 	andeq	r0, r0, r0
     85c:	000000e0 	andeq	r0, r0, r0, ror #1
     860:	000000e4 	andeq	r0, r0, r4, ror #1
     864:	00000000 	andeq	r0, r0, r0
     868:	00000024 	andeq	r0, r0, r4, lsr #32
     86c:	000000e4 	andeq	r0, r0, r4, ror #1
     870:	00000000 	andeq	r0, r0, r0
     874:	0000004e 	andeq	r0, r0, lr, asr #32
     878:	80010488 	andhi	r0, r1, r8, lsl #9
     87c:	00000ae0 	andeq	r0, r0, r0, ror #21
     880:	00210026 	eoreq	r0, r1, r6, lsr #32
     884:	80018000 	andhi	r8, r1, r0
     888:	00000000 	andeq	r0, r0, r0
     88c:	00000064 	andeq	r0, r0, r4, rrx
     890:	80010488 	andhi	r0, r1, r8, lsl #9
     894:	00000007 	andeq	r0, r0, r7
     898:	00020064 	andeq	r0, r2, r4, rrx
     89c:	80010488 	andhi	r0, r1, r8, lsl #9
     8a0:	00000aee 	andeq	r0, r0, lr, ror #21
     8a4:	00020064 	andeq	r0, r2, r4, rrx
     8a8:	80010488 	andhi	r0, r1, r8, lsl #9
     8ac:	0000003d 	andeq	r0, r0, sp, lsr r0
     8b0:	0000003c 	andeq	r0, r0, ip, lsr r0
     8b4:	00000000 	andeq	r0, r0, r0
     8b8:	0000004c 	andeq	r0, r0, ip, asr #32
     8bc:	00000080 	andeq	r0, r0, r0, lsl #1
     8c0:	00000000 	andeq	r0, r0, r0
     8c4:	00000076 	andeq	r0, r0, r6, ror r0
     8c8:	00000080 	andeq	r0, r0, r0, lsl #1
     8cc:	00000000 	andeq	r0, r0, r0
     8d0:	00000094 	muleq	r0, r4, r0
     8d4:	00000080 	andeq	r0, r0, r0, lsl #1
     8d8:	00000000 	andeq	r0, r0, r0
     8dc:	000000c3 	andeq	r0, r0, r3, asr #1
     8e0:	00000080 	andeq	r0, r0, r0, lsl #1
     8e4:	00000000 	andeq	r0, r0, r0
     8e8:	000000ee 	andeq	r0, r0, lr, ror #1
     8ec:	00000080 	andeq	r0, r0, r0, lsl #1
     8f0:	00000000 	andeq	r0, r0, r0
     8f4:	0000011e 	andeq	r0, r0, lr, lsl r1
     8f8:	00000080 	andeq	r0, r0, r0, lsl #1
     8fc:	00000000 	andeq	r0, r0, r0
     900:	0000016f 	andeq	r0, r0, pc, ror #2
     904:	00000080 	andeq	r0, r0, r0, lsl #1
     908:	00000000 	andeq	r0, r0, r0
     90c:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
     910:	00000080 	andeq	r0, r0, r0, lsl #1
     914:	00000000 	andeq	r0, r0, r0
     918:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
     91c:	00000080 	andeq	r0, r0, r0, lsl #1
     920:	00000000 	andeq	r0, r0, r0
     924:	0000020e 	andeq	r0, r0, lr, lsl #4
     928:	00000080 	andeq	r0, r0, r0, lsl #1
     92c:	00000000 	andeq	r0, r0, r0
     930:	00000238 	andeq	r0, r0, r8, lsr r2
     934:	00000080 	andeq	r0, r0, r0, lsl #1
     938:	00000000 	andeq	r0, r0, r0
     93c:	00000261 	andeq	r0, r0, r1, ror #4
     940:	00000080 	andeq	r0, r0, r0, lsl #1
     944:	00000000 	andeq	r0, r0, r0
     948:	0000027b 	andeq	r0, r0, fp, ror r2
     94c:	00000080 	andeq	r0, r0, r0, lsl #1
     950:	00000000 	andeq	r0, r0, r0
     954:	00000296 	muleq	r0, r6, r2
     958:	00000080 	andeq	r0, r0, r0, lsl #1
     95c:	00000000 	andeq	r0, r0, r0
     960:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
     964:	00000080 	andeq	r0, r0, r0, lsl #1
     968:	00000000 	andeq	r0, r0, r0
     96c:	000002d7 	ldrdeq	r0, [r0], -r7
     970:	00000080 	andeq	r0, r0, r0, lsl #1
     974:	00000000 	andeq	r0, r0, r0
     978:	000002f7 	strdeq	r0, [r0], -r7
     97c:	00000080 	andeq	r0, r0, r0, lsl #1
     980:	00000000 	andeq	r0, r0, r0
     984:	0000031c 	andeq	r0, r0, ip, lsl r3
     988:	00000080 	andeq	r0, r0, r0, lsl #1
     98c:	00000000 	andeq	r0, r0, r0
     990:	00000346 	andeq	r0, r0, r6, asr #6
     994:	00000080 	andeq	r0, r0, r0, lsl #1
     998:	00000000 	andeq	r0, r0, r0
     99c:	0000036a 	andeq	r0, r0, sl, ror #6
     9a0:	00000080 	andeq	r0, r0, r0, lsl #1
     9a4:	00000000 	andeq	r0, r0, r0
     9a8:	00000393 	muleq	r0, r3, r3
     9ac:	00000080 	andeq	r0, r0, r0, lsl #1
     9b0:	00000000 	andeq	r0, r0, r0
     9b4:	000003c1 	andeq	r0, r0, r1, asr #7
     9b8:	00000080 	andeq	r0, r0, r0, lsl #1
     9bc:	00000000 	andeq	r0, r0, r0
     9c0:	000003e7 	andeq	r0, r0, r7, ror #7
     9c4:	00000080 	andeq	r0, r0, r0, lsl #1
     9c8:	00000000 	andeq	r0, r0, r0
     9cc:	00000407 	andeq	r0, r0, r7, lsl #8
     9d0:	00000080 	andeq	r0, r0, r0, lsl #1
     9d4:	00000000 	andeq	r0, r0, r0
     9d8:	0000042c 	andeq	r0, r0, ip, lsr #8
     9dc:	00000080 	andeq	r0, r0, r0, lsl #1
     9e0:	00000000 	andeq	r0, r0, r0
     9e4:	00000456 	andeq	r0, r0, r6, asr r4
     9e8:	00000080 	andeq	r0, r0, r0, lsl #1
     9ec:	00000000 	andeq	r0, r0, r0
     9f0:	00000485 	andeq	r0, r0, r5, lsl #9
     9f4:	00000080 	andeq	r0, r0, r0, lsl #1
     9f8:	00000000 	andeq	r0, r0, r0
     9fc:	000004ae 	andeq	r0, r0, lr, lsr #9
     a00:	00000080 	andeq	r0, r0, r0, lsl #1
     a04:	00000000 	andeq	r0, r0, r0
     a08:	000004dc 	ldrdeq	r0, [r0], -ip
     a0c:	00000080 	andeq	r0, r0, r0, lsl #1
     a10:	00000000 	andeq	r0, r0, r0
     a14:	0000050f 	andeq	r0, r0, pc, lsl #10
     a18:	00000080 	andeq	r0, r0, r0, lsl #1
     a1c:	00000000 	andeq	r0, r0, r0
     a20:	00000530 	andeq	r0, r0, r0, lsr r5
     a24:	00000080 	andeq	r0, r0, r0, lsl #1
     a28:	00000000 	andeq	r0, r0, r0
     a2c:	00000550 	andeq	r0, r0, r0, asr r5
     a30:	00000080 	andeq	r0, r0, r0, lsl #1
     a34:	00000000 	andeq	r0, r0, r0
     a38:	00000575 	andeq	r0, r0, r5, ror r5
     a3c:	00000080 	andeq	r0, r0, r0, lsl #1
     a40:	00000000 	andeq	r0, r0, r0
     a44:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
     a48:	00000080 	andeq	r0, r0, r0, lsl #1
     a4c:	00000000 	andeq	r0, r0, r0
     a50:	000005c3 	andeq	r0, r0, r3, asr #11
     a54:	00000080 	andeq	r0, r0, r0, lsl #1
     a58:	00000000 	andeq	r0, r0, r0
     a5c:	000005ec 	andeq	r0, r0, ip, ror #11
     a60:	00000080 	andeq	r0, r0, r0, lsl #1
     a64:	00000000 	andeq	r0, r0, r0
     a68:	0000061a 	andeq	r0, r0, sl, lsl r6
     a6c:	00000080 	andeq	r0, r0, r0, lsl #1
     a70:	00000000 	andeq	r0, r0, r0
     a74:	00000640 	andeq	r0, r0, r0, asr #12
     a78:	00000080 	andeq	r0, r0, r0, lsl #1
     a7c:	00000000 	andeq	r0, r0, r0
     a80:	00000660 	andeq	r0, r0, r0, ror #12
     a84:	00000080 	andeq	r0, r0, r0, lsl #1
     a88:	00000000 	andeq	r0, r0, r0
     a8c:	00000685 	andeq	r0, r0, r5, lsl #13
     a90:	00000080 	andeq	r0, r0, r0, lsl #1
     a94:	00000000 	andeq	r0, r0, r0
     a98:	000006af 	andeq	r0, r0, pc, lsr #13
     a9c:	00000080 	andeq	r0, r0, r0, lsl #1
     aa0:	00000000 	andeq	r0, r0, r0
     aa4:	000006de 	ldrdeq	r0, [r0], -lr
     aa8:	00000080 	andeq	r0, r0, r0, lsl #1
     aac:	00000000 	andeq	r0, r0, r0
     ab0:	00000707 	andeq	r0, r0, r7, lsl #14
     ab4:	00000080 	andeq	r0, r0, r0, lsl #1
     ab8:	00000000 	andeq	r0, r0, r0
     abc:	00000735 	andeq	r0, r0, r5, lsr r7
     ac0:	00000080 	andeq	r0, r0, r0, lsl #1
     ac4:	00000000 	andeq	r0, r0, r0
     ac8:	00000768 	andeq	r0, r0, r8, ror #14
     acc:	00000080 	andeq	r0, r0, r0, lsl #1
     ad0:	00000000 	andeq	r0, r0, r0
     ad4:	00000b05 	andeq	r0, r0, r5, lsl #22
     ad8:	00000082 	andeq	r0, r0, r2, lsl #1
     adc:	00000000 	andeq	r0, r0, r0
     ae0:	0000078a 	andeq	r0, r0, sl, lsl #15
     ae4:	000000c2 	andeq	r0, r0, r2, asr #1
     ae8:	00002d60 	andeq	r2, r0, r0, ror #26
     aec:	00000000 	andeq	r0, r0, r0
     af0:	000000a2 	andeq	r0, r0, r2, lsr #1
     af4:	00000000 	andeq	r0, r0, r0
     af8:	00000886 	andeq	r0, r0, r6, lsl #17
     afc:	000000c2 	andeq	r0, r0, r2, asr #1
     b00:	00003ac8 	andeq	r3, r0, r8, asr #21
     b04:	00000b18 	andeq	r0, r0, r8, lsl fp
     b08:	00000080 	andeq	r0, r0, r0, lsl #1
     b0c:	00000000 	andeq	r0, r0, r0
     b10:	00000cc1 	andeq	r0, r0, r1, asr #25
     b14:	002a0024 	eoreq	r0, sl, r4, lsr #32
     b18:	80010488 	andhi	r0, r1, r8, lsl #9
     b1c:	00000ccf 	andeq	r0, r0, pc, asr #25
     b20:	002a00a0 	eoreq	r0, sl, r0, lsr #1
     b24:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
     b28:	00000000 	andeq	r0, r0, r0
     b2c:	0000002e 	andeq	r0, r0, lr, lsr #32
     b30:	80010488 	andhi	r0, r1, r8, lsl #9
     b34:	00000000 	andeq	r0, r0, r0
     b38:	002a0044 	eoreq	r0, sl, r4, asr #32
	...
     b44:	002b0044 	eoreq	r0, fp, r4, asr #32
     b48:	00000014 	andeq	r0, r0, r4, lsl r0
     b4c:	00000000 	andeq	r0, r0, r0
     b50:	002d0044 	eoreq	r0, sp, r4, asr #32
     b54:	00000020 	andeq	r0, r0, r0, lsr #32
     b58:	00000000 	andeq	r0, r0, r0
     b5c:	00000024 	andeq	r0, r0, r4, lsr #32
     b60:	00000028 	andeq	r0, r0, r8, lsr #32
     b64:	00000000 	andeq	r0, r0, r0
     b68:	0000004e 	andeq	r0, r0, lr, asr #32
     b6c:	800104b0 			; <UNDEFINED> instruction: 0x800104b0
     b70:	00000cdc 	ldrdeq	r0, [r0], -ip
     b74:	002f0024 	eoreq	r0, pc, r4, lsr #32
     b78:	800104b0 			; <UNDEFINED> instruction: 0x800104b0
     b7c:	00000000 	andeq	r0, r0, r0
     b80:	0000002e 	andeq	r0, r0, lr, lsr #32
     b84:	800104b0 			; <UNDEFINED> instruction: 0x800104b0
     b88:	00000000 	andeq	r0, r0, r0
     b8c:	002f0044 	eoreq	r0, pc, r4, asr #32
	...
     b98:	00310044 	eorseq	r0, r1, r4, asr #32
     b9c:	0000000c 	andeq	r0, r0, ip
     ba0:	00000000 	andeq	r0, r0, r0
     ba4:	00350044 	eorseq	r0, r5, r4, asr #32
     ba8:	0000001c 	andeq	r0, r0, ip, lsl r0
     bac:	00000000 	andeq	r0, r0, r0
     bb0:	00360044 	eorseq	r0, r6, r4, asr #32
     bb4:	0000002c 	andeq	r0, r0, ip, lsr #32
     bb8:	00000000 	andeq	r0, r0, r0
     bbc:	00390044 	eorseq	r0, r9, r4, asr #32
     bc0:	00000034 	andeq	r0, r0, r4, lsr r0
     bc4:	00000000 	andeq	r0, r0, r0
     bc8:	003a0044 	eorseq	r0, sl, r4, asr #32
     bcc:	00000044 	andeq	r0, r0, r4, asr #32
     bd0:	00000000 	andeq	r0, r0, r0
     bd4:	003d0044 	eorseq	r0, sp, r4, asr #32
     bd8:	0000004c 	andeq	r0, r0, ip, asr #32
     bdc:	00000000 	andeq	r0, r0, r0
     be0:	00400044 	subeq	r0, r0, r4, asr #32
     be4:	0000005c 	andeq	r0, r0, ip, asr r0
     be8:	00000000 	andeq	r0, r0, r0
     bec:	00480044 	subeq	r0, r8, r4, asr #32
     bf0:	0000006c 	andeq	r0, r0, ip, rrx
     bf4:	00000000 	andeq	r0, r0, r0
     bf8:	004a0044 	subeq	r0, sl, r4, asr #32
     bfc:	0000007c 	andeq	r0, r0, ip, ror r0
     c00:	00000000 	andeq	r0, r0, r0
     c04:	004d0044 	subeq	r0, sp, r4, asr #32
     c08:	0000008c 	andeq	r0, r0, ip, lsl #1
     c0c:	00000000 	andeq	r0, r0, r0
     c10:	00500044 	subseq	r0, r0, r4, asr #32
     c14:	0000009c 	muleq	r0, ip, r0
     c18:	00000000 	andeq	r0, r0, r0
     c1c:	00540044 	subseq	r0, r4, r4, asr #32
     c20:	000000ac 	andeq	r0, r0, ip, lsr #1
     c24:	00000000 	andeq	r0, r0, r0
     c28:	00550044 	subseq	r0, r5, r4, asr #32
     c2c:	000000bc 	strheq	r0, [r0], -ip
     c30:	00000000 	andeq	r0, r0, r0
     c34:	00000024 	andeq	r0, r0, r4, lsr #32
     c38:	000000c0 	andeq	r0, r0, r0, asr #1
     c3c:	00000000 	andeq	r0, r0, r0
     c40:	0000004e 	andeq	r0, r0, lr, asr #32
     c44:	80010570 	andhi	r0, r1, r0, ror r5
     c48:	00000cf0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
     c4c:	00570024 	subseq	r0, r7, r4, lsr #32
     c50:	80010570 	andhi	r0, r1, r0, ror r5
     c54:	00000d05 	andeq	r0, r0, r5, lsl #26
     c58:	005700a0 	subseq	r0, r7, r0, lsr #1
     c5c:	fffffff3 			; <UNDEFINED> instruction: 0xfffffff3
     c60:	00000000 	andeq	r0, r0, r0
     c64:	0000002e 	andeq	r0, r0, lr, lsr #32
     c68:	80010570 	andhi	r0, r1, r0, ror r5
     c6c:	00000000 	andeq	r0, r0, r0
     c70:	00570044 	subseq	r0, r7, r4, asr #32
	...
     c7c:	00590044 	subseq	r0, r9, r4, asr #32
     c80:	00000018 	andeq	r0, r0, r8, lsl r0
     c84:	00000000 	andeq	r0, r0, r0
     c88:	00590044 	subseq	r0, r9, r4, asr #32
     c8c:	0000001c 	andeq	r0, r0, ip, lsl r0
     c90:	00000000 	andeq	r0, r0, r0
     c94:	005a0044 	subseq	r0, sl, r4, asr #32
     c98:	00000034 	andeq	r0, r0, r4, lsr r0
     c9c:	00000000 	andeq	r0, r0, r0
     ca0:	005b0044 	subseq	r0, fp, r4, asr #32
     ca4:	00000044 	andeq	r0, r0, r4, asr #32
     ca8:	00000000 	andeq	r0, r0, r0
     cac:	00000024 	andeq	r0, r0, r4, lsr #32
     cb0:	0000004c 	andeq	r0, r0, ip, asr #32
     cb4:	00000000 	andeq	r0, r0, r0
     cb8:	0000004e 	andeq	r0, r0, lr, asr #32
     cbc:	800105bc 			; <UNDEFINED> instruction: 0x800105bc
     cc0:	00000d0e 	andeq	r0, r0, lr, lsl #26
     cc4:	005d0024 	subseq	r0, sp, r4, lsr #32
     cc8:	800105bc 			; <UNDEFINED> instruction: 0x800105bc
     ccc:	00000000 	andeq	r0, r0, r0
     cd0:	0000002e 	andeq	r0, r0, lr, lsr #32
     cd4:	800105bc 			; <UNDEFINED> instruction: 0x800105bc
     cd8:	00000000 	andeq	r0, r0, r0
     cdc:	005d0044 	subseq	r0, sp, r4, asr #32
	...
     ce8:	005e0044 	subseq	r0, lr, r4, asr #32
     cec:	0000000c 	andeq	r0, r0, ip
     cf0:	00000000 	andeq	r0, r0, r0
     cf4:	005f0044 	subseq	r0, pc, r4, asr #32
     cf8:	00000018 	andeq	r0, r0, r8, lsl r0
     cfc:	00000000 	andeq	r0, r0, r0
     d00:	00000024 	andeq	r0, r0, r4, lsr #32
     d04:	00000020 	andeq	r0, r0, r0, lsr #32
     d08:	00000000 	andeq	r0, r0, r0
     d0c:	0000004e 	andeq	r0, r0, lr, asr #32
     d10:	800105dc 	ldrdhi	r0, [r1], -ip
     d14:	00000d21 	andeq	r0, r0, r1, lsr #26
     d18:	00610024 	rsbeq	r0, r1, r4, lsr #32
     d1c:	800105dc 	ldrdhi	r0, [r1], -ip
     d20:	00000000 	andeq	r0, r0, r0
     d24:	0000002e 	andeq	r0, r0, lr, lsr #32
     d28:	800105dc 	ldrdhi	r0, [r1], -ip
     d2c:	00000000 	andeq	r0, r0, r0
     d30:	00610044 	rsbeq	r0, r1, r4, asr #32
	...
     d3c:	00620044 	rsbeq	r0, r2, r4, asr #32
     d40:	0000000c 	andeq	r0, r0, ip
     d44:	00000000 	andeq	r0, r0, r0
     d48:	00630044 	rsbeq	r0, r3, r4, asr #32
     d4c:	0000002c 	andeq	r0, r0, ip, lsr #32
     d50:	00000000 	andeq	r0, r0, r0
     d54:	00000024 	andeq	r0, r0, r4, lsr #32
     d58:	00000034 	andeq	r0, r0, r4, lsr r0
     d5c:	00000000 	andeq	r0, r0, r0
     d60:	0000004e 	andeq	r0, r0, lr, asr #32
     d64:	80010610 	andhi	r0, r1, r0, lsl r6
     d68:	00000000 	andeq	r0, r0, r0
     d6c:	00000064 	andeq	r0, r0, r4, rrx
     d70:	80010610 	andhi	r0, r1, r0, lsl r6
     d74:	00000007 	andeq	r0, r0, r7
     d78:	00020064 	andeq	r0, r2, r4, rrx
     d7c:	80010610 	andhi	r0, r1, r0, lsl r6
     d80:	00000d38 	andeq	r0, r0, r8, lsr sp
     d84:	00020064 	andeq	r0, r2, r4, rrx
     d88:	80010610 	andhi	r0, r1, r0, lsl r6
     d8c:	0000003d 	andeq	r0, r0, sp, lsr r0
     d90:	0000003c 	andeq	r0, r0, ip, lsr r0
     d94:	00000000 	andeq	r0, r0, r0
     d98:	0000004c 	andeq	r0, r0, ip, asr #32
     d9c:	00000080 	andeq	r0, r0, r0, lsl #1
     da0:	00000000 	andeq	r0, r0, r0
     da4:	00000076 	andeq	r0, r0, r6, ror r0
     da8:	00000080 	andeq	r0, r0, r0, lsl #1
     dac:	00000000 	andeq	r0, r0, r0
     db0:	00000094 	muleq	r0, r4, r0
     db4:	00000080 	andeq	r0, r0, r0, lsl #1
     db8:	00000000 	andeq	r0, r0, r0
     dbc:	000000c3 	andeq	r0, r0, r3, asr #1
     dc0:	00000080 	andeq	r0, r0, r0, lsl #1
     dc4:	00000000 	andeq	r0, r0, r0
     dc8:	000000ee 	andeq	r0, r0, lr, ror #1
     dcc:	00000080 	andeq	r0, r0, r0, lsl #1
     dd0:	00000000 	andeq	r0, r0, r0
     dd4:	0000011e 	andeq	r0, r0, lr, lsl r1
     dd8:	00000080 	andeq	r0, r0, r0, lsl #1
     ddc:	00000000 	andeq	r0, r0, r0
     de0:	0000016f 	andeq	r0, r0, pc, ror #2
     de4:	00000080 	andeq	r0, r0, r0, lsl #1
     de8:	00000000 	andeq	r0, r0, r0
     dec:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
     df0:	00000080 	andeq	r0, r0, r0, lsl #1
     df4:	00000000 	andeq	r0, r0, r0
     df8:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
     dfc:	00000080 	andeq	r0, r0, r0, lsl #1
     e00:	00000000 	andeq	r0, r0, r0
     e04:	0000020e 	andeq	r0, r0, lr, lsl #4
     e08:	00000080 	andeq	r0, r0, r0, lsl #1
     e0c:	00000000 	andeq	r0, r0, r0
     e10:	00000238 	andeq	r0, r0, r8, lsr r2
     e14:	00000080 	andeq	r0, r0, r0, lsl #1
     e18:	00000000 	andeq	r0, r0, r0
     e1c:	00000261 	andeq	r0, r0, r1, ror #4
     e20:	00000080 	andeq	r0, r0, r0, lsl #1
     e24:	00000000 	andeq	r0, r0, r0
     e28:	0000027b 	andeq	r0, r0, fp, ror r2
     e2c:	00000080 	andeq	r0, r0, r0, lsl #1
     e30:	00000000 	andeq	r0, r0, r0
     e34:	00000296 	muleq	r0, r6, r2
     e38:	00000080 	andeq	r0, r0, r0, lsl #1
     e3c:	00000000 	andeq	r0, r0, r0
     e40:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
     e44:	00000080 	andeq	r0, r0, r0, lsl #1
     e48:	00000000 	andeq	r0, r0, r0
     e4c:	000002d7 	ldrdeq	r0, [r0], -r7
     e50:	00000080 	andeq	r0, r0, r0, lsl #1
     e54:	00000000 	andeq	r0, r0, r0
     e58:	000002f7 	strdeq	r0, [r0], -r7
     e5c:	00000080 	andeq	r0, r0, r0, lsl #1
     e60:	00000000 	andeq	r0, r0, r0
     e64:	0000031c 	andeq	r0, r0, ip, lsl r3
     e68:	00000080 	andeq	r0, r0, r0, lsl #1
     e6c:	00000000 	andeq	r0, r0, r0
     e70:	00000346 	andeq	r0, r0, r6, asr #6
     e74:	00000080 	andeq	r0, r0, r0, lsl #1
     e78:	00000000 	andeq	r0, r0, r0
     e7c:	0000036a 	andeq	r0, r0, sl, ror #6
     e80:	00000080 	andeq	r0, r0, r0, lsl #1
     e84:	00000000 	andeq	r0, r0, r0
     e88:	00000393 	muleq	r0, r3, r3
     e8c:	00000080 	andeq	r0, r0, r0, lsl #1
     e90:	00000000 	andeq	r0, r0, r0
     e94:	000003c1 	andeq	r0, r0, r1, asr #7
     e98:	00000080 	andeq	r0, r0, r0, lsl #1
     e9c:	00000000 	andeq	r0, r0, r0
     ea0:	000003e7 	andeq	r0, r0, r7, ror #7
     ea4:	00000080 	andeq	r0, r0, r0, lsl #1
     ea8:	00000000 	andeq	r0, r0, r0
     eac:	00000407 	andeq	r0, r0, r7, lsl #8
     eb0:	00000080 	andeq	r0, r0, r0, lsl #1
     eb4:	00000000 	andeq	r0, r0, r0
     eb8:	0000042c 	andeq	r0, r0, ip, lsr #8
     ebc:	00000080 	andeq	r0, r0, r0, lsl #1
     ec0:	00000000 	andeq	r0, r0, r0
     ec4:	00000456 	andeq	r0, r0, r6, asr r4
     ec8:	00000080 	andeq	r0, r0, r0, lsl #1
     ecc:	00000000 	andeq	r0, r0, r0
     ed0:	00000485 	andeq	r0, r0, r5, lsl #9
     ed4:	00000080 	andeq	r0, r0, r0, lsl #1
     ed8:	00000000 	andeq	r0, r0, r0
     edc:	000004ae 	andeq	r0, r0, lr, lsr #9
     ee0:	00000080 	andeq	r0, r0, r0, lsl #1
     ee4:	00000000 	andeq	r0, r0, r0
     ee8:	000004dc 	ldrdeq	r0, [r0], -ip
     eec:	00000080 	andeq	r0, r0, r0, lsl #1
     ef0:	00000000 	andeq	r0, r0, r0
     ef4:	0000050f 	andeq	r0, r0, pc, lsl #10
     ef8:	00000080 	andeq	r0, r0, r0, lsl #1
     efc:	00000000 	andeq	r0, r0, r0
     f00:	00000530 	andeq	r0, r0, r0, lsr r5
     f04:	00000080 	andeq	r0, r0, r0, lsl #1
     f08:	00000000 	andeq	r0, r0, r0
     f0c:	00000550 	andeq	r0, r0, r0, asr r5
     f10:	00000080 	andeq	r0, r0, r0, lsl #1
     f14:	00000000 	andeq	r0, r0, r0
     f18:	00000575 	andeq	r0, r0, r5, ror r5
     f1c:	00000080 	andeq	r0, r0, r0, lsl #1
     f20:	00000000 	andeq	r0, r0, r0
     f24:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
     f28:	00000080 	andeq	r0, r0, r0, lsl #1
     f2c:	00000000 	andeq	r0, r0, r0
     f30:	000005c3 	andeq	r0, r0, r3, asr #11
     f34:	00000080 	andeq	r0, r0, r0, lsl #1
     f38:	00000000 	andeq	r0, r0, r0
     f3c:	000005ec 	andeq	r0, r0, ip, ror #11
     f40:	00000080 	andeq	r0, r0, r0, lsl #1
     f44:	00000000 	andeq	r0, r0, r0
     f48:	0000061a 	andeq	r0, r0, sl, lsl r6
     f4c:	00000080 	andeq	r0, r0, r0, lsl #1
     f50:	00000000 	andeq	r0, r0, r0
     f54:	00000640 	andeq	r0, r0, r0, asr #12
     f58:	00000080 	andeq	r0, r0, r0, lsl #1
     f5c:	00000000 	andeq	r0, r0, r0
     f60:	00000660 	andeq	r0, r0, r0, ror #12
     f64:	00000080 	andeq	r0, r0, r0, lsl #1
     f68:	00000000 	andeq	r0, r0, r0
     f6c:	00000685 	andeq	r0, r0, r5, lsl #13
     f70:	00000080 	andeq	r0, r0, r0, lsl #1
     f74:	00000000 	andeq	r0, r0, r0
     f78:	000006af 	andeq	r0, r0, pc, lsr #13
     f7c:	00000080 	andeq	r0, r0, r0, lsl #1
     f80:	00000000 	andeq	r0, r0, r0
     f84:	000006de 	ldrdeq	r0, [r0], -lr
     f88:	00000080 	andeq	r0, r0, r0, lsl #1
     f8c:	00000000 	andeq	r0, r0, r0
     f90:	00000707 	andeq	r0, r0, r7, lsl #14
     f94:	00000080 	andeq	r0, r0, r0, lsl #1
     f98:	00000000 	andeq	r0, r0, r0
     f9c:	00000735 	andeq	r0, r0, r5, lsr r7
     fa0:	00000080 	andeq	r0, r0, r0, lsl #1
     fa4:	00000000 	andeq	r0, r0, r0
     fa8:	00000768 	andeq	r0, r0, r8, ror #14
     fac:	00000080 	andeq	r0, r0, r0, lsl #1
     fb0:	00000000 	andeq	r0, r0, r0
     fb4:	00000d50 	andeq	r0, r0, r0, asr sp
     fb8:	00000082 	andeq	r0, r0, r2, lsl #1
     fbc:	00000000 	andeq	r0, r0, r0
     fc0:	0000078a 	andeq	r0, r0, sl, lsl #15
     fc4:	000000c2 	andeq	r0, r0, r2, asr #1
     fc8:	00002d60 	andeq	r2, r0, r0, ror #26
     fcc:	00000000 	andeq	r0, r0, r0
     fd0:	000000a2 	andeq	r0, r0, r2, lsr #1
     fd4:	00000000 	andeq	r0, r0, r0
     fd8:	00000886 	andeq	r0, r0, r6, lsl #17
     fdc:	000000c2 	andeq	r0, r0, r2, asr #1
     fe0:	00003ac8 	andeq	r3, r0, r8, asr #21
     fe4:	0000077c 	andeq	r0, r0, ip, ror r7
     fe8:	000000c2 	andeq	r0, r0, r2, asr #1
     fec:	00000bc7 	andeq	r0, r0, r7, asr #23
     ff0:	00000d60 	andeq	r0, r0, r0, ror #26
     ff4:	00150024 	andseq	r0, r5, r4, lsr #32
     ff8:	80010610 	andhi	r0, r1, r0, lsl r6
     ffc:	00000d79 	andeq	r0, r0, r9, ror sp
    1000:	001500a0 	andseq	r0, r5, r0, lsr #1
    1004:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    1008:	00000000 	andeq	r0, r0, r0
    100c:	0000002e 	andeq	r0, r0, lr, lsr #32
    1010:	80010610 	andhi	r0, r1, r0, lsl r6
    1014:	00000000 	andeq	r0, r0, r0
    1018:	00150044 	andseq	r0, r5, r4, asr #32
	...
    1024:	00160044 	andseq	r0, r6, r4, asr #32
    1028:	00000014 	andeq	r0, r0, r4, lsl r0
    102c:	00000000 	andeq	r0, r0, r0
    1030:	00170044 	andseq	r0, r7, r4, asr #32
    1034:	00000020 	andeq	r0, r0, r0, lsr #32
    1038:	00000000 	andeq	r0, r0, r0
    103c:	00180044 	andseq	r0, r8, r4, asr #32
    1040:	00000030 	andeq	r0, r0, r0, lsr r0
    1044:	00000000 	andeq	r0, r0, r0
    1048:	00190044 	andseq	r0, r9, r4, asr #32
    104c:	00000040 	andeq	r0, r0, r0, asr #32
    1050:	00000000 	andeq	r0, r0, r0
    1054:	001a0044 	andseq	r0, sl, r4, asr #32
    1058:	0000004c 	andeq	r0, r0, ip, asr #32
    105c:	00000000 	andeq	r0, r0, r0
    1060:	001b0044 	andseq	r0, fp, r4, asr #32
    1064:	00000060 	andeq	r0, r0, r0, rrx
    1068:	00000d94 	muleq	r0, r4, sp
    106c:	00160080 	andseq	r0, r6, r0, lsl #1
    1070:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    1074:	00000000 	andeq	r0, r0, r0
    1078:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    1084:	000000e0 	andeq	r0, r0, r0, ror #1
    1088:	00000068 	andeq	r0, r0, r8, rrx
    108c:	00000000 	andeq	r0, r0, r0
    1090:	00000024 	andeq	r0, r0, r4, lsr #32
    1094:	00000068 	andeq	r0, r0, r8, rrx
    1098:	00000000 	andeq	r0, r0, r0
    109c:	0000004e 	andeq	r0, r0, lr, asr #32
    10a0:	80010678 	andhi	r0, r1, r8, ror r6
    10a4:	00000db0 			; <UNDEFINED> instruction: 0x00000db0
    10a8:	001d0024 	andseq	r0, sp, r4, lsr #32
    10ac:	80010678 	andhi	r0, r1, r8, ror r6
    10b0:	00000000 	andeq	r0, r0, r0
    10b4:	0000002e 	andeq	r0, r0, lr, lsr #32
    10b8:	80010678 	andhi	r0, r1, r8, ror r6
    10bc:	00000000 	andeq	r0, r0, r0
    10c0:	001d0044 	andseq	r0, sp, r4, asr #32
	...
    10cc:	001e0044 	andseq	r0, lr, r4, asr #32
    10d0:	00000010 	andeq	r0, r0, r0, lsl r0
    10d4:	00000000 	andeq	r0, r0, r0
    10d8:	001f0044 	andseq	r0, pc, r4, asr #32
    10dc:	0000001c 	andeq	r0, r0, ip, lsl r0
    10e0:	00000000 	andeq	r0, r0, r0
    10e4:	00200044 	eoreq	r0, r0, r4, asr #32
    10e8:	0000002c 	andeq	r0, r0, ip, lsr #32
    10ec:	00000dcc 	andeq	r0, r0, ip, asr #27
    10f0:	001e0080 	andseq	r0, lr, r0, lsl #1
    10f4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    10f8:	00000000 	andeq	r0, r0, r0
    10fc:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    1108:	000000e0 	andeq	r0, r0, r0, ror #1
    110c:	00000034 	andeq	r0, r0, r4, lsr r0
    1110:	00000000 	andeq	r0, r0, r0
    1114:	00000024 	andeq	r0, r0, r4, lsr #32
    1118:	00000034 	andeq	r0, r0, r4, lsr r0
    111c:	00000000 	andeq	r0, r0, r0
    1120:	0000004e 	andeq	r0, r0, lr, asr #32
    1124:	800106ac 	andhi	r0, r1, ip, lsr #13
    1128:	00000dd9 	ldrdeq	r0, [r0], -r9
    112c:	00230024 	eoreq	r0, r3, r4, lsr #32
    1130:	800106ac 	andhi	r0, r1, ip, lsr #13
    1134:	00000000 	andeq	r0, r0, r0
    1138:	0000002e 	andeq	r0, r0, lr, lsr #32
    113c:	800106ac 	andhi	r0, r1, ip, lsr #13
    1140:	00000000 	andeq	r0, r0, r0
    1144:	00230044 	eoreq	r0, r3, r4, asr #32
	...
    1150:	00240044 	eoreq	r0, r4, r4, asr #32
    1154:	00000010 	andeq	r0, r0, r0, lsl r0
    1158:	00000000 	andeq	r0, r0, r0
    115c:	00250044 	eoreq	r0, r5, r4, asr #32
    1160:	0000001c 	andeq	r0, r0, ip, lsl r0
    1164:	00000000 	andeq	r0, r0, r0
    1168:	00260044 	eoreq	r0, r6, r4, asr #32
    116c:	00000028 	andeq	r0, r0, r8, lsr #32
    1170:	00000000 	andeq	r0, r0, r0
    1174:	002a0044 	eoreq	r0, sl, r4, asr #32
    1178:	00000038 	andeq	r0, r0, r8, lsr r0
    117c:	00000000 	andeq	r0, r0, r0
    1180:	002b0044 	eoreq	r0, fp, r4, asr #32
    1184:	00000048 	andeq	r0, r0, r8, asr #32
    1188:	00000000 	andeq	r0, r0, r0
    118c:	002c0044 	eoreq	r0, ip, r4, asr #32
    1190:	00000050 	andeq	r0, r0, r0, asr r0
    1194:	00000000 	andeq	r0, r0, r0
    1198:	002d0044 	eoreq	r0, sp, r4, asr #32
    119c:	00000058 	andeq	r0, r0, r8, asr r0
    11a0:	00000dcc 	andeq	r0, r0, ip, asr #27
    11a4:	00240080 	eoreq	r0, r4, r0, lsl #1
    11a8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    11ac:	00000000 	andeq	r0, r0, r0
    11b0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    11bc:	000000e0 	andeq	r0, r0, r0, ror #1
    11c0:	00000060 	andeq	r0, r0, r0, rrx
    11c4:	00000000 	andeq	r0, r0, r0
    11c8:	00000024 	andeq	r0, r0, r4, lsr #32
    11cc:	00000060 	andeq	r0, r0, r0, rrx
    11d0:	00000000 	andeq	r0, r0, r0
    11d4:	0000004e 	andeq	r0, r0, lr, asr #32
    11d8:	8001070c 	andhi	r0, r1, ip, lsl #14
    11dc:	00000000 	andeq	r0, r0, r0
    11e0:	00000064 	andeq	r0, r0, r4, rrx
    11e4:	8001070c 	andhi	r0, r1, ip, lsl #14
    11e8:	00000007 	andeq	r0, r0, r7
    11ec:	00020064 	andeq	r0, r2, r4, rrx
    11f0:	8001070c 	andhi	r0, r1, ip, lsl #14
    11f4:	00000deb 	andeq	r0, r0, fp, ror #27
    11f8:	00020064 	andeq	r0, r2, r4, rrx
    11fc:	8001070c 	andhi	r0, r1, ip, lsl #14
    1200:	0000003d 	andeq	r0, r0, sp, lsr r0
    1204:	0000003c 	andeq	r0, r0, ip, lsr r0
    1208:	00000000 	andeq	r0, r0, r0
    120c:	0000004c 	andeq	r0, r0, ip, asr #32
    1210:	00000080 	andeq	r0, r0, r0, lsl #1
    1214:	00000000 	andeq	r0, r0, r0
    1218:	00000076 	andeq	r0, r0, r6, ror r0
    121c:	00000080 	andeq	r0, r0, r0, lsl #1
    1220:	00000000 	andeq	r0, r0, r0
    1224:	00000094 	muleq	r0, r4, r0
    1228:	00000080 	andeq	r0, r0, r0, lsl #1
    122c:	00000000 	andeq	r0, r0, r0
    1230:	000000c3 	andeq	r0, r0, r3, asr #1
    1234:	00000080 	andeq	r0, r0, r0, lsl #1
    1238:	00000000 	andeq	r0, r0, r0
    123c:	000000ee 	andeq	r0, r0, lr, ror #1
    1240:	00000080 	andeq	r0, r0, r0, lsl #1
    1244:	00000000 	andeq	r0, r0, r0
    1248:	0000011e 	andeq	r0, r0, lr, lsl r1
    124c:	00000080 	andeq	r0, r0, r0, lsl #1
    1250:	00000000 	andeq	r0, r0, r0
    1254:	0000016f 	andeq	r0, r0, pc, ror #2
    1258:	00000080 	andeq	r0, r0, r0, lsl #1
    125c:	00000000 	andeq	r0, r0, r0
    1260:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    1264:	00000080 	andeq	r0, r0, r0, lsl #1
    1268:	00000000 	andeq	r0, r0, r0
    126c:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    1270:	00000080 	andeq	r0, r0, r0, lsl #1
    1274:	00000000 	andeq	r0, r0, r0
    1278:	0000020e 	andeq	r0, r0, lr, lsl #4
    127c:	00000080 	andeq	r0, r0, r0, lsl #1
    1280:	00000000 	andeq	r0, r0, r0
    1284:	00000238 	andeq	r0, r0, r8, lsr r2
    1288:	00000080 	andeq	r0, r0, r0, lsl #1
    128c:	00000000 	andeq	r0, r0, r0
    1290:	00000261 	andeq	r0, r0, r1, ror #4
    1294:	00000080 	andeq	r0, r0, r0, lsl #1
    1298:	00000000 	andeq	r0, r0, r0
    129c:	0000027b 	andeq	r0, r0, fp, ror r2
    12a0:	00000080 	andeq	r0, r0, r0, lsl #1
    12a4:	00000000 	andeq	r0, r0, r0
    12a8:	00000296 	muleq	r0, r6, r2
    12ac:	00000080 	andeq	r0, r0, r0, lsl #1
    12b0:	00000000 	andeq	r0, r0, r0
    12b4:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    12b8:	00000080 	andeq	r0, r0, r0, lsl #1
    12bc:	00000000 	andeq	r0, r0, r0
    12c0:	000002d7 	ldrdeq	r0, [r0], -r7
    12c4:	00000080 	andeq	r0, r0, r0, lsl #1
    12c8:	00000000 	andeq	r0, r0, r0
    12cc:	000002f7 	strdeq	r0, [r0], -r7
    12d0:	00000080 	andeq	r0, r0, r0, lsl #1
    12d4:	00000000 	andeq	r0, r0, r0
    12d8:	0000031c 	andeq	r0, r0, ip, lsl r3
    12dc:	00000080 	andeq	r0, r0, r0, lsl #1
    12e0:	00000000 	andeq	r0, r0, r0
    12e4:	00000346 	andeq	r0, r0, r6, asr #6
    12e8:	00000080 	andeq	r0, r0, r0, lsl #1
    12ec:	00000000 	andeq	r0, r0, r0
    12f0:	0000036a 	andeq	r0, r0, sl, ror #6
    12f4:	00000080 	andeq	r0, r0, r0, lsl #1
    12f8:	00000000 	andeq	r0, r0, r0
    12fc:	00000393 	muleq	r0, r3, r3
    1300:	00000080 	andeq	r0, r0, r0, lsl #1
    1304:	00000000 	andeq	r0, r0, r0
    1308:	000003c1 	andeq	r0, r0, r1, asr #7
    130c:	00000080 	andeq	r0, r0, r0, lsl #1
    1310:	00000000 	andeq	r0, r0, r0
    1314:	000003e7 	andeq	r0, r0, r7, ror #7
    1318:	00000080 	andeq	r0, r0, r0, lsl #1
    131c:	00000000 	andeq	r0, r0, r0
    1320:	00000407 	andeq	r0, r0, r7, lsl #8
    1324:	00000080 	andeq	r0, r0, r0, lsl #1
    1328:	00000000 	andeq	r0, r0, r0
    132c:	0000042c 	andeq	r0, r0, ip, lsr #8
    1330:	00000080 	andeq	r0, r0, r0, lsl #1
    1334:	00000000 	andeq	r0, r0, r0
    1338:	00000456 	andeq	r0, r0, r6, asr r4
    133c:	00000080 	andeq	r0, r0, r0, lsl #1
    1340:	00000000 	andeq	r0, r0, r0
    1344:	00000485 	andeq	r0, r0, r5, lsl #9
    1348:	00000080 	andeq	r0, r0, r0, lsl #1
    134c:	00000000 	andeq	r0, r0, r0
    1350:	000004ae 	andeq	r0, r0, lr, lsr #9
    1354:	00000080 	andeq	r0, r0, r0, lsl #1
    1358:	00000000 	andeq	r0, r0, r0
    135c:	000004dc 	ldrdeq	r0, [r0], -ip
    1360:	00000080 	andeq	r0, r0, r0, lsl #1
    1364:	00000000 	andeq	r0, r0, r0
    1368:	0000050f 	andeq	r0, r0, pc, lsl #10
    136c:	00000080 	andeq	r0, r0, r0, lsl #1
    1370:	00000000 	andeq	r0, r0, r0
    1374:	00000530 	andeq	r0, r0, r0, lsr r5
    1378:	00000080 	andeq	r0, r0, r0, lsl #1
    137c:	00000000 	andeq	r0, r0, r0
    1380:	00000550 	andeq	r0, r0, r0, asr r5
    1384:	00000080 	andeq	r0, r0, r0, lsl #1
    1388:	00000000 	andeq	r0, r0, r0
    138c:	00000575 	andeq	r0, r0, r5, ror r5
    1390:	00000080 	andeq	r0, r0, r0, lsl #1
    1394:	00000000 	andeq	r0, r0, r0
    1398:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    139c:	00000080 	andeq	r0, r0, r0, lsl #1
    13a0:	00000000 	andeq	r0, r0, r0
    13a4:	000005c3 	andeq	r0, r0, r3, asr #11
    13a8:	00000080 	andeq	r0, r0, r0, lsl #1
    13ac:	00000000 	andeq	r0, r0, r0
    13b0:	000005ec 	andeq	r0, r0, ip, ror #11
    13b4:	00000080 	andeq	r0, r0, r0, lsl #1
    13b8:	00000000 	andeq	r0, r0, r0
    13bc:	0000061a 	andeq	r0, r0, sl, lsl r6
    13c0:	00000080 	andeq	r0, r0, r0, lsl #1
    13c4:	00000000 	andeq	r0, r0, r0
    13c8:	00000640 	andeq	r0, r0, r0, asr #12
    13cc:	00000080 	andeq	r0, r0, r0, lsl #1
    13d0:	00000000 	andeq	r0, r0, r0
    13d4:	00000660 	andeq	r0, r0, r0, ror #12
    13d8:	00000080 	andeq	r0, r0, r0, lsl #1
    13dc:	00000000 	andeq	r0, r0, r0
    13e0:	00000685 	andeq	r0, r0, r5, lsl #13
    13e4:	00000080 	andeq	r0, r0, r0, lsl #1
    13e8:	00000000 	andeq	r0, r0, r0
    13ec:	000006af 	andeq	r0, r0, pc, lsr #13
    13f0:	00000080 	andeq	r0, r0, r0, lsl #1
    13f4:	00000000 	andeq	r0, r0, r0
    13f8:	000006de 	ldrdeq	r0, [r0], -lr
    13fc:	00000080 	andeq	r0, r0, r0, lsl #1
    1400:	00000000 	andeq	r0, r0, r0
    1404:	00000707 	andeq	r0, r0, r7, lsl #14
    1408:	00000080 	andeq	r0, r0, r0, lsl #1
    140c:	00000000 	andeq	r0, r0, r0
    1410:	00000735 	andeq	r0, r0, r5, lsr r7
    1414:	00000080 	andeq	r0, r0, r0, lsl #1
    1418:	00000000 	andeq	r0, r0, r0
    141c:	00000768 	andeq	r0, r0, r8, ror #14
    1420:	00000080 	andeq	r0, r0, r0, lsl #1
    1424:	00000000 	andeq	r0, r0, r0
    1428:	00000e06 	andeq	r0, r0, r6, lsl #28
    142c:	00000082 	andeq	r0, r0, r2, lsl #1
    1430:	00000000 	andeq	r0, r0, r0
    1434:	0000078a 	andeq	r0, r0, sl, lsl #15
    1438:	000000c2 	andeq	r0, r0, r2, asr #1
    143c:	00002d60 	andeq	r2, r0, r0, ror #26
    1440:	00000000 	andeq	r0, r0, r0
    1444:	000000a2 	andeq	r0, r0, r2, lsr #1
    1448:	00000000 	andeq	r0, r0, r0
    144c:	00000886 	andeq	r0, r0, r6, lsl #17
    1450:	000000c2 	andeq	r0, r0, r2, asr #1
    1454:	00003ac8 	andeq	r3, r0, r8, asr #21
    1458:	00000e19 	andeq	r0, r0, r9, lsl lr
    145c:	00040024 	andeq	r0, r4, r4, lsr #32
    1460:	8001070c 	andhi	r0, r1, ip, lsl #14
    1464:	00000000 	andeq	r0, r0, r0
    1468:	0000002e 	andeq	r0, r0, lr, lsr #32
    146c:	8001070c 	andhi	r0, r1, ip, lsl #14
    1470:	00000000 	andeq	r0, r0, r0
    1474:	00040044 	andeq	r0, r4, r4, asr #32
	...
    1480:	00050044 	andeq	r0, r5, r4, asr #32
    1484:	0000000c 	andeq	r0, r0, ip
    1488:	00000000 	andeq	r0, r0, r0
    148c:	00060044 	andeq	r0, r6, r4, asr #32
    1490:	00000010 	andeq	r0, r0, r0, lsl r0
    1494:	00000000 	andeq	r0, r0, r0
    1498:	00000024 	andeq	r0, r0, r4, lsr #32
    149c:	00000018 	andeq	r0, r0, r8, lsl r0
    14a0:	00000000 	andeq	r0, r0, r0
    14a4:	0000004e 	andeq	r0, r0, lr, asr #32
    14a8:	80010724 	andhi	r0, r1, r4, lsr #14
    14ac:	00000e2e 	andeq	r0, r0, lr, lsr #28
    14b0:	00080024 	andeq	r0, r8, r4, lsr #32
    14b4:	80010724 	andhi	r0, r1, r4, lsr #14
    14b8:	00000000 	andeq	r0, r0, r0
    14bc:	0000002e 	andeq	r0, r0, lr, lsr #32
    14c0:	80010724 	andhi	r0, r1, r4, lsr #14
    14c4:	00000000 	andeq	r0, r0, r0
    14c8:	00080044 	andeq	r0, r8, r4, asr #32
	...
    14d4:	00090044 	andeq	r0, r9, r4, asr #32
    14d8:	0000000c 	andeq	r0, r0, ip
    14dc:	00000000 	andeq	r0, r0, r0
    14e0:	000a0044 	andeq	r0, sl, r4, asr #32
    14e4:	00000010 	andeq	r0, r0, r0, lsl r0
    14e8:	00000000 	andeq	r0, r0, r0
    14ec:	00000024 	andeq	r0, r0, r4, lsr #32
    14f0:	00000018 	andeq	r0, r0, r8, lsl r0
    14f4:	00000000 	andeq	r0, r0, r0
    14f8:	0000004e 	andeq	r0, r0, lr, asr #32
    14fc:	8001073c 	andhi	r0, r1, ip, lsr r7
    1500:	00000e44 	andeq	r0, r0, r4, asr #28
    1504:	000c0024 	andeq	r0, ip, r4, lsr #32
    1508:	8001073c 	andhi	r0, r1, ip, lsr r7
    150c:	00000000 	andeq	r0, r0, r0
    1510:	0000002e 	andeq	r0, r0, lr, lsr #32
    1514:	8001073c 	andhi	r0, r1, ip, lsr r7
    1518:	00000000 	andeq	r0, r0, r0
    151c:	000c0044 	andeq	r0, ip, r4, asr #32
	...
    1528:	000d0044 	andeq	r0, sp, r4, asr #32
    152c:	0000000c 	andeq	r0, r0, ip
    1530:	00000000 	andeq	r0, r0, r0
    1534:	000e0044 	andeq	r0, lr, r4, asr #32
    1538:	00000010 	andeq	r0, r0, r0, lsl r0
    153c:	00000000 	andeq	r0, r0, r0
    1540:	00000024 	andeq	r0, r0, r4, lsr #32
    1544:	00000018 	andeq	r0, r0, r8, lsl r0
    1548:	00000000 	andeq	r0, r0, r0
    154c:	0000004e 	andeq	r0, r0, lr, asr #32
    1550:	80010754 	andhi	r0, r1, r4, asr r7
    1554:	00000e5a 	andeq	r0, r0, sl, asr lr
    1558:	00100024 	andseq	r0, r0, r4, lsr #32
    155c:	80010754 	andhi	r0, r1, r4, asr r7
    1560:	00000000 	andeq	r0, r0, r0
    1564:	0000002e 	andeq	r0, r0, lr, lsr #32
    1568:	80010754 	andhi	r0, r1, r4, asr r7
    156c:	00000000 	andeq	r0, r0, r0
    1570:	00100044 	andseq	r0, r0, r4, asr #32
	...
    157c:	00110044 	andseq	r0, r1, r4, asr #32
    1580:	0000000c 	andeq	r0, r0, ip
    1584:	00000000 	andeq	r0, r0, r0
    1588:	00120044 	andseq	r0, r2, r4, asr #32
    158c:	00000010 	andeq	r0, r0, r0, lsl r0
    1590:	00000000 	andeq	r0, r0, r0
    1594:	00000024 	andeq	r0, r0, r4, lsr #32
    1598:	00000018 	andeq	r0, r0, r8, lsl r0
    159c:	00000000 	andeq	r0, r0, r0
    15a0:	0000004e 	andeq	r0, r0, lr, asr #32
    15a4:	8001076c 	andhi	r0, r1, ip, ror #14
    15a8:	00000e6c 	andeq	r0, r0, ip, ror #28
    15ac:	00140024 	andseq	r0, r4, r4, lsr #32
    15b0:	8001076c 	andhi	r0, r1, ip, ror #14
    15b4:	00000000 	andeq	r0, r0, r0
    15b8:	0000002e 	andeq	r0, r0, lr, lsr #32
    15bc:	8001076c 	andhi	r0, r1, ip, ror #14
    15c0:	00000000 	andeq	r0, r0, r0
    15c4:	00140044 	andseq	r0, r4, r4, asr #32
	...
    15d0:	00150044 	andseq	r0, r5, r4, asr #32
    15d4:	0000000c 	andeq	r0, r0, ip
    15d8:	00000000 	andeq	r0, r0, r0
    15dc:	00160044 	andseq	r0, r6, r4, asr #32
    15e0:	00000010 	andeq	r0, r0, r0, lsl r0
    15e4:	00000000 	andeq	r0, r0, r0
    15e8:	00000024 	andeq	r0, r0, r4, lsr #32
    15ec:	00000018 	andeq	r0, r0, r8, lsl r0
    15f0:	00000000 	andeq	r0, r0, r0
    15f4:	0000004e 	andeq	r0, r0, lr, asr #32
    15f8:	80010784 	andhi	r0, r1, r4, lsl #15
    15fc:	00000000 	andeq	r0, r0, r0
    1600:	00000064 	andeq	r0, r0, r4, rrx
    1604:	80010784 	andhi	r0, r1, r4, lsl #15
    1608:	00000007 	andeq	r0, r0, r7
    160c:	00020064 	andeq	r0, r2, r4, rrx
    1610:	80010784 	andhi	r0, r1, r4, lsl #15
    1614:	00000e7f 	andeq	r0, r0, pc, ror lr
    1618:	00020064 	andeq	r0, r2, r4, rrx
    161c:	80010784 	andhi	r0, r1, r4, lsl #15
    1620:	0000003d 	andeq	r0, r0, sp, lsr r0
    1624:	0000003c 	andeq	r0, r0, ip, lsr r0
    1628:	00000000 	andeq	r0, r0, r0
    162c:	0000004c 	andeq	r0, r0, ip, asr #32
    1630:	00000080 	andeq	r0, r0, r0, lsl #1
    1634:	00000000 	andeq	r0, r0, r0
    1638:	00000076 	andeq	r0, r0, r6, ror r0
    163c:	00000080 	andeq	r0, r0, r0, lsl #1
    1640:	00000000 	andeq	r0, r0, r0
    1644:	00000094 	muleq	r0, r4, r0
    1648:	00000080 	andeq	r0, r0, r0, lsl #1
    164c:	00000000 	andeq	r0, r0, r0
    1650:	000000c3 	andeq	r0, r0, r3, asr #1
    1654:	00000080 	andeq	r0, r0, r0, lsl #1
    1658:	00000000 	andeq	r0, r0, r0
    165c:	000000ee 	andeq	r0, r0, lr, ror #1
    1660:	00000080 	andeq	r0, r0, r0, lsl #1
    1664:	00000000 	andeq	r0, r0, r0
    1668:	0000011e 	andeq	r0, r0, lr, lsl r1
    166c:	00000080 	andeq	r0, r0, r0, lsl #1
    1670:	00000000 	andeq	r0, r0, r0
    1674:	0000016f 	andeq	r0, r0, pc, ror #2
    1678:	00000080 	andeq	r0, r0, r0, lsl #1
    167c:	00000000 	andeq	r0, r0, r0
    1680:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    1684:	00000080 	andeq	r0, r0, r0, lsl #1
    1688:	00000000 	andeq	r0, r0, r0
    168c:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    1690:	00000080 	andeq	r0, r0, r0, lsl #1
    1694:	00000000 	andeq	r0, r0, r0
    1698:	0000020e 	andeq	r0, r0, lr, lsl #4
    169c:	00000080 	andeq	r0, r0, r0, lsl #1
    16a0:	00000000 	andeq	r0, r0, r0
    16a4:	00000238 	andeq	r0, r0, r8, lsr r2
    16a8:	00000080 	andeq	r0, r0, r0, lsl #1
    16ac:	00000000 	andeq	r0, r0, r0
    16b0:	00000261 	andeq	r0, r0, r1, ror #4
    16b4:	00000080 	andeq	r0, r0, r0, lsl #1
    16b8:	00000000 	andeq	r0, r0, r0
    16bc:	0000027b 	andeq	r0, r0, fp, ror r2
    16c0:	00000080 	andeq	r0, r0, r0, lsl #1
    16c4:	00000000 	andeq	r0, r0, r0
    16c8:	00000296 	muleq	r0, r6, r2
    16cc:	00000080 	andeq	r0, r0, r0, lsl #1
    16d0:	00000000 	andeq	r0, r0, r0
    16d4:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    16d8:	00000080 	andeq	r0, r0, r0, lsl #1
    16dc:	00000000 	andeq	r0, r0, r0
    16e0:	000002d7 	ldrdeq	r0, [r0], -r7
    16e4:	00000080 	andeq	r0, r0, r0, lsl #1
    16e8:	00000000 	andeq	r0, r0, r0
    16ec:	000002f7 	strdeq	r0, [r0], -r7
    16f0:	00000080 	andeq	r0, r0, r0, lsl #1
    16f4:	00000000 	andeq	r0, r0, r0
    16f8:	0000031c 	andeq	r0, r0, ip, lsl r3
    16fc:	00000080 	andeq	r0, r0, r0, lsl #1
    1700:	00000000 	andeq	r0, r0, r0
    1704:	00000346 	andeq	r0, r0, r6, asr #6
    1708:	00000080 	andeq	r0, r0, r0, lsl #1
    170c:	00000000 	andeq	r0, r0, r0
    1710:	0000036a 	andeq	r0, r0, sl, ror #6
    1714:	00000080 	andeq	r0, r0, r0, lsl #1
    1718:	00000000 	andeq	r0, r0, r0
    171c:	00000393 	muleq	r0, r3, r3
    1720:	00000080 	andeq	r0, r0, r0, lsl #1
    1724:	00000000 	andeq	r0, r0, r0
    1728:	000003c1 	andeq	r0, r0, r1, asr #7
    172c:	00000080 	andeq	r0, r0, r0, lsl #1
    1730:	00000000 	andeq	r0, r0, r0
    1734:	000003e7 	andeq	r0, r0, r7, ror #7
    1738:	00000080 	andeq	r0, r0, r0, lsl #1
    173c:	00000000 	andeq	r0, r0, r0
    1740:	00000407 	andeq	r0, r0, r7, lsl #8
    1744:	00000080 	andeq	r0, r0, r0, lsl #1
    1748:	00000000 	andeq	r0, r0, r0
    174c:	0000042c 	andeq	r0, r0, ip, lsr #8
    1750:	00000080 	andeq	r0, r0, r0, lsl #1
    1754:	00000000 	andeq	r0, r0, r0
    1758:	00000456 	andeq	r0, r0, r6, asr r4
    175c:	00000080 	andeq	r0, r0, r0, lsl #1
    1760:	00000000 	andeq	r0, r0, r0
    1764:	00000485 	andeq	r0, r0, r5, lsl #9
    1768:	00000080 	andeq	r0, r0, r0, lsl #1
    176c:	00000000 	andeq	r0, r0, r0
    1770:	000004ae 	andeq	r0, r0, lr, lsr #9
    1774:	00000080 	andeq	r0, r0, r0, lsl #1
    1778:	00000000 	andeq	r0, r0, r0
    177c:	000004dc 	ldrdeq	r0, [r0], -ip
    1780:	00000080 	andeq	r0, r0, r0, lsl #1
    1784:	00000000 	andeq	r0, r0, r0
    1788:	0000050f 	andeq	r0, r0, pc, lsl #10
    178c:	00000080 	andeq	r0, r0, r0, lsl #1
    1790:	00000000 	andeq	r0, r0, r0
    1794:	00000530 	andeq	r0, r0, r0, lsr r5
    1798:	00000080 	andeq	r0, r0, r0, lsl #1
    179c:	00000000 	andeq	r0, r0, r0
    17a0:	00000550 	andeq	r0, r0, r0, asr r5
    17a4:	00000080 	andeq	r0, r0, r0, lsl #1
    17a8:	00000000 	andeq	r0, r0, r0
    17ac:	00000575 	andeq	r0, r0, r5, ror r5
    17b0:	00000080 	andeq	r0, r0, r0, lsl #1
    17b4:	00000000 	andeq	r0, r0, r0
    17b8:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    17bc:	00000080 	andeq	r0, r0, r0, lsl #1
    17c0:	00000000 	andeq	r0, r0, r0
    17c4:	000005c3 	andeq	r0, r0, r3, asr #11
    17c8:	00000080 	andeq	r0, r0, r0, lsl #1
    17cc:	00000000 	andeq	r0, r0, r0
    17d0:	000005ec 	andeq	r0, r0, ip, ror #11
    17d4:	00000080 	andeq	r0, r0, r0, lsl #1
    17d8:	00000000 	andeq	r0, r0, r0
    17dc:	0000061a 	andeq	r0, r0, sl, lsl r6
    17e0:	00000080 	andeq	r0, r0, r0, lsl #1
    17e4:	00000000 	andeq	r0, r0, r0
    17e8:	00000640 	andeq	r0, r0, r0, asr #12
    17ec:	00000080 	andeq	r0, r0, r0, lsl #1
    17f0:	00000000 	andeq	r0, r0, r0
    17f4:	00000660 	andeq	r0, r0, r0, ror #12
    17f8:	00000080 	andeq	r0, r0, r0, lsl #1
    17fc:	00000000 	andeq	r0, r0, r0
    1800:	00000685 	andeq	r0, r0, r5, lsl #13
    1804:	00000080 	andeq	r0, r0, r0, lsl #1
    1808:	00000000 	andeq	r0, r0, r0
    180c:	000006af 	andeq	r0, r0, pc, lsr #13
    1810:	00000080 	andeq	r0, r0, r0, lsl #1
    1814:	00000000 	andeq	r0, r0, r0
    1818:	000006de 	ldrdeq	r0, [r0], -lr
    181c:	00000080 	andeq	r0, r0, r0, lsl #1
    1820:	00000000 	andeq	r0, r0, r0
    1824:	00000707 	andeq	r0, r0, r7, lsl #14
    1828:	00000080 	andeq	r0, r0, r0, lsl #1
    182c:	00000000 	andeq	r0, r0, r0
    1830:	00000735 	andeq	r0, r0, r5, lsr r7
    1834:	00000080 	andeq	r0, r0, r0, lsl #1
    1838:	00000000 	andeq	r0, r0, r0
    183c:	00000768 	andeq	r0, r0, r8, ror #14
    1840:	00000080 	andeq	r0, r0, r0, lsl #1
    1844:	00000000 	andeq	r0, r0, r0
    1848:	00000e06 	andeq	r0, r0, r6, lsl #28
    184c:	000000c2 	andeq	r0, r0, r2, asr #1
    1850:	00000000 	andeq	r0, r0, r0
    1854:	0000078a 	andeq	r0, r0, sl, lsl #15
    1858:	000000c2 	andeq	r0, r0, r2, asr #1
    185c:	00002d60 	andeq	r2, r0, r0, ror #26
    1860:	0000077c 	andeq	r0, r0, ip, ror r7
    1864:	000000c2 	andeq	r0, r0, r2, asr #1
    1868:	00000bc7 	andeq	r0, r0, r7, asr #23
    186c:	00000886 	andeq	r0, r0, r6, lsl #17
    1870:	000000c2 	andeq	r0, r0, r2, asr #1
    1874:	00003ac8 	andeq	r3, r0, r8, asr #21
    1878:	00000e8e 	andeq	r0, r0, lr, lsl #29
    187c:	00150024 	andseq	r0, r5, r4, lsr #32
    1880:	80010784 	andhi	r0, r1, r4, lsl #15
    1884:	00000000 	andeq	r0, r0, r0
    1888:	0000002e 	andeq	r0, r0, lr, lsr #32
    188c:	80010784 	andhi	r0, r1, r4, lsl #15
    1890:	00000000 	andeq	r0, r0, r0
    1894:	00160044 	andseq	r0, r6, r4, asr #32
	...
    18a0:	00170044 	andseq	r0, r7, r4, asr #32
    18a4:	0000000c 	andeq	r0, r0, ip
    18a8:	00000000 	andeq	r0, r0, r0
    18ac:	00180044 	andseq	r0, r8, r4, asr #32
    18b0:	0000001c 	andeq	r0, r0, ip, lsl r0
    18b4:	00000000 	andeq	r0, r0, r0
    18b8:	001a0044 	andseq	r0, sl, r4, asr #32
    18bc:	0000002c 	andeq	r0, r0, ip, lsr #32
    18c0:	00000000 	andeq	r0, r0, r0
    18c4:	001b0044 	andseq	r0, fp, r4, asr #32
    18c8:	00000030 	andeq	r0, r0, r0, lsr r0
    18cc:	00000000 	andeq	r0, r0, r0
    18d0:	001c0044 	andseq	r0, ip, r4, asr #32
    18d4:	0000004c 	andeq	r0, r0, ip, asr #32
    18d8:	00000000 	andeq	r0, r0, r0
    18dc:	00000024 	andeq	r0, r0, r4, lsr #32
    18e0:	0000005c 	andeq	r0, r0, ip, asr r0
    18e4:	00000000 	andeq	r0, r0, r0
    18e8:	0000004e 	andeq	r0, r0, lr, asr #32
    18ec:	800107e0 	andhi	r0, r1, r0, ror #15
    18f0:	00000e9f 	muleq	r0, pc, lr	; <UNPREDICTABLE>
    18f4:	001e0024 	andseq	r0, lr, r4, lsr #32
    18f8:	800107e0 	andhi	r0, r1, r0, ror #15
    18fc:	00000eb0 			; <UNDEFINED> instruction: 0x00000eb0
    1900:	001e00a0 	andseq	r0, lr, r0, lsr #1
    1904:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    1908:	00000000 	andeq	r0, r0, r0
    190c:	0000002e 	andeq	r0, r0, lr, lsr #32
    1910:	800107e0 	andhi	r0, r1, r0, ror #15
    1914:	00000000 	andeq	r0, r0, r0
    1918:	001e0044 	andseq	r0, lr, r4, asr #32
	...
    1924:	001f0044 	andseq	r0, pc, r4, asr #32
    1928:	00000014 	andeq	r0, r0, r4, lsl r0
    192c:	00000000 	andeq	r0, r0, r0
    1930:	00210044 	eoreq	r0, r1, r4, asr #32
    1934:	0000001c 	andeq	r0, r0, ip, lsl r0
    1938:	00000000 	andeq	r0, r0, r0
    193c:	00220044 	eoreq	r0, r2, r4, asr #32
    1940:	0000003c 	andeq	r0, r0, ip, lsr r0
    1944:	00000000 	andeq	r0, r0, r0
    1948:	00230044 	eoreq	r0, r3, r4, asr #32
    194c:	00000048 	andeq	r0, r0, r8, asr #32
    1950:	00000000 	andeq	r0, r0, r0
    1954:	00240044 	eoreq	r0, r4, r4, asr #32
    1958:	0000004c 	andeq	r0, r0, ip, asr #32
    195c:	00000000 	andeq	r0, r0, r0
    1960:	00250044 	eoreq	r0, r5, r4, asr #32
    1964:	00000054 	andeq	r0, r0, r4, asr r0
    1968:	00000000 	andeq	r0, r0, r0
    196c:	00260044 	eoreq	r0, r6, r4, asr #32
    1970:	00000058 	andeq	r0, r0, r8, asr r0
    1974:	00000ecb 	andeq	r0, r0, fp, asr #29
    1978:	001f0080 	andseq	r0, pc, r0, lsl #1
    197c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    1980:	00000000 	andeq	r0, r0, r0
    1984:	000000c0 	andeq	r0, r0, r0, asr #1
    1988:	00000000 	andeq	r0, r0, r0
    198c:	00000ed3 	ldrdeq	r0, [r0], -r3
    1990:	00210080 	eoreq	r0, r1, r0, lsl #1
    1994:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    1998:	00000000 	andeq	r0, r0, r0
    199c:	000000c0 	andeq	r0, r0, r0, asr #1
    19a0:	0000001c 	andeq	r0, r0, ip, lsl r0
    19a4:	00000000 	andeq	r0, r0, r0
    19a8:	000000e0 	andeq	r0, r0, r0, ror #1
    19ac:	00000054 	andeq	r0, r0, r4, asr r0
    19b0:	00000000 	andeq	r0, r0, r0
    19b4:	000000e0 	andeq	r0, r0, r0, ror #1
    19b8:	00000060 	andeq	r0, r0, r0, rrx
    19bc:	00000000 	andeq	r0, r0, r0
    19c0:	00000024 	andeq	r0, r0, r4, lsr #32
    19c4:	00000060 	andeq	r0, r0, r0, rrx
    19c8:	00000000 	andeq	r0, r0, r0
    19cc:	0000004e 	andeq	r0, r0, lr, asr #32
    19d0:	80010840 	andhi	r0, r1, r0, asr #16
    19d4:	00000edb 	ldrdeq	r0, [r0], -fp
    19d8:	00280024 	eoreq	r0, r8, r4, lsr #32
    19dc:	80010840 	andhi	r0, r1, r0, asr #16
    19e0:	00000eed 	andeq	r0, r0, sp, ror #29
    19e4:	002800a0 	eoreq	r0, r8, r0, lsr #1
    19e8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    19ec:	00000000 	andeq	r0, r0, r0
    19f0:	0000002e 	andeq	r0, r0, lr, lsr #32
    19f4:	80010840 	andhi	r0, r1, r0, asr #16
    19f8:	00000000 	andeq	r0, r0, r0
    19fc:	00280044 	eoreq	r0, r8, r4, asr #32
	...
    1a08:	00290044 	eoreq	r0, r9, r4, asr #32
    1a0c:	00000014 	andeq	r0, r0, r4, lsl r0
    1a10:	00000000 	andeq	r0, r0, r0
    1a14:	002a0044 	eoreq	r0, sl, r4, asr #32
    1a18:	00000020 	andeq	r0, r0, r0, lsr #32
    1a1c:	00000000 	andeq	r0, r0, r0
    1a20:	002b0044 	eoreq	r0, fp, r4, asr #32
    1a24:	00000028 	andeq	r0, r0, r8, lsr #32
    1a28:	00000000 	andeq	r0, r0, r0
    1a2c:	002c0044 	eoreq	r0, ip, r4, asr #32
    1a30:	00000038 	andeq	r0, r0, r8, lsr r0
    1a34:	00000000 	andeq	r0, r0, r0
    1a38:	00000024 	andeq	r0, r0, r4, lsr #32
    1a3c:	00000040 	andeq	r0, r0, r0, asr #32
    1a40:	00000000 	andeq	r0, r0, r0
    1a44:	0000004e 	andeq	r0, r0, lr, asr #32
    1a48:	80010880 	andhi	r0, r1, r0, lsl #17
    1a4c:	00000ef6 	strdeq	r0, [r0], -r6
    1a50:	00320024 	eorseq	r0, r2, r4, lsr #32
    1a54:	80010880 	andhi	r0, r1, r0, lsl #17
    1a58:	00000000 	andeq	r0, r0, r0
    1a5c:	0000002e 	andeq	r0, r0, lr, lsr #32
    1a60:	80010880 	andhi	r0, r1, r0, lsl #17
    1a64:	00000000 	andeq	r0, r0, r0
    1a68:	00330044 	eorseq	r0, r3, r4, asr #32
	...
    1a74:	00340044 	eorseq	r0, r4, r4, asr #32
    1a78:	00000010 	andeq	r0, r0, r0, lsl r0
    1a7c:	00000000 	andeq	r0, r0, r0
    1a80:	00360044 	eorseq	r0, r6, r4, asr #32
    1a84:	00000018 	andeq	r0, r0, r8, lsl r0
    1a88:	00000000 	andeq	r0, r0, r0
    1a8c:	00370044 	eorseq	r0, r7, r4, asr #32
    1a90:	00000038 	andeq	r0, r0, r8, lsr r0
    1a94:	00000000 	andeq	r0, r0, r0
    1a98:	00380044 	eorseq	r0, r8, r4, asr #32
    1a9c:	00000054 	andeq	r0, r0, r4, asr r0
    1aa0:	00000000 	andeq	r0, r0, r0
    1aa4:	003b0044 	eorseq	r0, fp, r4, asr #32
    1aa8:	0000007c 	andeq	r0, r0, ip, ror r0
    1aac:	00000000 	andeq	r0, r0, r0
    1ab0:	003c0044 	eorseq	r0, ip, r4, asr #32
    1ab4:	00000080 	andeq	r0, r0, r0, lsl #1
    1ab8:	00000f07 	andeq	r0, r0, r7, lsl #30
    1abc:	00340080 	eorseq	r0, r4, r0, lsl #1
    1ac0:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    1ac4:	00000000 	andeq	r0, r0, r0
    1ac8:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    1ad4:	000000e0 	andeq	r0, r0, r0, ror #1
    1ad8:	000000a4 	andeq	r0, r0, r4, lsr #1
    1adc:	00000000 	andeq	r0, r0, r0
    1ae0:	00000024 	andeq	r0, r0, r4, lsr #32
    1ae4:	000000a4 	andeq	r0, r0, r4, lsr #1
    1ae8:	00000000 	andeq	r0, r0, r0
    1aec:	0000004e 	andeq	r0, r0, lr, asr #32
    1af0:	80010924 	andhi	r0, r1, r4, lsr #18
    1af4:	00000f15 	andeq	r0, r0, r5, lsl pc
    1af8:	003f0024 	eorseq	r0, pc, r4, lsr #32
    1afc:	80010924 	andhi	r0, r1, r4, lsr #18
    1b00:	00000f28 	andeq	r0, r0, r8, lsr #30
    1b04:	003f00a0 	eorseq	r0, pc, r0, lsr #1
    1b08:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    1b0c:	00000f37 	andeq	r0, r0, r7, lsr pc
    1b10:	003f00a0 	eorseq	r0, pc, r0, lsr #1
    1b14:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    1b18:	00000000 	andeq	r0, r0, r0
    1b1c:	0000002e 	andeq	r0, r0, lr, lsr #32
    1b20:	80010924 	andhi	r0, r1, r4, lsr #18
    1b24:	00000000 	andeq	r0, r0, r0
    1b28:	00400044 	subeq	r0, r0, r4, asr #32
	...
    1b34:	00410044 	subeq	r0, r1, r4, asr #32
    1b38:	00000018 	andeq	r0, r0, r8, lsl r0
    1b3c:	00000000 	andeq	r0, r0, r0
    1b40:	00420044 	subeq	r0, r2, r4, asr #32
    1b44:	00000024 	andeq	r0, r0, r4, lsr #32
    1b48:	00000000 	andeq	r0, r0, r0
    1b4c:	00430044 	subeq	r0, r3, r4, asr #32
    1b50:	00000034 	andeq	r0, r0, r4, lsr r0
    1b54:	00000000 	andeq	r0, r0, r0
    1b58:	00440044 	subeq	r0, r4, r4, asr #32
    1b5c:	0000003c 	andeq	r0, r0, ip, lsr r0
    1b60:	00000000 	andeq	r0, r0, r0
    1b64:	00450044 	subeq	r0, r5, r4, asr #32
    1b68:	00000040 	andeq	r0, r0, r0, asr #32
    1b6c:	00000000 	andeq	r0, r0, r0
    1b70:	00000024 	andeq	r0, r0, r4, lsr #32
    1b74:	0000004c 	andeq	r0, r0, ip, asr #32
    1b78:	00000000 	andeq	r0, r0, r0
    1b7c:	0000004e 	andeq	r0, r0, lr, asr #32
    1b80:	80010970 	andhi	r0, r1, r0, ror r9
    1b84:	00000f49 	andeq	r0, r0, r9, asr #30
    1b88:	004b0024 	subeq	r0, fp, r4, lsr #32
    1b8c:	80010970 	andhi	r0, r1, r0, ror r9
    1b90:	00000000 	andeq	r0, r0, r0
    1b94:	0000002e 	andeq	r0, r0, lr, lsr #32
    1b98:	80010970 	andhi	r0, r1, r0, ror r9
    1b9c:	00000000 	andeq	r0, r0, r0
    1ba0:	004c0044 	subeq	r0, ip, r4, asr #32
	...
    1bac:	004d0044 	subeq	r0, sp, r4, asr #32
    1bb0:	00000010 	andeq	r0, r0, r0, lsl r0
    1bb4:	00000000 	andeq	r0, r0, r0
    1bb8:	004e0044 	subeq	r0, lr, r4, asr #32
    1bbc:	00000014 	andeq	r0, r0, r4, lsl r0
    1bc0:	00000000 	andeq	r0, r0, r0
    1bc4:	004f0044 	subeq	r0, pc, r4, asr #32
    1bc8:	0000001c 	andeq	r0, r0, ip, lsl r0
    1bcc:	00000000 	andeq	r0, r0, r0
    1bd0:	00510044 	subseq	r0, r1, r4, asr #32
    1bd4:	00000024 	andeq	r0, r0, r4, lsr #32
    1bd8:	00000000 	andeq	r0, r0, r0
    1bdc:	00530044 	subseq	r0, r3, r4, asr #32
    1be0:	00000040 	andeq	r0, r0, r0, asr #32
    1be4:	00000000 	andeq	r0, r0, r0
    1be8:	00550044 	subseq	r0, r5, r4, asr #32
    1bec:	00000058 	andeq	r0, r0, r8, asr r0
    1bf0:	00000000 	andeq	r0, r0, r0
    1bf4:	00560044 	subseq	r0, r6, r4, asr #32
    1bf8:	00000078 	andeq	r0, r0, r8, ror r0
    1bfc:	00000000 	andeq	r0, r0, r0
    1c00:	004d0044 	subeq	r0, sp, r4, asr #32
    1c04:	00000088 	andeq	r0, r0, r8, lsl #1
    1c08:	00000000 	andeq	r0, r0, r0
    1c0c:	00590044 	subseq	r0, r9, r4, asr #32
    1c10:	00000098 	muleq	r0, r8, r0
    1c14:	00000f66 	andeq	r0, r0, r6, ror #30
    1c18:	004e0080 	subeq	r0, lr, r0, lsl #1
    1c1c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    1c20:	00000f74 	andeq	r0, r0, r4, ror pc
    1c24:	004f0080 	subeq	r0, pc, r0, lsl #1
    1c28:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    1c2c:	00000000 	andeq	r0, r0, r0
    1c30:	000000c0 	andeq	r0, r0, r0, asr #1
    1c34:	00000014 	andeq	r0, r0, r4, lsl r0
    1c38:	00000000 	andeq	r0, r0, r0
    1c3c:	000000e0 	andeq	r0, r0, r0, ror #1
    1c40:	00000088 	andeq	r0, r0, r8, lsl #1
    1c44:	00000000 	andeq	r0, r0, r0
    1c48:	00000024 	andeq	r0, r0, r4, lsr #32
    1c4c:	000000b4 	strheq	r0, [r0], -r4
    1c50:	00000000 	andeq	r0, r0, r0
    1c54:	0000004e 	andeq	r0, r0, lr, asr #32
    1c58:	80010a24 	andhi	r0, r1, r4, lsr #20
    1c5c:	00000f7f 	andeq	r0, r0, pc, ror pc
    1c60:	00090028 	andeq	r0, r9, r8, lsr #32
    1c64:	80024000 	andhi	r4, r2, r0
    1c68:	00000fc1 	andeq	r0, r0, r1, asr #31
    1c6c:	000a0026 	andeq	r0, sl, r6, lsr #32
    1c70:	80024010 	andhi	r4, r2, r0, lsl r0
    1c74:	00000fda 	ldrdeq	r0, [r0], -sl
    1c78:	000b0026 	andeq	r0, fp, r6, lsr #32
    1c7c:	80024014 	andhi	r4, r2, r4, lsl r0
    1c80:	00000000 	andeq	r0, r0, r0
    1c84:	00000064 	andeq	r0, r0, r4, rrx
    1c88:	80010a24 	andhi	r0, r1, r4, lsr #20
    1c8c:	00000007 	andeq	r0, r0, r7
    1c90:	00020064 	andeq	r0, r2, r4, rrx
    1c94:	80010a24 	andhi	r0, r1, r4, lsr #20
    1c98:	00000ff3 	strdeq	r0, [r0], -r3
    1c9c:	00020064 	andeq	r0, r2, r4, rrx
    1ca0:	80010a24 	andhi	r0, r1, r4, lsr #20
    1ca4:	0000003d 	andeq	r0, r0, sp, lsr r0
    1ca8:	0000003c 	andeq	r0, r0, ip, lsr r0
    1cac:	00000000 	andeq	r0, r0, r0
    1cb0:	0000004c 	andeq	r0, r0, ip, asr #32
    1cb4:	00000080 	andeq	r0, r0, r0, lsl #1
    1cb8:	00000000 	andeq	r0, r0, r0
    1cbc:	00000076 	andeq	r0, r0, r6, ror r0
    1cc0:	00000080 	andeq	r0, r0, r0, lsl #1
    1cc4:	00000000 	andeq	r0, r0, r0
    1cc8:	00000094 	muleq	r0, r4, r0
    1ccc:	00000080 	andeq	r0, r0, r0, lsl #1
    1cd0:	00000000 	andeq	r0, r0, r0
    1cd4:	000000c3 	andeq	r0, r0, r3, asr #1
    1cd8:	00000080 	andeq	r0, r0, r0, lsl #1
    1cdc:	00000000 	andeq	r0, r0, r0
    1ce0:	000000ee 	andeq	r0, r0, lr, ror #1
    1ce4:	00000080 	andeq	r0, r0, r0, lsl #1
    1ce8:	00000000 	andeq	r0, r0, r0
    1cec:	0000011e 	andeq	r0, r0, lr, lsl r1
    1cf0:	00000080 	andeq	r0, r0, r0, lsl #1
    1cf4:	00000000 	andeq	r0, r0, r0
    1cf8:	0000016f 	andeq	r0, r0, pc, ror #2
    1cfc:	00000080 	andeq	r0, r0, r0, lsl #1
    1d00:	00000000 	andeq	r0, r0, r0
    1d04:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    1d08:	00000080 	andeq	r0, r0, r0, lsl #1
    1d0c:	00000000 	andeq	r0, r0, r0
    1d10:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    1d14:	00000080 	andeq	r0, r0, r0, lsl #1
    1d18:	00000000 	andeq	r0, r0, r0
    1d1c:	0000020e 	andeq	r0, r0, lr, lsl #4
    1d20:	00000080 	andeq	r0, r0, r0, lsl #1
    1d24:	00000000 	andeq	r0, r0, r0
    1d28:	00000238 	andeq	r0, r0, r8, lsr r2
    1d2c:	00000080 	andeq	r0, r0, r0, lsl #1
    1d30:	00000000 	andeq	r0, r0, r0
    1d34:	00000261 	andeq	r0, r0, r1, ror #4
    1d38:	00000080 	andeq	r0, r0, r0, lsl #1
    1d3c:	00000000 	andeq	r0, r0, r0
    1d40:	0000027b 	andeq	r0, r0, fp, ror r2
    1d44:	00000080 	andeq	r0, r0, r0, lsl #1
    1d48:	00000000 	andeq	r0, r0, r0
    1d4c:	00000296 	muleq	r0, r6, r2
    1d50:	00000080 	andeq	r0, r0, r0, lsl #1
    1d54:	00000000 	andeq	r0, r0, r0
    1d58:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    1d5c:	00000080 	andeq	r0, r0, r0, lsl #1
    1d60:	00000000 	andeq	r0, r0, r0
    1d64:	000002d7 	ldrdeq	r0, [r0], -r7
    1d68:	00000080 	andeq	r0, r0, r0, lsl #1
    1d6c:	00000000 	andeq	r0, r0, r0
    1d70:	000002f7 	strdeq	r0, [r0], -r7
    1d74:	00000080 	andeq	r0, r0, r0, lsl #1
    1d78:	00000000 	andeq	r0, r0, r0
    1d7c:	0000031c 	andeq	r0, r0, ip, lsl r3
    1d80:	00000080 	andeq	r0, r0, r0, lsl #1
    1d84:	00000000 	andeq	r0, r0, r0
    1d88:	00000346 	andeq	r0, r0, r6, asr #6
    1d8c:	00000080 	andeq	r0, r0, r0, lsl #1
    1d90:	00000000 	andeq	r0, r0, r0
    1d94:	0000036a 	andeq	r0, r0, sl, ror #6
    1d98:	00000080 	andeq	r0, r0, r0, lsl #1
    1d9c:	00000000 	andeq	r0, r0, r0
    1da0:	00000393 	muleq	r0, r3, r3
    1da4:	00000080 	andeq	r0, r0, r0, lsl #1
    1da8:	00000000 	andeq	r0, r0, r0
    1dac:	000003c1 	andeq	r0, r0, r1, asr #7
    1db0:	00000080 	andeq	r0, r0, r0, lsl #1
    1db4:	00000000 	andeq	r0, r0, r0
    1db8:	000003e7 	andeq	r0, r0, r7, ror #7
    1dbc:	00000080 	andeq	r0, r0, r0, lsl #1
    1dc0:	00000000 	andeq	r0, r0, r0
    1dc4:	00000407 	andeq	r0, r0, r7, lsl #8
    1dc8:	00000080 	andeq	r0, r0, r0, lsl #1
    1dcc:	00000000 	andeq	r0, r0, r0
    1dd0:	0000042c 	andeq	r0, r0, ip, lsr #8
    1dd4:	00000080 	andeq	r0, r0, r0, lsl #1
    1dd8:	00000000 	andeq	r0, r0, r0
    1ddc:	00000456 	andeq	r0, r0, r6, asr r4
    1de0:	00000080 	andeq	r0, r0, r0, lsl #1
    1de4:	00000000 	andeq	r0, r0, r0
    1de8:	00000485 	andeq	r0, r0, r5, lsl #9
    1dec:	00000080 	andeq	r0, r0, r0, lsl #1
    1df0:	00000000 	andeq	r0, r0, r0
    1df4:	000004ae 	andeq	r0, r0, lr, lsr #9
    1df8:	00000080 	andeq	r0, r0, r0, lsl #1
    1dfc:	00000000 	andeq	r0, r0, r0
    1e00:	000004dc 	ldrdeq	r0, [r0], -ip
    1e04:	00000080 	andeq	r0, r0, r0, lsl #1
    1e08:	00000000 	andeq	r0, r0, r0
    1e0c:	0000050f 	andeq	r0, r0, pc, lsl #10
    1e10:	00000080 	andeq	r0, r0, r0, lsl #1
    1e14:	00000000 	andeq	r0, r0, r0
    1e18:	00000530 	andeq	r0, r0, r0, lsr r5
    1e1c:	00000080 	andeq	r0, r0, r0, lsl #1
    1e20:	00000000 	andeq	r0, r0, r0
    1e24:	00000550 	andeq	r0, r0, r0, asr r5
    1e28:	00000080 	andeq	r0, r0, r0, lsl #1
    1e2c:	00000000 	andeq	r0, r0, r0
    1e30:	00000575 	andeq	r0, r0, r5, ror r5
    1e34:	00000080 	andeq	r0, r0, r0, lsl #1
    1e38:	00000000 	andeq	r0, r0, r0
    1e3c:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    1e40:	00000080 	andeq	r0, r0, r0, lsl #1
    1e44:	00000000 	andeq	r0, r0, r0
    1e48:	000005c3 	andeq	r0, r0, r3, asr #11
    1e4c:	00000080 	andeq	r0, r0, r0, lsl #1
    1e50:	00000000 	andeq	r0, r0, r0
    1e54:	000005ec 	andeq	r0, r0, ip, ror #11
    1e58:	00000080 	andeq	r0, r0, r0, lsl #1
    1e5c:	00000000 	andeq	r0, r0, r0
    1e60:	0000061a 	andeq	r0, r0, sl, lsl r6
    1e64:	00000080 	andeq	r0, r0, r0, lsl #1
    1e68:	00000000 	andeq	r0, r0, r0
    1e6c:	00000640 	andeq	r0, r0, r0, asr #12
    1e70:	00000080 	andeq	r0, r0, r0, lsl #1
    1e74:	00000000 	andeq	r0, r0, r0
    1e78:	00000660 	andeq	r0, r0, r0, ror #12
    1e7c:	00000080 	andeq	r0, r0, r0, lsl #1
    1e80:	00000000 	andeq	r0, r0, r0
    1e84:	00000685 	andeq	r0, r0, r5, lsl #13
    1e88:	00000080 	andeq	r0, r0, r0, lsl #1
    1e8c:	00000000 	andeq	r0, r0, r0
    1e90:	000006af 	andeq	r0, r0, pc, lsr #13
    1e94:	00000080 	andeq	r0, r0, r0, lsl #1
    1e98:	00000000 	andeq	r0, r0, r0
    1e9c:	000006de 	ldrdeq	r0, [r0], -lr
    1ea0:	00000080 	andeq	r0, r0, r0, lsl #1
    1ea4:	00000000 	andeq	r0, r0, r0
    1ea8:	00000707 	andeq	r0, r0, r7, lsl #14
    1eac:	00000080 	andeq	r0, r0, r0, lsl #1
    1eb0:	00000000 	andeq	r0, r0, r0
    1eb4:	00000735 	andeq	r0, r0, r5, lsr r7
    1eb8:	00000080 	andeq	r0, r0, r0, lsl #1
    1ebc:	00000000 	andeq	r0, r0, r0
    1ec0:	00000768 	andeq	r0, r0, r8, ror #14
    1ec4:	00000080 	andeq	r0, r0, r0, lsl #1
    1ec8:	00000000 	andeq	r0, r0, r0
    1ecc:	00000886 	andeq	r0, r0, r6, lsl #17
    1ed0:	000000c2 	andeq	r0, r0, r2, asr #1
    1ed4:	00003ac8 	andeq	r3, r0, r8, asr #21
    1ed8:	0000078a 	andeq	r0, r0, sl, lsl #15
    1edc:	000000c2 	andeq	r0, r0, r2, asr #1
    1ee0:	00002d60 	andeq	r2, r0, r0, ror #26
    1ee4:	00001000 	andeq	r1, r0, r0
    1ee8:	00000082 	andeq	r0, r0, r2, lsl #1
    1eec:	0000100f 	andeq	r1, r0, pc
    1ef0:	00001014 	andeq	r1, r0, r4, lsl r0
    1ef4:	00000080 	andeq	r0, r0, r0, lsl #1
    1ef8:	00000000 	andeq	r0, r0, r0
    1efc:	0000103f 	andeq	r1, r0, pc, lsr r0
    1f00:	00080080 	andeq	r0, r8, r0, lsl #1
	...
    1f0c:	000000a2 	andeq	r0, r0, r2, lsr #1
    1f10:	00000000 	andeq	r0, r0, r0
    1f14:	00001056 	andeq	r1, r0, r6, asr r0
    1f18:	00000082 	andeq	r0, r0, r2, lsl #1
    1f1c:	00000000 	andeq	r0, r0, r0
    1f20:	00001067 	andeq	r1, r0, r7, rrx
    1f24:	00000082 	andeq	r0, r0, r2, lsl #1
    1f28:	00003803 	andeq	r3, r0, r3, lsl #16
    1f2c:	0000107a 	andeq	r1, r0, sl, ror r0
    1f30:	00000080 	andeq	r0, r0, r0, lsl #1
    1f34:	00000000 	andeq	r0, r0, r0
    1f38:	0000111b 	andeq	r1, r0, fp, lsl r1
    1f3c:	00110080 	andseq	r0, r1, r0, lsl #1
    1f40:	00000000 	andeq	r0, r0, r0
    1f44:	00001131 	andeq	r1, r0, r1, lsr r1
    1f48:	00160080 	andseq	r0, r6, r0, lsl #1
	...
    1f54:	000000a2 	andeq	r0, r0, r2, lsr #1
	...
    1f60:	000000a2 	andeq	r0, r0, r2, lsr #1
    1f64:	00000000 	andeq	r0, r0, r0
    1f68:	00001173 	andeq	r1, r0, r3, ror r1
    1f6c:	00000082 	andeq	r0, r0, r2, lsl #1
    1f70:	0000c58e 	andeq	ip, r0, lr, lsl #11
    1f74:	00001182 	andeq	r1, r0, r2, lsl #3
    1f78:	00000082 	andeq	r0, r0, r2, lsl #1
    1f7c:	00004df6 	strdeq	r4, [r0], -r6
    1f80:	0000119b 	muleq	r0, fp, r1
    1f84:	00000080 	andeq	r0, r0, r0, lsl #1
    1f88:	00000000 	andeq	r0, r0, r0
    1f8c:	00001210 	andeq	r1, r0, r0, lsl r2
    1f90:	000d0080 	andeq	r0, sp, r0, lsl #1
    1f94:	00000000 	andeq	r0, r0, r0
    1f98:	00001227 	andeq	r1, r0, r7, lsr #4
    1f9c:	00180080 	andseq	r0, r8, r0, lsl #1
	...
    1fa8:	000000a2 	andeq	r0, r0, r2, lsr #1
    1fac:	00000000 	andeq	r0, r0, r0
    1fb0:	000012f3 	strdeq	r1, [r0], -r3
    1fb4:	00000082 	andeq	r0, r0, r2, lsl #1
    1fb8:	00002a19 	andeq	r2, r0, r9, lsl sl
    1fbc:	00001306 	andeq	r1, r0, r6, lsl #6
    1fc0:	00000082 	andeq	r0, r0, r2, lsl #1
    1fc4:	0000151b 	andeq	r1, r0, fp, lsl r5
    1fc8:	00001318 	andeq	r1, r0, r8, lsl r3
    1fcc:	000d0080 	andeq	r0, sp, r0, lsl #1
	...
    1fd8:	000000a2 	andeq	r0, r0, r2, lsr #1
    1fdc:	00000000 	andeq	r0, r0, r0
    1fe0:	00001373 	andeq	r1, r0, r3, ror r3
    1fe4:	00000080 	andeq	r0, r0, r0, lsl #1
    1fe8:	00000000 	andeq	r0, r0, r0
    1fec:	000013c7 	andeq	r1, r0, r7, asr #7
    1ff0:	000c0080 	andeq	r0, ip, r0, lsl #1
    1ff4:	00000000 	andeq	r0, r0, r0
    1ff8:	000013de 	ldrdeq	r1, [r0], -lr
    1ffc:	00110080 	andseq	r0, r1, r0, lsl #1
	...
    2008:	000000a2 	andeq	r0, r0, r2, lsr #1
    200c:	00000000 	andeq	r0, r0, r0
    2010:	00001425 	andeq	r1, r0, r5, lsr #8
    2014:	00000082 	andeq	r0, r0, r2, lsl #1
    2018:	00001fc5 	andeq	r1, r0, r5, asr #31
    201c:	00001435 	andeq	r1, r0, r5, lsr r4
    2020:	00000080 	andeq	r0, r0, r0, lsl #1
    2024:	00000000 	andeq	r0, r0, r0
    2028:	000014ad 	andeq	r1, r0, sp, lsr #9
    202c:	00100080 	andseq	r0, r0, r0, lsl #1
	...
    2038:	000000a2 	andeq	r0, r0, r2, lsr #1
    203c:	00000000 	andeq	r0, r0, r0
    2040:	000014c3 	andeq	r1, r0, r3, asr #9
    2044:	00000082 	andeq	r0, r0, r2, lsl #1
    2048:	00002279 	andeq	r2, r0, r9, ror r2
    204c:	000014d6 	ldrdeq	r1, [r0], -r6
    2050:	000f0080 	andeq	r0, pc, r0, lsl #1
	...
    205c:	000000a2 	andeq	r0, r0, r2, lsr #1
    2060:	00000000 	andeq	r0, r0, r0
    2064:	00001569 	andeq	r1, r0, r9, ror #10
    2068:	00180080 	andseq	r0, r8, r0, lsl #1
    206c:	00000000 	andeq	r0, r0, r0
    2070:	00001594 	muleq	r0, r4, r5
    2074:	00000080 	andeq	r0, r0, r0, lsl #1
    2078:	00000000 	andeq	r0, r0, r0
    207c:	000015ec 	andeq	r1, r0, ip, ror #11
    2080:	00000080 	andeq	r0, r0, r0, lsl #1
    2084:	00000000 	andeq	r0, r0, r0
    2088:	00001671 	andeq	r1, r0, r1, ror r6
    208c:	00310080 	eorseq	r0, r1, r0, lsl #1
    2090:	00000000 	andeq	r0, r0, r0
    2094:	000016c6 	andeq	r1, r0, r6, asr #13
    2098:	004c0080 	subeq	r0, ip, r0, lsl #1
	...
    20a4:	000000a2 	andeq	r0, r0, r2, lsr #1
    20a8:	00000000 	andeq	r0, r0, r0
    20ac:	0000077c 	andeq	r0, r0, ip, ror r7
    20b0:	00000082 	andeq	r0, r0, r2, lsl #1
    20b4:	00000927 	andeq	r0, r0, r7, lsr #18
    20b8:	00001887 	andeq	r1, r0, r7, lsl #17
    20bc:	00090080 	andeq	r0, r9, r0, lsl #1
	...
    20c8:	000000a2 	andeq	r0, r0, r2, lsr #1
    20cc:	00000000 	andeq	r0, r0, r0
    20d0:	000018a7 	andeq	r1, r0, r7, lsr #17
    20d4:	00100024 	andseq	r0, r0, r4, lsr #32
    20d8:	80010a24 	andhi	r0, r1, r4, lsr #20
    20dc:	00000000 	andeq	r0, r0, r0
    20e0:	0000002e 	andeq	r0, r0, lr, lsr #32
    20e4:	80010a24 	andhi	r0, r1, r4, lsr #20
    20e8:	00000000 	andeq	r0, r0, r0
    20ec:	00110044 	andseq	r0, r1, r4, asr #32
	...
    20f8:	00140044 	andseq	r0, r4, r4, asr #32
    20fc:	00000018 	andeq	r0, r0, r8, lsl r0
    2100:	00000000 	andeq	r0, r0, r0
    2104:	00160044 	andseq	r0, r6, r4, asr #32
    2108:	00000024 	andeq	r0, r0, r4, lsr #32
    210c:	00000000 	andeq	r0, r0, r0
    2110:	00180044 	andseq	r0, r8, r4, asr #32
    2114:	00000048 	andeq	r0, r0, r8, asr #32
    2118:	00000000 	andeq	r0, r0, r0
    211c:	001b0044 	andseq	r0, fp, r4, asr #32
    2120:	0000005c 	andeq	r0, r0, ip, asr r0
    2124:	00000000 	andeq	r0, r0, r0
    2128:	001c0044 	andseq	r0, ip, r4, asr #32
    212c:	00000074 	andeq	r0, r0, r4, ror r0
    2130:	000018bc 			; <UNDEFINED> instruction: 0x000018bc
    2134:	00140080 	andseq	r0, r4, r0, lsl #1
    2138:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    213c:	00000000 	andeq	r0, r0, r0
    2140:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    214c:	000000e0 	andeq	r0, r0, r0, ror #1
    2150:	00000088 	andeq	r0, r0, r8, lsl #1
    2154:	00000000 	andeq	r0, r0, r0
    2158:	00000024 	andeq	r0, r0, r4, lsr #32
    215c:	00000088 	andeq	r0, r0, r8, lsl #1
    2160:	00000000 	andeq	r0, r0, r0
    2164:	0000004e 	andeq	r0, r0, lr, asr #32
    2168:	80010aac 	andhi	r0, r1, ip, lsr #21
    216c:	000018c7 	andeq	r1, r0, r7, asr #17
    2170:	001e0024 	andseq	r0, lr, r4, lsr #32
    2174:	80010aac 	andhi	r0, r1, ip, lsr #21
    2178:	000018db 	ldrdeq	r1, [r0], -fp
    217c:	001e00a0 	andseq	r0, lr, r0, lsr #1
    2180:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2184:	00000000 	andeq	r0, r0, r0
    2188:	0000002e 	andeq	r0, r0, lr, lsr #32
    218c:	80010aac 	andhi	r0, r1, ip, lsr #21
    2190:	00000000 	andeq	r0, r0, r0
    2194:	001f0044 	andseq	r0, pc, r4, asr #32
	...
    21a0:	00200044 	eoreq	r0, r0, r4, asr #32
    21a4:	0000001c 	andeq	r0, r0, ip, lsl r0
    21a8:	00000000 	andeq	r0, r0, r0
    21ac:	00240044 	eoreq	r0, r4, r4, asr #32
    21b0:	0000002c 	andeq	r0, r0, ip, lsr #32
    21b4:	00000000 	andeq	r0, r0, r0
    21b8:	00260044 	eoreq	r0, r6, r4, asr #32
    21bc:	0000004c 	andeq	r0, r0, ip, asr #32
    21c0:	00000000 	andeq	r0, r0, r0
    21c4:	00290044 	eoreq	r0, r9, r4, asr #32
    21c8:	00000078 	andeq	r0, r0, r8, ror r0
    21cc:	00000000 	andeq	r0, r0, r0
    21d0:	002c0044 	eoreq	r0, ip, r4, asr #32
    21d4:	00000094 	muleq	r0, r4, r0
    21d8:	00000000 	andeq	r0, r0, r0
    21dc:	002f0044 	eoreq	r0, pc, r4, asr #32
    21e0:	000000cc 	andeq	r0, r0, ip, asr #1
    21e4:	00000000 	andeq	r0, r0, r0
    21e8:	00320044 	eorseq	r0, r2, r4, asr #32
    21ec:	0000010c 	andeq	r0, r0, ip, lsl #2
    21f0:	00000000 	andeq	r0, r0, r0
    21f4:	00330044 	eorseq	r0, r3, r4, asr #32
    21f8:	0000014c 	andeq	r0, r0, ip, asr #2
    21fc:	00000000 	andeq	r0, r0, r0
    2200:	00000024 	andeq	r0, r0, r4, lsr #32
    2204:	0000015c 	andeq	r0, r0, ip, asr r1
    2208:	00000000 	andeq	r0, r0, r0
    220c:	0000004e 	andeq	r0, r0, lr, asr #32
    2210:	80010c08 	andhi	r0, r1, r8, lsl #24
    2214:	000018e6 	andeq	r1, r0, r6, ror #17
    2218:	003e0024 	eorseq	r0, lr, r4, lsr #32
    221c:	80010c08 	andhi	r0, r1, r8, lsl #24
    2220:	00000000 	andeq	r0, r0, r0
    2224:	0000002e 	andeq	r0, r0, lr, lsr #32
    2228:	80010c08 	andhi	r0, r1, r8, lsl #24
    222c:	00000000 	andeq	r0, r0, r0
    2230:	003f0044 	eorseq	r0, pc, r4, asr #32
	...
    223c:	00480044 	subeq	r0, r8, r4, asr #32
    2240:	00000018 	andeq	r0, r0, r8, lsl r0
    2244:	00000000 	andeq	r0, r0, r0
    2248:	004e0044 	subeq	r0, lr, r4, asr #32
    224c:	00000034 	andeq	r0, r0, r4, lsr r0
    2250:	00000000 	andeq	r0, r0, r0
    2254:	00530044 	subseq	r0, r3, r4, asr #32
    2258:	00000038 	andeq	r0, r0, r8, lsr r0
    225c:	00000000 	andeq	r0, r0, r0
    2260:	005a0044 	subseq	r0, sl, r4, asr #32
    2264:	0000003c 	andeq	r0, r0, ip, lsr r0
    2268:	00000000 	andeq	r0, r0, r0
    226c:	005b0044 	subseq	r0, fp, r4, asr #32
    2270:	00000054 	andeq	r0, r0, r4, asr r0
    2274:	00000000 	andeq	r0, r0, r0
    2278:	00630044 	rsbeq	r0, r3, r4, asr #32
    227c:	00000070 	andeq	r0, r0, r0, ror r0
    2280:	00000000 	andeq	r0, r0, r0
    2284:	00620044 	rsbeq	r0, r2, r4, asr #32
    2288:	00000078 	andeq	r0, r0, r8, ror r0
    228c:	00000000 	andeq	r0, r0, r0
    2290:	00650044 	rsbeq	r0, r5, r4, asr #32
    2294:	0000008c 	andeq	r0, r0, ip, lsl #1
    2298:	00000000 	andeq	r0, r0, r0
    229c:	00670044 	rsbeq	r0, r7, r4, asr #32
    22a0:	00000090 	muleq	r0, r0, r0
    22a4:	00000000 	andeq	r0, r0, r0
    22a8:	00680044 	rsbeq	r0, r8, r4, asr #32
    22ac:	00000094 	muleq	r0, r4, r0
    22b0:	00000000 	andeq	r0, r0, r0
    22b4:	00700044 	rsbseq	r0, r0, r4, asr #32
    22b8:	000000a4 	andeq	r0, r0, r4, lsr #1
    22bc:	00000000 	andeq	r0, r0, r0
    22c0:	00730044 	rsbseq	r0, r3, r4, asr #32
    22c4:	000000b8 	strheq	r0, [r0], -r8
    22c8:	00000000 	andeq	r0, r0, r0
    22cc:	00750044 	rsbseq	r0, r5, r4, asr #32
    22d0:	000000e4 	andeq	r0, r0, r4, ror #1
    22d4:	00000000 	andeq	r0, r0, r0
    22d8:	00760044 	rsbseq	r0, r6, r4, asr #32
    22dc:	000000ec 	andeq	r0, r0, ip, ror #1
    22e0:	00000000 	andeq	r0, r0, r0
    22e4:	00770044 	rsbseq	r0, r7, r4, asr #32
    22e8:	00000110 	andeq	r0, r0, r0, lsl r1
    22ec:	00000000 	andeq	r0, r0, r0
    22f0:	00780044 	rsbseq	r0, r8, r4, asr #32
    22f4:	0000011c 	andeq	r0, r0, ip, lsl r1
    22f8:	00000000 	andeq	r0, r0, r0
    22fc:	00790044 	rsbseq	r0, r9, r4, asr #32
    2300:	0000012c 	andeq	r0, r0, ip, lsr #2
    2304:	00000000 	andeq	r0, r0, r0
    2308:	007c0044 	rsbseq	r0, ip, r4, asr #32
    230c:	00000130 	andeq	r0, r0, r0, lsr r1
    2310:	00000000 	andeq	r0, r0, r0
    2314:	007d0044 	rsbseq	r0, sp, r4, asr #32
    2318:	00000148 	andeq	r0, r0, r8, asr #2
    231c:	00000000 	andeq	r0, r0, r0
    2320:	00800044 	addeq	r0, r0, r4, asr #32
    2324:	00000170 	andeq	r0, r0, r0, ror r1
    2328:	00000000 	andeq	r0, r0, r0
    232c:	00810044 	addeq	r0, r1, r4, asr #32
    2330:	00000174 	andeq	r0, r0, r4, ror r1
    2334:	00000000 	andeq	r0, r0, r0
    2338:	00840044 	addeq	r0, r4, r4, asr #32
    233c:	00000178 	andeq	r0, r0, r8, ror r1
    2340:	00000000 	andeq	r0, r0, r0
    2344:	00850044 	addeq	r0, r5, r4, asr #32
    2348:	0000017c 	andeq	r0, r0, ip, ror r1
    234c:	00000000 	andeq	r0, r0, r0
    2350:	00890044 	addeq	r0, r9, r4, asr #32
    2354:	00000180 	andeq	r0, r0, r0, lsl #3
    2358:	00000000 	andeq	r0, r0, r0
    235c:	008a0044 	addeq	r0, sl, r4, asr #32
    2360:	00000184 	andeq	r0, r0, r4, lsl #3
    2364:	000018fa 	strdeq	r1, [r0], -sl
    2368:	006f0028 	rsbeq	r0, pc, r8, lsr #32
    236c:	80024020 	andhi	r4, r2, r0, lsr #32
    2370:	0000190e 	andeq	r1, r0, lr, lsl #18
    2374:	00750080 	rsbseq	r0, r5, r0, lsl #1
    2378:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    237c:	00001919 	andeq	r1, r0, r9, lsl r9
    2380:	00760080 	rsbseq	r0, r6, r0, lsl #1
    2384:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2388:	00000000 	andeq	r0, r0, r0
    238c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    2398:	000000e0 	andeq	r0, r0, r0, ror #1
    239c:	000001bc 			; <UNDEFINED> instruction: 0x000001bc
    23a0:	00000000 	andeq	r0, r0, r0
    23a4:	00000024 	andeq	r0, r0, r4, lsr #32
    23a8:	000001bc 			; <UNDEFINED> instruction: 0x000001bc
    23ac:	00000000 	andeq	r0, r0, r0
    23b0:	0000004e 	andeq	r0, r0, lr, asr #32
    23b4:	80010dc4 	andhi	r0, r1, r4, asr #27
    23b8:	00001921 	andeq	r1, r0, r1, lsr #18
    23bc:	00060026 	andeq	r0, r6, r6, lsr #32
    23c0:	80015194 	mulhi	r1, r4, r1
    23c4:	0000193d 	andeq	r1, r0, sp, lsr r9
    23c8:	00390020 	eorseq	r0, r9, r0, lsr #32
    23cc:	00000000 	andeq	r0, r0, r0
    23d0:	00001955 	andeq	r1, r0, r5, asr r9
    23d4:	003a0020 	eorseq	r0, sl, r0, lsr #32
    23d8:	00000000 	andeq	r0, r0, r0
    23dc:	00001969 	andeq	r1, r0, r9, ror #18
    23e0:	000e0020 	andeq	r0, lr, r0, lsr #32
	...
    23ec:	00000064 	andeq	r0, r0, r4, rrx
    23f0:	80010dc4 	andhi	r0, r1, r4, asr #27
    23f4:	00000007 	andeq	r0, r0, r7
    23f8:	00020064 	andeq	r0, r2, r4, rrx
    23fc:	80010dc4 	andhi	r0, r1, r4, asr #27
    2400:	0000197b 	andeq	r1, r0, fp, ror r9
    2404:	00020064 	andeq	r0, r2, r4, rrx
    2408:	80010dc4 	andhi	r0, r1, r4, asr #27
    240c:	0000003d 	andeq	r0, r0, sp, lsr r0
    2410:	0000003c 	andeq	r0, r0, ip, lsr r0
    2414:	00000000 	andeq	r0, r0, r0
    2418:	0000004c 	andeq	r0, r0, ip, asr #32
    241c:	00000080 	andeq	r0, r0, r0, lsl #1
    2420:	00000000 	andeq	r0, r0, r0
    2424:	00000076 	andeq	r0, r0, r6, ror r0
    2428:	00000080 	andeq	r0, r0, r0, lsl #1
    242c:	00000000 	andeq	r0, r0, r0
    2430:	00000094 	muleq	r0, r4, r0
    2434:	00000080 	andeq	r0, r0, r0, lsl #1
    2438:	00000000 	andeq	r0, r0, r0
    243c:	000000c3 	andeq	r0, r0, r3, asr #1
    2440:	00000080 	andeq	r0, r0, r0, lsl #1
    2444:	00000000 	andeq	r0, r0, r0
    2448:	000000ee 	andeq	r0, r0, lr, ror #1
    244c:	00000080 	andeq	r0, r0, r0, lsl #1
    2450:	00000000 	andeq	r0, r0, r0
    2454:	0000011e 	andeq	r0, r0, lr, lsl r1
    2458:	00000080 	andeq	r0, r0, r0, lsl #1
    245c:	00000000 	andeq	r0, r0, r0
    2460:	0000016f 	andeq	r0, r0, pc, ror #2
    2464:	00000080 	andeq	r0, r0, r0, lsl #1
    2468:	00000000 	andeq	r0, r0, r0
    246c:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    2470:	00000080 	andeq	r0, r0, r0, lsl #1
    2474:	00000000 	andeq	r0, r0, r0
    2478:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    247c:	00000080 	andeq	r0, r0, r0, lsl #1
    2480:	00000000 	andeq	r0, r0, r0
    2484:	0000020e 	andeq	r0, r0, lr, lsl #4
    2488:	00000080 	andeq	r0, r0, r0, lsl #1
    248c:	00000000 	andeq	r0, r0, r0
    2490:	00000238 	andeq	r0, r0, r8, lsr r2
    2494:	00000080 	andeq	r0, r0, r0, lsl #1
    2498:	00000000 	andeq	r0, r0, r0
    249c:	00000261 	andeq	r0, r0, r1, ror #4
    24a0:	00000080 	andeq	r0, r0, r0, lsl #1
    24a4:	00000000 	andeq	r0, r0, r0
    24a8:	0000027b 	andeq	r0, r0, fp, ror r2
    24ac:	00000080 	andeq	r0, r0, r0, lsl #1
    24b0:	00000000 	andeq	r0, r0, r0
    24b4:	00000296 	muleq	r0, r6, r2
    24b8:	00000080 	andeq	r0, r0, r0, lsl #1
    24bc:	00000000 	andeq	r0, r0, r0
    24c0:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    24c4:	00000080 	andeq	r0, r0, r0, lsl #1
    24c8:	00000000 	andeq	r0, r0, r0
    24cc:	000002d7 	ldrdeq	r0, [r0], -r7
    24d0:	00000080 	andeq	r0, r0, r0, lsl #1
    24d4:	00000000 	andeq	r0, r0, r0
    24d8:	000002f7 	strdeq	r0, [r0], -r7
    24dc:	00000080 	andeq	r0, r0, r0, lsl #1
    24e0:	00000000 	andeq	r0, r0, r0
    24e4:	0000031c 	andeq	r0, r0, ip, lsl r3
    24e8:	00000080 	andeq	r0, r0, r0, lsl #1
    24ec:	00000000 	andeq	r0, r0, r0
    24f0:	00000346 	andeq	r0, r0, r6, asr #6
    24f4:	00000080 	andeq	r0, r0, r0, lsl #1
    24f8:	00000000 	andeq	r0, r0, r0
    24fc:	0000036a 	andeq	r0, r0, sl, ror #6
    2500:	00000080 	andeq	r0, r0, r0, lsl #1
    2504:	00000000 	andeq	r0, r0, r0
    2508:	00000393 	muleq	r0, r3, r3
    250c:	00000080 	andeq	r0, r0, r0, lsl #1
    2510:	00000000 	andeq	r0, r0, r0
    2514:	000003c1 	andeq	r0, r0, r1, asr #7
    2518:	00000080 	andeq	r0, r0, r0, lsl #1
    251c:	00000000 	andeq	r0, r0, r0
    2520:	000003e7 	andeq	r0, r0, r7, ror #7
    2524:	00000080 	andeq	r0, r0, r0, lsl #1
    2528:	00000000 	andeq	r0, r0, r0
    252c:	00000407 	andeq	r0, r0, r7, lsl #8
    2530:	00000080 	andeq	r0, r0, r0, lsl #1
    2534:	00000000 	andeq	r0, r0, r0
    2538:	0000042c 	andeq	r0, r0, ip, lsr #8
    253c:	00000080 	andeq	r0, r0, r0, lsl #1
    2540:	00000000 	andeq	r0, r0, r0
    2544:	00000456 	andeq	r0, r0, r6, asr r4
    2548:	00000080 	andeq	r0, r0, r0, lsl #1
    254c:	00000000 	andeq	r0, r0, r0
    2550:	00000485 	andeq	r0, r0, r5, lsl #9
    2554:	00000080 	andeq	r0, r0, r0, lsl #1
    2558:	00000000 	andeq	r0, r0, r0
    255c:	000004ae 	andeq	r0, r0, lr, lsr #9
    2560:	00000080 	andeq	r0, r0, r0, lsl #1
    2564:	00000000 	andeq	r0, r0, r0
    2568:	000004dc 	ldrdeq	r0, [r0], -ip
    256c:	00000080 	andeq	r0, r0, r0, lsl #1
    2570:	00000000 	andeq	r0, r0, r0
    2574:	0000050f 	andeq	r0, r0, pc, lsl #10
    2578:	00000080 	andeq	r0, r0, r0, lsl #1
    257c:	00000000 	andeq	r0, r0, r0
    2580:	00000530 	andeq	r0, r0, r0, lsr r5
    2584:	00000080 	andeq	r0, r0, r0, lsl #1
    2588:	00000000 	andeq	r0, r0, r0
    258c:	00000550 	andeq	r0, r0, r0, asr r5
    2590:	00000080 	andeq	r0, r0, r0, lsl #1
    2594:	00000000 	andeq	r0, r0, r0
    2598:	00000575 	andeq	r0, r0, r5, ror r5
    259c:	00000080 	andeq	r0, r0, r0, lsl #1
    25a0:	00000000 	andeq	r0, r0, r0
    25a4:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    25a8:	00000080 	andeq	r0, r0, r0, lsl #1
    25ac:	00000000 	andeq	r0, r0, r0
    25b0:	000005c3 	andeq	r0, r0, r3, asr #11
    25b4:	00000080 	andeq	r0, r0, r0, lsl #1
    25b8:	00000000 	andeq	r0, r0, r0
    25bc:	000005ec 	andeq	r0, r0, ip, ror #11
    25c0:	00000080 	andeq	r0, r0, r0, lsl #1
    25c4:	00000000 	andeq	r0, r0, r0
    25c8:	0000061a 	andeq	r0, r0, sl, lsl r6
    25cc:	00000080 	andeq	r0, r0, r0, lsl #1
    25d0:	00000000 	andeq	r0, r0, r0
    25d4:	00000640 	andeq	r0, r0, r0, asr #12
    25d8:	00000080 	andeq	r0, r0, r0, lsl #1
    25dc:	00000000 	andeq	r0, r0, r0
    25e0:	00000660 	andeq	r0, r0, r0, ror #12
    25e4:	00000080 	andeq	r0, r0, r0, lsl #1
    25e8:	00000000 	andeq	r0, r0, r0
    25ec:	00000685 	andeq	r0, r0, r5, lsl #13
    25f0:	00000080 	andeq	r0, r0, r0, lsl #1
    25f4:	00000000 	andeq	r0, r0, r0
    25f8:	000006af 	andeq	r0, r0, pc, lsr #13
    25fc:	00000080 	andeq	r0, r0, r0, lsl #1
    2600:	00000000 	andeq	r0, r0, r0
    2604:	000006de 	ldrdeq	r0, [r0], -lr
    2608:	00000080 	andeq	r0, r0, r0, lsl #1
    260c:	00000000 	andeq	r0, r0, r0
    2610:	00000707 	andeq	r0, r0, r7, lsl #14
    2614:	00000080 	andeq	r0, r0, r0, lsl #1
    2618:	00000000 	andeq	r0, r0, r0
    261c:	00000735 	andeq	r0, r0, r5, lsr r7
    2620:	00000080 	andeq	r0, r0, r0, lsl #1
    2624:	00000000 	andeq	r0, r0, r0
    2628:	00000768 	andeq	r0, r0, r8, ror #14
    262c:	00000080 	andeq	r0, r0, r0, lsl #1
    2630:	00000000 	andeq	r0, r0, r0
    2634:	0000077c 	andeq	r0, r0, ip, ror r7
    2638:	000000c2 	andeq	r0, r0, r2, asr #1
    263c:	00000bc7 	andeq	r0, r0, r7, asr #23
    2640:	0000078a 	andeq	r0, r0, sl, lsl #15
    2644:	000000c2 	andeq	r0, r0, r2, asr #1
    2648:	00002d60 	andeq	r2, r0, r0, ror #26
    264c:	00001985 	andeq	r1, r0, r5, lsl #19
    2650:	000a0024 	andeq	r0, sl, r4, lsr #32
    2654:	80010dc4 	andhi	r0, r1, r4, asr #27
    2658:	00000a54 	andeq	r0, r0, r4, asr sl
    265c:	000a00a0 	andeq	r0, sl, r0, lsr #1
    2660:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2664:	000019a6 	andeq	r1, r0, r6, lsr #19
    2668:	000a00a0 	andeq	r0, sl, r0, lsr #1
    266c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    2670:	00000000 	andeq	r0, r0, r0
    2674:	0000002e 	andeq	r0, r0, lr, lsr #32
    2678:	80010dc4 	andhi	r0, r1, r4, asr #27
    267c:	00000000 	andeq	r0, r0, r0
    2680:	000b0044 	andeq	r0, fp, r4, asr #32
	...
    268c:	000c0044 	andeq	r0, ip, r4, asr #32
    2690:	00000018 	andeq	r0, r0, r8, lsl r0
    2694:	00000000 	andeq	r0, r0, r0
    2698:	000d0044 	andeq	r0, sp, r4, asr #32
    269c:	00000020 	andeq	r0, r0, r0, lsr #32
    26a0:	00000000 	andeq	r0, r0, r0
    26a4:	000e0044 	andeq	r0, lr, r4, asr #32
    26a8:	00000034 	andeq	r0, r0, r4, lsr r0
    26ac:	00000000 	andeq	r0, r0, r0
    26b0:	00000024 	andeq	r0, r0, r4, lsr #32
    26b4:	00000040 	andeq	r0, r0, r0, asr #32
    26b8:	00000000 	andeq	r0, r0, r0
    26bc:	0000004e 	andeq	r0, r0, lr, asr #32
    26c0:	80010e04 	andhi	r0, r1, r4, lsl #28
    26c4:	000019b5 			; <UNDEFINED> instruction: 0x000019b5
    26c8:	00160024 	andseq	r0, r6, r4, lsr #32
    26cc:	80010e04 	andhi	r0, r1, r4, lsl #28
    26d0:	00000000 	andeq	r0, r0, r0
    26d4:	0000002e 	andeq	r0, r0, lr, lsr #32
    26d8:	80010e04 	andhi	r0, r1, r4, lsl #28
    26dc:	00000000 	andeq	r0, r0, r0
    26e0:	00170044 	andseq	r0, r7, r4, asr #32
	...
    26ec:	00180044 	andseq	r0, r8, r4, asr #32
    26f0:	00000010 	andeq	r0, r0, r0, lsl r0
    26f4:	00000000 	andeq	r0, r0, r0
    26f8:	001a0044 	andseq	r0, sl, r4, asr #32
    26fc:	00000018 	andeq	r0, r0, r8, lsl r0
    2700:	00000000 	andeq	r0, r0, r0
    2704:	001c0044 	andseq	r0, ip, r4, asr #32
    2708:	00000024 	andeq	r0, r0, r4, lsr #32
    270c:	00000000 	andeq	r0, r0, r0
    2710:	001d0044 	andseq	r0, sp, r4, asr #32
    2714:	00000030 	andeq	r0, r0, r0, lsr r0
    2718:	00000000 	andeq	r0, r0, r0
    271c:	001e0044 	andseq	r0, lr, r4, asr #32
    2720:	0000004c 	andeq	r0, r0, ip, asr #32
    2724:	00000000 	andeq	r0, r0, r0
    2728:	001f0044 	andseq	r0, pc, r4, asr #32
    272c:	00000060 	andeq	r0, r0, r0, rrx
    2730:	00000000 	andeq	r0, r0, r0
    2734:	00200044 	eoreq	r0, r0, r4, asr #32
    2738:	0000006c 	andeq	r0, r0, ip, rrx
    273c:	00000000 	andeq	r0, r0, r0
    2740:	001c0044 	andseq	r0, ip, r4, asr #32
    2744:	00000074 	andeq	r0, r0, r4, ror r0
    2748:	00000000 	andeq	r0, r0, r0
    274c:	001c0044 	andseq	r0, ip, r4, asr #32
    2750:	00000080 	andeq	r0, r0, r0, lsl #1
    2754:	00000000 	andeq	r0, r0, r0
    2758:	00230044 	eoreq	r0, r3, r4, asr #32
    275c:	0000008c 	andeq	r0, r0, ip, lsl #1
    2760:	000019c7 	andeq	r1, r0, r7, asr #19
    2764:	00180080 	andseq	r0, r8, r0, lsl #1
    2768:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    276c:	000019d5 	ldrdeq	r1, [r0], -r5
    2770:	00190080 	andseq	r0, r9, r0, lsl #1
    2774:	ffffff6c 			; <UNDEFINED> instruction: 0xffffff6c
    2778:	00000000 	andeq	r0, r0, r0
    277c:	000000c0 	andeq	r0, r0, r0, asr #1
    2780:	00000000 	andeq	r0, r0, r0
    2784:	00000ad8 	ldrdeq	r0, [r0], -r8
    2788:	001c0080 	andseq	r0, ip, r0, lsl #1
    278c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2790:	00000000 	andeq	r0, r0, r0
    2794:	000000c0 	andeq	r0, r0, r0, asr #1
    2798:	00000024 	andeq	r0, r0, r4, lsr #32
    279c:	00000000 	andeq	r0, r0, r0
    27a0:	000000e0 	andeq	r0, r0, r0, ror #1
    27a4:	0000008c 	andeq	r0, r0, ip, lsl #1
    27a8:	00000000 	andeq	r0, r0, r0
    27ac:	000000e0 	andeq	r0, r0, r0, ror #1
    27b0:	00000098 	muleq	r0, r8, r0
    27b4:	00000000 	andeq	r0, r0, r0
    27b8:	00000024 	andeq	r0, r0, r4, lsr #32
    27bc:	00000098 	muleq	r0, r8, r0
    27c0:	00000000 	andeq	r0, r0, r0
    27c4:	0000004e 	andeq	r0, r0, lr, asr #32
    27c8:	80010e9c 	mulhi	r1, ip, lr
    27cc:	00001a14 	andeq	r1, r0, r4, lsl sl
    27d0:	00040026 	andeq	r0, r4, r6, lsr #32
    27d4:	8002402c 	andhi	r4, r2, ip, lsr #32
    27d8:	00000000 	andeq	r0, r0, r0
    27dc:	00000064 	andeq	r0, r0, r4, rrx
    27e0:	80010e9c 	mulhi	r1, ip, lr
    27e4:	00000007 	andeq	r0, r0, r7
    27e8:	00020064 	andeq	r0, r2, r4, rrx
    27ec:	80010e9c 	mulhi	r1, ip, lr
    27f0:	00001a43 	andeq	r1, r0, r3, asr #20
    27f4:	00020064 	andeq	r0, r2, r4, rrx
    27f8:	80010e9c 	mulhi	r1, ip, lr
    27fc:	0000003d 	andeq	r0, r0, sp, lsr r0
    2800:	0000003c 	andeq	r0, r0, ip, lsr r0
    2804:	00000000 	andeq	r0, r0, r0
    2808:	0000004c 	andeq	r0, r0, ip, asr #32
    280c:	00000080 	andeq	r0, r0, r0, lsl #1
    2810:	00000000 	andeq	r0, r0, r0
    2814:	00000076 	andeq	r0, r0, r6, ror r0
    2818:	00000080 	andeq	r0, r0, r0, lsl #1
    281c:	00000000 	andeq	r0, r0, r0
    2820:	00000094 	muleq	r0, r4, r0
    2824:	00000080 	andeq	r0, r0, r0, lsl #1
    2828:	00000000 	andeq	r0, r0, r0
    282c:	000000c3 	andeq	r0, r0, r3, asr #1
    2830:	00000080 	andeq	r0, r0, r0, lsl #1
    2834:	00000000 	andeq	r0, r0, r0
    2838:	000000ee 	andeq	r0, r0, lr, ror #1
    283c:	00000080 	andeq	r0, r0, r0, lsl #1
    2840:	00000000 	andeq	r0, r0, r0
    2844:	0000011e 	andeq	r0, r0, lr, lsl r1
    2848:	00000080 	andeq	r0, r0, r0, lsl #1
    284c:	00000000 	andeq	r0, r0, r0
    2850:	0000016f 	andeq	r0, r0, pc, ror #2
    2854:	00000080 	andeq	r0, r0, r0, lsl #1
    2858:	00000000 	andeq	r0, r0, r0
    285c:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    2860:	00000080 	andeq	r0, r0, r0, lsl #1
    2864:	00000000 	andeq	r0, r0, r0
    2868:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    286c:	00000080 	andeq	r0, r0, r0, lsl #1
    2870:	00000000 	andeq	r0, r0, r0
    2874:	0000020e 	andeq	r0, r0, lr, lsl #4
    2878:	00000080 	andeq	r0, r0, r0, lsl #1
    287c:	00000000 	andeq	r0, r0, r0
    2880:	00000238 	andeq	r0, r0, r8, lsr r2
    2884:	00000080 	andeq	r0, r0, r0, lsl #1
    2888:	00000000 	andeq	r0, r0, r0
    288c:	00000261 	andeq	r0, r0, r1, ror #4
    2890:	00000080 	andeq	r0, r0, r0, lsl #1
    2894:	00000000 	andeq	r0, r0, r0
    2898:	0000027b 	andeq	r0, r0, fp, ror r2
    289c:	00000080 	andeq	r0, r0, r0, lsl #1
    28a0:	00000000 	andeq	r0, r0, r0
    28a4:	00000296 	muleq	r0, r6, r2
    28a8:	00000080 	andeq	r0, r0, r0, lsl #1
    28ac:	00000000 	andeq	r0, r0, r0
    28b0:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    28b4:	00000080 	andeq	r0, r0, r0, lsl #1
    28b8:	00000000 	andeq	r0, r0, r0
    28bc:	000002d7 	ldrdeq	r0, [r0], -r7
    28c0:	00000080 	andeq	r0, r0, r0, lsl #1
    28c4:	00000000 	andeq	r0, r0, r0
    28c8:	000002f7 	strdeq	r0, [r0], -r7
    28cc:	00000080 	andeq	r0, r0, r0, lsl #1
    28d0:	00000000 	andeq	r0, r0, r0
    28d4:	0000031c 	andeq	r0, r0, ip, lsl r3
    28d8:	00000080 	andeq	r0, r0, r0, lsl #1
    28dc:	00000000 	andeq	r0, r0, r0
    28e0:	00000346 	andeq	r0, r0, r6, asr #6
    28e4:	00000080 	andeq	r0, r0, r0, lsl #1
    28e8:	00000000 	andeq	r0, r0, r0
    28ec:	0000036a 	andeq	r0, r0, sl, ror #6
    28f0:	00000080 	andeq	r0, r0, r0, lsl #1
    28f4:	00000000 	andeq	r0, r0, r0
    28f8:	00000393 	muleq	r0, r3, r3
    28fc:	00000080 	andeq	r0, r0, r0, lsl #1
    2900:	00000000 	andeq	r0, r0, r0
    2904:	000003c1 	andeq	r0, r0, r1, asr #7
    2908:	00000080 	andeq	r0, r0, r0, lsl #1
    290c:	00000000 	andeq	r0, r0, r0
    2910:	000003e7 	andeq	r0, r0, r7, ror #7
    2914:	00000080 	andeq	r0, r0, r0, lsl #1
    2918:	00000000 	andeq	r0, r0, r0
    291c:	00000407 	andeq	r0, r0, r7, lsl #8
    2920:	00000080 	andeq	r0, r0, r0, lsl #1
    2924:	00000000 	andeq	r0, r0, r0
    2928:	0000042c 	andeq	r0, r0, ip, lsr #8
    292c:	00000080 	andeq	r0, r0, r0, lsl #1
    2930:	00000000 	andeq	r0, r0, r0
    2934:	00000456 	andeq	r0, r0, r6, asr r4
    2938:	00000080 	andeq	r0, r0, r0, lsl #1
    293c:	00000000 	andeq	r0, r0, r0
    2940:	00000485 	andeq	r0, r0, r5, lsl #9
    2944:	00000080 	andeq	r0, r0, r0, lsl #1
    2948:	00000000 	andeq	r0, r0, r0
    294c:	000004ae 	andeq	r0, r0, lr, lsr #9
    2950:	00000080 	andeq	r0, r0, r0, lsl #1
    2954:	00000000 	andeq	r0, r0, r0
    2958:	000004dc 	ldrdeq	r0, [r0], -ip
    295c:	00000080 	andeq	r0, r0, r0, lsl #1
    2960:	00000000 	andeq	r0, r0, r0
    2964:	0000050f 	andeq	r0, r0, pc, lsl #10
    2968:	00000080 	andeq	r0, r0, r0, lsl #1
    296c:	00000000 	andeq	r0, r0, r0
    2970:	00000530 	andeq	r0, r0, r0, lsr r5
    2974:	00000080 	andeq	r0, r0, r0, lsl #1
    2978:	00000000 	andeq	r0, r0, r0
    297c:	00000550 	andeq	r0, r0, r0, asr r5
    2980:	00000080 	andeq	r0, r0, r0, lsl #1
    2984:	00000000 	andeq	r0, r0, r0
    2988:	00000575 	andeq	r0, r0, r5, ror r5
    298c:	00000080 	andeq	r0, r0, r0, lsl #1
    2990:	00000000 	andeq	r0, r0, r0
    2994:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    2998:	00000080 	andeq	r0, r0, r0, lsl #1
    299c:	00000000 	andeq	r0, r0, r0
    29a0:	000005c3 	andeq	r0, r0, r3, asr #11
    29a4:	00000080 	andeq	r0, r0, r0, lsl #1
    29a8:	00000000 	andeq	r0, r0, r0
    29ac:	000005ec 	andeq	r0, r0, ip, ror #11
    29b0:	00000080 	andeq	r0, r0, r0, lsl #1
    29b4:	00000000 	andeq	r0, r0, r0
    29b8:	0000061a 	andeq	r0, r0, sl, lsl r6
    29bc:	00000080 	andeq	r0, r0, r0, lsl #1
    29c0:	00000000 	andeq	r0, r0, r0
    29c4:	00000640 	andeq	r0, r0, r0, asr #12
    29c8:	00000080 	andeq	r0, r0, r0, lsl #1
    29cc:	00000000 	andeq	r0, r0, r0
    29d0:	00000660 	andeq	r0, r0, r0, ror #12
    29d4:	00000080 	andeq	r0, r0, r0, lsl #1
    29d8:	00000000 	andeq	r0, r0, r0
    29dc:	00000685 	andeq	r0, r0, r5, lsl #13
    29e0:	00000080 	andeq	r0, r0, r0, lsl #1
    29e4:	00000000 	andeq	r0, r0, r0
    29e8:	000006af 	andeq	r0, r0, pc, lsr #13
    29ec:	00000080 	andeq	r0, r0, r0, lsl #1
    29f0:	00000000 	andeq	r0, r0, r0
    29f4:	000006de 	ldrdeq	r0, [r0], -lr
    29f8:	00000080 	andeq	r0, r0, r0, lsl #1
    29fc:	00000000 	andeq	r0, r0, r0
    2a00:	00000707 	andeq	r0, r0, r7, lsl #14
    2a04:	00000080 	andeq	r0, r0, r0, lsl #1
    2a08:	00000000 	andeq	r0, r0, r0
    2a0c:	00000735 	andeq	r0, r0, r5, lsr r7
    2a10:	00000080 	andeq	r0, r0, r0, lsl #1
    2a14:	00000000 	andeq	r0, r0, r0
    2a18:	00000768 	andeq	r0, r0, r8, ror #14
    2a1c:	00000080 	andeq	r0, r0, r0, lsl #1
    2a20:	00000000 	andeq	r0, r0, r0
    2a24:	00001a52 	andeq	r1, r0, r2, asr sl
    2a28:	00000082 	andeq	r0, r0, r2, lsl #1
    2a2c:	0000ad5a 	andeq	sl, r0, sl, asr sp
    2a30:	00001a68 	andeq	r1, r0, r8, ror #20
    2a34:	00000080 	andeq	r0, r0, r0, lsl #1
    2a38:	00000000 	andeq	r0, r0, r0
    2a3c:	00001cb3 			; <UNDEFINED> instruction: 0x00001cb3
    2a40:	002b0080 	eoreq	r0, fp, r0, lsl #1
	...
    2a4c:	000000a2 	andeq	r0, r0, r2, lsr #1
    2a50:	00000000 	andeq	r0, r0, r0
    2a54:	00001ccd 	andeq	r1, r0, sp, asr #25
    2a58:	00000082 	andeq	r0, r0, r2, lsl #1
    2a5c:	00000000 	andeq	r0, r0, r0
    2a60:	0000078a 	andeq	r0, r0, sl, lsl #15
    2a64:	000000c2 	andeq	r0, r0, r2, asr #1
    2a68:	00002d60 	andeq	r2, r0, r0, ror #26
    2a6c:	00000000 	andeq	r0, r0, r0
    2a70:	000000a2 	andeq	r0, r0, r2, lsr #1
    2a74:	00000000 	andeq	r0, r0, r0
    2a78:	00001173 	andeq	r1, r0, r3, ror r1
    2a7c:	00000082 	andeq	r0, r0, r2, lsl #1
    2a80:	0000c9ad 	andeq	ip, r0, sp, lsr #19
    2a84:	00000886 	andeq	r0, r0, r6, lsl #17
    2a88:	000000c2 	andeq	r0, r0, r2, asr #1
    2a8c:	00003ac8 	andeq	r3, r0, r8, asr #21
    2a90:	00001182 	andeq	r1, r0, r2, lsl #3
    2a94:	000000c2 	andeq	r0, r0, r2, asr #1
    2a98:	00004df6 	strdeq	r4, [r0], -r6
    2a9c:	000012f3 	strdeq	r1, [r0], -r3
    2aa0:	000000c2 	andeq	r0, r0, r2, asr #1
    2aa4:	00002a19 	andeq	r2, r0, r9, lsl sl
    2aa8:	00001306 	andeq	r1, r0, r6, lsl #6
    2aac:	000000c2 	andeq	r0, r0, r2, asr #1
    2ab0:	0000151b 	andeq	r1, r0, fp, lsl r5
    2ab4:	00001425 	andeq	r1, r0, r5, lsr #8
    2ab8:	000000c2 	andeq	r0, r0, r2, asr #1
    2abc:	00001fc5 	andeq	r1, r0, r5, asr #31
    2ac0:	000014c3 	andeq	r1, r0, r3, asr #9
    2ac4:	00000082 	andeq	r0, r0, r2, lsl #1
    2ac8:	00002743 	andeq	r2, r0, r3, asr #14
    2acc:	00001cde 	ldrdeq	r1, [r0], -lr
    2ad0:	000f0080 	andeq	r0, pc, r0, lsl #1
	...
    2adc:	000000a2 	andeq	r0, r0, r2, lsr #1
    2ae0:	00000000 	andeq	r0, r0, r0
    2ae4:	00001d8a 	andeq	r1, r0, sl, lsl #27
    2ae8:	00180080 	andseq	r0, r8, r0, lsl #1
    2aec:	00000000 	andeq	r0, r0, r0
    2af0:	00001db5 			; <UNDEFINED> instruction: 0x00001db5
    2af4:	00000080 	andeq	r0, r0, r0, lsl #1
    2af8:	00000000 	andeq	r0, r0, r0
    2afc:	00001e0d 	andeq	r1, r0, sp, lsl #28
    2b00:	00000080 	andeq	r0, r0, r0, lsl #1
    2b04:	00000000 	andeq	r0, r0, r0
    2b08:	00001e92 	muleq	r0, r2, lr
    2b0c:	00310080 	eorseq	r0, r1, r0, lsl #1
    2b10:	00000000 	andeq	r0, r0, r0
    2b14:	00001ee6 	andeq	r1, r0, r6, ror #29
    2b18:	004c0080 	subeq	r0, ip, r0, lsl #1
	...
    2b24:	000000a2 	andeq	r0, r0, r2, lsr #1
    2b28:	00000000 	andeq	r0, r0, r0
    2b2c:	00001056 	andeq	r1, r0, r6, asr r0
    2b30:	000000c2 	andeq	r0, r0, r2, asr #1
    2b34:	00000000 	andeq	r0, r0, r0
    2b38:	00001067 	andeq	r1, r0, r7, rrx
    2b3c:	00000082 	andeq	r0, r0, r2, lsl #1
    2b40:	00002f44 	andeq	r2, r0, r4, asr #30
    2b44:	000020bf 	strheq	r2, [r0], -pc	; <UNPREDICTABLE>
    2b48:	00000080 	andeq	r0, r0, r0, lsl #1
    2b4c:	00000000 	andeq	r0, r0, r0
    2b50:	0000213b 	andeq	r2, r0, fp, lsr r1
    2b54:	00110080 	andseq	r0, r1, r0, lsl #1
    2b58:	00000000 	andeq	r0, r0, r0
    2b5c:	00002153 	andeq	r2, r0, r3, asr r1
    2b60:	00160080 	andseq	r0, r6, r0, lsl #1
	...
    2b6c:	000000a2 	andeq	r0, r0, r2, lsr #1
    2b70:	00000000 	andeq	r0, r0, r0
    2b74:	00002199 	muleq	r0, r9, r1
    2b78:	00000082 	andeq	r0, r0, r2, lsl #1
    2b7c:	0000112c 	andeq	r1, r0, ip, lsr #2
    2b80:	000021a9 	andeq	r2, r0, r9, lsr #3
    2b84:	00090080 	andeq	r0, r9, r0, lsl #1
	...
    2b90:	000000a2 	andeq	r0, r0, r2, lsr #1
    2b94:	00000000 	andeq	r0, r0, r0
    2b98:	000021fb 	strdeq	r2, [r0], -fp
    2b9c:	00000082 	andeq	r0, r0, r2, lsl #1
    2ba0:	00001a90 	muleq	r0, r0, sl
    2ba4:	0000220c 	andeq	r2, r0, ip, lsl #4
    2ba8:	00000080 	andeq	r0, r0, r0, lsl #1
    2bac:	00000000 	andeq	r0, r0, r0
    2bb0:	00002268 	andeq	r2, r0, r8, ror #4
    2bb4:	000f0080 	andeq	r0, pc, r0, lsl #1
	...
    2bc0:	000000a2 	andeq	r0, r0, r2, lsr #1
    2bc4:	00000000 	andeq	r0, r0, r0
    2bc8:	0000227f 	andeq	r2, r0, pc, ror r2
    2bcc:	00130024 	andseq	r0, r3, r4, lsr #32
    2bd0:	80010e9c 	mulhi	r1, ip, lr
    2bd4:	00000eed 	andeq	r0, r0, sp, ror #29
    2bd8:	001300a0 	andseq	r0, r3, r0, lsr #1
    2bdc:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2be0:	00000000 	andeq	r0, r0, r0
    2be4:	0000002e 	andeq	r0, r0, lr, lsr #32
    2be8:	80010e9c 	mulhi	r1, ip, lr
    2bec:	00000000 	andeq	r0, r0, r0
    2bf0:	00140044 	andseq	r0, r4, r4, asr #32
	...
    2bfc:	00150044 	andseq	r0, r5, r4, asr #32
    2c00:	00000014 	andeq	r0, r0, r4, lsl r0
    2c04:	00000000 	andeq	r0, r0, r0
    2c08:	00160044 	andseq	r0, r6, r4, asr #32
    2c0c:	0000001c 	andeq	r0, r0, ip, lsl r0
    2c10:	00000000 	andeq	r0, r0, r0
    2c14:	00170044 	andseq	r0, r7, r4, asr #32
    2c18:	00000020 	andeq	r0, r0, r0, lsr #32
    2c1c:	00000000 	andeq	r0, r0, r0
    2c20:	00000024 	andeq	r0, r0, r4, lsr #32
    2c24:	0000002c 	andeq	r0, r0, ip, lsr #32
    2c28:	00000000 	andeq	r0, r0, r0
    2c2c:	0000004e 	andeq	r0, r0, lr, asr #32
    2c30:	80010ec8 	andhi	r0, r1, r8, asr #29
    2c34:	00002298 	muleq	r0, r8, r2
    2c38:	00190024 	andseq	r0, r9, r4, lsr #32
    2c3c:	80010ec8 	andhi	r0, r1, r8, asr #29
    2c40:	00000000 	andeq	r0, r0, r0
    2c44:	0000002e 	andeq	r0, r0, lr, lsr #32
    2c48:	80010ec8 	andhi	r0, r1, r8, asr #29
    2c4c:	00000000 	andeq	r0, r0, r0
    2c50:	001a0044 	andseq	r0, sl, r4, asr #32
	...
    2c5c:	001b0044 	andseq	r0, fp, r4, asr #32
    2c60:	00000010 	andeq	r0, r0, r0, lsl r0
    2c64:	00000000 	andeq	r0, r0, r0
    2c68:	001c0044 	andseq	r0, ip, r4, asr #32
    2c6c:	00000018 	andeq	r0, r0, r8, lsl r0
    2c70:	00000000 	andeq	r0, r0, r0
    2c74:	001d0044 	andseq	r0, sp, r4, asr #32
    2c78:	0000001c 	andeq	r0, r0, ip, lsl r0
    2c7c:	000022b1 			; <UNDEFINED> instruction: 0x000022b1
    2c80:	001b0080 	andseq	r0, fp, r0, lsl #1
    2c84:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2c88:	00000000 	andeq	r0, r0, r0
    2c8c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    2c98:	000000e0 	andeq	r0, r0, r0, ror #1
    2c9c:	00000028 	andeq	r0, r0, r8, lsr #32
    2ca0:	00000000 	andeq	r0, r0, r0
    2ca4:	00000024 	andeq	r0, r0, r4, lsr #32
    2ca8:	00000028 	andeq	r0, r0, r8, lsr #32
    2cac:	00000000 	andeq	r0, r0, r0
    2cb0:	0000004e 	andeq	r0, r0, lr, asr #32
    2cb4:	80010ef0 	strdhi	r0, [r1], -r0	; <UNPREDICTABLE>
    2cb8:	000022b9 			; <UNDEFINED> instruction: 0x000022b9
    2cbc:	001f0024 	andseq	r0, pc, r4, lsr #32
    2cc0:	80010ef0 	strdhi	r0, [r1], -r0	; <UNPREDICTABLE>
    2cc4:	000022d0 	ldrdeq	r2, [r0], -r0
    2cc8:	001f00a0 	andseq	r0, pc, r0, lsr #1
    2ccc:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    2cd0:	000022dc 	ldrdeq	r2, [r0], -ip
    2cd4:	001f00a0 	andseq	r0, pc, r0, lsr #1
    2cd8:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    2cdc:	00000000 	andeq	r0, r0, r0
    2ce0:	0000002e 	andeq	r0, r0, lr, lsr #32
    2ce4:	80010ef0 	strdhi	r0, [r1], -r0	; <UNPREDICTABLE>
    2ce8:	00000000 	andeq	r0, r0, r0
    2cec:	001f0044 	andseq	r0, pc, r4, asr #32
	...
    2cf8:	00200044 	eoreq	r0, r0, r4, asr #32
    2cfc:	00000020 	andeq	r0, r0, r0, lsr #32
    2d00:	00000000 	andeq	r0, r0, r0
    2d04:	00210044 	eoreq	r0, r1, r4, asr #32
    2d08:	00000028 	andeq	r0, r0, r8, lsr #32
    2d0c:	00000000 	andeq	r0, r0, r0
    2d10:	00230044 	eoreq	r0, r3, r4, asr #32
    2d14:	00000030 	andeq	r0, r0, r0, lsr r0
    2d18:	00000000 	andeq	r0, r0, r0
    2d1c:	00230044 	eoreq	r0, r3, r4, asr #32
    2d20:	0000003c 	andeq	r0, r0, ip, lsr r0
    2d24:	00000000 	andeq	r0, r0, r0
    2d28:	00240044 	eoreq	r0, r4, r4, asr #32
    2d2c:	00000048 	andeq	r0, r0, r8, asr #32
    2d30:	00000000 	andeq	r0, r0, r0
    2d34:	00260044 	eoreq	r0, r6, r4, asr #32
    2d38:	00000050 	andeq	r0, r0, r0, asr r0
    2d3c:	00000000 	andeq	r0, r0, r0
    2d40:	00280044 	eoreq	r0, r8, r4, asr #32
    2d44:	00000070 	andeq	r0, r0, r0, ror r0
    2d48:	00000000 	andeq	r0, r0, r0
    2d4c:	00290044 	eoreq	r0, r9, r4, asr #32
    2d50:	00000094 	muleq	r0, r4, r0
    2d54:	00000000 	andeq	r0, r0, r0
    2d58:	002b0044 	eoreq	r0, fp, r4, asr #32
    2d5c:	0000009c 	muleq	r0, ip, r0
    2d60:	00000000 	andeq	r0, r0, r0
    2d64:	002c0044 	eoreq	r0, ip, r4, asr #32
    2d68:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    2d6c:	00000000 	andeq	r0, r0, r0
    2d70:	002d0044 	eoreq	r0, sp, r4, asr #32
    2d74:	000000b4 	strheq	r0, [r0], -r4
    2d78:	000022e8 	andeq	r2, r0, r8, ror #5
    2d7c:	00200080 	eoreq	r0, r0, r0, lsl #1
    2d80:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2d84:	000022f3 	strdeq	r2, [r0], -r3
    2d88:	00210080 	eoreq	r0, r1, r0, lsl #1
    2d8c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    2d90:	00000000 	andeq	r0, r0, r0
    2d94:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    2da0:	000000e0 	andeq	r0, r0, r0, ror #1
    2da4:	000000c8 	andeq	r0, r0, r8, asr #1
    2da8:	00000000 	andeq	r0, r0, r0
    2dac:	00000024 	andeq	r0, r0, r4, lsr #32
    2db0:	000000c8 	andeq	r0, r0, r8, asr #1
    2db4:	00000000 	andeq	r0, r0, r0
    2db8:	0000004e 	andeq	r0, r0, lr, asr #32
    2dbc:	80010fb8 			; <UNDEFINED> instruction: 0x80010fb8
    2dc0:	000022fc 	strdeq	r2, [r0], -ip
    2dc4:	002f0024 	eoreq	r0, pc, r4, lsr #32
    2dc8:	80010fb8 			; <UNDEFINED> instruction: 0x80010fb8
    2dcc:	00000000 	andeq	r0, r0, r0
    2dd0:	0000002e 	andeq	r0, r0, lr, lsr #32
    2dd4:	80010fb8 			; <UNDEFINED> instruction: 0x80010fb8
    2dd8:	00000000 	andeq	r0, r0, r0
    2ddc:	00300044 	eorseq	r0, r0, r4, asr #32
	...
    2de8:	00310044 	eorseq	r0, r1, r4, asr #32
    2dec:	0000000c 	andeq	r0, r0, ip
    2df0:	00000000 	andeq	r0, r0, r0
    2df4:	00320044 	eorseq	r0, r2, r4, asr #32
    2df8:	00000014 	andeq	r0, r0, r4, lsl r0
    2dfc:	00000000 	andeq	r0, r0, r0
    2e00:	00000024 	andeq	r0, r0, r4, lsr #32
    2e04:	0000001c 	andeq	r0, r0, ip, lsl r0
    2e08:	00000000 	andeq	r0, r0, r0
    2e0c:	0000004e 	andeq	r0, r0, lr, asr #32
    2e10:	80010fd4 	ldrdhi	r0, [r1], -r4
    2e14:	00002310 	andeq	r2, r0, r0, lsl r3
    2e18:	00340024 	eorseq	r0, r4, r4, lsr #32
    2e1c:	80010fd4 	ldrdhi	r0, [r1], -r4
    2e20:	00000000 	andeq	r0, r0, r0
    2e24:	0000002e 	andeq	r0, r0, lr, lsr #32
    2e28:	80010fd4 	ldrdhi	r0, [r1], -r4
    2e2c:	00000000 	andeq	r0, r0, r0
    2e30:	00350044 	eorseq	r0, r5, r4, asr #32
	...
    2e3c:	00360044 	eorseq	r0, r6, r4, asr #32
    2e40:	00000014 	andeq	r0, r0, r4, lsl r0
    2e44:	00000000 	andeq	r0, r0, r0
    2e48:	00370044 	eorseq	r0, r7, r4, asr #32
    2e4c:	00000024 	andeq	r0, r0, r4, lsr #32
    2e50:	00000000 	andeq	r0, r0, r0
    2e54:	00000024 	andeq	r0, r0, r4, lsr #32
    2e58:	00000034 	andeq	r0, r0, r4, lsr r0
    2e5c:	00000000 	andeq	r0, r0, r0
    2e60:	0000004e 	andeq	r0, r0, lr, asr #32
    2e64:	80011008 	andhi	r1, r1, r8
    2e68:	00002326 	andeq	r2, r0, r6, lsr #6
    2e6c:	00390024 	eorseq	r0, r9, r4, lsr #32
    2e70:	80011008 	andhi	r1, r1, r8
    2e74:	000022d0 	ldrdeq	r2, [r0], -r0
    2e78:	003900a0 	eorseq	r0, r9, r0, lsr #1
    2e7c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2e80:	00000000 	andeq	r0, r0, r0
    2e84:	0000002e 	andeq	r0, r0, lr, lsr #32
    2e88:	80011008 	andhi	r1, r1, r8
    2e8c:	00000000 	andeq	r0, r0, r0
    2e90:	003a0044 	eorseq	r0, sl, r4, asr #32
	...
    2e9c:	003c0044 	eorseq	r0, ip, r4, asr #32
    2ea0:	00000014 	andeq	r0, r0, r4, lsl r0
    2ea4:	00000000 	andeq	r0, r0, r0
    2ea8:	003d0044 	eorseq	r0, sp, r4, asr #32
    2eac:	00000018 	andeq	r0, r0, r8, lsl r0
    2eb0:	00000000 	andeq	r0, r0, r0
    2eb4:	003e0044 	eorseq	r0, lr, r4, asr #32
    2eb8:	0000001c 	andeq	r0, r0, ip, lsl r0
    2ebc:	00000000 	andeq	r0, r0, r0
    2ec0:	00000024 	andeq	r0, r0, r4, lsr #32
    2ec4:	00000028 	andeq	r0, r0, r8, lsr #32
    2ec8:	00000000 	andeq	r0, r0, r0
    2ecc:	0000004e 	andeq	r0, r0, lr, asr #32
    2ed0:	80011030 	andhi	r1, r1, r0, lsr r0
    2ed4:	0000233a 	andeq	r2, r0, sl, lsr r3
    2ed8:	00400024 	subeq	r0, r0, r4, lsr #32
    2edc:	80011030 	andhi	r1, r1, r0, lsr r0
    2ee0:	000022d0 	ldrdeq	r2, [r0], -r0
    2ee4:	004000a0 	subeq	r0, r0, r0, lsr #1
    2ee8:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    2eec:	00000000 	andeq	r0, r0, r0
    2ef0:	0000002e 	andeq	r0, r0, lr, lsr #32
    2ef4:	80011030 	andhi	r1, r1, r0, lsr r0
    2ef8:	00000000 	andeq	r0, r0, r0
    2efc:	00410044 	subeq	r0, r1, r4, asr #32
	...
    2f08:	00420044 	subeq	r0, r2, r4, asr #32
    2f0c:	0000001c 	andeq	r0, r0, ip, lsl r0
    2f10:	00000000 	andeq	r0, r0, r0
    2f14:	00430044 	subeq	r0, r3, r4, asr #32
    2f18:	00000028 	andeq	r0, r0, r8, lsr #32
    2f1c:	00000000 	andeq	r0, r0, r0
    2f20:	00440044 	subeq	r0, r4, r4, asr #32
    2f24:	00000034 	andeq	r0, r0, r4, lsr r0
    2f28:	00000000 	andeq	r0, r0, r0
    2f2c:	00450044 	subeq	r0, r5, r4, asr #32
    2f30:	00000048 	andeq	r0, r0, r8, asr #32
    2f34:	00000000 	andeq	r0, r0, r0
    2f38:	00460044 	subeq	r0, r6, r4, asr #32
    2f3c:	0000005c 	andeq	r0, r0, ip, asr r0
    2f40:	00000000 	andeq	r0, r0, r0
    2f44:	00480044 	subeq	r0, r8, r4, asr #32
    2f48:	00000060 	andeq	r0, r0, r0, rrx
    2f4c:	00000000 	andeq	r0, r0, r0
    2f50:	00490044 	subeq	r0, r9, r4, asr #32
    2f54:	00000064 	andeq	r0, r0, r4, rrx
    2f58:	0000234e 	andeq	r2, r0, lr, asr #6
    2f5c:	00420080 	subeq	r0, r2, r0, lsl #1
    2f60:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    2f64:	00000000 	andeq	r0, r0, r0
    2f68:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    2f74:	000000e0 	andeq	r0, r0, r0, ror #1
    2f78:	00000078 	andeq	r0, r0, r8, ror r0
    2f7c:	00000000 	andeq	r0, r0, r0
    2f80:	00000024 	andeq	r0, r0, r4, lsr #32
    2f84:	00000078 	andeq	r0, r0, r8, ror r0
    2f88:	00000000 	andeq	r0, r0, r0
    2f8c:	0000004e 	andeq	r0, r0, lr, asr #32
    2f90:	800110a8 	andhi	r1, r1, r8, lsr #1
    2f94:	00002361 	andeq	r2, r0, r1, ror #6
    2f98:	004c0024 	subeq	r0, ip, r4, lsr #32
    2f9c:	800110a8 	andhi	r1, r1, r8, lsr #1
    2fa0:	00000000 	andeq	r0, r0, r0
    2fa4:	0000002e 	andeq	r0, r0, lr, lsr #32
    2fa8:	800110a8 	andhi	r1, r1, r8, lsr #1
    2fac:	00000000 	andeq	r0, r0, r0
    2fb0:	004c0044 	subeq	r0, ip, r4, asr #32
	...
    2fbc:	004d0044 	subeq	r0, sp, r4, asr #32
    2fc0:	0000000c 	andeq	r0, r0, ip
    2fc4:	00000000 	andeq	r0, r0, r0
    2fc8:	004e0044 	subeq	r0, lr, r4, asr #32
    2fcc:	00000010 	andeq	r0, r0, r0, lsl r0
    2fd0:	00000000 	andeq	r0, r0, r0
    2fd4:	004f0044 	subeq	r0, pc, r4, asr #32
    2fd8:	00000014 	andeq	r0, r0, r4, lsl r0
    2fdc:	00000000 	andeq	r0, r0, r0
    2fe0:	00000024 	andeq	r0, r0, r4, lsr #32
    2fe4:	0000001c 	andeq	r0, r0, ip, lsl r0
    2fe8:	00000000 	andeq	r0, r0, r0
    2fec:	0000004e 	andeq	r0, r0, lr, asr #32
    2ff0:	800110c4 	andhi	r1, r1, r4, asr #1
    2ff4:	00002376 	andeq	r2, r0, r6, ror r3
    2ff8:	00510024 	subseq	r0, r1, r4, lsr #32
    2ffc:	800110c4 	andhi	r1, r1, r4, asr #1
    3000:	000022d0 	ldrdeq	r2, [r0], -r0
    3004:	005100a0 	subseq	r0, r1, r0, lsr #1
    3008:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    300c:	00000000 	andeq	r0, r0, r0
    3010:	0000002e 	andeq	r0, r0, lr, lsr #32
    3014:	800110c4 	andhi	r1, r1, r4, asr #1
    3018:	00000000 	andeq	r0, r0, r0
    301c:	00510044 	subseq	r0, r1, r4, asr #32
	...
    3028:	00520044 	subseq	r0, r2, r4, asr #32
    302c:	0000001c 	andeq	r0, r0, ip, lsl r0
    3030:	00000000 	andeq	r0, r0, r0
    3034:	00530044 	subseq	r0, r3, r4, asr #32
    3038:	00000040 	andeq	r0, r0, r0, asr #32
    303c:	00000000 	andeq	r0, r0, r0
    3040:	00540044 	subseq	r0, r4, r4, asr #32
    3044:	00000044 	andeq	r0, r0, r4, asr #32
    3048:	0000238d 	andeq	r2, r0, sp, lsl #7
    304c:	00520080 	subseq	r0, r2, r0, lsl #1
    3050:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3054:	00000000 	andeq	r0, r0, r0
    3058:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    3064:	000000e0 	andeq	r0, r0, r0, ror #1
    3068:	00000058 	andeq	r0, r0, r8, asr r0
    306c:	00000000 	andeq	r0, r0, r0
    3070:	00000024 	andeq	r0, r0, r4, lsr #32
    3074:	00000058 	andeq	r0, r0, r8, asr r0
    3078:	00000000 	andeq	r0, r0, r0
    307c:	0000004e 	andeq	r0, r0, lr, asr #32
    3080:	8001111c 	andhi	r1, r1, ip, lsl r1
    3084:	00002395 	muleq	r0, r5, r3
    3088:	00560024 	subseq	r0, r6, r4, lsr #32
    308c:	8001111c 	andhi	r1, r1, ip, lsl r1
    3090:	000022d0 	ldrdeq	r2, [r0], -r0
    3094:	005600a0 	subseq	r0, r6, r0, lsr #1
    3098:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    309c:	00000000 	andeq	r0, r0, r0
    30a0:	0000002e 	andeq	r0, r0, lr, lsr #32
    30a4:	8001111c 	andhi	r1, r1, ip, lsl r1
    30a8:	00000000 	andeq	r0, r0, r0
    30ac:	00560044 	subseq	r0, r6, r4, asr #32
	...
    30b8:	00570044 	subseq	r0, r7, r4, asr #32
    30bc:	0000001c 	andeq	r0, r0, ip, lsl r0
    30c0:	00000000 	andeq	r0, r0, r0
    30c4:	00580044 	subseq	r0, r8, r4, asr #32
    30c8:	00000024 	andeq	r0, r0, r4, lsr #32
    30cc:	00000000 	andeq	r0, r0, r0
    30d0:	00590044 	subseq	r0, r9, r4, asr #32
    30d4:	00000040 	andeq	r0, r0, r0, asr #32
    30d8:	00000000 	andeq	r0, r0, r0
    30dc:	005a0044 	subseq	r0, sl, r4, asr #32
    30e0:	00000044 	andeq	r0, r0, r4, asr #32
    30e4:	0000238d 	andeq	r2, r0, sp, lsl #7
    30e8:	00570080 	subseq	r0, r7, r0, lsl #1
    30ec:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    30f0:	00000000 	andeq	r0, r0, r0
    30f4:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    3100:	000000e0 	andeq	r0, r0, r0, ror #1
    3104:	00000058 	andeq	r0, r0, r8, asr r0
    3108:	00000000 	andeq	r0, r0, r0
    310c:	00000024 	andeq	r0, r0, r4, lsr #32
    3110:	00000058 	andeq	r0, r0, r8, asr r0
    3114:	00000000 	andeq	r0, r0, r0
    3118:	0000004e 	andeq	r0, r0, lr, asr #32
    311c:	80011174 	andhi	r1, r1, r4, ror r1
    3120:	000023aa 	andeq	r2, r0, sl, lsr #7
    3124:	005c0024 	subseq	r0, ip, r4, lsr #32
    3128:	80011174 	andhi	r1, r1, r4, ror r1
    312c:	000022d0 	ldrdeq	r2, [r0], -r0
    3130:	005c00a0 	subseq	r0, ip, r0, lsr #1
    3134:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    3138:	000022dc 	ldrdeq	r2, [r0], -ip
    313c:	005c00a0 	subseq	r0, ip, r0, lsr #1
    3140:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    3144:	000023c5 	andeq	r2, r0, r5, asr #7
    3148:	005c00a0 	subseq	r0, ip, r0, lsr #1
    314c:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    3150:	00000000 	andeq	r0, r0, r0
    3154:	0000002e 	andeq	r0, r0, lr, lsr #32
    3158:	80011174 	andhi	r1, r1, r4, ror r1
    315c:	00000000 	andeq	r0, r0, r0
    3160:	005c0044 	subseq	r0, ip, r4, asr #32
	...
    316c:	005d0044 	subseq	r0, sp, r4, asr #32
    3170:	0000001c 	andeq	r0, r0, ip, lsl r0
    3174:	00000000 	andeq	r0, r0, r0
    3178:	005e0044 	subseq	r0, lr, r4, asr #32
    317c:	00000024 	andeq	r0, r0, r4, lsr #32
    3180:	00000000 	andeq	r0, r0, r0
    3184:	005f0044 	subseq	r0, pc, r4, asr #32
    3188:	00000038 	andeq	r0, r0, r8, lsr r0
    318c:	000023d1 	ldrdeq	r2, [r0], -r1
    3190:	005d0080 	subseq	r0, sp, r0, lsl #1
    3194:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3198:	00000000 	andeq	r0, r0, r0
    319c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    31a8:	000000e0 	andeq	r0, r0, r0, ror #1
    31ac:	00000044 	andeq	r0, r0, r4, asr #32
    31b0:	00000000 	andeq	r0, r0, r0
    31b4:	00000024 	andeq	r0, r0, r4, lsr #32
    31b8:	00000044 	andeq	r0, r0, r4, asr #32
    31bc:	00000000 	andeq	r0, r0, r0
    31c0:	0000004e 	andeq	r0, r0, lr, asr #32
    31c4:	800111b8 			; <UNDEFINED> instruction: 0x800111b8
    31c8:	000023d9 	ldrdeq	r2, [r0], -r9
    31cc:	00610024 	rsbeq	r0, r1, r4, lsr #32
    31d0:	800111b8 			; <UNDEFINED> instruction: 0x800111b8
    31d4:	000022d0 	ldrdeq	r2, [r0], -r0
    31d8:	006100a0 	rsbeq	r0, r1, r0, lsr #1
    31dc:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    31e0:	00000000 	andeq	r0, r0, r0
    31e4:	0000002e 	andeq	r0, r0, lr, lsr #32
    31e8:	800111b8 			; <UNDEFINED> instruction: 0x800111b8
    31ec:	00000000 	andeq	r0, r0, r0
    31f0:	00610044 	rsbeq	r0, r1, r4, asr #32
	...
    31fc:	00620044 	rsbeq	r0, r2, r4, asr #32
    3200:	00000014 	andeq	r0, r0, r4, lsl r0
    3204:	00000000 	andeq	r0, r0, r0
    3208:	00630044 	rsbeq	r0, r3, r4, asr #32
    320c:	00000020 	andeq	r0, r0, r0, lsr #32
    3210:	00000000 	andeq	r0, r0, r0
    3214:	00640044 	rsbeq	r0, r4, r4, asr #32
    3218:	0000002c 	andeq	r0, r0, ip, lsr #32
    321c:	00000000 	andeq	r0, r0, r0
    3220:	00660044 	rsbeq	r0, r6, r4, asr #32
    3224:	00000034 	andeq	r0, r0, r4, lsr r0
    3228:	00000000 	andeq	r0, r0, r0
    322c:	00670044 	rsbeq	r0, r7, r4, asr #32
    3230:	00000038 	andeq	r0, r0, r8, lsr r0
    3234:	000023f4 	strdeq	r2, [r0], -r4
    3238:	00620080 	rsbeq	r0, r2, r0, lsl #1
    323c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3240:	00000000 	andeq	r0, r0, r0
    3244:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    3250:	000000e0 	andeq	r0, r0, r0, ror #1
    3254:	00000044 	andeq	r0, r0, r4, asr #32
    3258:	00000000 	andeq	r0, r0, r0
    325c:	00000024 	andeq	r0, r0, r4, lsr #32
    3260:	00000044 	andeq	r0, r0, r4, asr #32
    3264:	00000000 	andeq	r0, r0, r0
    3268:	0000004e 	andeq	r0, r0, lr, asr #32
    326c:	800111fc 	strdhi	r1, [r1], -ip
    3270:	000023fe 	strdeq	r2, [r0], -lr
    3274:	00690024 	rsbeq	r0, r9, r4, lsr #32
    3278:	800111fc 	strdhi	r1, [r1], -ip
    327c:	000022d0 	ldrdeq	r2, [r0], -r0
    3280:	006900a0 	rsbeq	r0, r9, r0, lsr #1
    3284:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    3288:	000022dc 	ldrdeq	r2, [r0], -ip
    328c:	006900a0 	rsbeq	r0, r9, r0, lsr #1
    3290:	ffffffd4 			; <UNDEFINED> instruction: 0xffffffd4
    3294:	000023c5 	andeq	r2, r0, r5, asr #7
    3298:	006900a0 	rsbeq	r0, r9, r0, lsr #1
    329c:	ffffffd0 			; <UNDEFINED> instruction: 0xffffffd0
    32a0:	00000000 	andeq	r0, r0, r0
    32a4:	0000002e 	andeq	r0, r0, lr, lsr #32
    32a8:	800111fc 	strdhi	r1, [r1], -ip
    32ac:	00000000 	andeq	r0, r0, r0
    32b0:	00690044 	rsbeq	r0, r9, r4, asr #32
	...
    32bc:	006a0044 	rsbeq	r0, sl, r4, asr #32
    32c0:	00000024 	andeq	r0, r0, r4, lsr #32
    32c4:	00000000 	andeq	r0, r0, r0
    32c8:	006b0044 	rsbeq	r0, fp, r4, asr #32
    32cc:	0000002c 	andeq	r0, r0, ip, lsr #32
    32d0:	00000000 	andeq	r0, r0, r0
    32d4:	006c0044 	rsbeq	r0, ip, r4, asr #32
    32d8:	00000034 	andeq	r0, r0, r4, lsr r0
    32dc:	00000000 	andeq	r0, r0, r0
    32e0:	006d0044 	rsbeq	r0, sp, r4, asr #32
    32e4:	00000040 	andeq	r0, r0, r0, asr #32
    32e8:	00000000 	andeq	r0, r0, r0
    32ec:	006f0044 	rsbeq	r0, pc, r4, asr #32
    32f0:	0000004c 	andeq	r0, r0, ip, asr #32
    32f4:	00000000 	andeq	r0, r0, r0
    32f8:	00700044 	rsbseq	r0, r0, r4, asr #32
    32fc:	00000068 	andeq	r0, r0, r8, rrx
    3300:	00000000 	andeq	r0, r0, r0
    3304:	00700044 	rsbseq	r0, r0, r4, asr #32
    3308:	00000074 	andeq	r0, r0, r4, ror r0
    330c:	00000000 	andeq	r0, r0, r0
    3310:	00710044 	rsbseq	r0, r1, r4, asr #32
    3314:	00000080 	andeq	r0, r0, r0, lsl #1
    3318:	00000000 	andeq	r0, r0, r0
    331c:	00730044 	rsbseq	r0, r3, r4, asr #32
    3320:	00000088 	andeq	r0, r0, r8, lsl #1
    3324:	00000000 	andeq	r0, r0, r0
    3328:	00740044 	rsbseq	r0, r4, r4, asr #32
    332c:	00000098 	muleq	r0, r8, r0
    3330:	00000000 	andeq	r0, r0, r0
    3334:	00750044 	rsbseq	r0, r5, r4, asr #32
    3338:	000000a4 	andeq	r0, r0, r4, lsr #1
    333c:	00000000 	andeq	r0, r0, r0
    3340:	00780044 	rsbseq	r0, r8, r4, asr #32
    3344:	000000ac 	andeq	r0, r0, ip, lsr #1
    3348:	00000000 	andeq	r0, r0, r0
    334c:	00780044 	rsbseq	r0, r8, r4, asr #32
    3350:	000000b8 	strheq	r0, [r0], -r8
    3354:	00000000 	andeq	r0, r0, r0
    3358:	00790044 	rsbseq	r0, r9, r4, asr #32
    335c:	000000c8 	andeq	r0, r0, r8, asr #1
    3360:	00000000 	andeq	r0, r0, r0
    3364:	007c0044 	rsbseq	r0, ip, r4, asr #32
    3368:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    336c:	00000000 	andeq	r0, r0, r0
    3370:	007d0044 	rsbseq	r0, sp, r4, asr #32
    3374:	000000f4 	strdeq	r0, [r0], -r4
    3378:	00000000 	andeq	r0, r0, r0
    337c:	007e0044 	rsbseq	r0, lr, r4, asr #32
    3380:	00000100 	andeq	r0, r0, r0, lsl #2
    3384:	00000000 	andeq	r0, r0, r0
    3388:	007f0044 	rsbseq	r0, pc, r4, asr #32
    338c:	00000108 	andeq	r0, r0, r8, lsl #2
    3390:	00000000 	andeq	r0, r0, r0
    3394:	00800044 	addeq	r0, r0, r4, asr #32
    3398:	00000128 	andeq	r0, r0, r8, lsr #2
    339c:	00000000 	andeq	r0, r0, r0
    33a0:	00810044 	addeq	r0, r1, r4, asr #32
    33a4:	00000134 	andeq	r0, r0, r4, lsr r1
    33a8:	00000000 	andeq	r0, r0, r0
    33ac:	00820044 	addeq	r0, r2, r4, asr #32
    33b0:	00000138 	andeq	r0, r0, r8, lsr r1
    33b4:	00002418 	andeq	r2, r0, r8, lsl r4
    33b8:	006a0080 	rsbeq	r0, sl, r0, lsl #1
    33bc:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    33c0:	00002425 	andeq	r2, r0, r5, lsr #8
    33c4:	006b0080 	rsbeq	r0, fp, r0, lsl #1
    33c8:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    33cc:	00002431 	andeq	r2, r0, r1, lsr r4
    33d0:	006c0080 	rsbeq	r0, ip, r0, lsl #1
    33d4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    33d8:	000022f3 	strdeq	r2, [r0], -r3
    33dc:	006f0080 	rsbeq	r0, pc, r0, lsl #1
    33e0:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    33e4:	0000243e 	andeq	r2, r0, lr, lsr r4
    33e8:	00730080 	rsbseq	r0, r3, r0, lsl #1
    33ec:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    33f0:	0000244d 	andeq	r2, r0, sp, asr #8
    33f4:	007c0080 	rsbseq	r0, ip, r0, lsl #1
    33f8:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    33fc:	00000000 	andeq	r0, r0, r0
    3400:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    340c:	000000e0 	andeq	r0, r0, r0, ror #1
    3410:	00000150 	andeq	r0, r0, r0, asr r1
    3414:	00000000 	andeq	r0, r0, r0
    3418:	00000024 	andeq	r0, r0, r4, lsr #32
    341c:	00000150 	andeq	r0, r0, r0, asr r1
    3420:	00000000 	andeq	r0, r0, r0
    3424:	0000004e 	andeq	r0, r0, lr, asr #32
    3428:	8001134c 	andhi	r1, r1, ip, asr #6
    342c:	00002457 	andeq	r2, r0, r7, asr r4
    3430:	00840024 	addeq	r0, r4, r4, lsr #32
    3434:	8001134c 	andhi	r1, r1, ip, asr #6
    3438:	00000000 	andeq	r0, r0, r0
    343c:	0000002e 	andeq	r0, r0, lr, lsr #32
    3440:	8001134c 	andhi	r1, r1, ip, asr #6
    3444:	00000000 	andeq	r0, r0, r0
    3448:	00840044 	addeq	r0, r4, r4, asr #32
	...
    3454:	00850044 	addeq	r0, r5, r4, asr #32
    3458:	00000018 	andeq	r0, r0, r8, lsl r0
    345c:	00000000 	andeq	r0, r0, r0
    3460:	00860044 	addeq	r0, r6, r4, asr #32
    3464:	00000028 	andeq	r0, r0, r8, lsr #32
    3468:	00000000 	andeq	r0, r0, r0
    346c:	00870044 	addeq	r0, r7, r4, asr #32
    3470:	00000048 	andeq	r0, r0, r8, asr #32
    3474:	00000000 	andeq	r0, r0, r0
    3478:	00880044 	addeq	r0, r8, r4, asr #32
    347c:	00000054 	andeq	r0, r0, r4, asr r0
    3480:	00000000 	andeq	r0, r0, r0
    3484:	008a0044 	addeq	r0, sl, r4, asr #32
    3488:	0000005c 	andeq	r0, r0, ip, asr r0
    348c:	00000000 	andeq	r0, r0, r0
    3490:	008b0044 	addeq	r0, fp, r4, asr #32
    3494:	00000064 	andeq	r0, r0, r4, rrx
    3498:	00000000 	andeq	r0, r0, r0
    349c:	008c0044 	addeq	r0, ip, r4, asr #32
    34a0:	00000068 	andeq	r0, r0, r8, rrx
    34a4:	00000000 	andeq	r0, r0, r0
    34a8:	008d0044 	addeq	r0, sp, r4, asr #32
    34ac:	00000070 	andeq	r0, r0, r0, ror r0
    34b0:	00000000 	andeq	r0, r0, r0
    34b4:	008e0044 	addeq	r0, lr, r4, asr #32
    34b8:	00000084 	andeq	r0, r0, r4, lsl #1
    34bc:	00000000 	andeq	r0, r0, r0
    34c0:	008f0044 	addeq	r0, pc, r4, asr #32
    34c4:	00000088 	andeq	r0, r0, r8, lsl #1
    34c8:	00000000 	andeq	r0, r0, r0
    34cc:	00900044 	addseq	r0, r0, r4, asr #32
    34d0:	0000009c 	muleq	r0, ip, r0
    34d4:	00000000 	andeq	r0, r0, r0
    34d8:	00910044 	addseq	r0, r1, r4, asr #32
    34dc:	000000a8 	andeq	r0, r0, r8, lsr #1
    34e0:	00000000 	andeq	r0, r0, r0
    34e4:	00930044 	addseq	r0, r3, r4, asr #32
    34e8:	000000b4 	strheq	r0, [r0], -r4
    34ec:	00000000 	andeq	r0, r0, r0
    34f0:	00940044 	addseq	r0, r4, r4, asr #32
    34f4:	000000c0 	andeq	r0, r0, r0, asr #1
    34f8:	00000000 	andeq	r0, r0, r0
    34fc:	00950044 	addseq	r0, r5, r4, asr #32
    3500:	000000d4 	ldrdeq	r0, [r0], -r4
    3504:	00000000 	andeq	r0, r0, r0
    3508:	00970044 	addseq	r0, r7, r4, asr #32
    350c:	000000dc 	ldrdeq	r0, [r0], -ip
    3510:	00000000 	andeq	r0, r0, r0
    3514:	008e0044 	addeq	r0, lr, r4, asr #32
    3518:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    351c:	00000000 	andeq	r0, r0, r0
    3520:	00990044 	addseq	r0, r9, r4, asr #32
    3524:	000000fc 	strdeq	r0, [r0], -ip
    3528:	00000000 	andeq	r0, r0, r0
    352c:	009a0044 	addseq	r0, sl, r4, asr #32
    3530:	00000108 	andeq	r0, r0, r8, lsl #2
    3534:	00000000 	andeq	r0, r0, r0
    3538:	009b0044 	addseq	r0, fp, r4, asr #32
    353c:	00000128 	andeq	r0, r0, r8, lsr #2
    3540:	00000000 	andeq	r0, r0, r0
    3544:	008b0044 	addeq	r0, fp, r4, asr #32
    3548:	00000130 	andeq	r0, r0, r0, lsr r1
    354c:	00000000 	andeq	r0, r0, r0
    3550:	009d0044 	addseq	r0, sp, r4, asr #32
    3554:	0000013c 	andeq	r0, r0, ip, lsr r1
    3558:	00000000 	andeq	r0, r0, r0
    355c:	009e0044 	addseq	r0, lr, r4, asr #32
    3560:	00000150 	andeq	r0, r0, r0, asr r1
    3564:	00000000 	andeq	r0, r0, r0
    3568:	009f0044 	addseq	r0, pc, r4, asr #32
    356c:	00000154 	andeq	r0, r0, r4, asr r1
    3570:	00002472 	andeq	r2, r0, r2, ror r4
    3574:	00850080 	addeq	r0, r5, r0, lsl #1
    3578:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    357c:	0000244d 	andeq	r2, r0, sp, asr #8
    3580:	00860080 	addeq	r0, r6, r0, lsl #1
    3584:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    3588:	00000ecb 	andeq	r0, r0, fp, asr #29
    358c:	008a0080 	addeq	r0, sl, r0, lsl #1
    3590:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    3594:	00000000 	andeq	r0, r0, r0
    3598:	000000c0 	andeq	r0, r0, r0, asr #1
    359c:	00000000 	andeq	r0, r0, r0
    35a0:	0000247b 	andeq	r2, r0, fp, ror r4
    35a4:	008c0080 	addeq	r0, ip, r0, lsl #1
    35a8:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    35ac:	00002483 	andeq	r2, r0, r3, lsl #9
    35b0:	008d0080 	addeq	r0, sp, r0, lsl #1
    35b4:	ffffffe7 			; <UNDEFINED> instruction: 0xffffffe7
    35b8:	00000000 	andeq	r0, r0, r0
    35bc:	000000c0 	andeq	r0, r0, r0, asr #1
    35c0:	00000068 	andeq	r0, r0, r8, rrx
    35c4:	00000000 	andeq	r0, r0, r0
    35c8:	000000e0 	andeq	r0, r0, r0, ror #1
    35cc:	00000130 	andeq	r0, r0, r0, lsr r1
    35d0:	00000000 	andeq	r0, r0, r0
    35d4:	000000e0 	andeq	r0, r0, r0, ror #1
    35d8:	0000016c 	andeq	r0, r0, ip, ror #2
    35dc:	00000000 	andeq	r0, r0, r0
    35e0:	00000024 	andeq	r0, r0, r4, lsr #32
    35e4:	0000016c 	andeq	r0, r0, ip, ror #2
    35e8:	00000000 	andeq	r0, r0, r0
    35ec:	0000004e 	andeq	r0, r0, lr, asr #32
    35f0:	800114b8 			; <UNDEFINED> instruction: 0x800114b8
    35f4:	0000248b 	andeq	r2, r0, fp, lsl #9
    35f8:	00a10024 	adceq	r0, r1, r4, lsr #32
    35fc:	800114b8 			; <UNDEFINED> instruction: 0x800114b8
    3600:	000022d0 	ldrdeq	r2, [r0], -r0
    3604:	00a100a0 	adceq	r0, r1, r0, lsr #1
    3608:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    360c:	000022dc 	ldrdeq	r2, [r0], -ip
    3610:	00a100a0 	adceq	r0, r1, r0, lsr #1
    3614:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    3618:	00000000 	andeq	r0, r0, r0
    361c:	0000002e 	andeq	r0, r0, lr, lsr #32
    3620:	800114b8 			; <UNDEFINED> instruction: 0x800114b8
    3624:	00000000 	andeq	r0, r0, r0
    3628:	00a10044 	adceq	r0, r1, r4, asr #32
	...
    3634:	00a20044 	adceq	r0, r2, r4, asr #32
    3638:	00000020 	andeq	r0, r0, r0, lsr #32
    363c:	00000000 	andeq	r0, r0, r0
    3640:	00a30044 	adceq	r0, r3, r4, asr #32
    3644:	00000028 	andeq	r0, r0, r8, lsr #32
    3648:	00000000 	andeq	r0, r0, r0
    364c:	00a40044 	adceq	r0, r4, r4, asr #32
    3650:	00000030 	andeq	r0, r0, r0, lsr r0
    3654:	00000000 	andeq	r0, r0, r0
    3658:	00a60044 	adceq	r0, r6, r4, asr #32
    365c:	00000038 	andeq	r0, r0, r8, lsr r0
    3660:	00000000 	andeq	r0, r0, r0
    3664:	00a70044 	adceq	r0, r7, r4, asr #32
    3668:	00000058 	andeq	r0, r0, r8, asr r0
    366c:	00000000 	andeq	r0, r0, r0
    3670:	00a80044 	adceq	r0, r8, r4, asr #32
    3674:	00000064 	andeq	r0, r0, r4, rrx
    3678:	00000000 	andeq	r0, r0, r0
    367c:	00aa0044 	adceq	r0, sl, r4, asr #32
    3680:	0000006c 	andeq	r0, r0, ip, rrx
    3684:	00000000 	andeq	r0, r0, r0
    3688:	00ab0044 	adceq	r0, fp, r4, asr #32
    368c:	00000078 	andeq	r0, r0, r8, ror r0
    3690:	00000000 	andeq	r0, r0, r0
    3694:	00ad0044 	adceq	r0, sp, r4, asr #32
    3698:	00000088 	andeq	r0, r0, r8, lsl #1
    369c:	00000000 	andeq	r0, r0, r0
    36a0:	00ae0044 	adceq	r0, lr, r4, asr #32
    36a4:	0000008c 	andeq	r0, r0, ip, lsl #1
    36a8:	00002418 	andeq	r2, r0, r8, lsl r4
    36ac:	00a20080 	adceq	r0, r2, r0, lsl #1
    36b0:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    36b4:	000024a5 	andeq	r2, r0, r5, lsr #9
    36b8:	00a30080 	adceq	r0, r3, r0, lsl #1
    36bc:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    36c0:	00002425 	andeq	r2, r0, r5, lsr #8
    36c4:	00a40080 	adceq	r0, r4, r0, lsl #1
    36c8:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    36cc:	000022f3 	strdeq	r2, [r0], -r3
    36d0:	00a60080 	adceq	r0, r6, r0, lsl #1
    36d4:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    36d8:	00000000 	andeq	r0, r0, r0
    36dc:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    36e8:	000000e0 	andeq	r0, r0, r0, ror #1
    36ec:	000000a0 	andeq	r0, r0, r0, lsr #1
    36f0:	00000000 	andeq	r0, r0, r0
    36f4:	00000024 	andeq	r0, r0, r4, lsr #32
    36f8:	000000a0 	andeq	r0, r0, r0, lsr #1
    36fc:	00000000 	andeq	r0, r0, r0
    3700:	0000004e 	andeq	r0, r0, lr, asr #32
    3704:	80011558 	andhi	r1, r1, r8, asr r5
    3708:	000024b9 			; <UNDEFINED> instruction: 0x000024b9
    370c:	00b00024 	adcseq	r0, r0, r4, lsr #32
    3710:	80011558 	andhi	r1, r1, r8, asr r5
    3714:	000022d0 	ldrdeq	r2, [r0], -r0
    3718:	00b000a0 	adcseq	r0, r0, r0, lsr #1
    371c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3720:	00000000 	andeq	r0, r0, r0
    3724:	0000002e 	andeq	r0, r0, lr, lsr #32
    3728:	80011558 	andhi	r1, r1, r8, asr r5
    372c:	00000000 	andeq	r0, r0, r0
    3730:	00b00044 	adcseq	r0, r0, r4, asr #32
	...
    373c:	00b10044 	adcseq	r0, r1, r4, asr #32
    3740:	00000014 	andeq	r0, r0, r4, lsl r0
    3744:	00000000 	andeq	r0, r0, r0
    3748:	00b20044 	adcseq	r0, r2, r4, asr #32
    374c:	00000018 	andeq	r0, r0, r8, lsl r0
    3750:	00000000 	andeq	r0, r0, r0
    3754:	00000024 	andeq	r0, r0, r4, lsr #32
    3758:	00000024 	andeq	r0, r0, r4, lsr #32
    375c:	00000000 	andeq	r0, r0, r0
    3760:	0000004e 	andeq	r0, r0, lr, asr #32
    3764:	8001157c 	andhi	r1, r1, ip, ror r5
    3768:	000024cc 	andeq	r2, r0, ip, asr #9
    376c:	00b40024 	adcseq	r0, r4, r4, lsr #32
    3770:	8001157c 	andhi	r1, r1, ip, ror r5
    3774:	000022d0 	ldrdeq	r2, [r0], -r0
    3778:	00b400a0 	adcseq	r0, r4, r0, lsr #1
    377c:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    3780:	000022dc 	ldrdeq	r2, [r0], -ip
    3784:	00b400a0 	adcseq	r0, r4, r0, lsr #1
    3788:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    378c:	000023c5 	andeq	r2, r0, r5, asr #7
    3790:	00b400a0 	adcseq	r0, r4, r0, lsr #1
    3794:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    3798:	00000000 	andeq	r0, r0, r0
    379c:	0000002e 	andeq	r0, r0, lr, lsr #32
    37a0:	8001157c 	andhi	r1, r1, ip, ror r5
    37a4:	00000000 	andeq	r0, r0, r0
    37a8:	00b40044 	adcseq	r0, r4, r4, asr #32
	...
    37b4:	00b50044 	adcseq	r0, r5, r4, asr #32
    37b8:	0000001c 	andeq	r0, r0, ip, lsl r0
    37bc:	00000000 	andeq	r0, r0, r0
    37c0:	00b60044 	adcseq	r0, r6, r4, asr #32
    37c4:	00000028 	andeq	r0, r0, r8, lsr #32
    37c8:	00000000 	andeq	r0, r0, r0
    37cc:	00b70044 	adcseq	r0, r7, r4, asr #32
    37d0:	00000034 	andeq	r0, r0, r4, lsr r0
    37d4:	00000000 	andeq	r0, r0, r0
    37d8:	00b90044 	adcseq	r0, r9, r4, asr #32
    37dc:	0000003c 	andeq	r0, r0, ip, lsr r0
    37e0:	00000000 	andeq	r0, r0, r0
    37e4:	00ba0044 	adcseq	r0, sl, r4, asr #32
    37e8:	0000004c 	andeq	r0, r0, ip, asr #32
    37ec:	00000000 	andeq	r0, r0, r0
    37f0:	00bb0044 	adcseq	r0, fp, r4, asr #32
    37f4:	00000058 	andeq	r0, r0, r8, asr r0
    37f8:	00000000 	andeq	r0, r0, r0
    37fc:	00bd0044 	adcseq	r0, sp, r4, asr #32
    3800:	00000060 	andeq	r0, r0, r0, rrx
    3804:	00000000 	andeq	r0, r0, r0
    3808:	00c00044 	sbceq	r0, r0, r4, asr #32
    380c:	00000070 	andeq	r0, r0, r0, ror r0
    3810:	00000000 	andeq	r0, r0, r0
    3814:	00c10044 	sbceq	r0, r1, r4, asr #32
    3818:	0000007c 	andeq	r0, r0, ip, ror r0
    381c:	00000000 	andeq	r0, r0, r0
    3820:	00c20044 	sbceq	r0, r2, r4, asr #32
    3824:	000000a0 	andeq	r0, r0, r0, lsr #1
    3828:	00000000 	andeq	r0, r0, r0
    382c:	00c30044 	sbceq	r0, r3, r4, asr #32
    3830:	000000c0 	andeq	r0, r0, r0, asr #1
    3834:	00000000 	andeq	r0, r0, r0
    3838:	00c40044 	sbceq	r0, r4, r4, asr #32
    383c:	000000e0 	andeq	r0, r0, r0, ror #1
    3840:	00000000 	andeq	r0, r0, r0
    3844:	00c50044 	sbceq	r0, r5, r4, asr #32
    3848:	00000100 	andeq	r0, r0, r0, lsl #2
    384c:	00000000 	andeq	r0, r0, r0
    3850:	00c00044 	sbceq	r0, r0, r4, asr #32
    3854:	00000108 	andeq	r0, r0, r8, lsl #2
    3858:	00000000 	andeq	r0, r0, r0
    385c:	00c00044 	sbceq	r0, r0, r4, asr #32
    3860:	00000114 	andeq	r0, r0, r4, lsl r1
    3864:	00000000 	andeq	r0, r0, r0
    3868:	00c90044 	sbceq	r0, r9, r4, asr #32
    386c:	00000120 	andeq	r0, r0, r0, lsr #2
    3870:	00000000 	andeq	r0, r0, r0
    3874:	00ca0044 	sbceq	r0, sl, r4, asr #32
    3878:	00000130 	andeq	r0, r0, r0, lsr r1
    387c:	00000000 	andeq	r0, r0, r0
    3880:	00cb0044 	sbceq	r0, fp, r4, asr #32
    3884:	00000134 	andeq	r0, r0, r4, lsr r1
    3888:	000024e2 	andeq	r2, r0, r2, ror #9
    388c:	00b50080 	adcseq	r0, r5, r0, lsl #1
    3890:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    3894:	000024ee 	andeq	r2, r0, lr, ror #9
    3898:	00b90080 	adcseq	r0, r9, r0, lsl #1
    389c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    38a0:	00000ecb 	andeq	r0, r0, fp, asr #29
    38a4:	00bf0080 	adcseq	r0, pc, r0, lsl #1
    38a8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    38ac:	00000000 	andeq	r0, r0, r0
    38b0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    38bc:	000000e0 	andeq	r0, r0, r0, ror #1
    38c0:	00000140 	andeq	r0, r0, r0, asr #2
    38c4:	00000000 	andeq	r0, r0, r0
    38c8:	00000024 	andeq	r0, r0, r4, lsr #32
    38cc:	00000140 	andeq	r0, r0, r0, asr #2
    38d0:	00000000 	andeq	r0, r0, r0
    38d4:	0000004e 	andeq	r0, r0, lr, asr #32
    38d8:	800116bc 			; <UNDEFINED> instruction: 0x800116bc
    38dc:	000024f7 	strdeq	r2, [r0], -r7
    38e0:	00cd0024 	sbceq	r0, sp, r4, lsr #32
    38e4:	800116bc 			; <UNDEFINED> instruction: 0x800116bc
    38e8:	000022d0 	ldrdeq	r2, [r0], -r0
    38ec:	00cd00a0 	sbceq	r0, sp, r0, lsr #1
    38f0:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    38f4:	000022dc 	ldrdeq	r2, [r0], -ip
    38f8:	00cd00a0 	sbceq	r0, sp, r0, lsr #1
    38fc:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    3900:	00000000 	andeq	r0, r0, r0
    3904:	0000002e 	andeq	r0, r0, lr, lsr #32
    3908:	800116bc 			; <UNDEFINED> instruction: 0x800116bc
    390c:	00000000 	andeq	r0, r0, r0
    3910:	00cd0044 	sbceq	r0, sp, r4, asr #32
	...
    391c:	00ce0044 	sbceq	r0, lr, r4, asr #32
    3920:	00000018 	andeq	r0, r0, r8, lsl r0
    3924:	00000000 	andeq	r0, r0, r0
    3928:	00cf0044 	sbceq	r0, pc, r4, asr #32
    392c:	00000024 	andeq	r0, r0, r4, lsr #32
    3930:	00000000 	andeq	r0, r0, r0
    3934:	00d00044 	sbcseq	r0, r0, r4, asr #32
    3938:	00000030 	andeq	r0, r0, r0, lsr r0
    393c:	00000000 	andeq	r0, r0, r0
    3940:	00d20044 	sbcseq	r0, r2, r4, asr #32
    3944:	00000038 	andeq	r0, r0, r8, lsr r0
    3948:	00000000 	andeq	r0, r0, r0
    394c:	00d30044 	sbcseq	r0, r3, r4, asr #32
    3950:	00000040 	andeq	r0, r0, r0, asr #32
    3954:	00000000 	andeq	r0, r0, r0
    3958:	00d30044 	sbcseq	r0, r3, r4, asr #32
    395c:	0000004c 	andeq	r0, r0, ip, asr #32
    3960:	00000000 	andeq	r0, r0, r0
    3964:	00d40044 	sbcseq	r0, r4, r4, asr #32
    3968:	00000058 	andeq	r0, r0, r8, asr r0
    396c:	00000000 	andeq	r0, r0, r0
    3970:	00d60044 	sbcseq	r0, r6, r4, asr #32
    3974:	00000060 	andeq	r0, r0, r0, rrx
    3978:	00000000 	andeq	r0, r0, r0
    397c:	00d80044 	sbcseq	r0, r8, r4, asr #32
    3980:	000000a0 	andeq	r0, r0, r0, lsr #1
    3984:	00000000 	andeq	r0, r0, r0
    3988:	00d90044 	sbcseq	r0, r9, r4, asr #32
    398c:	000000c0 	andeq	r0, r0, r0, asr #1
    3990:	00000000 	andeq	r0, r0, r0
    3994:	00da0044 	sbcseq	r0, sl, r4, asr #32
    3998:	000000e0 	andeq	r0, r0, r0, ror #1
    399c:	00000000 	andeq	r0, r0, r0
    39a0:	00db0044 	sbcseq	r0, fp, r4, asr #32
    39a4:	00000100 	andeq	r0, r0, r0, lsl #2
    39a8:	00000000 	andeq	r0, r0, r0
    39ac:	00dc0044 	sbcseq	r0, ip, r4, asr #32
    39b0:	00000104 	andeq	r0, r0, r4, lsl #2
    39b4:	000024e2 	andeq	r2, r0, r2, ror #9
    39b8:	00ce0080 	sbceq	r0, lr, r0, lsl #1
    39bc:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    39c0:	0000250e 	andeq	r2, r0, lr, lsl #10
    39c4:	00d20080 	sbcseq	r0, r2, r0, lsl #1
    39c8:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    39cc:	00000000 	andeq	r0, r0, r0
    39d0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    39dc:	000000e0 	andeq	r0, r0, r0, ror #1
    39e0:	00000110 	andeq	r0, r0, r0, lsl r1
    39e4:	00000000 	andeq	r0, r0, r0
    39e8:	00000024 	andeq	r0, r0, r4, lsr #32
    39ec:	00000110 	andeq	r0, r0, r0, lsl r1
    39f0:	00000000 	andeq	r0, r0, r0
    39f4:	0000004e 	andeq	r0, r0, lr, asr #32
    39f8:	800117cc 	andhi	r1, r1, ip, asr #15
    39fc:	00002517 	andeq	r2, r0, r7, lsl r5
    3a00:	00de0024 	sbcseq	r0, lr, r4, lsr #32
    3a04:	800117cc 	andhi	r1, r1, ip, asr #15
    3a08:	000022d0 	ldrdeq	r2, [r0], -r0
    3a0c:	00de00a0 	sbcseq	r0, lr, r0, lsr #1
    3a10:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    3a14:	000022dc 	ldrdeq	r2, [r0], -ip
    3a18:	00de00a0 	sbcseq	r0, lr, r0, lsr #1
    3a1c:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    3a20:	00000000 	andeq	r0, r0, r0
    3a24:	0000002e 	andeq	r0, r0, lr, lsr #32
    3a28:	800117cc 	andhi	r1, r1, ip, asr #15
    3a2c:	00000000 	andeq	r0, r0, r0
    3a30:	00de0044 	sbcseq	r0, lr, r4, asr #32
	...
    3a3c:	00df0044 	sbcseq	r0, pc, r4, asr #32
    3a40:	00000018 	andeq	r0, r0, r8, lsl r0
    3a44:	00000000 	andeq	r0, r0, r0
    3a48:	00e00044 	rsceq	r0, r0, r4, asr #32
    3a4c:	00000024 	andeq	r0, r0, r4, lsr #32
    3a50:	00000000 	andeq	r0, r0, r0
    3a54:	00e10044 	rsceq	r0, r1, r4, asr #32
    3a58:	00000030 	andeq	r0, r0, r0, lsr r0
    3a5c:	00000000 	andeq	r0, r0, r0
    3a60:	00e20044 	rsceq	r0, r2, r4, asr #32
    3a64:	00000038 	andeq	r0, r0, r8, lsr r0
    3a68:	00000000 	andeq	r0, r0, r0
    3a6c:	00e30044 	rsceq	r0, r3, r4, asr #32
    3a70:	00000040 	andeq	r0, r0, r0, asr #32
    3a74:	00000000 	andeq	r0, r0, r0
    3a78:	00e30044 	rsceq	r0, r3, r4, asr #32
    3a7c:	0000004c 	andeq	r0, r0, ip, asr #32
    3a80:	00000000 	andeq	r0, r0, r0
    3a84:	00e40044 	rsceq	r0, r4, r4, asr #32
    3a88:	00000058 	andeq	r0, r0, r8, asr r0
    3a8c:	00000000 	andeq	r0, r0, r0
    3a90:	00e50044 	rsceq	r0, r5, r4, asr #32
    3a94:	00000060 	andeq	r0, r0, r0, rrx
    3a98:	00000000 	andeq	r0, r0, r0
    3a9c:	00e60044 	rsceq	r0, r6, r4, asr #32
    3aa0:	00000080 	andeq	r0, r0, r0, lsl #1
    3aa4:	00000000 	andeq	r0, r0, r0
    3aa8:	00e70044 	rsceq	r0, r7, r4, asr #32
    3aac:	0000008c 	andeq	r0, r0, ip, lsl #1
    3ab0:	00000000 	andeq	r0, r0, r0
    3ab4:	00e80044 	rsceq	r0, r8, r4, asr #32
    3ab8:	00000094 	muleq	r0, r4, r0
    3abc:	00000000 	andeq	r0, r0, r0
    3ac0:	00e90044 	rsceq	r0, r9, r4, asr #32
    3ac4:	0000009c 	muleq	r0, ip, r0
    3ac8:	000024e2 	andeq	r2, r0, r2, ror #9
    3acc:	00df0080 	sbcseq	r0, pc, r0, lsl #1
    3ad0:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3ad4:	0000250e 	andeq	r2, r0, lr, lsl #10
    3ad8:	00e20080 	rsceq	r0, r2, r0, lsl #1
    3adc:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    3ae0:	000024ee 	andeq	r2, r0, lr, ror #9
    3ae4:	00e50080 	rsceq	r0, r5, r0, lsl #1
    3ae8:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    3aec:	00000000 	andeq	r0, r0, r0
    3af0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    3afc:	000000e0 	andeq	r0, r0, r0, ror #1
    3b00:	000000a8 	andeq	r0, r0, r8, lsr #1
    3b04:	00000000 	andeq	r0, r0, r0
    3b08:	00000024 	andeq	r0, r0, r4, lsr #32
    3b0c:	000000a8 	andeq	r0, r0, r8, lsr #1
    3b10:	00000000 	andeq	r0, r0, r0
    3b14:	0000004e 	andeq	r0, r0, lr, asr #32
    3b18:	80011874 	andhi	r1, r1, r4, ror r8
    3b1c:	0000252d 	andeq	r2, r0, sp, lsr #10
    3b20:	00eb0024 	rsceq	r0, fp, r4, lsr #32
    3b24:	80011874 	andhi	r1, r1, r4, ror r8
    3b28:	000022d0 	ldrdeq	r2, [r0], -r0
    3b2c:	00eb00a0 	rsceq	r0, fp, r0, lsr #1
    3b30:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    3b34:	000022dc 	ldrdeq	r2, [r0], -ip
    3b38:	00eb00a0 	rsceq	r0, fp, r0, lsr #1
    3b3c:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    3b40:	000023c5 	andeq	r2, r0, r5, asr #7
    3b44:	00eb00a0 	rsceq	r0, fp, r0, lsr #1
    3b48:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    3b4c:	00000000 	andeq	r0, r0, r0
    3b50:	0000002e 	andeq	r0, r0, lr, lsr #32
    3b54:	80011874 	andhi	r1, r1, r4, ror r8
    3b58:	00000000 	andeq	r0, r0, r0
    3b5c:	00eb0044 	rsceq	r0, fp, r4, asr #32
	...
    3b68:	00ec0044 	rsceq	r0, ip, r4, asr #32
    3b6c:	0000001c 	andeq	r0, r0, ip, lsl r0
    3b70:	00000000 	andeq	r0, r0, r0
    3b74:	00ed0044 	rsceq	r0, sp, r4, asr #32
    3b78:	00000028 	andeq	r0, r0, r8, lsr #32
    3b7c:	00000000 	andeq	r0, r0, r0
    3b80:	00ee0044 	rsceq	r0, lr, r4, asr #32
    3b84:	00000034 	andeq	r0, r0, r4, lsr r0
    3b88:	00000000 	andeq	r0, r0, r0
    3b8c:	00f00044 	rscseq	r0, r0, r4, asr #32
    3b90:	0000003c 	andeq	r0, r0, ip, lsr r0
    3b94:	00000000 	andeq	r0, r0, r0
    3b98:	00f10044 	rscseq	r0, r1, r4, asr #32
    3b9c:	00000044 	andeq	r0, r0, r4, asr #32
    3ba0:	00000000 	andeq	r0, r0, r0
    3ba4:	00f10044 	rscseq	r0, r1, r4, asr #32
    3ba8:	00000050 	andeq	r0, r0, r0, asr r0
    3bac:	00000000 	andeq	r0, r0, r0
    3bb0:	00f20044 	rscseq	r0, r2, r4, asr #32
    3bb4:	0000005c 	andeq	r0, r0, ip, asr r0
    3bb8:	00000000 	andeq	r0, r0, r0
    3bbc:	00f40044 	rscseq	r0, r4, r4, asr #32
    3bc0:	00000064 	andeq	r0, r0, r4, rrx
    3bc4:	00000000 	andeq	r0, r0, r0
    3bc8:	00f50044 	rscseq	r0, r5, r4, asr #32
    3bcc:	00000084 	andeq	r0, r0, r4, lsl #1
    3bd0:	00000000 	andeq	r0, r0, r0
    3bd4:	00f50044 	rscseq	r0, r5, r4, asr #32
    3bd8:	00000090 	muleq	r0, r0, r0
    3bdc:	00000000 	andeq	r0, r0, r0
    3be0:	00f60044 	rscseq	r0, r6, r4, asr #32
    3be4:	000000a0 	andeq	r0, r0, r0, lsr #1
    3be8:	00000000 	andeq	r0, r0, r0
    3bec:	00f70044 	rscseq	r0, r7, r4, asr #32
    3bf0:	000000a8 	andeq	r0, r0, r8, lsr #1
    3bf4:	00000000 	andeq	r0, r0, r0
    3bf8:	00f80044 	rscseq	r0, r8, r4, asr #32
    3bfc:	000000c8 	andeq	r0, r0, r8, asr #1
    3c00:	00000000 	andeq	r0, r0, r0
    3c04:	00f90044 	rscseq	r0, r9, r4, asr #32
    3c08:	000000cc 	andeq	r0, r0, ip, asr #1
    3c0c:	000024e2 	andeq	r2, r0, r2, ror #9
    3c10:	00ec0080 	rsceq	r0, ip, r0, lsl #1
    3c14:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3c18:	0000250e 	andeq	r2, r0, lr, lsl #10
    3c1c:	00f00080 	rscseq	r0, r0, r0, lsl #1
    3c20:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    3c24:	000024ee 	andeq	r2, r0, lr, ror #9
    3c28:	00f40080 	rscseq	r0, r4, r0, lsl #1
    3c2c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    3c30:	00000000 	andeq	r0, r0, r0
    3c34:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    3c40:	000000e0 	andeq	r0, r0, r0, ror #1
    3c44:	000000d8 	ldrdeq	r0, [r0], -r8
    3c48:	00000000 	andeq	r0, r0, r0
    3c4c:	00000024 	andeq	r0, r0, r4, lsr #32
    3c50:	000000d8 	ldrdeq	r0, [r0], -r8
    3c54:	00000000 	andeq	r0, r0, r0
    3c58:	0000004e 	andeq	r0, r0, lr, asr #32
    3c5c:	8001194c 	andhi	r1, r1, ip, asr #18
    3c60:	00002543 	andeq	r2, r0, r3, asr #10
    3c64:	00fb0024 	rscseq	r0, fp, r4, lsr #32
    3c68:	8001194c 	andhi	r1, r1, ip, asr #18
    3c6c:	000022d0 	ldrdeq	r2, [r0], -r0
    3c70:	00fb00a0 	rscseq	r0, fp, r0, lsr #1
    3c74:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    3c78:	000022dc 	ldrdeq	r2, [r0], -ip
    3c7c:	00fb00a0 	rscseq	r0, fp, r0, lsr #1
    3c80:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    3c84:	00000000 	andeq	r0, r0, r0
    3c88:	0000002e 	andeq	r0, r0, lr, lsr #32
    3c8c:	8001194c 	andhi	r1, r1, ip, asr #18
    3c90:	00000000 	andeq	r0, r0, r0
    3c94:	00fb0044 	rscseq	r0, fp, r4, asr #32
	...
    3ca0:	00fc0044 	rscseq	r0, ip, r4, asr #32
    3ca4:	00000018 	andeq	r0, r0, r8, lsl r0
    3ca8:	00000000 	andeq	r0, r0, r0
    3cac:	00fd0044 	rscseq	r0, sp, r4, asr #32
    3cb0:	00000024 	andeq	r0, r0, r4, lsr #32
    3cb4:	00000000 	andeq	r0, r0, r0
    3cb8:	00fe0044 	rscseq	r0, lr, r4, asr #32
    3cbc:	00000030 	andeq	r0, r0, r0, lsr r0
    3cc0:	00000000 	andeq	r0, r0, r0
    3cc4:	01000044 	tsteq	r0, r4, asr #32
    3cc8:	00000038 	andeq	r0, r0, r8, lsr r0
    3ccc:	00000000 	andeq	r0, r0, r0
    3cd0:	01010044 	tsteq	r1, r4, asr #32
    3cd4:	00000040 	andeq	r0, r0, r0, asr #32
    3cd8:	00000000 	andeq	r0, r0, r0
    3cdc:	01010044 	tsteq	r1, r4, asr #32
    3ce0:	0000004c 	andeq	r0, r0, ip, asr #32
    3ce4:	00000000 	andeq	r0, r0, r0
    3ce8:	01020044 	tsteq	r2, r4, asr #32
    3cec:	00000058 	andeq	r0, r0, r8, asr r0
    3cf0:	00000000 	andeq	r0, r0, r0
    3cf4:	01040044 	tsteq	r4, r4, asr #32
    3cf8:	00000060 	andeq	r0, r0, r0, rrx
    3cfc:	00000000 	andeq	r0, r0, r0
    3d00:	01050044 	tsteq	r5, r4, asr #32
    3d04:	00000080 	andeq	r0, r0, r0, lsl #1
    3d08:	00000000 	andeq	r0, r0, r0
    3d0c:	01050044 	tsteq	r5, r4, asr #32
    3d10:	0000008c 	andeq	r0, r0, ip, lsl #1
    3d14:	00000000 	andeq	r0, r0, r0
    3d18:	01060044 	tsteq	r6, r4, asr #32
    3d1c:	0000009c 	muleq	r0, ip, r0
    3d20:	00000000 	andeq	r0, r0, r0
    3d24:	01070044 	tsteq	r7, r4, asr #32
    3d28:	000000a4 	andeq	r0, r0, r4, lsr #1
    3d2c:	00000000 	andeq	r0, r0, r0
    3d30:	01080044 	tsteq	r8, r4, asr #32
    3d34:	000000c0 	andeq	r0, r0, r0, asr #1
    3d38:	000024e2 	andeq	r2, r0, r2, ror #9
    3d3c:	00fc0080 	rscseq	r0, ip, r0, lsl #1
    3d40:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3d44:	0000250e 	andeq	r2, r0, lr, lsl #10
    3d48:	01000080 	smlabbeq	r0, r0, r0, r0
    3d4c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    3d50:	000024ee 	andeq	r2, r0, lr, ror #9
    3d54:	01040080 	smlabbeq	r4, r0, r0, r0
    3d58:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    3d5c:	00000000 	andeq	r0, r0, r0
    3d60:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    3d6c:	000000e0 	andeq	r0, r0, r0, ror #1
    3d70:	000000cc 	andeq	r0, r0, ip, asr #1
    3d74:	00000000 	andeq	r0, r0, r0
    3d78:	00000024 	andeq	r0, r0, r4, lsr #32
    3d7c:	000000cc 	andeq	r0, r0, ip, asr #1
    3d80:	00000000 	andeq	r0, r0, r0
    3d84:	0000004e 	andeq	r0, r0, lr, asr #32
    3d88:	80011a18 	andhi	r1, r1, r8, lsl sl
    3d8c:	0000255c 	andeq	r2, r0, ip, asr r5
    3d90:	010a0024 	tsteq	sl, r4, lsr #32
    3d94:	80011a18 	andhi	r1, r1, r8, lsl sl
    3d98:	000022d0 	ldrdeq	r2, [r0], -r0
    3d9c:	010a00a0 	smlatbeq	sl, r0, r0, r0
    3da0:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3da4:	00000000 	andeq	r0, r0, r0
    3da8:	0000002e 	andeq	r0, r0, lr, lsr #32
    3dac:	80011a18 	andhi	r1, r1, r8, lsl sl
    3db0:	00000000 	andeq	r0, r0, r0
    3db4:	010a0044 	tsteq	sl, r4, asr #32
	...
    3dc0:	010b0044 	tsteq	fp, r4, asr #32
    3dc4:	00000014 	andeq	r0, r0, r4, lsl r0
    3dc8:	00000000 	andeq	r0, r0, r0
    3dcc:	010c0044 	tsteq	ip, r4, asr #32
    3dd0:	00000024 	andeq	r0, r0, r4, lsr #32
    3dd4:	00000000 	andeq	r0, r0, r0
    3dd8:	00000024 	andeq	r0, r0, r4, lsr #32
    3ddc:	00000030 	andeq	r0, r0, r0, lsr r0
    3de0:	00000000 	andeq	r0, r0, r0
    3de4:	0000004e 	andeq	r0, r0, lr, asr #32
    3de8:	80011a48 	andhi	r1, r1, r8, asr #20
    3dec:	00002574 	andeq	r2, r0, r4, ror r5
    3df0:	010e0024 	tsteq	lr, r4, lsr #32
    3df4:	80011a48 	andhi	r1, r1, r8, asr #20
    3df8:	000022d0 	ldrdeq	r2, [r0], -r0
    3dfc:	010e00a0 	smlatbeq	lr, r0, r0, r0
    3e00:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3e04:	00000000 	andeq	r0, r0, r0
    3e08:	0000002e 	andeq	r0, r0, lr, lsr #32
    3e0c:	80011a48 	andhi	r1, r1, r8, asr #20
    3e10:	00000000 	andeq	r0, r0, r0
    3e14:	010e0044 	tsteq	lr, r4, asr #32
	...
    3e20:	010f0044 	tsteq	pc, r4, asr #32
    3e24:	00000014 	andeq	r0, r0, r4, lsl r0
    3e28:	00000000 	andeq	r0, r0, r0
    3e2c:	01100044 	tsteq	r0, r4, asr #32
    3e30:	00000024 	andeq	r0, r0, r4, lsr #32
    3e34:	00000000 	andeq	r0, r0, r0
    3e38:	00000024 	andeq	r0, r0, r4, lsr #32
    3e3c:	00000030 	andeq	r0, r0, r0, lsr r0
    3e40:	00000000 	andeq	r0, r0, r0
    3e44:	0000004e 	andeq	r0, r0, lr, asr #32
    3e48:	80011a78 	andhi	r1, r1, r8, ror sl
    3e4c:	0000258c 	andeq	r2, r0, ip, lsl #11
    3e50:	01120024 	tsteq	r2, r4, lsr #32
    3e54:	80011a78 	andhi	r1, r1, r8, ror sl
    3e58:	0000259c 	muleq	r0, ip, r5
    3e5c:	011200a0 	tsteq	r2, r0, lsr #1
    3e60:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    3e64:	00000000 	andeq	r0, r0, r0
    3e68:	0000002e 	andeq	r0, r0, lr, lsr #32
    3e6c:	80011a78 	andhi	r1, r1, r8, ror sl
    3e70:	00000000 	andeq	r0, r0, r0
    3e74:	01120044 	tsteq	r2, r4, asr #32
	...
    3e80:	01130044 	tsteq	r3, r4, asr #32
    3e84:	0000001c 	andeq	r0, r0, ip, lsl r0
    3e88:	00000000 	andeq	r0, r0, r0
    3e8c:	01140044 	tsteq	r4, r4, asr #32
    3e90:	00000024 	andeq	r0, r0, r4, lsr #32
    3e94:	00000000 	andeq	r0, r0, r0
    3e98:	01150044 	tsteq	r5, r4, asr #32
    3e9c:	00000030 	andeq	r0, r0, r0, lsr r0
    3ea0:	00000000 	andeq	r0, r0, r0
    3ea4:	01160044 	tsteq	r6, r4, asr #32
    3ea8:	00000054 	andeq	r0, r0, r4, asr r0
    3eac:	00000000 	andeq	r0, r0, r0
    3eb0:	01150044 	tsteq	r5, r4, asr #32
    3eb4:	00000064 	andeq	r0, r0, r4, rrx
    3eb8:	00000000 	andeq	r0, r0, r0
    3ebc:	01160044 	tsteq	r6, r4, asr #32
    3ec0:	0000006c 	andeq	r0, r0, ip, rrx
    3ec4:	00000000 	andeq	r0, r0, r0
    3ec8:	01180044 	tsteq	r8, r4, asr #32
    3ecc:	00000078 	andeq	r0, r0, r8, ror r0
    3ed0:	00000000 	andeq	r0, r0, r0
    3ed4:	01170044 	tsteq	r7, r4, asr #32
    3ed8:	000000a8 	andeq	r0, r0, r8, lsr #1
    3edc:	00000000 	andeq	r0, r0, r0
    3ee0:	01190044 	tsteq	r9, r4, asr #32
    3ee4:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    3ee8:	00000000 	andeq	r0, r0, r0
    3eec:	01140044 	tsteq	r4, r4, asr #32
    3ef0:	000000bc 	strheq	r0, [r0], -ip
    3ef4:	00000000 	andeq	r0, r0, r0
    3ef8:	01140044 	tsteq	r4, r4, asr #32
    3efc:	000000c8 	andeq	r0, r0, r8, asr #1
    3f00:	00000000 	andeq	r0, r0, r0
    3f04:	011c0044 	tsteq	ip, r4, asr #32
    3f08:	000000d4 	ldrdeq	r0, [r0], -r4
    3f0c:	00000000 	andeq	r0, r0, r0
    3f10:	011d0044 	tsteq	sp, r4, asr #32
    3f14:	000000d8 	ldrdeq	r0, [r0], -r8
    3f18:	000025a9 	andeq	r2, r0, r9, lsr #11
    3f1c:	01130080 	tsteq	r3, r0, lsl #1
    3f20:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    3f24:	00000000 	andeq	r0, r0, r0
    3f28:	000000c0 	andeq	r0, r0, r0, asr #1
    3f2c:	00000000 	andeq	r0, r0, r0
    3f30:	00000ecb 	andeq	r0, r0, fp, asr #29
    3f34:	01140080 	tsteq	r4, r0, lsl #1
    3f38:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    3f3c:	00000000 	andeq	r0, r0, r0
    3f40:	000000c0 	andeq	r0, r0, r0, asr #1
    3f44:	00000024 	andeq	r0, r0, r4, lsr #32
    3f48:	00000000 	andeq	r0, r0, r0
    3f4c:	000000e0 	andeq	r0, r0, r0, ror #1
    3f50:	000000d4 	ldrdeq	r0, [r0], -r4
    3f54:	00000000 	andeq	r0, r0, r0
    3f58:	000000e0 	andeq	r0, r0, r0, ror #1
    3f5c:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    3f60:	00000000 	andeq	r0, r0, r0
    3f64:	00000024 	andeq	r0, r0, r4, lsr #32
    3f68:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    3f6c:	00000000 	andeq	r0, r0, r0
    3f70:	0000004e 	andeq	r0, r0, lr, asr #32
    3f74:	80011b68 	andhi	r1, r1, r8, ror #22
    3f78:	000025b3 			; <UNDEFINED> instruction: 0x000025b3
    3f7c:	011f0024 	tsteq	pc, r4, lsr #32
    3f80:	80011b68 	andhi	r1, r1, r8, ror #22
    3f84:	000022d0 	ldrdeq	r2, [r0], -r0
    3f88:	011f00a0 	tsteq	pc, r0, lsr #1
    3f8c:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    3f90:	000022dc 	ldrdeq	r2, [r0], -ip
    3f94:	011f00a0 	tsteq	pc, r0, lsr #1
    3f98:	ffffffd4 			; <UNDEFINED> instruction: 0xffffffd4
    3f9c:	00000000 	andeq	r0, r0, r0
    3fa0:	0000002e 	andeq	r0, r0, lr, lsr #32
    3fa4:	80011b68 	andhi	r1, r1, r8, ror #22
    3fa8:	00000000 	andeq	r0, r0, r0
    3fac:	011f0044 	tsteq	pc, r4, asr #32
	...
    3fb8:	01200044 			; <UNDEFINED> instruction: 0x01200044
    3fbc:	00000020 	andeq	r0, r0, r0, lsr #32
    3fc0:	00000000 	andeq	r0, r0, r0
    3fc4:	01210044 			; <UNDEFINED> instruction: 0x01210044
    3fc8:	00000028 	andeq	r0, r0, r8, lsr #32
    3fcc:	00000000 	andeq	r0, r0, r0
    3fd0:	01220044 			; <UNDEFINED> instruction: 0x01220044
    3fd4:	00000034 	andeq	r0, r0, r4, lsr r0
    3fd8:	00000000 	andeq	r0, r0, r0
    3fdc:	01230044 			; <UNDEFINED> instruction: 0x01230044
    3fe0:	00000040 	andeq	r0, r0, r0, asr #32
    3fe4:	00000000 	andeq	r0, r0, r0
    3fe8:	01260044 			; <UNDEFINED> instruction: 0x01260044
    3fec:	00000048 	andeq	r0, r0, r8, asr #32
    3ff0:	00000000 	andeq	r0, r0, r0
    3ff4:	01270044 			; <UNDEFINED> instruction: 0x01270044
    3ff8:	00000074 	andeq	r0, r0, r4, ror r0
    3ffc:	00000000 	andeq	r0, r0, r0
    4000:	01280044 			; <UNDEFINED> instruction: 0x01280044
    4004:	00000080 	andeq	r0, r0, r0, lsl #1
    4008:	00000000 	andeq	r0, r0, r0
    400c:	012a0044 			; <UNDEFINED> instruction: 0x012a0044
    4010:	00000088 	andeq	r0, r0, r8, lsl #1
    4014:	00000000 	andeq	r0, r0, r0
    4018:	012b0044 			; <UNDEFINED> instruction: 0x012b0044
    401c:	00000090 	muleq	r0, r0, r0
    4020:	00000000 	andeq	r0, r0, r0
    4024:	012c0044 			; <UNDEFINED> instruction: 0x012c0044
    4028:	0000009c 	muleq	r0, ip, r0
    402c:	00000000 	andeq	r0, r0, r0
    4030:	012d0044 			; <UNDEFINED> instruction: 0x012d0044
    4034:	000000c0 	andeq	r0, r0, r0, asr #1
    4038:	00000000 	andeq	r0, r0, r0
    403c:	012c0044 			; <UNDEFINED> instruction: 0x012c0044
    4040:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    4044:	00000000 	andeq	r0, r0, r0
    4048:	012d0044 			; <UNDEFINED> instruction: 0x012d0044
    404c:	000000d8 	ldrdeq	r0, [r0], -r8
    4050:	00000000 	andeq	r0, r0, r0
    4054:	012f0044 			; <UNDEFINED> instruction: 0x012f0044
    4058:	000000e4 	andeq	r0, r0, r4, ror #1
    405c:	00000000 	andeq	r0, r0, r0
    4060:	012e0044 			; <UNDEFINED> instruction: 0x012e0044
    4064:	00000114 	andeq	r0, r0, r4, lsl r1
    4068:	00000000 	andeq	r0, r0, r0
    406c:	01300044 	teqeq	r0, r4, asr #32
    4070:	0000011c 	andeq	r0, r0, ip, lsl r1
    4074:	00000000 	andeq	r0, r0, r0
    4078:	01310044 	teqeq	r1, r4, asr #32
    407c:	00000150 	andeq	r0, r0, r0, asr r1
    4080:	00000000 	andeq	r0, r0, r0
    4084:	01320044 	teqeq	r2, r4, asr #32
    4088:	00000188 	andeq	r0, r0, r8, lsl #3
    408c:	00000000 	andeq	r0, r0, r0
    4090:	01330044 	teqeq	r3, r4, asr #32
    4094:	000001c0 	andeq	r0, r0, r0, asr #3
    4098:	00000000 	andeq	r0, r0, r0
    409c:	01340044 	teqeq	r4, r4, asr #32
    40a0:	000001f8 	strdeq	r0, [r0], -r8
    40a4:	00000000 	andeq	r0, r0, r0
    40a8:	01350044 	teqeq	r5, r4, asr #32
    40ac:	00000238 	andeq	r0, r0, r8, lsr r2
    40b0:	00000000 	andeq	r0, r0, r0
    40b4:	012b0044 			; <UNDEFINED> instruction: 0x012b0044
    40b8:	00000244 	andeq	r0, r0, r4, asr #4
    40bc:	00000000 	andeq	r0, r0, r0
    40c0:	012b0044 			; <UNDEFINED> instruction: 0x012b0044
    40c4:	00000250 	andeq	r0, r0, r0, asr r2
    40c8:	00000000 	andeq	r0, r0, r0
    40cc:	012b0044 			; <UNDEFINED> instruction: 0x012b0044
    40d0:	0000025c 	andeq	r0, r0, ip, asr r2
    40d4:	00000000 	andeq	r0, r0, r0
    40d8:	01390044 	teqeq	r9, r4, asr #32
    40dc:	0000026c 	andeq	r0, r0, ip, ror #4
    40e0:	00000000 	andeq	r0, r0, r0
    40e4:	013a0044 	teqeq	sl, r4, asr #32
    40e8:	00000278 	andeq	r0, r0, r8, ror r2
    40ec:	00000000 	andeq	r0, r0, r0
    40f0:	013b0044 	teqeq	fp, r4, asr #32
    40f4:	0000027c 	andeq	r0, r0, ip, ror r2
    40f8:	000025cb 	andeq	r2, r0, fp, asr #11
    40fc:	01200080 	smlawbeq	r0, r0, r0, r0
    4100:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4104:	000025d7 	ldrdeq	r2, [r0], -r7
    4108:	01210080 	smlawbeq	r1, r0, r0, r0
    410c:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    4110:	000025e1 	andeq	r2, r0, r1, ror #11
    4114:	01260080 	smlawbeq	r6, r0, r0, r0
    4118:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    411c:	0000247b 	andeq	r2, r0, fp, ror r4
    4120:	012a0080 	smlawbeq	sl, r0, r0, r0
    4124:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4128:	00000000 	andeq	r0, r0, r0
    412c:	000000c0 	andeq	r0, r0, r0, asr #1
    4130:	00000000 	andeq	r0, r0, r0
    4134:	00000ecb 	andeq	r0, r0, fp, asr #29
    4138:	012b0080 	smlawbeq	fp, r0, r0, r0
    413c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    4140:	00000000 	andeq	r0, r0, r0
    4144:	000000c0 	andeq	r0, r0, r0, asr #1
    4148:	00000090 	muleq	r0, r0, r0
    414c:	00000000 	andeq	r0, r0, r0
    4150:	000000e0 	andeq	r0, r0, r0, ror #1
    4154:	0000026c 	andeq	r0, r0, ip, ror #4
    4158:	00000000 	andeq	r0, r0, r0
    415c:	000000e0 	andeq	r0, r0, r0, ror #1
    4160:	00000294 	muleq	r0, r4, r2
    4164:	00000000 	andeq	r0, r0, r0
    4168:	00000024 	andeq	r0, r0, r4, lsr #32
    416c:	00000294 	muleq	r0, r4, r2
    4170:	00000000 	andeq	r0, r0, r0
    4174:	0000004e 	andeq	r0, r0, lr, asr #32
    4178:	80011dfc 	strdhi	r1, [r1], -ip
    417c:	000025f6 	strdeq	r2, [r0], -r6
    4180:	013d0024 	teqeq	sp, r4, lsr #32
    4184:	80011dfc 	strdhi	r1, [r1], -ip
    4188:	000022d0 	ldrdeq	r2, [r0], -r0
    418c:	013d00a0 	teqeq	sp, r0, lsr #1
    4190:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4194:	000022dc 	ldrdeq	r2, [r0], -ip
    4198:	013d00a0 	teqeq	sp, r0, lsr #1
    419c:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    41a0:	00000000 	andeq	r0, r0, r0
    41a4:	0000002e 	andeq	r0, r0, lr, lsr #32
    41a8:	80011dfc 	strdhi	r1, [r1], -ip
    41ac:	00000000 	andeq	r0, r0, r0
    41b0:	013d0044 	teqeq	sp, r4, asr #32
	...
    41bc:	013e0044 	teqeq	lr, r4, asr #32
    41c0:	00000020 	andeq	r0, r0, r0, lsr #32
    41c4:	00000000 	andeq	r0, r0, r0
    41c8:	013f0044 	teqeq	pc, r4, asr #32
    41cc:	00000028 	andeq	r0, r0, r8, lsr #32
    41d0:	00000000 	andeq	r0, r0, r0
    41d4:	01400044 	cmpeq	r0, r4, asr #32
    41d8:	00000038 	andeq	r0, r0, r8, lsr r0
    41dc:	00000000 	andeq	r0, r0, r0
    41e0:	013f0044 	teqeq	pc, r4, asr #32
    41e4:	00000048 	andeq	r0, r0, r8, asr #32
    41e8:	00000000 	andeq	r0, r0, r0
    41ec:	01410044 	cmpeq	r1, r4, asr #32
    41f0:	00000058 	andeq	r0, r0, r8, asr r0
    41f4:	00000000 	andeq	r0, r0, r0
    41f8:	01420044 	cmpeq	r2, r4, asr #32
    41fc:	0000005c 	andeq	r0, r0, ip, asr r0
    4200:	0000260c 	andeq	r2, r0, ip, lsl #12
    4204:	013e0080 	teqeq	lr, r0, lsl #1
    4208:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    420c:	00000000 	andeq	r0, r0, r0
    4210:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    421c:	000000e0 	andeq	r0, r0, r0, ror #1
    4220:	00000070 	andeq	r0, r0, r0, ror r0
    4224:	00000000 	andeq	r0, r0, r0
    4228:	00000024 	andeq	r0, r0, r4, lsr #32
    422c:	00000070 	andeq	r0, r0, r0, ror r0
    4230:	00000000 	andeq	r0, r0, r0
    4234:	0000004e 	andeq	r0, r0, lr, asr #32
    4238:	80011e6c 	andhi	r1, r1, ip, ror #28
    423c:	00002616 	andeq	r2, r0, r6, lsl r6
    4240:	01440024 	cmpeq	r4, r4, lsr #32
    4244:	80011e6c 	andhi	r1, r1, ip, ror #28
    4248:	000022d0 	ldrdeq	r2, [r0], -r0
    424c:	014400a0 	smlaltbeq	r0, r4, r0, r0
    4250:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4254:	00000000 	andeq	r0, r0, r0
    4258:	0000002e 	andeq	r0, r0, lr, lsr #32
    425c:	80011e6c 	andhi	r1, r1, ip, ror #28
    4260:	00000000 	andeq	r0, r0, r0
    4264:	01440044 	cmpeq	r4, r4, asr #32
	...
    4270:	01450044 	cmpeq	r5, r4, asr #32
    4274:	0000001c 	andeq	r0, r0, ip, lsl r0
    4278:	00000000 	andeq	r0, r0, r0
    427c:	01460044 	cmpeq	r6, r4, asr #32
    4280:	00000024 	andeq	r0, r0, r4, lsr #32
    4284:	00000000 	andeq	r0, r0, r0
    4288:	01470044 	cmpeq	r7, r4, asr #32
    428c:	00000044 	andeq	r0, r0, r4, asr #32
    4290:	00000000 	andeq	r0, r0, r0
    4294:	01480044 	cmpeq	r8, r4, asr #32
    4298:	00000048 	andeq	r0, r0, r8, asr #32
    429c:	0000262c 	andeq	r2, r0, ip, lsr #12
    42a0:	01450080 	smlalbbeq	r0, r5, r0, r0
    42a4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    42a8:	00000000 	andeq	r0, r0, r0
    42ac:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    42b8:	000000e0 	andeq	r0, r0, r0, ror #1
    42bc:	0000005c 	andeq	r0, r0, ip, asr r0
    42c0:	00000000 	andeq	r0, r0, r0
    42c4:	00000024 	andeq	r0, r0, r4, lsr #32
    42c8:	0000005c 	andeq	r0, r0, ip, asr r0
    42cc:	00000000 	andeq	r0, r0, r0
    42d0:	0000004e 	andeq	r0, r0, lr, asr #32
    42d4:	80011ec8 	andhi	r1, r1, r8, asr #29
    42d8:	00002637 	andeq	r2, r0, r7, lsr r6
    42dc:	014a0024 	cmpeq	sl, r4, lsr #32
    42e0:	80011ec8 	andhi	r1, r1, r8, asr #29
    42e4:	000022d0 	ldrdeq	r2, [r0], -r0
    42e8:	014a00a0 	smlaltbeq	r0, sl, r0, r0
    42ec:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    42f0:	000022dc 	ldrdeq	r2, [r0], -ip
    42f4:	014a00a0 	smlaltbeq	r0, sl, r0, r0
    42f8:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    42fc:	00000000 	andeq	r0, r0, r0
    4300:	0000002e 	andeq	r0, r0, lr, lsr #32
    4304:	80011ec8 	andhi	r1, r1, r8, asr #29
    4308:	00000000 	andeq	r0, r0, r0
    430c:	014a0044 	cmpeq	sl, r4, asr #32
	...
    4318:	014b0044 	cmpeq	fp, r4, asr #32
    431c:	00000020 	andeq	r0, r0, r0, lsr #32
    4320:	00000000 	andeq	r0, r0, r0
    4324:	014c0044 	cmpeq	ip, r4, asr #32
    4328:	00000028 	andeq	r0, r0, r8, lsr #32
    432c:	00000000 	andeq	r0, r0, r0
    4330:	014d0044 	cmpeq	sp, r4, asr #32
    4334:	0000004c 	andeq	r0, r0, ip, asr #32
    4338:	00000000 	andeq	r0, r0, r0
    433c:	014e0044 	cmpeq	lr, r4, asr #32
    4340:	00000050 	andeq	r0, r0, r0, asr r0
    4344:	0000264d 	andeq	r2, r0, sp, asr #12
    4348:	014b0080 	smlalbbeq	r0, fp, r0, r0
    434c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4350:	00000000 	andeq	r0, r0, r0
    4354:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    4360:	000000e0 	andeq	r0, r0, r0, ror #1
    4364:	00000064 	andeq	r0, r0, r4, rrx
    4368:	00000000 	andeq	r0, r0, r0
    436c:	00000024 	andeq	r0, r0, r4, lsr #32
    4370:	00000064 	andeq	r0, r0, r4, rrx
    4374:	00000000 	andeq	r0, r0, r0
    4378:	0000004e 	andeq	r0, r0, lr, asr #32
    437c:	80011f2c 	andhi	r1, r1, ip, lsr #30
    4380:	00002657 	andeq	r2, r0, r7, asr r6
    4384:	01500024 	cmpeq	r0, r4, lsr #32
    4388:	80011f2c 	andhi	r1, r1, ip, lsr #30
    438c:	000022d0 	ldrdeq	r2, [r0], -r0
    4390:	015000a0 	cmpeq	r0, r0, lsr #1
    4394:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4398:	000022dc 	ldrdeq	r2, [r0], -ip
    439c:	015000a0 	cmpeq	r0, r0, lsr #1
    43a0:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    43a4:	00000000 	andeq	r0, r0, r0
    43a8:	0000002e 	andeq	r0, r0, lr, lsr #32
    43ac:	80011f2c 	andhi	r1, r1, ip, lsr #30
    43b0:	00000000 	andeq	r0, r0, r0
    43b4:	01500044 	cmpeq	r0, r4, asr #32
	...
    43c0:	01510044 	cmpeq	r1, r4, asr #32
    43c4:	00000020 	andeq	r0, r0, r0, lsr #32
    43c8:	00000000 	andeq	r0, r0, r0
    43cc:	01510044 	cmpeq	r1, r4, asr #32
    43d0:	0000002c 	andeq	r0, r0, ip, lsr #32
    43d4:	00000000 	andeq	r0, r0, r0
    43d8:	01520044 	cmpeq	r2, r4, asr #32
    43dc:	00000044 	andeq	r0, r0, r4, asr #32
    43e0:	00000000 	andeq	r0, r0, r0
    43e4:	01550044 	cmpeq	r5, r4, asr #32
    43e8:	0000004c 	andeq	r0, r0, ip, asr #32
    43ec:	00000000 	andeq	r0, r0, r0
    43f0:	01560044 	cmpeq	r6, r4, asr #32
    43f4:	00000058 	andeq	r0, r0, r8, asr r0
    43f8:	00000000 	andeq	r0, r0, r0
    43fc:	01570044 	cmpeq	r7, r4, asr #32
    4400:	00000064 	andeq	r0, r0, r4, rrx
    4404:	00000000 	andeq	r0, r0, r0
    4408:	015a0044 	cmpeq	sl, r4, asr #32
    440c:	0000006c 	andeq	r0, r0, ip, rrx
    4410:	00000000 	andeq	r0, r0, r0
    4414:	015b0044 	cmpeq	fp, r4, asr #32
    4418:	00000078 	andeq	r0, r0, r8, ror r0
    441c:	00000000 	andeq	r0, r0, r0
    4420:	015c0044 	cmpeq	ip, r4, asr #32
    4424:	0000007c 	andeq	r0, r0, ip, ror r0
    4428:	000024e2 	andeq	r2, r0, r2, ror #9
    442c:	01550080 	cmpeq	r5, r0, lsl #1
    4430:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4434:	00000000 	andeq	r0, r0, r0
    4438:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    4444:	000000e0 	andeq	r0, r0, r0, ror #1
    4448:	00000090 	muleq	r0, r0, r0
    444c:	00000000 	andeq	r0, r0, r0
    4450:	00000024 	andeq	r0, r0, r4, lsr #32
    4454:	00000090 	muleq	r0, r0, r0
    4458:	00000000 	andeq	r0, r0, r0
    445c:	0000004e 	andeq	r0, r0, lr, asr #32
    4460:	80011fbc 			; <UNDEFINED> instruction: 0x80011fbc
    4464:	0000266d 	andeq	r2, r0, sp, ror #12
    4468:	015e0024 	cmpeq	lr, r4, lsr #32
    446c:	80011fbc 			; <UNDEFINED> instruction: 0x80011fbc
    4470:	000022d0 	ldrdeq	r2, [r0], -r0
    4474:	015e00a0 	cmpeq	lr, r0, lsr #1
    4478:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    447c:	00000000 	andeq	r0, r0, r0
    4480:	0000002e 	andeq	r0, r0, lr, lsr #32
    4484:	80011fbc 			; <UNDEFINED> instruction: 0x80011fbc
    4488:	00000000 	andeq	r0, r0, r0
    448c:	015e0044 	cmpeq	lr, r4, asr #32
	...
    4498:	015f0044 	cmpeq	pc, r4, asr #32
    449c:	00000014 	andeq	r0, r0, r4, lsl r0
    44a0:	00000000 	andeq	r0, r0, r0
    44a4:	01600044 	cmneq	r0, r4, asr #32
    44a8:	00000020 	andeq	r0, r0, r0, lsr #32
    44ac:	00000000 	andeq	r0, r0, r0
    44b0:	01610044 	cmneq	r1, r4, asr #32
    44b4:	0000002c 	andeq	r0, r0, ip, lsr #32
    44b8:	00000000 	andeq	r0, r0, r0
    44bc:	01620044 	cmneq	r2, r4, asr #32
    44c0:	00000034 	andeq	r0, r0, r4, lsr r0
    44c4:	00000000 	andeq	r0, r0, r0
    44c8:	01630044 	cmneq	r3, r4, asr #32
    44cc:	0000003c 	andeq	r0, r0, ip, lsr r0
    44d0:	000024e2 	andeq	r2, r0, r2, ror #9
    44d4:	015f0080 	cmpeq	pc, r0, lsl #1
    44d8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    44dc:	00000000 	andeq	r0, r0, r0
    44e0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    44ec:	000000e0 	andeq	r0, r0, r0, ror #1
    44f0:	00000048 	andeq	r0, r0, r8, asr #32
    44f4:	00000000 	andeq	r0, r0, r0
    44f8:	00000024 	andeq	r0, r0, r4, lsr #32
    44fc:	00000048 	andeq	r0, r0, r8, asr #32
    4500:	00000000 	andeq	r0, r0, r0
    4504:	0000004e 	andeq	r0, r0, lr, asr #32
    4508:	80012004 	andhi	r2, r1, r4
    450c:	00002683 	andeq	r2, r0, r3, lsl #13
    4510:	018f0024 	orreq	r0, pc, r4, lsr #32
    4514:	80012004 	andhi	r2, r1, r4
    4518:	00002698 	muleq	r0, r8, r6
    451c:	018f00a0 	orreq	r0, pc, r0, lsr #1
    4520:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4524:	000022d0 	ldrdeq	r2, [r0], -r0
    4528:	018f00a0 	orreq	r0, pc, r0, lsr #1
    452c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    4530:	000022dc 	ldrdeq	r2, [r0], -ip
    4534:	018f00a0 	orreq	r0, pc, r0, lsr #1
    4538:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    453c:	000023c5 	andeq	r2, r0, r5, asr #7
    4540:	018f00a0 	orreq	r0, pc, r0, lsr #1
    4544:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    4548:	00000000 	andeq	r0, r0, r0
    454c:	0000002e 	andeq	r0, r0, lr, lsr #32
    4550:	80012004 	andhi	r2, r1, r4
    4554:	00000000 	andeq	r0, r0, r0
    4558:	01900044 	orrseq	r0, r0, r4, asr #32
	...
    4564:	01910044 	orrseq	r0, r1, r4, asr #32
    4568:	00000020 	andeq	r0, r0, r0, lsr #32
    456c:	00000000 	andeq	r0, r0, r0
    4570:	01920044 	orrseq	r0, r2, r4, asr #32
    4574:	00000044 	andeq	r0, r0, r4, asr #32
    4578:	00000000 	andeq	r0, r0, r0
    457c:	00000024 	andeq	r0, r0, r4, lsr #32
    4580:	00000054 	andeq	r0, r0, r4, asr r0
    4584:	00000000 	andeq	r0, r0, r0
    4588:	0000004e 	andeq	r0, r0, lr, asr #32
    458c:	80012058 	andhi	r2, r1, r8, asr r0
    4590:	000026a4 	andeq	r2, r0, r4, lsr #13
    4594:	00060026 	andeq	r0, r6, r6, lsr #32
    4598:	80015238 	andhi	r5, r1, r8, lsr r2
    459c:	000026c0 	andeq	r2, r0, r0, asr #13
    45a0:	01650026 	cmneq	r5, r6, lsr #32
    45a4:	80020038 	andhi	r0, r2, r8, lsr r0
    45a8:	00000000 	andeq	r0, r0, r0
    45ac:	00000064 	andeq	r0, r0, r4, rrx
    45b0:	80012058 	andhi	r2, r1, r8, asr r0
    45b4:	00000007 	andeq	r0, r0, r7
    45b8:	00020064 	andeq	r0, r2, r4, rrx
    45bc:	80012058 	andhi	r2, r1, r8, asr r0
    45c0:	00002704 	andeq	r2, r0, r4, lsl #14
    45c4:	00020064 	andeq	r0, r2, r4, rrx
    45c8:	80012058 	andhi	r2, r1, r8, asr r0
    45cc:	0000003d 	andeq	r0, r0, sp, lsr r0
    45d0:	0000003c 	andeq	r0, r0, ip, lsr r0
    45d4:	00000000 	andeq	r0, r0, r0
    45d8:	0000004c 	andeq	r0, r0, ip, asr #32
    45dc:	00000080 	andeq	r0, r0, r0, lsl #1
    45e0:	00000000 	andeq	r0, r0, r0
    45e4:	00000076 	andeq	r0, r0, r6, ror r0
    45e8:	00000080 	andeq	r0, r0, r0, lsl #1
    45ec:	00000000 	andeq	r0, r0, r0
    45f0:	00000094 	muleq	r0, r4, r0
    45f4:	00000080 	andeq	r0, r0, r0, lsl #1
    45f8:	00000000 	andeq	r0, r0, r0
    45fc:	000000c3 	andeq	r0, r0, r3, asr #1
    4600:	00000080 	andeq	r0, r0, r0, lsl #1
    4604:	00000000 	andeq	r0, r0, r0
    4608:	000000ee 	andeq	r0, r0, lr, ror #1
    460c:	00000080 	andeq	r0, r0, r0, lsl #1
    4610:	00000000 	andeq	r0, r0, r0
    4614:	0000011e 	andeq	r0, r0, lr, lsl r1
    4618:	00000080 	andeq	r0, r0, r0, lsl #1
    461c:	00000000 	andeq	r0, r0, r0
    4620:	0000016f 	andeq	r0, r0, pc, ror #2
    4624:	00000080 	andeq	r0, r0, r0, lsl #1
    4628:	00000000 	andeq	r0, r0, r0
    462c:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    4630:	00000080 	andeq	r0, r0, r0, lsl #1
    4634:	00000000 	andeq	r0, r0, r0
    4638:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    463c:	00000080 	andeq	r0, r0, r0, lsl #1
    4640:	00000000 	andeq	r0, r0, r0
    4644:	0000020e 	andeq	r0, r0, lr, lsl #4
    4648:	00000080 	andeq	r0, r0, r0, lsl #1
    464c:	00000000 	andeq	r0, r0, r0
    4650:	00000238 	andeq	r0, r0, r8, lsr r2
    4654:	00000080 	andeq	r0, r0, r0, lsl #1
    4658:	00000000 	andeq	r0, r0, r0
    465c:	00000261 	andeq	r0, r0, r1, ror #4
    4660:	00000080 	andeq	r0, r0, r0, lsl #1
    4664:	00000000 	andeq	r0, r0, r0
    4668:	0000027b 	andeq	r0, r0, fp, ror r2
    466c:	00000080 	andeq	r0, r0, r0, lsl #1
    4670:	00000000 	andeq	r0, r0, r0
    4674:	00000296 	muleq	r0, r6, r2
    4678:	00000080 	andeq	r0, r0, r0, lsl #1
    467c:	00000000 	andeq	r0, r0, r0
    4680:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    4684:	00000080 	andeq	r0, r0, r0, lsl #1
    4688:	00000000 	andeq	r0, r0, r0
    468c:	000002d7 	ldrdeq	r0, [r0], -r7
    4690:	00000080 	andeq	r0, r0, r0, lsl #1
    4694:	00000000 	andeq	r0, r0, r0
    4698:	000002f7 	strdeq	r0, [r0], -r7
    469c:	00000080 	andeq	r0, r0, r0, lsl #1
    46a0:	00000000 	andeq	r0, r0, r0
    46a4:	0000031c 	andeq	r0, r0, ip, lsl r3
    46a8:	00000080 	andeq	r0, r0, r0, lsl #1
    46ac:	00000000 	andeq	r0, r0, r0
    46b0:	00000346 	andeq	r0, r0, r6, asr #6
    46b4:	00000080 	andeq	r0, r0, r0, lsl #1
    46b8:	00000000 	andeq	r0, r0, r0
    46bc:	0000036a 	andeq	r0, r0, sl, ror #6
    46c0:	00000080 	andeq	r0, r0, r0, lsl #1
    46c4:	00000000 	andeq	r0, r0, r0
    46c8:	00000393 	muleq	r0, r3, r3
    46cc:	00000080 	andeq	r0, r0, r0, lsl #1
    46d0:	00000000 	andeq	r0, r0, r0
    46d4:	000003c1 	andeq	r0, r0, r1, asr #7
    46d8:	00000080 	andeq	r0, r0, r0, lsl #1
    46dc:	00000000 	andeq	r0, r0, r0
    46e0:	000003e7 	andeq	r0, r0, r7, ror #7
    46e4:	00000080 	andeq	r0, r0, r0, lsl #1
    46e8:	00000000 	andeq	r0, r0, r0
    46ec:	00000407 	andeq	r0, r0, r7, lsl #8
    46f0:	00000080 	andeq	r0, r0, r0, lsl #1
    46f4:	00000000 	andeq	r0, r0, r0
    46f8:	0000042c 	andeq	r0, r0, ip, lsr #8
    46fc:	00000080 	andeq	r0, r0, r0, lsl #1
    4700:	00000000 	andeq	r0, r0, r0
    4704:	00000456 	andeq	r0, r0, r6, asr r4
    4708:	00000080 	andeq	r0, r0, r0, lsl #1
    470c:	00000000 	andeq	r0, r0, r0
    4710:	00000485 	andeq	r0, r0, r5, lsl #9
    4714:	00000080 	andeq	r0, r0, r0, lsl #1
    4718:	00000000 	andeq	r0, r0, r0
    471c:	000004ae 	andeq	r0, r0, lr, lsr #9
    4720:	00000080 	andeq	r0, r0, r0, lsl #1
    4724:	00000000 	andeq	r0, r0, r0
    4728:	000004dc 	ldrdeq	r0, [r0], -ip
    472c:	00000080 	andeq	r0, r0, r0, lsl #1
    4730:	00000000 	andeq	r0, r0, r0
    4734:	0000050f 	andeq	r0, r0, pc, lsl #10
    4738:	00000080 	andeq	r0, r0, r0, lsl #1
    473c:	00000000 	andeq	r0, r0, r0
    4740:	00000530 	andeq	r0, r0, r0, lsr r5
    4744:	00000080 	andeq	r0, r0, r0, lsl #1
    4748:	00000000 	andeq	r0, r0, r0
    474c:	00000550 	andeq	r0, r0, r0, asr r5
    4750:	00000080 	andeq	r0, r0, r0, lsl #1
    4754:	00000000 	andeq	r0, r0, r0
    4758:	00000575 	andeq	r0, r0, r5, ror r5
    475c:	00000080 	andeq	r0, r0, r0, lsl #1
    4760:	00000000 	andeq	r0, r0, r0
    4764:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    4768:	00000080 	andeq	r0, r0, r0, lsl #1
    476c:	00000000 	andeq	r0, r0, r0
    4770:	000005c3 	andeq	r0, r0, r3, asr #11
    4774:	00000080 	andeq	r0, r0, r0, lsl #1
    4778:	00000000 	andeq	r0, r0, r0
    477c:	000005ec 	andeq	r0, r0, ip, ror #11
    4780:	00000080 	andeq	r0, r0, r0, lsl #1
    4784:	00000000 	andeq	r0, r0, r0
    4788:	0000061a 	andeq	r0, r0, sl, lsl r6
    478c:	00000080 	andeq	r0, r0, r0, lsl #1
    4790:	00000000 	andeq	r0, r0, r0
    4794:	00000640 	andeq	r0, r0, r0, asr #12
    4798:	00000080 	andeq	r0, r0, r0, lsl #1
    479c:	00000000 	andeq	r0, r0, r0
    47a0:	00000660 	andeq	r0, r0, r0, ror #12
    47a4:	00000080 	andeq	r0, r0, r0, lsl #1
    47a8:	00000000 	andeq	r0, r0, r0
    47ac:	00000685 	andeq	r0, r0, r5, lsl #13
    47b0:	00000080 	andeq	r0, r0, r0, lsl #1
    47b4:	00000000 	andeq	r0, r0, r0
    47b8:	000006af 	andeq	r0, r0, pc, lsr #13
    47bc:	00000080 	andeq	r0, r0, r0, lsl #1
    47c0:	00000000 	andeq	r0, r0, r0
    47c4:	000006de 	ldrdeq	r0, [r0], -lr
    47c8:	00000080 	andeq	r0, r0, r0, lsl #1
    47cc:	00000000 	andeq	r0, r0, r0
    47d0:	00000707 	andeq	r0, r0, r7, lsl #14
    47d4:	00000080 	andeq	r0, r0, r0, lsl #1
    47d8:	00000000 	andeq	r0, r0, r0
    47dc:	00000735 	andeq	r0, r0, r5, lsr r7
    47e0:	00000080 	andeq	r0, r0, r0, lsl #1
    47e4:	00000000 	andeq	r0, r0, r0
    47e8:	00000768 	andeq	r0, r0, r8, ror #14
    47ec:	00000080 	andeq	r0, r0, r0, lsl #1
    47f0:	00000000 	andeq	r0, r0, r0
    47f4:	00000886 	andeq	r0, r0, r6, lsl #17
    47f8:	000000c2 	andeq	r0, r0, r2, asr #1
    47fc:	00003ac8 	andeq	r3, r0, r8, asr #21
    4800:	0000078a 	andeq	r0, r0, sl, lsl #15
    4804:	000000c2 	andeq	r0, r0, r2, asr #1
    4808:	00002d60 	andeq	r2, r0, r0, ror #26
    480c:	00002715 	andeq	r2, r0, r5, lsl r7
    4810:	00030020 	andeq	r0, r3, r0, lsr #32
    4814:	00000000 	andeq	r0, r0, r0
    4818:	00002756 	andeq	r2, r0, r6, asr r7
    481c:	00040020 	andeq	r0, r4, r0, lsr #32
    4820:	00000000 	andeq	r0, r0, r0
    4824:	00002768 	andeq	r2, r0, r8, ror #14
    4828:	00070020 	andeq	r0, r7, r0, lsr #32
	...
    4834:	00000064 	andeq	r0, r0, r4, rrx
    4838:	80012058 	andhi	r2, r1, r8, asr r0
    483c:	00000007 	andeq	r0, r0, r7
    4840:	00020064 	andeq	r0, r2, r4, rrx
    4844:	80012058 	andhi	r2, r1, r8, asr r0
    4848:	00002796 	muleq	r0, r6, r7
    484c:	00020064 	andeq	r0, r2, r4, rrx
    4850:	80012058 	andhi	r2, r1, r8, asr r0
    4854:	0000003d 	andeq	r0, r0, sp, lsr r0
    4858:	0000003c 	andeq	r0, r0, ip, lsr r0
    485c:	00000000 	andeq	r0, r0, r0
    4860:	0000004c 	andeq	r0, r0, ip, asr #32
    4864:	00000080 	andeq	r0, r0, r0, lsl #1
    4868:	00000000 	andeq	r0, r0, r0
    486c:	00000076 	andeq	r0, r0, r6, ror r0
    4870:	00000080 	andeq	r0, r0, r0, lsl #1
    4874:	00000000 	andeq	r0, r0, r0
    4878:	00000094 	muleq	r0, r4, r0
    487c:	00000080 	andeq	r0, r0, r0, lsl #1
    4880:	00000000 	andeq	r0, r0, r0
    4884:	000000c3 	andeq	r0, r0, r3, asr #1
    4888:	00000080 	andeq	r0, r0, r0, lsl #1
    488c:	00000000 	andeq	r0, r0, r0
    4890:	000000ee 	andeq	r0, r0, lr, ror #1
    4894:	00000080 	andeq	r0, r0, r0, lsl #1
    4898:	00000000 	andeq	r0, r0, r0
    489c:	0000011e 	andeq	r0, r0, lr, lsl r1
    48a0:	00000080 	andeq	r0, r0, r0, lsl #1
    48a4:	00000000 	andeq	r0, r0, r0
    48a8:	0000016f 	andeq	r0, r0, pc, ror #2
    48ac:	00000080 	andeq	r0, r0, r0, lsl #1
    48b0:	00000000 	andeq	r0, r0, r0
    48b4:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    48b8:	00000080 	andeq	r0, r0, r0, lsl #1
    48bc:	00000000 	andeq	r0, r0, r0
    48c0:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    48c4:	00000080 	andeq	r0, r0, r0, lsl #1
    48c8:	00000000 	andeq	r0, r0, r0
    48cc:	0000020e 	andeq	r0, r0, lr, lsl #4
    48d0:	00000080 	andeq	r0, r0, r0, lsl #1
    48d4:	00000000 	andeq	r0, r0, r0
    48d8:	00000238 	andeq	r0, r0, r8, lsr r2
    48dc:	00000080 	andeq	r0, r0, r0, lsl #1
    48e0:	00000000 	andeq	r0, r0, r0
    48e4:	00000261 	andeq	r0, r0, r1, ror #4
    48e8:	00000080 	andeq	r0, r0, r0, lsl #1
    48ec:	00000000 	andeq	r0, r0, r0
    48f0:	0000027b 	andeq	r0, r0, fp, ror r2
    48f4:	00000080 	andeq	r0, r0, r0, lsl #1
    48f8:	00000000 	andeq	r0, r0, r0
    48fc:	00000296 	muleq	r0, r6, r2
    4900:	00000080 	andeq	r0, r0, r0, lsl #1
    4904:	00000000 	andeq	r0, r0, r0
    4908:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    490c:	00000080 	andeq	r0, r0, r0, lsl #1
    4910:	00000000 	andeq	r0, r0, r0
    4914:	000002d7 	ldrdeq	r0, [r0], -r7
    4918:	00000080 	andeq	r0, r0, r0, lsl #1
    491c:	00000000 	andeq	r0, r0, r0
    4920:	000002f7 	strdeq	r0, [r0], -r7
    4924:	00000080 	andeq	r0, r0, r0, lsl #1
    4928:	00000000 	andeq	r0, r0, r0
    492c:	0000031c 	andeq	r0, r0, ip, lsl r3
    4930:	00000080 	andeq	r0, r0, r0, lsl #1
    4934:	00000000 	andeq	r0, r0, r0
    4938:	00000346 	andeq	r0, r0, r6, asr #6
    493c:	00000080 	andeq	r0, r0, r0, lsl #1
    4940:	00000000 	andeq	r0, r0, r0
    4944:	0000036a 	andeq	r0, r0, sl, ror #6
    4948:	00000080 	andeq	r0, r0, r0, lsl #1
    494c:	00000000 	andeq	r0, r0, r0
    4950:	00000393 	muleq	r0, r3, r3
    4954:	00000080 	andeq	r0, r0, r0, lsl #1
    4958:	00000000 	andeq	r0, r0, r0
    495c:	000003c1 	andeq	r0, r0, r1, asr #7
    4960:	00000080 	andeq	r0, r0, r0, lsl #1
    4964:	00000000 	andeq	r0, r0, r0
    4968:	000003e7 	andeq	r0, r0, r7, ror #7
    496c:	00000080 	andeq	r0, r0, r0, lsl #1
    4970:	00000000 	andeq	r0, r0, r0
    4974:	00000407 	andeq	r0, r0, r7, lsl #8
    4978:	00000080 	andeq	r0, r0, r0, lsl #1
    497c:	00000000 	andeq	r0, r0, r0
    4980:	0000042c 	andeq	r0, r0, ip, lsr #8
    4984:	00000080 	andeq	r0, r0, r0, lsl #1
    4988:	00000000 	andeq	r0, r0, r0
    498c:	00000456 	andeq	r0, r0, r6, asr r4
    4990:	00000080 	andeq	r0, r0, r0, lsl #1
    4994:	00000000 	andeq	r0, r0, r0
    4998:	00000485 	andeq	r0, r0, r5, lsl #9
    499c:	00000080 	andeq	r0, r0, r0, lsl #1
    49a0:	00000000 	andeq	r0, r0, r0
    49a4:	000004ae 	andeq	r0, r0, lr, lsr #9
    49a8:	00000080 	andeq	r0, r0, r0, lsl #1
    49ac:	00000000 	andeq	r0, r0, r0
    49b0:	000004dc 	ldrdeq	r0, [r0], -ip
    49b4:	00000080 	andeq	r0, r0, r0, lsl #1
    49b8:	00000000 	andeq	r0, r0, r0
    49bc:	0000050f 	andeq	r0, r0, pc, lsl #10
    49c0:	00000080 	andeq	r0, r0, r0, lsl #1
    49c4:	00000000 	andeq	r0, r0, r0
    49c8:	00000530 	andeq	r0, r0, r0, lsr r5
    49cc:	00000080 	andeq	r0, r0, r0, lsl #1
    49d0:	00000000 	andeq	r0, r0, r0
    49d4:	00000550 	andeq	r0, r0, r0, asr r5
    49d8:	00000080 	andeq	r0, r0, r0, lsl #1
    49dc:	00000000 	andeq	r0, r0, r0
    49e0:	00000575 	andeq	r0, r0, r5, ror r5
    49e4:	00000080 	andeq	r0, r0, r0, lsl #1
    49e8:	00000000 	andeq	r0, r0, r0
    49ec:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    49f0:	00000080 	andeq	r0, r0, r0, lsl #1
    49f4:	00000000 	andeq	r0, r0, r0
    49f8:	000005c3 	andeq	r0, r0, r3, asr #11
    49fc:	00000080 	andeq	r0, r0, r0, lsl #1
    4a00:	00000000 	andeq	r0, r0, r0
    4a04:	000005ec 	andeq	r0, r0, ip, ror #11
    4a08:	00000080 	andeq	r0, r0, r0, lsl #1
    4a0c:	00000000 	andeq	r0, r0, r0
    4a10:	0000061a 	andeq	r0, r0, sl, lsl r6
    4a14:	00000080 	andeq	r0, r0, r0, lsl #1
    4a18:	00000000 	andeq	r0, r0, r0
    4a1c:	00000640 	andeq	r0, r0, r0, asr #12
    4a20:	00000080 	andeq	r0, r0, r0, lsl #1
    4a24:	00000000 	andeq	r0, r0, r0
    4a28:	00000660 	andeq	r0, r0, r0, ror #12
    4a2c:	00000080 	andeq	r0, r0, r0, lsl #1
    4a30:	00000000 	andeq	r0, r0, r0
    4a34:	00000685 	andeq	r0, r0, r5, lsl #13
    4a38:	00000080 	andeq	r0, r0, r0, lsl #1
    4a3c:	00000000 	andeq	r0, r0, r0
    4a40:	000006af 	andeq	r0, r0, pc, lsr #13
    4a44:	00000080 	andeq	r0, r0, r0, lsl #1
    4a48:	00000000 	andeq	r0, r0, r0
    4a4c:	000006de 	ldrdeq	r0, [r0], -lr
    4a50:	00000080 	andeq	r0, r0, r0, lsl #1
    4a54:	00000000 	andeq	r0, r0, r0
    4a58:	00000707 	andeq	r0, r0, r7, lsl #14
    4a5c:	00000080 	andeq	r0, r0, r0, lsl #1
    4a60:	00000000 	andeq	r0, r0, r0
    4a64:	00000735 	andeq	r0, r0, r5, lsr r7
    4a68:	00000080 	andeq	r0, r0, r0, lsl #1
    4a6c:	00000000 	andeq	r0, r0, r0
    4a70:	00000768 	andeq	r0, r0, r8, ror #14
    4a74:	00000080 	andeq	r0, r0, r0, lsl #1
    4a78:	00000000 	andeq	r0, r0, r0
    4a7c:	00000886 	andeq	r0, r0, r6, lsl #17
    4a80:	000000c2 	andeq	r0, r0, r2, asr #1
    4a84:	00003ac8 	andeq	r3, r0, r8, asr #21
    4a88:	0000078a 	andeq	r0, r0, sl, lsl #15
    4a8c:	000000c2 	andeq	r0, r0, r2, asr #1
    4a90:	00002d60 	andeq	r2, r0, r0, ror #26
    4a94:	00001000 	andeq	r1, r0, r0
    4a98:	000000c2 	andeq	r0, r0, r2, asr #1
    4a9c:	0000100f 	andeq	r1, r0, pc
    4aa0:	000027a3 	andeq	r2, r0, r3, lsr #15
    4aa4:	000a0024 	andeq	r0, sl, r4, lsr #32
    4aa8:	80012058 	andhi	r2, r1, r8, asr r0
    4aac:	000027b4 			; <UNDEFINED> instruction: 0x000027b4
    4ab0:	000a00a0 	andeq	r0, sl, r0, lsr #1
    4ab4:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    4ab8:	000027c6 	andeq	r2, r0, r6, asr #15
    4abc:	000a00a0 	andeq	r0, sl, r0, lsr #1
    4ac0:	ffffffd4 			; <UNDEFINED> instruction: 0xffffffd4
    4ac4:	000027d3 	ldrdeq	r2, [r0], -r3
    4ac8:	000a00a0 	andeq	r0, sl, r0, lsr #1
    4acc:	ffffffd0 			; <UNDEFINED> instruction: 0xffffffd0
    4ad0:	000027e1 	andeq	r2, r0, r1, ror #15
    4ad4:	000a00a0 	andeq	r0, sl, r0, lsr #1
    4ad8:	ffffffcc 			; <UNDEFINED> instruction: 0xffffffcc
    4adc:	000027ed 	andeq	r2, r0, sp, ror #15
    4ae0:	000a00a0 	andeq	r0, sl, r0, lsr #1
    4ae4:	00000004 	andeq	r0, r0, r4
    4ae8:	00000000 	andeq	r0, r0, r0
    4aec:	0000002e 	andeq	r0, r0, lr, lsr #32
    4af0:	80012058 	andhi	r2, r1, r8, asr r0
    4af4:	00000000 	andeq	r0, r0, r0
    4af8:	000b0044 	andeq	r0, fp, r4, asr #32
	...
    4b04:	000c0044 	andeq	r0, ip, r4, asr #32
    4b08:	00000020 	andeq	r0, r0, r0, lsr #32
    4b0c:	00000000 	andeq	r0, r0, r0
    4b10:	000d0044 	andeq	r0, sp, r4, asr #32
    4b14:	00000028 	andeq	r0, r0, r8, lsr #32
    4b18:	00000000 	andeq	r0, r0, r0
    4b1c:	000f0044 	andeq	r0, pc, r4, asr #32
    4b20:	00000030 	andeq	r0, r0, r0, lsr r0
    4b24:	00000000 	andeq	r0, r0, r0
    4b28:	00100044 	andseq	r0, r0, r4, asr #32
    4b2c:	00000040 	andeq	r0, r0, r0, asr #32
    4b30:	00000000 	andeq	r0, r0, r0
    4b34:	00110044 	andseq	r0, r1, r4, asr #32
    4b38:	00000050 	andeq	r0, r0, r0, asr r0
    4b3c:	00000000 	andeq	r0, r0, r0
    4b40:	00140044 	andseq	r0, r4, r4, asr #32
    4b44:	00000068 	andeq	r0, r0, r8, rrx
    4b48:	00000000 	andeq	r0, r0, r0
    4b4c:	00150044 	andseq	r0, r5, r4, asr #32
    4b50:	00000070 	andeq	r0, r0, r0, ror r0
    4b54:	00000000 	andeq	r0, r0, r0
    4b58:	00180044 	andseq	r0, r8, r4, asr #32
    4b5c:	0000007c 	andeq	r0, r0, ip, ror r0
    4b60:	00000000 	andeq	r0, r0, r0
    4b64:	00190044 	andseq	r0, r9, r4, asr #32
    4b68:	00000090 	muleq	r0, r0, r0
    4b6c:	00000000 	andeq	r0, r0, r0
    4b70:	00170044 	andseq	r0, r7, r4, asr #32
    4b74:	0000009c 	muleq	r0, ip, r0
    4b78:	00000000 	andeq	r0, r0, r0
    4b7c:	00150044 	andseq	r0, r5, r4, asr #32
    4b80:	000000a8 	andeq	r0, r0, r8, lsr #1
    4b84:	00000000 	andeq	r0, r0, r0
    4b88:	001c0044 	andseq	r0, ip, r4, asr #32
    4b8c:	000000b8 	strheq	r0, [r0], -r8
    4b90:	00000000 	andeq	r0, r0, r0
    4b94:	001d0044 	andseq	r0, sp, r4, asr #32
    4b98:	000000c0 	andeq	r0, r0, r0, asr #1
    4b9c:	00002800 	andeq	r2, r0, r0, lsl #16
    4ba0:	000c0080 	andeq	r0, ip, r0, lsl #1
    4ba4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4ba8:	00002816 	andeq	r2, r0, r6, lsl r8
    4bac:	000d0080 	andeq	r0, sp, r0, lsl #1
    4bb0:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    4bb4:	0000282b 	andeq	r2, r0, fp, lsr #16
    4bb8:	000f0080 	andeq	r0, pc, r0, lsl #1
    4bbc:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4bc0:	0000283e 	andeq	r2, r0, lr, lsr r8
    4bc4:	00100080 	andseq	r0, r0, r0, lsl #1
    4bc8:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    4bcc:	00002852 	andeq	r2, r0, r2, asr r8
    4bd0:	00110080 	andseq	r0, r1, r0, lsl #1
    4bd4:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    4bd8:	00000000 	andeq	r0, r0, r0
    4bdc:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    4be8:	000000e0 	andeq	r0, r0, r0, ror #1
    4bec:	000000c8 	andeq	r0, r0, r8, asr #1
    4bf0:	00000000 	andeq	r0, r0, r0
    4bf4:	00000024 	andeq	r0, r0, r4, lsr #32
    4bf8:	000000c8 	andeq	r0, r0, r8, asr #1
    4bfc:	00000000 	andeq	r0, r0, r0
    4c00:	0000004e 	andeq	r0, r0, lr, asr #32
    4c04:	80012120 	andhi	r2, r1, r0, lsr #2
    4c08:	00002864 	andeq	r2, r0, r4, ror #16
    4c0c:	00240024 	eoreq	r0, r4, r4, lsr #32
    4c10:	80012120 	andhi	r2, r1, r0, lsr #2
    4c14:	00002874 	andeq	r2, r0, r4, ror r8
    4c18:	002400a0 	eoreq	r0, r4, r0, lsr #1
    4c1c:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    4c20:	0000287f 	andeq	r2, r0, pc, ror r8
    4c24:	002400a0 	eoreq	r0, r4, r0, lsr #1
    4c28:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    4c2c:	00002892 	muleq	r0, r2, r8
    4c30:	002500a0 	eoreq	r0, r5, r0, lsr #1
    4c34:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    4c38:	000027ed 	andeq	r2, r0, sp, ror #15
    4c3c:	002500a0 	eoreq	r0, r5, r0, lsr #1
    4c40:	ffffffd4 			; <UNDEFINED> instruction: 0xffffffd4
    4c44:	00000000 	andeq	r0, r0, r0
    4c48:	0000002e 	andeq	r0, r0, lr, lsr #32
    4c4c:	80012120 	andhi	r2, r1, r0, lsr #2
    4c50:	00000000 	andeq	r0, r0, r0
    4c54:	00260044 	eoreq	r0, r6, r4, asr #32
	...
    4c60:	00270044 	eoreq	r0, r7, r4, asr #32
    4c64:	00000020 	andeq	r0, r0, r0, lsr #32
    4c68:	00000000 	andeq	r0, r0, r0
    4c6c:	00290044 	eoreq	r0, r9, r4, asr #32
    4c70:	00000028 	andeq	r0, r0, r8, lsr #32
    4c74:	00000000 	andeq	r0, r0, r0
    4c78:	002a0044 	eoreq	r0, sl, r4, asr #32
    4c7c:	00000034 	andeq	r0, r0, r4, lsr r0
    4c80:	00000000 	andeq	r0, r0, r0
    4c84:	002d0044 	eoreq	r0, sp, r4, asr #32
    4c88:	00000044 	andeq	r0, r0, r4, asr #32
    4c8c:	00000000 	andeq	r0, r0, r0
    4c90:	002e0044 	eoreq	r0, lr, r4, asr #32
    4c94:	00000068 	andeq	r0, r0, r8, rrx
    4c98:	00000000 	andeq	r0, r0, r0
    4c9c:	002f0044 	eoreq	r0, pc, r4, asr #32
    4ca0:	00000070 	andeq	r0, r0, r0, ror r0
    4ca4:	00000000 	andeq	r0, r0, r0
    4ca8:	00310044 	eorseq	r0, r1, r4, asr #32
    4cac:	00000080 	andeq	r0, r0, r0, lsl #1
    4cb0:	00000000 	andeq	r0, r0, r0
    4cb4:	00320044 	eorseq	r0, r2, r4, asr #32
    4cb8:	000000ac 	andeq	r0, r0, ip, lsr #1
    4cbc:	00000000 	andeq	r0, r0, r0
    4cc0:	00330044 	eorseq	r0, r3, r4, asr #32
    4cc4:	000000cc 	andeq	r0, r0, ip, asr #1
    4cc8:	00000000 	andeq	r0, r0, r0
    4ccc:	00370044 	eorseq	r0, r7, r4, asr #32
    4cd0:	000000ec 	andeq	r0, r0, ip, ror #1
    4cd4:	00000000 	andeq	r0, r0, r0
    4cd8:	003b0044 	eorseq	r0, fp, r4, asr #32
    4cdc:	00000110 	andeq	r0, r0, r0, lsl r1
    4ce0:	00000000 	andeq	r0, r0, r0
    4ce4:	003c0044 	eorseq	r0, ip, r4, asr #32
    4ce8:	00000130 	andeq	r0, r0, r0, lsr r1
    4cec:	00000000 	andeq	r0, r0, r0
    4cf0:	003d0044 	eorseq	r0, sp, r4, asr #32
    4cf4:	0000014c 	andeq	r0, r0, ip, asr #2
    4cf8:	00000000 	andeq	r0, r0, r0
    4cfc:	003e0044 	eorseq	r0, lr, r4, asr #32
    4d00:	00000168 	andeq	r0, r0, r8, ror #2
    4d04:	00000000 	andeq	r0, r0, r0
    4d08:	003f0044 	eorseq	r0, pc, r4, asr #32
    4d0c:	0000018c 	andeq	r0, r0, ip, lsl #3
    4d10:	00000000 	andeq	r0, r0, r0
    4d14:	00400044 	subeq	r0, r0, r4, asr #32
    4d18:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    4d1c:	000028a2 	andeq	r2, r0, r2, lsr #17
    4d20:	00270080 	eoreq	r0, r7, r0, lsl #1
    4d24:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4d28:	000028ba 			; <UNDEFINED> instruction: 0x000028ba
    4d2c:	00290080 	eoreq	r0, r9, r0, lsl #1
    4d30:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    4d34:	000028cd 	andeq	r2, r0, sp, asr #17
    4d38:	002a0080 	eoreq	r0, sl, r0, lsl #1
    4d3c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4d40:	00000000 	andeq	r0, r0, r0
    4d44:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    4d50:	000000e0 	andeq	r0, r0, r0, ror #1
    4d54:	000001bc 			; <UNDEFINED> instruction: 0x000001bc
    4d58:	00000000 	andeq	r0, r0, r0
    4d5c:	00000024 	andeq	r0, r0, r4, lsr #32
    4d60:	000001bc 			; <UNDEFINED> instruction: 0x000001bc
    4d64:	00000000 	andeq	r0, r0, r0
    4d68:	0000004e 	andeq	r0, r0, lr, asr #32
    4d6c:	800122dc 	ldrdhi	r2, [r1], -ip
    4d70:	000028dd 	ldrdeq	r2, [r0], -sp
    4d74:	00430024 	subeq	r0, r3, r4, lsr #32
    4d78:	800122dc 	ldrdhi	r2, [r1], -ip
    4d7c:	00002874 	andeq	r2, r0, r4, ror r8
    4d80:	004300a0 	subeq	r0, r3, r0, lsr #1
    4d84:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    4d88:	0000287f 	andeq	r2, r0, pc, ror r8
    4d8c:	004300a0 	subeq	r0, r3, r0, lsr #1
    4d90:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    4d94:	00000000 	andeq	r0, r0, r0
    4d98:	0000002e 	andeq	r0, r0, lr, lsr #32
    4d9c:	800122dc 	ldrdhi	r2, [r1], -ip
    4da0:	00000000 	andeq	r0, r0, r0
    4da4:	00440044 	subeq	r0, r4, r4, asr #32
	...
    4db0:	00450044 	subeq	r0, r5, r4, asr #32
    4db4:	00000018 	andeq	r0, r0, r8, lsl r0
    4db8:	00000000 	andeq	r0, r0, r0
    4dbc:	00470044 	subeq	r0, r7, r4, asr #32
    4dc0:	00000020 	andeq	r0, r0, r0, lsr #32
    4dc4:	00000000 	andeq	r0, r0, r0
    4dc8:	00480044 	subeq	r0, r8, r4, asr #32
    4dcc:	0000002c 	andeq	r0, r0, ip, lsr #32
    4dd0:	00000000 	andeq	r0, r0, r0
    4dd4:	004a0044 	subeq	r0, sl, r4, asr #32
    4dd8:	0000003c 	andeq	r0, r0, ip, lsr r0
    4ddc:	00000000 	andeq	r0, r0, r0
    4de0:	004b0044 	subeq	r0, fp, r4, asr #32
    4de4:	00000060 	andeq	r0, r0, r0, rrx
    4de8:	00000000 	andeq	r0, r0, r0
    4dec:	004c0044 	subeq	r0, ip, r4, asr #32
    4df0:	0000007c 	andeq	r0, r0, ip, ror r0
    4df4:	000028ef 	andeq	r2, r0, pc, ror #17
    4df8:	00450080 	subeq	r0, r5, r0, lsl #1
    4dfc:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4e00:	000028ba 			; <UNDEFINED> instruction: 0x000028ba
    4e04:	00470080 	subeq	r0, r7, r0, lsl #1
    4e08:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    4e0c:	000028cd 	andeq	r2, r0, sp, asr #17
    4e10:	00480080 	subeq	r0, r8, r0, lsl #1
    4e14:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4e18:	00000000 	andeq	r0, r0, r0
    4e1c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    4e28:	000000e0 	andeq	r0, r0, r0, ror #1
    4e2c:	00000084 	andeq	r0, r0, r4, lsl #1
    4e30:	00000000 	andeq	r0, r0, r0
    4e34:	00000024 	andeq	r0, r0, r4, lsr #32
    4e38:	00000084 	andeq	r0, r0, r4, lsl #1
    4e3c:	00000000 	andeq	r0, r0, r0
    4e40:	0000004e 	andeq	r0, r0, lr, asr #32
    4e44:	80012360 	andhi	r2, r1, r0, ror #6
    4e48:	00002900 	andeq	r2, r0, r0, lsl #18
    4e4c:	00530024 	subseq	r0, r3, r4, lsr #32
    4e50:	80012360 	andhi	r2, r1, r0, ror #6
    4e54:	00002874 	andeq	r2, r0, r4, ror r8
    4e58:	005300a0 	subseq	r0, r3, r0, lsr #1
    4e5c:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    4e60:	00002919 	andeq	r2, r0, r9, lsl r9
    4e64:	005300a0 	subseq	r0, r3, r0, lsr #1
    4e68:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    4e6c:	00000000 	andeq	r0, r0, r0
    4e70:	0000002e 	andeq	r0, r0, lr, lsr #32
    4e74:	80012360 	andhi	r2, r1, r0, ror #6
    4e78:	00000000 	andeq	r0, r0, r0
    4e7c:	00540044 	subseq	r0, r4, r4, asr #32
	...
    4e88:	00550044 	subseq	r0, r5, r4, asr #32
    4e8c:	00000018 	andeq	r0, r0, r8, lsl r0
    4e90:	00000000 	andeq	r0, r0, r0
    4e94:	00560044 	subseq	r0, r6, r4, asr #32
    4e98:	00000020 	andeq	r0, r0, r0, lsr #32
    4e9c:	00000000 	andeq	r0, r0, r0
    4ea0:	00570044 	subseq	r0, r7, r4, asr #32
    4ea4:	00000028 	andeq	r0, r0, r8, lsr #32
    4ea8:	00000000 	andeq	r0, r0, r0
    4eac:	00580044 	subseq	r0, r8, r4, asr #32
    4eb0:	00000030 	andeq	r0, r0, r0, lsr r0
    4eb4:	00000000 	andeq	r0, r0, r0
    4eb8:	005a0044 	subseq	r0, sl, r4, asr #32
    4ebc:	00000038 	andeq	r0, r0, r8, lsr r0
    4ec0:	00000000 	andeq	r0, r0, r0
    4ec4:	005b0044 	subseq	r0, fp, r4, asr #32
    4ec8:	00000050 	andeq	r0, r0, r0, asr r0
    4ecc:	00000000 	andeq	r0, r0, r0
    4ed0:	005c0044 	subseq	r0, ip, r4, asr #32
    4ed4:	00000064 	andeq	r0, r0, r4, rrx
    4ed8:	00000000 	andeq	r0, r0, r0
    4edc:	005d0044 	subseq	r0, sp, r4, asr #32
    4ee0:	0000007c 	andeq	r0, r0, ip, ror r0
    4ee4:	00000000 	andeq	r0, r0, r0
    4ee8:	005e0044 	subseq	r0, lr, r4, asr #32
    4eec:	00000088 	andeq	r0, r0, r8, lsl #1
    4ef0:	00000000 	andeq	r0, r0, r0
    4ef4:	00600044 	rsbeq	r0, r0, r4, asr #32
    4ef8:	000000ac 	andeq	r0, r0, ip, lsr #1
    4efc:	00000000 	andeq	r0, r0, r0
    4f00:	00610044 	rsbeq	r0, r1, r4, asr #32
    4f04:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    4f08:	00002928 	andeq	r2, r0, r8, lsr #18
    4f0c:	00550080 	subseq	r0, r5, r0, lsl #1
    4f10:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4f14:	00002934 	andeq	r2, r0, r4, lsr r9
    4f18:	00560080 	subseq	r0, r6, r0, lsl #1
    4f1c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    4f20:	00002940 	andeq	r2, r0, r0, asr #18
    4f24:	00570080 	subseq	r0, r7, r0, lsl #1
    4f28:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4f2c:	0000294d 	andeq	r2, r0, sp, asr #18
    4f30:	00580080 	subseq	r0, r8, r0, lsl #1
    4f34:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    4f38:	00000000 	andeq	r0, r0, r0
    4f3c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    4f48:	000000e0 	andeq	r0, r0, r0, ror #1
    4f4c:	000000bc 	strheq	r0, [r0], -ip
    4f50:	00000000 	andeq	r0, r0, r0
    4f54:	00000024 	andeq	r0, r0, r4, lsr #32
    4f58:	000000bc 	strheq	r0, [r0], -ip
    4f5c:	00000000 	andeq	r0, r0, r0
    4f60:	0000004e 	andeq	r0, r0, lr, asr #32
    4f64:	8001241c 	andhi	r2, r1, ip, lsl r4
    4f68:	00002968 	andeq	r2, r0, r8, ror #18
    4f6c:	00630024 	rsbeq	r0, r3, r4, lsr #32
    4f70:	8001241c 	andhi	r2, r1, ip, lsl r4
    4f74:	00002874 	andeq	r2, r0, r4, ror r8
    4f78:	006300a0 	rsbeq	r0, r3, r0, lsr #1
    4f7c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    4f80:	00000000 	andeq	r0, r0, r0
    4f84:	0000002e 	andeq	r0, r0, lr, lsr #32
    4f88:	8001241c 	andhi	r2, r1, ip, lsl r4
    4f8c:	00000000 	andeq	r0, r0, r0
    4f90:	00640044 	rsbeq	r0, r4, r4, asr #32
	...
    4f9c:	00650044 	rsbeq	r0, r5, r4, asr #32
    4fa0:	00000014 	andeq	r0, r0, r4, lsl r0
    4fa4:	00000000 	andeq	r0, r0, r0
    4fa8:	00660044 	rsbeq	r0, r6, r4, asr #32
    4fac:	00000020 	andeq	r0, r0, r0, lsr #32
    4fb0:	00000000 	andeq	r0, r0, r0
    4fb4:	00670044 	rsbeq	r0, r7, r4, asr #32
    4fb8:	00000044 	andeq	r0, r0, r4, asr #32
    4fbc:	00000000 	andeq	r0, r0, r0
    4fc0:	00680044 	rsbeq	r0, r8, r4, asr #32
    4fc4:	00000068 	andeq	r0, r0, r8, rrx
    4fc8:	00000000 	andeq	r0, r0, r0
    4fcc:	00650044 	rsbeq	r0, r5, r4, asr #32
    4fd0:	00000070 	andeq	r0, r0, r0, ror r0
    4fd4:	00000000 	andeq	r0, r0, r0
    4fd8:	00650044 	rsbeq	r0, r5, r4, asr #32
    4fdc:	0000007c 	andeq	r0, r0, ip, ror r0
    4fe0:	00000000 	andeq	r0, r0, r0
    4fe4:	006b0044 	rsbeq	r0, fp, r4, asr #32
    4fe8:	00000088 	andeq	r0, r0, r8, lsl #1
    4fec:	00000ecb 	andeq	r0, r0, fp, asr #29
    4ff0:	00650080 	rsbeq	r0, r5, r0, lsl #1
    4ff4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    4ff8:	00000000 	andeq	r0, r0, r0
    4ffc:	000000c0 	andeq	r0, r0, r0, asr #1
    5000:	00000014 	andeq	r0, r0, r4, lsl r0
    5004:	0000297f 	andeq	r2, r0, pc, ror r9
    5008:	00670080 	rsbeq	r0, r7, r0, lsl #1
    500c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5010:	00000000 	andeq	r0, r0, r0
    5014:	000000c0 	andeq	r0, r0, r0, asr #1
    5018:	00000044 	andeq	r0, r0, r4, asr #32
    501c:	00000000 	andeq	r0, r0, r0
    5020:	000000e0 	andeq	r0, r0, r0, ror #1
    5024:	00000070 	andeq	r0, r0, r0, ror r0
    5028:	00000000 	andeq	r0, r0, r0
    502c:	000000e0 	andeq	r0, r0, r0, ror #1
    5030:	00000088 	andeq	r0, r0, r8, lsl #1
    5034:	00000000 	andeq	r0, r0, r0
    5038:	00000024 	andeq	r0, r0, r4, lsr #32
    503c:	00000090 	muleq	r0, r0, r0
    5040:	00000000 	andeq	r0, r0, r0
    5044:	0000004e 	andeq	r0, r0, lr, asr #32
    5048:	800124ac 	andhi	r2, r1, ip, lsr #9
    504c:	00000000 	andeq	r0, r0, r0
    5050:	00000064 	andeq	r0, r0, r4, rrx
    5054:	800124ac 	andhi	r2, r1, ip, lsr #9
    5058:	00000007 	andeq	r0, r0, r7
    505c:	00020064 	andeq	r0, r2, r4, rrx
    5060:	800124ac 	andhi	r2, r1, ip, lsr #9
    5064:	00002990 	muleq	r0, r0, r9
    5068:	00020064 	andeq	r0, r2, r4, rrx
    506c:	800124ac 	andhi	r2, r1, ip, lsr #9
    5070:	0000003d 	andeq	r0, r0, sp, lsr r0
    5074:	0000003c 	andeq	r0, r0, ip, lsr r0
    5078:	00000000 	andeq	r0, r0, r0
    507c:	0000004c 	andeq	r0, r0, ip, asr #32
    5080:	00000080 	andeq	r0, r0, r0, lsl #1
    5084:	00000000 	andeq	r0, r0, r0
    5088:	00000076 	andeq	r0, r0, r6, ror r0
    508c:	00000080 	andeq	r0, r0, r0, lsl #1
    5090:	00000000 	andeq	r0, r0, r0
    5094:	00000094 	muleq	r0, r4, r0
    5098:	00000080 	andeq	r0, r0, r0, lsl #1
    509c:	00000000 	andeq	r0, r0, r0
    50a0:	000000c3 	andeq	r0, r0, r3, asr #1
    50a4:	00000080 	andeq	r0, r0, r0, lsl #1
    50a8:	00000000 	andeq	r0, r0, r0
    50ac:	000000ee 	andeq	r0, r0, lr, ror #1
    50b0:	00000080 	andeq	r0, r0, r0, lsl #1
    50b4:	00000000 	andeq	r0, r0, r0
    50b8:	0000011e 	andeq	r0, r0, lr, lsl r1
    50bc:	00000080 	andeq	r0, r0, r0, lsl #1
    50c0:	00000000 	andeq	r0, r0, r0
    50c4:	0000016f 	andeq	r0, r0, pc, ror #2
    50c8:	00000080 	andeq	r0, r0, r0, lsl #1
    50cc:	00000000 	andeq	r0, r0, r0
    50d0:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    50d4:	00000080 	andeq	r0, r0, r0, lsl #1
    50d8:	00000000 	andeq	r0, r0, r0
    50dc:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    50e0:	00000080 	andeq	r0, r0, r0, lsl #1
    50e4:	00000000 	andeq	r0, r0, r0
    50e8:	0000020e 	andeq	r0, r0, lr, lsl #4
    50ec:	00000080 	andeq	r0, r0, r0, lsl #1
    50f0:	00000000 	andeq	r0, r0, r0
    50f4:	00000238 	andeq	r0, r0, r8, lsr r2
    50f8:	00000080 	andeq	r0, r0, r0, lsl #1
    50fc:	00000000 	andeq	r0, r0, r0
    5100:	00000261 	andeq	r0, r0, r1, ror #4
    5104:	00000080 	andeq	r0, r0, r0, lsl #1
    5108:	00000000 	andeq	r0, r0, r0
    510c:	0000027b 	andeq	r0, r0, fp, ror r2
    5110:	00000080 	andeq	r0, r0, r0, lsl #1
    5114:	00000000 	andeq	r0, r0, r0
    5118:	00000296 	muleq	r0, r6, r2
    511c:	00000080 	andeq	r0, r0, r0, lsl #1
    5120:	00000000 	andeq	r0, r0, r0
    5124:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    5128:	00000080 	andeq	r0, r0, r0, lsl #1
    512c:	00000000 	andeq	r0, r0, r0
    5130:	000002d7 	ldrdeq	r0, [r0], -r7
    5134:	00000080 	andeq	r0, r0, r0, lsl #1
    5138:	00000000 	andeq	r0, r0, r0
    513c:	000002f7 	strdeq	r0, [r0], -r7
    5140:	00000080 	andeq	r0, r0, r0, lsl #1
    5144:	00000000 	andeq	r0, r0, r0
    5148:	0000031c 	andeq	r0, r0, ip, lsl r3
    514c:	00000080 	andeq	r0, r0, r0, lsl #1
    5150:	00000000 	andeq	r0, r0, r0
    5154:	00000346 	andeq	r0, r0, r6, asr #6
    5158:	00000080 	andeq	r0, r0, r0, lsl #1
    515c:	00000000 	andeq	r0, r0, r0
    5160:	0000036a 	andeq	r0, r0, sl, ror #6
    5164:	00000080 	andeq	r0, r0, r0, lsl #1
    5168:	00000000 	andeq	r0, r0, r0
    516c:	00000393 	muleq	r0, r3, r3
    5170:	00000080 	andeq	r0, r0, r0, lsl #1
    5174:	00000000 	andeq	r0, r0, r0
    5178:	000003c1 	andeq	r0, r0, r1, asr #7
    517c:	00000080 	andeq	r0, r0, r0, lsl #1
    5180:	00000000 	andeq	r0, r0, r0
    5184:	000003e7 	andeq	r0, r0, r7, ror #7
    5188:	00000080 	andeq	r0, r0, r0, lsl #1
    518c:	00000000 	andeq	r0, r0, r0
    5190:	00000407 	andeq	r0, r0, r7, lsl #8
    5194:	00000080 	andeq	r0, r0, r0, lsl #1
    5198:	00000000 	andeq	r0, r0, r0
    519c:	0000042c 	andeq	r0, r0, ip, lsr #8
    51a0:	00000080 	andeq	r0, r0, r0, lsl #1
    51a4:	00000000 	andeq	r0, r0, r0
    51a8:	00000456 	andeq	r0, r0, r6, asr r4
    51ac:	00000080 	andeq	r0, r0, r0, lsl #1
    51b0:	00000000 	andeq	r0, r0, r0
    51b4:	00000485 	andeq	r0, r0, r5, lsl #9
    51b8:	00000080 	andeq	r0, r0, r0, lsl #1
    51bc:	00000000 	andeq	r0, r0, r0
    51c0:	000004ae 	andeq	r0, r0, lr, lsr #9
    51c4:	00000080 	andeq	r0, r0, r0, lsl #1
    51c8:	00000000 	andeq	r0, r0, r0
    51cc:	000004dc 	ldrdeq	r0, [r0], -ip
    51d0:	00000080 	andeq	r0, r0, r0, lsl #1
    51d4:	00000000 	andeq	r0, r0, r0
    51d8:	0000050f 	andeq	r0, r0, pc, lsl #10
    51dc:	00000080 	andeq	r0, r0, r0, lsl #1
    51e0:	00000000 	andeq	r0, r0, r0
    51e4:	00000530 	andeq	r0, r0, r0, lsr r5
    51e8:	00000080 	andeq	r0, r0, r0, lsl #1
    51ec:	00000000 	andeq	r0, r0, r0
    51f0:	00000550 	andeq	r0, r0, r0, asr r5
    51f4:	00000080 	andeq	r0, r0, r0, lsl #1
    51f8:	00000000 	andeq	r0, r0, r0
    51fc:	00000575 	andeq	r0, r0, r5, ror r5
    5200:	00000080 	andeq	r0, r0, r0, lsl #1
    5204:	00000000 	andeq	r0, r0, r0
    5208:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    520c:	00000080 	andeq	r0, r0, r0, lsl #1
    5210:	00000000 	andeq	r0, r0, r0
    5214:	000005c3 	andeq	r0, r0, r3, asr #11
    5218:	00000080 	andeq	r0, r0, r0, lsl #1
    521c:	00000000 	andeq	r0, r0, r0
    5220:	000005ec 	andeq	r0, r0, ip, ror #11
    5224:	00000080 	andeq	r0, r0, r0, lsl #1
    5228:	00000000 	andeq	r0, r0, r0
    522c:	0000061a 	andeq	r0, r0, sl, lsl r6
    5230:	00000080 	andeq	r0, r0, r0, lsl #1
    5234:	00000000 	andeq	r0, r0, r0
    5238:	00000640 	andeq	r0, r0, r0, asr #12
    523c:	00000080 	andeq	r0, r0, r0, lsl #1
    5240:	00000000 	andeq	r0, r0, r0
    5244:	00000660 	andeq	r0, r0, r0, ror #12
    5248:	00000080 	andeq	r0, r0, r0, lsl #1
    524c:	00000000 	andeq	r0, r0, r0
    5250:	00000685 	andeq	r0, r0, r5, lsl #13
    5254:	00000080 	andeq	r0, r0, r0, lsl #1
    5258:	00000000 	andeq	r0, r0, r0
    525c:	000006af 	andeq	r0, r0, pc, lsr #13
    5260:	00000080 	andeq	r0, r0, r0, lsl #1
    5264:	00000000 	andeq	r0, r0, r0
    5268:	000006de 	ldrdeq	r0, [r0], -lr
    526c:	00000080 	andeq	r0, r0, r0, lsl #1
    5270:	00000000 	andeq	r0, r0, r0
    5274:	00000707 	andeq	r0, r0, r7, lsl #14
    5278:	00000080 	andeq	r0, r0, r0, lsl #1
    527c:	00000000 	andeq	r0, r0, r0
    5280:	00000735 	andeq	r0, r0, r5, lsr r7
    5284:	00000080 	andeq	r0, r0, r0, lsl #1
    5288:	00000000 	andeq	r0, r0, r0
    528c:	00000768 	andeq	r0, r0, r8, ror #14
    5290:	00000080 	andeq	r0, r0, r0, lsl #1
    5294:	00000000 	andeq	r0, r0, r0
    5298:	00001000 	andeq	r1, r0, r0
    529c:	000000c2 	andeq	r0, r0, r2, asr #1
    52a0:	0000100f 	andeq	r1, r0, pc
    52a4:	0000078a 	andeq	r0, r0, sl, lsl #15
    52a8:	000000c2 	andeq	r0, r0, r2, asr #1
    52ac:	00002d60 	andeq	r2, r0, r0, ror #26
    52b0:	00000886 	andeq	r0, r0, r6, lsl #17
    52b4:	000000c2 	andeq	r0, r0, r2, asr #1
    52b8:	00003ac8 	andeq	r3, r0, r8, asr #21
    52bc:	000029a0 	andeq	r2, r0, r0, lsr #19
    52c0:	00130024 	andseq	r0, r3, r4, lsr #32
    52c4:	800124ac 	andhi	r2, r1, ip, lsr #9
    52c8:	000029b3 			; <UNDEFINED> instruction: 0x000029b3
    52cc:	001300a0 	andseq	r0, r3, r0, lsr #1
    52d0:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    52d4:	000029c0 	andeq	r2, r0, r0, asr #19
    52d8:	001300a0 	andseq	r0, r3, r0, lsr #1
    52dc:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    52e0:	00000000 	andeq	r0, r0, r0
    52e4:	0000002e 	andeq	r0, r0, lr, lsr #32
    52e8:	800124ac 	andhi	r2, r1, ip, lsr #9
    52ec:	00000000 	andeq	r0, r0, r0
    52f0:	00140044 	andseq	r0, r4, r4, asr #32
	...
    52fc:	00150044 	andseq	r0, r5, r4, asr #32
    5300:	00000018 	andeq	r0, r0, r8, lsl r0
    5304:	00000000 	andeq	r0, r0, r0
    5308:	00160044 	andseq	r0, r6, r4, asr #32
    530c:	00000030 	andeq	r0, r0, r0, lsr r0
    5310:	00000000 	andeq	r0, r0, r0
    5314:	00170044 	andseq	r0, r7, r4, asr #32
    5318:	00000040 	andeq	r0, r0, r0, asr #32
    531c:	00000000 	andeq	r0, r0, r0
    5320:	001d0044 	andseq	r0, sp, r4, asr #32
    5324:	00000048 	andeq	r0, r0, r8, asr #32
    5328:	00000000 	andeq	r0, r0, r0
    532c:	00200044 	eoreq	r0, r0, r4, asr #32
    5330:	00000054 	andeq	r0, r0, r4, asr r0
    5334:	00000000 	andeq	r0, r0, r0
    5338:	001e0044 	andseq	r0, lr, r4, asr #32
    533c:	0000007c 	andeq	r0, r0, ip, ror r0
    5340:	00000000 	andeq	r0, r0, r0
    5344:	001d0044 	andseq	r0, sp, r4, asr #32
    5348:	00000088 	andeq	r0, r0, r8, lsl #1
    534c:	00000000 	andeq	r0, r0, r0
    5350:	00220044 	eoreq	r0, r2, r4, asr #32
    5354:	00000098 	muleq	r0, r8, r0
    5358:	000029cb 	andeq	r2, r0, fp, asr #19
    535c:	00150080 	andseq	r0, r5, r0, lsl #1
    5360:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5364:	000029e6 	andeq	r2, r0, r6, ror #19
    5368:	00160080 	andseq	r0, r6, r0, lsl #1
    536c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    5370:	000029f8 	strdeq	r2, [r0], -r8
    5374:	00170080 	andseq	r0, r7, r0, lsl #1
    5378:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    537c:	00000000 	andeq	r0, r0, r0
    5380:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    538c:	000000e0 	andeq	r0, r0, r0, ror #1
    5390:	000000a8 	andeq	r0, r0, r8, lsr #1
    5394:	00000000 	andeq	r0, r0, r0
    5398:	00000024 	andeq	r0, r0, r4, lsr #32
    539c:	000000a8 	andeq	r0, r0, r8, lsr #1
    53a0:	00000000 	andeq	r0, r0, r0
    53a4:	0000004e 	andeq	r0, r0, lr, asr #32
    53a8:	80012554 	andhi	r2, r1, r4, asr r5
    53ac:	00002a0b 	andeq	r2, r0, fp, lsl #20
    53b0:	00250024 	eoreq	r0, r5, r4, lsr #32
    53b4:	80012554 	andhi	r2, r1, r4, asr r5
    53b8:	00000000 	andeq	r0, r0, r0
    53bc:	0000002e 	andeq	r0, r0, lr, lsr #32
    53c0:	80012554 	andhi	r2, r1, r4, asr r5
    53c4:	00000000 	andeq	r0, r0, r0
    53c8:	00260044 	eoreq	r0, r6, r4, asr #32
	...
    53d4:	00270044 	eoreq	r0, r7, r4, asr #32
    53d8:	00000010 	andeq	r0, r0, r0, lsl r0
    53dc:	00000000 	andeq	r0, r0, r0
    53e0:	00290044 	eoreq	r0, r9, r4, asr #32
    53e4:	00000018 	andeq	r0, r0, r8, lsl r0
    53e8:	00000000 	andeq	r0, r0, r0
    53ec:	002a0044 	eoreq	r0, sl, r4, asr #32
    53f0:	0000002c 	andeq	r0, r0, ip, lsr #32
    53f4:	00000000 	andeq	r0, r0, r0
    53f8:	002b0044 	eoreq	r0, fp, r4, asr #32
    53fc:	0000003c 	andeq	r0, r0, ip, lsr r0
    5400:	00000000 	andeq	r0, r0, r0
    5404:	002e0044 	eoreq	r0, lr, r4, asr #32
    5408:	00000058 	andeq	r0, r0, r8, asr r0
    540c:	00000000 	andeq	r0, r0, r0
    5410:	002f0044 	eoreq	r0, pc, r4, asr #32
    5414:	0000005c 	andeq	r0, r0, ip, asr r0
    5418:	00002a22 	andeq	r2, r0, r2, lsr #20
    541c:	00270080 	eoreq	r0, r7, r0, lsl #1
    5420:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    5424:	00000000 	andeq	r0, r0, r0
    5428:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    5434:	000000e0 	andeq	r0, r0, r0, ror #1
    5438:	00000078 	andeq	r0, r0, r8, ror r0
    543c:	00000000 	andeq	r0, r0, r0
    5440:	00000024 	andeq	r0, r0, r4, lsr #32
    5444:	00000078 	andeq	r0, r0, r8, ror r0
    5448:	00000000 	andeq	r0, r0, r0
    544c:	0000004e 	andeq	r0, r0, lr, asr #32
    5450:	800125cc 	andhi	r2, r1, ip, asr #11
    5454:	00002a30 	andeq	r2, r0, r0, lsr sl
    5458:	00320024 	eorseq	r0, r2, r4, lsr #32
    545c:	800125cc 	andhi	r2, r1, ip, asr #11
    5460:	00002a3e 	andeq	r2, r0, lr, lsr sl
    5464:	003200a0 	eorseq	r0, r2, r0, lsr #1
    5468:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    546c:	00000000 	andeq	r0, r0, r0
    5470:	0000002e 	andeq	r0, r0, lr, lsr #32
    5474:	800125cc 	andhi	r2, r1, ip, asr #11
    5478:	00000000 	andeq	r0, r0, r0
    547c:	00330044 	eorseq	r0, r3, r4, asr #32
	...
    5488:	00340044 	eorseq	r0, r4, r4, asr #32
    548c:	00000014 	andeq	r0, r0, r4, lsl r0
    5490:	00000000 	andeq	r0, r0, r0
    5494:	00350044 	eorseq	r0, r5, r4, asr #32
    5498:	0000003c 	andeq	r0, r0, ip, lsr r0
    549c:	00000000 	andeq	r0, r0, r0
    54a0:	00000024 	andeq	r0, r0, r4, lsr #32
    54a4:	0000004c 	andeq	r0, r0, ip, asr #32
    54a8:	00000000 	andeq	r0, r0, r0
    54ac:	0000004e 	andeq	r0, r0, lr, asr #32
    54b0:	80012618 	andhi	r2, r1, r8, lsl r6
    54b4:	00002a4b 	andeq	r2, r0, fp, asr #20
    54b8:	00380024 	eorseq	r0, r8, r4, lsr #32
    54bc:	80012618 	andhi	r2, r1, r8, lsl r6
    54c0:	00000000 	andeq	r0, r0, r0
    54c4:	0000002e 	andeq	r0, r0, lr, lsr #32
    54c8:	80012618 	andhi	r2, r1, r8, lsl r6
    54cc:	00000000 	andeq	r0, r0, r0
    54d0:	00390044 	eorseq	r0, r9, r4, asr #32
	...
    54dc:	003a0044 	eorseq	r0, sl, r4, asr #32
    54e0:	00000010 	andeq	r0, r0, r0, lsl r0
    54e4:	00000000 	andeq	r0, r0, r0
    54e8:	00400044 	subeq	r0, r0, r4, asr #32
    54ec:	00000018 	andeq	r0, r0, r8, lsl r0
    54f0:	00000000 	andeq	r0, r0, r0
    54f4:	00410044 	subeq	r0, r1, r4, asr #32
    54f8:	0000002c 	andeq	r0, r0, ip, lsr #32
    54fc:	00000000 	andeq	r0, r0, r0
    5500:	00420044 	subeq	r0, r2, r4, asr #32
    5504:	00000034 	andeq	r0, r0, r4, lsr r0
    5508:	00000000 	andeq	r0, r0, r0
    550c:	00430044 	subeq	r0, r3, r4, asr #32
    5510:	00000040 	andeq	r0, r0, r0, asr #32
    5514:	00000000 	andeq	r0, r0, r0
    5518:	00440044 	subeq	r0, r4, r4, asr #32
    551c:	00000048 	andeq	r0, r0, r8, asr #32
    5520:	00000000 	andeq	r0, r0, r0
    5524:	00450044 	subeq	r0, r5, r4, asr #32
    5528:	00000058 	andeq	r0, r0, r8, asr r0
    552c:	00000000 	andeq	r0, r0, r0
    5530:	00460044 	subeq	r0, r6, r4, asr #32
    5534:	00000068 	andeq	r0, r0, r8, rrx
    5538:	00000000 	andeq	r0, r0, r0
    553c:	004a0044 	subeq	r0, sl, r4, asr #32
    5540:	00000078 	andeq	r0, r0, r8, ror r0
    5544:	00000000 	andeq	r0, r0, r0
    5548:	004b0044 	subeq	r0, fp, r4, asr #32
    554c:	0000008c 	andeq	r0, r0, ip, lsl #1
    5550:	00000000 	andeq	r0, r0, r0
    5554:	004c0044 	subeq	r0, ip, r4, asr #32
    5558:	0000009c 	muleq	r0, ip, r0
    555c:	00000000 	andeq	r0, r0, r0
    5560:	004f0044 	subeq	r0, pc, r4, asr #32
    5564:	000000b8 	strheq	r0, [r0], -r8
    5568:	00000000 	andeq	r0, r0, r0
    556c:	00500044 	subseq	r0, r0, r4, asr #32
    5570:	000000bc 	strheq	r0, [r0], -ip
    5574:	00002a22 	andeq	r2, r0, r2, lsr #20
    5578:	003a0080 	eorseq	r0, sl, r0, lsl #1
    557c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    5580:	00000000 	andeq	r0, r0, r0
    5584:	000000c0 	andeq	r0, r0, r0, asr #1
    5588:	00000000 	andeq	r0, r0, r0
    558c:	00002a5c 	andeq	r2, r0, ip, asr sl
    5590:	00410080 	subeq	r0, r1, r0, lsl #1
    5594:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5598:	00000000 	andeq	r0, r0, r0
    559c:	000000c0 	andeq	r0, r0, r0, asr #1
    55a0:	0000002c 	andeq	r0, r0, ip, lsr #32
    55a4:	00000000 	andeq	r0, r0, r0
    55a8:	000000e0 	andeq	r0, r0, r0, ror #1
    55ac:	00000078 	andeq	r0, r0, r8, ror r0
    55b0:	00000000 	andeq	r0, r0, r0
    55b4:	000000e0 	andeq	r0, r0, r0, ror #1
    55b8:	000000dc 	ldrdeq	r0, [r0], -ip
    55bc:	00000000 	andeq	r0, r0, r0
    55c0:	00000024 	andeq	r0, r0, r4, lsr #32
    55c4:	000000dc 	ldrdeq	r0, [r0], -ip
    55c8:	00000000 	andeq	r0, r0, r0
    55cc:	0000004e 	andeq	r0, r0, lr, asr #32
    55d0:	800126f4 	strdhi	r2, [r1], -r4
    55d4:	00002a68 	andeq	r2, r0, r8, ror #20
    55d8:	00530024 	subseq	r0, r3, r4, lsr #32
    55dc:	800126f4 	strdhi	r2, [r1], -r4
    55e0:	00002a78 	andeq	r2, r0, r8, ror sl
    55e4:	005300a0 	subseq	r0, r3, r0, lsr #1
    55e8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    55ec:	00000000 	andeq	r0, r0, r0
    55f0:	0000002e 	andeq	r0, r0, lr, lsr #32
    55f4:	800126f4 	strdhi	r2, [r1], -r4
    55f8:	00000000 	andeq	r0, r0, r0
    55fc:	00540044 	subseq	r0, r4, r4, asr #32
	...
    5608:	00550044 	subseq	r0, r5, r4, asr #32
    560c:	00000014 	andeq	r0, r0, r4, lsl r0
    5610:	00000000 	andeq	r0, r0, r0
    5614:	00560044 	subseq	r0, r6, r4, asr #32
    5618:	0000003c 	andeq	r0, r0, ip, lsr r0
    561c:	00000000 	andeq	r0, r0, r0
    5620:	00000024 	andeq	r0, r0, r4, lsr #32
    5624:	0000004c 	andeq	r0, r0, ip, asr #32
    5628:	00000000 	andeq	r0, r0, r0
    562c:	0000004e 	andeq	r0, r0, lr, asr #32
    5630:	80012740 	andhi	r2, r1, r0, asr #14
    5634:	00002a84 	andeq	r2, r0, r4, lsl #21
    5638:	005c0024 	subseq	r0, ip, r4, lsr #32
    563c:	80012740 	andhi	r2, r1, r0, asr #14
    5640:	00000000 	andeq	r0, r0, r0
    5644:	0000002e 	andeq	r0, r0, lr, lsr #32
    5648:	80012740 	andhi	r2, r1, r0, asr #14
    564c:	00000000 	andeq	r0, r0, r0
    5650:	005c0044 	subseq	r0, ip, r4, asr #32
	...
    565c:	005d0044 	subseq	r0, sp, r4, asr #32
    5660:	00000010 	andeq	r0, r0, r0, lsl r0
    5664:	00000000 	andeq	r0, r0, r0
    5668:	005e0044 	subseq	r0, lr, r4, asr #32
    566c:	00000018 	andeq	r0, r0, r8, lsl r0
    5670:	00000000 	andeq	r0, r0, r0
    5674:	00610044 	rsbeq	r0, r1, r4, asr #32
    5678:	00000020 	andeq	r0, r0, r0, lsr #32
    567c:	00000000 	andeq	r0, r0, r0
    5680:	00620044 	rsbeq	r0, r2, r4, asr #32
    5684:	00000030 	andeq	r0, r0, r0, lsr r0
    5688:	00000000 	andeq	r0, r0, r0
    568c:	00630044 	rsbeq	r0, r3, r4, asr #32
    5690:	00000034 	andeq	r0, r0, r4, lsr r0
    5694:	00000000 	andeq	r0, r0, r0
    5698:	00640044 	rsbeq	r0, r4, r4, asr #32
    569c:	00000040 	andeq	r0, r0, r0, asr #32
    56a0:	00000000 	andeq	r0, r0, r0
    56a4:	00620044 	rsbeq	r0, r2, r4, asr #32
    56a8:	0000004c 	andeq	r0, r0, ip, asr #32
    56ac:	00000000 	andeq	r0, r0, r0
    56b0:	00680044 	rsbeq	r0, r8, r4, asr #32
    56b4:	00000058 	andeq	r0, r0, r8, asr r0
    56b8:	00000000 	andeq	r0, r0, r0
    56bc:	00690044 	rsbeq	r0, r9, r4, asr #32
    56c0:	00000068 	andeq	r0, r0, r8, rrx
    56c4:	00000000 	andeq	r0, r0, r0
    56c8:	006a0044 	rsbeq	r0, sl, r4, asr #32
    56cc:	0000006c 	andeq	r0, r0, ip, rrx
    56d0:	00000000 	andeq	r0, r0, r0
    56d4:	006b0044 	rsbeq	r0, fp, r4, asr #32
    56d8:	00000078 	andeq	r0, r0, r8, ror r0
    56dc:	00000000 	andeq	r0, r0, r0
    56e0:	00690044 	rsbeq	r0, r9, r4, asr #32
    56e4:	00000084 	andeq	r0, r0, r4, lsl #1
    56e8:	00000000 	andeq	r0, r0, r0
    56ec:	006e0044 	rsbeq	r0, lr, r4, asr #32
    56f0:	00000090 	muleq	r0, r0, r0
    56f4:	00000000 	andeq	r0, r0, r0
    56f8:	006f0044 	rsbeq	r0, pc, r4, asr #32
    56fc:	00000094 	muleq	r0, r4, r0
    5700:	00002940 	andeq	r2, r0, r0, asr #18
    5704:	005d0080 	subseq	r0, sp, r0, lsl #1
    5708:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    570c:	00002a9d 	muleq	r0, sp, sl
    5710:	005e0080 	subseq	r0, lr, r0, lsl #1
    5714:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5718:	00000000 	andeq	r0, r0, r0
    571c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    5728:	000000e0 	andeq	r0, r0, r0, ror #1
    572c:	000000a8 	andeq	r0, r0, r8, lsr #1
    5730:	00000000 	andeq	r0, r0, r0
    5734:	00000024 	andeq	r0, r0, r4, lsr #32
    5738:	000000a8 	andeq	r0, r0, r8, lsr #1
    573c:	00000000 	andeq	r0, r0, r0
    5740:	0000004e 	andeq	r0, r0, lr, asr #32
    5744:	800127e8 	andhi	r2, r1, r8, ror #15
    5748:	00002ab7 			; <UNDEFINED> instruction: 0x00002ab7
    574c:	00750024 	rsbseq	r0, r5, r4, lsr #32
    5750:	800127e8 	andhi	r2, r1, r8, ror #15
    5754:	00002acf 	andeq	r2, r0, pc, asr #21
    5758:	007500a0 	rsbseq	r0, r5, r0, lsr #1
    575c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    5760:	00002ae0 	andeq	r2, r0, r0, ror #21
    5764:	007600a0 	rsbseq	r0, r6, r0, lsr #1
    5768:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    576c:	00000000 	andeq	r0, r0, r0
    5770:	0000002e 	andeq	r0, r0, lr, lsr #32
    5774:	800127e8 	andhi	r2, r1, r8, ror #15
    5778:	00000000 	andeq	r0, r0, r0
    577c:	00770044 	rsbseq	r0, r7, r4, asr #32
	...
    5788:	00780044 	rsbseq	r0, r8, r4, asr #32
    578c:	00000018 	andeq	r0, r0, r8, lsl r0
    5790:	00000000 	andeq	r0, r0, r0
    5794:	00790044 	rsbseq	r0, r9, r4, asr #32
    5798:	00000020 	andeq	r0, r0, r0, lsr #32
    579c:	00000000 	andeq	r0, r0, r0
    57a0:	007b0044 	rsbseq	r0, fp, r4, asr #32
    57a4:	0000002c 	andeq	r0, r0, ip, lsr #32
    57a8:	00000000 	andeq	r0, r0, r0
    57ac:	007c0044 	rsbseq	r0, ip, r4, asr #32
    57b0:	00000030 	andeq	r0, r0, r0, lsr r0
    57b4:	00002af4 	strdeq	r2, [r0], -r4
    57b8:	00780080 	rsbseq	r0, r8, r0, lsl #1
    57bc:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    57c0:	00000000 	andeq	r0, r0, r0
    57c4:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    57d0:	000000e0 	andeq	r0, r0, r0, ror #1
    57d4:	0000003c 	andeq	r0, r0, ip, lsr r0
    57d8:	00000000 	andeq	r0, r0, r0
    57dc:	00000024 	andeq	r0, r0, r4, lsr #32
    57e0:	0000003c 	andeq	r0, r0, ip, lsr r0
    57e4:	00000000 	andeq	r0, r0, r0
    57e8:	0000004e 	andeq	r0, r0, lr, asr #32
    57ec:	80012824 	andhi	r2, r1, r4, lsr #16
    57f0:	00002b00 	andeq	r2, r0, r0, lsl #22
    57f4:	000f0026 	andeq	r0, pc, r6, lsr #32
    57f8:	80027000 	andhi	r7, r2, r0
    57fc:	00002b13 	andeq	r2, r0, r3, lsl fp
    5800:	00100026 	andseq	r0, r0, r6, lsr #32
    5804:	80027004 	andhi	r7, r2, r4
    5808:	00000000 	andeq	r0, r0, r0
    580c:	00000064 	andeq	r0, r0, r4, rrx
    5810:	80012824 	andhi	r2, r1, r4, lsr #16
    5814:	00000007 	andeq	r0, r0, r7
    5818:	00020064 	andeq	r0, r2, r4, rrx
    581c:	80012824 	andhi	r2, r1, r4, lsr #16
    5820:	00002b26 	andeq	r2, r0, r6, lsr #22
    5824:	00020064 	andeq	r0, r2, r4, rrx
    5828:	80012824 	andhi	r2, r1, r4, lsr #16
    582c:	0000003d 	andeq	r0, r0, sp, lsr r0
    5830:	0000003c 	andeq	r0, r0, ip, lsr r0
    5834:	00000000 	andeq	r0, r0, r0
    5838:	0000004c 	andeq	r0, r0, ip, asr #32
    583c:	00000080 	andeq	r0, r0, r0, lsl #1
    5840:	00000000 	andeq	r0, r0, r0
    5844:	00000076 	andeq	r0, r0, r6, ror r0
    5848:	00000080 	andeq	r0, r0, r0, lsl #1
    584c:	00000000 	andeq	r0, r0, r0
    5850:	00000094 	muleq	r0, r4, r0
    5854:	00000080 	andeq	r0, r0, r0, lsl #1
    5858:	00000000 	andeq	r0, r0, r0
    585c:	000000c3 	andeq	r0, r0, r3, asr #1
    5860:	00000080 	andeq	r0, r0, r0, lsl #1
    5864:	00000000 	andeq	r0, r0, r0
    5868:	000000ee 	andeq	r0, r0, lr, ror #1
    586c:	00000080 	andeq	r0, r0, r0, lsl #1
    5870:	00000000 	andeq	r0, r0, r0
    5874:	0000011e 	andeq	r0, r0, lr, lsl r1
    5878:	00000080 	andeq	r0, r0, r0, lsl #1
    587c:	00000000 	andeq	r0, r0, r0
    5880:	0000016f 	andeq	r0, r0, pc, ror #2
    5884:	00000080 	andeq	r0, r0, r0, lsl #1
    5888:	00000000 	andeq	r0, r0, r0
    588c:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    5890:	00000080 	andeq	r0, r0, r0, lsl #1
    5894:	00000000 	andeq	r0, r0, r0
    5898:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    589c:	00000080 	andeq	r0, r0, r0, lsl #1
    58a0:	00000000 	andeq	r0, r0, r0
    58a4:	0000020e 	andeq	r0, r0, lr, lsl #4
    58a8:	00000080 	andeq	r0, r0, r0, lsl #1
    58ac:	00000000 	andeq	r0, r0, r0
    58b0:	00000238 	andeq	r0, r0, r8, lsr r2
    58b4:	00000080 	andeq	r0, r0, r0, lsl #1
    58b8:	00000000 	andeq	r0, r0, r0
    58bc:	00000261 	andeq	r0, r0, r1, ror #4
    58c0:	00000080 	andeq	r0, r0, r0, lsl #1
    58c4:	00000000 	andeq	r0, r0, r0
    58c8:	0000027b 	andeq	r0, r0, fp, ror r2
    58cc:	00000080 	andeq	r0, r0, r0, lsl #1
    58d0:	00000000 	andeq	r0, r0, r0
    58d4:	00000296 	muleq	r0, r6, r2
    58d8:	00000080 	andeq	r0, r0, r0, lsl #1
    58dc:	00000000 	andeq	r0, r0, r0
    58e0:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    58e4:	00000080 	andeq	r0, r0, r0, lsl #1
    58e8:	00000000 	andeq	r0, r0, r0
    58ec:	000002d7 	ldrdeq	r0, [r0], -r7
    58f0:	00000080 	andeq	r0, r0, r0, lsl #1
    58f4:	00000000 	andeq	r0, r0, r0
    58f8:	000002f7 	strdeq	r0, [r0], -r7
    58fc:	00000080 	andeq	r0, r0, r0, lsl #1
    5900:	00000000 	andeq	r0, r0, r0
    5904:	0000031c 	andeq	r0, r0, ip, lsl r3
    5908:	00000080 	andeq	r0, r0, r0, lsl #1
    590c:	00000000 	andeq	r0, r0, r0
    5910:	00000346 	andeq	r0, r0, r6, asr #6
    5914:	00000080 	andeq	r0, r0, r0, lsl #1
    5918:	00000000 	andeq	r0, r0, r0
    591c:	0000036a 	andeq	r0, r0, sl, ror #6
    5920:	00000080 	andeq	r0, r0, r0, lsl #1
    5924:	00000000 	andeq	r0, r0, r0
    5928:	00000393 	muleq	r0, r3, r3
    592c:	00000080 	andeq	r0, r0, r0, lsl #1
    5930:	00000000 	andeq	r0, r0, r0
    5934:	000003c1 	andeq	r0, r0, r1, asr #7
    5938:	00000080 	andeq	r0, r0, r0, lsl #1
    593c:	00000000 	andeq	r0, r0, r0
    5940:	000003e7 	andeq	r0, r0, r7, ror #7
    5944:	00000080 	andeq	r0, r0, r0, lsl #1
    5948:	00000000 	andeq	r0, r0, r0
    594c:	00000407 	andeq	r0, r0, r7, lsl #8
    5950:	00000080 	andeq	r0, r0, r0, lsl #1
    5954:	00000000 	andeq	r0, r0, r0
    5958:	0000042c 	andeq	r0, r0, ip, lsr #8
    595c:	00000080 	andeq	r0, r0, r0, lsl #1
    5960:	00000000 	andeq	r0, r0, r0
    5964:	00000456 	andeq	r0, r0, r6, asr r4
    5968:	00000080 	andeq	r0, r0, r0, lsl #1
    596c:	00000000 	andeq	r0, r0, r0
    5970:	00000485 	andeq	r0, r0, r5, lsl #9
    5974:	00000080 	andeq	r0, r0, r0, lsl #1
    5978:	00000000 	andeq	r0, r0, r0
    597c:	000004ae 	andeq	r0, r0, lr, lsr #9
    5980:	00000080 	andeq	r0, r0, r0, lsl #1
    5984:	00000000 	andeq	r0, r0, r0
    5988:	000004dc 	ldrdeq	r0, [r0], -ip
    598c:	00000080 	andeq	r0, r0, r0, lsl #1
    5990:	00000000 	andeq	r0, r0, r0
    5994:	0000050f 	andeq	r0, r0, pc, lsl #10
    5998:	00000080 	andeq	r0, r0, r0, lsl #1
    599c:	00000000 	andeq	r0, r0, r0
    59a0:	00000530 	andeq	r0, r0, r0, lsr r5
    59a4:	00000080 	andeq	r0, r0, r0, lsl #1
    59a8:	00000000 	andeq	r0, r0, r0
    59ac:	00000550 	andeq	r0, r0, r0, asr r5
    59b0:	00000080 	andeq	r0, r0, r0, lsl #1
    59b4:	00000000 	andeq	r0, r0, r0
    59b8:	00000575 	andeq	r0, r0, r5, ror r5
    59bc:	00000080 	andeq	r0, r0, r0, lsl #1
    59c0:	00000000 	andeq	r0, r0, r0
    59c4:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    59c8:	00000080 	andeq	r0, r0, r0, lsl #1
    59cc:	00000000 	andeq	r0, r0, r0
    59d0:	000005c3 	andeq	r0, r0, r3, asr #11
    59d4:	00000080 	andeq	r0, r0, r0, lsl #1
    59d8:	00000000 	andeq	r0, r0, r0
    59dc:	000005ec 	andeq	r0, r0, ip, ror #11
    59e0:	00000080 	andeq	r0, r0, r0, lsl #1
    59e4:	00000000 	andeq	r0, r0, r0
    59e8:	0000061a 	andeq	r0, r0, sl, lsl r6
    59ec:	00000080 	andeq	r0, r0, r0, lsl #1
    59f0:	00000000 	andeq	r0, r0, r0
    59f4:	00000640 	andeq	r0, r0, r0, asr #12
    59f8:	00000080 	andeq	r0, r0, r0, lsl #1
    59fc:	00000000 	andeq	r0, r0, r0
    5a00:	00000660 	andeq	r0, r0, r0, ror #12
    5a04:	00000080 	andeq	r0, r0, r0, lsl #1
    5a08:	00000000 	andeq	r0, r0, r0
    5a0c:	00000685 	andeq	r0, r0, r5, lsl #13
    5a10:	00000080 	andeq	r0, r0, r0, lsl #1
    5a14:	00000000 	andeq	r0, r0, r0
    5a18:	000006af 	andeq	r0, r0, pc, lsr #13
    5a1c:	00000080 	andeq	r0, r0, r0, lsl #1
    5a20:	00000000 	andeq	r0, r0, r0
    5a24:	000006de 	ldrdeq	r0, [r0], -lr
    5a28:	00000080 	andeq	r0, r0, r0, lsl #1
    5a2c:	00000000 	andeq	r0, r0, r0
    5a30:	00000707 	andeq	r0, r0, r7, lsl #14
    5a34:	00000080 	andeq	r0, r0, r0, lsl #1
    5a38:	00000000 	andeq	r0, r0, r0
    5a3c:	00000735 	andeq	r0, r0, r5, lsr r7
    5a40:	00000080 	andeq	r0, r0, r0, lsl #1
    5a44:	00000000 	andeq	r0, r0, r0
    5a48:	00000768 	andeq	r0, r0, r8, ror #14
    5a4c:	00000080 	andeq	r0, r0, r0, lsl #1
    5a50:	00000000 	andeq	r0, r0, r0
    5a54:	00001182 	andeq	r1, r0, r2, lsl #3
    5a58:	000000c2 	andeq	r0, r0, r2, asr #1
    5a5c:	00004df6 	strdeq	r4, [r0], -r6
    5a60:	0000078a 	andeq	r0, r0, sl, lsl #15
    5a64:	000000c2 	andeq	r0, r0, r2, asr #1
    5a68:	00002d60 	andeq	r2, r0, r0, ror #26
    5a6c:	00000886 	andeq	r0, r0, r6, lsl #17
    5a70:	000000c2 	andeq	r0, r0, r2, asr #1
    5a74:	00003ac8 	andeq	r3, r0, r8, asr #21
    5a78:	00002b3b 	andeq	r2, r0, fp, lsr fp
    5a7c:	00080024 	andeq	r0, r8, r4, lsr #32
    5a80:	80012824 	andhi	r2, r1, r4, lsr #16
    5a84:	00002b4c 	andeq	r2, r0, ip, asr #22
    5a88:	000800a0 	andeq	r0, r8, r0, lsr #1
    5a8c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    5a90:	00002b55 	andeq	r2, r0, r5, asr fp
    5a94:	000800a0 	andeq	r0, r8, r0, lsr #1
    5a98:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    5a9c:	00000000 	andeq	r0, r0, r0
    5aa0:	0000002e 	andeq	r0, r0, lr, lsr #32
    5aa4:	80012824 	andhi	r2, r1, r4, lsr #16
    5aa8:	00000000 	andeq	r0, r0, r0
    5aac:	00080044 	andeq	r0, r8, r4, asr #32
	...
    5ab8:	00090044 	andeq	r0, r9, r4, asr #32
    5abc:	00000018 	andeq	r0, r0, r8, lsl r0
    5ac0:	00000000 	andeq	r0, r0, r0
    5ac4:	000a0044 	andeq	r0, sl, r4, asr #32
    5ac8:	00000020 	andeq	r0, r0, r0, lsr #32
    5acc:	00000000 	andeq	r0, r0, r0
    5ad0:	000b0044 	andeq	r0, fp, r4, asr #32
    5ad4:	00000028 	andeq	r0, r0, r8, lsr #32
    5ad8:	00000000 	andeq	r0, r0, r0
    5adc:	000c0044 	andeq	r0, ip, r4, asr #32
    5ae0:	00000044 	andeq	r0, r0, r4, asr #32
    5ae4:	00000000 	andeq	r0, r0, r0
    5ae8:	000d0044 	andeq	r0, sp, r4, asr #32
    5aec:	00000058 	andeq	r0, r0, r8, asr r0
    5af0:	00000000 	andeq	r0, r0, r0
    5af4:	000e0044 	andeq	r0, lr, r4, asr #32
    5af8:	00000078 	andeq	r0, r0, r8, ror r0
    5afc:	00000000 	andeq	r0, r0, r0
    5b00:	000f0044 	andeq	r0, pc, r4, asr #32
    5b04:	0000007c 	andeq	r0, r0, ip, ror r0
    5b08:	00002b61 	andeq	r2, r0, r1, ror #22
    5b0c:	00090080 	andeq	r0, r9, r0, lsl #1
    5b10:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    5b14:	00002b71 	andeq	r2, r0, r1, ror fp
    5b18:	000a0080 	andeq	r0, sl, r0, lsl #1
    5b1c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5b20:	00000000 	andeq	r0, r0, r0
    5b24:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    5b30:	000000e0 	andeq	r0, r0, r0, ror #1
    5b34:	00000088 	andeq	r0, r0, r8, lsl #1
    5b38:	00000000 	andeq	r0, r0, r0
    5b3c:	00000024 	andeq	r0, r0, r4, lsr #32
    5b40:	00000088 	andeq	r0, r0, r8, lsl #1
    5b44:	00000000 	andeq	r0, r0, r0
    5b48:	0000004e 	andeq	r0, r0, lr, asr #32
    5b4c:	800128ac 	andhi	r2, r1, ip, lsr #17
    5b50:	00002b7e 	andeq	r2, r0, lr, ror fp
    5b54:	00120024 	andseq	r0, r2, r4, lsr #32
    5b58:	800128ac 	andhi	r2, r1, ip, lsr #17
    5b5c:	00002b8f 	andeq	r2, r0, pc, lsl #23
    5b60:	001200a0 	andseq	r0, r2, r0, lsr #1
    5b64:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    5b68:	00002ba0 	andeq	r2, r0, r0, lsr #23
    5b6c:	001200a0 	andseq	r0, r2, r0, lsr #1
    5b70:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    5b74:	00002b55 	andeq	r2, r0, r5, asr fp
    5b78:	001200a0 	andseq	r0, r2, r0, lsr #1
    5b7c:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    5b80:	00000000 	andeq	r0, r0, r0
    5b84:	0000002e 	andeq	r0, r0, lr, lsr #32
    5b88:	800128ac 	andhi	r2, r1, ip, lsr #17
    5b8c:	00000000 	andeq	r0, r0, r0
    5b90:	00120044 	andseq	r0, r2, r4, asr #32
	...
    5b9c:	00130044 	andseq	r0, r3, r4, asr #32
    5ba0:	0000001c 	andeq	r0, r0, ip, lsl r0
    5ba4:	00000000 	andeq	r0, r0, r0
    5ba8:	00150044 	andseq	r0, r5, r4, asr #32
    5bac:	00000024 	andeq	r0, r0, r4, lsr #32
    5bb0:	00000000 	andeq	r0, r0, r0
    5bb4:	00160044 	andseq	r0, r6, r4, asr #32
    5bb8:	00000050 	andeq	r0, r0, r0, asr r0
    5bbc:	00000000 	andeq	r0, r0, r0
    5bc0:	00190044 	andseq	r0, r9, r4, asr #32
    5bc4:	00000054 	andeq	r0, r0, r4, asr r0
    5bc8:	00000000 	andeq	r0, r0, r0
    5bcc:	001a0044 	andseq	r0, sl, r4, asr #32
    5bd0:	00000064 	andeq	r0, r0, r4, rrx
    5bd4:	00000000 	andeq	r0, r0, r0
    5bd8:	001b0044 	andseq	r0, fp, r4, asr #32
    5bdc:	00000078 	andeq	r0, r0, r8, ror r0
    5be0:	00000000 	andeq	r0, r0, r0
    5be4:	001c0044 	andseq	r0, ip, r4, asr #32
    5be8:	000000a0 	andeq	r0, r0, r0, lsr #1
    5bec:	00000000 	andeq	r0, r0, r0
    5bf0:	001e0044 	andseq	r0, lr, r4, asr #32
    5bf4:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    5bf8:	00000000 	andeq	r0, r0, r0
    5bfc:	001f0044 	andseq	r0, pc, r4, asr #32
    5c00:	000000c8 	andeq	r0, r0, r8, asr #1
    5c04:	00000000 	andeq	r0, r0, r0
    5c08:	00200044 	eoreq	r0, r0, r4, asr #32
    5c0c:	000000d8 	ldrdeq	r0, [r0], -r8
    5c10:	00000000 	andeq	r0, r0, r0
    5c14:	00210044 	eoreq	r0, r1, r4, asr #32
    5c18:	000000e8 	andeq	r0, r0, r8, ror #1
    5c1c:	00000000 	andeq	r0, r0, r0
    5c20:	00230044 	eoreq	r0, r3, r4, asr #32
    5c24:	000000f8 	strdeq	r0, [r0], -r8
    5c28:	00000000 	andeq	r0, r0, r0
    5c2c:	00240044 	eoreq	r0, r4, r4, asr #32
    5c30:	00000104 	andeq	r0, r0, r4, lsl #2
    5c34:	00000000 	andeq	r0, r0, r0
    5c38:	00260044 	eoreq	r0, r6, r4, asr #32
    5c3c:	00000110 	andeq	r0, r0, r0, lsl r1
    5c40:	00000000 	andeq	r0, r0, r0
    5c44:	00270044 	eoreq	r0, r7, r4, asr #32
    5c48:	00000124 	andeq	r0, r0, r4, lsr #2
    5c4c:	00000000 	andeq	r0, r0, r0
    5c50:	00280044 	eoreq	r0, r8, r4, asr #32
    5c54:	00000130 	andeq	r0, r0, r0, lsr r1
    5c58:	00002b61 	andeq	r2, r0, r1, ror #22
    5c5c:	00130080 	andseq	r0, r3, r0, lsl #1
    5c60:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    5c64:	00002bae 	andeq	r2, r0, lr, lsr #23
    5c68:	001a0080 	andseq	r0, sl, r0, lsl #1
    5c6c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5c70:	00002bb6 			; <UNDEFINED> instruction: 0x00002bb6
    5c74:	001b0080 	andseq	r0, fp, r0, lsl #1
    5c78:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    5c7c:	00000000 	andeq	r0, r0, r0
    5c80:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    5c8c:	000000e0 	andeq	r0, r0, r0, ror #1
    5c90:	00000138 	andeq	r0, r0, r8, lsr r1
    5c94:	00000000 	andeq	r0, r0, r0
    5c98:	00000024 	andeq	r0, r0, r4, lsr #32
    5c9c:	00000138 	andeq	r0, r0, r8, lsr r1
    5ca0:	00000000 	andeq	r0, r0, r0
    5ca4:	0000004e 	andeq	r0, r0, lr, asr #32
    5ca8:	800129e4 	andhi	r2, r1, r4, ror #19
    5cac:	00002bc6 	andeq	r2, r0, r6, asr #23
    5cb0:	002a0024 	eoreq	r0, sl, r4, lsr #32
    5cb4:	800129e4 	andhi	r2, r1, r4, ror #19
    5cb8:	00002bd9 	ldrdeq	r2, [r0], -r9
    5cbc:	002a00a0 	eoreq	r0, sl, r0, lsr #1
    5cc0:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    5cc4:	00002b55 	andeq	r2, r0, r5, asr fp
    5cc8:	002a00a0 	eoreq	r0, sl, r0, lsr #1
    5ccc:	ffffffd4 			; <UNDEFINED> instruction: 0xffffffd4
    5cd0:	00000000 	andeq	r0, r0, r0
    5cd4:	0000002e 	andeq	r0, r0, lr, lsr #32
    5cd8:	800129e4 	andhi	r2, r1, r4, ror #19
    5cdc:	00000000 	andeq	r0, r0, r0
    5ce0:	002a0044 	eoreq	r0, sl, r4, asr #32
	...
    5cec:	002b0044 	eoreq	r0, fp, r4, asr #32
    5cf0:	00000018 	andeq	r0, r0, r8, lsl r0
    5cf4:	00000000 	andeq	r0, r0, r0
    5cf8:	002c0044 	eoreq	r0, ip, r4, asr #32
    5cfc:	00000024 	andeq	r0, r0, r4, lsr #32
    5d00:	00000000 	andeq	r0, r0, r0
    5d04:	002d0044 	eoreq	r0, sp, r4, asr #32
    5d08:	00000028 	andeq	r0, r0, r8, lsr #32
    5d0c:	00000000 	andeq	r0, r0, r0
    5d10:	002d0044 	eoreq	r0, sp, r4, asr #32
    5d14:	00000040 	andeq	r0, r0, r0, asr #32
    5d18:	00000000 	andeq	r0, r0, r0
    5d1c:	002e0044 	eoreq	r0, lr, r4, asr #32
    5d20:	0000005c 	andeq	r0, r0, ip, asr r0
    5d24:	00000000 	andeq	r0, r0, r0
    5d28:	00310044 	eorseq	r0, r1, r4, asr #32
    5d2c:	0000006c 	andeq	r0, r0, ip, rrx
    5d30:	00000000 	andeq	r0, r0, r0
    5d34:	00320044 	eorseq	r0, r2, r4, asr #32
    5d38:	0000007c 	andeq	r0, r0, ip, ror r0
    5d3c:	00000000 	andeq	r0, r0, r0
    5d40:	00330044 	eorseq	r0, r3, r4, asr #32
    5d44:	0000008c 	andeq	r0, r0, ip, lsl #1
    5d48:	00000000 	andeq	r0, r0, r0
    5d4c:	002c0044 	eoreq	r0, ip, r4, asr #32
    5d50:	00000098 	muleq	r0, r8, r0
    5d54:	00000000 	andeq	r0, r0, r0
    5d58:	00380044 	eorseq	r0, r8, r4, asr #32
    5d5c:	000000a4 	andeq	r0, r0, r4, lsr #1
    5d60:	00000000 	andeq	r0, r0, r0
    5d64:	00390044 	eorseq	r0, r9, r4, asr #32
    5d68:	000000ac 	andeq	r0, r0, ip, lsr #1
    5d6c:	00000000 	andeq	r0, r0, r0
    5d70:	003b0044 	eorseq	r0, fp, r4, asr #32
    5d74:	000000bc 	strheq	r0, [r0], -ip
    5d78:	00000000 	andeq	r0, r0, r0
    5d7c:	003c0044 	eorseq	r0, ip, r4, asr #32
    5d80:	000000c8 	andeq	r0, r0, r8, asr #1
    5d84:	00000000 	andeq	r0, r0, r0
    5d88:	003d0044 	eorseq	r0, sp, r4, asr #32
    5d8c:	000000d8 	ldrdeq	r0, [r0], -r8
    5d90:	00000000 	andeq	r0, r0, r0
    5d94:	003f0044 	eorseq	r0, pc, r4, asr #32
    5d98:	000000e4 	andeq	r0, r0, r4, ror #1
    5d9c:	00000000 	andeq	r0, r0, r0
    5da0:	00400044 	subeq	r0, r0, r4, asr #32
    5da4:	00000100 	andeq	r0, r0, r0, lsl #2
    5da8:	00000000 	andeq	r0, r0, r0
    5dac:	00410044 	subeq	r0, r1, r4, asr #32
    5db0:	0000012c 	andeq	r0, r0, ip, lsr #2
    5db4:	00000000 	andeq	r0, r0, r0
    5db8:	00430044 	subeq	r0, r3, r4, asr #32
    5dbc:	00000134 	andeq	r0, r0, r4, lsr r1
    5dc0:	00000000 	andeq	r0, r0, r0
    5dc4:	00440044 	subeq	r0, r4, r4, asr #32
    5dc8:	0000014c 	andeq	r0, r0, ip, asr #2
    5dcc:	00000000 	andeq	r0, r0, r0
    5dd0:	00460044 	subeq	r0, r6, r4, asr #32
    5dd4:	0000015c 	andeq	r0, r0, ip, asr r1
    5dd8:	00000000 	andeq	r0, r0, r0
    5ddc:	00470044 	subeq	r0, r7, r4, asr #32
    5de0:	0000016c 	andeq	r0, r0, ip, ror #2
    5de4:	00000000 	andeq	r0, r0, r0
    5de8:	004a0044 	subeq	r0, sl, r4, asr #32
    5dec:	00000178 	andeq	r0, r0, r8, ror r1
    5df0:	00000000 	andeq	r0, r0, r0
    5df4:	004b0044 	subeq	r0, fp, r4, asr #32
    5df8:	00000188 	andeq	r0, r0, r8, lsl #3
    5dfc:	00000000 	andeq	r0, r0, r0
    5e00:	004e0044 	subeq	r0, lr, r4, asr #32
    5e04:	00000198 	muleq	r0, r8, r1
    5e08:	00000000 	andeq	r0, r0, r0
    5e0c:	004f0044 	subeq	r0, pc, r4, asr #32
    5e10:	000001a8 	andeq	r0, r0, r8, lsr #3
    5e14:	00000000 	andeq	r0, r0, r0
    5e18:	00500044 	subseq	r0, r0, r4, asr #32
    5e1c:	000001b8 			; <UNDEFINED> instruction: 0x000001b8
    5e20:	00000000 	andeq	r0, r0, r0
    5e24:	00530044 	subseq	r0, r3, r4, asr #32
    5e28:	000001c4 	andeq	r0, r0, r4, asr #3
    5e2c:	00000000 	andeq	r0, r0, r0
    5e30:	00540044 	subseq	r0, r4, r4, asr #32
    5e34:	000001d4 	ldrdeq	r0, [r0], -r4
    5e38:	00000000 	andeq	r0, r0, r0
    5e3c:	00550044 	subseq	r0, r5, r4, asr #32
    5e40:	000001dc 	ldrdeq	r0, [r0], -ip
    5e44:	00002b71 	andeq	r2, r0, r1, ror fp
    5e48:	002b0080 	eoreq	r0, fp, r0, lsl #1
    5e4c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    5e50:	00002b61 	andeq	r2, r0, r1, ror #22
    5e54:	00380080 	eorseq	r0, r8, r0, lsl #1
    5e58:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    5e5c:	00002be3 	andeq	r2, r0, r3, ror #23
    5e60:	00390080 	eorseq	r0, r9, r0, lsl #1
    5e64:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    5e68:	00002bf4 	strdeq	r2, [r0], -r4
    5e6c:	003b0080 	eorseq	r0, fp, r0, lsl #1
    5e70:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5e74:	00002bae 	andeq	r2, r0, lr, lsr #23
    5e78:	003f0080 	eorseq	r0, pc, r0, lsl #1
    5e7c:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    5e80:	00000000 	andeq	r0, r0, r0
    5e84:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    5e90:	000000e0 	andeq	r0, r0, r0, ror #1
    5e94:	000001e8 	andeq	r0, r0, r8, ror #3
    5e98:	00000000 	andeq	r0, r0, r0
    5e9c:	00000024 	andeq	r0, r0, r4, lsr #32
    5ea0:	000001e8 	andeq	r0, r0, r8, ror #3
    5ea4:	00000000 	andeq	r0, r0, r0
    5ea8:	0000004e 	andeq	r0, r0, lr, asr #32
    5eac:	80012bcc 	andhi	r2, r1, ip, asr #23
    5eb0:	00002c00 	andeq	r2, r0, r0, lsl #24
    5eb4:	005a0024 	subseq	r0, sl, r4, lsr #32
    5eb8:	80012bcc 	andhi	r2, r1, ip, asr #23
    5ebc:	00002bd9 	ldrdeq	r2, [r0], -r9
    5ec0:	005a00a0 	subseq	r0, sl, r0, lsr #1
    5ec4:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    5ec8:	00002ba0 	andeq	r2, r0, r0, lsr #23
    5ecc:	005a00a0 	subseq	r0, sl, r0, lsr #1
    5ed0:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    5ed4:	00000000 	andeq	r0, r0, r0
    5ed8:	0000002e 	andeq	r0, r0, lr, lsr #32
    5edc:	80012bcc 	andhi	r2, r1, ip, asr #23
    5ee0:	00000000 	andeq	r0, r0, r0
    5ee4:	005a0044 	subseq	r0, sl, r4, asr #32
	...
    5ef0:	005b0044 	subseq	r0, fp, r4, asr #32
    5ef4:	00000018 	andeq	r0, r0, r8, lsl r0
    5ef8:	00000000 	andeq	r0, r0, r0
    5efc:	005d0044 	subseq	r0, sp, r4, asr #32
    5f00:	00000020 	andeq	r0, r0, r0, lsr #32
    5f04:	00000000 	andeq	r0, r0, r0
    5f08:	005e0044 	subseq	r0, lr, r4, asr #32
    5f0c:	0000002c 	andeq	r0, r0, ip, lsr #32
    5f10:	00000000 	andeq	r0, r0, r0
    5f14:	005e0044 	subseq	r0, lr, r4, asr #32
    5f18:	00000038 	andeq	r0, r0, r8, lsr r0
    5f1c:	00000000 	andeq	r0, r0, r0
    5f20:	005f0044 	subseq	r0, pc, r4, asr #32
    5f24:	00000050 	andeq	r0, r0, r0, asr r0
    5f28:	00000000 	andeq	r0, r0, r0
    5f2c:	00600044 	rsbeq	r0, r0, r4, asr #32
    5f30:	00000090 	muleq	r0, r0, r0
    5f34:	00000000 	andeq	r0, r0, r0
    5f38:	00610044 	rsbeq	r0, r1, r4, asr #32
    5f3c:	000000a0 	andeq	r0, r0, r0, lsr #1
    5f40:	00000000 	andeq	r0, r0, r0
    5f44:	00620044 	rsbeq	r0, r2, r4, asr #32
    5f48:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    5f4c:	00000000 	andeq	r0, r0, r0
    5f50:	00640044 	rsbeq	r0, r4, r4, asr #32
    5f54:	000000c0 	andeq	r0, r0, r0, asr #1
    5f58:	00000000 	andeq	r0, r0, r0
    5f5c:	00650044 	rsbeq	r0, r5, r4, asr #32
    5f60:	000000d4 	ldrdeq	r0, [r0], -r4
    5f64:	00000000 	andeq	r0, r0, r0
    5f68:	00690044 	rsbeq	r0, r9, r4, asr #32
    5f6c:	000000e0 	andeq	r0, r0, r0, ror #1
    5f70:	00000000 	andeq	r0, r0, r0
    5f74:	006a0044 	rsbeq	r0, sl, r4, asr #32
    5f78:	000000ec 	andeq	r0, r0, ip, ror #1
    5f7c:	00000000 	andeq	r0, r0, r0
    5f80:	006a0044 	rsbeq	r0, sl, r4, asr #32
    5f84:	000000f8 	strdeq	r0, [r0], -r8
    5f88:	00000000 	andeq	r0, r0, r0
    5f8c:	006b0044 	rsbeq	r0, fp, r4, asr #32
    5f90:	00000110 	andeq	r0, r0, r0, lsl r1
    5f94:	00000000 	andeq	r0, r0, r0
    5f98:	006c0044 	rsbeq	r0, ip, r4, asr #32
    5f9c:	00000150 	andeq	r0, r0, r0, asr r1
    5fa0:	00000000 	andeq	r0, r0, r0
    5fa4:	006d0044 	rsbeq	r0, sp, r4, asr #32
    5fa8:	00000160 	andeq	r0, r0, r0, ror #2
    5fac:	00000000 	andeq	r0, r0, r0
    5fb0:	006e0044 	rsbeq	r0, lr, r4, asr #32
    5fb4:	00000170 	andeq	r0, r0, r0, ror r1
    5fb8:	00000000 	andeq	r0, r0, r0
    5fbc:	006f0044 	rsbeq	r0, pc, r4, asr #32
    5fc0:	00000180 	andeq	r0, r0, r0, lsl #3
    5fc4:	00000000 	andeq	r0, r0, r0
    5fc8:	00700044 	rsbseq	r0, r0, r4, asr #32
    5fcc:	00000194 	muleq	r0, r4, r1
    5fd0:	00000000 	andeq	r0, r0, r0
    5fd4:	00720044 	rsbseq	r0, r2, r4, asr #32
    5fd8:	000001a0 	andeq	r0, r0, r0, lsr #3
    5fdc:	00002b61 	andeq	r2, r0, r1, ror #22
    5fe0:	005b0080 	subseq	r0, fp, r0, lsl #1
    5fe4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    5fe8:	00002c11 	andeq	r2, r0, r1, lsl ip
    5fec:	005d0080 	subseq	r0, sp, r0, lsl #1
    5ff0:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    5ff4:	00000000 	andeq	r0, r0, r0
    5ff8:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    6004:	000000e0 	andeq	r0, r0, r0, ror #1
    6008:	000001a8 	andeq	r0, r0, r8, lsr #3
    600c:	00000000 	andeq	r0, r0, r0
    6010:	00000024 	andeq	r0, r0, r4, lsr #32
    6014:	000001a8 	andeq	r0, r0, r8, lsr #3
    6018:	00000000 	andeq	r0, r0, r0
    601c:	0000004e 	andeq	r0, r0, lr, asr #32
    6020:	80012d74 	andhi	r2, r1, r4, ror sp
    6024:	00002c1a 	andeq	r2, r0, sl, lsl ip
    6028:	00770024 	rsbseq	r0, r7, r4, lsr #32
    602c:	80012d74 	andhi	r2, r1, r4, ror sp
    6030:	00002bd9 	ldrdeq	r2, [r0], -r9
    6034:	007700a0 	rsbseq	r0, r7, r0, lsr #1
    6038:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    603c:	00000000 	andeq	r0, r0, r0
    6040:	0000002e 	andeq	r0, r0, lr, lsr #32
    6044:	80012d74 	andhi	r2, r1, r4, ror sp
    6048:	00000000 	andeq	r0, r0, r0
    604c:	00770044 	rsbseq	r0, r7, r4, asr #32
	...
    6058:	00780044 	rsbseq	r0, r8, r4, asr #32
    605c:	00000014 	andeq	r0, r0, r4, lsl r0
    6060:	00000000 	andeq	r0, r0, r0
    6064:	00790044 	rsbseq	r0, r9, r4, asr #32
    6068:	0000001c 	andeq	r0, r0, ip, lsl r0
    606c:	00000000 	andeq	r0, r0, r0
    6070:	007b0044 	rsbseq	r0, fp, r4, asr #32
    6074:	00000028 	andeq	r0, r0, r8, lsr #32
    6078:	00000000 	andeq	r0, r0, r0
    607c:	007b0044 	rsbseq	r0, fp, r4, asr #32
    6080:	00000044 	andeq	r0, r0, r4, asr #32
    6084:	00000000 	andeq	r0, r0, r0
    6088:	007c0044 	rsbseq	r0, ip, r4, asr #32
    608c:	00000054 	andeq	r0, r0, r4, asr r0
    6090:	00000000 	andeq	r0, r0, r0
    6094:	007e0044 	rsbseq	r0, lr, r4, asr #32
    6098:	00000058 	andeq	r0, r0, r8, asr r0
    609c:	00000000 	andeq	r0, r0, r0
    60a0:	007f0044 	rsbseq	r0, pc, r4, asr #32
    60a4:	0000007c 	andeq	r0, r0, ip, ror r0
    60a8:	00000000 	andeq	r0, r0, r0
    60ac:	00800044 	addeq	r0, r0, r4, asr #32
    60b0:	00000090 	muleq	r0, r0, r0
    60b4:	00000000 	andeq	r0, r0, r0
    60b8:	00810044 	addeq	r0, r1, r4, asr #32
    60bc:	000000a0 	andeq	r0, r0, r0, lsr #1
    60c0:	00000000 	andeq	r0, r0, r0
    60c4:	00820044 	addeq	r0, r2, r4, asr #32
    60c8:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    60cc:	00000000 	andeq	r0, r0, r0
    60d0:	00840044 	addeq	r0, r4, r4, asr #32
    60d4:	000000bc 	strheq	r0, [r0], -ip
    60d8:	00000000 	andeq	r0, r0, r0
    60dc:	00850044 	addeq	r0, r5, r4, asr #32
    60e0:	000000d8 	ldrdeq	r0, [r0], -r8
    60e4:	00002b61 	andeq	r2, r0, r1, ror #22
    60e8:	00780080 	rsbseq	r0, r8, r0, lsl #1
    60ec:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    60f0:	00002c2c 	andeq	r2, r0, ip, lsr #24
    60f4:	00790080 	rsbseq	r0, r9, r0, lsl #1
    60f8:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    60fc:	00002c37 	andeq	r2, r0, r7, lsr ip
    6100:	007e0080 	rsbseq	r0, lr, r0, lsl #1
    6104:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    6108:	00000000 	andeq	r0, r0, r0
    610c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    6118:	000000e0 	andeq	r0, r0, r0, ror #1
    611c:	000000e0 	andeq	r0, r0, r0, ror #1
    6120:	00000000 	andeq	r0, r0, r0
    6124:	00000024 	andeq	r0, r0, r4, lsr #32
    6128:	000000e0 	andeq	r0, r0, r0, ror #1
    612c:	00000000 	andeq	r0, r0, r0
    6130:	0000004e 	andeq	r0, r0, lr, asr #32
    6134:	80012e54 	andhi	r2, r1, r4, asr lr
    6138:	00002c43 	andeq	r2, r0, r3, asr #24
    613c:	00870024 	addeq	r0, r7, r4, lsr #32
    6140:	80012e54 	andhi	r2, r1, r4, asr lr
    6144:	00002bd9 	ldrdeq	r2, [r0], -r9
    6148:	008700a0 	addeq	r0, r7, r0, lsr #1
    614c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    6150:	00002b4c 	andeq	r2, r0, ip, asr #22
    6154:	008700a0 	addeq	r0, r7, r0, lsr #1
    6158:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    615c:	00000000 	andeq	r0, r0, r0
    6160:	0000002e 	andeq	r0, r0, lr, lsr #32
    6164:	80012e54 	andhi	r2, r1, r4, asr lr
    6168:	00000000 	andeq	r0, r0, r0
    616c:	00870044 	addeq	r0, r7, r4, asr #32
	...
    6178:	00880044 	addeq	r0, r8, r4, asr #32
    617c:	00000018 	andeq	r0, r0, r8, lsl r0
    6180:	00000000 	andeq	r0, r0, r0
    6184:	00890044 	addeq	r0, r9, r4, asr #32
    6188:	00000020 	andeq	r0, r0, r0, lsr #32
    618c:	00000000 	andeq	r0, r0, r0
    6190:	008a0044 	addeq	r0, sl, r4, asr #32
    6194:	00000030 	andeq	r0, r0, r0, lsr r0
    6198:	00000000 	andeq	r0, r0, r0
    619c:	008b0044 	addeq	r0, fp, r4, asr #32
    61a0:	00000034 	andeq	r0, r0, r4, lsr r0
    61a4:	00000000 	andeq	r0, r0, r0
    61a8:	008c0044 	addeq	r0, ip, r4, asr #32
    61ac:	00000048 	andeq	r0, r0, r8, asr #32
    61b0:	00000000 	andeq	r0, r0, r0
    61b4:	008e0044 	addeq	r0, lr, r4, asr #32
    61b8:	00000058 	andeq	r0, r0, r8, asr r0
    61bc:	00000000 	andeq	r0, r0, r0
    61c0:	008f0044 	addeq	r0, pc, r4, asr #32
    61c4:	00000064 	andeq	r0, r0, r4, rrx
    61c8:	00000000 	andeq	r0, r0, r0
    61cc:	00900044 	addseq	r0, r0, r4, asr #32
    61d0:	0000006c 	andeq	r0, r0, ip, rrx
    61d4:	00002b61 	andeq	r2, r0, r1, ror #22
    61d8:	00880080 	addeq	r0, r8, r0, lsl #1
    61dc:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    61e0:	00002b71 	andeq	r2, r0, r1, ror fp
    61e4:	008b0080 	addeq	r0, fp, r0, lsl #1
    61e8:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    61ec:	00000000 	andeq	r0, r0, r0
    61f0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    61fc:	000000e0 	andeq	r0, r0, r0, ror #1
    6200:	00000074 	andeq	r0, r0, r4, ror r0
    6204:	00000000 	andeq	r0, r0, r0
    6208:	00000024 	andeq	r0, r0, r4, lsr #32
    620c:	00000074 	andeq	r0, r0, r4, ror r0
    6210:	00000000 	andeq	r0, r0, r0
    6214:	0000004e 	andeq	r0, r0, lr, asr #32
    6218:	80012ec8 	andhi	r2, r1, r8, asr #29
    621c:	00000000 	andeq	r0, r0, r0
    6220:	00000064 	andeq	r0, r0, r4, rrx
    6224:	80012ec8 	andhi	r2, r1, r8, asr #29
    6228:	00000007 	andeq	r0, r0, r7
    622c:	00020064 	andeq	r0, r2, r4, rrx
    6230:	80012ec8 	andhi	r2, r1, r8, asr #29
    6234:	00002c55 	andeq	r2, r0, r5, asr ip
    6238:	00020064 	andeq	r0, r2, r4, rrx
    623c:	80012ec8 	andhi	r2, r1, r8, asr #29
    6240:	0000003d 	andeq	r0, r0, sp, lsr r0
    6244:	0000003c 	andeq	r0, r0, ip, lsr r0
    6248:	00000000 	andeq	r0, r0, r0
    624c:	0000004c 	andeq	r0, r0, ip, asr #32
    6250:	00000080 	andeq	r0, r0, r0, lsl #1
    6254:	00000000 	andeq	r0, r0, r0
    6258:	00000076 	andeq	r0, r0, r6, ror r0
    625c:	00000080 	andeq	r0, r0, r0, lsl #1
    6260:	00000000 	andeq	r0, r0, r0
    6264:	00000094 	muleq	r0, r4, r0
    6268:	00000080 	andeq	r0, r0, r0, lsl #1
    626c:	00000000 	andeq	r0, r0, r0
    6270:	000000c3 	andeq	r0, r0, r3, asr #1
    6274:	00000080 	andeq	r0, r0, r0, lsl #1
    6278:	00000000 	andeq	r0, r0, r0
    627c:	000000ee 	andeq	r0, r0, lr, ror #1
    6280:	00000080 	andeq	r0, r0, r0, lsl #1
    6284:	00000000 	andeq	r0, r0, r0
    6288:	0000011e 	andeq	r0, r0, lr, lsl r1
    628c:	00000080 	andeq	r0, r0, r0, lsl #1
    6290:	00000000 	andeq	r0, r0, r0
    6294:	0000016f 	andeq	r0, r0, pc, ror #2
    6298:	00000080 	andeq	r0, r0, r0, lsl #1
    629c:	00000000 	andeq	r0, r0, r0
    62a0:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    62a4:	00000080 	andeq	r0, r0, r0, lsl #1
    62a8:	00000000 	andeq	r0, r0, r0
    62ac:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    62b0:	00000080 	andeq	r0, r0, r0, lsl #1
    62b4:	00000000 	andeq	r0, r0, r0
    62b8:	0000020e 	andeq	r0, r0, lr, lsl #4
    62bc:	00000080 	andeq	r0, r0, r0, lsl #1
    62c0:	00000000 	andeq	r0, r0, r0
    62c4:	00000238 	andeq	r0, r0, r8, lsr r2
    62c8:	00000080 	andeq	r0, r0, r0, lsl #1
    62cc:	00000000 	andeq	r0, r0, r0
    62d0:	00000261 	andeq	r0, r0, r1, ror #4
    62d4:	00000080 	andeq	r0, r0, r0, lsl #1
    62d8:	00000000 	andeq	r0, r0, r0
    62dc:	0000027b 	andeq	r0, r0, fp, ror r2
    62e0:	00000080 	andeq	r0, r0, r0, lsl #1
    62e4:	00000000 	andeq	r0, r0, r0
    62e8:	00000296 	muleq	r0, r6, r2
    62ec:	00000080 	andeq	r0, r0, r0, lsl #1
    62f0:	00000000 	andeq	r0, r0, r0
    62f4:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    62f8:	00000080 	andeq	r0, r0, r0, lsl #1
    62fc:	00000000 	andeq	r0, r0, r0
    6300:	000002d7 	ldrdeq	r0, [r0], -r7
    6304:	00000080 	andeq	r0, r0, r0, lsl #1
    6308:	00000000 	andeq	r0, r0, r0
    630c:	000002f7 	strdeq	r0, [r0], -r7
    6310:	00000080 	andeq	r0, r0, r0, lsl #1
    6314:	00000000 	andeq	r0, r0, r0
    6318:	0000031c 	andeq	r0, r0, ip, lsl r3
    631c:	00000080 	andeq	r0, r0, r0, lsl #1
    6320:	00000000 	andeq	r0, r0, r0
    6324:	00000346 	andeq	r0, r0, r6, asr #6
    6328:	00000080 	andeq	r0, r0, r0, lsl #1
    632c:	00000000 	andeq	r0, r0, r0
    6330:	0000036a 	andeq	r0, r0, sl, ror #6
    6334:	00000080 	andeq	r0, r0, r0, lsl #1
    6338:	00000000 	andeq	r0, r0, r0
    633c:	00000393 	muleq	r0, r3, r3
    6340:	00000080 	andeq	r0, r0, r0, lsl #1
    6344:	00000000 	andeq	r0, r0, r0
    6348:	000003c1 	andeq	r0, r0, r1, asr #7
    634c:	00000080 	andeq	r0, r0, r0, lsl #1
    6350:	00000000 	andeq	r0, r0, r0
    6354:	000003e7 	andeq	r0, r0, r7, ror #7
    6358:	00000080 	andeq	r0, r0, r0, lsl #1
    635c:	00000000 	andeq	r0, r0, r0
    6360:	00000407 	andeq	r0, r0, r7, lsl #8
    6364:	00000080 	andeq	r0, r0, r0, lsl #1
    6368:	00000000 	andeq	r0, r0, r0
    636c:	0000042c 	andeq	r0, r0, ip, lsr #8
    6370:	00000080 	andeq	r0, r0, r0, lsl #1
    6374:	00000000 	andeq	r0, r0, r0
    6378:	00000456 	andeq	r0, r0, r6, asr r4
    637c:	00000080 	andeq	r0, r0, r0, lsl #1
    6380:	00000000 	andeq	r0, r0, r0
    6384:	00000485 	andeq	r0, r0, r5, lsl #9
    6388:	00000080 	andeq	r0, r0, r0, lsl #1
    638c:	00000000 	andeq	r0, r0, r0
    6390:	000004ae 	andeq	r0, r0, lr, lsr #9
    6394:	00000080 	andeq	r0, r0, r0, lsl #1
    6398:	00000000 	andeq	r0, r0, r0
    639c:	000004dc 	ldrdeq	r0, [r0], -ip
    63a0:	00000080 	andeq	r0, r0, r0, lsl #1
    63a4:	00000000 	andeq	r0, r0, r0
    63a8:	0000050f 	andeq	r0, r0, pc, lsl #10
    63ac:	00000080 	andeq	r0, r0, r0, lsl #1
    63b0:	00000000 	andeq	r0, r0, r0
    63b4:	00000530 	andeq	r0, r0, r0, lsr r5
    63b8:	00000080 	andeq	r0, r0, r0, lsl #1
    63bc:	00000000 	andeq	r0, r0, r0
    63c0:	00000550 	andeq	r0, r0, r0, asr r5
    63c4:	00000080 	andeq	r0, r0, r0, lsl #1
    63c8:	00000000 	andeq	r0, r0, r0
    63cc:	00000575 	andeq	r0, r0, r5, ror r5
    63d0:	00000080 	andeq	r0, r0, r0, lsl #1
    63d4:	00000000 	andeq	r0, r0, r0
    63d8:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    63dc:	00000080 	andeq	r0, r0, r0, lsl #1
    63e0:	00000000 	andeq	r0, r0, r0
    63e4:	000005c3 	andeq	r0, r0, r3, asr #11
    63e8:	00000080 	andeq	r0, r0, r0, lsl #1
    63ec:	00000000 	andeq	r0, r0, r0
    63f0:	000005ec 	andeq	r0, r0, ip, ror #11
    63f4:	00000080 	andeq	r0, r0, r0, lsl #1
    63f8:	00000000 	andeq	r0, r0, r0
    63fc:	0000061a 	andeq	r0, r0, sl, lsl r6
    6400:	00000080 	andeq	r0, r0, r0, lsl #1
    6404:	00000000 	andeq	r0, r0, r0
    6408:	00000640 	andeq	r0, r0, r0, asr #12
    640c:	00000080 	andeq	r0, r0, r0, lsl #1
    6410:	00000000 	andeq	r0, r0, r0
    6414:	00000660 	andeq	r0, r0, r0, ror #12
    6418:	00000080 	andeq	r0, r0, r0, lsl #1
    641c:	00000000 	andeq	r0, r0, r0
    6420:	00000685 	andeq	r0, r0, r5, lsl #13
    6424:	00000080 	andeq	r0, r0, r0, lsl #1
    6428:	00000000 	andeq	r0, r0, r0
    642c:	000006af 	andeq	r0, r0, pc, lsr #13
    6430:	00000080 	andeq	r0, r0, r0, lsl #1
    6434:	00000000 	andeq	r0, r0, r0
    6438:	000006de 	ldrdeq	r0, [r0], -lr
    643c:	00000080 	andeq	r0, r0, r0, lsl #1
    6440:	00000000 	andeq	r0, r0, r0
    6444:	00000707 	andeq	r0, r0, r7, lsl #14
    6448:	00000080 	andeq	r0, r0, r0, lsl #1
    644c:	00000000 	andeq	r0, r0, r0
    6450:	00000735 	andeq	r0, r0, r5, lsr r7
    6454:	00000080 	andeq	r0, r0, r0, lsl #1
    6458:	00000000 	andeq	r0, r0, r0
    645c:	00000768 	andeq	r0, r0, r8, ror #14
    6460:	00000080 	andeq	r0, r0, r0, lsl #1
    6464:	00000000 	andeq	r0, r0, r0
    6468:	00002c66 	andeq	r2, r0, r6, ror #24
    646c:	00000082 	andeq	r0, r0, r2, lsl #1
    6470:	00000000 	andeq	r0, r0, r0
    6474:	0000078a 	andeq	r0, r0, sl, lsl #15
    6478:	000000c2 	andeq	r0, r0, r2, asr #1
    647c:	00002d60 	andeq	r2, r0, r0, ror #26
    6480:	00000000 	andeq	r0, r0, r0
    6484:	000000a2 	andeq	r0, r0, r2, lsr #1
    6488:	00000000 	andeq	r0, r0, r0
    648c:	00001182 	andeq	r1, r0, r2, lsl #3
    6490:	000000c2 	andeq	r0, r0, r2, asr #1
    6494:	00004df6 	strdeq	r4, [r0], -r6
    6498:	00000886 	andeq	r0, r0, r6, lsl #17
    649c:	000000c2 	andeq	r0, r0, r2, asr #1
    64a0:	00003ac8 	andeq	r3, r0, r8, asr #21
    64a4:	00001056 	andeq	r1, r0, r6, asr r0
    64a8:	000000c2 	andeq	r0, r0, r2, asr #1
    64ac:	00000000 	andeq	r0, r0, r0
    64b0:	00001067 	andeq	r1, r0, r7, rrx
    64b4:	000000c2 	andeq	r0, r0, r2, asr #1
    64b8:	00003803 	andeq	r3, r0, r3, lsl #16
    64bc:	00002c7b 	andeq	r2, r0, fp, ror ip
    64c0:	00090024 	andeq	r0, r9, r4, lsr #32
    64c4:	80012ec8 	andhi	r2, r1, r8, asr #29
    64c8:	00002c8c 	andeq	r2, r0, ip, lsl #25
    64cc:	000900a0 	andeq	r0, r9, r0, lsr #1
    64d0:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    64d4:	00002c97 	muleq	r0, r7, ip
    64d8:	000900a0 	andeq	r0, r9, r0, lsr #1
    64dc:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    64e0:	00000000 	andeq	r0, r0, r0
    64e4:	0000002e 	andeq	r0, r0, lr, lsr #32
    64e8:	80012ec8 	andhi	r2, r1, r8, asr #29
    64ec:	00000000 	andeq	r0, r0, r0
    64f0:	00090044 	andeq	r0, r9, r4, asr #32
	...
    64fc:	000b0044 	andeq	r0, fp, r4, asr #32
    6500:	00000018 	andeq	r0, r0, r8, lsl r0
    6504:	00000000 	andeq	r0, r0, r0
    6508:	000c0044 	andeq	r0, ip, r4, asr #32
    650c:	00000044 	andeq	r0, r0, r4, asr #32
    6510:	00000000 	andeq	r0, r0, r0
    6514:	00000024 	andeq	r0, r0, r4, lsr #32
    6518:	00000054 	andeq	r0, r0, r4, asr r0
    651c:	00000000 	andeq	r0, r0, r0
    6520:	0000004e 	andeq	r0, r0, lr, asr #32
    6524:	80012f1c 	andhi	r2, r1, ip, lsl pc
    6528:	00002ca4 	andeq	r2, r0, r4, lsr #25
    652c:	000e0024 	andeq	r0, lr, r4, lsr #32
    6530:	80012f1c 	andhi	r2, r1, ip, lsl pc
    6534:	00002c8c 	andeq	r2, r0, ip, lsl #25
    6538:	000e00a0 	andeq	r0, lr, r0, lsr #1
    653c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    6540:	00002c97 	muleq	r0, r7, ip
    6544:	000e00a0 	andeq	r0, lr, r0, lsr #1
    6548:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    654c:	00000000 	andeq	r0, r0, r0
    6550:	0000002e 	andeq	r0, r0, lr, lsr #32
    6554:	80012f1c 	andhi	r2, r1, ip, lsl pc
    6558:	00000000 	andeq	r0, r0, r0
    655c:	000e0044 	andeq	r0, lr, r4, asr #32
	...
    6568:	00100044 	andseq	r0, r0, r4, asr #32
    656c:	00000020 	andeq	r0, r0, r0, lsr #32
    6570:	00000000 	andeq	r0, r0, r0
    6574:	00110044 	andseq	r0, r1, r4, asr #32
    6578:	00000040 	andeq	r0, r0, r0, asr #32
    657c:	00000000 	andeq	r0, r0, r0
    6580:	00120044 	andseq	r0, r2, r4, asr #32
    6584:	00000058 	andeq	r0, r0, r8, asr r0
    6588:	00000000 	andeq	r0, r0, r0
    658c:	00140044 	andseq	r0, r4, r4, asr #32
    6590:	00000060 	andeq	r0, r0, r0, rrx
    6594:	00000000 	andeq	r0, r0, r0
    6598:	00150044 	andseq	r0, r5, r4, asr #32
    659c:	00000070 	andeq	r0, r0, r0, ror r0
    65a0:	00000000 	andeq	r0, r0, r0
    65a4:	00160044 	andseq	r0, r6, r4, asr #32
    65a8:	00000074 	andeq	r0, r0, r4, ror r0
    65ac:	00002cb4 			; <UNDEFINED> instruction: 0x00002cb4
    65b0:	00100080 	andseq	r0, r0, r0, lsl #1
    65b4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    65b8:	00000000 	andeq	r0, r0, r0
    65bc:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    65c8:	000000e0 	andeq	r0, r0, r0, ror #1
    65cc:	00000090 	muleq	r0, r0, r0
    65d0:	00000000 	andeq	r0, r0, r0
    65d4:	00000024 	andeq	r0, r0, r4, lsr #32
    65d8:	00000090 	muleq	r0, r0, r0
    65dc:	00000000 	andeq	r0, r0, r0
    65e0:	0000004e 	andeq	r0, r0, lr, asr #32
    65e4:	80012fac 	andhi	r2, r1, ip, lsr #31
    65e8:	00002cbd 			; <UNDEFINED> instruction: 0x00002cbd
    65ec:	00180024 	andseq	r0, r8, r4, lsr #32
    65f0:	80012fac 	andhi	r2, r1, ip, lsr #31
    65f4:	00002c8c 	andeq	r2, r0, ip, lsl #25
    65f8:	001800a0 	andseq	r0, r8, r0, lsr #1
    65fc:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    6600:	00000000 	andeq	r0, r0, r0
    6604:	0000002e 	andeq	r0, r0, lr, lsr #32
    6608:	80012fac 	andhi	r2, r1, ip, lsr #31
    660c:	00000000 	andeq	r0, r0, r0
    6610:	00180044 	andseq	r0, r8, r4, asr #32
	...
    661c:	001a0044 	andseq	r0, sl, r4, asr #32
    6620:	00000014 	andeq	r0, r0, r4, lsl r0
    6624:	00000000 	andeq	r0, r0, r0
    6628:	001b0044 	andseq	r0, fp, r4, asr #32
    662c:	00000020 	andeq	r0, r0, r0, lsr #32
    6630:	00000000 	andeq	r0, r0, r0
    6634:	00000024 	andeq	r0, r0, r4, lsr #32
    6638:	00000030 	andeq	r0, r0, r0, lsr r0
    663c:	00000000 	andeq	r0, r0, r0
    6640:	0000004e 	andeq	r0, r0, lr, asr #32
    6644:	80012fdc 	ldrdhi	r2, [r1], -ip
    6648:	00002cd1 	ldrdeq	r2, [r0], -r1
    664c:	001d0024 	andseq	r0, sp, r4, lsr #32
    6650:	80012fdc 	ldrdhi	r2, [r1], -ip
    6654:	00000000 	andeq	r0, r0, r0
    6658:	0000002e 	andeq	r0, r0, lr, lsr #32
    665c:	80012fdc 	ldrdhi	r2, [r1], -ip
    6660:	00000000 	andeq	r0, r0, r0
    6664:	001d0044 	andseq	r0, sp, r4, asr #32
	...
    6670:	001e0044 	andseq	r0, lr, r4, asr #32
    6674:	0000000c 	andeq	r0, r0, ip
    6678:	00000000 	andeq	r0, r0, r0
    667c:	001f0044 	andseq	r0, pc, r4, asr #32
    6680:	00000020 	andeq	r0, r0, r0, lsr #32
    6684:	00000000 	andeq	r0, r0, r0
    6688:	00200044 	eoreq	r0, r0, r4, asr #32
    668c:	00000034 	andeq	r0, r0, r4, lsr r0
    6690:	00000000 	andeq	r0, r0, r0
    6694:	00210044 	eoreq	r0, r1, r4, asr #32
    6698:	00000048 	andeq	r0, r0, r8, asr #32
    669c:	00000000 	andeq	r0, r0, r0
    66a0:	00000024 	andeq	r0, r0, r4, lsr #32
    66a4:	00000064 	andeq	r0, r0, r4, rrx
    66a8:	00000000 	andeq	r0, r0, r0
    66ac:	0000004e 	andeq	r0, r0, lr, asr #32
    66b0:	80013040 	andhi	r3, r1, r0, asr #32
    66b4:	00002ce0 	andeq	r2, r0, r0, ror #25
    66b8:	00230024 	eoreq	r0, r3, r4, lsr #32
    66bc:	80013040 	andhi	r3, r1, r0, asr #32
    66c0:	00002b55 	andeq	r2, r0, r5, asr fp
    66c4:	002300a0 	eoreq	r0, r3, r0, lsr #1
    66c8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    66cc:	00000000 	andeq	r0, r0, r0
    66d0:	0000002e 	andeq	r0, r0, lr, lsr #32
    66d4:	80013040 	andhi	r3, r1, r0, asr #32
    66d8:	00000000 	andeq	r0, r0, r0
    66dc:	00230044 	eoreq	r0, r3, r4, asr #32
	...
    66e8:	00240044 	eoreq	r0, r4, r4, asr #32
    66ec:	00000014 	andeq	r0, r0, r4, lsl r0
    66f0:	00000000 	andeq	r0, r0, r0
    66f4:	00250044 	eoreq	r0, r5, r4, asr #32
    66f8:	0000002c 	andeq	r0, r0, ip, lsr #32
    66fc:	00000000 	andeq	r0, r0, r0
    6700:	00000024 	andeq	r0, r0, r4, lsr #32
    6704:	0000003c 	andeq	r0, r0, ip, lsr r0
    6708:	00000000 	andeq	r0, r0, r0
    670c:	0000004e 	andeq	r0, r0, lr, asr #32
    6710:	8001307c 	andhi	r3, r1, ip, ror r0
    6714:	00002cef 	andeq	r2, r0, pc, ror #25
    6718:	00270024 	eoreq	r0, r7, r4, lsr #32
    671c:	8001307c 	andhi	r3, r1, ip, ror r0
    6720:	00002cfe 	strdeq	r2, [r0], -lr
    6724:	002700a0 	eoreq	r0, r7, r0, lsr #1
    6728:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    672c:	00000000 	andeq	r0, r0, r0
    6730:	0000002e 	andeq	r0, r0, lr, lsr #32
    6734:	8001307c 	andhi	r3, r1, ip, ror r0
    6738:	00000000 	andeq	r0, r0, r0
    673c:	00270044 	eoreq	r0, r7, r4, asr #32
	...
    6748:	00280044 	eoreq	r0, r8, r4, asr #32
    674c:	00000014 	andeq	r0, r0, r4, lsl r0
    6750:	00000000 	andeq	r0, r0, r0
    6754:	00290044 	eoreq	r0, r9, r4, asr #32
    6758:	00000028 	andeq	r0, r0, r8, lsr #32
    675c:	00000000 	andeq	r0, r0, r0
    6760:	00000024 	andeq	r0, r0, r4, lsr #32
    6764:	00000034 	andeq	r0, r0, r4, lsr r0
    6768:	00000000 	andeq	r0, r0, r0
    676c:	0000004e 	andeq	r0, r0, lr, asr #32
    6770:	800130b0 	strhhi	r3, [r1], -r0
    6774:	00002d07 	andeq	r2, r0, r7, lsl #26
    6778:	00060028 	andeq	r0, r6, r8, lsr #32
    677c:	80027008 	andhi	r7, r2, r8
    6780:	00002d17 	andeq	r2, r0, r7, lsl sp
    6784:	00070026 	andeq	r0, r7, r6, lsr #32
    6788:	800200ac 	andhi	r0, r2, ip, lsr #1
    678c:	00000000 	andeq	r0, r0, r0
    6790:	00000064 	andeq	r0, r0, r4, rrx
    6794:	800130b0 	strhhi	r3, [r1], -r0
    6798:	00000007 	andeq	r0, r0, r7
    679c:	00020064 	andeq	r0, r2, r4, rrx
    67a0:	800130b0 	strhhi	r3, [r1], -r0
    67a4:	00002d2e 	andeq	r2, r0, lr, lsr #26
    67a8:	00020064 	andeq	r0, r2, r4, rrx
    67ac:	800130b0 	strhhi	r3, [r1], -r0
    67b0:	0000003d 	andeq	r0, r0, sp, lsr r0
    67b4:	0000003c 	andeq	r0, r0, ip, lsr r0
    67b8:	00000000 	andeq	r0, r0, r0
    67bc:	0000004c 	andeq	r0, r0, ip, asr #32
    67c0:	00000080 	andeq	r0, r0, r0, lsl #1
    67c4:	00000000 	andeq	r0, r0, r0
    67c8:	00000076 	andeq	r0, r0, r6, ror r0
    67cc:	00000080 	andeq	r0, r0, r0, lsl #1
    67d0:	00000000 	andeq	r0, r0, r0
    67d4:	00000094 	muleq	r0, r4, r0
    67d8:	00000080 	andeq	r0, r0, r0, lsl #1
    67dc:	00000000 	andeq	r0, r0, r0
    67e0:	000000c3 	andeq	r0, r0, r3, asr #1
    67e4:	00000080 	andeq	r0, r0, r0, lsl #1
    67e8:	00000000 	andeq	r0, r0, r0
    67ec:	000000ee 	andeq	r0, r0, lr, ror #1
    67f0:	00000080 	andeq	r0, r0, r0, lsl #1
    67f4:	00000000 	andeq	r0, r0, r0
    67f8:	0000011e 	andeq	r0, r0, lr, lsl r1
    67fc:	00000080 	andeq	r0, r0, r0, lsl #1
    6800:	00000000 	andeq	r0, r0, r0
    6804:	0000016f 	andeq	r0, r0, pc, ror #2
    6808:	00000080 	andeq	r0, r0, r0, lsl #1
    680c:	00000000 	andeq	r0, r0, r0
    6810:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    6814:	00000080 	andeq	r0, r0, r0, lsl #1
    6818:	00000000 	andeq	r0, r0, r0
    681c:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    6820:	00000080 	andeq	r0, r0, r0, lsl #1
    6824:	00000000 	andeq	r0, r0, r0
    6828:	0000020e 	andeq	r0, r0, lr, lsl #4
    682c:	00000080 	andeq	r0, r0, r0, lsl #1
    6830:	00000000 	andeq	r0, r0, r0
    6834:	00000238 	andeq	r0, r0, r8, lsr r2
    6838:	00000080 	andeq	r0, r0, r0, lsl #1
    683c:	00000000 	andeq	r0, r0, r0
    6840:	00000261 	andeq	r0, r0, r1, ror #4
    6844:	00000080 	andeq	r0, r0, r0, lsl #1
    6848:	00000000 	andeq	r0, r0, r0
    684c:	0000027b 	andeq	r0, r0, fp, ror r2
    6850:	00000080 	andeq	r0, r0, r0, lsl #1
    6854:	00000000 	andeq	r0, r0, r0
    6858:	00000296 	muleq	r0, r6, r2
    685c:	00000080 	andeq	r0, r0, r0, lsl #1
    6860:	00000000 	andeq	r0, r0, r0
    6864:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    6868:	00000080 	andeq	r0, r0, r0, lsl #1
    686c:	00000000 	andeq	r0, r0, r0
    6870:	000002d7 	ldrdeq	r0, [r0], -r7
    6874:	00000080 	andeq	r0, r0, r0, lsl #1
    6878:	00000000 	andeq	r0, r0, r0
    687c:	000002f7 	strdeq	r0, [r0], -r7
    6880:	00000080 	andeq	r0, r0, r0, lsl #1
    6884:	00000000 	andeq	r0, r0, r0
    6888:	0000031c 	andeq	r0, r0, ip, lsl r3
    688c:	00000080 	andeq	r0, r0, r0, lsl #1
    6890:	00000000 	andeq	r0, r0, r0
    6894:	00000346 	andeq	r0, r0, r6, asr #6
    6898:	00000080 	andeq	r0, r0, r0, lsl #1
    689c:	00000000 	andeq	r0, r0, r0
    68a0:	0000036a 	andeq	r0, r0, sl, ror #6
    68a4:	00000080 	andeq	r0, r0, r0, lsl #1
    68a8:	00000000 	andeq	r0, r0, r0
    68ac:	00000393 	muleq	r0, r3, r3
    68b0:	00000080 	andeq	r0, r0, r0, lsl #1
    68b4:	00000000 	andeq	r0, r0, r0
    68b8:	000003c1 	andeq	r0, r0, r1, asr #7
    68bc:	00000080 	andeq	r0, r0, r0, lsl #1
    68c0:	00000000 	andeq	r0, r0, r0
    68c4:	000003e7 	andeq	r0, r0, r7, ror #7
    68c8:	00000080 	andeq	r0, r0, r0, lsl #1
    68cc:	00000000 	andeq	r0, r0, r0
    68d0:	00000407 	andeq	r0, r0, r7, lsl #8
    68d4:	00000080 	andeq	r0, r0, r0, lsl #1
    68d8:	00000000 	andeq	r0, r0, r0
    68dc:	0000042c 	andeq	r0, r0, ip, lsr #8
    68e0:	00000080 	andeq	r0, r0, r0, lsl #1
    68e4:	00000000 	andeq	r0, r0, r0
    68e8:	00000456 	andeq	r0, r0, r6, asr r4
    68ec:	00000080 	andeq	r0, r0, r0, lsl #1
    68f0:	00000000 	andeq	r0, r0, r0
    68f4:	00000485 	andeq	r0, r0, r5, lsl #9
    68f8:	00000080 	andeq	r0, r0, r0, lsl #1
    68fc:	00000000 	andeq	r0, r0, r0
    6900:	000004ae 	andeq	r0, r0, lr, lsr #9
    6904:	00000080 	andeq	r0, r0, r0, lsl #1
    6908:	00000000 	andeq	r0, r0, r0
    690c:	000004dc 	ldrdeq	r0, [r0], -ip
    6910:	00000080 	andeq	r0, r0, r0, lsl #1
    6914:	00000000 	andeq	r0, r0, r0
    6918:	0000050f 	andeq	r0, r0, pc, lsl #10
    691c:	00000080 	andeq	r0, r0, r0, lsl #1
    6920:	00000000 	andeq	r0, r0, r0
    6924:	00000530 	andeq	r0, r0, r0, lsr r5
    6928:	00000080 	andeq	r0, r0, r0, lsl #1
    692c:	00000000 	andeq	r0, r0, r0
    6930:	00000550 	andeq	r0, r0, r0, asr r5
    6934:	00000080 	andeq	r0, r0, r0, lsl #1
    6938:	00000000 	andeq	r0, r0, r0
    693c:	00000575 	andeq	r0, r0, r5, ror r5
    6940:	00000080 	andeq	r0, r0, r0, lsl #1
    6944:	00000000 	andeq	r0, r0, r0
    6948:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    694c:	00000080 	andeq	r0, r0, r0, lsl #1
    6950:	00000000 	andeq	r0, r0, r0
    6954:	000005c3 	andeq	r0, r0, r3, asr #11
    6958:	00000080 	andeq	r0, r0, r0, lsl #1
    695c:	00000000 	andeq	r0, r0, r0
    6960:	000005ec 	andeq	r0, r0, ip, ror #11
    6964:	00000080 	andeq	r0, r0, r0, lsl #1
    6968:	00000000 	andeq	r0, r0, r0
    696c:	0000061a 	andeq	r0, r0, sl, lsl r6
    6970:	00000080 	andeq	r0, r0, r0, lsl #1
    6974:	00000000 	andeq	r0, r0, r0
    6978:	00000640 	andeq	r0, r0, r0, asr #12
    697c:	00000080 	andeq	r0, r0, r0, lsl #1
    6980:	00000000 	andeq	r0, r0, r0
    6984:	00000660 	andeq	r0, r0, r0, ror #12
    6988:	00000080 	andeq	r0, r0, r0, lsl #1
    698c:	00000000 	andeq	r0, r0, r0
    6990:	00000685 	andeq	r0, r0, r5, lsl #13
    6994:	00000080 	andeq	r0, r0, r0, lsl #1
    6998:	00000000 	andeq	r0, r0, r0
    699c:	000006af 	andeq	r0, r0, pc, lsr #13
    69a0:	00000080 	andeq	r0, r0, r0, lsl #1
    69a4:	00000000 	andeq	r0, r0, r0
    69a8:	000006de 	ldrdeq	r0, [r0], -lr
    69ac:	00000080 	andeq	r0, r0, r0, lsl #1
    69b0:	00000000 	andeq	r0, r0, r0
    69b4:	00000707 	andeq	r0, r0, r7, lsl #14
    69b8:	00000080 	andeq	r0, r0, r0, lsl #1
    69bc:	00000000 	andeq	r0, r0, r0
    69c0:	00000735 	andeq	r0, r0, r5, lsr r7
    69c4:	00000080 	andeq	r0, r0, r0, lsl #1
    69c8:	00000000 	andeq	r0, r0, r0
    69cc:	00000768 	andeq	r0, r0, r8, ror #14
    69d0:	00000080 	andeq	r0, r0, r0, lsl #1
    69d4:	00000000 	andeq	r0, r0, r0
    69d8:	00000886 	andeq	r0, r0, r6, lsl #17
    69dc:	000000c2 	andeq	r0, r0, r2, asr #1
    69e0:	00003ac8 	andeq	r3, r0, r8, asr #21
    69e4:	0000078a 	andeq	r0, r0, sl, lsl #15
    69e8:	000000c2 	andeq	r0, r0, r2, asr #1
    69ec:	00002d60 	andeq	r2, r0, r0, ror #26
    69f0:	00001000 	andeq	r1, r0, r0
    69f4:	000000c2 	andeq	r0, r0, r2, asr #1
    69f8:	0000100f 	andeq	r1, r0, pc
    69fc:	00001173 	andeq	r1, r0, r3, ror r1
    6a00:	000000c2 	andeq	r0, r0, r2, asr #1
    6a04:	0000c9ad 	andeq	ip, r0, sp, lsr #19
    6a08:	00001182 	andeq	r1, r0, r2, lsl #3
    6a0c:	000000c2 	andeq	r0, r0, r2, asr #1
    6a10:	00004df6 	strdeq	r4, [r0], -r6
    6a14:	000012f3 	strdeq	r1, [r0], -r3
    6a18:	000000c2 	andeq	r0, r0, r2, asr #1
    6a1c:	00002a19 	andeq	r2, r0, r9, lsl sl
    6a20:	00001306 	andeq	r1, r0, r6, lsl #6
    6a24:	000000c2 	andeq	r0, r0, r2, asr #1
    6a28:	0000151b 	andeq	r1, r0, fp, lsl r5
    6a2c:	00001425 	andeq	r1, r0, r5, lsr #8
    6a30:	000000c2 	andeq	r0, r0, r2, asr #1
    6a34:	00001fc5 	andeq	r1, r0, r5, asr #31
    6a38:	000014c3 	andeq	r1, r0, r3, asr #9
    6a3c:	000000c2 	andeq	r0, r0, r2, asr #1
    6a40:	00002743 	andeq	r2, r0, r3, asr #14
    6a44:	00001056 	andeq	r1, r0, r6, asr r0
    6a48:	000000c2 	andeq	r0, r0, r2, asr #1
    6a4c:	00000000 	andeq	r0, r0, r0
    6a50:	00001067 	andeq	r1, r0, r7, rrx
    6a54:	000000c2 	andeq	r0, r0, r2, asr #1
    6a58:	00002f44 	andeq	r2, r0, r4, asr #30
    6a5c:	00002d39 	andeq	r2, r0, r9, lsr sp
    6a60:	00000082 	andeq	r0, r0, r2, lsl #1
    6a64:	000095eb 	andeq	r9, r0, fp, ror #11
    6a68:	00002d47 	andeq	r2, r0, r7, asr #26
    6a6c:	00000080 	andeq	r0, r0, r0, lsl #1
    6a70:	00000000 	andeq	r0, r0, r0
    6a74:	00002d98 	muleq	r0, r8, sp
    6a78:	00000080 	andeq	r0, r0, r0, lsl #1
    6a7c:	00000000 	andeq	r0, r0, r0
    6a80:	00002eea 	andeq	r2, r0, sl, ror #29
    6a84:	00000080 	andeq	r0, r0, r0, lsl #1
	...
    6a90:	000000a2 	andeq	r0, r0, r2, lsr #1
    6a94:	00000000 	andeq	r0, r0, r0
    6a98:	00002f99 	muleq	r0, r9, pc	; <UNPREDICTABLE>
    6a9c:	00150024 	andseq	r0, r5, r4, lsr #32
    6aa0:	800130b0 	strhhi	r3, [r1], -r0
    6aa4:	00000000 	andeq	r0, r0, r0
    6aa8:	0000002e 	andeq	r0, r0, lr, lsr #32
    6aac:	800130b0 	strhhi	r3, [r1], -r0
    6ab0:	00000000 	andeq	r0, r0, r0
    6ab4:	00160044 	andseq	r0, r6, r4, asr #32
	...
    6ac0:	00170044 	andseq	r0, r7, r4, asr #32
    6ac4:	00000018 	andeq	r0, r0, r8, lsl r0
    6ac8:	00000000 	andeq	r0, r0, r0
    6acc:	00180044 	andseq	r0, r8, r4, asr #32
    6ad0:	00000024 	andeq	r0, r0, r4, lsr #32
    6ad4:	00000000 	andeq	r0, r0, r0
    6ad8:	00170044 	andseq	r0, r7, r4, asr #32
    6adc:	00000044 	andeq	r0, r0, r4, asr #32
    6ae0:	00000000 	andeq	r0, r0, r0
    6ae4:	00170044 	andseq	r0, r7, r4, asr #32
    6ae8:	00000050 	andeq	r0, r0, r0, asr r0
    6aec:	00000000 	andeq	r0, r0, r0
    6af0:	00190044 	andseq	r0, r9, r4, asr #32
    6af4:	0000005c 	andeq	r0, r0, ip, asr r0
    6af8:	00000000 	andeq	r0, r0, r0
    6afc:	001a0044 	andseq	r0, sl, r4, asr #32
    6b00:	0000006c 	andeq	r0, r0, ip, rrx
    6b04:	00000ecb 	andeq	r0, r0, fp, asr #29
    6b08:	00170080 	andseq	r0, r7, r0, lsl #1
    6b0c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    6b10:	00000000 	andeq	r0, r0, r0
    6b14:	000000c0 	andeq	r0, r0, r0, asr #1
    6b18:	00000018 	andeq	r0, r0, r8, lsl r0
    6b1c:	00000000 	andeq	r0, r0, r0
    6b20:	000000e0 	andeq	r0, r0, r0, ror #1
    6b24:	0000005c 	andeq	r0, r0, ip, asr r0
    6b28:	00000000 	andeq	r0, r0, r0
    6b2c:	00000024 	andeq	r0, r0, r4, lsr #32
    6b30:	00000080 	andeq	r0, r0, r0, lsl #1
    6b34:	00000000 	andeq	r0, r0, r0
    6b38:	0000004e 	andeq	r0, r0, lr, asr #32
    6b3c:	80013130 	andhi	r3, r1, r0, lsr r1
    6b40:	00002faa 	andeq	r2, r0, sl, lsr #31
    6b44:	001d0024 	andseq	r0, sp, r4, lsr #32
    6b48:	80013130 	andhi	r3, r1, r0, lsr r1
    6b4c:	00002fc2 	andeq	r2, r0, r2, asr #31
    6b50:	001d00a0 	andseq	r0, sp, r0, lsr #1
    6b54:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    6b58:	00002fcb 	andeq	r2, r0, fp, asr #31
    6b5c:	001d00a0 	andseq	r0, sp, r0, lsr #1
    6b60:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    6b64:	00000000 	andeq	r0, r0, r0
    6b68:	0000002e 	andeq	r0, r0, lr, lsr #32
    6b6c:	80013130 	andhi	r3, r1, r0, lsr r1
    6b70:	00000000 	andeq	r0, r0, r0
    6b74:	001e0044 	andseq	r0, lr, r4, asr #32
	...
    6b80:	001f0044 	andseq	r0, pc, r4, asr #32
    6b84:	00000018 	andeq	r0, r0, r8, lsl r0
    6b88:	00000000 	andeq	r0, r0, r0
    6b8c:	00200044 	eoreq	r0, r0, r4, asr #32
    6b90:	00000020 	andeq	r0, r0, r0, lsr #32
    6b94:	00000000 	andeq	r0, r0, r0
    6b98:	00210044 	eoreq	r0, r1, r4, asr #32
    6b9c:	0000002c 	andeq	r0, r0, ip, lsr #32
    6ba0:	00000000 	andeq	r0, r0, r0
    6ba4:	00220044 	eoreq	r0, r2, r4, asr #32
    6ba8:	00000034 	andeq	r0, r0, r4, lsr r0
    6bac:	00000000 	andeq	r0, r0, r0
    6bb0:	00230044 	eoreq	r0, r3, r4, asr #32
    6bb4:	00000040 	andeq	r0, r0, r0, asr #32
    6bb8:	00000000 	andeq	r0, r0, r0
    6bbc:	00240044 	eoreq	r0, r4, r4, asr #32
    6bc0:	0000004c 	andeq	r0, r0, ip, asr #32
    6bc4:	00000000 	andeq	r0, r0, r0
    6bc8:	00260044 	eoreq	r0, r6, r4, asr #32
    6bcc:	00000054 	andeq	r0, r0, r4, asr r0
    6bd0:	00000000 	andeq	r0, r0, r0
    6bd4:	00280044 	eoreq	r0, r8, r4, asr #32
    6bd8:	00000064 	andeq	r0, r0, r4, rrx
    6bdc:	00000000 	andeq	r0, r0, r0
    6be0:	002a0044 	eoreq	r0, sl, r4, asr #32
    6be4:	00000074 	andeq	r0, r0, r4, ror r0
    6be8:	00000000 	andeq	r0, r0, r0
    6bec:	00280044 	eoreq	r0, r8, r4, asr #32
    6bf0:	00000078 	andeq	r0, r0, r8, ror r0
    6bf4:	00000000 	andeq	r0, r0, r0
    6bf8:	002c0044 	eoreq	r0, ip, r4, asr #32
    6bfc:	00000090 	muleq	r0, r0, r0
    6c00:	00000000 	andeq	r0, r0, r0
    6c04:	00200044 	eoreq	r0, r0, r4, asr #32
    6c08:	000000a4 	andeq	r0, r0, r4, lsr #1
    6c0c:	00000000 	andeq	r0, r0, r0
    6c10:	00200044 	eoreq	r0, r0, r4, asr #32
    6c14:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    6c18:	00000000 	andeq	r0, r0, r0
    6c1c:	002e0044 	eoreq	r0, lr, r4, asr #32
    6c20:	000000c0 	andeq	r0, r0, r0, asr #1
    6c24:	00000000 	andeq	r0, r0, r0
    6c28:	002f0044 	eoreq	r0, pc, r4, asr #32
    6c2c:	000000c4 	andeq	r0, r0, r4, asr #1
    6c30:	0000234e 	andeq	r2, r0, lr, asr #6
    6c34:	001f0080 	andseq	r0, pc, r0, lsl #1
    6c38:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    6c3c:	00000000 	andeq	r0, r0, r0
    6c40:	000000c0 	andeq	r0, r0, r0, asr #1
    6c44:	00000000 	andeq	r0, r0, r0
    6c48:	00000ecb 	andeq	r0, r0, fp, asr #29
    6c4c:	00200080 	eoreq	r0, r0, r0, lsl #1
    6c50:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    6c54:	00000000 	andeq	r0, r0, r0
    6c58:	000000c0 	andeq	r0, r0, r0, asr #1
    6c5c:	00000020 	andeq	r0, r0, r0, lsr #32
    6c60:	00002fdc 	ldrdeq	r2, [r0], -ip
    6c64:	00210080 	eoreq	r0, r1, r0, lsl #1
    6c68:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    6c6c:	00000000 	andeq	r0, r0, r0
    6c70:	000000c0 	andeq	r0, r0, r0, asr #1
    6c74:	0000002c 	andeq	r0, r0, ip, lsr #32
    6c78:	00000000 	andeq	r0, r0, r0
    6c7c:	000000e0 	andeq	r0, r0, r0, ror #1
    6c80:	000000a4 	andeq	r0, r0, r4, lsr #1
    6c84:	00000000 	andeq	r0, r0, r0
    6c88:	000000e0 	andeq	r0, r0, r0, ror #1
    6c8c:	000000c0 	andeq	r0, r0, r0, asr #1
    6c90:	00000000 	andeq	r0, r0, r0
    6c94:	000000e0 	andeq	r0, r0, r0, ror #1
    6c98:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    6c9c:	00000000 	andeq	r0, r0, r0
    6ca0:	00000024 	andeq	r0, r0, r4, lsr #32
    6ca4:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    6ca8:	00000000 	andeq	r0, r0, r0
    6cac:	0000004e 	andeq	r0, r0, lr, asr #32
    6cb0:	80013200 	andhi	r3, r1, r0, lsl #4
    6cb4:	00002fe7 	andeq	r2, r0, r7, ror #31
    6cb8:	00320024 	eorseq	r0, r2, r4, lsr #32
    6cbc:	80013200 	andhi	r3, r1, r0, lsl #4
    6cc0:	00002fc2 	andeq	r2, r0, r2, asr #31
    6cc4:	003200a0 	eorseq	r0, r2, r0, lsr #1
    6cc8:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    6ccc:	00002fcb 	andeq	r2, r0, fp, asr #31
    6cd0:	003200a0 	eorseq	r0, r2, r0, lsr #1
    6cd4:	ffffffd4 			; <UNDEFINED> instruction: 0xffffffd4
    6cd8:	00000000 	andeq	r0, r0, r0
    6cdc:	0000002e 	andeq	r0, r0, lr, lsr #32
    6ce0:	80013200 	andhi	r3, r1, r0, lsl #4
    6ce4:	00000000 	andeq	r0, r0, r0
    6ce8:	00330044 	eorseq	r0, r3, r4, asr #32
	...
    6cf4:	00340044 	eorseq	r0, r4, r4, asr #32
    6cf8:	00000018 	andeq	r0, r0, r8, lsl r0
    6cfc:	00000000 	andeq	r0, r0, r0
    6d00:	00350044 	eorseq	r0, r5, r4, asr #32
    6d04:	00000020 	andeq	r0, r0, r0, lsr #32
    6d08:	00000000 	andeq	r0, r0, r0
    6d0c:	00360044 	eorseq	r0, r6, r4, asr #32
    6d10:	0000002c 	andeq	r0, r0, ip, lsr #32
    6d14:	00000000 	andeq	r0, r0, r0
    6d18:	00370044 	eorseq	r0, r7, r4, asr #32
    6d1c:	0000003c 	andeq	r0, r0, ip, lsr r0
    6d20:	00000000 	andeq	r0, r0, r0
    6d24:	003a0044 	eorseq	r0, sl, r4, asr #32
    6d28:	00000054 	andeq	r0, r0, r4, asr r0
    6d2c:	00000000 	andeq	r0, r0, r0
    6d30:	003b0044 	eorseq	r0, fp, r4, asr #32
    6d34:	00000060 	andeq	r0, r0, r0, rrx
    6d38:	00000000 	andeq	r0, r0, r0
    6d3c:	003d0044 	eorseq	r0, sp, r4, asr #32
    6d40:	0000006c 	andeq	r0, r0, ip, rrx
    6d44:	00000000 	andeq	r0, r0, r0
    6d48:	003f0044 	eorseq	r0, pc, r4, asr #32
    6d4c:	00000080 	andeq	r0, r0, r0, lsl #1
    6d50:	00000000 	andeq	r0, r0, r0
    6d54:	00400044 	subeq	r0, r0, r4, asr #32
    6d58:	00000094 	muleq	r0, r4, r0
    6d5c:	00000000 	andeq	r0, r0, r0
    6d60:	00410044 	subeq	r0, r1, r4, asr #32
    6d64:	000000a4 	andeq	r0, r0, r4, lsr #1
    6d68:	00000000 	andeq	r0, r0, r0
    6d6c:	00350044 	eorseq	r0, r5, r4, asr #32
    6d70:	000000a8 	andeq	r0, r0, r8, lsr #1
    6d74:	00000000 	andeq	r0, r0, r0
    6d78:	00350044 	eorseq	r0, r5, r4, asr #32
    6d7c:	000000b4 	strheq	r0, [r0], -r4
    6d80:	00000000 	andeq	r0, r0, r0
    6d84:	00440044 	subeq	r0, r4, r4, asr #32
    6d88:	000000c4 	andeq	r0, r0, r4, asr #1
    6d8c:	000024e2 	andeq	r2, r0, r2, ror #9
    6d90:	00340080 	eorseq	r0, r4, r0, lsl #1
    6d94:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    6d98:	00000000 	andeq	r0, r0, r0
    6d9c:	000000c0 	andeq	r0, r0, r0, asr #1
    6da0:	00000000 	andeq	r0, r0, r0
    6da4:	00000ecb 	andeq	r0, r0, fp, asr #29
    6da8:	00350080 	eorseq	r0, r5, r0, lsl #1
    6dac:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    6db0:	00000000 	andeq	r0, r0, r0
    6db4:	000000c0 	andeq	r0, r0, r0, asr #1
    6db8:	00000020 	andeq	r0, r0, r0, lsr #32
    6dbc:	00003000 	andeq	r3, r0, r0
    6dc0:	00360080 	eorseq	r0, r6, r0, lsl #1
    6dc4:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    6dc8:	00003012 	andeq	r3, r0, r2, lsl r0
    6dcc:	00370080 	eorseq	r0, r7, r0, lsl #1
    6dd0:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    6dd4:	00003025 	andeq	r3, r0, r5, lsr #32
    6dd8:	003a0080 	eorseq	r0, sl, r0, lsl #1
    6ddc:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    6de0:	00000000 	andeq	r0, r0, r0
    6de4:	000000c0 	andeq	r0, r0, r0, asr #1
    6de8:	0000002c 	andeq	r0, r0, ip, lsr #32
    6dec:	00000000 	andeq	r0, r0, r0
    6df0:	000000e0 	andeq	r0, r0, r0, ror #1
    6df4:	000000a8 	andeq	r0, r0, r8, lsr #1
    6df8:	00000000 	andeq	r0, r0, r0
    6dfc:	000000e0 	andeq	r0, r0, r0, ror #1
    6e00:	000000c4 	andeq	r0, r0, r4, asr #1
    6e04:	00000000 	andeq	r0, r0, r0
    6e08:	000000e0 	andeq	r0, r0, r0, ror #1
    6e0c:	000000cc 	andeq	r0, r0, ip, asr #1
    6e10:	00000000 	andeq	r0, r0, r0
    6e14:	00000024 	andeq	r0, r0, r4, lsr #32
    6e18:	000000cc 	andeq	r0, r0, ip, asr #1
    6e1c:	00000000 	andeq	r0, r0, r0
    6e20:	0000004e 	andeq	r0, r0, lr, asr #32
    6e24:	800132cc 	andhi	r3, r1, ip, asr #5
    6e28:	00003036 	andeq	r3, r0, r6, lsr r0
    6e2c:	00460024 	subeq	r0, r6, r4, lsr #32
    6e30:	800132cc 	andhi	r3, r1, ip, asr #5
    6e34:	00002fc2 	andeq	r2, r0, r2, asr #31
    6e38:	004600a0 	subeq	r0, r6, r0, lsr #1
    6e3c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    6e40:	00000000 	andeq	r0, r0, r0
    6e44:	0000002e 	andeq	r0, r0, lr, lsr #32
    6e48:	800132cc 	andhi	r3, r1, ip, asr #5
    6e4c:	00000000 	andeq	r0, r0, r0
    6e50:	00460044 	subeq	r0, r6, r4, asr #32
	...
    6e5c:	00470044 	subeq	r0, r7, r4, asr #32
    6e60:	00000014 	andeq	r0, r0, r4, lsl r0
    6e64:	00000000 	andeq	r0, r0, r0
    6e68:	00480044 	subeq	r0, r8, r4, asr #32
    6e6c:	0000001c 	andeq	r0, r0, ip, lsl r0
    6e70:	00000000 	andeq	r0, r0, r0
    6e74:	00490044 	subeq	r0, r9, r4, asr #32
    6e78:	00000024 	andeq	r0, r0, r4, lsr #32
    6e7c:	000024e2 	andeq	r2, r0, r2, ror #9
    6e80:	00470080 	subeq	r0, r7, r0, lsl #1
    6e84:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    6e88:	00000000 	andeq	r0, r0, r0
    6e8c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    6e98:	000000e0 	andeq	r0, r0, r0, ror #1
    6e9c:	00000030 	andeq	r0, r0, r0, lsr r0
    6ea0:	00000000 	andeq	r0, r0, r0
    6ea4:	00000024 	andeq	r0, r0, r4, lsr #32
    6ea8:	00000030 	andeq	r0, r0, r0, lsr r0
    6eac:	00000000 	andeq	r0, r0, r0
    6eb0:	0000004e 	andeq	r0, r0, lr, asr #32
    6eb4:	800132fc 	strdhi	r3, [r1], -ip
    6eb8:	0000304c 	andeq	r3, r0, ip, asr #32
    6ebc:	004c0024 	subeq	r0, ip, r4, lsr #32
    6ec0:	800132fc 	strdhi	r3, [r1], -ip
    6ec4:	00000000 	andeq	r0, r0, r0
    6ec8:	0000002e 	andeq	r0, r0, lr, lsr #32
    6ecc:	800132fc 	strdhi	r3, [r1], -ip
    6ed0:	00000000 	andeq	r0, r0, r0
    6ed4:	004d0044 	subeq	r0, sp, r4, asr #32
	...
    6ee0:	00500044 	subseq	r0, r0, r4, asr #32
    6ee4:	00000018 	andeq	r0, r0, r8, lsl r0
    6ee8:	00000000 	andeq	r0, r0, r0
    6eec:	00510044 	subseq	r0, r1, r4, asr #32
    6ef0:	00000020 	andeq	r0, r0, r0, lsr #32
    6ef4:	00000000 	andeq	r0, r0, r0
    6ef8:	00520044 	subseq	r0, r2, r4, asr #32
    6efc:	00000028 	andeq	r0, r0, r8, lsr #32
    6f00:	00000000 	andeq	r0, r0, r0
    6f04:	00540044 	subseq	r0, r4, r4, asr #32
    6f08:	00000030 	andeq	r0, r0, r0, lsr r0
    6f0c:	00000000 	andeq	r0, r0, r0
    6f10:	00550044 	subseq	r0, r5, r4, asr #32
    6f14:	0000003c 	andeq	r0, r0, ip, lsr r0
    6f18:	00000000 	andeq	r0, r0, r0
    6f1c:	00560044 	subseq	r0, r6, r4, asr #32
    6f20:	00000060 	andeq	r0, r0, r0, rrx
    6f24:	00000000 	andeq	r0, r0, r0
    6f28:	00570044 	subseq	r0, r7, r4, asr #32
    6f2c:	00000070 	andeq	r0, r0, r0, ror r0
    6f30:	00000000 	andeq	r0, r0, r0
    6f34:	00540044 	subseq	r0, r4, r4, asr #32
    6f38:	00000074 	andeq	r0, r0, r4, ror r0
    6f3c:	00000000 	andeq	r0, r0, r0
    6f40:	00540044 	subseq	r0, r4, r4, asr #32
    6f44:	00000080 	andeq	r0, r0, r0, lsl #1
    6f48:	00000000 	andeq	r0, r0, r0
    6f4c:	005b0044 	subseq	r0, fp, r4, asr #32
    6f50:	0000008c 	andeq	r0, r0, ip, lsl #1
    6f54:	00000000 	andeq	r0, r0, r0
    6f58:	005c0044 	subseq	r0, ip, r4, asr #32
    6f5c:	000000a0 	andeq	r0, r0, r0, lsr #1
    6f60:	00000000 	andeq	r0, r0, r0
    6f64:	005d0044 	subseq	r0, sp, r4, asr #32
    6f68:	000000a8 	andeq	r0, r0, r8, lsr #1
    6f6c:	00000000 	andeq	r0, r0, r0
    6f70:	005f0044 	subseq	r0, pc, r4, asr #32
    6f74:	000000d4 	ldrdeq	r0, [r0], -r4
    6f78:	00000000 	andeq	r0, r0, r0
    6f7c:	00600044 	rsbeq	r0, r0, r4, asr #32
    6f80:	000000dc 	ldrdeq	r0, [r0], -ip
    6f84:	00000000 	andeq	r0, r0, r0
    6f88:	00620044 	rsbeq	r0, r2, r4, asr #32
    6f8c:	000000e4 	andeq	r0, r0, r4, ror #1
    6f90:	00000000 	andeq	r0, r0, r0
    6f94:	00630044 	rsbeq	r0, r3, r4, asr #32
    6f98:	00000104 	andeq	r0, r0, r4, lsl #2
    6f9c:	00000000 	andeq	r0, r0, r0
    6fa0:	00670044 	rsbeq	r0, r7, r4, asr #32
    6fa4:	0000010c 	andeq	r0, r0, ip, lsl #2
    6fa8:	00000000 	andeq	r0, r0, r0
    6fac:	00650044 	rsbeq	r0, r5, r4, asr #32
    6fb0:	00000110 	andeq	r0, r0, r0, lsl r1
    6fb4:	00000000 	andeq	r0, r0, r0
    6fb8:	006c0044 	rsbeq	r0, ip, r4, asr #32
    6fbc:	0000012c 	andeq	r0, r0, ip, lsr #2
    6fc0:	00000000 	andeq	r0, r0, r0
    6fc4:	006a0044 	rsbeq	r0, sl, r4, asr #32
    6fc8:	00000130 	andeq	r0, r0, r0, lsr r1
    6fcc:	00000000 	andeq	r0, r0, r0
    6fd0:	006f0044 	rsbeq	r0, pc, r4, asr #32
    6fd4:	0000014c 	andeq	r0, r0, ip, asr #2
    6fd8:	00000000 	andeq	r0, r0, r0
    6fdc:	00700044 	rsbseq	r0, r0, r4, asr #32
    6fe0:	00000168 	andeq	r0, r0, r8, ror #2
    6fe4:	00000000 	andeq	r0, r0, r0
    6fe8:	00710044 	rsbseq	r0, r1, r4, asr #32
    6fec:	0000017c 	andeq	r0, r0, ip, ror r1
    6ff0:	00000000 	andeq	r0, r0, r0
    6ff4:	00720044 	rsbseq	r0, r2, r4, asr #32
    6ff8:	00000190 	muleq	r0, r0, r1
    6ffc:	00000000 	andeq	r0, r0, r0
    7000:	00730044 	rsbseq	r0, r3, r4, asr #32
    7004:	000001a4 	andeq	r0, r0, r4, lsr #3
    7008:	00000000 	andeq	r0, r0, r0
    700c:	00750044 	rsbseq	r0, r5, r4, asr #32
    7010:	000001c8 	andeq	r0, r0, r8, asr #3
    7014:	00000000 	andeq	r0, r0, r0
    7018:	00760044 	rsbseq	r0, r6, r4, asr #32
    701c:	000001dc 	ldrdeq	r0, [r0], -ip
    7020:	00000000 	andeq	r0, r0, r0
    7024:	00770044 	rsbseq	r0, r7, r4, asr #32
    7028:	000001f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    702c:	00000000 	andeq	r0, r0, r0
    7030:	00790044 	rsbseq	r0, r9, r4, asr #32
    7034:	00000204 	andeq	r0, r0, r4, lsl #4
    7038:	00000000 	andeq	r0, r0, r0
    703c:	007a0044 	rsbseq	r0, sl, r4, asr #32
    7040:	00000218 	andeq	r0, r0, r8, lsl r2
    7044:	00000000 	andeq	r0, r0, r0
    7048:	007c0044 	rsbseq	r0, ip, r4, asr #32
    704c:	0000022c 	andeq	r0, r0, ip, lsr #4
    7050:	00000000 	andeq	r0, r0, r0
    7054:	007d0044 	rsbseq	r0, sp, r4, asr #32
    7058:	00000240 	andeq	r0, r0, r0, asr #4
    705c:	00000000 	andeq	r0, r0, r0
    7060:	007e0044 	rsbseq	r0, lr, r4, asr #32
    7064:	0000025c 	andeq	r0, r0, ip, asr r2
    7068:	00000000 	andeq	r0, r0, r0
    706c:	007f0044 	rsbseq	r0, pc, r4, asr #32
    7070:	00000270 	andeq	r0, r0, r0, ror r2
    7074:	00000000 	andeq	r0, r0, r0
    7078:	00800044 	addeq	r0, r0, r4, asr #32
    707c:	00000284 	andeq	r0, r0, r4, lsl #5
    7080:	00000000 	andeq	r0, r0, r0
    7084:	00810044 	addeq	r0, r1, r4, asr #32
    7088:	0000029c 	muleq	r0, ip, r2
    708c:	00000000 	andeq	r0, r0, r0
    7090:	00820044 	addeq	r0, r2, r4, asr #32
    7094:	000002b4 			; <UNDEFINED> instruction: 0x000002b4
    7098:	00000000 	andeq	r0, r0, r0
    709c:	00840044 	addeq	r0, r4, r4, asr #32
    70a0:	000002cc 	andeq	r0, r0, ip, asr #5
    70a4:	00000000 	andeq	r0, r0, r0
    70a8:	00850044 	addeq	r0, r5, r4, asr #32
    70ac:	000002e0 	andeq	r0, r0, r0, ror #5
    70b0:	00000000 	andeq	r0, r0, r0
    70b4:	00870044 	addeq	r0, r7, r4, asr #32
    70b8:	000002f4 	strdeq	r0, [r0], -r4
    70bc:	00000000 	andeq	r0, r0, r0
    70c0:	00880044 	addeq	r0, r8, r4, asr #32
    70c4:	00000314 	andeq	r0, r0, r4, lsl r3
    70c8:	00000000 	andeq	r0, r0, r0
    70cc:	00890044 	addeq	r0, r9, r4, asr #32
    70d0:	00000320 	andeq	r0, r0, r0, lsr #6
    70d4:	0000305f 	andeq	r3, r0, pc, asr r0
    70d8:	004e0026 	subeq	r0, lr, r6, lsr #32
    70dc:	80228004 	eorhi	r8, r2, r4
    70e0:	0000306c 	andeq	r3, r0, ip, rrx
    70e4:	004f0026 	subeq	r0, pc, r6, lsr #32
    70e8:	80020000 	andhi	r0, r2, r0
    70ec:	00003079 	andeq	r3, r0, r9, ror r0
    70f0:	00500080 	subseq	r0, r0, r0, lsl #1
    70f4:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    70f8:	00003083 	andeq	r3, r0, r3, lsl #1
    70fc:	00510080 	subseq	r0, r1, r0, lsl #1
    7100:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    7104:	00003095 	muleq	r0, r5, r0
    7108:	00520080 	subseq	r0, r2, r0, lsl #1
    710c:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    7110:	00000000 	andeq	r0, r0, r0
    7114:	000000c0 	andeq	r0, r0, r0, asr #1
    7118:	00000000 	andeq	r0, r0, r0
    711c:	00000ecb 	andeq	r0, r0, fp, asr #29
    7120:	00540080 	subseq	r0, r4, r0, lsl #1
    7124:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    7128:	00000000 	andeq	r0, r0, r0
    712c:	000000c0 	andeq	r0, r0, r0, asr #1
    7130:	00000030 	andeq	r0, r0, r0, lsr r0
    7134:	00000000 	andeq	r0, r0, r0
    7138:	000000e0 	andeq	r0, r0, r0, ror #1
    713c:	0000008c 	andeq	r0, r0, ip, lsl #1
    7140:	00000000 	andeq	r0, r0, r0
    7144:	000000e0 	andeq	r0, r0, r0, ror #1
    7148:	000003b8 			; <UNDEFINED> instruction: 0x000003b8
    714c:	00000000 	andeq	r0, r0, r0
    7150:	00000024 	andeq	r0, r0, r4, lsr #32
    7154:	000003b8 			; <UNDEFINED> instruction: 0x000003b8
    7158:	00000000 	andeq	r0, r0, r0
    715c:	0000004e 	andeq	r0, r0, lr, asr #32
    7160:	800136b4 			; <UNDEFINED> instruction: 0x800136b4
    7164:	000030a5 	andeq	r3, r0, r5, lsr #1
    7168:	008b0024 	addeq	r0, fp, r4, lsr #32
    716c:	800136b4 			; <UNDEFINED> instruction: 0x800136b4
    7170:	00000000 	andeq	r0, r0, r0
    7174:	0000002e 	andeq	r0, r0, lr, lsr #32
    7178:	800136b4 			; <UNDEFINED> instruction: 0x800136b4
    717c:	00000000 	andeq	r0, r0, r0
    7180:	008c0044 	addeq	r0, ip, r4, asr #32
	...
    718c:	008d0044 	addeq	r0, sp, r4, asr #32
    7190:	00000014 	andeq	r0, r0, r4, lsl r0
    7194:	00000000 	andeq	r0, r0, r0
    7198:	008e0044 	addeq	r0, lr, r4, asr #32
    719c:	00000024 	andeq	r0, r0, r4, lsr #32
    71a0:	00000000 	andeq	r0, r0, r0
    71a4:	00000024 	andeq	r0, r0, r4, lsr #32
    71a8:	00000034 	andeq	r0, r0, r4, lsr r0
    71ac:	00000000 	andeq	r0, r0, r0
    71b0:	0000004e 	andeq	r0, r0, lr, asr #32
    71b4:	800136e8 	andhi	r3, r1, r8, ror #13
    71b8:	000030c6 	andeq	r3, r0, r6, asr #1
    71bc:	00910024 	addseq	r0, r1, r4, lsr #32
    71c0:	800136e8 	andhi	r3, r1, r8, ror #13
    71c4:	000030d7 	ldrdeq	r3, [r0], -r7
    71c8:	009100a0 	addseq	r0, r1, r0, lsr #1
    71cc:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    71d0:	00000000 	andeq	r0, r0, r0
    71d4:	0000002e 	andeq	r0, r0, lr, lsr #32
    71d8:	800136e8 	andhi	r3, r1, r8, ror #13
    71dc:	00000000 	andeq	r0, r0, r0
    71e0:	00920044 	addseq	r0, r2, r4, asr #32
	...
    71ec:	00940044 	addseq	r0, r4, r4, asr #32
    71f0:	00000014 	andeq	r0, r0, r4, lsl r0
    71f4:	00000000 	andeq	r0, r0, r0
    71f8:	00950044 	addseq	r0, r5, r4, asr #32
    71fc:	00000020 	andeq	r0, r0, r0, lsr #32
    7200:	00000000 	andeq	r0, r0, r0
    7204:	00960044 	addseq	r0, r6, r4, asr #32
    7208:	00000040 	andeq	r0, r0, r0, asr #32
    720c:	00000000 	andeq	r0, r0, r0
    7210:	00970044 	addseq	r0, r7, r4, asr #32
    7214:	0000004c 	andeq	r0, r0, ip, asr #32
    7218:	00000000 	andeq	r0, r0, r0
    721c:	00940044 	addseq	r0, r4, r4, asr #32
    7220:	00000074 	andeq	r0, r0, r4, ror r0
    7224:	00000000 	andeq	r0, r0, r0
    7228:	00940044 	addseq	r0, r4, r4, asr #32
    722c:	00000080 	andeq	r0, r0, r0, lsl #1
    7230:	00000000 	andeq	r0, r0, r0
    7234:	009b0044 	addseq	r0, fp, r4, asr #32
    7238:	0000008c 	andeq	r0, r0, ip, lsl #1
    723c:	00000000 	andeq	r0, r0, r0
    7240:	009c0044 	addseq	r0, ip, r4, asr #32
    7244:	0000009c 	muleq	r0, ip, r0
    7248:	00000000 	andeq	r0, r0, r0
    724c:	009d0044 	addseq	r0, sp, r4, asr #32
    7250:	000000ac 	andeq	r0, r0, ip, lsr #1
    7254:	00000000 	andeq	r0, r0, r0
    7258:	009e0044 	addseq	r0, lr, r4, asr #32
    725c:	000000bc 	strheq	r0, [r0], -ip
    7260:	00000000 	andeq	r0, r0, r0
    7264:	009f0044 	addseq	r0, pc, r4, asr #32
    7268:	000000d4 	ldrdeq	r0, [r0], -r4
    726c:	00000000 	andeq	r0, r0, r0
    7270:	00a00044 	adceq	r0, r0, r4, asr #32
    7274:	000000e4 	andeq	r0, r0, r4, ror #1
    7278:	00000000 	andeq	r0, r0, r0
    727c:	00a10044 	adceq	r0, r1, r4, asr #32
    7280:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    7284:	00000ecb 	andeq	r0, r0, fp, asr #29
    7288:	00940080 	addseq	r0, r4, r0, lsl #1
    728c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    7290:	00000000 	andeq	r0, r0, r0
    7294:	000000c0 	andeq	r0, r0, r0, asr #1
    7298:	00000014 	andeq	r0, r0, r4, lsl r0
    729c:	000024ee 	andeq	r2, r0, lr, ror #9
    72a0:	00950080 	addseq	r0, r5, r0, lsl #1
    72a4:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    72a8:	00000000 	andeq	r0, r0, r0
    72ac:	000000c0 	andeq	r0, r0, r0, asr #1
    72b0:	00000020 	andeq	r0, r0, r0, lsr #32
    72b4:	00000000 	andeq	r0, r0, r0
    72b8:	000000e0 	andeq	r0, r0, r0, ror #1
    72bc:	00000074 	andeq	r0, r0, r4, ror r0
    72c0:	00000000 	andeq	r0, r0, r0
    72c4:	000000e0 	andeq	r0, r0, r0, ror #1
    72c8:	0000008c 	andeq	r0, r0, ip, lsl #1
    72cc:	00000000 	andeq	r0, r0, r0
    72d0:	00000024 	andeq	r0, r0, r4, lsr #32
    72d4:	000000f8 	strdeq	r0, [r0], -r8
    72d8:	00000000 	andeq	r0, r0, r0
    72dc:	0000004e 	andeq	r0, r0, lr, asr #32
    72e0:	800137e0 	andhi	r3, r1, r0, ror #15
    72e4:	000030e4 	andeq	r3, r0, r4, ror #1
    72e8:	00a40024 	adceq	r0, r4, r4, lsr #32
    72ec:	800137e0 	andhi	r3, r1, r0, ror #15
    72f0:	000030d7 	ldrdeq	r3, [r0], -r7
    72f4:	00a400a0 	adceq	r0, r4, r0, lsr #1
    72f8:	ffffffc8 			; <UNDEFINED> instruction: 0xffffffc8
    72fc:	000030f4 	strdeq	r3, [r0], -r4
    7300:	00a400a0 	adceq	r0, r4, r0, lsr #1
    7304:	ffffffc4 			; <UNDEFINED> instruction: 0xffffffc4
    7308:	00000000 	andeq	r0, r0, r0
    730c:	0000002e 	andeq	r0, r0, lr, lsr #32
    7310:	800137e0 	andhi	r3, r1, r0, ror #15
    7314:	00000000 	andeq	r0, r0, r0
    7318:	00a50044 	adceq	r0, r5, r4, asr #32
	...
    7324:	00a60044 	adceq	r0, r6, r4, asr #32
    7328:	00000018 	andeq	r0, r0, r8, lsl r0
    732c:	00000000 	andeq	r0, r0, r0
    7330:	00a70044 	adceq	r0, r7, r4, asr #32
    7334:	00000020 	andeq	r0, r0, r0, lsr #32
    7338:	00000000 	andeq	r0, r0, r0
    733c:	00a80044 	adceq	r0, r8, r4, asr #32
    7340:	00000028 	andeq	r0, r0, r8, lsr #32
    7344:	00000000 	andeq	r0, r0, r0
    7348:	00aa0044 	adceq	r0, sl, r4, asr #32
    734c:	00000030 	andeq	r0, r0, r0, lsr r0
    7350:	00000000 	andeq	r0, r0, r0
    7354:	00ab0044 	adceq	r0, fp, r4, asr #32
    7358:	00000038 	andeq	r0, r0, r8, lsr r0
    735c:	00000000 	andeq	r0, r0, r0
    7360:	00ac0044 	adceq	r0, ip, r4, asr #32
    7364:	00000048 	andeq	r0, r0, r8, asr #32
    7368:	00000000 	andeq	r0, r0, r0
    736c:	00ae0044 	adceq	r0, lr, r4, asr #32
    7370:	00000050 	andeq	r0, r0, r0, asr r0
    7374:	00000000 	andeq	r0, r0, r0
    7378:	00af0044 	adceq	r0, pc, r4, asr #32
    737c:	0000005c 	andeq	r0, r0, ip, asr r0
    7380:	00000000 	andeq	r0, r0, r0
    7384:	00b10044 	adcseq	r0, r1, r4, asr #32
    7388:	00000068 	andeq	r0, r0, r8, rrx
    738c:	00000000 	andeq	r0, r0, r0
    7390:	00b20044 	adcseq	r0, r2, r4, asr #32
    7394:	00000074 	andeq	r0, r0, r4, ror r0
    7398:	00000000 	andeq	r0, r0, r0
    739c:	00b30044 	adcseq	r0, r3, r4, asr #32
    73a0:	0000007c 	andeq	r0, r0, ip, ror r0
    73a4:	00000000 	andeq	r0, r0, r0
    73a8:	00b60044 	adcseq	r0, r6, r4, asr #32
    73ac:	0000008c 	andeq	r0, r0, ip, lsl #1
    73b0:	00000000 	andeq	r0, r0, r0
    73b4:	00b70044 	adcseq	r0, r7, r4, asr #32
    73b8:	00000090 	muleq	r0, r0, r0
    73bc:	00000000 	andeq	r0, r0, r0
    73c0:	00b80044 	adcseq	r0, r8, r4, asr #32
    73c4:	000000a8 	andeq	r0, r0, r8, lsr #1
    73c8:	00000000 	andeq	r0, r0, r0
    73cc:	00b90044 	adcseq	r0, r9, r4, asr #32
    73d0:	000000b8 	strheq	r0, [r0], -r8
    73d4:	00000000 	andeq	r0, r0, r0
    73d8:	00b60044 	adcseq	r0, r6, r4, asr #32
    73dc:	000000c0 	andeq	r0, r0, r0, asr #1
    73e0:	00000000 	andeq	r0, r0, r0
    73e4:	00be0044 	adcseq	r0, lr, r4, asr #32
    73e8:	000000e4 	andeq	r0, r0, r4, ror #1
    73ec:	00000000 	andeq	r0, r0, r0
    73f0:	00bf0044 	adcseq	r0, pc, r4, asr #32
    73f4:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    73f8:	00000000 	andeq	r0, r0, r0
    73fc:	00c00044 	sbceq	r0, r0, r4, asr #32
    7400:	00000104 	andeq	r0, r0, r4, lsl #2
    7404:	00000000 	andeq	r0, r0, r0
    7408:	00c10044 	sbceq	r0, r1, r4, asr #32
    740c:	00000124 	andeq	r0, r0, r4, lsr #2
    7410:	00000000 	andeq	r0, r0, r0
    7414:	00c30044 	sbceq	r0, r3, r4, asr #32
    7418:	00000130 	andeq	r0, r0, r0, lsr r1
    741c:	00000000 	andeq	r0, r0, r0
    7420:	00c40044 	sbceq	r0, r4, r4, asr #32
    7424:	00000144 	andeq	r0, r0, r4, asr #2
    7428:	00000000 	andeq	r0, r0, r0
    742c:	00be0044 	adcseq	r0, lr, r4, asr #32
    7430:	0000015c 	andeq	r0, r0, ip, asr r1
    7434:	00000000 	andeq	r0, r0, r0
    7438:	00be0044 	adcseq	r0, lr, r4, asr #32
    743c:	00000168 	andeq	r0, r0, r8, ror #2
    7440:	00000000 	andeq	r0, r0, r0
    7444:	00c70044 	sbceq	r0, r7, r4, asr #32
    7448:	0000017c 	andeq	r0, r0, ip, ror r1
    744c:	00000000 	andeq	r0, r0, r0
    7450:	00b10044 	adcseq	r0, r1, r4, asr #32
    7454:	00000188 	andeq	r0, r0, r8, lsl #3
    7458:	00000000 	andeq	r0, r0, r0
    745c:	00b10044 	adcseq	r0, r1, r4, asr #32
    7460:	00000194 	muleq	r0, r4, r1
    7464:	00000000 	andeq	r0, r0, r0
    7468:	00ca0044 	sbceq	r0, sl, r4, asr #32
    746c:	000001a4 	andeq	r0, r0, r4, lsr #3
    7470:	00000000 	andeq	r0, r0, r0
    7474:	00cb0044 	sbceq	r0, fp, r4, asr #32
    7478:	000001b8 			; <UNDEFINED> instruction: 0x000001b8
    747c:	00000000 	andeq	r0, r0, r0
    7480:	00cd0044 	sbceq	r0, sp, r4, asr #32
    7484:	000001c4 	andeq	r0, r0, r4, asr #3
    7488:	00000000 	andeq	r0, r0, r0
    748c:	00ce0044 	sbceq	r0, lr, r4, asr #32
    7490:	000001dc 	ldrdeq	r0, [r0], -ip
    7494:	00000000 	andeq	r0, r0, r0
    7498:	00cf0044 	sbceq	r0, pc, r4, asr #32
    749c:	000001e8 	andeq	r0, r0, r8, ror #3
    74a0:	00000000 	andeq	r0, r0, r0
    74a4:	00d00044 	sbcseq	r0, r0, r4, asr #32
    74a8:	000001fc 	strdeq	r0, [r0], -ip
    74ac:	00000000 	andeq	r0, r0, r0
    74b0:	00d20044 	sbcseq	r0, r2, r4, asr #32
    74b4:	0000020c 	andeq	r0, r0, ip, lsl #4
    74b8:	00000000 	andeq	r0, r0, r0
    74bc:	00d30044 	sbcseq	r0, r3, r4, asr #32
    74c0:	00000210 	andeq	r0, r0, r0, lsl r2
    74c4:	00003106 	andeq	r3, r0, r6, lsl #2
    74c8:	00a60080 	adceq	r0, r6, r0, lsl #1
    74cc:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    74d0:	0000311d 	andeq	r3, r0, sp, lsl r1
    74d4:	00a70080 	adceq	r0, r7, r0, lsl #1
    74d8:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    74dc:	00000ecb 	andeq	r0, r0, fp, asr #29
    74e0:	00a80080 	adceq	r0, r8, r0, lsl #1
    74e4:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    74e8:	00003133 	andeq	r3, r0, r3, lsr r1
    74ec:	00aa0080 	adceq	r0, sl, r0, lsl #1
    74f0:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    74f4:	00000000 	andeq	r0, r0, r0
    74f8:	000000c0 	andeq	r0, r0, r0, asr #1
    74fc:	00000000 	andeq	r0, r0, r0
    7500:	00003149 	andeq	r3, r0, r9, asr #2
    7504:	00b20080 	adcseq	r0, r2, r0, lsl #1
    7508:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    750c:	00003151 	andeq	r3, r0, r1, asr r1
    7510:	00b30080 	adcseq	r0, r3, r0, lsl #1
    7514:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    7518:	00000000 	andeq	r0, r0, r0
    751c:	000000c0 	andeq	r0, r0, r0, asr #1
    7520:	00000074 	andeq	r0, r0, r4, ror r0
    7524:	00003167 	andeq	r3, r0, r7, ror #2
    7528:	00bf0080 	adcseq	r0, pc, r0, lsl #1
    752c:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    7530:	00003173 	andeq	r3, r0, r3, ror r1
    7534:	00c00080 	sbceq	r0, r0, r0, lsl #1
    7538:	ffffffd4 			; <UNDEFINED> instruction: 0xffffffd4
    753c:	0000317f 	andeq	r3, r0, pc, ror r1
    7540:	00c10080 	sbceq	r0, r1, r0, lsl #1
    7544:	ffffffd0 			; <UNDEFINED> instruction: 0xffffffd0
    7548:	00003189 	andeq	r3, r0, r9, lsl #3
    754c:	00c30080 	sbceq	r0, r3, r0, lsl #1
    7550:	ffffffcc 			; <UNDEFINED> instruction: 0xffffffcc
    7554:	00000000 	andeq	r0, r0, r0
    7558:	000000c0 	andeq	r0, r0, r0, asr #1
    755c:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    7560:	00000000 	andeq	r0, r0, r0
    7564:	000000e0 	andeq	r0, r0, r0, ror #1
    7568:	0000015c 	andeq	r0, r0, ip, asr r1
    756c:	00000000 	andeq	r0, r0, r0
    7570:	000000e0 	andeq	r0, r0, r0, ror #1
    7574:	00000188 	andeq	r0, r0, r8, lsl #3
    7578:	00000000 	andeq	r0, r0, r0
    757c:	000000e0 	andeq	r0, r0, r0, ror #1
    7580:	00000220 	andeq	r0, r0, r0, lsr #4
    7584:	00000000 	andeq	r0, r0, r0
    7588:	00000024 	andeq	r0, r0, r4, lsr #32
    758c:	00000220 	andeq	r0, r0, r0, lsr #4
    7590:	00000000 	andeq	r0, r0, r0
    7594:	0000004e 	andeq	r0, r0, lr, asr #32
    7598:	80013a00 	andhi	r3, r1, r0, lsl #20
    759c:	00003198 	muleq	r0, r8, r1
    75a0:	00d60024 	sbcseq	r0, r6, r4, lsr #32
    75a4:	80013a00 	andhi	r3, r1, r0, lsl #20
    75a8:	000030d7 	ldrdeq	r3, [r0], -r7
    75ac:	00d600a0 	sbcseq	r0, r6, r0, lsr #1
    75b0:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    75b4:	00000000 	andeq	r0, r0, r0
    75b8:	0000002e 	andeq	r0, r0, lr, lsr #32
    75bc:	80013a00 	andhi	r3, r1, r0, lsl #20
    75c0:	00000000 	andeq	r0, r0, r0
    75c4:	00d70044 	sbcseq	r0, r7, r4, asr #32
	...
    75d0:	00d80044 	sbcseq	r0, r8, r4, asr #32
    75d4:	0000001c 	andeq	r0, r0, ip, lsl r0
    75d8:	00000000 	andeq	r0, r0, r0
    75dc:	00da0044 	sbcseq	r0, sl, r4, asr #32
    75e0:	0000002c 	andeq	r0, r0, ip, lsr #32
    75e4:	00000000 	andeq	r0, r0, r0
    75e8:	00dd0044 	sbcseq	r0, sp, r4, asr #32
    75ec:	00000040 	andeq	r0, r0, r0, asr #32
    75f0:	00000000 	andeq	r0, r0, r0
    75f4:	00de0044 	sbcseq	r0, lr, r4, asr #32
    75f8:	00000044 	andeq	r0, r0, r4, asr #32
    75fc:	00000000 	andeq	r0, r0, r0
    7600:	00e00044 	rsceq	r0, r0, r4, asr #32
    7604:	00000048 	andeq	r0, r0, r8, asr #32
    7608:	00000000 	andeq	r0, r0, r0
    760c:	00e10044 	rsceq	r0, r1, r4, asr #32
    7610:	00000058 	andeq	r0, r0, r8, asr r0
    7614:	00000000 	andeq	r0, r0, r0
    7618:	00000024 	andeq	r0, r0, r4, lsr #32
    761c:	00000068 	andeq	r0, r0, r8, rrx
    7620:	00000000 	andeq	r0, r0, r0
    7624:	0000004e 	andeq	r0, r0, lr, asr #32
    7628:	80013a68 	andhi	r3, r1, r8, ror #20
    762c:	000031aa 	andeq	r3, r0, sl, lsr #3
    7630:	00e30024 	rsceq	r0, r3, r4, lsr #32
    7634:	80013a68 	andhi	r3, r1, r8, ror #20
    7638:	000031ba 			; <UNDEFINED> instruction: 0x000031ba
    763c:	00e300a0 	rsceq	r0, r3, r0, lsr #1
    7640:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    7644:	00000000 	andeq	r0, r0, r0
    7648:	0000002e 	andeq	r0, r0, lr, lsr #32
    764c:	80013a68 	andhi	r3, r1, r8, ror #20
    7650:	00000000 	andeq	r0, r0, r0
    7654:	00e40044 	rsceq	r0, r4, r4, asr #32
	...
    7660:	00e50044 	rsceq	r0, r5, r4, asr #32
    7664:	0000001c 	andeq	r0, r0, ip, lsl r0
    7668:	00000000 	andeq	r0, r0, r0
    766c:	00e50044 	rsceq	r0, r5, r4, asr #32
    7670:	00000028 	andeq	r0, r0, r8, lsr #32
    7674:	00000000 	andeq	r0, r0, r0
    7678:	00e60044 	rsceq	r0, r6, r4, asr #32
    767c:	00000034 	andeq	r0, r0, r4, lsr r0
    7680:	00000000 	andeq	r0, r0, r0
    7684:	00e80044 	rsceq	r0, r8, r4, asr #32
    7688:	0000003c 	andeq	r0, r0, ip, lsr r0
    768c:	00000000 	andeq	r0, r0, r0
    7690:	00e90044 	rsceq	r0, r9, r4, asr #32
    7694:	00000054 	andeq	r0, r0, r4, asr r0
    7698:	00000000 	andeq	r0, r0, r0
    769c:	00000024 	andeq	r0, r0, r4, lsr #32
    76a0:	00000068 	andeq	r0, r0, r8, rrx
    76a4:	00000000 	andeq	r0, r0, r0
    76a8:	0000004e 	andeq	r0, r0, lr, asr #32
    76ac:	80013ad0 	ldrdhi	r3, [r1], -r0
    76b0:	000031c5 	andeq	r3, r0, r5, asr #3
    76b4:	00eb0024 	rsceq	r0, fp, r4, lsr #32
    76b8:	80013ad0 	ldrdhi	r3, [r1], -r0
    76bc:	00000000 	andeq	r0, r0, r0
    76c0:	0000002e 	andeq	r0, r0, lr, lsr #32
    76c4:	80013ad0 	ldrdhi	r3, [r1], -r0
    76c8:	00000000 	andeq	r0, r0, r0
    76cc:	00ec0044 	rsceq	r0, ip, r4, asr #32
	...
    76d8:	00ed0044 	rsceq	r0, sp, r4, asr #32
    76dc:	00000018 	andeq	r0, r0, r8, lsl r0
    76e0:	00000000 	andeq	r0, r0, r0
    76e4:	00ee0044 	rsceq	r0, lr, r4, asr #32
    76e8:	00000020 	andeq	r0, r0, r0, lsr #32
    76ec:	00000000 	andeq	r0, r0, r0
    76f0:	00ef0044 	rsceq	r0, pc, r4, asr #32
    76f4:	00000030 	andeq	r0, r0, r0, lsr r0
    76f8:	00000000 	andeq	r0, r0, r0
    76fc:	00f10044 	rscseq	r0, r1, r4, asr #32
    7700:	00000038 	andeq	r0, r0, r8, lsr r0
    7704:	00000000 	andeq	r0, r0, r0
    7708:	00f20044 	rscseq	r0, r2, r4, asr #32
    770c:	00000040 	andeq	r0, r0, r0, asr #32
    7710:	00000000 	andeq	r0, r0, r0
    7714:	00f30044 	rscseq	r0, r3, r4, asr #32
    7718:	00000064 	andeq	r0, r0, r4, rrx
    771c:	00000000 	andeq	r0, r0, r0
    7720:	00f40044 	rscseq	r0, r4, r4, asr #32
    7724:	00000074 	andeq	r0, r0, r4, ror r0
    7728:	00000000 	andeq	r0, r0, r0
    772c:	00f80044 	rscseq	r0, r8, r4, asr #32
    7730:	0000007c 	andeq	r0, r0, ip, ror r0
    7734:	00000000 	andeq	r0, r0, r0
    7738:	00f90044 	rscseq	r0, r9, r4, asr #32
    773c:	00000088 	andeq	r0, r0, r8, lsl #1
    7740:	00000000 	andeq	r0, r0, r0
    7744:	00fa0044 	rscseq	r0, sl, r4, asr #32
    7748:	000000a4 	andeq	r0, r0, r4, lsr #1
    774c:	00000000 	andeq	r0, r0, r0
    7750:	00fb0044 	rscseq	r0, fp, r4, asr #32
    7754:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    7758:	00000000 	andeq	r0, r0, r0
    775c:	00fd0044 	rscseq	r0, sp, r4, asr #32
    7760:	000000b8 	strheq	r0, [r0], -r8
    7764:	00000000 	andeq	r0, r0, r0
    7768:	00f80044 	rscseq	r0, r8, r4, asr #32
    776c:	000000c8 	andeq	r0, r0, r8, asr #1
    7770:	00000000 	andeq	r0, r0, r0
    7774:	00f80044 	rscseq	r0, r8, r4, asr #32
    7778:	000000d4 	ldrdeq	r0, [r0], -r4
    777c:	00000000 	andeq	r0, r0, r0
    7780:	01010044 	tsteq	r1, r4, asr #32
    7784:	000000e8 	andeq	r0, r0, r8, ror #1
    7788:	00000000 	andeq	r0, r0, r0
    778c:	01020044 	tsteq	r2, r4, asr #32
    7790:	00000108 	andeq	r0, r0, r8, lsl #2
    7794:	00000000 	andeq	r0, r0, r0
    7798:	01050044 	tsteq	r5, r4, asr #32
    779c:	00000128 	andeq	r0, r0, r8, lsr #2
    77a0:	00000000 	andeq	r0, r0, r0
    77a4:	01080044 	tsteq	r8, r4, asr #32
    77a8:	00000148 	andeq	r0, r0, r8, asr #2
    77ac:	00000000 	andeq	r0, r0, r0
    77b0:	01090044 	tsteq	r9, r4, asr #32
    77b4:	00000168 	andeq	r0, r0, r8, ror #2
    77b8:	00000000 	andeq	r0, r0, r0
    77bc:	010c0044 	tsteq	ip, r4, asr #32
    77c0:	00000174 	andeq	r0, r0, r4, ror r1
    77c4:	00000000 	andeq	r0, r0, r0
    77c8:	010f0044 	tsteq	pc, r4, asr #32
    77cc:	00000180 	andeq	r0, r0, r0, lsl #3
    77d0:	00000000 	andeq	r0, r0, r0
    77d4:	01120044 	tsteq	r2, r4, asr #32
    77d8:	0000018c 	andeq	r0, r0, ip, lsl #3
    77dc:	00000000 	andeq	r0, r0, r0
    77e0:	01150044 	tsteq	r5, r4, asr #32
    77e4:	0000019c 	muleq	r0, ip, r1
    77e8:	00000000 	andeq	r0, r0, r0
    77ec:	01180044 	tsteq	r8, r4, asr #32
    77f0:	000001ac 	andeq	r0, r0, ip, lsr #3
    77f4:	00000000 	andeq	r0, r0, r0
    77f8:	011b0044 	tsteq	fp, r4, asr #32
    77fc:	000001c8 	andeq	r0, r0, r8, asr #3
    7800:	00000000 	andeq	r0, r0, r0
    7804:	011e0044 	tsteq	lr, r4, asr #32
    7808:	000001e4 	andeq	r0, r0, r4, ror #3
    780c:	00000000 	andeq	r0, r0, r0
    7810:	011f0044 	tsteq	pc, r4, asr #32
    7814:	000001f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    7818:	00000000 	andeq	r0, r0, r0
    781c:	01200044 			; <UNDEFINED> instruction: 0x01200044
    7820:	00000210 	andeq	r0, r0, r0, lsl r2
    7824:	00000000 	andeq	r0, r0, r0
    7828:	01210044 			; <UNDEFINED> instruction: 0x01210044
    782c:	0000021c 	andeq	r0, r0, ip, lsl r2
    7830:	00000000 	andeq	r0, r0, r0
    7834:	01220044 			; <UNDEFINED> instruction: 0x01220044
    7838:	0000023c 	andeq	r0, r0, ip, lsr r2
    783c:	00000000 	andeq	r0, r0, r0
    7840:	01230044 			; <UNDEFINED> instruction: 0x01230044
    7844:	00000274 	andeq	r0, r0, r4, ror r2
    7848:	00000000 	andeq	r0, r0, r0
    784c:	01240044 			; <UNDEFINED> instruction: 0x01240044
    7850:	000002ac 	andeq	r0, r0, ip, lsr #5
    7854:	00000000 	andeq	r0, r0, r0
    7858:	011e0044 	tsteq	lr, r4, asr #32
    785c:	000002d4 	ldrdeq	r0, [r0], -r4
    7860:	00000000 	andeq	r0, r0, r0
    7864:	011e0044 	tsteq	lr, r4, asr #32
    7868:	000002e0 	andeq	r0, r0, r0, ror #5
    786c:	00000000 	andeq	r0, r0, r0
    7870:	01280044 			; <UNDEFINED> instruction: 0x01280044
    7874:	000002ec 	andeq	r0, r0, ip, ror #5
    7878:	00000000 	andeq	r0, r0, r0
    787c:	01290044 			; <UNDEFINED> instruction: 0x01290044
    7880:	000002f4 	strdeq	r0, [r0], -r4
    7884:	000031d2 	ldrdeq	r3, [r0], -r2
    7888:	00ed0080 	rsceq	r0, sp, r0, lsl #1
    788c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    7890:	000031df 	ldrdeq	r3, [r0], -pc	; <UNPREDICTABLE>
    7894:	00ee0080 	rsceq	r0, lr, r0, lsl #1
    7898:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    789c:	00000ad8 	ldrdeq	r0, [r0], -r8
    78a0:	00ef0080 	rsceq	r0, pc, r0, lsl #1
    78a4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    78a8:	00000000 	andeq	r0, r0, r0
    78ac:	000000c0 	andeq	r0, r0, r0, asr #1
    78b0:	00000000 	andeq	r0, r0, r0
    78b4:	000031ed 	andeq	r3, r0, sp, ror #3
    78b8:	00f90080 	rscseq	r0, r9, r0, lsl #1
    78bc:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    78c0:	000031fe 	strdeq	r3, [r0], -lr
    78c4:	00fa0080 	rscseq	r0, sl, r0, lsl #1
    78c8:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    78cc:	0000320d 	andeq	r3, r0, sp, lsl #4
    78d0:	00fb0080 	rscseq	r0, fp, r0, lsl #1
    78d4:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    78d8:	00000000 	andeq	r0, r0, r0
    78dc:	000000c0 	andeq	r0, r0, r0, asr #1
    78e0:	00000088 	andeq	r0, r0, r8, lsl #1
    78e4:	00000000 	andeq	r0, r0, r0
    78e8:	000000e0 	andeq	r0, r0, r0, ror #1
    78ec:	000000c8 	andeq	r0, r0, r8, asr #1
    78f0:	000024ee 	andeq	r2, r0, lr, ror #9
    78f4:	011f0080 	tsteq	pc, r0, lsl #1
    78f8:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    78fc:	00000000 	andeq	r0, r0, r0
    7900:	000000c0 	andeq	r0, r0, r0, asr #1
    7904:	000001f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    7908:	00000000 	andeq	r0, r0, r0
    790c:	000000e0 	andeq	r0, r0, r0, ror #1
    7910:	000002d4 	ldrdeq	r0, [r0], -r4
    7914:	00000000 	andeq	r0, r0, r0
    7918:	000000e0 	andeq	r0, r0, r0, ror #1
    791c:	0000030c 	andeq	r0, r0, ip, lsl #6
    7920:	00000000 	andeq	r0, r0, r0
    7924:	00000024 	andeq	r0, r0, r4, lsr #32
    7928:	0000030c 	andeq	r0, r0, ip, lsl #6
    792c:	00000000 	andeq	r0, r0, r0
    7930:	0000004e 	andeq	r0, r0, lr, asr #32
    7934:	80013ddc 	ldrdhi	r3, [r1], -ip
    7938:	0000321d 	andeq	r3, r0, sp, lsl r2
    793c:	012b0024 			; <UNDEFINED> instruction: 0x012b0024
    7940:	80013ddc 	ldrdhi	r3, [r1], -ip
    7944:	00000000 	andeq	r0, r0, r0
    7948:	0000002e 	andeq	r0, r0, lr, lsr #32
    794c:	80013ddc 	ldrdhi	r3, [r1], -ip
    7950:	00000000 	andeq	r0, r0, r0
    7954:	012b0044 			; <UNDEFINED> instruction: 0x012b0044
	...
    7960:	012e0044 			; <UNDEFINED> instruction: 0x012e0044
    7964:	00000018 	andeq	r0, r0, r8, lsl r0
    7968:	00000000 	andeq	r0, r0, r0
    796c:	012f0044 			; <UNDEFINED> instruction: 0x012f0044
    7970:	0000002c 	andeq	r0, r0, ip, lsr #32
    7974:	00000000 	andeq	r0, r0, r0
    7978:	01310044 	teqeq	r1, r4, asr #32
    797c:	00000030 	andeq	r0, r0, r0, lsr r0
    7980:	00000000 	andeq	r0, r0, r0
    7984:	01320044 	teqeq	r2, r4, asr #32
    7988:	00000048 	andeq	r0, r0, r8, asr #32
    798c:	00000000 	andeq	r0, r0, r0
    7990:	01330044 	teqeq	r3, r4, asr #32
    7994:	0000004c 	andeq	r0, r0, ip, asr #32
    7998:	00000000 	andeq	r0, r0, r0
    799c:	01350044 	teqeq	r5, r4, asr #32
    79a0:	00000050 	andeq	r0, r0, r0, asr r0
    79a4:	00000000 	andeq	r0, r0, r0
    79a8:	01360044 	teqeq	r6, r4, asr #32
    79ac:	0000005c 	andeq	r0, r0, ip, asr r0
    79b0:	00000000 	andeq	r0, r0, r0
    79b4:	01380044 	teqeq	r8, r4, asr #32
    79b8:	00000078 	andeq	r0, r0, r8, ror r0
    79bc:	00000000 	andeq	r0, r0, r0
    79c0:	01390044 	teqeq	r9, r4, asr #32
    79c4:	00000088 	andeq	r0, r0, r8, lsl #1
    79c8:	00000000 	andeq	r0, r0, r0
    79cc:	01380044 	teqeq	r8, r4, asr #32
    79d0:	000000a0 	andeq	r0, r0, r0, lsr #1
    79d4:	00000000 	andeq	r0, r0, r0
    79d8:	013a0044 	teqeq	sl, r4, asr #32
    79dc:	000000a8 	andeq	r0, r0, r8, lsr #1
    79e0:	00000000 	andeq	r0, r0, r0
    79e4:	013b0044 	teqeq	fp, r4, asr #32
    79e8:	000000b4 	strheq	r0, [r0], -r4
    79ec:	00000000 	andeq	r0, r0, r0
    79f0:	01350044 	teqeq	r5, r4, asr #32
    79f4:	000000c0 	andeq	r0, r0, r0, asr #1
    79f8:	00000000 	andeq	r0, r0, r0
    79fc:	01350044 	teqeq	r5, r4, asr #32
    7a00:	000000cc 	andeq	r0, r0, ip, asr #1
    7a04:	00000000 	andeq	r0, r0, r0
    7a08:	013f0044 	teqeq	pc, r4, asr #32
    7a0c:	000000d8 	ldrdeq	r0, [r0], -r8
    7a10:	00000000 	andeq	r0, r0, r0
    7a14:	01400044 	cmpeq	r0, r4, asr #32
    7a18:	000000ec 	andeq	r0, r0, ip, ror #1
    7a1c:	00000000 	andeq	r0, r0, r0
    7a20:	01420044 	cmpeq	r2, r4, asr #32
    7a24:	000000fc 	strdeq	r0, [r0], -ip
    7a28:	00000000 	andeq	r0, r0, r0
    7a2c:	01430044 	cmpeq	r3, r4, asr #32
    7a30:	00000100 	andeq	r0, r0, r0, lsl #2
    7a34:	00000000 	andeq	r0, r0, r0
    7a38:	01440044 	cmpeq	r4, r4, asr #32
    7a3c:	00000104 	andeq	r0, r0, r4, lsl #2
    7a40:	00000ecb 	andeq	r0, r0, fp, asr #29
    7a44:	012c0080 	smlawbeq	ip, r0, r0, r0
    7a48:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    7a4c:	00000000 	andeq	r0, r0, r0
    7a50:	000000c0 	andeq	r0, r0, r0, asr #1
    7a54:	00000000 	andeq	r0, r0, r0
    7a58:	000024e2 	andeq	r2, r0, r2, ror #9
    7a5c:	01360080 	teqeq	r6, r0, lsl #1
    7a60:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    7a64:	00000000 	andeq	r0, r0, r0
    7a68:	000000c0 	andeq	r0, r0, r0, asr #1
    7a6c:	0000005c 	andeq	r0, r0, ip, asr r0
    7a70:	00000000 	andeq	r0, r0, r0
    7a74:	000000e0 	andeq	r0, r0, r0, ror #1
    7a78:	000000c0 	andeq	r0, r0, r0, asr #1
    7a7c:	00000000 	andeq	r0, r0, r0
    7a80:	000000e0 	andeq	r0, r0, r0, ror #1
    7a84:	0000011c 	andeq	r0, r0, ip, lsl r1
    7a88:	00000000 	andeq	r0, r0, r0
    7a8c:	00000024 	andeq	r0, r0, r4, lsr #32
    7a90:	0000011c 	andeq	r0, r0, ip, lsl r1
    7a94:	00000000 	andeq	r0, r0, r0
    7a98:	0000004e 	andeq	r0, r0, lr, asr #32
    7a9c:	80013ef8 	strdhi	r3, [r1], -r8
    7aa0:	0000322e 	andeq	r3, r0, lr, lsr #4
    7aa4:	01460024 	cmpeq	r6, r4, lsr #32
    7aa8:	80013ef8 	strdhi	r3, [r1], -r8
    7aac:	00000000 	andeq	r0, r0, r0
    7ab0:	0000002e 	andeq	r0, r0, lr, lsr #32
    7ab4:	80013ef8 	strdhi	r3, [r1], -r8
    7ab8:	00000000 	andeq	r0, r0, r0
    7abc:	01460044 	cmpeq	r6, r4, asr #32
	...
    7ac8:	01470044 	cmpeq	r7, r4, asr #32
    7acc:	0000000c 	andeq	r0, r0, ip
    7ad0:	00000000 	andeq	r0, r0, r0
    7ad4:	01480044 	cmpeq	r8, r4, asr #32
    7ad8:	00000010 	andeq	r0, r0, r0, lsl r0
    7adc:	00000000 	andeq	r0, r0, r0
    7ae0:	00000024 	andeq	r0, r0, r4, lsr #32
    7ae4:	00000014 	andeq	r0, r0, r4, lsl r0
    7ae8:	00000000 	andeq	r0, r0, r0
    7aec:	0000004e 	andeq	r0, r0, lr, asr #32
    7af0:	80013f0c 	andhi	r3, r1, ip, lsl #30
    7af4:	00003242 	andeq	r3, r0, r2, asr #4
    7af8:	00060026 	andeq	r0, r6, r6, lsr #32
    7afc:	8001523c 	andhi	r5, r1, ip, lsr r2
    7b00:	0000325e 	andeq	r3, r0, lr, asr r2
    7b04:	00100028 	andseq	r0, r0, r8, lsr #32
    7b08:	80028000 	andhi	r8, r2, r0
    7b0c:	0000329b 	muleq	r0, fp, r2
    7b10:	00120020 	andseq	r0, r2, r0, lsr #32
    7b14:	00000000 	andeq	r0, r0, r0
    7b18:	000032b3 			; <UNDEFINED> instruction: 0x000032b3
    7b1c:	000d0020 	andeq	r0, sp, r0, lsr #32
	...
    7b28:	00000064 	andeq	r0, r0, r4, rrx
    7b2c:	80013f0c 	andhi	r3, r1, ip, lsl #30
    7b30:	00000007 	andeq	r0, r0, r7
    7b34:	00020064 	andeq	r0, r2, r4, rrx
    7b38:	80013f10 	andhi	r3, r1, r0, lsl pc
    7b3c:	000032dd 	ldrdeq	r3, [r0], -sp
    7b40:	00020064 	andeq	r0, r2, r4, rrx
    7b44:	80013f10 	andhi	r3, r1, r0, lsl pc
    7b48:	0000003d 	andeq	r0, r0, sp, lsr r0
    7b4c:	0000003c 	andeq	r0, r0, ip, lsr r0
    7b50:	00000000 	andeq	r0, r0, r0
    7b54:	0000004c 	andeq	r0, r0, ip, asr #32
    7b58:	00000080 	andeq	r0, r0, r0, lsl #1
    7b5c:	00000000 	andeq	r0, r0, r0
    7b60:	00000076 	andeq	r0, r0, r6, ror r0
    7b64:	00000080 	andeq	r0, r0, r0, lsl #1
    7b68:	00000000 	andeq	r0, r0, r0
    7b6c:	00000094 	muleq	r0, r4, r0
    7b70:	00000080 	andeq	r0, r0, r0, lsl #1
    7b74:	00000000 	andeq	r0, r0, r0
    7b78:	000000c3 	andeq	r0, r0, r3, asr #1
    7b7c:	00000080 	andeq	r0, r0, r0, lsl #1
    7b80:	00000000 	andeq	r0, r0, r0
    7b84:	000000ee 	andeq	r0, r0, lr, ror #1
    7b88:	00000080 	andeq	r0, r0, r0, lsl #1
    7b8c:	00000000 	andeq	r0, r0, r0
    7b90:	0000011e 	andeq	r0, r0, lr, lsl r1
    7b94:	00000080 	andeq	r0, r0, r0, lsl #1
    7b98:	00000000 	andeq	r0, r0, r0
    7b9c:	0000016f 	andeq	r0, r0, pc, ror #2
    7ba0:	00000080 	andeq	r0, r0, r0, lsl #1
    7ba4:	00000000 	andeq	r0, r0, r0
    7ba8:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    7bac:	00000080 	andeq	r0, r0, r0, lsl #1
    7bb0:	00000000 	andeq	r0, r0, r0
    7bb4:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    7bb8:	00000080 	andeq	r0, r0, r0, lsl #1
    7bbc:	00000000 	andeq	r0, r0, r0
    7bc0:	0000020e 	andeq	r0, r0, lr, lsl #4
    7bc4:	00000080 	andeq	r0, r0, r0, lsl #1
    7bc8:	00000000 	andeq	r0, r0, r0
    7bcc:	00000238 	andeq	r0, r0, r8, lsr r2
    7bd0:	00000080 	andeq	r0, r0, r0, lsl #1
    7bd4:	00000000 	andeq	r0, r0, r0
    7bd8:	00000261 	andeq	r0, r0, r1, ror #4
    7bdc:	00000080 	andeq	r0, r0, r0, lsl #1
    7be0:	00000000 	andeq	r0, r0, r0
    7be4:	0000027b 	andeq	r0, r0, fp, ror r2
    7be8:	00000080 	andeq	r0, r0, r0, lsl #1
    7bec:	00000000 	andeq	r0, r0, r0
    7bf0:	00000296 	muleq	r0, r6, r2
    7bf4:	00000080 	andeq	r0, r0, r0, lsl #1
    7bf8:	00000000 	andeq	r0, r0, r0
    7bfc:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    7c00:	00000080 	andeq	r0, r0, r0, lsl #1
    7c04:	00000000 	andeq	r0, r0, r0
    7c08:	000002d7 	ldrdeq	r0, [r0], -r7
    7c0c:	00000080 	andeq	r0, r0, r0, lsl #1
    7c10:	00000000 	andeq	r0, r0, r0
    7c14:	000002f7 	strdeq	r0, [r0], -r7
    7c18:	00000080 	andeq	r0, r0, r0, lsl #1
    7c1c:	00000000 	andeq	r0, r0, r0
    7c20:	0000031c 	andeq	r0, r0, ip, lsl r3
    7c24:	00000080 	andeq	r0, r0, r0, lsl #1
    7c28:	00000000 	andeq	r0, r0, r0
    7c2c:	00000346 	andeq	r0, r0, r6, asr #6
    7c30:	00000080 	andeq	r0, r0, r0, lsl #1
    7c34:	00000000 	andeq	r0, r0, r0
    7c38:	0000036a 	andeq	r0, r0, sl, ror #6
    7c3c:	00000080 	andeq	r0, r0, r0, lsl #1
    7c40:	00000000 	andeq	r0, r0, r0
    7c44:	00000393 	muleq	r0, r3, r3
    7c48:	00000080 	andeq	r0, r0, r0, lsl #1
    7c4c:	00000000 	andeq	r0, r0, r0
    7c50:	000003c1 	andeq	r0, r0, r1, asr #7
    7c54:	00000080 	andeq	r0, r0, r0, lsl #1
    7c58:	00000000 	andeq	r0, r0, r0
    7c5c:	000003e7 	andeq	r0, r0, r7, ror #7
    7c60:	00000080 	andeq	r0, r0, r0, lsl #1
    7c64:	00000000 	andeq	r0, r0, r0
    7c68:	00000407 	andeq	r0, r0, r7, lsl #8
    7c6c:	00000080 	andeq	r0, r0, r0, lsl #1
    7c70:	00000000 	andeq	r0, r0, r0
    7c74:	0000042c 	andeq	r0, r0, ip, lsr #8
    7c78:	00000080 	andeq	r0, r0, r0, lsl #1
    7c7c:	00000000 	andeq	r0, r0, r0
    7c80:	00000456 	andeq	r0, r0, r6, asr r4
    7c84:	00000080 	andeq	r0, r0, r0, lsl #1
    7c88:	00000000 	andeq	r0, r0, r0
    7c8c:	00000485 	andeq	r0, r0, r5, lsl #9
    7c90:	00000080 	andeq	r0, r0, r0, lsl #1
    7c94:	00000000 	andeq	r0, r0, r0
    7c98:	000004ae 	andeq	r0, r0, lr, lsr #9
    7c9c:	00000080 	andeq	r0, r0, r0, lsl #1
    7ca0:	00000000 	andeq	r0, r0, r0
    7ca4:	000004dc 	ldrdeq	r0, [r0], -ip
    7ca8:	00000080 	andeq	r0, r0, r0, lsl #1
    7cac:	00000000 	andeq	r0, r0, r0
    7cb0:	0000050f 	andeq	r0, r0, pc, lsl #10
    7cb4:	00000080 	andeq	r0, r0, r0, lsl #1
    7cb8:	00000000 	andeq	r0, r0, r0
    7cbc:	00000530 	andeq	r0, r0, r0, lsr r5
    7cc0:	00000080 	andeq	r0, r0, r0, lsl #1
    7cc4:	00000000 	andeq	r0, r0, r0
    7cc8:	00000550 	andeq	r0, r0, r0, asr r5
    7ccc:	00000080 	andeq	r0, r0, r0, lsl #1
    7cd0:	00000000 	andeq	r0, r0, r0
    7cd4:	00000575 	andeq	r0, r0, r5, ror r5
    7cd8:	00000080 	andeq	r0, r0, r0, lsl #1
    7cdc:	00000000 	andeq	r0, r0, r0
    7ce0:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    7ce4:	00000080 	andeq	r0, r0, r0, lsl #1
    7ce8:	00000000 	andeq	r0, r0, r0
    7cec:	000005c3 	andeq	r0, r0, r3, asr #11
    7cf0:	00000080 	andeq	r0, r0, r0, lsl #1
    7cf4:	00000000 	andeq	r0, r0, r0
    7cf8:	000005ec 	andeq	r0, r0, ip, ror #11
    7cfc:	00000080 	andeq	r0, r0, r0, lsl #1
    7d00:	00000000 	andeq	r0, r0, r0
    7d04:	0000061a 	andeq	r0, r0, sl, lsl r6
    7d08:	00000080 	andeq	r0, r0, r0, lsl #1
    7d0c:	00000000 	andeq	r0, r0, r0
    7d10:	00000640 	andeq	r0, r0, r0, asr #12
    7d14:	00000080 	andeq	r0, r0, r0, lsl #1
    7d18:	00000000 	andeq	r0, r0, r0
    7d1c:	00000660 	andeq	r0, r0, r0, ror #12
    7d20:	00000080 	andeq	r0, r0, r0, lsl #1
    7d24:	00000000 	andeq	r0, r0, r0
    7d28:	00000685 	andeq	r0, r0, r5, lsl #13
    7d2c:	00000080 	andeq	r0, r0, r0, lsl #1
    7d30:	00000000 	andeq	r0, r0, r0
    7d34:	000006af 	andeq	r0, r0, pc, lsr #13
    7d38:	00000080 	andeq	r0, r0, r0, lsl #1
    7d3c:	00000000 	andeq	r0, r0, r0
    7d40:	000006de 	ldrdeq	r0, [r0], -lr
    7d44:	00000080 	andeq	r0, r0, r0, lsl #1
    7d48:	00000000 	andeq	r0, r0, r0
    7d4c:	00000707 	andeq	r0, r0, r7, lsl #14
    7d50:	00000080 	andeq	r0, r0, r0, lsl #1
    7d54:	00000000 	andeq	r0, r0, r0
    7d58:	00000735 	andeq	r0, r0, r5, lsr r7
    7d5c:	00000080 	andeq	r0, r0, r0, lsl #1
    7d60:	00000000 	andeq	r0, r0, r0
    7d64:	00000768 	andeq	r0, r0, r8, ror #14
    7d68:	00000080 	andeq	r0, r0, r0, lsl #1
    7d6c:	00000000 	andeq	r0, r0, r0
    7d70:	000012f3 	strdeq	r1, [r0], -r3
    7d74:	000000c2 	andeq	r0, r0, r2, asr #1
    7d78:	00002a19 	andeq	r2, r0, r9, lsl sl
    7d7c:	0000078a 	andeq	r0, r0, sl, lsl #15
    7d80:	000000c2 	andeq	r0, r0, r2, asr #1
    7d84:	00002d60 	andeq	r2, r0, r0, ror #26
    7d88:	00001306 	andeq	r1, r0, r6, lsl #6
    7d8c:	000000c2 	andeq	r0, r0, r2, asr #1
    7d90:	0000151b 	andeq	r1, r0, fp, lsl r5
    7d94:	00001173 	andeq	r1, r0, r3, ror r1
    7d98:	000000c2 	andeq	r0, r0, r2, asr #1
    7d9c:	0000c9ad 	andeq	ip, r0, sp, lsr #19
    7da0:	00000886 	andeq	r0, r0, r6, lsl #17
    7da4:	000000c2 	andeq	r0, r0, r2, asr #1
    7da8:	00003ac8 	andeq	r3, r0, r8, asr #21
    7dac:	00001182 	andeq	r1, r0, r2, lsl #3
    7db0:	000000c2 	andeq	r0, r0, r2, asr #1
    7db4:	00004df6 	strdeq	r4, [r0], -r6
    7db8:	00001425 	andeq	r1, r0, r5, lsr #8
    7dbc:	000000c2 	andeq	r0, r0, r2, asr #1
    7dc0:	00001fc5 	andeq	r1, r0, r5, asr #31
    7dc4:	000014c3 	andeq	r1, r0, r3, asr #9
    7dc8:	000000c2 	andeq	r0, r0, r2, asr #1
    7dcc:	00002743 	andeq	r2, r0, r3, asr #14
    7dd0:	000032ec 	andeq	r3, r0, ip, ror #5
    7dd4:	000f0024 	andeq	r0, pc, r4, lsr #32
    7dd8:	80013f10 	andhi	r3, r1, r0, lsl pc
    7ddc:	00003302 	andeq	r3, r0, r2, lsl #6
    7de0:	000f00a0 	andeq	r0, pc, r0, lsr #1
    7de4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    7de8:	00000000 	andeq	r0, r0, r0
    7dec:	0000002e 	andeq	r0, r0, lr, lsr #32
    7df0:	80013f10 	andhi	r3, r1, r0, lsl pc
    7df4:	00001306 	andeq	r1, r0, r6, lsl #6
    7df8:	00000084 	andeq	r0, r0, r4, lsl #1
    7dfc:	80013f10 	andhi	r3, r1, r0, lsl pc
    7e00:	00000000 	andeq	r0, r0, r0
    7e04:	000f0044 	andeq	r0, pc, r4, asr #32
	...
    7e10:	00100044 	andseq	r0, r0, r4, asr #32
    7e14:	00000014 	andeq	r0, r0, r4, lsl r0
    7e18:	00000000 	andeq	r0, r0, r0
    7e1c:	00110044 	andseq	r0, r1, r4, asr #32
    7e20:	00000020 	andeq	r0, r0, r0, lsr #32
    7e24:	00000000 	andeq	r0, r0, r0
    7e28:	00120044 	andseq	r0, r2, r4, asr #32
    7e2c:	00000028 	andeq	r0, r0, r8, lsr #32
    7e30:	00000000 	andeq	r0, r0, r0
    7e34:	00130044 	andseq	r0, r3, r4, asr #32
    7e38:	00000034 	andeq	r0, r0, r4, lsr r0
    7e3c:	00000000 	andeq	r0, r0, r0
    7e40:	00000024 	andeq	r0, r0, r4, lsr #32
    7e44:	00000040 	andeq	r0, r0, r0, asr #32
    7e48:	00000000 	andeq	r0, r0, r0
    7e4c:	0000004e 	andeq	r0, r0, lr, asr #32
    7e50:	80013f50 	andhi	r3, r1, r0, asr pc
    7e54:	0000330d 	andeq	r3, r0, sp, lsl #6
    7e58:	00080024 	andeq	r0, r8, r4, lsr #32
    7e5c:	80013f50 	andhi	r3, r1, r0, asr pc
    7e60:	0000331a 	andeq	r3, r0, sl, lsl r3
    7e64:	000800a0 	andeq	r0, r8, r0, lsr #1
    7e68:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    7e6c:	000031ba 			; <UNDEFINED> instruction: 0x000031ba
    7e70:	000800a0 	andeq	r0, r8, r0, lsr #1
    7e74:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    7e78:	00003302 	andeq	r3, r0, r2, lsl #6
    7e7c:	000800a0 	andeq	r0, r8, r0, lsr #1
    7e80:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    7e84:	00000000 	andeq	r0, r0, r0
    7e88:	0000002e 	andeq	r0, r0, lr, lsr #32
    7e8c:	80013f50 	andhi	r3, r1, r0, asr pc
    7e90:	000032dd 	ldrdeq	r3, [r0], -sp
    7e94:	00000084 	andeq	r0, r0, r4, lsl #1
    7e98:	80013f50 	andhi	r3, r1, r0, asr pc
    7e9c:	00000000 	andeq	r0, r0, r0
    7ea0:	00080044 	andeq	r0, r8, r4, asr #32
	...
    7eac:	00090044 	andeq	r0, r9, r4, asr #32
    7eb0:	00000024 	andeq	r0, r0, r4, lsr #32
    7eb4:	00000000 	andeq	r0, r0, r0
    7eb8:	000a0044 	andeq	r0, sl, r4, asr #32
    7ebc:	00000030 	andeq	r0, r0, r0, lsr r0
    7ec0:	00000000 	andeq	r0, r0, r0
    7ec4:	000d0044 	andeq	r0, sp, r4, asr #32
    7ec8:	00000038 	andeq	r0, r0, r8, lsr r0
    7ecc:	00000000 	andeq	r0, r0, r0
    7ed0:	000e0044 	andeq	r0, lr, r4, asr #32
    7ed4:	00000044 	andeq	r0, r0, r4, asr #32
    7ed8:	00000000 	andeq	r0, r0, r0
    7edc:	000f0044 	andeq	r0, pc, r4, asr #32
    7ee0:	00000050 	andeq	r0, r0, r0, asr r0
    7ee4:	00000000 	andeq	r0, r0, r0
    7ee8:	00120044 	andseq	r0, r2, r4, asr #32
    7eec:	00000058 	andeq	r0, r0, r8, asr r0
    7ef0:	00000000 	andeq	r0, r0, r0
    7ef4:	00130044 	andseq	r0, r3, r4, asr #32
    7ef8:	00000064 	andeq	r0, r0, r4, rrx
    7efc:	00000000 	andeq	r0, r0, r0
    7f00:	00140044 	andseq	r0, r4, r4, asr #32
    7f04:	00000070 	andeq	r0, r0, r0, ror r0
    7f08:	00000000 	andeq	r0, r0, r0
    7f0c:	00170044 	andseq	r0, r7, r4, asr #32
    7f10:	00000078 	andeq	r0, r0, r8, ror r0
    7f14:	00000000 	andeq	r0, r0, r0
    7f18:	00180044 	andseq	r0, r8, r4, asr #32
    7f1c:	00000084 	andeq	r0, r0, r4, lsl #1
    7f20:	00000000 	andeq	r0, r0, r0
    7f24:	00190044 	andseq	r0, r9, r4, asr #32
    7f28:	00000098 	muleq	r0, r8, r0
    7f2c:	00000000 	andeq	r0, r0, r0
    7f30:	001a0044 	andseq	r0, sl, r4, asr #32
    7f34:	000000a8 	andeq	r0, r0, r8, lsr #1
    7f38:	00000000 	andeq	r0, r0, r0
    7f3c:	001b0044 	andseq	r0, fp, r4, asr #32
    7f40:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    7f44:	00000000 	andeq	r0, r0, r0
    7f48:	001e0044 	andseq	r0, lr, r4, asr #32
    7f4c:	000000b8 	strheq	r0, [r0], -r8
    7f50:	00000000 	andeq	r0, r0, r0
    7f54:	001f0044 	andseq	r0, pc, r4, asr #32
    7f58:	000000c4 	andeq	r0, r0, r4, asr #1
    7f5c:	00000000 	andeq	r0, r0, r0
    7f60:	00210044 	eoreq	r0, r1, r4, asr #32
    7f64:	000000ec 	andeq	r0, r0, ip, ror #1
    7f68:	00000000 	andeq	r0, r0, r0
    7f6c:	00220044 	eoreq	r0, r2, r4, asr #32
    7f70:	000000f8 	strdeq	r0, [r0], -r8
    7f74:	00000000 	andeq	r0, r0, r0
    7f78:	00230044 	eoreq	r0, r3, r4, asr #32
    7f7c:	00000110 	andeq	r0, r0, r0, lsl r1
    7f80:	00000000 	andeq	r0, r0, r0
    7f84:	00250044 	eoreq	r0, r5, r4, asr #32
    7f88:	00000128 	andeq	r0, r0, r8, lsr #2
    7f8c:	00000000 	andeq	r0, r0, r0
    7f90:	00260044 	eoreq	r0, r6, r4, asr #32
    7f94:	00000134 	andeq	r0, r0, r4, lsr r1
    7f98:	00000000 	andeq	r0, r0, r0
    7f9c:	00280044 	eoreq	r0, r8, r4, asr #32
    7fa0:	00000140 	andeq	r0, r0, r0, asr #2
    7fa4:	00000000 	andeq	r0, r0, r0
    7fa8:	00290044 	eoreq	r0, r9, r4, asr #32
    7fac:	00000150 	andeq	r0, r0, r0, asr r1
    7fb0:	00000000 	andeq	r0, r0, r0
    7fb4:	002c0044 	eoreq	r0, ip, r4, asr #32
    7fb8:	00000170 	andeq	r0, r0, r0, ror r1
    7fbc:	00000000 	andeq	r0, r0, r0
    7fc0:	002d0044 	eoreq	r0, sp, r4, asr #32
    7fc4:	00000180 	andeq	r0, r0, r0, lsl #3
    7fc8:	00000000 	andeq	r0, r0, r0
    7fcc:	002e0044 	eoreq	r0, lr, r4, asr #32
    7fd0:	00000190 	muleq	r0, r0, r1
    7fd4:	00000000 	andeq	r0, r0, r0
    7fd8:	00310044 	eorseq	r0, r1, r4, asr #32
    7fdc:	0000019c 	muleq	r0, ip, r1
    7fe0:	00000000 	andeq	r0, r0, r0
    7fe4:	00320044 	eorseq	r0, r2, r4, asr #32
    7fe8:	000001a4 	andeq	r0, r0, r4, lsr #3
    7fec:	00003324 	andeq	r3, r0, r4, lsr #6
    7ff0:	000d0080 	andeq	r0, sp, r0, lsl #1
    7ff4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    7ff8:	00003339 	andeq	r3, r0, r9, lsr r3
    7ffc:	00120080 	andseq	r0, r2, r0, lsl #1
    8000:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    8004:	00003343 	andeq	r3, r0, r3, asr #6
    8008:	00170080 	andseq	r0, r7, r0, lsl #1
    800c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    8010:	00000000 	andeq	r0, r0, r0
    8014:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    8020:	000000e0 	andeq	r0, r0, r0, ror #1
    8024:	000001c0 	andeq	r0, r0, r0, asr #3
    8028:	00000000 	andeq	r0, r0, r0
    802c:	00000024 	andeq	r0, r0, r4, lsr #32
    8030:	000001c0 	andeq	r0, r0, r0, asr #3
    8034:	00000000 	andeq	r0, r0, r0
    8038:	0000004e 	andeq	r0, r0, lr, asr #32
    803c:	80014110 	andhi	r4, r1, r0, lsl r1
    8040:	00003351 	andeq	r3, r0, r1, asr r3
    8044:	00340024 	eorseq	r0, r4, r4, lsr #32
    8048:	80014110 	andhi	r4, r1, r0, lsl r1
    804c:	0000331a 	andeq	r3, r0, sl, lsl r3
    8050:	003400a0 	eorseq	r0, r4, r0, lsr #1
    8054:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    8058:	00000000 	andeq	r0, r0, r0
    805c:	0000002e 	andeq	r0, r0, lr, lsr #32
    8060:	80014110 	andhi	r4, r1, r0, lsl r1
    8064:	00000000 	andeq	r0, r0, r0
    8068:	00340044 	eorseq	r0, r4, r4, asr #32
	...
    8074:	00350044 	eorseq	r0, r5, r4, asr #32
    8078:	0000001c 	andeq	r0, r0, ip, lsl r0
    807c:	00000000 	andeq	r0, r0, r0
    8080:	00360044 	eorseq	r0, r6, r4, asr #32
    8084:	00000030 	andeq	r0, r0, r0, lsr r0
    8088:	00000000 	andeq	r0, r0, r0
    808c:	00380044 	eorseq	r0, r8, r4, asr #32
    8090:	0000003c 	andeq	r0, r0, ip, lsr r0
    8094:	00000000 	andeq	r0, r0, r0
    8098:	00390044 	eorseq	r0, r9, r4, asr #32
    809c:	00000040 	andeq	r0, r0, r0, asr #32
    80a0:	00000000 	andeq	r0, r0, r0
    80a4:	00390044 	eorseq	r0, r9, r4, asr #32
    80a8:	0000004c 	andeq	r0, r0, ip, asr #32
    80ac:	00000000 	andeq	r0, r0, r0
    80b0:	003b0044 	eorseq	r0, fp, r4, asr #32
    80b4:	00000064 	andeq	r0, r0, r4, rrx
    80b8:	00000000 	andeq	r0, r0, r0
    80bc:	00380044 	eorseq	r0, r8, r4, asr #32
    80c0:	00000070 	andeq	r0, r0, r0, ror r0
    80c4:	00000000 	andeq	r0, r0, r0
    80c8:	003e0044 	eorseq	r0, lr, r4, asr #32
    80cc:	0000007c 	andeq	r0, r0, ip, ror r0
    80d0:	00000000 	andeq	r0, r0, r0
    80d4:	003f0044 	eorseq	r0, pc, r4, asr #32
    80d8:	00000088 	andeq	r0, r0, r8, lsl #1
    80dc:	00000000 	andeq	r0, r0, r0
    80e0:	00410044 	subeq	r0, r1, r4, asr #32
    80e4:	00000090 	muleq	r0, r0, r0
    80e8:	00000000 	andeq	r0, r0, r0
    80ec:	00430044 	subeq	r0, r3, r4, asr #32
    80f0:	000000a4 	andeq	r0, r0, r4, lsr #1
    80f4:	00000000 	andeq	r0, r0, r0
    80f8:	00440044 	subeq	r0, r4, r4, asr #32
    80fc:	000000c4 	andeq	r0, r0, r4, asr #1
    8100:	00000000 	andeq	r0, r0, r0
    8104:	00450044 	subeq	r0, r5, r4, asr #32
    8108:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    810c:	00000000 	andeq	r0, r0, r0
    8110:	00470044 	subeq	r0, r7, r4, asr #32
    8114:	000000d8 	ldrdeq	r0, [r0], -r8
    8118:	00000000 	andeq	r0, r0, r0
    811c:	004a0044 	subeq	r0, sl, r4, asr #32
    8120:	000000f0 	strdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    8124:	00000000 	andeq	r0, r0, r0
    8128:	004b0044 	subeq	r0, fp, r4, asr #32
    812c:	00000100 	andeq	r0, r0, r0, lsl #2
    8130:	00000000 	andeq	r0, r0, r0
    8134:	004d0044 	subeq	r0, sp, r4, asr #32
    8138:	00000118 	andeq	r0, r0, r8, lsl r1
    813c:	00000000 	andeq	r0, r0, r0
    8140:	004f0044 	subeq	r0, pc, r4, asr #32
    8144:	00000128 	andeq	r0, r0, r8, lsr #2
    8148:	00000000 	andeq	r0, r0, r0
    814c:	00500044 	subseq	r0, r0, r4, asr #32
    8150:	00000138 	andeq	r0, r0, r8, lsr r1
    8154:	00000000 	andeq	r0, r0, r0
    8158:	00520044 	subseq	r0, r2, r4, asr #32
    815c:	00000150 	andeq	r0, r0, r0, asr r1
    8160:	00000000 	andeq	r0, r0, r0
    8164:	00550044 	subseq	r0, r5, r4, asr #32
    8168:	00000160 	andeq	r0, r0, r0, ror #2
    816c:	00000000 	andeq	r0, r0, r0
    8170:	00560044 	subseq	r0, r6, r4, asr #32
    8174:	00000170 	andeq	r0, r0, r0, ror r1
    8178:	00000000 	andeq	r0, r0, r0
    817c:	00570044 	subseq	r0, r7, r4, asr #32
    8180:	00000178 	andeq	r0, r0, r8, ror r1
    8184:	00000000 	andeq	r0, r0, r0
    8188:	00580044 	subseq	r0, r8, r4, asr #32
    818c:	0000017c 	andeq	r0, r0, ip, ror r1
    8190:	0000335e 	andeq	r3, r0, lr, asr r3
    8194:	00350080 	eorseq	r0, r5, r0, lsl #1
    8198:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    819c:	00003339 	andeq	r3, r0, r9, lsr r3
    81a0:	00360080 	eorseq	r0, r6, r0, lsl #1
    81a4:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    81a8:	00003343 	andeq	r3, r0, r3, asr #6
    81ac:	00410080 	subeq	r0, r1, r0, lsl #1
    81b0:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    81b4:	00003372 	andeq	r3, r0, r2, ror r3
    81b8:	00430080 	subeq	r0, r3, r0, lsl #1
    81bc:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    81c0:	00000000 	andeq	r0, r0, r0
    81c4:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    81d0:	000000e0 	andeq	r0, r0, r0, ror #1
    81d4:	00000190 	muleq	r0, r0, r1
    81d8:	00000000 	andeq	r0, r0, r0
    81dc:	00000024 	andeq	r0, r0, r4, lsr #32
    81e0:	00000190 	muleq	r0, r0, r1
    81e4:	00000000 	andeq	r0, r0, r0
    81e8:	0000004e 	andeq	r0, r0, lr, asr #32
    81ec:	800142a0 	andhi	r4, r1, r0, lsr #5
    81f0:	0000337c 	andeq	r3, r0, ip, ror r3
    81f4:	005a0024 	subseq	r0, sl, r4, lsr #32
    81f8:	800142a0 	andhi	r4, r1, r0, lsr #5
    81fc:	00003396 	muleq	r0, r6, r3
    8200:	005a00a0 	subseq	r0, sl, r0, lsr #1
    8204:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    8208:	00000000 	andeq	r0, r0, r0
    820c:	0000002e 	andeq	r0, r0, lr, lsr #32
    8210:	800142a0 	andhi	r4, r1, r0, lsr #5
    8214:	00000000 	andeq	r0, r0, r0
    8218:	005a0044 	subseq	r0, sl, r4, asr #32
	...
    8224:	005b0044 	subseq	r0, fp, r4, asr #32
    8228:	00000014 	andeq	r0, r0, r4, lsl r0
    822c:	00000000 	andeq	r0, r0, r0
    8230:	005c0044 	subseq	r0, ip, r4, asr #32
    8234:	00000020 	andeq	r0, r0, r0, lsr #32
    8238:	00000000 	andeq	r0, r0, r0
    823c:	005d0044 	subseq	r0, sp, r4, asr #32
    8240:	00000024 	andeq	r0, r0, r4, lsr #32
    8244:	00000000 	andeq	r0, r0, r0
    8248:	005e0044 	subseq	r0, lr, r4, asr #32
    824c:	0000002c 	andeq	r0, r0, ip, lsr #32
    8250:	00000000 	andeq	r0, r0, r0
    8254:	005f0044 	subseq	r0, pc, r4, asr #32
    8258:	00000038 	andeq	r0, r0, r8, lsr r0
    825c:	00000000 	andeq	r0, r0, r0
    8260:	00600044 	rsbeq	r0, r0, r4, asr #32
    8264:	00000048 	andeq	r0, r0, r8, asr #32
    8268:	00000000 	andeq	r0, r0, r0
    826c:	005c0044 	subseq	r0, ip, r4, asr #32
    8270:	00000050 	andeq	r0, r0, r0, asr r0
    8274:	00000000 	andeq	r0, r0, r0
    8278:	00620044 	rsbeq	r0, r2, r4, asr #32
    827c:	0000005c 	andeq	r0, r0, ip, asr r0
    8280:	00000000 	andeq	r0, r0, r0
    8284:	00630044 	rsbeq	r0, r3, r4, asr #32
    8288:	00000068 	andeq	r0, r0, r8, rrx
    828c:	00000000 	andeq	r0, r0, r0
    8290:	00640044 	rsbeq	r0, r4, r4, asr #32
    8294:	00000074 	andeq	r0, r0, r4, ror r0
    8298:	00003339 	andeq	r3, r0, r9, lsr r3
    829c:	005b0080 	subseq	r0, fp, r0, lsl #1
    82a0:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    82a4:	00000000 	andeq	r0, r0, r0
    82a8:	000000c0 	andeq	r0, r0, r0, asr #1
    82ac:	00000000 	andeq	r0, r0, r0
    82b0:	000033a4 	andeq	r3, r0, r4, lsr #7
    82b4:	005d0080 	subseq	r0, sp, r0, lsl #1
    82b8:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    82bc:	00000000 	andeq	r0, r0, r0
    82c0:	000000c0 	andeq	r0, r0, r0, asr #1
    82c4:	00000024 	andeq	r0, r0, r4, lsr #32
    82c8:	00000000 	andeq	r0, r0, r0
    82cc:	000000e0 	andeq	r0, r0, r0, ror #1
    82d0:	00000050 	andeq	r0, r0, r0, asr r0
    82d4:	00000000 	andeq	r0, r0, r0
    82d8:	000000e0 	andeq	r0, r0, r0, ror #1
    82dc:	0000007c 	andeq	r0, r0, ip, ror r0
    82e0:	00000000 	andeq	r0, r0, r0
    82e4:	00000024 	andeq	r0, r0, r4, lsr #32
    82e8:	0000007c 	andeq	r0, r0, ip, ror r0
    82ec:	00000000 	andeq	r0, r0, r0
    82f0:	0000004e 	andeq	r0, r0, lr, asr #32
    82f4:	8001431c 	andhi	r4, r1, ip, lsl r3
    82f8:	000033ad 	andeq	r3, r0, sp, lsr #7
    82fc:	00060026 	andeq	r0, r6, r6, lsr #32
    8300:	80015268 	andhi	r5, r1, r8, ror #4
    8304:	000033c9 	andeq	r3, r0, r9, asr #7
    8308:	00060026 	andeq	r0, r6, r6, lsr #32
    830c:	80244808 	eorhi	r4, r4, r8, lsl #16
    8310:	00000000 	andeq	r0, r0, r0
    8314:	00000064 	andeq	r0, r0, r4, rrx
    8318:	8001431c 	andhi	r4, r1, ip, lsl r3
    831c:	00000007 	andeq	r0, r0, r7
    8320:	00020064 	andeq	r0, r2, r4, rrx
    8324:	8001431c 	andhi	r4, r1, ip, lsl r3
    8328:	000033dc 	ldrdeq	r3, [r0], -ip
    832c:	00020064 	andeq	r0, r2, r4, rrx
    8330:	8001431c 	andhi	r4, r1, ip, lsl r3
    8334:	0000003d 	andeq	r0, r0, sp, lsr r0
    8338:	0000003c 	andeq	r0, r0, ip, lsr r0
    833c:	00000000 	andeq	r0, r0, r0
    8340:	0000004c 	andeq	r0, r0, ip, asr #32
    8344:	00000080 	andeq	r0, r0, r0, lsl #1
    8348:	00000000 	andeq	r0, r0, r0
    834c:	00000076 	andeq	r0, r0, r6, ror r0
    8350:	00000080 	andeq	r0, r0, r0, lsl #1
    8354:	00000000 	andeq	r0, r0, r0
    8358:	00000094 	muleq	r0, r4, r0
    835c:	00000080 	andeq	r0, r0, r0, lsl #1
    8360:	00000000 	andeq	r0, r0, r0
    8364:	000000c3 	andeq	r0, r0, r3, asr #1
    8368:	00000080 	andeq	r0, r0, r0, lsl #1
    836c:	00000000 	andeq	r0, r0, r0
    8370:	000000ee 	andeq	r0, r0, lr, ror #1
    8374:	00000080 	andeq	r0, r0, r0, lsl #1
    8378:	00000000 	andeq	r0, r0, r0
    837c:	0000011e 	andeq	r0, r0, lr, lsl r1
    8380:	00000080 	andeq	r0, r0, r0, lsl #1
    8384:	00000000 	andeq	r0, r0, r0
    8388:	0000016f 	andeq	r0, r0, pc, ror #2
    838c:	00000080 	andeq	r0, r0, r0, lsl #1
    8390:	00000000 	andeq	r0, r0, r0
    8394:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    8398:	00000080 	andeq	r0, r0, r0, lsl #1
    839c:	00000000 	andeq	r0, r0, r0
    83a0:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    83a4:	00000080 	andeq	r0, r0, r0, lsl #1
    83a8:	00000000 	andeq	r0, r0, r0
    83ac:	0000020e 	andeq	r0, r0, lr, lsl #4
    83b0:	00000080 	andeq	r0, r0, r0, lsl #1
    83b4:	00000000 	andeq	r0, r0, r0
    83b8:	00000238 	andeq	r0, r0, r8, lsr r2
    83bc:	00000080 	andeq	r0, r0, r0, lsl #1
    83c0:	00000000 	andeq	r0, r0, r0
    83c4:	00000261 	andeq	r0, r0, r1, ror #4
    83c8:	00000080 	andeq	r0, r0, r0, lsl #1
    83cc:	00000000 	andeq	r0, r0, r0
    83d0:	0000027b 	andeq	r0, r0, fp, ror r2
    83d4:	00000080 	andeq	r0, r0, r0, lsl #1
    83d8:	00000000 	andeq	r0, r0, r0
    83dc:	00000296 	muleq	r0, r6, r2
    83e0:	00000080 	andeq	r0, r0, r0, lsl #1
    83e4:	00000000 	andeq	r0, r0, r0
    83e8:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    83ec:	00000080 	andeq	r0, r0, r0, lsl #1
    83f0:	00000000 	andeq	r0, r0, r0
    83f4:	000002d7 	ldrdeq	r0, [r0], -r7
    83f8:	00000080 	andeq	r0, r0, r0, lsl #1
    83fc:	00000000 	andeq	r0, r0, r0
    8400:	000002f7 	strdeq	r0, [r0], -r7
    8404:	00000080 	andeq	r0, r0, r0, lsl #1
    8408:	00000000 	andeq	r0, r0, r0
    840c:	0000031c 	andeq	r0, r0, ip, lsl r3
    8410:	00000080 	andeq	r0, r0, r0, lsl #1
    8414:	00000000 	andeq	r0, r0, r0
    8418:	00000346 	andeq	r0, r0, r6, asr #6
    841c:	00000080 	andeq	r0, r0, r0, lsl #1
    8420:	00000000 	andeq	r0, r0, r0
    8424:	0000036a 	andeq	r0, r0, sl, ror #6
    8428:	00000080 	andeq	r0, r0, r0, lsl #1
    842c:	00000000 	andeq	r0, r0, r0
    8430:	00000393 	muleq	r0, r3, r3
    8434:	00000080 	andeq	r0, r0, r0, lsl #1
    8438:	00000000 	andeq	r0, r0, r0
    843c:	000003c1 	andeq	r0, r0, r1, asr #7
    8440:	00000080 	andeq	r0, r0, r0, lsl #1
    8444:	00000000 	andeq	r0, r0, r0
    8448:	000003e7 	andeq	r0, r0, r7, ror #7
    844c:	00000080 	andeq	r0, r0, r0, lsl #1
    8450:	00000000 	andeq	r0, r0, r0
    8454:	00000407 	andeq	r0, r0, r7, lsl #8
    8458:	00000080 	andeq	r0, r0, r0, lsl #1
    845c:	00000000 	andeq	r0, r0, r0
    8460:	0000042c 	andeq	r0, r0, ip, lsr #8
    8464:	00000080 	andeq	r0, r0, r0, lsl #1
    8468:	00000000 	andeq	r0, r0, r0
    846c:	00000456 	andeq	r0, r0, r6, asr r4
    8470:	00000080 	andeq	r0, r0, r0, lsl #1
    8474:	00000000 	andeq	r0, r0, r0
    8478:	00000485 	andeq	r0, r0, r5, lsl #9
    847c:	00000080 	andeq	r0, r0, r0, lsl #1
    8480:	00000000 	andeq	r0, r0, r0
    8484:	000004ae 	andeq	r0, r0, lr, lsr #9
    8488:	00000080 	andeq	r0, r0, r0, lsl #1
    848c:	00000000 	andeq	r0, r0, r0
    8490:	000004dc 	ldrdeq	r0, [r0], -ip
    8494:	00000080 	andeq	r0, r0, r0, lsl #1
    8498:	00000000 	andeq	r0, r0, r0
    849c:	0000050f 	andeq	r0, r0, pc, lsl #10
    84a0:	00000080 	andeq	r0, r0, r0, lsl #1
    84a4:	00000000 	andeq	r0, r0, r0
    84a8:	00000530 	andeq	r0, r0, r0, lsr r5
    84ac:	00000080 	andeq	r0, r0, r0, lsl #1
    84b0:	00000000 	andeq	r0, r0, r0
    84b4:	00000550 	andeq	r0, r0, r0, asr r5
    84b8:	00000080 	andeq	r0, r0, r0, lsl #1
    84bc:	00000000 	andeq	r0, r0, r0
    84c0:	00000575 	andeq	r0, r0, r5, ror r5
    84c4:	00000080 	andeq	r0, r0, r0, lsl #1
    84c8:	00000000 	andeq	r0, r0, r0
    84cc:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    84d0:	00000080 	andeq	r0, r0, r0, lsl #1
    84d4:	00000000 	andeq	r0, r0, r0
    84d8:	000005c3 	andeq	r0, r0, r3, asr #11
    84dc:	00000080 	andeq	r0, r0, r0, lsl #1
    84e0:	00000000 	andeq	r0, r0, r0
    84e4:	000005ec 	andeq	r0, r0, ip, ror #11
    84e8:	00000080 	andeq	r0, r0, r0, lsl #1
    84ec:	00000000 	andeq	r0, r0, r0
    84f0:	0000061a 	andeq	r0, r0, sl, lsl r6
    84f4:	00000080 	andeq	r0, r0, r0, lsl #1
    84f8:	00000000 	andeq	r0, r0, r0
    84fc:	00000640 	andeq	r0, r0, r0, asr #12
    8500:	00000080 	andeq	r0, r0, r0, lsl #1
    8504:	00000000 	andeq	r0, r0, r0
    8508:	00000660 	andeq	r0, r0, r0, ror #12
    850c:	00000080 	andeq	r0, r0, r0, lsl #1
    8510:	00000000 	andeq	r0, r0, r0
    8514:	00000685 	andeq	r0, r0, r5, lsl #13
    8518:	00000080 	andeq	r0, r0, r0, lsl #1
    851c:	00000000 	andeq	r0, r0, r0
    8520:	000006af 	andeq	r0, r0, pc, lsr #13
    8524:	00000080 	andeq	r0, r0, r0, lsl #1
    8528:	00000000 	andeq	r0, r0, r0
    852c:	000006de 	ldrdeq	r0, [r0], -lr
    8530:	00000080 	andeq	r0, r0, r0, lsl #1
    8534:	00000000 	andeq	r0, r0, r0
    8538:	00000707 	andeq	r0, r0, r7, lsl #14
    853c:	00000080 	andeq	r0, r0, r0, lsl #1
    8540:	00000000 	andeq	r0, r0, r0
    8544:	00000735 	andeq	r0, r0, r5, lsr r7
    8548:	00000080 	andeq	r0, r0, r0, lsl #1
    854c:	00000000 	andeq	r0, r0, r0
    8550:	00000768 	andeq	r0, r0, r8, ror #14
    8554:	00000080 	andeq	r0, r0, r0, lsl #1
    8558:	00000000 	andeq	r0, r0, r0
    855c:	00001425 	andeq	r1, r0, r5, lsr #8
    8560:	000000c2 	andeq	r0, r0, r2, asr #1
    8564:	00001fc5 	andeq	r1, r0, r5, asr #31
    8568:	0000078a 	andeq	r0, r0, sl, lsl #15
    856c:	000000c2 	andeq	r0, r0, r2, asr #1
    8570:	00002d60 	andeq	r2, r0, r0, ror #26
    8574:	000033e8 	andeq	r3, r0, r8, ror #7
    8578:	00070024 	andeq	r0, r7, r4, lsr #32
    857c:	8001431c 	andhi	r4, r1, ip, lsl r3
    8580:	000033fe 	strdeq	r3, [r0], -lr
    8584:	000700a0 	andeq	r0, r7, r0, lsr #1
    8588:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    858c:	00000000 	andeq	r0, r0, r0
    8590:	0000002e 	andeq	r0, r0, lr, lsr #32
    8594:	8001431c 	andhi	r4, r1, ip, lsl r3
    8598:	00000000 	andeq	r0, r0, r0
    859c:	00070044 	andeq	r0, r7, r4, asr #32
	...
    85a8:	00080044 	andeq	r0, r8, r4, asr #32
    85ac:	00000014 	andeq	r0, r0, r4, lsl r0
    85b0:	00000000 	andeq	r0, r0, r0
    85b4:	00090044 	andeq	r0, r9, r4, asr #32
    85b8:	00000020 	andeq	r0, r0, r0, lsr #32
    85bc:	00000000 	andeq	r0, r0, r0
    85c0:	000a0044 	andeq	r0, sl, r4, asr #32
    85c4:	0000002c 	andeq	r0, r0, ip, lsr #32
    85c8:	00000000 	andeq	r0, r0, r0
    85cc:	000c0044 	andeq	r0, ip, r4, asr #32
    85d0:	00000034 	andeq	r0, r0, r4, lsr r0
    85d4:	00000000 	andeq	r0, r0, r0
    85d8:	000d0044 	andeq	r0, sp, r4, asr #32
    85dc:	00000044 	andeq	r0, r0, r4, asr #32
    85e0:	00000000 	andeq	r0, r0, r0
    85e4:	000e0044 	andeq	r0, lr, r4, asr #32
    85e8:	00000058 	andeq	r0, r0, r8, asr r0
    85ec:	00000000 	andeq	r0, r0, r0
    85f0:	00110044 	andseq	r0, r1, r4, asr #32
    85f4:	0000006c 	andeq	r0, r0, ip, rrx
    85f8:	00000000 	andeq	r0, r0, r0
    85fc:	00120044 	andseq	r0, r2, r4, asr #32
    8600:	00000080 	andeq	r0, r0, r0, lsl #1
    8604:	00000000 	andeq	r0, r0, r0
    8608:	00130044 	andseq	r0, r3, r4, asr #32
    860c:	00000094 	muleq	r0, r4, r0
    8610:	00000000 	andeq	r0, r0, r0
    8614:	00160044 	andseq	r0, r6, r4, asr #32
    8618:	000000a4 	andeq	r0, r0, r4, lsr #1
    861c:	00000000 	andeq	r0, r0, r0
    8620:	00170044 	andseq	r0, r7, r4, asr #32
    8624:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    8628:	00000000 	andeq	r0, r0, r0
    862c:	00180044 	andseq	r0, r8, r4, asr #32
    8630:	000000b4 	strheq	r0, [r0], -r4
    8634:	00003410 	andeq	r3, r0, r0, lsl r4
    8638:	00080080 	andeq	r0, r8, r0, lsl #1
    863c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    8640:	00000000 	andeq	r0, r0, r0
    8644:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    8650:	000000e0 	andeq	r0, r0, r0, ror #1
    8654:	000000d4 	ldrdeq	r0, [r0], -r4
    8658:	00000000 	andeq	r0, r0, r0
    865c:	00000024 	andeq	r0, r0, r4, lsr #32
    8660:	000000d4 	ldrdeq	r0, [r0], -r4
    8664:	00000000 	andeq	r0, r0, r0
    8668:	0000004e 	andeq	r0, r0, lr, asr #32
    866c:	800143f0 	strdhi	r4, [r1], -r0
    8670:	0000341a 	andeq	r3, r0, sl, lsl r4
    8674:	001a0024 	andseq	r0, sl, r4, lsr #32
    8678:	800143f0 	strdhi	r4, [r1], -r0
    867c:	0000342a 	andeq	r3, r0, sl, lsr #8
    8680:	001a00a0 	andseq	r0, sl, r0, lsr #1
    8684:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    8688:	00003435 	andeq	r3, r0, r5, lsr r4
    868c:	001a00a0 	andseq	r0, sl, r0, lsr #1
    8690:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    8694:	00000000 	andeq	r0, r0, r0
    8698:	0000002e 	andeq	r0, r0, lr, lsr #32
    869c:	800143f0 	strdhi	r4, [r1], -r0
    86a0:	00000000 	andeq	r0, r0, r0
    86a4:	001a0044 	andseq	r0, sl, r4, asr #32
	...
    86b0:	001b0044 	andseq	r0, fp, r4, asr #32
    86b4:	00000018 	andeq	r0, r0, r8, lsl r0
    86b8:	00000000 	andeq	r0, r0, r0
    86bc:	001c0044 	andseq	r0, ip, r4, asr #32
    86c0:	00000024 	andeq	r0, r0, r4, lsr #32
    86c4:	00000000 	andeq	r0, r0, r0
    86c8:	001e0044 	andseq	r0, lr, r4, asr #32
    86cc:	00000028 	andeq	r0, r0, r8, lsr #32
    86d0:	00000000 	andeq	r0, r0, r0
    86d4:	001f0044 	andseq	r0, pc, r4, asr #32
    86d8:	00000038 	andeq	r0, r0, r8, lsr r0
    86dc:	00000000 	andeq	r0, r0, r0
    86e0:	00200044 	eoreq	r0, r0, r4, asr #32
    86e4:	00000048 	andeq	r0, r0, r8, asr #32
    86e8:	00000000 	andeq	r0, r0, r0
    86ec:	00230044 	eoreq	r0, r3, r4, asr #32
    86f0:	00000064 	andeq	r0, r0, r4, rrx
    86f4:	00000000 	andeq	r0, r0, r0
    86f8:	00240044 	eoreq	r0, r4, r4, asr #32
    86fc:	00000074 	andeq	r0, r0, r4, ror r0
    8700:	00000000 	andeq	r0, r0, r0
    8704:	00270044 	eoreq	r0, r7, r4, asr #32
    8708:	0000008c 	andeq	r0, r0, ip, lsl #1
    870c:	00000000 	andeq	r0, r0, r0
    8710:	00280044 	eoreq	r0, r8, r4, asr #32
    8714:	000000ac 	andeq	r0, r0, ip, lsr #1
    8718:	00000000 	andeq	r0, r0, r0
    871c:	00290044 	eoreq	r0, r9, r4, asr #32
    8720:	000000bc 	strheq	r0, [r0], -ip
    8724:	00000000 	andeq	r0, r0, r0
    8728:	002a0044 	eoreq	r0, sl, r4, asr #32
    872c:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    8730:	00000000 	andeq	r0, r0, r0
    8734:	002b0044 	eoreq	r0, fp, r4, asr #32
    8738:	000000e0 	andeq	r0, r0, r0, ror #1
    873c:	00000000 	andeq	r0, r0, r0
    8740:	002d0044 	eoreq	r0, sp, r4, asr #32
    8744:	000000f4 	strdeq	r0, [r0], -r4
    8748:	00000000 	andeq	r0, r0, r0
    874c:	002e0044 	eoreq	r0, lr, r4, asr #32
    8750:	0000010c 	andeq	r0, r0, ip, lsl #2
    8754:	00000000 	andeq	r0, r0, r0
    8758:	002f0044 	eoreq	r0, pc, r4, asr #32
    875c:	00000120 	andeq	r0, r0, r0, lsr #2
    8760:	00000000 	andeq	r0, r0, r0
    8764:	00310044 	eorseq	r0, r1, r4, asr #32
    8768:	00000128 	andeq	r0, r0, r8, lsr #2
    876c:	00000000 	andeq	r0, r0, r0
    8770:	00000024 	andeq	r0, r0, r4, lsr #32
    8774:	00000138 	andeq	r0, r0, r8, lsr r1
    8778:	00000000 	andeq	r0, r0, r0
    877c:	0000004e 	andeq	r0, r0, lr, asr #32
    8780:	80014528 	andhi	r4, r1, r8, lsr #10
    8784:	00003442 	andeq	r3, r0, r2, asr #8
    8788:	00330024 	eorseq	r0, r3, r4, lsr #32
    878c:	80014528 	andhi	r4, r1, r8, lsr #10
    8790:	0000342a 	andeq	r3, r0, sl, lsr #8
    8794:	003300a0 	eorseq	r0, r3, r0, lsr #1
    8798:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    879c:	00003435 	andeq	r3, r0, r5, lsr r4
    87a0:	003300a0 	eorseq	r0, r3, r0, lsr #1
    87a4:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    87a8:	00000000 	andeq	r0, r0, r0
    87ac:	0000002e 	andeq	r0, r0, lr, lsr #32
    87b0:	80014528 	andhi	r4, r1, r8, lsr #10
    87b4:	00000000 	andeq	r0, r0, r0
    87b8:	00330044 	eorseq	r0, r3, r4, asr #32
	...
    87c4:	00340044 	eorseq	r0, r4, r4, asr #32
    87c8:	00000018 	andeq	r0, r0, r8, lsl r0
    87cc:	00000000 	andeq	r0, r0, r0
    87d0:	00350044 	eorseq	r0, r5, r4, asr #32
    87d4:	00000024 	andeq	r0, r0, r4, lsr #32
    87d8:	00000000 	andeq	r0, r0, r0
    87dc:	00370044 	eorseq	r0, r7, r4, asr #32
    87e0:	00000028 	andeq	r0, r0, r8, lsr #32
    87e4:	00000000 	andeq	r0, r0, r0
    87e8:	00380044 	eorseq	r0, r8, r4, asr #32
    87ec:	00000038 	andeq	r0, r0, r8, lsr r0
    87f0:	00000000 	andeq	r0, r0, r0
    87f4:	003a0044 	eorseq	r0, sl, r4, asr #32
    87f8:	00000054 	andeq	r0, r0, r4, asr r0
    87fc:	00000000 	andeq	r0, r0, r0
    8800:	003b0044 	eorseq	r0, fp, r4, asr #32
    8804:	0000006c 	andeq	r0, r0, ip, rrx
    8808:	00000000 	andeq	r0, r0, r0
    880c:	00000024 	andeq	r0, r0, r4, lsr #32
    8810:	00000074 	andeq	r0, r0, r4, ror r0
    8814:	00000000 	andeq	r0, r0, r0
    8818:	0000004e 	andeq	r0, r0, lr, asr #32
    881c:	8001459c 	mulhi	r1, ip, r5
    8820:	00003450 	andeq	r3, r0, r0, asr r4
    8824:	00050026 	andeq	r0, r5, r6, lsr #32
    8828:	8024480c 	eorhi	r4, r4, ip, lsl #16
    882c:	00000000 	andeq	r0, r0, r0
    8830:	00000064 	andeq	r0, r0, r4, rrx
    8834:	8001459c 	mulhi	r1, ip, r5
    8838:	00000007 	andeq	r0, r0, r7
    883c:	00020064 	andeq	r0, r2, r4, rrx
    8840:	8001459c 	mulhi	r1, ip, r5
    8844:	00003465 	andeq	r3, r0, r5, ror #8
    8848:	00020064 	andeq	r0, r2, r4, rrx
    884c:	8001459c 	mulhi	r1, ip, r5
    8850:	0000003d 	andeq	r0, r0, sp, lsr r0
    8854:	0000003c 	andeq	r0, r0, ip, lsr r0
    8858:	00000000 	andeq	r0, r0, r0
    885c:	0000004c 	andeq	r0, r0, ip, asr #32
    8860:	00000080 	andeq	r0, r0, r0, lsl #1
    8864:	00000000 	andeq	r0, r0, r0
    8868:	00000076 	andeq	r0, r0, r6, ror r0
    886c:	00000080 	andeq	r0, r0, r0, lsl #1
    8870:	00000000 	andeq	r0, r0, r0
    8874:	00000094 	muleq	r0, r4, r0
    8878:	00000080 	andeq	r0, r0, r0, lsl #1
    887c:	00000000 	andeq	r0, r0, r0
    8880:	000000c3 	andeq	r0, r0, r3, asr #1
    8884:	00000080 	andeq	r0, r0, r0, lsl #1
    8888:	00000000 	andeq	r0, r0, r0
    888c:	000000ee 	andeq	r0, r0, lr, ror #1
    8890:	00000080 	andeq	r0, r0, r0, lsl #1
    8894:	00000000 	andeq	r0, r0, r0
    8898:	0000011e 	andeq	r0, r0, lr, lsl r1
    889c:	00000080 	andeq	r0, r0, r0, lsl #1
    88a0:	00000000 	andeq	r0, r0, r0
    88a4:	0000016f 	andeq	r0, r0, pc, ror #2
    88a8:	00000080 	andeq	r0, r0, r0, lsl #1
    88ac:	00000000 	andeq	r0, r0, r0
    88b0:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    88b4:	00000080 	andeq	r0, r0, r0, lsl #1
    88b8:	00000000 	andeq	r0, r0, r0
    88bc:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    88c0:	00000080 	andeq	r0, r0, r0, lsl #1
    88c4:	00000000 	andeq	r0, r0, r0
    88c8:	0000020e 	andeq	r0, r0, lr, lsl #4
    88cc:	00000080 	andeq	r0, r0, r0, lsl #1
    88d0:	00000000 	andeq	r0, r0, r0
    88d4:	00000238 	andeq	r0, r0, r8, lsr r2
    88d8:	00000080 	andeq	r0, r0, r0, lsl #1
    88dc:	00000000 	andeq	r0, r0, r0
    88e0:	00000261 	andeq	r0, r0, r1, ror #4
    88e4:	00000080 	andeq	r0, r0, r0, lsl #1
    88e8:	00000000 	andeq	r0, r0, r0
    88ec:	0000027b 	andeq	r0, r0, fp, ror r2
    88f0:	00000080 	andeq	r0, r0, r0, lsl #1
    88f4:	00000000 	andeq	r0, r0, r0
    88f8:	00000296 	muleq	r0, r6, r2
    88fc:	00000080 	andeq	r0, r0, r0, lsl #1
    8900:	00000000 	andeq	r0, r0, r0
    8904:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    8908:	00000080 	andeq	r0, r0, r0, lsl #1
    890c:	00000000 	andeq	r0, r0, r0
    8910:	000002d7 	ldrdeq	r0, [r0], -r7
    8914:	00000080 	andeq	r0, r0, r0, lsl #1
    8918:	00000000 	andeq	r0, r0, r0
    891c:	000002f7 	strdeq	r0, [r0], -r7
    8920:	00000080 	andeq	r0, r0, r0, lsl #1
    8924:	00000000 	andeq	r0, r0, r0
    8928:	0000031c 	andeq	r0, r0, ip, lsl r3
    892c:	00000080 	andeq	r0, r0, r0, lsl #1
    8930:	00000000 	andeq	r0, r0, r0
    8934:	00000346 	andeq	r0, r0, r6, asr #6
    8938:	00000080 	andeq	r0, r0, r0, lsl #1
    893c:	00000000 	andeq	r0, r0, r0
    8940:	0000036a 	andeq	r0, r0, sl, ror #6
    8944:	00000080 	andeq	r0, r0, r0, lsl #1
    8948:	00000000 	andeq	r0, r0, r0
    894c:	00000393 	muleq	r0, r3, r3
    8950:	00000080 	andeq	r0, r0, r0, lsl #1
    8954:	00000000 	andeq	r0, r0, r0
    8958:	000003c1 	andeq	r0, r0, r1, asr #7
    895c:	00000080 	andeq	r0, r0, r0, lsl #1
    8960:	00000000 	andeq	r0, r0, r0
    8964:	000003e7 	andeq	r0, r0, r7, ror #7
    8968:	00000080 	andeq	r0, r0, r0, lsl #1
    896c:	00000000 	andeq	r0, r0, r0
    8970:	00000407 	andeq	r0, r0, r7, lsl #8
    8974:	00000080 	andeq	r0, r0, r0, lsl #1
    8978:	00000000 	andeq	r0, r0, r0
    897c:	0000042c 	andeq	r0, r0, ip, lsr #8
    8980:	00000080 	andeq	r0, r0, r0, lsl #1
    8984:	00000000 	andeq	r0, r0, r0
    8988:	00000456 	andeq	r0, r0, r6, asr r4
    898c:	00000080 	andeq	r0, r0, r0, lsl #1
    8990:	00000000 	andeq	r0, r0, r0
    8994:	00000485 	andeq	r0, r0, r5, lsl #9
    8998:	00000080 	andeq	r0, r0, r0, lsl #1
    899c:	00000000 	andeq	r0, r0, r0
    89a0:	000004ae 	andeq	r0, r0, lr, lsr #9
    89a4:	00000080 	andeq	r0, r0, r0, lsl #1
    89a8:	00000000 	andeq	r0, r0, r0
    89ac:	000004dc 	ldrdeq	r0, [r0], -ip
    89b0:	00000080 	andeq	r0, r0, r0, lsl #1
    89b4:	00000000 	andeq	r0, r0, r0
    89b8:	0000050f 	andeq	r0, r0, pc, lsl #10
    89bc:	00000080 	andeq	r0, r0, r0, lsl #1
    89c0:	00000000 	andeq	r0, r0, r0
    89c4:	00000530 	andeq	r0, r0, r0, lsr r5
    89c8:	00000080 	andeq	r0, r0, r0, lsl #1
    89cc:	00000000 	andeq	r0, r0, r0
    89d0:	00000550 	andeq	r0, r0, r0, asr r5
    89d4:	00000080 	andeq	r0, r0, r0, lsl #1
    89d8:	00000000 	andeq	r0, r0, r0
    89dc:	00000575 	andeq	r0, r0, r5, ror r5
    89e0:	00000080 	andeq	r0, r0, r0, lsl #1
    89e4:	00000000 	andeq	r0, r0, r0
    89e8:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    89ec:	00000080 	andeq	r0, r0, r0, lsl #1
    89f0:	00000000 	andeq	r0, r0, r0
    89f4:	000005c3 	andeq	r0, r0, r3, asr #11
    89f8:	00000080 	andeq	r0, r0, r0, lsl #1
    89fc:	00000000 	andeq	r0, r0, r0
    8a00:	000005ec 	andeq	r0, r0, ip, ror #11
    8a04:	00000080 	andeq	r0, r0, r0, lsl #1
    8a08:	00000000 	andeq	r0, r0, r0
    8a0c:	0000061a 	andeq	r0, r0, sl, lsl r6
    8a10:	00000080 	andeq	r0, r0, r0, lsl #1
    8a14:	00000000 	andeq	r0, r0, r0
    8a18:	00000640 	andeq	r0, r0, r0, asr #12
    8a1c:	00000080 	andeq	r0, r0, r0, lsl #1
    8a20:	00000000 	andeq	r0, r0, r0
    8a24:	00000660 	andeq	r0, r0, r0, ror #12
    8a28:	00000080 	andeq	r0, r0, r0, lsl #1
    8a2c:	00000000 	andeq	r0, r0, r0
    8a30:	00000685 	andeq	r0, r0, r5, lsl #13
    8a34:	00000080 	andeq	r0, r0, r0, lsl #1
    8a38:	00000000 	andeq	r0, r0, r0
    8a3c:	000006af 	andeq	r0, r0, pc, lsr #13
    8a40:	00000080 	andeq	r0, r0, r0, lsl #1
    8a44:	00000000 	andeq	r0, r0, r0
    8a48:	000006de 	ldrdeq	r0, [r0], -lr
    8a4c:	00000080 	andeq	r0, r0, r0, lsl #1
    8a50:	00000000 	andeq	r0, r0, r0
    8a54:	00000707 	andeq	r0, r0, r7, lsl #14
    8a58:	00000080 	andeq	r0, r0, r0, lsl #1
    8a5c:	00000000 	andeq	r0, r0, r0
    8a60:	00000735 	andeq	r0, r0, r5, lsr r7
    8a64:	00000080 	andeq	r0, r0, r0, lsl #1
    8a68:	00000000 	andeq	r0, r0, r0
    8a6c:	00000768 	andeq	r0, r0, r8, ror #14
    8a70:	00000080 	andeq	r0, r0, r0, lsl #1
    8a74:	00000000 	andeq	r0, r0, r0
    8a78:	00001067 	andeq	r1, r0, r7, rrx
    8a7c:	000000c2 	andeq	r0, r0, r2, asr #1
    8a80:	00003803 	andeq	r3, r0, r3, lsl #16
    8a84:	0000078a 	andeq	r0, r0, sl, lsl #15
    8a88:	000000c2 	andeq	r0, r0, r2, asr #1
    8a8c:	00002d60 	andeq	r2, r0, r0, ror #26
    8a90:	00003474 	andeq	r3, r0, r4, ror r4
    8a94:	00040024 	andeq	r0, r4, r4, lsr #32
    8a98:	8001459c 	mulhi	r1, ip, r5
    8a9c:	00003489 	andeq	r3, r0, r9, lsl #9
    8aa0:	000400a0 	andeq	r0, r4, r0, lsr #1
    8aa4:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    8aa8:	0000349b 	muleq	r0, fp, r4
    8aac:	000400a0 	andeq	r0, r4, r0, lsr #1
    8ab0:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    8ab4:	00000000 	andeq	r0, r0, r0
    8ab8:	0000002e 	andeq	r0, r0, lr, lsr #32
    8abc:	8001459c 	mulhi	r1, ip, r5
    8ac0:	00000000 	andeq	r0, r0, r0
    8ac4:	00040044 	andeq	r0, r4, r4, asr #32
	...
    8ad0:	00050044 	andeq	r0, r5, r4, asr #32
    8ad4:	00000018 	andeq	r0, r0, r8, lsl r0
    8ad8:	00000000 	andeq	r0, r0, r0
    8adc:	00060044 	andeq	r0, r6, r4, asr #32
    8ae0:	00000024 	andeq	r0, r0, r4, lsr #32
    8ae4:	00000000 	andeq	r0, r0, r0
    8ae8:	00070044 	andeq	r0, r7, r4, asr #32
    8aec:	00000028 	andeq	r0, r0, r8, lsr #32
    8af0:	00000000 	andeq	r0, r0, r0
    8af4:	00080044 	andeq	r0, r8, r4, asr #32
    8af8:	00000038 	andeq	r0, r0, r8, lsr r0
    8afc:	00000000 	andeq	r0, r0, r0
    8b00:	00090044 	andeq	r0, r9, r4, asr #32
    8b04:	00000044 	andeq	r0, r0, r4, asr #32
    8b08:	00000000 	andeq	r0, r0, r0
    8b0c:	00060044 	andeq	r0, r6, r4, asr #32
    8b10:	00000050 	andeq	r0, r0, r0, asr r0
    8b14:	00000000 	andeq	r0, r0, r0
    8b18:	000b0044 	andeq	r0, fp, r4, asr #32
    8b1c:	0000005c 	andeq	r0, r0, ip, asr r0
    8b20:	00000000 	andeq	r0, r0, r0
    8b24:	000c0044 	andeq	r0, ip, r4, asr #32
    8b28:	00000068 	andeq	r0, r0, r8, rrx
    8b2c:	000034b6 			; <UNDEFINED> instruction: 0x000034b6
    8b30:	00050080 	andeq	r0, r5, r0, lsl #1
    8b34:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    8b38:	00000000 	andeq	r0, r0, r0
    8b3c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    8b48:	000000e0 	andeq	r0, r0, r0, ror #1
    8b4c:	00000070 	andeq	r0, r0, r0, ror r0
    8b50:	00000000 	andeq	r0, r0, r0
    8b54:	00000024 	andeq	r0, r0, r4, lsr #32
    8b58:	00000070 	andeq	r0, r0, r0, ror r0
    8b5c:	00000000 	andeq	r0, r0, r0
    8b60:	0000004e 	andeq	r0, r0, lr, asr #32
    8b64:	8001460c 	andhi	r4, r1, ip, lsl #12
    8b68:	000034c0 	andeq	r3, r0, r0, asr #9
    8b6c:	000e0024 	andeq	r0, lr, r4, lsr #32
    8b70:	8001460c 	andhi	r4, r1, ip, lsl #12
    8b74:	000034d4 	ldrdeq	r3, [r0], -r4
    8b78:	000e00a0 	andeq	r0, lr, r0, lsr #1
    8b7c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    8b80:	000034df 	ldrdeq	r3, [r0], -pc	; <UNPREDICTABLE>
    8b84:	000e00a0 	andeq	r0, lr, r0, lsr #1
    8b88:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    8b8c:	000034ea 	andeq	r3, r0, sl, ror #9
    8b90:	000e00a0 	andeq	r0, lr, r0, lsr #1
    8b94:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    8b98:	00000000 	andeq	r0, r0, r0
    8b9c:	0000002e 	andeq	r0, r0, lr, lsr #32
    8ba0:	8001460c 	andhi	r4, r1, ip, lsl #12
    8ba4:	00000000 	andeq	r0, r0, r0
    8ba8:	000e0044 	andeq	r0, lr, r4, asr #32
	...
    8bb4:	000f0044 	andeq	r0, pc, r4, asr #32
    8bb8:	0000001c 	andeq	r0, r0, ip, lsl r0
    8bbc:	00000000 	andeq	r0, r0, r0
    8bc0:	00100044 	andseq	r0, r0, r4, asr #32
    8bc4:	00000028 	andeq	r0, r0, r8, lsr #32
    8bc8:	00000000 	andeq	r0, r0, r0
    8bcc:	00150044 	andseq	r0, r5, r4, asr #32
    8bd0:	00000034 	andeq	r0, r0, r4, lsr r0
    8bd4:	00000000 	andeq	r0, r0, r0
    8bd8:	00160044 	andseq	r0, r6, r4, asr #32
    8bdc:	00000048 	andeq	r0, r0, r8, asr #32
    8be0:	00000000 	andeq	r0, r0, r0
    8be4:	00180044 	andseq	r0, r8, r4, asr #32
    8be8:	00000054 	andeq	r0, r0, r4, asr r0
    8bec:	00000000 	andeq	r0, r0, r0
    8bf0:	001b0044 	andseq	r0, fp, r4, asr #32
    8bf4:	00000060 	andeq	r0, r0, r0, rrx
    8bf8:	00000000 	andeq	r0, r0, r0
    8bfc:	001c0044 	andseq	r0, ip, r4, asr #32
    8c00:	00000070 	andeq	r0, r0, r0, ror r0
    8c04:	00000000 	andeq	r0, r0, r0
    8c08:	001d0044 	andseq	r0, sp, r4, asr #32
    8c0c:	00000088 	andeq	r0, r0, r8, lsl #1
    8c10:	00000000 	andeq	r0, r0, r0
    8c14:	001e0044 	andseq	r0, lr, r4, asr #32
    8c18:	0000009c 	muleq	r0, ip, r0
    8c1c:	00000000 	andeq	r0, r0, r0
    8c20:	00210044 	eoreq	r0, r1, r4, asr #32
    8c24:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    8c28:	00000000 	andeq	r0, r0, r0
    8c2c:	00220044 	eoreq	r0, r2, r4, asr #32
    8c30:	000000c8 	andeq	r0, r0, r8, asr #1
    8c34:	00000000 	andeq	r0, r0, r0
    8c38:	00250044 	eoreq	r0, r5, r4, asr #32
    8c3c:	000000d4 	ldrdeq	r0, [r0], -r4
    8c40:	00000000 	andeq	r0, r0, r0
    8c44:	00260044 	eoreq	r0, r6, r4, asr #32
    8c48:	000000e0 	andeq	r0, r0, r0, ror #1
    8c4c:	00000000 	andeq	r0, r0, r0
    8c50:	00280044 	eoreq	r0, r8, r4, asr #32
    8c54:	000000f8 	strdeq	r0, [r0], -r8
    8c58:	00000000 	andeq	r0, r0, r0
    8c5c:	00290044 	eoreq	r0, r9, r4, asr #32
    8c60:	00000108 	andeq	r0, r0, r8, lsl #2
    8c64:	00000000 	andeq	r0, r0, r0
    8c68:	002a0044 	eoreq	r0, sl, r4, asr #32
    8c6c:	00000114 	andeq	r0, r0, r4, lsl r1
    8c70:	00000000 	andeq	r0, r0, r0
    8c74:	002b0044 	eoreq	r0, fp, r4, asr #32
    8c78:	00000118 	andeq	r0, r0, r8, lsl r1
    8c7c:	00003510 	andeq	r3, r0, r0, lsl r5
    8c80:	00140080 	andseq	r0, r4, r0, lsl #1
    8c84:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    8c88:	000034b6 			; <UNDEFINED> instruction: 0x000034b6
    8c8c:	001b0080 	andseq	r0, fp, r0, lsl #1
    8c90:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    8c94:	00000000 	andeq	r0, r0, r0
    8c98:	000000c0 	andeq	r0, r0, r0, asr #1
    8c9c:	00000034 	andeq	r0, r0, r4, lsr r0
    8ca0:	00000000 	andeq	r0, r0, r0
    8ca4:	000000e0 	andeq	r0, r0, r0, ror #1
    8ca8:	00000114 	andeq	r0, r0, r4, lsl r1
    8cac:	00000000 	andeq	r0, r0, r0
    8cb0:	00000024 	andeq	r0, r0, r4, lsr #32
    8cb4:	00000120 	andeq	r0, r0, r0, lsr #2
    8cb8:	00000000 	andeq	r0, r0, r0
    8cbc:	0000004e 	andeq	r0, r0, lr, asr #32
    8cc0:	8001472c 	andhi	r4, r1, ip, lsr #14
    8cc4:	0000351e 	andeq	r3, r0, lr, lsl r5
    8cc8:	00300024 	eorseq	r0, r0, r4, lsr #32
    8ccc:	8001472c 	andhi	r4, r1, ip, lsr #14
    8cd0:	000034df 	ldrdeq	r3, [r0], -pc	; <UNPREDICTABLE>
    8cd4:	003000a0 	eorseq	r0, r0, r0, lsr #1
    8cd8:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    8cdc:	00003531 	andeq	r3, r0, r1, lsr r5
    8ce0:	003000a0 	eorseq	r0, r0, r0, lsr #1
    8ce4:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    8ce8:	0000353e 	andeq	r3, r0, lr, lsr r5
    8cec:	003000a0 	eorseq	r0, r0, r0, lsr #1
    8cf0:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    8cf4:	00000000 	andeq	r0, r0, r0
    8cf8:	0000002e 	andeq	r0, r0, lr, lsr #32
    8cfc:	8001472c 	andhi	r4, r1, ip, lsr #14
    8d00:	00000000 	andeq	r0, r0, r0
    8d04:	00300044 	eorseq	r0, r0, r4, asr #32
	...
    8d10:	00310044 	eorseq	r0, r1, r4, asr #32
    8d14:	0000001c 	andeq	r0, r0, ip, lsl r0
    8d18:	00000000 	andeq	r0, r0, r0
    8d1c:	00320044 	eorseq	r0, r2, r4, asr #32
    8d20:	00000028 	andeq	r0, r0, r8, lsr #32
    8d24:	00000000 	andeq	r0, r0, r0
    8d28:	00340044 	eorseq	r0, r4, r4, asr #32
    8d2c:	00000030 	andeq	r0, r0, r0, lsr r0
    8d30:	00000000 	andeq	r0, r0, r0
    8d34:	00350044 	eorseq	r0, r5, r4, asr #32
    8d38:	0000003c 	andeq	r0, r0, ip, lsr r0
    8d3c:	00000000 	andeq	r0, r0, r0
    8d40:	00360044 	eorseq	r0, r6, r4, asr #32
    8d44:	00000040 	andeq	r0, r0, r0, asr #32
    8d48:	00000000 	andeq	r0, r0, r0
    8d4c:	00370044 	eorseq	r0, r7, r4, asr #32
    8d50:	00000060 	andeq	r0, r0, r0, rrx
    8d54:	00000000 	andeq	r0, r0, r0
    8d58:	00380044 	eorseq	r0, r8, r4, asr #32
    8d5c:	00000070 	andeq	r0, r0, r0, ror r0
    8d60:	00000000 	andeq	r0, r0, r0
    8d64:	003a0044 	eorseq	r0, sl, r4, asr #32
    8d68:	0000007c 	andeq	r0, r0, ip, ror r0
    8d6c:	00000000 	andeq	r0, r0, r0
    8d70:	00350044 	eorseq	r0, r5, r4, asr #32
    8d74:	00000088 	andeq	r0, r0, r8, lsl #1
    8d78:	00000000 	andeq	r0, r0, r0
    8d7c:	003c0044 	eorseq	r0, ip, r4, asr #32
    8d80:	00000094 	muleq	r0, r4, r0
    8d84:	00000000 	andeq	r0, r0, r0
    8d88:	003d0044 	eorseq	r0, sp, r4, asr #32
    8d8c:	00000098 	muleq	r0, r8, r0
    8d90:	000034b6 			; <UNDEFINED> instruction: 0x000034b6
    8d94:	00340080 	eorseq	r0, r4, r0, lsl #1
    8d98:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    8d9c:	00000000 	andeq	r0, r0, r0
    8da0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    8dac:	000000e0 	andeq	r0, r0, r0, ror #1
    8db0:	000000a4 	andeq	r0, r0, r4, lsr #1
    8db4:	00000000 	andeq	r0, r0, r0
    8db8:	00000024 	andeq	r0, r0, r4, lsr #32
    8dbc:	000000a4 	andeq	r0, r0, r4, lsr #1
    8dc0:	00000000 	andeq	r0, r0, r0
    8dc4:	0000004e 	andeq	r0, r0, lr, asr #32
    8dc8:	800147d0 	ldrdhi	r4, [r1], -r0
    8dcc:	00000000 	andeq	r0, r0, r0
    8dd0:	00000064 	andeq	r0, r0, r4, rrx
    8dd4:	800147d0 	ldrdhi	r4, [r1], -r0
    8dd8:	00000007 	andeq	r0, r0, r7
    8ddc:	00020064 	andeq	r0, r2, r4, rrx
    8de0:	800147d0 	ldrdhi	r4, [r1], -r0
    8de4:	00003552 	andeq	r3, r0, r2, asr r5
    8de8:	00020064 	andeq	r0, r2, r4, rrx
    8dec:	800147d0 	ldrdhi	r4, [r1], -r0
    8df0:	0000003d 	andeq	r0, r0, sp, lsr r0
    8df4:	0000003c 	andeq	r0, r0, ip, lsr r0
    8df8:	00000000 	andeq	r0, r0, r0
    8dfc:	0000004c 	andeq	r0, r0, ip, asr #32
    8e00:	00000080 	andeq	r0, r0, r0, lsl #1
    8e04:	00000000 	andeq	r0, r0, r0
    8e08:	00000076 	andeq	r0, r0, r6, ror r0
    8e0c:	00000080 	andeq	r0, r0, r0, lsl #1
    8e10:	00000000 	andeq	r0, r0, r0
    8e14:	00000094 	muleq	r0, r4, r0
    8e18:	00000080 	andeq	r0, r0, r0, lsl #1
    8e1c:	00000000 	andeq	r0, r0, r0
    8e20:	000000c3 	andeq	r0, r0, r3, asr #1
    8e24:	00000080 	andeq	r0, r0, r0, lsl #1
    8e28:	00000000 	andeq	r0, r0, r0
    8e2c:	000000ee 	andeq	r0, r0, lr, ror #1
    8e30:	00000080 	andeq	r0, r0, r0, lsl #1
    8e34:	00000000 	andeq	r0, r0, r0
    8e38:	0000011e 	andeq	r0, r0, lr, lsl r1
    8e3c:	00000080 	andeq	r0, r0, r0, lsl #1
    8e40:	00000000 	andeq	r0, r0, r0
    8e44:	0000016f 	andeq	r0, r0, pc, ror #2
    8e48:	00000080 	andeq	r0, r0, r0, lsl #1
    8e4c:	00000000 	andeq	r0, r0, r0
    8e50:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    8e54:	00000080 	andeq	r0, r0, r0, lsl #1
    8e58:	00000000 	andeq	r0, r0, r0
    8e5c:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    8e60:	00000080 	andeq	r0, r0, r0, lsl #1
    8e64:	00000000 	andeq	r0, r0, r0
    8e68:	0000020e 	andeq	r0, r0, lr, lsl #4
    8e6c:	00000080 	andeq	r0, r0, r0, lsl #1
    8e70:	00000000 	andeq	r0, r0, r0
    8e74:	00000238 	andeq	r0, r0, r8, lsr r2
    8e78:	00000080 	andeq	r0, r0, r0, lsl #1
    8e7c:	00000000 	andeq	r0, r0, r0
    8e80:	00000261 	andeq	r0, r0, r1, ror #4
    8e84:	00000080 	andeq	r0, r0, r0, lsl #1
    8e88:	00000000 	andeq	r0, r0, r0
    8e8c:	0000027b 	andeq	r0, r0, fp, ror r2
    8e90:	00000080 	andeq	r0, r0, r0, lsl #1
    8e94:	00000000 	andeq	r0, r0, r0
    8e98:	00000296 	muleq	r0, r6, r2
    8e9c:	00000080 	andeq	r0, r0, r0, lsl #1
    8ea0:	00000000 	andeq	r0, r0, r0
    8ea4:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    8ea8:	00000080 	andeq	r0, r0, r0, lsl #1
    8eac:	00000000 	andeq	r0, r0, r0
    8eb0:	000002d7 	ldrdeq	r0, [r0], -r7
    8eb4:	00000080 	andeq	r0, r0, r0, lsl #1
    8eb8:	00000000 	andeq	r0, r0, r0
    8ebc:	000002f7 	strdeq	r0, [r0], -r7
    8ec0:	00000080 	andeq	r0, r0, r0, lsl #1
    8ec4:	00000000 	andeq	r0, r0, r0
    8ec8:	0000031c 	andeq	r0, r0, ip, lsl r3
    8ecc:	00000080 	andeq	r0, r0, r0, lsl #1
    8ed0:	00000000 	andeq	r0, r0, r0
    8ed4:	00000346 	andeq	r0, r0, r6, asr #6
    8ed8:	00000080 	andeq	r0, r0, r0, lsl #1
    8edc:	00000000 	andeq	r0, r0, r0
    8ee0:	0000036a 	andeq	r0, r0, sl, ror #6
    8ee4:	00000080 	andeq	r0, r0, r0, lsl #1
    8ee8:	00000000 	andeq	r0, r0, r0
    8eec:	00000393 	muleq	r0, r3, r3
    8ef0:	00000080 	andeq	r0, r0, r0, lsl #1
    8ef4:	00000000 	andeq	r0, r0, r0
    8ef8:	000003c1 	andeq	r0, r0, r1, asr #7
    8efc:	00000080 	andeq	r0, r0, r0, lsl #1
    8f00:	00000000 	andeq	r0, r0, r0
    8f04:	000003e7 	andeq	r0, r0, r7, ror #7
    8f08:	00000080 	andeq	r0, r0, r0, lsl #1
    8f0c:	00000000 	andeq	r0, r0, r0
    8f10:	00000407 	andeq	r0, r0, r7, lsl #8
    8f14:	00000080 	andeq	r0, r0, r0, lsl #1
    8f18:	00000000 	andeq	r0, r0, r0
    8f1c:	0000042c 	andeq	r0, r0, ip, lsr #8
    8f20:	00000080 	andeq	r0, r0, r0, lsl #1
    8f24:	00000000 	andeq	r0, r0, r0
    8f28:	00000456 	andeq	r0, r0, r6, asr r4
    8f2c:	00000080 	andeq	r0, r0, r0, lsl #1
    8f30:	00000000 	andeq	r0, r0, r0
    8f34:	00000485 	andeq	r0, r0, r5, lsl #9
    8f38:	00000080 	andeq	r0, r0, r0, lsl #1
    8f3c:	00000000 	andeq	r0, r0, r0
    8f40:	000004ae 	andeq	r0, r0, lr, lsr #9
    8f44:	00000080 	andeq	r0, r0, r0, lsl #1
    8f48:	00000000 	andeq	r0, r0, r0
    8f4c:	000004dc 	ldrdeq	r0, [r0], -ip
    8f50:	00000080 	andeq	r0, r0, r0, lsl #1
    8f54:	00000000 	andeq	r0, r0, r0
    8f58:	0000050f 	andeq	r0, r0, pc, lsl #10
    8f5c:	00000080 	andeq	r0, r0, r0, lsl #1
    8f60:	00000000 	andeq	r0, r0, r0
    8f64:	00000530 	andeq	r0, r0, r0, lsr r5
    8f68:	00000080 	andeq	r0, r0, r0, lsl #1
    8f6c:	00000000 	andeq	r0, r0, r0
    8f70:	00000550 	andeq	r0, r0, r0, asr r5
    8f74:	00000080 	andeq	r0, r0, r0, lsl #1
    8f78:	00000000 	andeq	r0, r0, r0
    8f7c:	00000575 	andeq	r0, r0, r5, ror r5
    8f80:	00000080 	andeq	r0, r0, r0, lsl #1
    8f84:	00000000 	andeq	r0, r0, r0
    8f88:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    8f8c:	00000080 	andeq	r0, r0, r0, lsl #1
    8f90:	00000000 	andeq	r0, r0, r0
    8f94:	000005c3 	andeq	r0, r0, r3, asr #11
    8f98:	00000080 	andeq	r0, r0, r0, lsl #1
    8f9c:	00000000 	andeq	r0, r0, r0
    8fa0:	000005ec 	andeq	r0, r0, ip, ror #11
    8fa4:	00000080 	andeq	r0, r0, r0, lsl #1
    8fa8:	00000000 	andeq	r0, r0, r0
    8fac:	0000061a 	andeq	r0, r0, sl, lsl r6
    8fb0:	00000080 	andeq	r0, r0, r0, lsl #1
    8fb4:	00000000 	andeq	r0, r0, r0
    8fb8:	00000640 	andeq	r0, r0, r0, asr #12
    8fbc:	00000080 	andeq	r0, r0, r0, lsl #1
    8fc0:	00000000 	andeq	r0, r0, r0
    8fc4:	00000660 	andeq	r0, r0, r0, ror #12
    8fc8:	00000080 	andeq	r0, r0, r0, lsl #1
    8fcc:	00000000 	andeq	r0, r0, r0
    8fd0:	00000685 	andeq	r0, r0, r5, lsl #13
    8fd4:	00000080 	andeq	r0, r0, r0, lsl #1
    8fd8:	00000000 	andeq	r0, r0, r0
    8fdc:	000006af 	andeq	r0, r0, pc, lsr #13
    8fe0:	00000080 	andeq	r0, r0, r0, lsl #1
    8fe4:	00000000 	andeq	r0, r0, r0
    8fe8:	000006de 	ldrdeq	r0, [r0], -lr
    8fec:	00000080 	andeq	r0, r0, r0, lsl #1
    8ff0:	00000000 	andeq	r0, r0, r0
    8ff4:	00000707 	andeq	r0, r0, r7, lsl #14
    8ff8:	00000080 	andeq	r0, r0, r0, lsl #1
    8ffc:	00000000 	andeq	r0, r0, r0
    9000:	00000735 	andeq	r0, r0, r5, lsr r7
    9004:	00000080 	andeq	r0, r0, r0, lsl #1
    9008:	00000000 	andeq	r0, r0, r0
    900c:	00000768 	andeq	r0, r0, r8, ror #14
    9010:	00000080 	andeq	r0, r0, r0, lsl #1
    9014:	00000000 	andeq	r0, r0, r0
    9018:	00002199 	muleq	r0, r9, r1
    901c:	00000082 	andeq	r0, r0, r2, lsl #1
    9020:	000015f6 	strdeq	r1, [r0], -r6
    9024:	0000355e 	andeq	r3, r0, lr, asr r5
    9028:	00090080 	andeq	r0, r9, r0, lsl #1
	...
    9034:	000000a2 	andeq	r0, r0, r2, lsr #1
    9038:	00000000 	andeq	r0, r0, r0
    903c:	000035c3 	andeq	r3, r0, r3, asr #11
    9040:	00000082 	andeq	r0, r0, r2, lsl #1
    9044:	00000000 	andeq	r0, r0, r0
    9048:	0000078a 	andeq	r0, r0, sl, lsl #15
    904c:	000000c2 	andeq	r0, r0, r2, asr #1
    9050:	00002d60 	andeq	r2, r0, r0, ror #26
    9054:	00000000 	andeq	r0, r0, r0
    9058:	000000a2 	andeq	r0, r0, r2, lsr #1
    905c:	00000000 	andeq	r0, r0, r0
    9060:	00001173 	andeq	r1, r0, r3, ror r1
    9064:	000000c2 	andeq	r0, r0, r2, asr #1
    9068:	0000c9ad 	andeq	ip, r0, sp, lsr #19
    906c:	00000886 	andeq	r0, r0, r6, lsl #17
    9070:	000000c2 	andeq	r0, r0, r2, asr #1
    9074:	00003ac8 	andeq	r3, r0, r8, asr #21
    9078:	00001182 	andeq	r1, r0, r2, lsl #3
    907c:	000000c2 	andeq	r0, r0, r2, asr #1
    9080:	00004df6 	strdeq	r4, [r0], -r6
    9084:	000012f3 	strdeq	r1, [r0], -r3
    9088:	000000c2 	andeq	r0, r0, r2, asr #1
    908c:	00002a19 	andeq	r2, r0, r9, lsl sl
    9090:	00001306 	andeq	r1, r0, r6, lsl #6
    9094:	000000c2 	andeq	r0, r0, r2, asr #1
    9098:	0000151b 	andeq	r1, r0, fp, lsl r5
    909c:	00001425 	andeq	r1, r0, r5, lsr #8
    90a0:	000000c2 	andeq	r0, r0, r2, asr #1
    90a4:	00001fc5 	andeq	r1, r0, r5, asr #31
    90a8:	000014c3 	andeq	r1, r0, r3, asr #9
    90ac:	000000c2 	andeq	r0, r0, r2, asr #1
    90b0:	00002279 	andeq	r2, r0, r9, ror r2
    90b4:	000035db 	ldrdeq	r3, [r0], -fp
    90b8:	000a0024 	andeq	r0, sl, r4, lsr #32
    90bc:	800147d0 	ldrdhi	r4, [r1], -r0
    90c0:	000035eb 	andeq	r3, r0, fp, ror #11
    90c4:	000a00a0 	andeq	r0, sl, r0, lsr #1
    90c8:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    90cc:	00000000 	andeq	r0, r0, r0
    90d0:	0000002e 	andeq	r0, r0, lr, lsr #32
    90d4:	800147d0 	ldrdhi	r4, [r1], -r0
    90d8:	00000000 	andeq	r0, r0, r0
    90dc:	000a0044 	andeq	r0, sl, r4, asr #32
	...
    90e8:	000b0044 	andeq	r0, fp, r4, asr #32
    90ec:	0000001c 	andeq	r0, r0, ip, lsl r0
    90f0:	00000000 	andeq	r0, r0, r0
    90f4:	000d0044 	andeq	r0, sp, r4, asr #32
    90f8:	00000030 	andeq	r0, r0, r0, lsr r0
    90fc:	00000000 	andeq	r0, r0, r0
    9100:	000e0044 	andeq	r0, lr, r4, asr #32
    9104:	0000003c 	andeq	r0, r0, ip, lsr r0
    9108:	00000000 	andeq	r0, r0, r0
    910c:	000f0044 	andeq	r0, pc, r4, asr #32
    9110:	00000060 	andeq	r0, r0, r0, rrx
    9114:	00000000 	andeq	r0, r0, r0
    9118:	00110044 	andseq	r0, r1, r4, asr #32
    911c:	00000064 	andeq	r0, r0, r4, rrx
    9120:	00000000 	andeq	r0, r0, r0
    9124:	00120044 	andseq	r0, r2, r4, asr #32
    9128:	00000094 	muleq	r0, r4, r0
    912c:	00000000 	andeq	r0, r0, r0
    9130:	00130044 	andseq	r0, r3, r4, asr #32
    9134:	000000b8 	strheq	r0, [r0], -r8
    9138:	00000000 	andeq	r0, r0, r0
    913c:	000d0044 	andeq	r0, sp, r4, asr #32
    9140:	000000c0 	andeq	r0, r0, r0, asr #1
    9144:	00000000 	andeq	r0, r0, r0
    9148:	000d0044 	andeq	r0, sp, r4, asr #32
    914c:	000000cc 	andeq	r0, r0, ip, asr #1
    9150:	00000000 	andeq	r0, r0, r0
    9154:	00170044 	andseq	r0, r7, r4, asr #32
    9158:	000000d8 	ldrdeq	r0, [r0], -r8
    915c:	00000000 	andeq	r0, r0, r0
    9160:	00180044 	andseq	r0, r8, r4, asr #32
    9164:	000000e4 	andeq	r0, r0, r4, ror #1
    9168:	00000000 	andeq	r0, r0, r0
    916c:	001b0044 	andseq	r0, fp, r4, asr #32
    9170:	000000ec 	andeq	r0, r0, ip, ror #1
    9174:	00000000 	andeq	r0, r0, r0
    9178:	001c0044 	andseq	r0, ip, r4, asr #32
    917c:	00000114 	andeq	r0, r0, r4, lsl r1
    9180:	00000000 	andeq	r0, r0, r0
    9184:	001d0044 	andseq	r0, sp, r4, asr #32
    9188:	00000138 	andeq	r0, r0, r8, lsr r1
    918c:	00000000 	andeq	r0, r0, r0
    9190:	001e0044 	andseq	r0, lr, r4, asr #32
    9194:	0000013c 	andeq	r0, r0, ip, lsr r1
    9198:	00003607 	andeq	r3, r0, r7, lsl #12
    919c:	000b0080 	andeq	r0, fp, r0, lsl #1
    91a0:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    91a4:	00000ecb 	andeq	r0, r0, fp, asr #29
    91a8:	000c0080 	andeq	r0, ip, r0, lsl #1
    91ac:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    91b0:	00000000 	andeq	r0, r0, r0
    91b4:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    91c0:	000000e0 	andeq	r0, r0, r0, ror #1
    91c4:	00000164 	andeq	r0, r0, r4, ror #2
    91c8:	00000000 	andeq	r0, r0, r0
    91cc:	00000024 	andeq	r0, r0, r4, lsr #32
    91d0:	00000164 	andeq	r0, r0, r4, ror #2
    91d4:	00000000 	andeq	r0, r0, r0
    91d8:	0000004e 	andeq	r0, r0, lr, asr #32
    91dc:	80014934 	andhi	r4, r1, r4, lsr r9
    91e0:	00003611 	andeq	r3, r0, r1, lsl r6
    91e4:	00200024 	eoreq	r0, r0, r4, lsr #32
    91e8:	80014934 	andhi	r4, r1, r4, lsr r9
    91ec:	00003621 	andeq	r3, r0, r1, lsr #12
    91f0:	002000a0 	eoreq	r0, r0, r0, lsr #1
    91f4:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    91f8:	00000000 	andeq	r0, r0, r0
    91fc:	0000002e 	andeq	r0, r0, lr, lsr #32
    9200:	80014934 	andhi	r4, r1, r4, lsr r9
    9204:	00000000 	andeq	r0, r0, r0
    9208:	00200044 	eoreq	r0, r0, r4, asr #32
	...
    9214:	00220044 	eoreq	r0, r2, r4, asr #32
    9218:	00000014 	andeq	r0, r0, r4, lsl r0
    921c:	00000000 	andeq	r0, r0, r0
    9220:	00230044 	eoreq	r0, r3, r4, asr #32
    9224:	00000020 	andeq	r0, r0, r0, lsr #32
    9228:	00000000 	andeq	r0, r0, r0
    922c:	00240044 	eoreq	r0, r4, r4, asr #32
    9230:	00000044 	andeq	r0, r0, r4, asr #32
    9234:	00000000 	andeq	r0, r0, r0
    9238:	00260044 	eoreq	r0, r6, r4, asr #32
    923c:	00000048 	andeq	r0, r0, r8, asr #32
    9240:	00000000 	andeq	r0, r0, r0
    9244:	00270044 	eoreq	r0, r7, r4, asr #32
    9248:	00000078 	andeq	r0, r0, r8, ror r0
    924c:	00000000 	andeq	r0, r0, r0
    9250:	00220044 	eoreq	r0, r2, r4, asr #32
    9254:	0000009c 	muleq	r0, ip, r0
    9258:	00000000 	andeq	r0, r0, r0
    925c:	00220044 	eoreq	r0, r2, r4, asr #32
    9260:	000000a8 	andeq	r0, r0, r8, lsr #1
    9264:	00000000 	andeq	r0, r0, r0
    9268:	002a0044 	eoreq	r0, sl, r4, asr #32
    926c:	000000b4 	strheq	r0, [r0], -r4
    9270:	00000000 	andeq	r0, r0, r0
    9274:	002b0044 	eoreq	r0, fp, r4, asr #32
    9278:	000000b8 	strheq	r0, [r0], -r8
    927c:	00000ecb 	andeq	r0, r0, fp, asr #29
    9280:	00210080 	eoreq	r0, r1, r0, lsl #1
    9284:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9288:	00000000 	andeq	r0, r0, r0
    928c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    9298:	000000e0 	andeq	r0, r0, r0, ror #1
    929c:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    92a0:	00000000 	andeq	r0, r0, r0
    92a4:	00000024 	andeq	r0, r0, r4, lsr #32
    92a8:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    92ac:	00000000 	andeq	r0, r0, r0
    92b0:	0000004e 	andeq	r0, r0, lr, asr #32
    92b4:	80014a04 	andhi	r4, r1, r4, lsl #20
    92b8:	0000362e 	andeq	r3, r0, lr, lsr #12
    92bc:	00060026 	andeq	r0, r6, r6, lsr #32
    92c0:	8001526c 	andhi	r5, r1, ip, ror #4
    92c4:	0000364a 	andeq	r3, r0, sl, asr #12
    92c8:	00080028 	andeq	r0, r8, r8, lsr #32
    92cc:	80244810 	eorhi	r4, r4, r0, lsl r8
    92d0:	00000000 	andeq	r0, r0, r0
    92d4:	00000064 	andeq	r0, r0, r4, rrx
    92d8:	80014a04 	andhi	r4, r1, r4, lsl #20
    92dc:	00000007 	andeq	r0, r0, r7
    92e0:	00020064 	andeq	r0, r2, r4, rrx
    92e4:	80014a04 	andhi	r4, r1, r4, lsl #20
    92e8:	00003675 	andeq	r3, r0, r5, ror r6
    92ec:	00020064 	andeq	r0, r2, r4, rrx
    92f0:	80014a04 	andhi	r4, r1, r4, lsl #20
    92f4:	0000003d 	andeq	r0, r0, sp, lsr r0
    92f8:	0000003c 	andeq	r0, r0, ip, lsr r0
    92fc:	00000000 	andeq	r0, r0, r0
    9300:	0000004c 	andeq	r0, r0, ip, asr #32
    9304:	00000080 	andeq	r0, r0, r0, lsl #1
    9308:	00000000 	andeq	r0, r0, r0
    930c:	00000076 	andeq	r0, r0, r6, ror r0
    9310:	00000080 	andeq	r0, r0, r0, lsl #1
    9314:	00000000 	andeq	r0, r0, r0
    9318:	00000094 	muleq	r0, r4, r0
    931c:	00000080 	andeq	r0, r0, r0, lsl #1
    9320:	00000000 	andeq	r0, r0, r0
    9324:	000000c3 	andeq	r0, r0, r3, asr #1
    9328:	00000080 	andeq	r0, r0, r0, lsl #1
    932c:	00000000 	andeq	r0, r0, r0
    9330:	000000ee 	andeq	r0, r0, lr, ror #1
    9334:	00000080 	andeq	r0, r0, r0, lsl #1
    9338:	00000000 	andeq	r0, r0, r0
    933c:	0000011e 	andeq	r0, r0, lr, lsl r1
    9340:	00000080 	andeq	r0, r0, r0, lsl #1
    9344:	00000000 	andeq	r0, r0, r0
    9348:	0000016f 	andeq	r0, r0, pc, ror #2
    934c:	00000080 	andeq	r0, r0, r0, lsl #1
    9350:	00000000 	andeq	r0, r0, r0
    9354:	000001b4 			; <UNDEFINED> instruction: 0x000001b4
    9358:	00000080 	andeq	r0, r0, r0, lsl #1
    935c:	00000000 	andeq	r0, r0, r0
    9360:	000001df 	ldrdeq	r0, [r0], -pc	; <UNPREDICTABLE>
    9364:	00000080 	andeq	r0, r0, r0, lsl #1
    9368:	00000000 	andeq	r0, r0, r0
    936c:	0000020e 	andeq	r0, r0, lr, lsl #4
    9370:	00000080 	andeq	r0, r0, r0, lsl #1
    9374:	00000000 	andeq	r0, r0, r0
    9378:	00000238 	andeq	r0, r0, r8, lsr r2
    937c:	00000080 	andeq	r0, r0, r0, lsl #1
    9380:	00000000 	andeq	r0, r0, r0
    9384:	00000261 	andeq	r0, r0, r1, ror #4
    9388:	00000080 	andeq	r0, r0, r0, lsl #1
    938c:	00000000 	andeq	r0, r0, r0
    9390:	0000027b 	andeq	r0, r0, fp, ror r2
    9394:	00000080 	andeq	r0, r0, r0, lsl #1
    9398:	00000000 	andeq	r0, r0, r0
    939c:	00000296 	muleq	r0, r6, r2
    93a0:	00000080 	andeq	r0, r0, r0, lsl #1
    93a4:	00000000 	andeq	r0, r0, r0
    93a8:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    93ac:	00000080 	andeq	r0, r0, r0, lsl #1
    93b0:	00000000 	andeq	r0, r0, r0
    93b4:	000002d7 	ldrdeq	r0, [r0], -r7
    93b8:	00000080 	andeq	r0, r0, r0, lsl #1
    93bc:	00000000 	andeq	r0, r0, r0
    93c0:	000002f7 	strdeq	r0, [r0], -r7
    93c4:	00000080 	andeq	r0, r0, r0, lsl #1
    93c8:	00000000 	andeq	r0, r0, r0
    93cc:	0000031c 	andeq	r0, r0, ip, lsl r3
    93d0:	00000080 	andeq	r0, r0, r0, lsl #1
    93d4:	00000000 	andeq	r0, r0, r0
    93d8:	00000346 	andeq	r0, r0, r6, asr #6
    93dc:	00000080 	andeq	r0, r0, r0, lsl #1
    93e0:	00000000 	andeq	r0, r0, r0
    93e4:	0000036a 	andeq	r0, r0, sl, ror #6
    93e8:	00000080 	andeq	r0, r0, r0, lsl #1
    93ec:	00000000 	andeq	r0, r0, r0
    93f0:	00000393 	muleq	r0, r3, r3
    93f4:	00000080 	andeq	r0, r0, r0, lsl #1
    93f8:	00000000 	andeq	r0, r0, r0
    93fc:	000003c1 	andeq	r0, r0, r1, asr #7
    9400:	00000080 	andeq	r0, r0, r0, lsl #1
    9404:	00000000 	andeq	r0, r0, r0
    9408:	000003e7 	andeq	r0, r0, r7, ror #7
    940c:	00000080 	andeq	r0, r0, r0, lsl #1
    9410:	00000000 	andeq	r0, r0, r0
    9414:	00000407 	andeq	r0, r0, r7, lsl #8
    9418:	00000080 	andeq	r0, r0, r0, lsl #1
    941c:	00000000 	andeq	r0, r0, r0
    9420:	0000042c 	andeq	r0, r0, ip, lsr #8
    9424:	00000080 	andeq	r0, r0, r0, lsl #1
    9428:	00000000 	andeq	r0, r0, r0
    942c:	00000456 	andeq	r0, r0, r6, asr r4
    9430:	00000080 	andeq	r0, r0, r0, lsl #1
    9434:	00000000 	andeq	r0, r0, r0
    9438:	00000485 	andeq	r0, r0, r5, lsl #9
    943c:	00000080 	andeq	r0, r0, r0, lsl #1
    9440:	00000000 	andeq	r0, r0, r0
    9444:	000004ae 	andeq	r0, r0, lr, lsr #9
    9448:	00000080 	andeq	r0, r0, r0, lsl #1
    944c:	00000000 	andeq	r0, r0, r0
    9450:	000004dc 	ldrdeq	r0, [r0], -ip
    9454:	00000080 	andeq	r0, r0, r0, lsl #1
    9458:	00000000 	andeq	r0, r0, r0
    945c:	0000050f 	andeq	r0, r0, pc, lsl #10
    9460:	00000080 	andeq	r0, r0, r0, lsl #1
    9464:	00000000 	andeq	r0, r0, r0
    9468:	00000530 	andeq	r0, r0, r0, lsr r5
    946c:	00000080 	andeq	r0, r0, r0, lsl #1
    9470:	00000000 	andeq	r0, r0, r0
    9474:	00000550 	andeq	r0, r0, r0, asr r5
    9478:	00000080 	andeq	r0, r0, r0, lsl #1
    947c:	00000000 	andeq	r0, r0, r0
    9480:	00000575 	andeq	r0, r0, r5, ror r5
    9484:	00000080 	andeq	r0, r0, r0, lsl #1
    9488:	00000000 	andeq	r0, r0, r0
    948c:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    9490:	00000080 	andeq	r0, r0, r0, lsl #1
    9494:	00000000 	andeq	r0, r0, r0
    9498:	000005c3 	andeq	r0, r0, r3, asr #11
    949c:	00000080 	andeq	r0, r0, r0, lsl #1
    94a0:	00000000 	andeq	r0, r0, r0
    94a4:	000005ec 	andeq	r0, r0, ip, ror #11
    94a8:	00000080 	andeq	r0, r0, r0, lsl #1
    94ac:	00000000 	andeq	r0, r0, r0
    94b0:	0000061a 	andeq	r0, r0, sl, lsl r6
    94b4:	00000080 	andeq	r0, r0, r0, lsl #1
    94b8:	00000000 	andeq	r0, r0, r0
    94bc:	00000640 	andeq	r0, r0, r0, asr #12
    94c0:	00000080 	andeq	r0, r0, r0, lsl #1
    94c4:	00000000 	andeq	r0, r0, r0
    94c8:	00000660 	andeq	r0, r0, r0, ror #12
    94cc:	00000080 	andeq	r0, r0, r0, lsl #1
    94d0:	00000000 	andeq	r0, r0, r0
    94d4:	00000685 	andeq	r0, r0, r5, lsl #13
    94d8:	00000080 	andeq	r0, r0, r0, lsl #1
    94dc:	00000000 	andeq	r0, r0, r0
    94e0:	000006af 	andeq	r0, r0, pc, lsr #13
    94e4:	00000080 	andeq	r0, r0, r0, lsl #1
    94e8:	00000000 	andeq	r0, r0, r0
    94ec:	000006de 	ldrdeq	r0, [r0], -lr
    94f0:	00000080 	andeq	r0, r0, r0, lsl #1
    94f4:	00000000 	andeq	r0, r0, r0
    94f8:	00000707 	andeq	r0, r0, r7, lsl #14
    94fc:	00000080 	andeq	r0, r0, r0, lsl #1
    9500:	00000000 	andeq	r0, r0, r0
    9504:	00000735 	andeq	r0, r0, r5, lsr r7
    9508:	00000080 	andeq	r0, r0, r0, lsl #1
    950c:	00000000 	andeq	r0, r0, r0
    9510:	00000768 	andeq	r0, r0, r8, ror #14
    9514:	00000080 	andeq	r0, r0, r0, lsl #1
    9518:	00000000 	andeq	r0, r0, r0
    951c:	00001173 	andeq	r1, r0, r3, ror r1
    9520:	000000c2 	andeq	r0, r0, r2, asr #1
    9524:	0000c9ad 	andeq	ip, r0, sp, lsr #19
    9528:	0000078a 	andeq	r0, r0, sl, lsl #15
    952c:	000000c2 	andeq	r0, r0, r2, asr #1
    9530:	00002d60 	andeq	r2, r0, r0, ror #26
    9534:	00000886 	andeq	r0, r0, r6, lsl #17
    9538:	000000c2 	andeq	r0, r0, r2, asr #1
    953c:	00003ac8 	andeq	r3, r0, r8, asr #21
    9540:	00001182 	andeq	r1, r0, r2, lsl #3
    9544:	000000c2 	andeq	r0, r0, r2, asr #1
    9548:	00004df6 	strdeq	r4, [r0], -r6
    954c:	000012f3 	strdeq	r1, [r0], -r3
    9550:	000000c2 	andeq	r0, r0, r2, asr #1
    9554:	00002a19 	andeq	r2, r0, r9, lsl sl
    9558:	00001306 	andeq	r1, r0, r6, lsl #6
    955c:	000000c2 	andeq	r0, r0, r2, asr #1
    9560:	0000151b 	andeq	r1, r0, fp, lsl r5
    9564:	00001425 	andeq	r1, r0, r5, lsr #8
    9568:	000000c2 	andeq	r0, r0, r2, asr #1
    956c:	00001fc5 	andeq	r1, r0, r5, asr #31
    9570:	000014c3 	andeq	r1, r0, r3, asr #9
    9574:	000000c2 	andeq	r0, r0, r2, asr #1
    9578:	00002743 	andeq	r2, r0, r3, asr #14
    957c:	0000077c 	andeq	r0, r0, ip, ror r7
    9580:	000000c2 	andeq	r0, r0, r2, asr #1
    9584:	00000927 	andeq	r0, r0, r7, lsr #18
    9588:	00003685 	andeq	r3, r0, r5, lsl #13
    958c:	00080024 	andeq	r0, r8, r4, lsr #32
    9590:	80014a04 	andhi	r4, r1, r4, lsl #20
    9594:	00000000 	andeq	r0, r0, r0
    9598:	0000002e 	andeq	r0, r0, lr, lsr #32
    959c:	80014a04 	andhi	r4, r1, r4, lsl #20
    95a0:	00000000 	andeq	r0, r0, r0
    95a4:	00090044 	andeq	r0, r9, r4, asr #32
	...
    95b0:	000a0044 	andeq	r0, sl, r4, asr #32
    95b4:	00000018 	andeq	r0, r0, r8, lsl r0
    95b8:	00000000 	andeq	r0, r0, r0
    95bc:	000b0044 	andeq	r0, fp, r4, asr #32
    95c0:	00000024 	andeq	r0, r0, r4, lsr #32
    95c4:	00000000 	andeq	r0, r0, r0
    95c8:	000d0044 	andeq	r0, sp, r4, asr #32
    95cc:	0000002c 	andeq	r0, r0, ip, lsr #32
    95d0:	00000000 	andeq	r0, r0, r0
    95d4:	000e0044 	andeq	r0, lr, r4, asr #32
    95d8:	00000048 	andeq	r0, r0, r8, asr #32
    95dc:	00000000 	andeq	r0, r0, r0
    95e0:	00100044 	andseq	r0, r0, r4, asr #32
    95e4:	0000005c 	andeq	r0, r0, ip, asr r0
    95e8:	00000000 	andeq	r0, r0, r0
    95ec:	00130044 	andseq	r0, r3, r4, asr #32
    95f0:	0000006c 	andeq	r0, r0, ip, rrx
    95f4:	00000000 	andeq	r0, r0, r0
    95f8:	00150044 	andseq	r0, r5, r4, asr #32
    95fc:	00000090 	muleq	r0, r0, r0
    9600:	00000000 	andeq	r0, r0, r0
    9604:	00160044 	andseq	r0, r6, r4, asr #32
    9608:	000000a0 	andeq	r0, r0, r0, lsr #1
    960c:	00000000 	andeq	r0, r0, r0
    9610:	00170044 	andseq	r0, r7, r4, asr #32
    9614:	000000b0 	strheq	r0, [r0], -r0	; <UNPREDICTABLE>
    9618:	00000000 	andeq	r0, r0, r0
    961c:	000a0044 	andeq	r0, sl, r4, asr #32
    9620:	000000b8 	strheq	r0, [r0], -r8
    9624:	00000000 	andeq	r0, r0, r0
    9628:	000a0044 	andeq	r0, sl, r4, asr #32
    962c:	000000c4 	andeq	r0, r0, r4, asr #1
    9630:	00000000 	andeq	r0, r0, r0
    9634:	001c0044 	andseq	r0, ip, r4, asr #32
    9638:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    963c:	00000ecb 	andeq	r0, r0, fp, asr #29
    9640:	000a0080 	andeq	r0, sl, r0, lsl #1
    9644:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9648:	00000000 	andeq	r0, r0, r0
    964c:	000000c0 	andeq	r0, r0, r0, asr #1
    9650:	00000018 	andeq	r0, r0, r8, lsl r0
    9654:	00003696 	muleq	r0, r6, r6
    9658:	000b0080 	andeq	r0, fp, r0, lsl #1
    965c:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    9660:	00000000 	andeq	r0, r0, r0
    9664:	000000c0 	andeq	r0, r0, r0, asr #1
    9668:	00000024 	andeq	r0, r0, r4, lsr #32
    966c:	00000000 	andeq	r0, r0, r0
    9670:	000000e0 	andeq	r0, r0, r0, ror #1
    9674:	000000b8 	strheq	r0, [r0], -r8
    9678:	00000000 	andeq	r0, r0, r0
    967c:	000000e0 	andeq	r0, r0, r0, ror #1
    9680:	000000d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
    9684:	00000000 	andeq	r0, r0, r0
    9688:	00000024 	andeq	r0, r0, r4, lsr #32
    968c:	000000f4 	strdeq	r0, [r0], -r4
    9690:	00000000 	andeq	r0, r0, r0
    9694:	0000004e 	andeq	r0, r0, lr, asr #32
    9698:	80014af8 	strdhi	r4, [r1], -r8
    969c:	000036a9 	andeq	r3, r0, r9, lsr #13
    96a0:	001f0024 	andseq	r0, pc, r4, lsr #32
    96a4:	80014af8 	strdhi	r4, [r1], -r8
    96a8:	00000000 	andeq	r0, r0, r0
    96ac:	0000002e 	andeq	r0, r0, lr, lsr #32
    96b0:	80014af8 	strdhi	r4, [r1], -r8
    96b4:	00000000 	andeq	r0, r0, r0
    96b8:	00200044 	eoreq	r0, r0, r4, asr #32
	...
    96c4:	00210044 	eoreq	r0, r1, r4, asr #32
    96c8:	0000000c 	andeq	r0, r0, ip
    96cc:	00000000 	andeq	r0, r0, r0
    96d0:	00220044 	eoreq	r0, r2, r4, asr #32
    96d4:	00000010 	andeq	r0, r0, r0, lsl r0
    96d8:	00000000 	andeq	r0, r0, r0
    96dc:	00230044 	eoreq	r0, r3, r4, asr #32
    96e0:	00000014 	andeq	r0, r0, r4, lsl r0
    96e4:	00000000 	andeq	r0, r0, r0
    96e8:	00000024 	andeq	r0, r0, r4, lsr #32
    96ec:	00000018 	andeq	r0, r0, r8, lsl r0
    96f0:	00000000 	andeq	r0, r0, r0
    96f4:	0000004e 	andeq	r0, r0, lr, asr #32
    96f8:	80014b10 	andhi	r4, r1, r0, lsl fp
    96fc:	000036bd 			; <UNDEFINED> instruction: 0x000036bd
    9700:	00270024 	eoreq	r0, r7, r4, lsr #32
    9704:	80014b10 	andhi	r4, r1, r0, lsl fp
    9708:	00000000 	andeq	r0, r0, r0
    970c:	0000002e 	andeq	r0, r0, lr, lsr #32
    9710:	80014b10 	andhi	r4, r1, r0, lsl fp
    9714:	00000000 	andeq	r0, r0, r0
    9718:	00280044 	eoreq	r0, r8, r4, asr #32
	...
    9724:	00290044 	eoreq	r0, r9, r4, asr #32
    9728:	00000014 	andeq	r0, r0, r4, lsl r0
    972c:	00000000 	andeq	r0, r0, r0
    9730:	002a0044 	eoreq	r0, sl, r4, asr #32
    9734:	00000024 	andeq	r0, r0, r4, lsr #32
    9738:	00000000 	andeq	r0, r0, r0
    973c:	002b0044 	eoreq	r0, fp, r4, asr #32
    9740:	00000030 	andeq	r0, r0, r0, lsr r0
    9744:	00000000 	andeq	r0, r0, r0
    9748:	002c0044 	eoreq	r0, ip, r4, asr #32
    974c:	0000004c 	andeq	r0, r0, ip, asr #32
    9750:	00000000 	andeq	r0, r0, r0
    9754:	00000024 	andeq	r0, r0, r4, lsr #32
    9758:	0000005c 	andeq	r0, r0, ip, asr r0
    975c:	00000000 	andeq	r0, r0, r0
    9760:	0000004e 	andeq	r0, r0, lr, asr #32
    9764:	80014b6c 	andhi	r4, r1, ip, ror #22
    9768:	00001921 	andeq	r1, r0, r1, lsr #18
    976c:	00060026 	andeq	r0, r6, r6, lsr #32
    9770:	80015270 	andhi	r5, r1, r0, ror r2
    9774:	000036d3 	ldrdeq	r3, [r0], -r3	; <UNPREDICTABLE>
    9778:	00060028 	andeq	r0, r6, r8, lsr #32
    977c:	80244b10 	eorhi	r4, r4, r0, lsl fp
    9780:	00000000 	andeq	r0, r0, r0
    9784:	00000064 	andeq	r0, r0, r4, rrx
    9788:	80014b6c 	andhi	r4, r1, ip, ror #22
    978c:	000036ea 	andeq	r3, r0, sl, ror #13
    9790:	00020064 	andeq	r0, r2, r4, rrx
    9794:	80014b6c 	andhi	r4, r1, ip, ror #22
    9798:	0000003d 	andeq	r0, r0, sp, lsr r0
    979c:	0000003c 	andeq	r0, r0, ip, lsr r0
    97a0:	00000000 	andeq	r0, r0, r0
    97a4:	0000004c 	andeq	r0, r0, ip, asr #32
    97a8:	00000080 	andeq	r0, r0, r0, lsl #1
    97ac:	00000000 	andeq	r0, r0, r0
    97b0:	000036fe 	strdeq	r3, [r0], -lr
    97b4:	00000080 	andeq	r0, r0, r0, lsl #1
    97b8:	00000000 	andeq	r0, r0, r0
    97bc:	00000094 	muleq	r0, r4, r0
    97c0:	00000080 	andeq	r0, r0, r0, lsl #1
    97c4:	00000000 	andeq	r0, r0, r0
    97c8:	00003718 	andeq	r3, r0, r8, lsl r7
    97cc:	00000080 	andeq	r0, r0, r0, lsl #1
    97d0:	00000000 	andeq	r0, r0, r0
    97d4:	00003741 	andeq	r3, r0, r1, asr #14
    97d8:	00000080 	andeq	r0, r0, r0, lsl #1
    97dc:	00000000 	andeq	r0, r0, r0
    97e0:	0000376f 	andeq	r3, r0, pc, ror #14
    97e4:	00000080 	andeq	r0, r0, r0, lsl #1
    97e8:	00000000 	andeq	r0, r0, r0
    97ec:	0000379a 	muleq	r0, sl, r7
    97f0:	00000080 	andeq	r0, r0, r0, lsl #1
    97f4:	00000000 	andeq	r0, r0, r0
    97f8:	000037c5 	andeq	r3, r0, r5, asr #15
    97fc:	00000080 	andeq	r0, r0, r0, lsl #1
    9800:	00000000 	andeq	r0, r0, r0
    9804:	000037eb 	andeq	r3, r0, fp, ror #15
    9808:	00000080 	andeq	r0, r0, r0, lsl #1
    980c:	00000000 	andeq	r0, r0, r0
    9810:	00003815 	andeq	r3, r0, r5, lsl r8
    9814:	00000080 	andeq	r0, r0, r0, lsl #1
    9818:	00000000 	andeq	r0, r0, r0
    981c:	0000383b 	andeq	r3, r0, fp, lsr r8
    9820:	00000080 	andeq	r0, r0, r0, lsl #1
    9824:	00000000 	andeq	r0, r0, r0
    9828:	00000261 	andeq	r0, r0, r1, ror #4
    982c:	00000080 	andeq	r0, r0, r0, lsl #1
    9830:	00000000 	andeq	r0, r0, r0
    9834:	0000027b 	andeq	r0, r0, fp, ror r2
    9838:	00000080 	andeq	r0, r0, r0, lsl #1
    983c:	00000000 	andeq	r0, r0, r0
    9840:	00000296 	muleq	r0, r6, r2
    9844:	00000080 	andeq	r0, r0, r0, lsl #1
    9848:	00000000 	andeq	r0, r0, r0
    984c:	000002b6 			; <UNDEFINED> instruction: 0x000002b6
    9850:	00000080 	andeq	r0, r0, r0, lsl #1
    9854:	00000000 	andeq	r0, r0, r0
    9858:	000002d7 	ldrdeq	r0, [r0], -r7
    985c:	00000080 	andeq	r0, r0, r0, lsl #1
    9860:	00000000 	andeq	r0, r0, r0
    9864:	000002f7 	strdeq	r0, [r0], -r7
    9868:	00000080 	andeq	r0, r0, r0, lsl #1
    986c:	00000000 	andeq	r0, r0, r0
    9870:	0000031c 	andeq	r0, r0, ip, lsl r3
    9874:	00000080 	andeq	r0, r0, r0, lsl #1
    9878:	00000000 	andeq	r0, r0, r0
    987c:	00000346 	andeq	r0, r0, r6, asr #6
    9880:	00000080 	andeq	r0, r0, r0, lsl #1
    9884:	00000000 	andeq	r0, r0, r0
    9888:	0000036a 	andeq	r0, r0, sl, ror #6
    988c:	00000080 	andeq	r0, r0, r0, lsl #1
    9890:	00000000 	andeq	r0, r0, r0
    9894:	00000393 	muleq	r0, r3, r3
    9898:	00000080 	andeq	r0, r0, r0, lsl #1
    989c:	00000000 	andeq	r0, r0, r0
    98a0:	000003c1 	andeq	r0, r0, r1, asr #7
    98a4:	00000080 	andeq	r0, r0, r0, lsl #1
    98a8:	00000000 	andeq	r0, r0, r0
    98ac:	000003e7 	andeq	r0, r0, r7, ror #7
    98b0:	00000080 	andeq	r0, r0, r0, lsl #1
    98b4:	00000000 	andeq	r0, r0, r0
    98b8:	00000407 	andeq	r0, r0, r7, lsl #8
    98bc:	00000080 	andeq	r0, r0, r0, lsl #1
    98c0:	00000000 	andeq	r0, r0, r0
    98c4:	0000042c 	andeq	r0, r0, ip, lsr #8
    98c8:	00000080 	andeq	r0, r0, r0, lsl #1
    98cc:	00000000 	andeq	r0, r0, r0
    98d0:	00000456 	andeq	r0, r0, r6, asr r4
    98d4:	00000080 	andeq	r0, r0, r0, lsl #1
    98d8:	00000000 	andeq	r0, r0, r0
    98dc:	00000485 	andeq	r0, r0, r5, lsl #9
    98e0:	00000080 	andeq	r0, r0, r0, lsl #1
    98e4:	00000000 	andeq	r0, r0, r0
    98e8:	000004ae 	andeq	r0, r0, lr, lsr #9
    98ec:	00000080 	andeq	r0, r0, r0, lsl #1
    98f0:	00000000 	andeq	r0, r0, r0
    98f4:	000004dc 	ldrdeq	r0, [r0], -ip
    98f8:	00000080 	andeq	r0, r0, r0, lsl #1
    98fc:	00000000 	andeq	r0, r0, r0
    9900:	0000050f 	andeq	r0, r0, pc, lsl #10
    9904:	00000080 	andeq	r0, r0, r0, lsl #1
    9908:	00000000 	andeq	r0, r0, r0
    990c:	00000530 	andeq	r0, r0, r0, lsr r5
    9910:	00000080 	andeq	r0, r0, r0, lsl #1
    9914:	00000000 	andeq	r0, r0, r0
    9918:	00000550 	andeq	r0, r0, r0, asr r5
    991c:	00000080 	andeq	r0, r0, r0, lsl #1
    9920:	00000000 	andeq	r0, r0, r0
    9924:	00000575 	andeq	r0, r0, r5, ror r5
    9928:	00000080 	andeq	r0, r0, r0, lsl #1
    992c:	00000000 	andeq	r0, r0, r0
    9930:	0000059f 	muleq	r0, pc, r5	; <UNPREDICTABLE>
    9934:	00000080 	andeq	r0, r0, r0, lsl #1
    9938:	00000000 	andeq	r0, r0, r0
    993c:	000005c3 	andeq	r0, r0, r3, asr #11
    9940:	00000080 	andeq	r0, r0, r0, lsl #1
    9944:	00000000 	andeq	r0, r0, r0
    9948:	000005ec 	andeq	r0, r0, ip, ror #11
    994c:	00000080 	andeq	r0, r0, r0, lsl #1
    9950:	00000000 	andeq	r0, r0, r0
    9954:	0000061a 	andeq	r0, r0, sl, lsl r6
    9958:	00000080 	andeq	r0, r0, r0, lsl #1
    995c:	00000000 	andeq	r0, r0, r0
    9960:	00000640 	andeq	r0, r0, r0, asr #12
    9964:	00000080 	andeq	r0, r0, r0, lsl #1
    9968:	00000000 	andeq	r0, r0, r0
    996c:	00000660 	andeq	r0, r0, r0, ror #12
    9970:	00000080 	andeq	r0, r0, r0, lsl #1
    9974:	00000000 	andeq	r0, r0, r0
    9978:	00000685 	andeq	r0, r0, r5, lsl #13
    997c:	00000080 	andeq	r0, r0, r0, lsl #1
    9980:	00000000 	andeq	r0, r0, r0
    9984:	000006af 	andeq	r0, r0, pc, lsr #13
    9988:	00000080 	andeq	r0, r0, r0, lsl #1
    998c:	00000000 	andeq	r0, r0, r0
    9990:	000006de 	ldrdeq	r0, [r0], -lr
    9994:	00000080 	andeq	r0, r0, r0, lsl #1
    9998:	00000000 	andeq	r0, r0, r0
    999c:	00000707 	andeq	r0, r0, r7, lsl #14
    99a0:	00000080 	andeq	r0, r0, r0, lsl #1
    99a4:	00000000 	andeq	r0, r0, r0
    99a8:	00000735 	andeq	r0, r0, r5, lsr r7
    99ac:	00000080 	andeq	r0, r0, r0, lsl #1
    99b0:	00000000 	andeq	r0, r0, r0
    99b4:	00000768 	andeq	r0, r0, r8, ror #14
    99b8:	00000080 	andeq	r0, r0, r0, lsl #1
    99bc:	00000000 	andeq	r0, r0, r0
    99c0:	000035c3 	andeq	r3, r0, r3, asr #11
    99c4:	000000c2 	andeq	r0, r0, r2, asr #1
    99c8:	00000000 	andeq	r0, r0, r0
    99cc:	00003860 	andeq	r3, r0, r0, ror #16
    99d0:	00000082 	andeq	r0, r0, r2, lsl #1
    99d4:	00002d60 	andeq	r2, r0, r0, ror #26
    99d8:	0000079a 	muleq	r0, sl, r7
    99dc:	00000080 	andeq	r0, r0, r0, lsl #1
    99e0:	00000000 	andeq	r0, r0, r0
    99e4:	000007ac 	andeq	r0, r0, ip, lsr #15
    99e8:	00000080 	andeq	r0, r0, r0, lsl #1
    99ec:	00000000 	andeq	r0, r0, r0
    99f0:	000007c1 	andeq	r0, r0, r1, asr #15
    99f4:	00000080 	andeq	r0, r0, r0, lsl #1
    99f8:	00000000 	andeq	r0, r0, r0
    99fc:	000007d7 	ldrdeq	r0, [r0], -r7
    9a00:	00000080 	andeq	r0, r0, r0, lsl #1
    9a04:	00000000 	andeq	r0, r0, r0
    9a08:	000007ec 	andeq	r0, r0, ip, ror #15
    9a0c:	00000080 	andeq	r0, r0, r0, lsl #1
    9a10:	00000000 	andeq	r0, r0, r0
    9a14:	00000802 	andeq	r0, r0, r2, lsl #16
    9a18:	00000080 	andeq	r0, r0, r0, lsl #1
    9a1c:	00000000 	andeq	r0, r0, r0
    9a20:	00000817 	andeq	r0, r0, r7, lsl r8
    9a24:	00000080 	andeq	r0, r0, r0, lsl #1
    9a28:	00000000 	andeq	r0, r0, r0
    9a2c:	0000082d 	andeq	r0, r0, sp, lsr #16
    9a30:	00000080 	andeq	r0, r0, r0, lsl #1
    9a34:	00000000 	andeq	r0, r0, r0
    9a38:	00000844 	andeq	r0, r0, r4, asr #16
    9a3c:	00000080 	andeq	r0, r0, r0, lsl #1
	...
    9a48:	000000a2 	andeq	r0, r0, r2, lsr #1
    9a4c:	00000000 	andeq	r0, r0, r0
    9a50:	0000387a 	andeq	r3, r0, sl, ror r8
    9a54:	00000024 	andeq	r0, r0, r4, lsr #32
    9a58:	80014b6c 	andhi	r4, r1, ip, ror #22
    9a5c:	00003891 	muleq	r0, r1, r8
    9a60:	000000a0 	andeq	r0, r0, r0, lsr #1
    9a64:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    9a68:	000038a0 	andeq	r3, r0, r0, lsr #17
    9a6c:	000000a0 	andeq	r0, r0, r0, lsr #1
    9a70:	ffffffdc 			; <UNDEFINED> instruction: 0xffffffdc
    9a74:	000038b7 			; <UNDEFINED> instruction: 0x000038b7
    9a78:	000000a0 	andeq	r0, r0, r0, lsr #1
    9a7c:	ffffffd8 			; <UNDEFINED> instruction: 0xffffffd8
    9a80:	00000000 	andeq	r0, r0, r0
    9a84:	00080044 	andeq	r0, r8, r4, asr #32
	...
    9a90:	00090044 	andeq	r0, r9, r4, asr #32
    9a94:	0000001c 	andeq	r0, r0, ip, lsl r0
    9a98:	00000000 	andeq	r0, r0, r0
    9a9c:	000a0044 	andeq	r0, sl, r4, asr #32
    9aa0:	00000024 	andeq	r0, r0, r4, lsr #32
    9aa4:	00000000 	andeq	r0, r0, r0
    9aa8:	000b0044 	andeq	r0, fp, r4, asr #32
    9aac:	0000002c 	andeq	r0, r0, ip, lsr #32
    9ab0:	00000000 	andeq	r0, r0, r0
    9ab4:	000d0044 	andeq	r0, sp, r4, asr #32
    9ab8:	00000034 	andeq	r0, r0, r4, lsr r0
    9abc:	00000000 	andeq	r0, r0, r0
    9ac0:	000e0044 	andeq	r0, lr, r4, asr #32
    9ac4:	00000040 	andeq	r0, r0, r0, asr #32
    9ac8:	00000000 	andeq	r0, r0, r0
    9acc:	000d0044 	andeq	r0, sp, r4, asr #32
    9ad0:	00000060 	andeq	r0, r0, r0, rrx
    9ad4:	00000000 	andeq	r0, r0, r0
    9ad8:	000d0044 	andeq	r0, sp, r4, asr #32
    9adc:	0000006c 	andeq	r0, r0, ip, rrx
    9ae0:	00000000 	andeq	r0, r0, r0
    9ae4:	00100044 	andseq	r0, r0, r4, asr #32
    9ae8:	0000007c 	andeq	r0, r0, ip, ror r0
    9aec:	00000000 	andeq	r0, r0, r0
    9af0:	00110044 	andseq	r0, r1, r4, asr #32
    9af4:	00000080 	andeq	r0, r0, r0, lsl #1
    9af8:	000038c0 	andeq	r3, r0, r0, asr #17
    9afc:	00000080 	andeq	r0, r0, r0, lsl #1
    9b00:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    9b04:	000038dc 	ldrdeq	r3, [r0], -ip
    9b08:	00000080 	andeq	r0, r0, r0, lsl #1
    9b0c:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    9b10:	000038f1 	strdeq	r3, [r0], -r1
    9b14:	00000080 	andeq	r0, r0, r0, lsl #1
    9b18:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9b1c:	00000000 	andeq	r0, r0, r0
    9b20:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    9b2c:	000000e0 	andeq	r0, r0, r0, ror #1
    9b30:	00000090 	muleq	r0, r0, r0
    9b34:	000038f9 	strdeq	r3, [r0], -r9
    9b38:	00000024 	andeq	r0, r0, r4, lsr #32
    9b3c:	80014bfc 	strdhi	r4, [r1], -ip
    9b40:	00003908 	andeq	r3, r0, r8, lsl #18
    9b44:	000000a0 	andeq	r0, r0, r0, lsr #1
    9b48:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    9b4c:	00003917 	andeq	r3, r0, r7, lsl r9
    9b50:	000000a0 	andeq	r0, r0, r0, lsr #1
    9b54:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    9b58:	00000000 	andeq	r0, r0, r0
    9b5c:	00180044 	andseq	r0, r8, r4, asr #32
	...
    9b68:	00190044 	andseq	r0, r9, r4, asr #32
    9b6c:	00000018 	andeq	r0, r0, r8, lsl r0
    9b70:	00000000 	andeq	r0, r0, r0
    9b74:	001b0044 	andseq	r0, fp, r4, asr #32
    9b78:	00000020 	andeq	r0, r0, r0, lsr #32
    9b7c:	00000000 	andeq	r0, r0, r0
    9b80:	001c0044 	andseq	r0, ip, r4, asr #32
    9b84:	00000024 	andeq	r0, r0, r4, lsr #32
    9b88:	00000000 	andeq	r0, r0, r0
    9b8c:	001d0044 	andseq	r0, sp, r4, asr #32
    9b90:	00000034 	andeq	r0, r0, r4, lsr r0
    9b94:	00000000 	andeq	r0, r0, r0
    9b98:	001e0044 	andseq	r0, lr, r4, asr #32
    9b9c:	00000040 	andeq	r0, r0, r0, asr #32
    9ba0:	00000000 	andeq	r0, r0, r0
    9ba4:	001b0044 	andseq	r0, fp, r4, asr #32
    9ba8:	0000004c 	andeq	r0, r0, ip, asr #32
    9bac:	00000000 	andeq	r0, r0, r0
    9bb0:	00210044 	eoreq	r0, r1, r4, asr #32
    9bb4:	0000005c 	andeq	r0, r0, ip, asr r0
    9bb8:	00000000 	andeq	r0, r0, r0
    9bbc:	00230044 	eoreq	r0, r3, r4, asr #32
    9bc0:	00000068 	andeq	r0, r0, r8, rrx
    9bc4:	00000000 	andeq	r0, r0, r0
    9bc8:	00240044 	eoreq	r0, r4, r4, asr #32
    9bcc:	0000006c 	andeq	r0, r0, ip, rrx
    9bd0:	0000392d 	andeq	r3, r0, sp, lsr #18
    9bd4:	00000080 	andeq	r0, r0, r0, lsl #1
    9bd8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9bdc:	00000000 	andeq	r0, r0, r0
    9be0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    9bec:	000000e0 	andeq	r0, r0, r0, ror #1
    9bf0:	0000007c 	andeq	r0, r0, ip, ror r0
    9bf4:	0000393b 	andeq	r3, r0, fp, lsr r9
    9bf8:	00000024 	andeq	r0, r0, r4, lsr #32
    9bfc:	80014c78 	andhi	r4, r1, r8, ror ip
    9c00:	00003908 	andeq	r3, r0, r8, lsl #18
    9c04:	000000a0 	andeq	r0, r0, r0, lsr #1
    9c08:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    9c0c:	0000394a 	andeq	r3, r0, sl, asr #18
    9c10:	000000a0 	andeq	r0, r0, r0, lsr #1
    9c14:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    9c18:	000038b7 			; <UNDEFINED> instruction: 0x000038b7
    9c1c:	000000a0 	andeq	r0, r0, r0, lsr #1
    9c20:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    9c24:	00000000 	andeq	r0, r0, r0
    9c28:	002d0044 	eoreq	r0, sp, r4, asr #32
	...
    9c34:	002e0044 	eoreq	r0, lr, r4, asr #32
    9c38:	0000001c 	andeq	r0, r0, ip, lsl r0
    9c3c:	00000000 	andeq	r0, r0, r0
    9c40:	002f0044 	eoreq	r0, pc, r4, asr #32
    9c44:	00000024 	andeq	r0, r0, r4, lsr #32
    9c48:	00000000 	andeq	r0, r0, r0
    9c4c:	00310044 	eorseq	r0, r1, r4, asr #32
    9c50:	0000002c 	andeq	r0, r0, ip, lsr #32
    9c54:	00000000 	andeq	r0, r0, r0
    9c58:	00320044 	eorseq	r0, r2, r4, asr #32
    9c5c:	00000030 	andeq	r0, r0, r0, lsr r0
    9c60:	00000000 	andeq	r0, r0, r0
    9c64:	00330044 	eorseq	r0, r3, r4, asr #32
    9c68:	00000050 	andeq	r0, r0, r0, asr r0
    9c6c:	00000000 	andeq	r0, r0, r0
    9c70:	00310044 	eorseq	r0, r1, r4, asr #32
    9c74:	0000005c 	andeq	r0, r0, ip, asr r0
    9c78:	00000000 	andeq	r0, r0, r0
    9c7c:	00310044 	eorseq	r0, r1, r4, asr #32
    9c80:	0000006c 	andeq	r0, r0, ip, rrx
    9c84:	00000000 	andeq	r0, r0, r0
    9c88:	00350044 	eorseq	r0, r5, r4, asr #32
    9c8c:	00000084 	andeq	r0, r0, r4, lsl #1
    9c90:	00000000 	andeq	r0, r0, r0
    9c94:	00370044 	eorseq	r0, r7, r4, asr #32
    9c98:	00000098 	muleq	r0, r8, r0
    9c9c:	00000000 	andeq	r0, r0, r0
    9ca0:	00380044 	eorseq	r0, r8, r4, asr #32
    9ca4:	000000a4 	andeq	r0, r0, r4, lsr #1
    9ca8:	00000000 	andeq	r0, r0, r0
    9cac:	00390044 	eorseq	r0, r9, r4, asr #32
    9cb0:	000000a8 	andeq	r0, r0, r8, lsr #1
    9cb4:	00003959 	andeq	r3, r0, r9, asr r9
    9cb8:	00000080 	andeq	r0, r0, r0, lsl #1
    9cbc:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    9cc0:	000038f1 	strdeq	r3, [r0], -r1
    9cc4:	00000080 	andeq	r0, r0, r0, lsl #1
    9cc8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9ccc:	00000000 	andeq	r0, r0, r0
    9cd0:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    9cdc:	000000e0 	andeq	r0, r0, r0, ror #1
    9ce0:	000000b8 	strheq	r0, [r0], -r8
    9ce4:	0000396a 	andeq	r3, r0, sl, ror #18
    9ce8:	00000024 	andeq	r0, r0, r4, lsr #32
    9cec:	80014d30 	andhi	r4, r1, r0, lsr sp
    9cf0:	00003978 	andeq	r3, r0, r8, ror r9
    9cf4:	000000a0 	andeq	r0, r0, r0, lsr #1
    9cf8:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9cfc:	00003983 	andeq	r3, r0, r3, lsl #19
    9d00:	000000a0 	andeq	r0, r0, r0, lsr #1
    9d04:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    9d08:	00000000 	andeq	r0, r0, r0
    9d0c:	00420044 	subeq	r0, r2, r4, asr #32
	...
    9d18:	00430044 	subeq	r0, r3, r4, asr #32
    9d1c:	00000018 	andeq	r0, r0, r8, lsl r0
    9d20:	00000000 	andeq	r0, r0, r0
    9d24:	00440044 	subeq	r0, r4, r4, asr #32
    9d28:	0000001c 	andeq	r0, r0, ip, lsl r0
    9d2c:	00000000 	andeq	r0, r0, r0
    9d30:	00450044 	subeq	r0, r5, r4, asr #32
    9d34:	00000028 	andeq	r0, r0, r8, lsr #32
    9d38:	00000000 	andeq	r0, r0, r0
    9d3c:	00430044 	subeq	r0, r3, r4, asr #32
    9d40:	00000034 	andeq	r0, r0, r4, lsr r0
    9d44:	00000000 	andeq	r0, r0, r0
    9d48:	00430044 	subeq	r0, r3, r4, asr #32
    9d4c:	0000004c 	andeq	r0, r0, ip, asr #32
    9d50:	00000000 	andeq	r0, r0, r0
    9d54:	00430044 	subeq	r0, r3, r4, asr #32
    9d58:	0000005c 	andeq	r0, r0, ip, asr r0
    9d5c:	00000000 	andeq	r0, r0, r0
    9d60:	00480044 	subeq	r0, r8, r4, asr #32
    9d64:	0000006c 	andeq	r0, r0, ip, rrx
    9d68:	00000000 	andeq	r0, r0, r0
    9d6c:	00490044 	subeq	r0, r9, r4, asr #32
    9d70:	00000084 	andeq	r0, r0, r4, lsl #1
    9d74:	0000398e 	andeq	r3, r0, lr, lsl #19
    9d78:	00000024 	andeq	r0, r0, r4, lsr #32
    9d7c:	80014dc4 	andhi	r4, r1, r4, asr #27
    9d80:	00003978 	andeq	r3, r0, r8, ror r9
    9d84:	000000a0 	andeq	r0, r0, r0, lsr #1
    9d88:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    9d8c:	00003983 	andeq	r3, r0, r3, lsl #19
    9d90:	000000a0 	andeq	r0, r0, r0, lsr #1
    9d94:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    9d98:	000038b7 			; <UNDEFINED> instruction: 0x000038b7
    9d9c:	000000a0 	andeq	r0, r0, r0, lsr #1
    9da0:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    9da4:	00000000 	andeq	r0, r0, r0
    9da8:	00510044 	subseq	r0, r1, r4, asr #32
	...
    9db4:	00520044 	subseq	r0, r2, r4, asr #32
    9db8:	0000001c 	andeq	r0, r0, ip, lsl r0
    9dbc:	00000000 	andeq	r0, r0, r0
    9dc0:	00540044 	subseq	r0, r4, r4, asr #32
    9dc4:	00000024 	andeq	r0, r0, r4, lsr #32
    9dc8:	00000000 	andeq	r0, r0, r0
    9dcc:	00550044 	subseq	r0, r5, r4, asr #32
    9dd0:	00000030 	andeq	r0, r0, r0, lsr r0
    9dd4:	00000000 	andeq	r0, r0, r0
    9dd8:	00570044 	subseq	r0, r7, r4, asr #32
    9ddc:	00000038 	andeq	r0, r0, r8, lsr r0
    9de0:	00000000 	andeq	r0, r0, r0
    9de4:	00580044 	subseq	r0, r8, r4, asr #32
    9de8:	0000003c 	andeq	r0, r0, ip, lsr r0
    9dec:	00000000 	andeq	r0, r0, r0
    9df0:	00590044 	subseq	r0, r9, r4, asr #32
    9df4:	00000048 	andeq	r0, r0, r8, asr #32
    9df8:	00000000 	andeq	r0, r0, r0
    9dfc:	005a0044 	subseq	r0, sl, r4, asr #32
    9e00:	00000054 	andeq	r0, r0, r4, asr r0
    9e04:	00000000 	andeq	r0, r0, r0
    9e08:	00570044 	subseq	r0, r7, r4, asr #32
    9e0c:	00000060 	andeq	r0, r0, r0, rrx
    9e10:	00000000 	andeq	r0, r0, r0
    9e14:	00570044 	subseq	r0, r7, r4, asr #32
    9e18:	00000078 	andeq	r0, r0, r8, ror r0
    9e1c:	00000000 	andeq	r0, r0, r0
    9e20:	00570044 	subseq	r0, r7, r4, asr #32
    9e24:	00000088 	andeq	r0, r0, r8, lsl #1
    9e28:	00000000 	andeq	r0, r0, r0
    9e2c:	00570044 	subseq	r0, r7, r4, asr #32
    9e30:	00000098 	muleq	r0, r8, r0
    9e34:	00000000 	andeq	r0, r0, r0
    9e38:	005d0044 	subseq	r0, sp, r4, asr #32
    9e3c:	000000ac 	andeq	r0, r0, ip, lsr #1
    9e40:	00000000 	andeq	r0, r0, r0
    9e44:	005e0044 	subseq	r0, lr, r4, asr #32
    9e48:	000000c4 	andeq	r0, r0, r4, asr #1
    9e4c:	000038f1 	strdeq	r3, [r0], -r1
    9e50:	00000080 	andeq	r0, r0, r0, lsl #1
    9e54:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9e58:	00000000 	andeq	r0, r0, r0
    9e5c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    9e68:	000000e0 	andeq	r0, r0, r0, ror #1
    9e6c:	000000d4 	ldrdeq	r0, [r0], -r4
    9e70:	0000399d 	muleq	r0, sp, r9
    9e74:	00000024 	andeq	r0, r0, r4, lsr #32
    9e78:	80014e98 	mulhi	r1, r8, lr
    9e7c:	000039ac 	andeq	r3, r0, ip, lsr #19
    9e80:	000000a0 	andeq	r0, r0, r0, lsr #1
    9e84:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    9e88:	000039b8 			; <UNDEFINED> instruction: 0x000039b8
    9e8c:	000000a0 	andeq	r0, r0, r0, lsr #1
    9e90:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    9e94:	00000000 	andeq	r0, r0, r0
    9e98:	00650044 	rsbeq	r0, r5, r4, asr #32
	...
    9ea4:	00660044 	rsbeq	r0, r6, r4, asr #32
    9ea8:	00000018 	andeq	r0, r0, r8, lsl r0
    9eac:	00000000 	andeq	r0, r0, r0
    9eb0:	00670044 	rsbeq	r0, r7, r4, asr #32
    9eb4:	0000001c 	andeq	r0, r0, ip, lsl r0
    9eb8:	00000000 	andeq	r0, r0, r0
    9ebc:	00660044 	rsbeq	r0, r6, r4, asr #32
    9ec0:	00000028 	andeq	r0, r0, r8, lsr #32
    9ec4:	00000000 	andeq	r0, r0, r0
    9ec8:	00660044 	rsbeq	r0, r6, r4, asr #32
    9ecc:	00000038 	andeq	r0, r0, r8, lsr r0
    9ed0:	00000000 	andeq	r0, r0, r0
    9ed4:	00690044 	rsbeq	r0, r9, r4, asr #32
    9ed8:	00000050 	andeq	r0, r0, r0, asr r0
    9edc:	00000000 	andeq	r0, r0, r0
    9ee0:	006a0044 	rsbeq	r0, sl, r4, asr #32
    9ee4:	00000068 	andeq	r0, r0, r8, rrx
    9ee8:	00000000 	andeq	r0, r0, r0
    9eec:	006c0044 	rsbeq	r0, ip, r4, asr #32
    9ef0:	00000070 	andeq	r0, r0, r0, ror r0
    9ef4:	00000000 	andeq	r0, r0, r0
    9ef8:	006d0044 	rsbeq	r0, sp, r4, asr #32
    9efc:	00000074 	andeq	r0, r0, r4, ror r0
    9f00:	000039c9 	andeq	r3, r0, r9, asr #19
    9f04:	00000024 	andeq	r0, r0, r4, lsr #32
    9f08:	80014f1c 	andhi	r4, r1, ip, lsl pc
    9f0c:	000039d8 	ldrdeq	r3, [r0], -r8
    9f10:	000000a0 	andeq	r0, r0, r0, lsr #1
    9f14:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    9f18:	000039e4 	andeq	r3, r0, r4, ror #19
    9f1c:	000000a0 	andeq	r0, r0, r0, lsr #1
    9f20:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    9f24:	00000000 	andeq	r0, r0, r0
    9f28:	00780044 	rsbseq	r0, r8, r4, asr #32
	...
    9f34:	007a0044 	rsbseq	r0, sl, r4, asr #32
    9f38:	00000018 	andeq	r0, r0, r8, lsl r0
    9f3c:	00000000 	andeq	r0, r0, r0
    9f40:	007c0044 	rsbseq	r0, ip, r4, asr #32
    9f44:	00000020 	andeq	r0, r0, r0, lsr #32
    9f48:	00000000 	andeq	r0, r0, r0
    9f4c:	007d0044 	rsbseq	r0, sp, r4, asr #32
    9f50:	0000002c 	andeq	r0, r0, ip, lsr #32
    9f54:	00000000 	andeq	r0, r0, r0
    9f58:	007f0044 	rsbseq	r0, pc, r4, asr #32
    9f5c:	0000003c 	andeq	r0, r0, ip, lsr r0
    9f60:	00000000 	andeq	r0, r0, r0
    9f64:	00820044 	addeq	r0, r2, r4, asr #32
    9f68:	0000004c 	andeq	r0, r0, ip, asr #32
    9f6c:	00000000 	andeq	r0, r0, r0
    9f70:	00830044 	addeq	r0, r3, r4, asr #32
    9f74:	00000050 	andeq	r0, r0, r0, asr r0
    9f78:	00000000 	andeq	r0, r0, r0
    9f7c:	00820044 	addeq	r0, r2, r4, asr #32
    9f80:	0000005c 	andeq	r0, r0, ip, asr r0
    9f84:	00000000 	andeq	r0, r0, r0
    9f88:	00820044 	addeq	r0, r2, r4, asr #32
    9f8c:	0000006c 	andeq	r0, r0, ip, rrx
    9f90:	00000000 	andeq	r0, r0, r0
    9f94:	00860044 	addeq	r0, r6, r4, asr #32
    9f98:	0000008c 	andeq	r0, r0, ip, lsl #1
    9f9c:	00000000 	andeq	r0, r0, r0
    9fa0:	00870044 	addeq	r0, r7, r4, asr #32
    9fa4:	0000009c 	muleq	r0, ip, r0
    9fa8:	00000000 	andeq	r0, r0, r0
    9fac:	00880044 	addeq	r0, r8, r4, asr #32
    9fb0:	000000ac 	andeq	r0, r0, ip, lsr #1
    9fb4:	00000000 	andeq	r0, r0, r0
    9fb8:	008c0044 	addeq	r0, ip, r4, asr #32
    9fbc:	000000b4 	strheq	r0, [r0], -r4
    9fc0:	00000000 	andeq	r0, r0, r0
    9fc4:	008d0044 	addeq	r0, sp, r4, asr #32
    9fc8:	000000b8 	strheq	r0, [r0], -r8
    9fcc:	00000000 	andeq	r0, r0, r0
    9fd0:	008c0044 	addeq	r0, ip, r4, asr #32
    9fd4:	000000d4 	ldrdeq	r0, [r0], -r4
    9fd8:	00000000 	andeq	r0, r0, r0
    9fdc:	008c0044 	addeq	r0, ip, r4, asr #32
    9fe0:	000000ec 	andeq	r0, r0, ip, ror #1
    9fe4:	00000000 	andeq	r0, r0, r0
    9fe8:	00900044 	addseq	r0, r0, r4, asr #32
    9fec:	00000114 	andeq	r0, r0, r4, lsl r1
    9ff0:	00000000 	andeq	r0, r0, r0
    9ff4:	00910044 	addseq	r0, r1, r4, asr #32
    9ff8:	0000012c 	andeq	r0, r0, ip, lsr #2
    9ffc:	00000000 	andeq	r0, r0, r0
    a000:	00920044 	addseq	r0, r2, r4, asr #32
    a004:	00000140 	andeq	r0, r0, r0, asr #2
    a008:	00000000 	andeq	r0, r0, r0
    a00c:	00950044 	addseq	r0, r5, r4, asr #32
    a010:	0000015c 	andeq	r0, r0, ip, asr r1
    a014:	00000000 	andeq	r0, r0, r0
    a018:	00960044 	addseq	r0, r6, r4, asr #32
    a01c:	00000160 	andeq	r0, r0, r0, ror #2
    a020:	000039f7 	strdeq	r3, [r0], -r7
    a024:	00000026 	andeq	r0, r0, r6, lsr #32
    a028:	80244b14 	eorhi	r4, r4, r4, lsl fp
    a02c:	00003a04 	andeq	r3, r0, r4, lsl #20
    a030:	00000080 	andeq	r0, r0, r0, lsl #1
    a034:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    a038:	00000000 	andeq	r0, r0, r0
    a03c:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    a048:	000000e0 	andeq	r0, r0, r0, ror #1
    a04c:	0000019c 	muleq	r0, ip, r1
    a050:	00003a11 	andeq	r3, r0, r1, lsl sl
    a054:	00000024 	andeq	r0, r0, r4, lsr #32
    a058:	800150b8 	strhhi	r5, [r1], -r8
    a05c:	00003891 	muleq	r0, r1, r8
    a060:	000000a0 	andeq	r0, r0, r0, lsr #1
    a064:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    a068:	00000eed 	andeq	r0, r0, sp, ror #29
    a06c:	000000a0 	andeq	r0, r0, r0, lsr #1
    a070:	ffffffe4 			; <UNDEFINED> instruction: 0xffffffe4
    a074:	00003a20 	andeq	r3, r0, r0, lsr #20
    a078:	000000a0 	andeq	r0, r0, r0, lsr #1
    a07c:	ffffffe0 			; <UNDEFINED> instruction: 0xffffffe0
    a080:	00000000 	andeq	r0, r0, r0
    a084:	009a0044 	addseq	r0, sl, r4, asr #32
	...
    a090:	009b0044 	addseq	r0, fp, r4, asr #32
    a094:	0000001c 	andeq	r0, r0, ip, lsl r0
    a098:	00000000 	andeq	r0, r0, r0
    a09c:	009d0044 	addseq	r0, sp, r4, asr #32
    a0a0:	00000024 	andeq	r0, r0, r4, lsr #32
    a0a4:	00000000 	andeq	r0, r0, r0
    a0a8:	009e0044 	addseq	r0, lr, r4, asr #32
    a0ac:	0000002c 	andeq	r0, r0, ip, lsr #32
    a0b0:	00000000 	andeq	r0, r0, r0
    a0b4:	009f0044 	addseq	r0, pc, r4, asr #32
    a0b8:	00000038 	andeq	r0, r0, r8, lsr r0
    a0bc:	00000000 	andeq	r0, r0, r0
    a0c0:	009e0044 	addseq	r0, lr, r4, asr #32
    a0c4:	00000050 	andeq	r0, r0, r0, asr r0
    a0c8:	00000000 	andeq	r0, r0, r0
    a0cc:	009e0044 	addseq	r0, lr, r4, asr #32
    a0d0:	0000005c 	andeq	r0, r0, ip, asr r0
    a0d4:	00000000 	andeq	r0, r0, r0
    a0d8:	00a10044 	adceq	r0, r1, r4, asr #32
    a0dc:	0000006c 	andeq	r0, r0, ip, rrx
    a0e0:	00000000 	andeq	r0, r0, r0
    a0e4:	00a20044 	adceq	r0, r2, r4, asr #32
    a0e8:	00000070 	andeq	r0, r0, r0, ror r0
    a0ec:	00003a2b 	andeq	r3, r0, fp, lsr #20
    a0f0:	00000080 	andeq	r0, r0, r0, lsl #1
    a0f4:	ffffffec 			; <UNDEFINED> instruction: 0xffffffec
    a0f8:	000038f1 	strdeq	r3, [r0], -r1
    a0fc:	00000080 	andeq	r0, r0, r0, lsl #1
    a100:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    a104:	00000000 	andeq	r0, r0, r0
    a108:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    a114:	000000e0 	andeq	r0, r0, r0, ror #1
    a118:	00000080 	andeq	r0, r0, r0, lsl #1
    a11c:	00003a40 	andeq	r3, r0, r0, asr #20
    a120:	00000024 	andeq	r0, r0, r4, lsr #32
    a124:	80015138 	andhi	r5, r1, r8, lsr r1
    a128:	000039ac 	andeq	r3, r0, ip, lsr #19
    a12c:	000000a0 	andeq	r0, r0, r0, lsr #1
    a130:	ffffffe8 			; <UNDEFINED> instruction: 0xffffffe8
    a134:	00000000 	andeq	r0, r0, r0
    a138:	00a60044 	adceq	r0, r6, r4, asr #32
	...
    a144:	00a70044 	adceq	r0, r7, r4, asr #32
    a148:	00000014 	andeq	r0, r0, r4, lsl r0
    a14c:	00000000 	andeq	r0, r0, r0
    a150:	00a80044 	adceq	r0, r8, r4, asr #32
    a154:	0000001c 	andeq	r0, r0, ip, lsl r0
    a158:	00000000 	andeq	r0, r0, r0
    a15c:	00a90044 	adceq	r0, r9, r4, asr #32
    a160:	00000020 	andeq	r0, r0, r0, lsr #32
    a164:	00000000 	andeq	r0, r0, r0
    a168:	00aa0044 	adceq	r0, sl, r4, asr #32
    a16c:	0000002c 	andeq	r0, r0, ip, lsr #32
    a170:	00000000 	andeq	r0, r0, r0
    a174:	00a80044 	adceq	r0, r8, r4, asr #32
    a178:	00000038 	andeq	r0, r0, r8, lsr r0
    a17c:	00000000 	andeq	r0, r0, r0
    a180:	00ad0044 	adceq	r0, sp, r4, asr #32
    a184:	00000048 	andeq	r0, r0, r8, asr #32
    a188:	00000000 	andeq	r0, r0, r0
    a18c:	00ae0044 	adceq	r0, lr, r4, asr #32
    a190:	0000004c 	andeq	r0, r0, ip, asr #32
    a194:	00003a4e 	andeq	r3, r0, lr, asr #20
    a198:	00000080 	andeq	r0, r0, r0, lsl #1
    a19c:	fffffff0 			; <UNDEFINED> instruction: 0xfffffff0
    a1a0:	00000000 	andeq	r0, r0, r0
    a1a4:	000000c0 	andeq	r0, r0, r0, asr #1
	...
    a1b0:	000000e0 	andeq	r0, r0, r0, ror #1
    a1b4:	0000005c 	andeq	r0, r0, ip, asr r0
    a1b8:	00000000 	andeq	r0, r0, r0
    a1bc:	00000064 	andeq	r0, r0, r4, rrx
    a1c0:	80015194 	mulhi	r1, r4, r1

Disassembly of section .stabstr:

00000000 <.stabstr>:
       0:	71726900 	cmnvc	r2, r0, lsl #18
       4:	2f00632e 	svccs	0x0000632e
       8:	72657355 	rsbvc	r7, r5, #1409286145	; 0x54000001
       c:	696d2f73 	stmdbvs	sp!, {r0, r1, r4, r5, r6, r8, r9, sl, fp, sp}^
      10:	772f6173 			; <UNDEFINED> instruction: 0x772f6173
      14:	2f6b726f 	svccs	0x006b726f
      18:	6b6f7745 	blvs	1bddd34 <start_address+0x1bcdd34>
      1c:	6b2f534f 	blvs	bd4d60 <start_address+0xbc4d60>
      20:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
      24:	61002f6c 	tstvs	r0, ip, ror #30
      28:	2f686372 	svccs	0x00686372
      2c:	70736172 	rsbsvc	r6, r3, r2, ror r1
      30:	732f3269 			; <UNDEFINED> instruction: 0x732f3269
      34:	692f6372 	stmdbvs	pc!, {r1, r4, r5, r6, r8, r9, sp, lr}	; <UNPREDICTABLE>
      38:	632e7172 			; <UNDEFINED> instruction: 0x632e7172
      3c:	63636700 	cmnvs	r3, #0, 14
      40:	6f635f32 	svcvs	0x00635f32
      44:	6c69706d 	stclvs	0, cr7, [r9], #-436	; 0xfffffe4c
      48:	002e6465 	eoreq	r6, lr, r5, ror #8
      4c:	3a746e69 	bcc	1d1b9f8 <start_address+0x1d0b9f8>
      50:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
      54:	723d2931 	eorsvc	r2, sp, #802816	; 0xc4000
      58:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
      5c:	322d3b29 	eorcc	r3, sp, #41984	; 0xa400
      60:	34373431 	ldrtcc	r3, [r7], #-1073	; 0x431
      64:	34363338 	ldrtcc	r3, [r6], #-824	; 0x338
      68:	31323b38 	teqcc	r2, r8, lsr fp
      6c:	38343734 	ldmdacc	r4!, {r2, r4, r5, r8, r9, sl, ip, sp}
      70:	37343633 			; <UNDEFINED> instruction: 0x37343633
      74:	6863003b 	stmdavs	r3!, {r0, r1, r3, r4, r5}^
      78:	743a7261 	ldrtvc	r7, [sl], #-609	; 0x261
      7c:	322c3028 	eorcc	r3, ip, #40	; 0x28
      80:	73403d29 	movtvc	r3, #3369	; 0xd29
      84:	28723b38 	ldmdacs	r2!, {r3, r4, r5, r8, r9, fp, ip, sp}^
      88:	29322c30 	ldmdbcs	r2!, {r4, r5, sl, fp, sp}
      8c:	323b303b 	eorscc	r3, fp, #59	; 0x3b
      90:	003b3535 	eorseq	r3, fp, r5, lsr r5
      94:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
      98:	746e6920 	strbtvc	r6, [lr], #-2336	; 0x920
      9c:	3028743a 	eorcc	r7, r8, sl, lsr r4
      a0:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
      a4:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
      a8:	2d3b2933 	ldccs	9, cr2, [fp, #-204]!	; 0xffffff34
      ac:	37343132 			; <UNDEFINED> instruction: 0x37343132
      b0:	36333834 			; <UNDEFINED> instruction: 0x36333834
      b4:	323b3834 	eorscc	r3, fp, #52, 16	; 0x340000
      b8:	34373431 	ldrtcc	r3, [r7], #-1073	; 0x431
      bc:	34363338 	ldrtcc	r3, [r6], #-824	; 0x338
      c0:	75003b37 	strvc	r3, [r0, #-2871]	; 0xb37
      c4:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
      c8:	2064656e 	rsbcs	r6, r4, lr, ror #10
      cc:	3a746e69 	bcc	1d1ba78 <start_address+0x1d0ba78>
      d0:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
      d4:	723d2934 	eorsvc	r2, sp, #52, 18	; 0xd0000
      d8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
      dc:	3b303b29 	blcc	c0ed88 <start_address+0xbfed88>
      e0:	37373330 			; <UNDEFINED> instruction: 0x37373330
      e4:	37373737 			; <UNDEFINED> instruction: 0x37373737
      e8:	37373737 			; <UNDEFINED> instruction: 0x37373737
      ec:	6f6c003b 	svcvs	0x006c003b
      f0:	7520676e 	strvc	r6, [r0, #-1902]!	; 0x76e
      f4:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
      f8:	2064656e 	rsbcs	r6, r4, lr, ror #10
      fc:	3a746e69 	bcc	1d1baa8 <start_address+0x1d0baa8>
     100:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     104:	723d2935 	eorsvc	r2, sp, #868352	; 0xd4000
     108:	352c3028 	strcc	r3, [ip, #-40]!	; 0x28
     10c:	3b303b29 	blcc	c0edb8 <start_address+0xbfedb8>
     110:	37373330 			; <UNDEFINED> instruction: 0x37373330
     114:	37373737 			; <UNDEFINED> instruction: 0x37373737
     118:	37373737 			; <UNDEFINED> instruction: 0x37373737
     11c:	6f6c003b 	svcvs	0x006c003b
     120:	6c20676e 	stcvs	7, cr6, [r0], #-440	; 0xfffffe48
     124:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     128:	3a746e69 	bcc	1d1bad4 <start_address+0x1d0bad4>
     12c:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     130:	403d2936 	eorsmi	r2, sp, r6, lsr r9
     134:	3b343673 	blcc	d0db08 <start_address+0xcfdb08>
     138:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     13c:	303b2936 	eorscc	r2, fp, r6, lsr r9
     140:	30303031 	eorscc	r3, r0, r1, lsr r0
     144:	30303030 	eorscc	r3, r0, r0, lsr r0
     148:	30303030 	eorscc	r3, r0, r0, lsr r0
     14c:	30303030 	eorscc	r3, r0, r0, lsr r0
     150:	30303030 	eorscc	r3, r0, r0, lsr r0
     154:	303b3030 	eorscc	r3, fp, r0, lsr r0
     158:	37373737 			; <UNDEFINED> instruction: 0x37373737
     15c:	37373737 			; <UNDEFINED> instruction: 0x37373737
     160:	37373737 			; <UNDEFINED> instruction: 0x37373737
     164:	37373737 			; <UNDEFINED> instruction: 0x37373737
     168:	37373737 			; <UNDEFINED> instruction: 0x37373737
     16c:	6c003b37 	stcvs	11, cr3, [r0], {55}	; 0x37
     170:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     174:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     178:	736e7520 	cmnvc	lr, #32, 10	; 0x8000000
     17c:	656e6769 	strbvs	r6, [lr, #-1897]!	; 0x769
     180:	6e692064 	cdpvs	0, 6, cr2, cr9, cr4, {3}
     184:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     188:	29372c30 	ldmdbcs	r7!, {r4, r5, sl, fp, sp}
     18c:	3673403d 			; <UNDEFINED> instruction: 0x3673403d
     190:	28723b34 	ldmdacs	r2!, {r2, r4, r5, r8, r9, fp, ip, sp}^
     194:	29372c30 	ldmdbcs	r7!, {r4, r5, sl, fp, sp}
     198:	303b303b 	eorscc	r3, fp, fp, lsr r0
     19c:	37373731 			; <UNDEFINED> instruction: 0x37373731
     1a0:	37373737 			; <UNDEFINED> instruction: 0x37373737
     1a4:	37373737 			; <UNDEFINED> instruction: 0x37373737
     1a8:	37373737 			; <UNDEFINED> instruction: 0x37373737
     1ac:	37373737 			; <UNDEFINED> instruction: 0x37373737
     1b0:	003b3737 	eorseq	r3, fp, r7, lsr r7
     1b4:	726f6873 	rsbvc	r6, pc, #7536640	; 0x730000
     1b8:	6e692074 	mcrvs	0, 3, r2, cr9, cr4, {3}
     1bc:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     1c0:	29382c30 	ldmdbcs	r8!, {r4, r5, sl, fp, sp}
     1c4:	3173403d 	cmncc	r3, sp, lsr r0
     1c8:	28723b36 	ldmdacs	r2!, {r1, r2, r4, r5, r8, r9, fp, ip, sp}^
     1cc:	29382c30 	ldmdbcs	r8!, {r4, r5, sl, fp, sp}
     1d0:	32332d3b 	eorscc	r2, r3, #3776	; 0xec0
     1d4:	3b383637 	blcc	e0dab8 <start_address+0xdfdab8>
     1d8:	36373233 			; <UNDEFINED> instruction: 0x36373233
     1dc:	73003b37 	movwvc	r3, #2871	; 0xb37
     1e0:	74726f68 	ldrbtvc	r6, [r2], #-3944	; 0xf68
     1e4:	736e7520 	cmnvc	lr, #32, 10	; 0x8000000
     1e8:	656e6769 	strbvs	r6, [lr, #-1897]!	; 0x769
     1ec:	6e692064 	cdpvs	0, 6, cr2, cr9, cr4, {3}
     1f0:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     1f4:	29392c30 	ldmdbcs	r9!, {r4, r5, sl, fp, sp}
     1f8:	3173403d 	cmncc	r3, sp, lsr r0
     1fc:	28723b36 	ldmdacs	r2!, {r1, r2, r4, r5, r8, r9, fp, ip, sp}^
     200:	29392c30 	ldmdbcs	r9!, {r4, r5, sl, fp, sp}
     204:	363b303b 			; <UNDEFINED> instruction: 0x363b303b
     208:	35333535 	ldrcc	r3, [r3, #-1333]!	; 0x535
     20c:	6973003b 	ldmdbvs	r3!, {r0, r1, r3, r4, r5}^
     210:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
     214:	61686320 	cmnvs	r8, r0, lsr #6
     218:	28743a72 	ldmdacs	r4!, {r1, r4, r5, r6, r9, fp, ip, sp}^
     21c:	30312c30 	eorscc	r2, r1, r0, lsr ip
     220:	73403d29 	movtvc	r3, #3369	; 0xd29
     224:	28723b38 	ldmdacs	r2!, {r3, r4, r5, r8, r9, fp, ip, sp}^
     228:	30312c30 	eorscc	r2, r1, r0, lsr ip
     22c:	312d3b29 			; <UNDEFINED> instruction: 0x312d3b29
     230:	313b3832 	teqcc	fp, r2, lsr r8
     234:	003b3732 	eorseq	r3, fp, r2, lsr r7
     238:	69736e75 	ldmdbvs	r3!, {r0, r2, r4, r5, r6, r9, sl, fp, sp, lr}^
     23c:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
     240:	61686320 	cmnvs	r8, r0, lsr #6
     244:	28743a72 	ldmdacs	r4!, {r1, r4, r5, r6, r9, fp, ip, sp}^
     248:	31312c30 	teqcc	r1, r0, lsr ip
     24c:	73403d29 	movtvc	r3, #3369	; 0xd29
     250:	28723b38 	ldmdacs	r2!, {r3, r4, r5, r8, r9, fp, ip, sp}^
     254:	31312c30 	teqcc	r1, r0, lsr ip
     258:	3b303b29 	blcc	c0ef04 <start_address+0xbfef04>
     25c:	3b353532 	blcc	d4d72c <start_address+0xd3d72c>
     260:	6f6c6600 	svcvs	0x006c6600
     264:	743a7461 	ldrtvc	r7, [sl], #-1121	; 0x461
     268:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     26c:	723d2932 	eorsvc	r2, sp, #819200	; 0xc8000
     270:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     274:	3b343b29 	blcc	d0ef20 <start_address+0xcfef20>
     278:	64003b30 	strvs	r3, [r0], #-2864	; 0xb30
     27c:	6c62756f 	cfstr64vs	mvdx7, [r2], #-444	; 0xfffffe44
     280:	28743a65 	ldmdacs	r4!, {r0, r2, r5, r6, r9, fp, ip, sp}^
     284:	33312c30 	teqcc	r1, #48, 24	; 0x3000
     288:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     28c:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     290:	303b383b 	eorscc	r3, fp, fp, lsr r8
     294:	6f6c003b 	svcvs	0x006c003b
     298:	6420676e 	strtvs	r6, [r0], #-1902	; 0x76e
     29c:	6c62756f 	cfstr64vs	mvdx7, [r2], #-444	; 0xfffffe44
     2a0:	28743a65 	ldmdacs	r4!, {r0, r2, r5, r6, r9, fp, ip, sp}^
     2a4:	34312c30 	ldrtcc	r2, [r1], #-3120	; 0xc30
     2a8:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     2ac:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     2b0:	303b383b 	eorscc	r3, fp, fp, lsr r8
     2b4:	6873003b 	ldmdavs	r3!, {r0, r1, r3, r4, r5}^
     2b8:	2074726f 	rsbscs	r7, r4, pc, ror #4
     2bc:	6172465f 	cmnvs	r2, pc, asr r6
     2c0:	743a7463 	ldrtvc	r7, [sl], #-1123	; 0x463
     2c4:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     2c8:	723d2935 	eorsvc	r2, sp, #868352	; 0xd4000
     2cc:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     2d0:	3b313b29 	blcc	c4ef7c <start_address+0xc3ef7c>
     2d4:	6c003b30 	stcvs	11, cr3, [r0], {48}	; 0x30
     2d8:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     2dc:	6172465f 	cmnvs	r2, pc, asr r6
     2e0:	743a7463 	ldrtvc	r7, [sl], #-1123	; 0x463
     2e4:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     2e8:	723d2936 	eorsvc	r2, sp, #884736	; 0xd8000
     2ec:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     2f0:	3b343b29 	blcc	d0ef9c <start_address+0xcfef9c>
     2f4:	6c003b30 	stcvs	11, cr3, [r0], {48}	; 0x30
     2f8:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     2fc:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     300:	72465f20 	subvc	r5, r6, #32, 30	; 0x80
     304:	3a746361 	bcc	1d19090 <start_address+0x1d09090>
     308:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     30c:	3d293731 	stccc	7, cr3, [r9, #-196]!	; 0xffffff3c
     310:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     314:	383b2931 	ldmdacc	fp!, {r0, r4, r5, r8, fp, sp}
     318:	003b303b 	eorseq	r3, fp, fp, lsr r0
     31c:	69736e75 	ldmdbvs	r3!, {r0, r2, r4, r5, r6, r9, sl, fp, sp, lr}^
     320:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
     324:	6f687320 	svcvs	0x00687320
     328:	5f207472 	svcpl	0x00207472
     32c:	63617246 	cmnvs	r1, #1610612740	; 0x60000004
     330:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     334:	38312c30 	ldmdacc	r1!, {r4, r5, sl, fp, sp}
     338:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     33c:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     340:	303b313b 	eorscc	r3, fp, fp, lsr r1
     344:	6e75003b 	mrcvs	0, 3, r0, cr5, cr11, {1}
     348:	6e676973 	mcrvs	9, 3, r6, cr7, cr3, {3}
     34c:	5f206465 	svcpl	0x00206465
     350:	63617246 	cmnvs	r1, #1610612740	; 0x60000004
     354:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     358:	39312c30 	ldmdbcc	r1!, {r4, r5, sl, fp, sp}
     35c:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     360:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     364:	303b323b 	eorscc	r3, fp, fp, lsr r2
     368:	6e75003b 	mrcvs	0, 3, r0, cr5, cr11, {1}
     36c:	6e676973 	mcrvs	9, 3, r6, cr7, cr3, {3}
     370:	6c206465 	cfstrsvs	mvf6, [r0], #-404	; 0xfffffe6c
     374:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     378:	6172465f 	cmnvs	r2, pc, asr r6
     37c:	743a7463 	ldrtvc	r7, [sl], #-1123	; 0x463
     380:	322c3028 	eorcc	r3, ip, #40	; 0x28
     384:	723d2930 	eorsvc	r2, sp, #48, 18	; 0xc0000
     388:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     38c:	3b343b29 	blcc	d0f038 <start_address+0xcff038>
     390:	75003b30 	strvc	r3, [r0, #-2864]	; 0xb30
     394:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
     398:	2064656e 	rsbcs	r6, r4, lr, ror #10
     39c:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     3a0:	6e6f6c20 	cdpvs	12, 6, cr6, cr15, cr0, {1}
     3a4:	465f2067 	ldrbmi	r2, [pc], -r7, rrx
     3a8:	74636172 	strbtvc	r6, [r3], #-370	; 0x172
     3ac:	3028743a 	eorcc	r7, r8, sl, lsr r4
     3b0:	2931322c 	ldmdbcs	r1!, {r2, r3, r5, r9, ip, sp}
     3b4:	3028723d 	eorcc	r7, r8, sp, lsr r2
     3b8:	3b29312c 	blcc	a4c870 <start_address+0xa3c870>
     3bc:	3b303b38 	blcc	c0f0a4 <start_address+0xbff0a4>
     3c0:	61535f00 	cmpvs	r3, r0, lsl #30
     3c4:	68732074 	ldmdavs	r3!, {r2, r4, r5, r6, sp}^
     3c8:	2074726f 	rsbscs	r7, r4, pc, ror #4
     3cc:	6172465f 	cmnvs	r2, pc, asr r6
     3d0:	743a7463 	ldrtvc	r7, [sl], #-1123	; 0x463
     3d4:	322c3028 	eorcc	r3, ip, #40	; 0x28
     3d8:	723d2932 	eorsvc	r2, sp, #819200	; 0xc8000
     3dc:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     3e0:	3b313b29 	blcc	c4f08c <start_address+0xc3f08c>
     3e4:	5f003b30 	svcpl	0x00003b30
     3e8:	20746153 	rsbscs	r6, r4, r3, asr r1
     3ec:	6172465f 	cmnvs	r2, pc, asr r6
     3f0:	743a7463 	ldrtvc	r7, [sl], #-1123	; 0x463
     3f4:	322c3028 	eorcc	r3, ip, #40	; 0x28
     3f8:	723d2933 	eorsvc	r2, sp, #835584	; 0xcc000
     3fc:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     400:	3b323b29 	blcc	c8f0ac <start_address+0xc7f0ac>
     404:	5f003b30 	svcpl	0x00003b30
     408:	20746153 	rsbscs	r6, r4, r3, asr r1
     40c:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     410:	72465f20 	subvc	r5, r6, #32, 30	; 0x80
     414:	3a746361 	bcc	1d191a0 <start_address+0x1d091a0>
     418:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     41c:	3d293432 	cfstrscc	mvf3, [r9, #-200]!	; 0xffffff38
     420:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     424:	343b2931 	ldrtcc	r2, [fp], #-2353	; 0x931
     428:	003b303b 	eorseq	r3, fp, fp, lsr r0
     42c:	7461535f 	strbtvc	r5, [r1], #-863	; 0x35f
     430:	6e6f6c20 	cdpvs	12, 6, cr6, cr15, cr0, {1}
     434:	6f6c2067 	svcvs	0x006c2067
     438:	5f20676e 	svcpl	0x0020676e
     43c:	63617246 	cmnvs	r1, #1610612740	; 0x60000004
     440:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     444:	35322c30 	ldrcc	r2, [r2, #-3120]!	; 0xc30
     448:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     44c:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     450:	303b383b 	eorscc	r3, fp, fp, lsr r8
     454:	535f003b 	cmppl	pc, #59	; 0x3b
     458:	75207461 	strvc	r7, [r0, #-1121]!	; 0x461
     45c:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
     460:	2064656e 	rsbcs	r6, r4, lr, ror #10
     464:	726f6873 	rsbvc	r6, pc, #7536640	; 0x730000
     468:	465f2074 			; <UNDEFINED> instruction: 0x465f2074
     46c:	74636172 	strbtvc	r6, [r3], #-370	; 0x172
     470:	3028743a 	eorcc	r7, r8, sl, lsr r4
     474:	2936322c 	ldmdbcs	r6!, {r2, r3, r5, r9, ip, sp}
     478:	3028723d 	eorcc	r7, r8, sp, lsr r2
     47c:	3b29312c 	blcc	a4c934 <start_address+0xa3c934>
     480:	3b303b31 	blcc	c0f14c <start_address+0xbff14c>
     484:	61535f00 	cmpvs	r3, r0, lsl #30
     488:	6e752074 	mrcvs	0, 3, r2, cr5, cr4, {3}
     48c:	6e676973 	mcrvs	9, 3, r6, cr7, cr3, {3}
     490:	5f206465 	svcpl	0x00206465
     494:	63617246 	cmnvs	r1, #1610612740	; 0x60000004
     498:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     49c:	37322c30 			; <UNDEFINED> instruction: 0x37322c30
     4a0:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     4a4:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     4a8:	303b323b 	eorscc	r3, fp, fp, lsr r2
     4ac:	535f003b 	cmppl	pc, #59	; 0x3b
     4b0:	75207461 	strvc	r7, [r0, #-1121]!	; 0x461
     4b4:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
     4b8:	2064656e 	rsbcs	r6, r4, lr, ror #10
     4bc:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     4c0:	72465f20 	subvc	r5, r6, #32, 30	; 0x80
     4c4:	3a746361 	bcc	1d19250 <start_address+0x1d09250>
     4c8:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     4cc:	3d293832 	stccc	8, cr3, [r9, #-200]!	; 0xffffff38
     4d0:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     4d4:	343b2931 	ldrtcc	r2, [fp], #-2353	; 0x931
     4d8:	003b303b 	eorseq	r3, fp, fp, lsr r0
     4dc:	7461535f 	strbtvc	r5, [r1], #-863	; 0x35f
     4e0:	736e7520 	cmnvc	lr, #32, 10	; 0x8000000
     4e4:	656e6769 	strbvs	r6, [lr, #-1897]!	; 0x769
     4e8:	6f6c2064 	svcvs	0x006c2064
     4ec:	6c20676e 	stcvs	7, cr6, [r0], #-440	; 0xfffffe48
     4f0:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     4f4:	6172465f 	cmnvs	r2, pc, asr r6
     4f8:	743a7463 	ldrtvc	r7, [sl], #-1123	; 0x463
     4fc:	322c3028 	eorcc	r3, ip, #40	; 0x28
     500:	723d2939 	eorsvc	r2, sp, #933888	; 0xe4000
     504:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     508:	3b383b29 	blcc	e0f1b4 <start_address+0xdff1b4>
     50c:	73003b30 	movwvc	r3, #2864	; 0xb30
     510:	74726f68 	ldrbtvc	r6, [r2], #-3944	; 0xf68
     514:	63415f20 	movtvs	r5, #7968	; 0x1f20
     518:	3a6d7563 	bcc	1b5daac <start_address+0x1b4daac>
     51c:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     520:	3d293033 	stccc	0, cr3, [r9, #-204]!	; 0xffffff34
     524:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     528:	323b2931 	eorscc	r2, fp, #802816	; 0xc4000
     52c:	003b303b 	eorseq	r3, fp, fp, lsr r0
     530:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     534:	63415f20 	movtvs	r5, #7968	; 0x1f20
     538:	3a6d7563 	bcc	1b5dacc <start_address+0x1b4dacc>
     53c:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     540:	3d293133 	stfccs	f3, [r9, #-204]!	; 0xffffff34
     544:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     548:	383b2931 	ldmdacc	fp!, {r0, r4, r5, r8, fp, sp}
     54c:	003b303b 	eorseq	r3, fp, fp, lsr r0
     550:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     554:	6e6f6c20 	cdpvs	12, 6, cr6, cr15, cr0, {1}
     558:	415f2067 	cmpmi	pc, r7, rrx
     55c:	6d756363 	ldclvs	3, cr6, [r5, #-396]!	; 0xfffffe74
     560:	3028743a 	eorcc	r7, r8, sl, lsr r4
     564:	2932332c 	ldmdbcs	r2!, {r2, r3, r5, r8, r9, ip, sp}
     568:	3028723d 	eorcc	r7, r8, sp, lsr r2
     56c:	3b29312c 	blcc	a4ca24 <start_address+0xa3ca24>
     570:	3b303b38 	blcc	c0f258 <start_address+0xbff258>
     574:	736e7500 	cmnvc	lr, #0, 10
     578:	656e6769 	strbvs	r6, [lr, #-1897]!	; 0x769
     57c:	68732064 	ldmdavs	r3!, {r2, r5, r6, sp}^
     580:	2074726f 	rsbscs	r7, r4, pc, ror #4
     584:	6363415f 	cmnvs	r3, #-1073741801	; 0xc0000017
     588:	743a6d75 	ldrtvc	r6, [sl], #-3445	; 0xd75
     58c:	332c3028 			; <UNDEFINED> instruction: 0x332c3028
     590:	723d2933 	eorsvc	r2, sp, #835584	; 0xcc000
     594:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     598:	3b323b29 	blcc	c8f244 <start_address+0xc7f244>
     59c:	75003b30 	strvc	r3, [r0, #-2864]	; 0xb30
     5a0:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
     5a4:	2064656e 	rsbcs	r6, r4, lr, ror #10
     5a8:	6363415f 	cmnvs	r3, #-1073741801	; 0xc0000017
     5ac:	743a6d75 	ldrtvc	r6, [sl], #-3445	; 0xd75
     5b0:	332c3028 			; <UNDEFINED> instruction: 0x332c3028
     5b4:	723d2934 	eorsvc	r2, sp, #52, 18	; 0xd0000
     5b8:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     5bc:	3b343b29 	blcc	d0f268 <start_address+0xcff268>
     5c0:	75003b30 	strvc	r3, [r0, #-2864]	; 0xb30
     5c4:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
     5c8:	2064656e 	rsbcs	r6, r4, lr, ror #10
     5cc:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     5d0:	63415f20 	movtvs	r5, #7968	; 0x1f20
     5d4:	3a6d7563 	bcc	1b5db68 <start_address+0x1b4db68>
     5d8:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     5dc:	3d293533 	cfstr32cc	mvfx3, [r9, #-204]!	; 0xffffff34
     5e0:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     5e4:	383b2931 	ldmdacc	fp!, {r0, r4, r5, r8, fp, sp}
     5e8:	003b303b 	eorseq	r3, fp, fp, lsr r0
     5ec:	69736e75 	ldmdbvs	r3!, {r0, r2, r4, r5, r6, r9, sl, fp, sp, lr}^
     5f0:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
     5f4:	6e6f6c20 	cdpvs	12, 6, cr6, cr15, cr0, {1}
     5f8:	6f6c2067 	svcvs	0x006c2067
     5fc:	5f20676e 	svcpl	0x0020676e
     600:	75636341 	strbvc	r6, [r3, #-833]!	; 0x341
     604:	28743a6d 	ldmdacs	r4!, {r0, r2, r3, r5, r6, r9, fp, ip, sp}^
     608:	36332c30 			; <UNDEFINED> instruction: 0x36332c30
     60c:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     610:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     614:	303b383b 	eorscc	r3, fp, fp, lsr r8
     618:	535f003b 	cmppl	pc, #59	; 0x3b
     61c:	73207461 			; <UNDEFINED> instruction: 0x73207461
     620:	74726f68 	ldrbtvc	r6, [r2], #-3944	; 0xf68
     624:	63415f20 	movtvs	r5, #7968	; 0x1f20
     628:	3a6d7563 	bcc	1b5dbbc <start_address+0x1b4dbbc>
     62c:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     630:	3d293733 	stccc	7, cr3, [r9, #-204]!	; 0xffffff34
     634:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     638:	323b2931 	eorscc	r2, fp, #802816	; 0xc4000
     63c:	003b303b 	eorseq	r3, fp, fp, lsr r0
     640:	7461535f 	strbtvc	r5, [r1], #-863	; 0x35f
     644:	63415f20 	movtvs	r5, #7968	; 0x1f20
     648:	3a6d7563 	bcc	1b5dbdc <start_address+0x1b4dbdc>
     64c:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     650:	3d293833 	stccc	8, cr3, [r9, #-204]!	; 0xffffff34
     654:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     658:	343b2931 	ldrtcc	r2, [fp], #-2353	; 0x931
     65c:	003b303b 	eorseq	r3, fp, fp, lsr r0
     660:	7461535f 	strbtvc	r5, [r1], #-863	; 0x35f
     664:	6e6f6c20 	cdpvs	12, 6, cr6, cr15, cr0, {1}
     668:	415f2067 	cmpmi	pc, r7, rrx
     66c:	6d756363 	ldclvs	3, cr6, [r5, #-396]!	; 0xfffffe74
     670:	3028743a 	eorcc	r7, r8, sl, lsr r4
     674:	2939332c 	ldmdbcs	r9!, {r2, r3, r5, r8, r9, ip, sp}
     678:	3028723d 	eorcc	r7, r8, sp, lsr r2
     67c:	3b29312c 	blcc	a4cb34 <start_address+0xa3cb34>
     680:	3b303b38 	blcc	c0f368 <start_address+0xbff368>
     684:	61535f00 	cmpvs	r3, r0, lsl #30
     688:	6f6c2074 	svcvs	0x006c2074
     68c:	6c20676e 	stcvs	7, cr6, [r0], #-440	; 0xfffffe48
     690:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     694:	6363415f 	cmnvs	r3, #-1073741801	; 0xc0000017
     698:	743a6d75 	ldrtvc	r6, [sl], #-3445	; 0xd75
     69c:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     6a0:	723d2930 	eorsvc	r2, sp, #48, 18	; 0xc0000
     6a4:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     6a8:	3b383b29 	blcc	e0f354 <start_address+0xdff354>
     6ac:	5f003b30 	svcpl	0x00003b30
     6b0:	20746153 	rsbscs	r6, r4, r3, asr r1
     6b4:	69736e75 	ldmdbvs	r3!, {r0, r2, r4, r5, r6, r9, sl, fp, sp, lr}^
     6b8:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
     6bc:	6f687320 	svcvs	0x00687320
     6c0:	5f207472 	svcpl	0x00207472
     6c4:	75636341 	strbvc	r6, [r3, #-833]!	; 0x341
     6c8:	28743a6d 	ldmdacs	r4!, {r0, r2, r3, r5, r6, r9, fp, ip, sp}^
     6cc:	31342c30 	teqcc	r4, r0, lsr ip
     6d0:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     6d4:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     6d8:	303b323b 	eorscc	r3, fp, fp, lsr r2
     6dc:	535f003b 	cmppl	pc, #59	; 0x3b
     6e0:	75207461 	strvc	r7, [r0, #-1121]!	; 0x461
     6e4:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
     6e8:	2064656e 	rsbcs	r6, r4, lr, ror #10
     6ec:	6363415f 	cmnvs	r3, #-1073741801	; 0xc0000017
     6f0:	743a6d75 	ldrtvc	r6, [sl], #-3445	; 0xd75
     6f4:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     6f8:	723d2932 	eorsvc	r2, sp, #819200	; 0xc8000
     6fc:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     700:	3b343b29 	blcc	d0f3ac <start_address+0xcff3ac>
     704:	5f003b30 	svcpl	0x00003b30
     708:	20746153 	rsbscs	r6, r4, r3, asr r1
     70c:	69736e75 	ldmdbvs	r3!, {r0, r2, r4, r5, r6, r9, sl, fp, sp, lr}^
     710:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
     714:	6e6f6c20 	cdpvs	12, 6, cr6, cr15, cr0, {1}
     718:	415f2067 	cmpmi	pc, r7, rrx
     71c:	6d756363 	ldclvs	3, cr6, [r5, #-396]!	; 0xfffffe74
     720:	3028743a 	eorcc	r7, r8, sl, lsr r4
     724:	2933342c 	ldmdbcs	r3!, {r2, r3, r5, sl, ip, sp}
     728:	3028723d 	eorcc	r7, r8, sp, lsr r2
     72c:	3b29312c 	blcc	a4cbe4 <start_address+0xa3cbe4>
     730:	3b303b38 	blcc	c0f418 <start_address+0xbff418>
     734:	61535f00 	cmpvs	r3, r0, lsl #30
     738:	6e752074 	mrcvs	0, 3, r2, cr5, cr4, {3}
     73c:	6e676973 	mcrvs	9, 3, r6, cr7, cr3, {3}
     740:	6c206465 	cfstrsvs	mvf6, [r0], #-404	; 0xfffffe6c
     744:	20676e6f 	rsbcs	r6, r7, pc, ror #28
     748:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
     74c:	63415f20 	movtvs	r5, #7968	; 0x1f20
     750:	3a6d7563 	bcc	1b5dce4 <start_address+0x1b4dce4>
     754:	2c302874 	ldccs	8, cr2, [r0], #-464	; 0xfffffe30
     758:	3d293434 	cfstrscc	mvf3, [r9, #-208]!	; 0xffffff30
     75c:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
     760:	383b2931 	ldmdacc	fp!, {r0, r4, r5, r8, fp, sp}
     764:	003b303b 	eorseq	r3, fp, fp, lsr r0
     768:	64696f76 	strbtvs	r6, [r9], #-3958	; 0xf76
     76c:	3028743a 	eorcc	r7, r8, sl, lsr r4
     770:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
     774:	2c30283d 	ldccs	8, cr2, [r0], #-244	; 0xffffff0c
     778:	00293534 	eoreq	r3, r9, r4, lsr r5
     77c:	6c636e69 	stclvs	14, cr6, [r3], #-420	; 0xfffffe5c
     780:	2f656475 	svccs	0x00656475
     784:	2e717269 	cdpcs	2, 7, cr7, cr1, cr9, {3}
     788:	6e690068 	cdpvs	0, 6, cr0, cr9, cr8, {3}
     78c:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
     790:	79742f65 	ldmdbvc	r4!, {r0, r2, r5, r6, r8, r9, sl, fp, sp}^
     794:	2e736570 	mrccs	5, 3, r6, cr3, cr0, {3}
     798:	6f620068 	svcvs	0x00620068
     79c:	743a6c6f 	ldrtvc	r6, [sl], #-3183	; 0xc6f
     7a0:	312c3228 			; <UNDEFINED> instruction: 0x312c3228
     7a4:	30283d29 	eorcc	r3, r8, r9, lsr #26
     7a8:	0029312c 	eoreq	r3, r9, ip, lsr #2
     7ac:	38746e69 	ldmdacc	r4!, {r0, r3, r5, r6, r9, sl, fp, sp, lr}^
     7b0:	743a745f 	ldrtvc	r7, [sl], #-1119	; 0x45f
     7b4:	322c3228 	eorcc	r3, ip, #40, 4	; 0x80000002
     7b8:	30283d29 	eorcc	r3, r8, r9, lsr #26
     7bc:	2930312c 	ldmdbcs	r0!, {r2, r3, r5, r8, ip, sp}
     7c0:	6e697500 	cdpvs	5, 6, cr7, cr9, cr0, {0}
     7c4:	745f3874 	ldrbvc	r3, [pc], #-2164	; 7cc <start_address-0xf834>
     7c8:	3228743a 	eorcc	r7, r8, #973078528	; 0x3a000000
     7cc:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
     7d0:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     7d4:	69002931 	stmdbvs	r0, {r0, r4, r5, r8, fp, sp}
     7d8:	3631746e 	ldrtcc	r7, [r1], -lr, ror #8
     7dc:	743a745f 	ldrtvc	r7, [sl], #-1119	; 0x45f
     7e0:	342c3228 	strtcc	r3, [ip], #-552	; 0x228
     7e4:	30283d29 	eorcc	r3, r8, r9, lsr #26
     7e8:	0029382c 	eoreq	r3, r9, ip, lsr #16
     7ec:	746e6975 	strbtvc	r6, [lr], #-2421	; 0x975
     7f0:	745f3631 	ldrbvc	r3, [pc], #-1585	; 7f8 <start_address-0xf808>
     7f4:	3228743a 	eorcc	r7, r8, #973078528	; 0x3a000000
     7f8:	3d29352c 	cfstr32cc	mvfx3, [r9, #-176]!	; 0xffffff50
     7fc:	392c3028 	stmdbcc	ip!, {r3, r5, ip, sp}
     800:	6e690029 	cdpvs	0, 6, cr0, cr9, cr9, {1}
     804:	5f323374 	svcpl	0x00323374
     808:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
     80c:	29362c32 	ldmdbcs	r6!, {r1, r4, r5, sl, fp, sp}
     810:	2c30283d 	ldccs	8, cr2, [r0], #-244	; 0xffffff0c
     814:	75002931 	strvc	r2, [r0, #-2353]	; 0x931
     818:	33746e69 	cmncc	r4, #1680	; 0x690
     81c:	3a745f32 	bcc	1d184ec <start_address+0x1d084ec>
     820:	2c322874 	ldccs	8, cr2, [r2], #-464	; 0xfffffe30
     824:	283d2937 	ldmdacs	sp!, {r0, r1, r2, r4, r5, r8, fp, sp}
     828:	29342c30 	ldmdbcs	r4!, {r4, r5, sl, fp, sp}
     82c:	6e697500 	cdpvs	5, 6, cr7, cr9, cr0, {0}
     830:	72747074 	rsbsvc	r7, r4, #116	; 0x74
     834:	743a745f 	ldrtvc	r7, [sl], #-1119	; 0x45f
     838:	382c3228 	stmdacc	ip!, {r3, r5, r9, ip, sp}
     83c:	32283d29 	eorcc	r3, r8, #2624	; 0xa40
     840:	0029372c 	eoreq	r3, r9, ip, lsr #14
     844:	657a6973 	ldrbvs	r6, [sl, #-2419]!	; 0x973
     848:	743a745f 	ldrtvc	r7, [sl], #-1119	; 0x45f
     84c:	392c3228 	stmdbcc	ip!, {r3, r5, r9, ip, sp}
     850:	32283d29 	eorcc	r3, r8, #2624	; 0xa40
     854:	0029372c 	eoreq	r3, r9, ip, lsr #14
     858:	65746e49 	ldrbvs	r6, [r4, #-3657]!	; 0xe49
     85c:	70757272 	rsbsvc	r7, r5, r2, ror r2
     860:	6e614874 	mcrvs	8, 3, r4, cr1, cr4, {3}
     864:	72656c64 	rsbvc	r6, r5, #100, 24	; 0x6400
     868:	28743a54 	ldmdacs	r4!, {r2, r4, r6, r9, fp, ip, sp}^
     86c:	29312c31 	ldmdbcs	r1!, {r0, r4, r5, sl, fp, sp}
     870:	2c31283d 	ldccs	8, cr2, [r1], #-244	; 0xffffff0c
     874:	2a3d2932 	bcs	f4ad44 <start_address+0xf3ad44>
     878:	332c3128 			; <UNDEFINED> instruction: 0x332c3128
     87c:	28663d29 	stmdacs	r6!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     880:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
     884:	6e690029 	cdpvs	0, 6, cr0, cr9, cr9, {1}
     888:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
     88c:	6d6d2f65 	stclvs	15, cr2, [sp, #-404]!	; 0xfffffe6c
     890:	756d6d2f 	strbvc	r6, [sp, #-3375]!	; 0xd2f
     894:	5000682e 	andpl	r6, r0, lr, lsr #16
     898:	44656761 	strbtmi	r6, [r5], #-1889	; 0x761
     89c:	6e457269 	cdpvs	2, 4, cr7, cr5, cr9, {3}
     8a0:	54797274 	ldrbtpl	r7, [r9], #-628	; 0x274
     8a4:	3328743a 			; <UNDEFINED> instruction: 0x3328743a
     8a8:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
     8ac:	322c3328 	eorcc	r3, ip, #40, 6	; 0xa0000000
     8b0:	34733d29 	ldrbtcc	r3, [r3], #-3369	; 0xd29
     8b4:	65707974 	ldrbvs	r7, [r0, #-2420]!	; 0x974
     8b8:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     8bc:	302c2934 	eorcc	r2, ip, r4, lsr r9
     8c0:	3a3b322c 	bcc	ecd178 <start_address+0xebd178>
     8c4:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     8c8:	2c322c29 	ldccs	12, cr2, [r2], #-164	; 0xffffff5c
     8cc:	6f643b33 	svcvs	0x00643b33
     8d0:	6e69616d 	powvsez	f6, f1, #5.0
     8d4:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     8d8:	352c2934 	strcc	r2, [ip, #-2356]!	; 0x934
     8dc:	3a3b342c 	bcc	ecd994 <start_address+0xebd994>
     8e0:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     8e4:	2c392c29 	ldccs	12, cr2, [r9], #-164	; 0xffffff5c
     8e8:	61623b31 	cmnvs	r2, r1, lsr fp
     8ec:	283a6573 	ldmdacs	sl!, {r0, r1, r4, r5, r6, r8, sl, sp, lr}
     8f0:	29342c30 	ldmdbcs	r4!, {r4, r5, sl, fp, sp}
     8f4:	2c30312c 	ldfcss	f3, [r0], #-176	; 0xffffff50
     8f8:	3b3b3232 	blcc	ecd1c8 <start_address+0xebd1c8>
     8fc:	67615000 	strbvs	r5, [r1, -r0]!
     900:	62615465 	rsbvs	r5, r1, #1694498816	; 0x65000000
     904:	6e45656c 	cdpvs	5, 4, cr6, cr5, cr12, {3}
     908:	54797274 	ldrbtpl	r7, [r9], #-628	; 0x274
     90c:	3328743a 			; <UNDEFINED> instruction: 0x3328743a
     910:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
     914:	342c3328 	strtcc	r3, [ip], #-808	; 0x328
     918:	34733d29 	ldrbtcc	r3, [r3], #-3369	; 0xd29
     91c:	65707974 	ldrbvs	r7, [r0, #-2420]!	; 0x974
     920:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     924:	302c2934 	eorcc	r2, ip, r4, lsr r9
     928:	623b322c 	eorsvs	r3, fp, #44, 4	; 0xc0000002
     92c:	65666675 	strbvs	r6, [r6, #-1653]!	; 0x675
     930:	6c626172 	stfvse	f6, [r2], #-456	; 0xfffffe38
     934:	30283a65 	eorcc	r3, r8, r5, ror #20
     938:	2c29342c 	cfstrscs	mvf3, [r9], #-176	; 0xffffff50
     93c:	3b312c32 	blcc	c4ba0c <start_address+0xc3ba0c>
     940:	68636163 	stmdavs	r3!, {r0, r1, r5, r6, r8, sp, lr}^
     944:	6c626165 	stfvse	f6, [r2], #-404	; 0xfffffe6c
     948:	30283a65 	eorcc	r3, r8, r5, ror #20
     94c:	2c29342c 	cfstrscs	mvf3, [r9], #-176	; 0xffffff50
     950:	3b312c33 	blcc	c4ba24 <start_address+0xc3ba24>
     954:	6d726570 	cfldr64vs	mvdx6, [r2, #-448]!	; 0xfffffe40
     958:	69737369 	ldmdbvs	r3!, {r0, r3, r5, r6, r8, r9, ip, sp, lr}^
     95c:	3a736e6f 	bcc	1cdc320 <start_address+0x1ccc320>
     960:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     964:	2c342c29 	ldccs	12, cr2, [r4], #-164	; 0xffffff5c
     968:	61623b38 	cmnvs	r2, r8, lsr fp
     96c:	283a6573 	ldmdacs	sl!, {r0, r1, r4, r5, r6, r8, sl, sp, lr}
     970:	29342c30 	ldmdbcs	r4!, {r4, r5, sl, fp, sp}
     974:	2c32312c 	ldfcss	f3, [r2], #-176	; 0xffffff50
     978:	3b3b3032 	blcc	ecca48 <start_address+0xebca48>
     97c:	756f7200 	strbvc	r7, [pc, #-512]!	; 784 <start_address-0xf87c>
     980:	676e6974 			; <UNDEFINED> instruction: 0x676e6974
     984:	726f635f 	rsbvc	r6, pc, #2080374785	; 0x7c000001
     988:	6e633065 	cdpvs	0, 6, cr3, cr3, cr5, {3}
     98c:	745f7674 	ldrbvc	r7, [pc], #-1652	; 994 <start_address-0xf66c>
     990:	6f635f6f 	svcvs	0x00635f6f
     994:	69306572 	ldmdbvs	r0!, {r1, r4, r5, r6, r8, sl, sp, lr}
     998:	463a7172 			; <UNDEFINED> instruction: 0x463a7172
     99c:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     9a0:	62002935 	andvs	r2, r0, #868352	; 0xd4000
     9a4:	3a657361 	bcc	195d730 <start_address+0x194d730>
     9a8:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
     9ac:	65720029 	ldrbvs	r0, [r2, #-41]!	; 0x29
     9b0:	635f6461 	cmpvs	pc, #1627389952	; 0x61000000
     9b4:	3065726f 	rsbcc	r7, r5, pc, ror #4
     9b8:	656d6974 	strbvs	r6, [sp, #-2420]!	; 0x974
     9bc:	65705f72 	ldrbvs	r5, [r0, #-3954]!	; 0xf72
     9c0:	6e69646e 	cdpvs	4, 6, cr6, cr9, cr14, {3}
     9c4:	28463a67 	stmdacs	r6, {r0, r1, r2, r5, r6, r9, fp, ip, sp}^
     9c8:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
     9cc:	706d7400 	rsbvc	r7, sp, r0, lsl #8
     9d0:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
     9d4:	65002937 	strvs	r2, [r0, #-2359]	; 0x937
     9d8:	6c62616e 	stfvse	f6, [r2], #-440	; 0xfffffe48
     9dc:	6e635f65 	cdpvs	15, 6, cr5, cr3, cr5, {3}
     9e0:	463a7674 			; <UNDEFINED> instruction: 0x463a7674
     9e4:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     9e8:	63002935 	movwvs	r2, #2357	; 0x935
     9ec:	5f76746e 	svcpl	0x0076746e
     9f0:	3a6c7463 	bcc	1b1db84 <start_address+0x1b0db84>
     9f4:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
     9f8:	69640029 	stmdbvs	r4!, {r0, r3, r5}^
     9fc:	6c626173 	stfvse	f6, [r2], #-460	; 0xfffffe34
     a00:	6e635f65 	cdpvs	15, 6, cr5, cr3, cr5, {3}
     a04:	463a7674 			; <UNDEFINED> instruction: 0x463a7674
     a08:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     a0c:	77002935 	smladxvc	r0, r5, r9, r2
     a10:	65746972 	ldrbvs	r6, [r4, #-2418]!	; 0x972
     a14:	746e635f 	strbtvc	r6, [lr], #-863	; 0x35f
     a18:	76745f76 	uhsub16vc	r5, r4, r6
     a1c:	463a6c61 	ldrtmi	r6, [sl], -r1, ror #24
     a20:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     a24:	76002935 			; <UNDEFINED> instruction: 0x76002935
     a28:	703a6c61 	eorsvc	r6, sl, r1, ror #24
     a2c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
     a30:	72690029 	rsbvc	r0, r9, #41	; 0x29
     a34:	696e4971 	stmdbvs	lr!, {r0, r4, r5, r6, r8, fp, lr}^
     a38:	28463a74 	stmdacs	r6, {r2, r4, r5, r6, r9, fp, ip, sp}^
     a3c:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
     a40:	6e650029 	cdpvs	0, 6, cr0, cr5, cr9, {1}
     a44:	656c6261 	strbvs	r6, [ip, #-609]!	; 0x261
     a48:	3a515249 	bcc	1455374 <start_address+0x1445374>
     a4c:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
     a50:	00293534 	eoreq	r3, r9, r4, lsr r5
     a54:	656e696c 	strbvs	r6, [lr, #-2412]!	; 0x96c
     a58:	3228703a 	eorcc	r7, r8, #58	; 0x3a
     a5c:	0029372c 	eoreq	r3, r9, ip, lsr #14
     a60:	63697076 	cmnvs	r9, #118	; 0x76
     a64:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     a68:	3d293634 	stccc	6, cr3, [r9, #-208]!	; 0xffffff30
     a6c:	2c30282a 	ldccs	8, cr2, [r0], #-168	; 0xffffff58
     a70:	3d293734 	stccc	7, cr3, [r9, #-208]!	; 0xffffff30
     a74:	2c322842 	ldccs	8, cr2, [r2], #-264	; 0xfffffef8
     a78:	64002937 	strvs	r2, [r0], #-2359	; 0x937
     a7c:	62617369 	rsbvs	r7, r1, #-1543503871	; 0xa4000001
     a80:	5249656c 	subpl	r6, r9, #108, 10	; 0x1b000000
     a84:	28463a51 	stmdacs	r6, {r0, r4, r6, r9, fp, ip, sp}^
     a88:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
     a8c:	70760029 	rsbsvc	r0, r6, r9, lsr #32
     a90:	283a6369 	ldmdacs	sl!, {r0, r3, r5, r6, r8, r9, sp, lr}
     a94:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
     a98:	65670029 	strbvs	r0, [r7, #-41]!	; 0x29
     a9c:	6e655074 	mcrvs	0, 3, r5, cr5, cr4, {3}
     aa0:	676e6964 	strbvs	r6, [lr, -r4, ror #18]!
     aa4:	73515249 	cmpvc	r1, #-1879048188	; 0x90000004
     aa8:	3028463a 	eorcc	r4, r8, sl, lsr r6
     aac:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
     ab0:	73657200 	cmnvc	r5, #0, 4
     ab4:	3a746c75 	bcc	1d1bc90 <start_address+0x1d0bc90>
     ab8:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
     abc:	3d293834 	stccc	8, cr3, [r9, #-208]!	; 0xffffff30
     ac0:	2c32282a 	ldccs	8, cr2, [r2], #-168	; 0xffffff58
     ac4:	69002931 	stmdbvs	r0, {r0, r4, r5, r8, fp, sp}
     ac8:	74537172 	ldrbvc	r7, [r3], #-370	; 0x172
     acc:	73757461 	cmnvc	r5, #1627389952	; 0x61000000
     ad0:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     ad4:	00293734 	eoreq	r3, r9, r4, lsr r7
     ad8:	32283a69 	eorcc	r3, r8, #430080	; 0x69000
     adc:	0029372c 	eoreq	r3, r9, ip, lsr #14
     ae0:	66746e63 	ldrbtvs	r6, [r4], -r3, ror #28
     ae4:	533a7172 	teqpl	sl, #-2147483620	; 0x8000001c
     ae8:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
     aec:	72610029 	rsbvc	r0, r1, #41	; 0x29
     af0:	722f6863 	eorvc	r6, pc, #6488064	; 0x630000
     af4:	69707361 	ldmdbvs	r0!, {r0, r5, r6, r8, r9, ip, sp, lr}^
     af8:	72732f32 	rsbsvc	r2, r3, #50, 30	; 0xc8
     afc:	61752f63 	cmnvs	r5, r3, ror #30
     b00:	632e7472 			; <UNDEFINED> instruction: 0x632e7472
     b04:	636e6900 	cmnvs	lr, #0, 18
     b08:	6564756c 	strbvs	r7, [r4, #-1388]!	; 0x56c
     b0c:	7665642f 	strbtvc	r6, [r5], -pc, lsr #8
     b10:	7261752f 	rsbvc	r7, r1, #197132288	; 0xbc00000
     b14:	00682e74 	rsbeq	r2, r8, r4, ror lr
     b18:	28543a20 	ldmdacs	r4, {r5, r9, fp, ip, sp}^
     b1c:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
     b20:	47653d29 	strbmi	r3, [r5, -r9, lsr #26]!
     b24:	5f4f4950 	svcpl	0x004f4950
     b28:	45534142 	ldrbmi	r4, [r3, #-322]	; 0x142
     b2c:	46464f5f 			; <UNDEFINED> instruction: 0x46464f5f
     b30:	3930323a 	ldmdbcc	r0!, {r1, r3, r4, r5, r9, ip, sp}
     b34:	32353137 	eorscc	r3, r5, #-1073741811	; 0xc000000d
     b38:	5050472c 	subspl	r4, r0, ip, lsr #14
     b3c:	323a4455 	eorscc	r4, sl, #1426063360	; 0x55000000
     b40:	33373930 	teqcc	r7, #48, 18	; 0xc0000
     b44:	472c3030 			; <UNDEFINED> instruction: 0x472c3030
     b48:	44555050 	ldrbmi	r5, [r5], #-80	; 0x50
     b4c:	304b4c43 	subcc	r4, fp, r3, asr #24
     b50:	3930323a 	ldmdbcc	r0!, {r1, r3, r4, r5, r9, ip, sp}
     b54:	34303337 	ldrtcc	r3, [r0], #-823	; 0x337
     b58:	5241552c 	subpl	r5, r1, #44, 10	; 0xb000000
     b5c:	425f3054 	subsmi	r3, pc, #84	; 0x54
     b60:	5f455341 	svcpl	0x00455341
     b64:	3a46464f 	bcc	11924a8 <start_address+0x11824a8>
     b68:	31303132 	teqcc	r0, r2, lsr r1
     b6c:	2c383432 	cfldrscs	mvf3, [r8], #-200	; 0xffffff38
     b70:	54524155 	ldrbpl	r4, [r2], #-341	; 0x155
     b74:	52445f30 	subpl	r5, r4, #48, 30	; 0xc0
     b78:	3031323a 	eorscc	r3, r1, sl, lsr r2
     b7c:	38343231 	ldmdacc	r4!, {r0, r4, r5, r9, ip, sp}
     b80:	5241552c 	subpl	r5, r1, #44, 10	; 0xb000000
     b84:	525f3054 	subspl	r3, pc, #84	; 0x54
     b88:	43455253 	movtmi	r5, #21075	; 0x5253
     b8c:	31323a52 	teqcc	r2, r2, asr sl
     b90:	35323130 	ldrcc	r3, [r2, #-304]!	; 0x130
     b94:	41552c32 	cmpmi	r5, r2, lsr ip
     b98:	5f305452 	svcpl	0x00305452
     b9c:	323a5246 	eorscc	r5, sl, #1610612740	; 0x60000004
     ba0:	32313031 	eorscc	r3, r1, #49	; 0x31
     ba4:	552c3237 	strpl	r3, [ip, #-567]!	; 0x237
     ba8:	30545241 	subscc	r5, r4, r1, asr #4
     bac:	504c495f 	subpl	r4, ip, pc, asr r9
     bb0:	31323a52 	teqcc	r2, r2, asr sl
     bb4:	38323130 	ldmdacc	r2!, {r4, r5, r8, ip, sp}
     bb8:	41552c30 	cmpmi	r5, r0, lsr ip
     bbc:	5f305452 	svcpl	0x00305452
     bc0:	44524249 	ldrbmi	r4, [r2], #-585	; 0x249
     bc4:	3031323a 	eorscc	r3, r1, sl, lsr r2
     bc8:	34383231 	ldrtcc	r3, [r8], #-561	; 0x231
     bcc:	5241552c 	subpl	r5, r1, #44, 10	; 0xb000000
     bd0:	465f3054 			; <UNDEFINED> instruction: 0x465f3054
     bd4:	3a445242 	bcc	11154e4 <start_address+0x11054e4>
     bd8:	31303132 	teqcc	r0, r2, lsr r1
     bdc:	2c383832 	ldccs	8, cr3, [r8], #-200	; 0xffffff38
     be0:	54524155 	ldrbpl	r4, [r2], #-341	; 0x155
     be4:	434c5f30 	movtmi	r5, #53040	; 0xcf30
     be8:	323a4852 	eorscc	r4, sl, #5373952	; 0x520000
     bec:	32313031 	eorscc	r3, r1, #49	; 0x31
     bf0:	552c3239 	strpl	r3, [ip, #-569]!	; 0x239
     bf4:	30545241 	subscc	r5, r4, r1, asr #4
     bf8:	3a52435f 	bcc	149197c <start_address+0x148197c>
     bfc:	31303132 	teqcc	r0, r2, lsr r1
     c00:	2c363932 	ldccs	9, cr3, [r6], #-200	; 0xffffff38
     c04:	54524155 	ldrbpl	r4, [r2], #-341	; 0x155
     c08:	46495f30 			; <UNDEFINED> instruction: 0x46495f30
     c0c:	323a534c 	eorscc	r5, sl, #76, 6	; 0x30000001
     c10:	33313031 	teqcc	r1, #49	; 0x31
     c14:	552c3030 	strpl	r3, [ip, #-48]!	; 0x30
     c18:	30545241 	subscc	r5, r4, r1, asr #4
     c1c:	534d495f 	movtpl	r4, #55647	; 0xd95f
     c20:	31323a43 	teqcc	r2, r3, asr #20
     c24:	30333130 	eorscc	r3, r3, r0, lsr r1
     c28:	41552c34 	cmpmi	r5, r4, lsr ip
     c2c:	5f305452 	svcpl	0x00305452
     c30:	3a534952 	bcc	14d3180 <start_address+0x14c3180>
     c34:	31303132 	teqcc	r0, r2, lsr r1
     c38:	2c383033 	ldccs	0, cr3, [r8], #-204	; 0xffffff34
     c3c:	54524155 	ldrbpl	r4, [r2], #-341	; 0x155
     c40:	494d5f30 	stmdbmi	sp, {r4, r5, r8, r9, sl, fp, ip, lr}^
     c44:	31323a53 	teqcc	r2, r3, asr sl
     c48:	31333130 	teqcc	r3, r0, lsr r1
     c4c:	41552c32 	cmpmi	r5, r2, lsr ip
     c50:	5f305452 	svcpl	0x00305452
     c54:	3a524349 	bcc	1491980 <start_address+0x1481980>
     c58:	31303132 	teqcc	r0, r2, lsr r1
     c5c:	2c363133 	ldfcss	f3, [r6], #-204	; 0xffffff34
     c60:	54524155 	ldrbpl	r4, [r2], #-341	; 0x155
     c64:	4d445f30 	stclmi	15, cr5, [r4, #-192]	; 0xffffff40
     c68:	3a524341 	bcc	1491974 <start_address+0x1481974>
     c6c:	31303132 	teqcc	r0, r2, lsr r1
     c70:	2c303233 	lfmcs	f3, 4, [r0], #-204	; 0xffffff34
     c74:	54524155 	ldrbpl	r4, [r2], #-341	; 0x155
     c78:	54495f30 	strbpl	r5, [r9], #-3888	; 0xf30
     c7c:	323a5243 	eorscc	r5, sl, #805306372	; 0x30000004
     c80:	33313031 	teqcc	r1, #49	; 0x31
     c84:	552c3637 	strpl	r3, [ip, #-1591]!	; 0x637
     c88:	30545241 	subscc	r5, r4, r1, asr #4
     c8c:	4954495f 	ldmdbmi	r4, {r0, r1, r2, r3, r4, r6, r8, fp, lr}^
     c90:	31323a50 	teqcc	r2, r0, asr sl
     c94:	38333130 	ldmdacc	r3!, {r4, r5, r8, ip, sp}
     c98:	41552c30 	cmpmi	r5, r0, lsr ip
     c9c:	5f305452 	svcpl	0x00305452
     ca0:	504f5449 	subpl	r5, pc, r9, asr #8
     ca4:	3031323a 	eorscc	r3, r1, sl, lsr r2
     ca8:	34383331 	ldrtcc	r3, [r8], #-817	; 0x331
     cac:	5241552c 	subpl	r5, r1, #44, 10	; 0xb000000
     cb0:	545f3054 	ldrbpl	r3, [pc], #-84	; cb8 <start_address-0xf348>
     cb4:	323a5244 	eorscc	r5, sl, #68, 4	; 0x40000004
     cb8:	33313031 	teqcc	r1, #49	; 0x31
     cbc:	3b2c3838 	blcc	b0eda4 <start_address+0xafeda4>
     cc0:	6c656400 	cfstrdvs	mvd6, [r5], #-0
     cc4:	663a7961 	ldrtvs	r7, [sl], -r1, ror #18
     cc8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     ccc:	63002935 	movwvs	r2, #2357	; 0x935
     cd0:	746e756f 	strbtvc	r7, [lr], #-1391	; 0x56f
     cd4:	3228703a 	eorcc	r7, r8, #58	; 0x3a
     cd8:	0029362c 	eoreq	r3, r9, ip, lsr #12
     cdc:	74726175 	ldrbtvc	r6, [r2], #-373	; 0x175
     ce0:	49766544 	ldmdbmi	r6!, {r2, r6, r8, sl, sp, lr}^
     ce4:	3a74696e 	bcc	1d1b2a4 <start_address+0x1d0b2a4>
     ce8:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
     cec:	00293534 	eoreq	r3, r9, r4, lsr r5
     cf0:	74726175 	ldrbtvc	r6, [r2], #-373	; 0x175
     cf4:	6e617254 	mcrvs	2, 3, r7, cr1, cr4, {2}
     cf8:	74696d73 	strbtvc	r6, [r9], #-3443	; 0xd73
     cfc:	3028463a 	eorcc	r4, r8, sl, lsr r6
     d00:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
     d04:	703a6300 	eorsvc	r6, sl, r0, lsl #6
     d08:	322c3028 	eorcc	r3, ip, #40	; 0x28
     d0c:	61750029 	cmnvs	r5, r9, lsr #32
     d10:	65527472 	ldrbvs	r7, [r2, #-1138]	; 0x472
     d14:	76696563 	strbtvc	r6, [r9], -r3, ror #10
     d18:	28463a65 	stmdacs	r6, {r0, r2, r5, r6, r9, fp, ip, sp}^
     d1c:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     d20:	72617500 	rsbvc	r7, r1, #0, 10
     d24:	61655274 	smcvs	21796	; 0x5524
     d28:	6f547964 	svcvs	0x00547964
     d2c:	76636552 			; <UNDEFINED> instruction: 0x76636552
     d30:	3228463a 	eorcc	r4, r8, #60817408	; 0x3a00000
     d34:	0029312c 	eoreq	r3, r9, ip, lsr #2
     d38:	68637261 	stmdavs	r3!, {r0, r5, r6, r9, ip, sp, lr}^
     d3c:	7361722f 	cmnvc	r1, #-268435454	; 0xf0000002
     d40:	2f326970 	svccs	0x00326970
     d44:	2f637273 	svccs	0x00637273
     d48:	656d6974 	strbvs	r6, [sp, #-2420]!	; 0x974
     d4c:	00632e72 	rsbeq	r2, r3, r2, ror lr
     d50:	6c636e69 	stclvs	14, cr6, [r3], #-420	; 0xfffffe5c
     d54:	2f656475 	svccs	0x00656475
     d58:	656d6974 	strbvs	r6, [sp, #-2420]!	; 0x974
     d5c:	00682e72 	rsbeq	r2, r8, r2, ror lr
     d60:	656d6974 	strbvs	r6, [sp, #-2420]!	; 0x974
     d64:	74655372 	strbtvc	r5, [r5], #-882	; 0x372
     d68:	65746e49 	ldrbvs	r6, [r4, #-3657]!	; 0xe49
     d6c:	6c617672 	stclvs	6, cr7, [r1], #-456	; 0xfffffe38
     d70:	3028463a 	eorcc	r4, r8, sl, lsr r6
     d74:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
     d78:	746e6900 	strbtvc	r6, [lr], #-2304	; 0x900
     d7c:	61767265 	cmnvs	r6, r5, ror #4
     d80:	63694d6c 	cmnvs	r9, #108, 26	; 0x1b00
     d84:	65736f72 	ldrbvs	r6, [r3, #-3954]!	; 0xf72
     d88:	646e6f63 	strbtvs	r6, [lr], #-3939	; 0xf63
     d8c:	3228703a 	eorcc	r7, r8, #58	; 0x3a
     d90:	0029372c 	eoreq	r3, r9, ip, lsr #14
     d94:	656d6974 	strbvs	r6, [sp, #-2420]!	; 0x974
     d98:	30283a72 	eorcc	r3, r8, r2, ror sl
     d9c:	2936342c 	ldmdbcs	r6!, {r2, r3, r5, sl, ip, sp}
     da0:	30282a3d 	eorcc	r2, r8, sp, lsr sl
     da4:	2937342c 	ldmdbcs	r7!, {r2, r3, r5, sl, ip, sp}
     da8:	3228423d 	eorcc	r4, r8, #-805306365	; 0xd0000003
     dac:	0029372c 	eoreq	r3, r9, ip, lsr #14
     db0:	656d6974 	strbvs	r6, [sp, #-2420]!	; 0x974
     db4:	656c4372 	strbvs	r4, [ip, #-882]!	; 0x372
     db8:	6e497261 	cdpvs	2, 4, cr7, cr9, cr1, {3}
     dbc:	72726574 	rsbsvc	r6, r2, #116, 10	; 0x1d000000
     dc0:	3a747075 	bcc	1d1cf9c <start_address+0x1d0cf9c>
     dc4:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
     dc8:	00293534 	eoreq	r3, r9, r4, lsr r5
     dcc:	656d6974 	strbvs	r6, [sp, #-2420]!	; 0x974
     dd0:	30283a72 	eorcc	r3, r8, r2, ror sl
     dd4:	2936342c 	ldmdbcs	r6!, {r2, r3, r5, sl, ip, sp}
     dd8:	6d697400 	cfstrdvs	mvd7, [r9, #-0]
     ddc:	6e497265 	cdpvs	2, 4, cr7, cr9, cr5, {3}
     de0:	463a7469 	ldrtmi	r7, [sl], -r9, ror #8
     de4:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     de8:	61002935 	tstvs	r0, r5, lsr r9
     dec:	2f686372 	svccs	0x00686372
     df0:	70736172 	rsbsvc	r6, r3, r2, ror r1
     df4:	732f3269 			; <UNDEFINED> instruction: 0x732f3269
     df8:	682f6372 	stmdavs	pc!, {r1, r4, r5, r6, r8, r9, sp, lr}	; <UNPREDICTABLE>
     dfc:	77647261 	strbvc	r7, [r4, -r1, ror #4]!
     e00:	2e657261 	cdpcs	2, 6, cr7, cr5, cr1, {3}
     e04:	6e690063 	cdpvs	0, 6, cr0, cr9, cr3, {3}
     e08:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
     e0c:	61682f65 	cmnvs	r8, r5, ror #30
     e10:	61776472 	cmnvs	r7, r2, ror r4
     e14:	682e6572 	stmdavs	lr!, {r1, r4, r5, r6, r8, sl, sp, lr}
     e18:	74656700 	strbtvc	r6, [r5], #-1792	; 0x700
     e1c:	52796850 	rsbspl	r6, r9, #80, 16	; 0x500000
     e20:	69536d61 	ldmdbvs	r3, {r0, r5, r6, r8, sl, fp, sp, lr}^
     e24:	463a657a 			; <UNDEFINED> instruction: 0x463a657a
     e28:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
     e2c:	65670029 	strbvs	r0, [r7, #-41]!	; 0x29
     e30:	494d4d74 	stmdbmi	sp, {r2, r4, r5, r6, r8, sl, fp, lr}^
     e34:	7361424f 	cmnvc	r1, #-268435452	; 0xf0000004
     e38:	79685065 	stmdbvc	r8!, {r0, r2, r5, r6, ip, lr}^
     e3c:	3228463a 	eorcc	r4, r8, #60817408	; 0x3a00000
     e40:	0029372c 	eoreq	r3, r9, ip, lsr #14
     e44:	4d746567 	cfldr64mi	mvdx6, [r4, #-412]!	; 0xfffffe64
     e48:	4d4f494d 	stclmi	9, cr4, [pc, #-308]	; d1c <start_address-0xf2e4>
     e4c:	69536d65 	ldmdbvs	r3, {r0, r2, r5, r6, r8, sl, fp, sp, lr}^
     e50:	463a657a 			; <UNDEFINED> instruction: 0x463a657a
     e54:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
     e58:	65670029 	strbvs	r0, [r7, #-41]!	; 0x29
     e5c:	72615574 	rsbvc	r5, r1, #116, 10	; 0x1d000000
     e60:	71724974 	cmnvc	r2, r4, ror r9
     e64:	3228463a 	eorcc	r4, r8, #60817408	; 0x3a00000
     e68:	0029372c 	eoreq	r3, r9, ip, lsr #14
     e6c:	54746567 	ldrbtpl	r6, [r4], #-1383	; 0x567
     e70:	72656d69 	rsbvc	r6, r5, #6720	; 0x1a40
     e74:	3a717249 	bcc	1c5d7a0 <start_address+0x1c4d7a0>
     e78:	2c322846 	ldccs	8, cr2, [r2], #-280	; 0xfffffee8
     e7c:	73002937 	movwvc	r2, #2359	; 0x937
     e80:	642f6372 	strtvs	r6, [pc], #-882	; e88 <start_address-0xf178>
     e84:	752f7665 	strvc	r7, [pc, #-1637]!	; 827 <start_address-0xf7d9>
     e88:	2e747261 	cdpcs	2, 7, cr7, cr4, cr1, {3}
     e8c:	61750063 	cmnvs	r5, r3, rrx
     e90:	6e497472 	mcrvs	4, 2, r7, cr9, cr2, {3}
     e94:	463a7469 	ldrtmi	r7, [sl], -r9, ror #8
     e98:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
     e9c:	75002935 	strvc	r2, [r0, #-2357]	; 0x935
     ea0:	50747261 	rsbspl	r7, r4, r1, ror #4
     ea4:	3a737475 	bcc	1cde080 <start_address+0x1cce080>
     ea8:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
     eac:	00293534 	eoreq	r3, r9, r4, lsr r5
     eb0:	3a727473 	bcc	1c9e084 <start_address+0x1c8e084>
     eb4:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
     eb8:	3d293634 	stccc	6, cr3, [r9, #-208]!	; 0xffffff30
     ebc:	2c30282a 	ldccs	8, cr2, [r0], #-168	; 0xffffff58
     ec0:	3d293734 	stccc	7, cr3, [r9, #-208]!	; 0xffffff30
     ec4:	2c30286b 	ldccs	8, cr2, [r0], #-428	; 0xfffffe54
     ec8:	69002932 	stmdbvs	r0, {r1, r4, r5, r8, fp, sp}
     ecc:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     ed0:	63002931 	movwvs	r2, #2353	; 0x931
     ed4:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     ed8:	75002931 	strvc	r2, [r0, #-2353]	; 0x931
     edc:	50747261 	rsbspl	r7, r4, r1, ror #4
     ee0:	68637475 	stmdavs	r3!, {r0, r2, r4, r5, r6, sl, ip, sp, lr}^
     ee4:	3028463a 	eorcc	r4, r8, sl, lsr r6
     ee8:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
     eec:	703a6300 	eorsvc	r6, sl, r0, lsl #6
     ef0:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     ef4:	61750029 	cmnvs	r5, r9, lsr #32
     ef8:	65477472 	strbvs	r7, [r7, #-1138]	; 0x472
     efc:	3a686374 	bcc	1a19cd4 <start_address+0x1a09cd4>
     f00:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
     f04:	6b002931 	blvs	b3d0 <start_address-0x4c30>
     f08:	6f637965 	svcvs	0x00637965
     f0c:	283a6564 	ldmdacs	sl!, {r2, r5, r6, r8, sl, sp, lr}
     f10:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     f14:	72696300 	rsbvc	r6, r9, #0, 6
     f18:	616c7563 	cmnvs	ip, r3, ror #10
     f1c:	636e4972 	cmnvs	lr, #1867776	; 0x1c8000
     f20:	3028663a 	eorcc	r6, r8, sl, lsr r6
     f24:	0029312c 	eoreq	r3, r9, ip, lsr #2
     f28:	7265706f 	rsbvc	r7, r5, #111	; 0x6f
     f2c:	3a646e61 	bcc	191c8b8 <start_address+0x190c8b8>
     f30:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
     f34:	63002931 	movwvs	r2, #2353	; 0x931
     f38:	6c637269 	sfmvs	f7, 2, [r3], #-420	; 0xfffffe5c
     f3c:	7a695365 	bvc	1a55cd8 <start_address+0x1a45cd8>
     f40:	28703a65 	ldmdacs	r0!, {r0, r2, r5, r6, r9, fp, ip, sp}^
     f44:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
     f48:	72617500 	rsbvc	r7, r1, #0, 10
     f4c:	746e4974 	strbtvc	r4, [lr], #-2420	; 0x974
     f50:	75727265 	ldrbvc	r7, [r2, #-613]!	; 0x265
     f54:	61487470 	hvcvs	34624	; 0x8740
     f58:	656c646e 	strbvs	r6, [ip, #-1134]!	; 0x46e
     f5c:	28663a72 	stmdacs	r6!, {r1, r4, r5, r6, r9, fp, ip, sp}^
     f60:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
     f64:	656e0029 	strbvs	r0, [lr, #-41]!	; 0x29
     f68:	61654877 	smcvs	21639	; 0x5487
     f6c:	30283a64 	eorcc	r3, r8, r4, ror #20
     f70:	0029312c 	eoreq	r3, r9, ip, lsr #2
     f74:	61746164 	cmnvs	r4, r4, ror #2
     f78:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
     f7c:	72002931 	andvc	r2, r0, #802816	; 0xc4000
     f80:	69656365 	stmdbvs	r5!, {r0, r2, r5, r6, r8, r9, sp, lr}^
     f84:	75426576 	strbvc	r6, [r2, #-1398]	; 0x576
     f88:	72656666 	rsbvc	r6, r5, #106954752	; 0x6600000
     f8c:	3028533a 	eorcc	r5, r8, sl, lsr r3
     f90:	2938342c 	ldmdbcs	r8!, {r2, r3, r5, sl, ip, sp}
     f94:	2872613d 	ldmdacs	r2!, {r0, r2, r3, r4, r5, r8, sp, lr}^
     f98:	39342c30 	ldmdbcc	r4!, {r4, r5, sl, fp, sp}
     f9c:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
     fa0:	39342c30 	ldmdbcc	r4!, {r4, r5, sl, fp, sp}
     fa4:	3b303b29 	blcc	c0fc50 <start_address+0xbffc50>
     fa8:	37373330 			; <UNDEFINED> instruction: 0x37373330
     fac:	37373737 			; <UNDEFINED> instruction: 0x37373737
     fb0:	37373737 			; <UNDEFINED> instruction: 0x37373737
     fb4:	3b303b3b 	blcc	c0fca8 <start_address+0xbffca8>
     fb8:	283b3531 	ldmdacs	fp!, {r0, r4, r5, r8, sl, ip, sp}
     fbc:	29322c30 	ldmdbcs	r2!, {r4, r5, sl, fp, sp}
     fc0:	63657200 	cmnvs	r5, #0, 4
     fc4:	65766965 	ldrbvs	r6, [r6, #-2405]!	; 0x965
     fc8:	66667542 	strbtvs	r7, [r6], -r2, asr #10
     fcc:	65487265 	strbvs	r7, [r8, #-613]	; 0x265
     fd0:	533a6461 	teqpl	sl, #1627389952	; 0x61000000
     fd4:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
     fd8:	65720029 	ldrbvs	r0, [r2, #-41]!	; 0x29
     fdc:	76696563 	strbtvc	r6, [r9], -r3, ror #10
     fe0:	66754265 	ldrbtvs	r4, [r5], -r5, ror #4
     fe4:	54726566 	ldrbtpl	r6, [r2], #-1382	; 0x566
     fe8:	3a6c6961 	bcc	1b1b574 <start_address+0x1b0b574>
     fec:	2c302853 	ldccs	8, cr2, [r0], #-332	; 0xfffffeb4
     ff0:	73002931 	movwvc	r2, #2353	; 0x931
     ff4:	6b2f6372 	blvs	bd9dc4 <start_address+0xbc9dc4>
     ff8:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
     ffc:	00632e6c 	rsbeq	r2, r3, ip, ror #28
    1000:	6c636e69 	stclvs	14, cr6, [r3], #-420	; 0xfffffe5c
    1004:	2f656475 	svccs	0x00656475
    1008:	6b2f6d6d 	blvs	bdc5c4 <start_address+0xbcc5c4>
    100c:	6f6c6c61 	svcvs	0x006c6c61
    1010:	00682e63 	rsbeq	r2, r8, r3, ror #28
    1014:	65676150 	strbvs	r6, [r7, #-336]!	; 0x150
    1018:	7473694c 	ldrbtvc	r6, [r3], #-2380	; 0x94c
    101c:	3328543a 			; <UNDEFINED> instruction: 0x3328543a
    1020:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
    1024:	656e3473 	strbvs	r3, [lr, #-1139]!	; 0x473
    1028:	283a7478 	ldmdacs	sl!, {r3, r4, r5, r6, sl, ip, sp, lr}
    102c:	29322c33 	ldmdbcs	r2!, {r0, r1, r4, r5, sl, fp, sp}
    1030:	33282a3d 			; <UNDEFINED> instruction: 0x33282a3d
    1034:	2c29312c 	stfcss	f3, [r9], #-176	; 0xffffff50
    1038:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    103c:	50003b3b 	andpl	r3, r0, fp, lsr fp
    1040:	4c656761 	stclmi	7, cr6, [r5], #-388	; 0xfffffe7c
    1044:	54747369 	ldrbtpl	r7, [r4], #-873	; 0x369
    1048:	3328743a 			; <UNDEFINED> instruction: 0x3328743a
    104c:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
    1050:	312c3328 			; <UNDEFINED> instruction: 0x312c3328
    1054:	6e690029 	cdpvs	0, 6, cr0, cr9, cr9, {1}
    1058:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
    105c:	656b2f65 	strbvs	r2, [fp, #-3941]!	; 0xf65
    1060:	6c656e72 	stclvs	14, cr6, [r5], #-456	; 0xfffffe38
    1064:	6900682e 	stmdbvs	r0, {r1, r2, r3, r5, fp, sp, lr}
    1068:	756c636e 	strbvc	r6, [ip, #-878]!	; 0x36e
    106c:	732f6564 			; <UNDEFINED> instruction: 0x732f6564
    1070:	646d6172 	strbtvs	r6, [sp], #-370	; 0x172
    1074:	2e6b7369 	cdpcs	3, 6, cr7, cr11, cr9, {3}
    1078:	61520068 	cmpvs	r2, r8, rrx
    107c:	6c69466d 	stclvs	6, cr4, [r9], #-436	; 0xfffffe4c
    1080:	28543a65 	ldmdacs	r4, {r0, r2, r5, r6, r9, fp, ip, sp}^
    1084:	29312c35 	ldmdbcs	r1!, {r0, r2, r4, r5, sl, fp, sp}
    1088:	3632733d 			; <UNDEFINED> instruction: 0x3632733d
    108c:	78656e38 	stmdavc	r5!, {r3, r4, r5, r9, sl, fp, sp, lr}^
    1090:	35283a74 	strcc	r3, [r8, #-2676]!	; 0xa74
    1094:	3d29322c 	sfmcc	f3, 4, [r9, #-176]!	; 0xffffff50
    1098:	2c35282a 	ldccs	8, cr2, [r5], #-168	; 0xffffff58
    109c:	302c2931 	eorcc	r2, ip, r1, lsr r9
    10a0:	3b32332c 	blcc	c8dd58 <start_address+0xc7dd58>
    10a4:	656d616e 	strbvs	r6, [sp, #-366]!	; 0x16e
    10a8:	2c35283a 	ldccs	8, cr2, [r5], #-232	; 0xffffff18
    10ac:	613d2933 	teqvs	sp, r3, lsr r9
    10b0:	2c352872 	ldccs	8, cr2, [r5], #-456	; 0xfffffe38
    10b4:	723d2934 	eorsvc	r2, sp, #52, 18	; 0xd0000
    10b8:	342c3528 	strtcc	r3, [ip], #-1320	; 0x528
    10bc:	3b303b29 	blcc	c0fd68 <start_address+0xbffd68>
    10c0:	37373330 			; <UNDEFINED> instruction: 0x37373330
    10c4:	37373737 			; <UNDEFINED> instruction: 0x37373737
    10c8:	37373737 			; <UNDEFINED> instruction: 0x37373737
    10cc:	3b303b3b 	blcc	c0fdc0 <start_address+0xbffdc0>
    10d0:	3b353532 	blcc	d4e5a0 <start_address+0xd3e5a0>
    10d4:	322c3028 	eorcc	r3, ip, #40	; 0x28
    10d8:	32332c29 	eorscc	r2, r3, #10496	; 0x2900
    10dc:	3430322c 	ldrtcc	r3, [r0], #-556	; 0x22c
    10e0:	6f633b38 	svcvs	0x00633b38
    10e4:	6e65746e 	cdpvs	4, 6, cr7, cr5, cr14, {3}
    10e8:	35283a74 	strcc	r3, [r8, #-2676]!	; 0xa74
    10ec:	3d29352c 	cfstr32cc	mvfx3, [r9, #-176]!	; 0xffffff50
    10f0:	2c35282a 	ldccs	8, cr2, [r5], #-168	; 0xffffff58
    10f4:	6b3d2936 	blvs	f4b5d4 <start_address+0xf3b5d4>
    10f8:	322c3028 	eorcc	r3, ip, #40	; 0x28
    10fc:	30322c29 	eorscc	r2, r2, r9, lsr #24
    1100:	332c3038 			; <UNDEFINED> instruction: 0x332c3038
    1104:	69733b32 	ldmdbvs	r3!, {r1, r4, r5, r8, r9, fp, ip, sp}^
    1108:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    110c:	29362c32 	ldmdbcs	r6!, {r1, r4, r5, sl, fp, sp}
    1110:	3131322c 	teqcc	r1, ip, lsr #4
    1114:	32332c32 	eorscc	r2, r3, #12800	; 0x3200
    1118:	52003b3b 	andpl	r3, r0, #60416	; 0xec00
    111c:	69466d61 	stmdbvs	r6, {r0, r5, r6, r8, sl, fp, sp, lr}^
    1120:	3a54656c 	bcc	151a6d8 <start_address+0x150a6d8>
    1124:	2c352874 	ldccs	8, cr2, [r5], #-464	; 0xfffffe30
    1128:	283d2937 	ldmdacs	sp!, {r0, r1, r2, r4, r5, r8, fp, sp}
    112c:	29312c35 	ldmdbcs	r1!, {r0, r2, r4, r5, sl, fp, sp}
    1130:	6d615200 	sfmvs	f5, 2, [r1, #-0]
    1134:	6b736944 	blvs	1cdb64c <start_address+0x1ccb64c>
    1138:	28743a54 	ldmdacs	r4!, {r2, r4, r6, r9, fp, ip, sp}^
    113c:	29382c35 	ldmdbcs	r8!, {r0, r2, r4, r5, sl, fp, sp}
    1140:	2c35283d 	ldccs	8, cr2, [r5], #-244	; 0xffffff0c
    1144:	733d2939 	teqvc	sp, #933888	; 0xe4000
    1148:	61656838 	cmnvs	r5, r8, lsr r8
    114c:	35283a64 	strcc	r3, [r8, #-2660]!	; 0xa64
    1150:	2930312c 	ldmdbcs	r0!, {r2, r3, r5, r8, ip, sp}
    1154:	35282a3d 	strcc	r2, [r8, #-2621]!	; 0xa3d
    1158:	2c29372c 	stccs	7, cr3, [r9], #-176	; 0xffffff50
    115c:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    1160:	6d61723b 	sfmvs	f7, 2, [r1, #-236]!	; 0xffffff14
    1164:	2c35283a 	ldccs	8, cr2, [r5], #-232	; 0xffffff18
    1168:	332c2935 			; <UNDEFINED> instruction: 0x332c2935
    116c:	32332c32 	eorscc	r2, r3, #12800	; 0x3200
    1170:	69003b3b 	stmdbvs	r0, {r0, r1, r3, r4, r5, r8, r9, fp, ip, sp}
    1174:	756c636e 	strbvc	r6, [ip, #-878]!	; 0x36e
    1178:	702f6564 	eorvc	r6, pc, r4, ror #10
    117c:	2e636f72 	mcrcs	15, 3, r6, cr3, cr2, {3}
    1180:	6e690068 	cdpvs	0, 6, cr0, cr9, cr8, {3}
    1184:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
    1188:	6d6d2f65 	stclvs	15, cr2, [sp, #-404]!	; 0xfffffe6c
    118c:	7572742f 	ldrbvc	r7, [r2, #-1071]!	; 0x42f
    1190:	616d6b6e 	cmnvs	sp, lr, ror #22
    1194:	636f6c6c 	cmnvs	pc, #108, 24	; 0x6c00
    1198:	4d00682e 	stcmi	8, cr6, [r0, #-184]	; 0xffffff48
    119c:	6c426d65 	mcrrvs	13, 6, r6, r2, cr5
    11a0:	3a6b636f 	bcc	1ad9f64 <start_address+0x1ac9f64>
    11a4:	2c372854 	ldccs	8, cr2, [r7], #-336	; 0xfffffeb0
    11a8:	733d2931 	teqvc	sp, #802816	; 0xc4000
    11ac:	656e3631 	strbvs	r3, [lr, #-1585]!	; 0x631
    11b0:	283a7478 	ldmdacs	sl!, {r3, r4, r5, r6, sl, ip, sp, lr}
    11b4:	29322c37 	ldmdbcs	r2!, {r0, r1, r2, r4, r5, sl, fp, sp}
    11b8:	37282a3d 			; <UNDEFINED> instruction: 0x37282a3d
    11bc:	2c29312c 	stfcss	f3, [r9], #-176	; 0xffffff50
    11c0:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    11c4:	6572703b 	ldrbvs	r7, [r2, #-59]!	; 0x3b
    11c8:	37283a76 			; <UNDEFINED> instruction: 0x37283a76
    11cc:	2c29322c 	sfmcs	f3, 4, [r9], #-176	; 0xffffff50
    11d0:	332c3233 			; <UNDEFINED> instruction: 0x332c3233
    11d4:	69733b32 	ldmdbvs	r3!, {r1, r4, r5, r8, r9, fp, ip, sp}^
    11d8:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    11dc:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    11e0:	2c34362c 	ldccs	6, cr3, [r4], #-176	; 0xffffff50
    11e4:	753b3133 	ldrvc	r3, [fp, #-307]!	; 0x133
    11e8:	3a646573 	bcc	191a7bc <start_address+0x190a7bc>
    11ec:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    11f0:	35392c29 	ldrcc	r2, [r9, #-3113]!	; 0xc29
    11f4:	6d3b312c 	ldfvss	f3, [fp, #-176]!	; 0xffffff50
    11f8:	283a6d65 	ldmdacs	sl!, {r0, r2, r5, r6, r8, sl, fp, sp, lr}
    11fc:	29332c37 	ldmdbcs	r3!, {r0, r1, r2, r4, r5, sl, fp, sp}
    1200:	30282a3d 	eorcc	r2, r8, sp, lsr sl
    1204:	2c29322c 	sfmcs	f3, 4, [r9], #-176	; 0xffffff50
    1208:	332c3639 			; <UNDEFINED> instruction: 0x332c3639
    120c:	003b3b32 	eorseq	r3, fp, r2, lsr fp
    1210:	426d654d 	rsbmi	r6, sp, #322961408	; 0x13400000
    1214:	6b636f6c 	blvs	18dcfcc <start_address+0x18ccfcc>
    1218:	28743a54 	ldmdacs	r4!, {r2, r4, r6, r9, fp, ip, sp}^
    121c:	29342c37 	ldmdbcs	r4!, {r0, r1, r2, r4, r5, sl, fp, sp}
    1220:	2c37283d 	ldccs	8, cr2, [r7], #-244	; 0xffffff0c
    1224:	4d002931 	stcmi	9, cr2, [r0, #-196]	; 0xffffff3c
    1228:	6f6c6c61 	svcvs	0x006c6c61
    122c:	743a5463 	ldrtvc	r5, [sl], #-1123	; 0x463
    1230:	352c3728 	strcc	r3, [ip, #-1832]!	; 0x728
    1234:	37283d29 	strcc	r3, [r8, -r9, lsr #26]!
    1238:	3d29362c 	stccc	6, cr3, [r9, #-176]!	; 0xffffff50
    123c:	61343273 	teqvs	r4, r3, ror r2
    1240:	283a6772 	ldmdacs	sl!, {r1, r4, r5, r6, r8, r9, sl, sp, lr}
    1244:	29372c37 	ldmdbcs	r7!, {r0, r1, r2, r4, r5, sl, fp, sp}
    1248:	30282a3d 	eorcc	r2, r8, sp, lsr sl
    124c:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    1250:	332c302c 			; <UNDEFINED> instruction: 0x332c302c
    1254:	78653b32 	stmdavc	r5!, {r1, r4, r5, r8, r9, fp, ip, sp}^
    1258:	646e6170 	strbtvs	r6, [lr], #-368	; 0x170
    125c:	2c37283a 	ldccs	8, cr2, [r7], #-232	; 0xffffff18
    1260:	2a3d2938 	bcs	f4b748 <start_address+0xf3b748>
    1264:	392c3728 	stmdbcc	ip!, {r3, r5, r8, r9, sl, ip, sp}
    1268:	28663d29 	stmdacs	r6!, {r0, r3, r5, r8, sl, fp, ip, sp}^
    126c:	29312c32 	ldmdbcs	r1!, {r1, r4, r5, sl, fp, sp}
    1270:	2c32332c 	ldccs	3, cr3, [r2], #-176	; 0xffffff50
    1274:	733b3233 	teqvc	fp, #805306371	; 0x30000003
    1278:	6e697268 	cdpvs	2, 6, cr7, cr9, cr8, {3}
    127c:	37283a6b 	strcc	r3, [r8, -fp, ror #20]!
    1280:	2930312c 	ldmdbcs	r0!, {r2, r3, r5, r8, ip, sp}
    1284:	37282a3d 			; <UNDEFINED> instruction: 0x37282a3d
    1288:	2931312c 	ldmdbcs	r1!, {r2, r3, r5, r8, ip, sp}
    128c:	3028663d 	eorcc	r6, r8, sp, lsr r6
    1290:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    1294:	2c34362c 	ldccs	6, cr3, [r4], #-176	; 0xffffff50
    1298:	673b3233 			; <UNDEFINED> instruction: 0x673b3233
    129c:	654d7465 	strbvs	r7, [sp, #-1125]	; 0x465
    12a0:	6961546d 	stmdbvs	r1!, {r0, r2, r3, r5, r6, sl, ip, lr}^
    12a4:	37283a6c 	strcc	r3, [r8, -ip, ror #20]!
    12a8:	2932312c 	ldmdbcs	r2!, {r2, r3, r5, r8, ip, sp}
    12ac:	37282a3d 			; <UNDEFINED> instruction: 0x37282a3d
    12b0:	2933312c 	ldmdbcs	r3!, {r2, r3, r5, r8, ip, sp}
    12b4:	3728663d 			; <UNDEFINED> instruction: 0x3728663d
    12b8:	2c29372c 	stccs	7, cr3, [r9], #-176	; 0xffffff50
    12bc:	332c3639 			; <UNDEFINED> instruction: 0x332c3639
    12c0:	486d3b32 	stmdami	sp!, {r1, r4, r5, r8, r9, fp, ip, sp}^
    12c4:	3a646165 	bcc	1919860 <start_address+0x1909860>
    12c8:	312c3728 			; <UNDEFINED> instruction: 0x312c3728
    12cc:	2a3d2934 	bcs	f4b7a4 <start_address+0xf3b7a4>
    12d0:	342c3728 	strtcc	r3, [ip], #-1832	; 0x728
    12d4:	32312c29 	eorscc	r2, r1, #10496	; 0x2900
    12d8:	32332c38 	eorscc	r2, r3, #56, 24	; 0x3800
    12dc:	61546d3b 	cmpvs	r4, fp, lsr sp
    12e0:	283a6c69 	ldmdacs	sl!, {r0, r3, r5, r6, sl, fp, sp, lr}
    12e4:	34312c37 	ldrtcc	r2, [r1], #-3127	; 0xc37
    12e8:	36312c29 	ldrtcc	r2, [r1], -r9, lsr #24
    12ec:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    12f0:	69003b3b 	stmdbvs	r0, {r0, r1, r3, r4, r5, r8, r9, fp, ip, sp}
    12f4:	756c636e 	strbvc	r6, [ip, #-878]!	; 0x36e
    12f8:	6b2f6564 	blvs	bda890 <start_address+0xbca890>
    12fc:	7373656d 	cmnvc	r3, #457179136	; 0x1b400000
    1300:	2e656761 	cdpcs	7, 6, cr6, cr5, cr1, {3}
    1304:	6e690068 	cdpvs	0, 6, cr0, cr9, cr8, {3}
    1308:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
    130c:	61702f65 	cmnvs	r0, r5, ror #30
    1310:	67616b63 	strbvs	r6, [r1, -r3, ror #22]!
    1314:	00682e65 	rsbeq	r2, r8, r5, ror #28
    1318:	6b636150 	blvs	18d9860 <start_address+0x18c9860>
    131c:	54656761 	strbtpl	r6, [r5], #-1889	; 0x761
    1320:	3928743a 	stmdbcc	r8!, {r1, r3, r4, r5, sl, ip, sp, lr}
    1324:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
    1328:	322c3928 	eorcc	r3, ip, #40, 18	; 0xa0000
    132c:	31733d29 	cmncc	r3, r9, lsr #26
    1330:	3a646936 	bcc	191b810 <start_address+0x190b810>
    1334:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    1338:	2c302c29 	ldccs	12, cr2, [r0], #-164	; 0xffffff5c
    133c:	703b3233 	eorsvc	r3, fp, r3, lsr r2
    1340:	283a6469 	ldmdacs	sl!, {r0, r3, r5, r6, sl, sp, lr}
    1344:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    1348:	2c32332c 	ldccs	3, cr3, [r2], #-176	; 0xffffff50
    134c:	743b3233 	ldrtvc	r3, [fp], #-563	; 0x233
    1350:	3a657079 	bcc	195d53c <start_address+0x194d53c>
    1354:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    1358:	34362c29 	ldrtcc	r2, [r6], #-3113	; 0xc29
    135c:	3b32332c 	blcc	c8e014 <start_address+0xc7e014>
    1360:	657a6973 	ldrbvs	r6, [sl, #-2419]!	; 0x973
    1364:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    1368:	392c2937 	stmdbcc	ip!, {r0, r1, r2, r4, r5, r8, fp, sp}
    136c:	32332c36 	eorscc	r2, r3, #13824	; 0x3600
    1370:	4b003b3b 	blmi	10064 <start_address+0x64>
    1374:	7373654d 	cmnvc	r3, #322961408	; 0x13400000
    1378:	3a656761 	bcc	195b104 <start_address+0x194b104>
    137c:	2c382854 	ldccs	8, cr2, [r8], #-336	; 0xfffffeb0
    1380:	733d2931 	teqvc	sp, #802816	; 0xc4000
    1384:	656e3231 	strbvs	r3, [lr, #-561]!	; 0x231
    1388:	283a7478 	ldmdacs	sl!, {r3, r4, r5, r6, sl, ip, sp, lr}
    138c:	29322c38 	ldmdbcs	r2!, {r3, r4, r5, sl, fp, sp}
    1390:	38282a3d 	stmdacc	r8!, {r0, r2, r3, r4, r5, r9, fp, sp}
    1394:	2c29312c 	stfcss	f3, [r9], #-176	; 0xffffff50
    1398:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    139c:	6572703b 	ldrbvs	r7, [r2, #-59]!	; 0x3b
    13a0:	38283a76 	stmdacc	r8!, {r1, r2, r4, r5, r6, r9, fp, ip, sp}
    13a4:	2c29322c 	sfmcs	f3, 4, [r9], #-176	; 0xffffff50
    13a8:	332c3233 			; <UNDEFINED> instruction: 0x332c3233
    13ac:	6b703b32 	blvs	1c1007c <start_address+0x1c0007c>
    13b0:	38283a67 	stmdacc	r8!, {r0, r1, r2, r5, r6, r9, fp, ip, sp}
    13b4:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
    13b8:	2c39282a 	ldccs	8, cr2, [r9], #-168	; 0xffffff58
    13bc:	362c2931 			; <UNDEFINED> instruction: 0x362c2931
    13c0:	32332c34 	eorscc	r2, r3, #52, 24	; 0x3400
    13c4:	4b003b3b 	blmi	100b8 <start_address+0xb8>
    13c8:	7373654d 	cmnvc	r3, #322961408	; 0x13400000
    13cc:	54656761 	strbtpl	r6, [r5], #-1889	; 0x761
    13d0:	3828743a 	stmdacc	r8!, {r1, r3, r4, r5, sl, ip, sp, lr}
    13d4:	3d29342c 	cfstrscc	mvf3, [r9, #-176]!	; 0xffffff50
    13d8:	312c3828 			; <UNDEFINED> instruction: 0x312c3828
    13dc:	654d0029 	strbvs	r0, [sp, #-41]	; 0x29
    13e0:	67617373 			; <UNDEFINED> instruction: 0x67617373
    13e4:	65755165 	ldrbvs	r5, [r5, #-357]!	; 0x165
    13e8:	3a546575 	bcc	151a9c4 <start_address+0x150a9c4>
    13ec:	2c382874 	ldccs	8, cr2, [r8], #-464	; 0xfffffe30
    13f0:	283d2935 	ldmdacs	sp!, {r0, r2, r4, r5, r8, fp, sp}
    13f4:	29362c38 	ldmdbcs	r6!, {r3, r4, r5, sl, fp, sp}
    13f8:	6838733d 	ldmdavs	r8!, {r0, r2, r3, r4, r5, r8, r9, ip, sp, lr}
    13fc:	3a646165 	bcc	1919998 <start_address+0x1909998>
    1400:	372c3828 	strcc	r3, [ip, -r8, lsr #16]!
    1404:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    1408:	29342c38 	ldmdbcs	r4!, {r3, r4, r5, sl, fp, sp}
    140c:	332c302c 			; <UNDEFINED> instruction: 0x332c302c
    1410:	61743b32 	cmnvs	r4, r2, lsr fp
    1414:	283a6c69 	ldmdacs	sl!, {r0, r3, r5, r6, sl, fp, sp, lr}
    1418:	29372c38 	ldmdbcs	r7!, {r3, r4, r5, sl, fp, sp}
    141c:	2c32332c 	ldccs	3, cr3, [r2], #-176	; 0xffffff50
    1420:	3b3b3233 	blcc	ecdcf4 <start_address+0xebdcf4>
    1424:	636e6900 	cmnvs	lr, #0, 18
    1428:	6564756c 	strbvs	r7, [r4, #-1388]!	; 0x56c
    142c:	69666b2f 	stmdbvs	r6!, {r0, r1, r2, r3, r5, r8, r9, fp, sp, lr}^
    1430:	682e656c 	stmdavs	lr!, {r2, r3, r5, r6, r8, sl, sp, lr}
    1434:	69464b00 	stmdbvs	r6, {r8, r9, fp, lr}^
    1438:	543a656c 	ldrtpl	r6, [sl], #-1388	; 0x56c
    143c:	2c303128 	ldfcss	f3, [r0], #-160	; 0xffffff60
    1440:	733d2931 	teqvc	sp, #802816	; 0xc4000
    1444:	656e3631 	strbvs	r3, [lr, #-1585]!	; 0x631
    1448:	283a7478 	ldmdacs	sl!, {r3, r4, r5, r6, sl, ip, sp, lr}
    144c:	322c3031 	eorcc	r3, ip, #49	; 0x31
    1450:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    1454:	312c3031 			; <UNDEFINED> instruction: 0x312c3031
    1458:	2c302c29 	ldccs	12, cr2, [r0], #-164	; 0xffffff5c
    145c:	703b3233 	eorsvc	r3, fp, r3, lsr r2
    1460:	3a766572 	bcc	1d9aa30 <start_address+0x1d8aa30>
    1464:	2c303128 	ldfcss	f3, [r0], #-160	; 0xffffff60
    1468:	332c2932 			; <UNDEFINED> instruction: 0x332c2932
    146c:	32332c32 	eorscc	r2, r3, #12800	; 0x3200
    1470:	4e73663b 	mrcmi	6, 3, r6, cr3, cr11, {1}
    1474:	4165646f 	cmnmi	r5, pc, ror #8
    1478:	3a726464 	bcc	1c9a610 <start_address+0x1c8a610>
    147c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    1480:	34362c29 	ldrtcc	r2, [r6], #-3113	; 0xc29
    1484:	3b32332c 	blcc	c8e13c <start_address+0xc7e13c>
    1488:	52666572 	rsbpl	r6, r6, #478150656	; 0x1c800000
    148c:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    1490:	392c2935 	stmdbcc	ip!, {r0, r2, r4, r5, r8, fp, sp}
    1494:	36312c36 			; <UNDEFINED> instruction: 0x36312c36
    1498:	6665723b 			; <UNDEFINED> instruction: 0x6665723b
    149c:	32283a57 	eorcc	r3, r8, #356352	; 0x57000
    14a0:	2c29352c 	cfstr32cs	mvfx3, [r9], #-176	; 0xffffff50
    14a4:	2c323131 	ldfcss	f3, [r2], #-196	; 0xffffff3c
    14a8:	3b3b3631 	blcc	eced74 <start_address+0xebed74>
    14ac:	69464b00 	stmdbvs	r6, {r8, r9, fp, lr}^
    14b0:	3a54656c 	bcc	151aa68 <start_address+0x150aa68>
    14b4:	30312874 	eorscc	r2, r1, r4, ror r8
    14b8:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
    14bc:	2c303128 	ldfcss	f3, [r0], #-160	; 0xffffff60
    14c0:	69002931 	stmdbvs	r0, {r0, r4, r5, r8, fp, sp}
    14c4:	756c636e 	strbvc	r6, [ip, #-878]!	; 0x36e
    14c8:	702f6564 	eorvc	r6, pc, r4, ror #10
    14cc:	69636f72 	stmdbvs	r3!, {r1, r4, r5, r6, r8, r9, sl, fp, sp, lr}^
    14d0:	2e6f666e 	cdpcs	6, 6, cr6, cr15, cr14, {3}
    14d4:	72500068 	subsvc	r0, r0, #104	; 0x68
    14d8:	6e49636f 	cdpvs	3, 4, cr6, cr9, cr15, {3}
    14dc:	3a546f66 	bcc	151d27c <start_address+0x150d27c>
    14e0:	31312874 	teqcc	r1, r4, ror r8
    14e4:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
    14e8:	2c313128 	ldfcss	f3, [r1], #-160	; 0xffffff60
    14ec:	733d2932 	teqvc	sp, #819200	; 0xc8000
    14f0:	70343431 	eorsvc	r3, r4, r1, lsr r4
    14f4:	283a6469 	ldmdacs	sl!, {r0, r3, r5, r6, sl, sp, lr}
    14f8:	29362c32 	ldmdbcs	r6!, {r1, r4, r5, sl, fp, sp}
    14fc:	332c302c 			; <UNDEFINED> instruction: 0x332c302c
    1500:	61663b32 	cmnvs	r6, r2, lsr fp
    1504:	72656874 	rsbvc	r6, r5, #116, 16	; 0x740000
    1508:	3a646950 	bcc	191ba50 <start_address+0x190ba50>
    150c:	362c3228 	strtcc	r3, [ip], -r8, lsr #4
    1510:	32332c29 	eorscc	r2, r3, #10496	; 0x2900
    1514:	3b32332c 	blcc	c8e1cc <start_address+0xc7e1cc>
    1518:	656e776f 	strbvs	r7, [lr, #-1903]!	; 0x76f
    151c:	32283a72 	eorcc	r3, r8, #466944	; 0x72000
    1520:	2c29362c 	stccs	6, cr3, [r9], #-176	; 0xffffff50
    1524:	332c3436 			; <UNDEFINED> instruction: 0x332c3436
    1528:	65683b32 	strbvs	r3, [r8, #-2866]!	; 0xb32
    152c:	69537061 	ldmdbvs	r3, {r0, r5, r6, ip, sp, lr}^
    1530:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    1534:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    1538:	2c36392c 	ldccs	9, cr3, [r6], #-176	; 0xffffff50
    153c:	633b3233 	teqvs	fp, #805306371	; 0x30000003
    1540:	283a646d 	ldmdacs	sl!, {r0, r2, r3, r5, r6, sl, sp, lr}
    1544:	332c3131 			; <UNDEFINED> instruction: 0x332c3131
    1548:	72613d29 	rsbvc	r3, r1, #2624	; 0xa40
    154c:	342c3528 	strtcc	r3, [ip], #-1320	; 0x528
    1550:	3b303b29 	blcc	c101fc <start_address+0xc001fc>
    1554:	3b373231 	blcc	dcde20 <start_address+0xdbde20>
    1558:	322c3028 	eorcc	r3, ip, #40	; 0x28
    155c:	32312c29 	eorscc	r2, r1, #10496	; 0x2900
    1560:	30312c38 	eorscc	r2, r1, r8, lsr ip
    1564:	3b3b3432 	blcc	ece634 <start_address+0xebe634>
    1568:	746e4500 	strbtvc	r4, [lr], #-1280	; 0x500
    156c:	75467972 	strbvc	r7, [r6, #-2418]	; 0x972
    1570:	6974636e 	ldmdbvs	r4!, {r1, r2, r3, r5, r6, r8, r9, sp, lr}^
    1574:	3a546e6f 	bcc	151cf38 <start_address+0x150cf38>
    1578:	2c362874 	ldccs	8, cr2, [r6], #-464	; 0xfffffe30
    157c:	283d2931 	ldmdacs	sp!, {r0, r4, r5, r8, fp, sp}
    1580:	29322c36 	ldmdbcs	r2!, {r1, r2, r4, r5, sl, fp, sp}
    1584:	36282a3d 			; <UNDEFINED> instruction: 0x36282a3d
    1588:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
    158c:	2c302866 	ldccs	8, cr2, [r0], #-408	; 0xfffffe68
    1590:	00293534 	eoreq	r3, r9, r4, lsr r5
    1594:	636f7250 	cmnvs	pc, #80, 4
    1598:	53737365 	cmnpl	r3, #-1811939327	; 0x94000001
    159c:	65746174 	ldrbvs	r6, [r4, #-372]!	; 0x174
    15a0:	3628543a 			; <UNDEFINED> instruction: 0x3628543a
    15a4:	3d29342c 	cfstrscc	mvf3, [r9, #-176]!	; 0xffffff50
    15a8:	3b387340 	blcc	e1e2b0 <start_address+0xe0e2b0>
    15ac:	554e5565 	strbpl	r5, [lr, #-1381]	; 0x565
    15b0:	3a444553 	bcc	1112b04 <start_address+0x1102b04>
    15b4:	52432c30 	subpl	r2, r3, #48, 24	; 0x3000
    15b8:	45544145 	ldrbmi	r4, [r4, #-325]	; 0x145
    15bc:	2c313a44 	ldccs	10, cr3, [r1], #-272	; 0xfffffef0
    15c0:	45454c53 	strbmi	r4, [r5, #-3155]	; 0xc53
    15c4:	474e4950 	smlsldmi	r4, lr, r0, r9
    15c8:	522c323a 	eorpl	r3, ip, #-1610612733	; 0xa0000003
    15cc:	59444145 	stmdbpl	r4, {r0, r2, r6, r8, lr}^
    15d0:	522c333a 	eorpl	r3, ip, #-402653184	; 0xe8000000
    15d4:	494e4e55 	stmdbmi	lr, {r0, r2, r4, r6, r9, sl, fp, lr}^
    15d8:	343a474e 	ldrtcc	r4, [sl], #-1870	; 0x74e
    15dc:	5245542c 	subpl	r5, r5, #44, 8	; 0x2c000000
    15e0:	414e494d 	cmpmi	lr, sp, asr #18
    15e4:	3a444554 	bcc	1112b3c <start_address+0x1102b3c>
    15e8:	003b2c35 	eorseq	r2, fp, r5, lsr ip
    15ec:	746e6f43 	strbtvc	r6, [lr], #-3907	; 0xf43
    15f0:	49747865 	ldmdbmi	r4!, {r0, r2, r5, r6, fp, ip, sp, lr}^
    15f4:	3a6d6574 	bcc	1b5abcc <start_address+0x1b4abcc>
    15f8:	2c362854 	ldccs	8, cr2, [r6], #-336	; 0xfffffeb0
    15fc:	403d2935 	eorsmi	r2, sp, r5, lsr r9
    1600:	653b3873 	ldrvs	r3, [fp, #-2163]!	; 0x873
    1604:	52535043 	subspl	r5, r3, #67	; 0x43
    1608:	522c303a 	eorpl	r3, ip, #58	; 0x3a
    160c:	41545345 	cmpmi	r4, r5, asr #6
    1610:	415f5452 	cmpmi	pc, r2, asr r4	; <UNPREDICTABLE>
    1614:	3a524444 	bcc	149272c <start_address+0x148272c>
    1618:	30522c31 	subscc	r2, r2, r1, lsr ip
    161c:	522c323a 	eorpl	r3, ip, #-1610612733	; 0xa0000003
    1620:	2c333a31 	ldccs	10, cr3, [r3], #-196	; 0xffffff3c
    1624:	343a3252 	ldrtcc	r3, [sl], #-594	; 0x252
    1628:	3a33522c 	bcc	cd5ee0 <start_address+0xcc5ee0>
    162c:	34522c35 	ldrbcc	r2, [r2], #-3125	; 0xc35
    1630:	522c363a 	eorpl	r3, ip, #60817408	; 0x3a00000
    1634:	2c373a35 	ldccs	10, cr3, [r7], #-212	; 0xffffff2c
    1638:	383a3652 	ldmdacc	sl!, {r1, r4, r6, r9, sl, ip, sp}
    163c:	3a37522c 	bcc	dd5ef4 <start_address+0xdc5ef4>
    1640:	38522c39 	ldmdacc	r2, {r0, r3, r4, r5, sl, fp, sp}^
    1644:	2c30313a 	ldfcss	f3, [r0], #-232	; 0xffffff18
    1648:	313a3952 	teqcc	sl, r2, asr r9
    164c:	31522c31 	cmpcc	r2, r1, lsr ip
    1650:	32313a30 	eorscc	r3, r1, #48, 20	; 0x30000
    1654:	3131522c 	teqcc	r1, ip, lsr #4
    1658:	2c33313a 	ldfcss	f3, [r3], #-232	; 0xffffff18
    165c:	3a323152 	bcc	c8dbac <start_address+0xc7dbac>
    1660:	532c3431 			; <UNDEFINED> instruction: 0x532c3431
    1664:	35313a50 	ldrcc	r3, [r1, #-2640]!	; 0xa50
    1668:	3a524c2c 	bcc	1494720 <start_address+0x1484720>
    166c:	3b2c3631 	blcc	b0ef38 <start_address+0xafef38>
    1670:	6f725000 	svcvs	0x00725000
    1674:	6c694663 	stclvs	6, cr4, [r9], #-396	; 0xfffffe74
    1678:	743a5465 	ldrtvc	r5, [sl], #-1125	; 0x465
    167c:	362c3628 	strtcc	r3, [ip], -r8, lsr #12
    1680:	36283d29 	strtcc	r3, [r8], -r9, lsr #26
    1684:	3d29372c 	stccc	7, cr3, [r9, #-176]!	; 0xffffff50
    1688:	6b323173 	blvs	c8dc5c <start_address+0xc7dc5c>
    168c:	36283a66 	strtcc	r3, [r8], -r6, ror #20
    1690:	3d29382c 	stccc	8, cr3, [r9, #-176]!	; 0xffffff50
    1694:	3031282a 	eorscc	r2, r1, sl, lsr #16
    1698:	2c29332c 	stccs	3, cr3, [r9], #-176	; 0xffffff50
    169c:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    16a0:	616c663b 	cmnvs	ip, fp, lsr r6
    16a4:	283a7367 	ldmdacs	sl!, {r0, r1, r2, r5, r6, r8, r9, ip, sp, lr}
    16a8:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    16ac:	2c32332c 	ldccs	3, cr3, [r2], #-176	; 0xffffff50
    16b0:	733b3233 	teqvc	fp, #805306371	; 0x30000003
    16b4:	3a6b6565 	bcc	1adac50 <start_address+0x1acac50>
    16b8:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    16bc:	34362c29 	ldrtcc	r2, [r6], #-3113	; 0xc29
    16c0:	3b32332c 	blcc	c8e378 <start_address+0xc7e378>
    16c4:	7250003b 	subsvc	r0, r0, #59	; 0x3b
    16c8:	7365636f 	cmnvc	r5, #-1140850687	; 0xbc000001
    16cc:	743a5473 	ldrtvc	r5, [sl], #-1139	; 0x473
    16d0:	392c3628 	stmdbcc	ip!, {r3, r5, r9, sl, ip, sp}
    16d4:	36283d29 	strtcc	r3, [r8], -r9, lsr #26
    16d8:	2930312c 	ldmdbcs	r0!, {r2, r3, r5, r8, ip, sp}
    16dc:	3139733d 	teqcc	r9, sp, lsr r3
    16e0:	61747332 	cmnvs	r4, r2, lsr r3
    16e4:	283a6574 	ldmdacs	sl!, {r2, r4, r5, r6, r8, sl, sp, lr}
    16e8:	29342c36 	ldmdbcs	r4!, {r1, r2, r4, r5, sl, fp, sp}
    16ec:	382c302c 	stmdacc	ip!, {r2, r3, r5, ip, sp}
    16f0:	6469703b 	strbtvs	r7, [r9], #-59	; 0x3b
    16f4:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    16f8:	332c2936 			; <UNDEFINED> instruction: 0x332c2936
    16fc:	32332c32 	eorscc	r2, r3, #12800	; 0x3200
    1700:	7461663b 	strbtvc	r6, [r1], #-1595	; 0x63b
    1704:	50726568 	rsbspl	r6, r2, r8, ror #10
    1708:	283a6469 	ldmdacs	sl!, {r0, r3, r5, r6, sl, sp, lr}
    170c:	29362c32 	ldmdbcs	r6!, {r1, r4, r5, sl, fp, sp}
    1710:	2c34362c 	ldccs	6, cr3, [r4], #-176	; 0xffffff50
    1714:	6f3b3233 	svcvs	0x003b3233
    1718:	72656e77 	rsbvc	r6, r5, #1904	; 0x770
    171c:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    1720:	392c2936 	stmdbcc	ip!, {r1, r2, r4, r5, r8, fp, sp}
    1724:	32332c36 	eorscc	r2, r3, #13824	; 0x3600
    1728:	646d633b 	strbtvs	r6, [sp], #-827	; 0x33b
    172c:	3131283a 	teqcc	r1, sl, lsr r8
    1730:	2c29332c 	stccs	3, cr3, [r9], #-176	; 0xffffff50
    1734:	2c383231 	lfmcs	f3, 4, [r8], #-196	; 0xffffff3c
    1738:	34323031 	ldrtcc	r3, [r2], #-49	; 0x31
    173c:	6477703b 	ldrbtvs	r7, [r7], #-59	; 0x3b
    1740:	2c35283a 	ldccs	8, cr2, [r5], #-232	; 0xffffff18
    1744:	312c2933 			; <UNDEFINED> instruction: 0x312c2933
    1748:	2c323531 	cfldr32cs	mvfx3, [r2], #-196	; 0xffffff3c
    174c:	38343032 	ldmdacc	r4!, {r1, r4, r5, ip, sp}
    1750:	746e653b 	strbtvc	r6, [lr], #-1339	; 0x53b
    1754:	283a7972 	ldmdacs	sl!, {r1, r4, r5, r6, r8, fp, ip, sp, lr}
    1758:	29312c36 	ldmdbcs	r1!, {r1, r2, r4, r5, sl, fp, sp}
    175c:	3032332c 	eorscc	r3, r2, ip, lsr #6
    1760:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    1764:	3a6d763b 	bcc	1b5f058 <start_address+0x1b4f058>
    1768:	312c3628 			; <UNDEFINED> instruction: 0x312c3628
    176c:	2a3d2931 	bcs	f4bc38 <start_address+0xf3bc38>
    1770:	312c3128 			; <UNDEFINED> instruction: 0x312c3128
    1774:	32332c29 	eorscc	r2, r3, #10496	; 0x2900
    1778:	332c3233 			; <UNDEFINED> instruction: 0x332c3233
    177c:	65683b32 	strbvs	r3, [r8, #-2866]!	; 0xb32
    1780:	69537061 	ldmdbvs	r3, {r0, r5, r6, ip, sp, lr}^
    1784:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    1788:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    178c:	3632332c 	ldrtcc	r3, [r2], -ip, lsr #6
    1790:	32332c34 	eorscc	r2, r3, #52, 24	; 0x3400
    1794:	6573753b 	ldrbvs	r7, [r3, #-1339]!	; 0x53b
    1798:	61745372 	cmnvs	r4, r2, ror r3
    179c:	283a6b63 	ldmdacs	sl!, {r0, r1, r5, r6, r8, r9, fp, sp, lr}
    17a0:	29332c37 	ldmdbcs	r3!, {r0, r1, r2, r4, r5, sl, fp, sp}
    17a4:	3932332c 	ldmdbcc	r2!, {r2, r3, r5, r8, r9, ip, sp}
    17a8:	32332c36 	eorscc	r2, r3, #13824	; 0x3600
    17ac:	72656b3b 	rsbvc	r6, r5, #60416	; 0xec00
    17b0:	536c656e 	cmnpl	ip, #461373440	; 0x1b800000
    17b4:	6b636174 	blvs	18d9d8c <start_address+0x18c9d8c>
    17b8:	2c37283a 	ldccs	8, cr2, [r7], #-232	; 0xffffff18
    17bc:	332c2933 			; <UNDEFINED> instruction: 0x332c2933
    17c0:	2c383233 	lfmcs	f3, 4, [r8], #-204	; 0xffffff34
    17c4:	633b3233 	teqvs	fp, #805306371	; 0x30000003
    17c8:	65746e6f 	ldrbvs	r6, [r4, #-3695]!	; 0xe6f
    17cc:	283a7478 	ldmdacs	sl!, {r3, r4, r5, r6, sl, ip, sp, lr}
    17d0:	32312c36 	eorscc	r2, r1, #13824	; 0x3600
    17d4:	72613d29 	rsbvc	r3, r1, #2624	; 0xa40
    17d8:	342c3528 	strtcc	r3, [ip], #-1320	; 0x528
    17dc:	3b303b29 	blcc	c10488 <start_address+0xc00488>
    17e0:	283b3631 	ldmdacs	fp!, {r0, r4, r5, r9, sl, ip, sp}
    17e4:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    17e8:	3633332c 	ldrtcc	r3, [r3], -ip, lsr #6
    17ec:	34352c30 	ldrtcc	r2, [r5], #-3120	; 0xc30
    17f0:	61773b34 	cmnvs	r7, r4, lsr fp
    17f4:	69507469 	ldmdbvs	r0, {r0, r3, r5, r6, sl, ip, sp, lr}^
    17f8:	30283a64 	eorcc	r3, r8, r4, ror #20
    17fc:	2c29312c 	stfcss	f3, [r9], #-176	; 0xffffff50
    1800:	34303933 	ldrtcc	r3, [r0], #-2355	; 0x933
    1804:	3b32332c 	blcc	c8e4bc <start_address+0xc7e4bc>
    1808:	6c696863 	stclvs	8, cr6, [r9], #-396	; 0xfffffe74
    180c:	74655264 	strbtvc	r5, [r5], #-612	; 0x264
    1810:	566e7275 			; <UNDEFINED> instruction: 0x566e7275
    1814:	65756c61 	ldrbvs	r6, [r5, #-3169]!	; 0xc61
    1818:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    181c:	332c2931 			; <UNDEFINED> instruction: 0x332c2931
    1820:	2c363339 	ldccs	3, cr3, [r6], #-228	; 0xffffff1c
    1824:	6d3b3233 	lfmvs	f3, 4, [fp, #-204]!	; 0xffffff34
    1828:	6f6c6c61 	svcvs	0x006c6c61
    182c:	6e614d63 	cdpvs	13, 6, cr4, cr1, cr3, {3}
    1830:	2c37283a 	ldccs	8, cr2, [r7], #-232	; 0xffffff18
    1834:	332c2935 			; <UNDEFINED> instruction: 0x332c2935
    1838:	2c383639 	ldccs	6, cr3, [r8], #-228	; 0xffffff1c
    183c:	3b323931 	blcc	c8fd08 <start_address+0xc7fd08>
    1840:	7373656d 	cmnvc	r3, #457179136	; 0x1b400000
    1844:	51656761 	cmnpl	r5, r1, ror #14
    1848:	65756575 	ldrbvs	r6, [r5, #-1397]!	; 0x575
    184c:	2c38283a 	ldccs	8, cr2, [r8], #-232	; 0xffffff18
    1850:	342c2935 	strtcc	r2, [ip], #-2357	; 0x935
    1854:	2c303631 	ldccs	6, cr3, [r0], #-196	; 0xffffff3c
    1858:	663b3436 			; <UNDEFINED> instruction: 0x663b3436
    185c:	73656c69 	cmnvc	r5, #26880	; 0x6900
    1860:	2c36283a 	ldccs	8, cr2, [r6], #-232	; 0xffffff18
    1864:	3d293331 	stccc	3, cr3, [r9, #-196]!	; 0xffffff3c
    1868:	35287261 	strcc	r7, [r8, #-609]!	; 0x261
    186c:	3b29342c 	blcc	a4e924 <start_address+0xa3e924>
    1870:	31333b30 	teqcc	r3, r0, lsr fp
    1874:	2c36283b 	ldccs	8, cr2, [r6], #-236	; 0xffffff14
    1878:	342c2936 	strtcc	r2, [ip], #-2358	; 0x936
    187c:	2c343232 	lfmcs	f3, 4, [r4], #-200	; 0xffffff38
    1880:	32373033 	eorscc	r3, r7, #51	; 0x33
    1884:	49003b3b 	stmdbmi	r0, {r0, r1, r3, r4, r5, r8, r9, fp, ip, sp}
    1888:	7265746e 	rsbvc	r7, r5, #1845493760	; 0x6e000000
    188c:	74707572 	ldrbtvc	r7, [r0], #-1394	; 0x572
    1890:	646e6148 	strbtvs	r6, [lr], #-328	; 0x148
    1894:	5472656c 	ldrbtpl	r6, [r2], #-1388	; 0x56c
    1898:	3128743a 			; <UNDEFINED> instruction: 0x3128743a
    189c:	29312c32 	ldmdbcs	r1!, {r1, r4, r5, sl, fp, sp}
    18a0:	2c36283d 	ldccs	8, cr2, [r6], #-244	; 0xffffff0c
    18a4:	69002932 	stmdbvs	r0, {r1, r4, r5, r8, fp, sp}
    18a8:	4b74696e 	blmi	1d1be68 <start_address+0x1d0be68>
    18ac:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
    18b0:	3a4d566c 	bcc	1357268 <start_address+0x1347268>
    18b4:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    18b8:	00293534 	eoreq	r3, r9, r4, lsr r5
    18bc:	646e656b 	strbtvs	r6, [lr], #-1387	; 0x56b
    18c0:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    18c4:	73002934 	movwvc	r2, #2356	; 0x934
    18c8:	654b7465 	strbvs	r7, [fp, #-1125]	; 0x465
    18cc:	6c656e72 	stclvs	14, cr6, [r5], #-456	; 0xfffffe38
    18d0:	463a4d56 			; <UNDEFINED> instruction: 0x463a4d56
    18d4:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    18d8:	76002935 			; <UNDEFINED> instruction: 0x76002935
    18dc:	28703a6d 	ldmdacs	r0!, {r0, r2, r3, r5, r6, r9, fp, ip, sp}^
    18e0:	31312c36 	teqcc	r1, r6, lsr ip
    18e4:	656b0029 	strbvs	r0, [fp, #-41]!	; 0x29
    18e8:	6c656e72 	stclvs	14, cr6, [r5], #-456	; 0xfffffe38
    18ec:	72746e45 	rsbsvc	r6, r4, #1104	; 0x450
    18f0:	28463a79 	stmdacs	r6, {r0, r3, r4, r5, r6, r9, fp, ip, sp}^
    18f4:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    18f8:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    18fc:	563a636f 	ldrtpl	r6, [sl], -pc, ror #6
    1900:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    1904:	2a3d2936 	bcs	f4bde4 <start_address+0xf3bde4>
    1908:	392c3628 	stmdbcc	ip!, {r3, r5, r9, sl, ip, sp}
    190c:	69730029 	ldmdbvs	r3!, {r0, r3, r5}^
    1910:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    1914:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    1918:	283a7000 	ldmdacs	sl!, {ip, sp, lr}
    191c:	29352c35 	ldmdbcs	r5!, {r0, r2, r4, r5, sl, fp, sp}
    1920:	474b5000 	strbmi	r5, [fp, -r0]
    1924:	5059545f 	subspl	r5, r9, pc, asr r4
    1928:	52455f45 	subpl	r5, r5, #276	; 0x114
    192c:	28533a52 	ldmdacs	r3, {r1, r4, r6, r9, fp, ip, sp}^
    1930:	37342c30 			; <UNDEFINED> instruction: 0x37342c30
    1934:	286b3d29 	stmdacs	fp!, {r0, r3, r5, r8, sl, fp, ip, sp}^
    1938:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    193c:	6e695f00 	cdpvs	15, 6, cr5, cr9, cr0, {0}
    1940:	61527469 	cmpvs	r2, r9, ror #8
    1944:	7369446d 	cmnvc	r9, #1828716544	; 0x6d000000
    1948:	7361426b 	cmnvc	r1, #-1342177274	; 0xb0000006
    194c:	28473a65 	stmdacs	r7, {r0, r2, r5, r6, r9, fp, ip, sp}^
    1950:	29332c37 	ldmdbcs	r3!, {r0, r1, r2, r4, r5, sl, fp, sp}
    1954:	6e695f00 	cdpvs	15, 6, cr5, cr9, cr0, {0}
    1958:	61527469 	cmpvs	r2, r9, ror #8
    195c:	7369446d 	cmnvc	r9, #1828716544	; 0x6d000000
    1960:	28473a6b 	stmdacs	r7, {r0, r1, r3, r5, r6, r9, fp, ip, sp}^
    1964:	29382c35 	ldmdbcs	r8!, {r0, r2, r4, r5, sl, fp, sp}
    1968:	656b5f00 	strbvs	r5, [fp, #-3840]!	; 0xf00
    196c:	6c656e72 	stclvs	14, cr6, [r5], #-456	; 0xfffffe38
    1970:	473a4d56 			; <UNDEFINED> instruction: 0x473a4d56
    1974:	312c3628 			; <UNDEFINED> instruction: 0x312c3628
    1978:	73002931 	movwvc	r2, #2353	; 0x931
    197c:	692f6372 	stmdbvs	pc!, {r1, r4, r5, r6, r8, r9, sp, lr}	; <UNPREDICTABLE>
    1980:	632e7172 			; <UNDEFINED> instruction: 0x632e7172
    1984:	67657200 	strbvs	r7, [r5, -r0, lsl #4]!
    1988:	65747369 	ldrbvs	r7, [r4, #-873]!	; 0x369
    198c:	746e4972 	strbtvc	r4, [lr], #-2418	; 0x972
    1990:	75727265 	ldrbvc	r7, [r2, #-613]!	; 0x265
    1994:	61487470 	hvcvs	34624	; 0x8740
    1998:	656c646e 	strbvs	r6, [ip, #-1134]!	; 0x46e
    199c:	28463a72 	stmdacs	r6, {r1, r4, r5, r6, r9, fp, ip, sp}^
    19a0:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    19a4:	61680029 	cmnvs	r8, r9, lsr #32
    19a8:	656c646e 	strbvs	r6, [ip, #-1134]!	; 0x46e
    19ac:	28703a72 	ldmdacs	r0!, {r1, r4, r5, r6, r9, fp, ip, sp}^
    19b0:	29312c31 	ldmdbcs	r1!, {r0, r4, r5, sl, fp, sp}
    19b4:	6e616800 	cdpvs	8, 6, cr6, cr1, cr0, {0}
    19b8:	49656c64 	stmdbmi	r5!, {r2, r5, r6, sl, fp, sp, lr}^
    19bc:	463a5152 			; <UNDEFINED> instruction: 0x463a5152
    19c0:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    19c4:	68002935 	stmdavs	r0, {r0, r2, r4, r5, r8, fp, sp}
    19c8:	6c646e61 	stclvs	14, cr6, [r4], #-388	; 0xfffffe7c
    19cc:	283a7265 	ldmdacs	sl!, {r0, r2, r5, r6, r9, ip, sp, lr}
    19d0:	29312c31 	ldmdbcs	r1!, {r0, r4, r5, sl, fp, sp}
    19d4:	6e657000 	cdpvs	0, 6, cr7, cr5, cr0, {0}
    19d8:	676e6964 	strbvs	r6, [lr, -r4, ror #18]!
    19dc:	73515249 	cmpvc	r1, #-1879048188	; 0x90000004
    19e0:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    19e4:	3d293634 	stccc	6, cr3, [r9, #-208]!	; 0xffffff30
    19e8:	30287261 	eorcc	r7, r8, r1, ror #4
    19ec:	2937342c 	ldmdbcs	r7!, {r2, r3, r5, sl, ip, sp}
    19f0:	3028723d 	eorcc	r7, r8, sp, lsr r2
    19f4:	2937342c 	ldmdbcs	r7!, {r2, r3, r5, sl, ip, sp}
    19f8:	303b303b 	eorscc	r3, fp, fp, lsr r0
    19fc:	37373733 			; <UNDEFINED> instruction: 0x37373733
    1a00:	37373737 			; <UNDEFINED> instruction: 0x37373737
    1a04:	3b373737 	blcc	dcf6e8 <start_address+0xdbf6e8>
    1a08:	333b303b 	teqcc	fp, #59	; 0x3b
    1a0c:	32283b31 	eorcc	r3, r8, #50176	; 0xc400
    1a10:	0029312c 	eoreq	r3, r9, ip, lsr #2
    1a14:	746e695f 	strbtvc	r6, [lr], #-2399	; 0x95f
    1a18:	75727265 	ldrbvc	r7, [r2, #-613]!	; 0x265
    1a1c:	61487470 	hvcvs	34624	; 0x8740
    1a20:	656c646e 	strbvs	r6, [ip, #-1134]!	; 0x46e
    1a24:	533a7372 	teqpl	sl, #-939524095	; 0xc8000001
    1a28:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    1a2c:	613d2938 	teqvs	sp, r8, lsr r9
    1a30:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
    1a34:	3b293734 	blcc	a4f70c <start_address+0xa3f70c>
    1a38:	31333b30 	teqcc	r3, r0, lsr fp
    1a3c:	2c31283b 	ldccs	8, cr2, [r1], #-236	; 0xffffff14
    1a40:	73002931 	movwvc	r2, #2353	; 0x931
    1a44:	732f6372 			; <UNDEFINED> instruction: 0x732f6372
    1a48:	61637379 	smcvs	14137	; 0x3739
    1a4c:	2e736c6c 	cdpcs	12, 7, cr6, cr3, cr12, {3}
    1a50:	6e690063 	cdpvs	0, 6, cr0, cr9, cr3, {3}
    1a54:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
    1a58:	79732f65 	ldmdbvc	r3!, {r0, r2, r5, r6, r8, r9, sl, fp, sp}^
    1a5c:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    1a60:	646f636c 	strbtvs	r6, [pc], #-876	; 1a68 <start_address-0xe598>
    1a64:	00682e65 	rsbeq	r2, r8, r5, ror #28
    1a68:	28543a20 	ldmdacs	r4, {r5, r9, fp, ip, sp}^
    1a6c:	29312c31 	ldmdbcs	r1!, {r0, r4, r5, sl, fp, sp}
    1a70:	3873403d 	ldmdacc	r3!, {r0, r2, r3, r4, r5, lr}^
    1a74:	5953653b 	ldmdbpl	r3, {r0, r1, r3, r4, r5, r8, sl, sp, lr}^
    1a78:	4c414353 	mcrrmi	3, 5, r4, r1, cr3
    1a7c:	444b5f4c 	strbmi	r5, [fp], #-3916	; 0xf4c
    1a80:	2c303a42 	ldccs	10, cr3, [r0], #-264	; 0xfffffef8
    1a84:	43535953 	cmpmi	r3, #1359872	; 0x14c000
    1a88:	5f4c4c41 	svcpl	0x004c4c41
    1a8c:	54524155 	ldrbpl	r4, [r2], #-341	; 0x155
    1a90:	5455505f 	ldrbpl	r5, [r5], #-95	; 0x5f
    1a94:	313a4843 	teqcc	sl, r3, asr #16
    1a98:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1a9c:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1aa0:	5241555f 	subpl	r5, r1, #398458880	; 0x17c00000
    1aa4:	45475f54 	strbmi	r5, [r7, #-3924]	; 0xf54
    1aa8:	3a484354 	bcc	1212800 <start_address+0x1202800>
    1aac:	59532c32 	ldmdbpl	r3, {r1, r4, r5, sl, fp, sp}^
    1ab0:	4c414353 	mcrrmi	3, 5, r4, r1, cr3
    1ab4:	4f465f4c 	svcmi	0x00465f4c
    1ab8:	333a4b52 	teqcc	sl, #83968	; 0x14800
    1abc:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1ac0:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1ac4:	5445475f 	strbpl	r4, [r5], #-1887	; 0x75f
    1ac8:	3a444950 	bcc	1114010 <start_address+0x1104010>
    1acc:	59532c34 	ldmdbpl	r3, {r2, r4, r5, sl, fp, sp}^
    1ad0:	4c414353 	mcrrmi	3, 5, r4, r1, cr3
    1ad4:	58455f4c 	stmdapl	r5, {r2, r3, r6, r8, r9, sl, fp, ip, lr}^
    1ad8:	455f4345 	ldrbmi	r4, [pc, #-837]	; 179b <start_address-0xe865>
    1adc:	353a464c 	ldrcc	r4, [sl, #-1612]!	; 0x64c
    1ae0:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1ae4:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1ae8:	4941575f 	stmdbmi	r1, {r0, r1, r2, r3, r4, r6, r8, r9, sl, ip, lr}^
    1aec:	2c363a54 	ldccs	10, cr3, [r6], #-336	; 0xfffffeb0
    1af0:	43535953 	cmpmi	r3, #1359872	; 0x14c000
    1af4:	5f4c4c41 	svcpl	0x004c4c41
    1af8:	4c454959 	mcrrmi	9, 5, r4, r5, cr9
    1afc:	2c373a44 	ldccs	10, cr3, [r7], #-272	; 0xfffffef0
    1b00:	43535953 	cmpmi	r3, #1359872	; 0x14c000
    1b04:	5f4c4c41 	svcpl	0x004c4c41
    1b08:	54495845 	strbpl	r5, [r9], #-2117	; 0x845
    1b0c:	532c383a 			; <UNDEFINED> instruction: 0x532c383a
    1b10:	41435359 	cmpmi	r3, r9, asr r3
    1b14:	505f4c4c 	subspl	r4, pc, ip, asr #24
    1b18:	4c4c414d 	stfmie	f4, [ip], {77}	; 0x4d
    1b1c:	393a434f 	ldmdbcc	sl!, {r0, r1, r2, r3, r6, r8, r9, lr}
    1b20:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1b24:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1b28:	5246505f 	subpl	r5, r6, #95	; 0x5f
    1b2c:	313a4545 	teqcc	sl, r5, asr #10
    1b30:	59532c30 	ldmdbpl	r3, {r4, r5, sl, fp, sp}^
    1b34:	4c414353 	mcrrmi	3, 5, r4, r1, cr3
    1b38:	45475f4c 	strbmi	r5, [r7, #-3916]	; 0xf4c
    1b3c:	4d435f54 	stclmi	15, cr5, [r3, #-336]	; 0xfffffeb0
    1b40:	31313a44 	teqcc	r1, r4, asr #20
    1b44:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1b48:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1b4c:	4e45535f 	mcrmi	3, 2, r5, cr5, cr15, {2}
    1b50:	534d5f44 	movtpl	r5, #57156	; 0xdf44
    1b54:	32313a47 	eorscc	r3, r1, #290816	; 0x47000
    1b58:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1b5c:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1b60:	4145525f 	cmpmi	r5, pc, asr r2
    1b64:	534d5f44 	movtpl	r5, #57156	; 0xdf44
    1b68:	33313a47 	teqcc	r1, #290816	; 0x47000
    1b6c:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1b70:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1b74:	494e495f 	stmdbmi	lr, {r0, r1, r2, r3, r4, r6, r8, fp, lr}^
    1b78:	5f445254 	svcpl	0x00445254
    1b7c:	44414552 	strbmi	r4, [r1], #-1362	; 0x552
    1b80:	2c34313a 	ldfcss	f3, [r4], #-232	; 0xffffff18
    1b84:	43535953 	cmpmi	r3, #1359872	; 0x14c000
    1b88:	5f4c4c41 	svcpl	0x004c4c41
    1b8c:	54494e49 	strbpl	r4, [r9], #-3657	; 0xe49
    1b90:	465f4452 			; <UNDEFINED> instruction: 0x465f4452
    1b94:	53454c49 	movtpl	r4, #23625	; 0x5c49
    1b98:	2c35313a 	ldfcss	f3, [r5], #-232	; 0xffffff18
    1b9c:	43535953 	cmpmi	r3, #1359872	; 0x14c000
    1ba0:	5f4c4c41 	svcpl	0x004c4c41
    1ba4:	54494e49 	strbpl	r4, [r9], #-3657	; 0xe49
    1ba8:	495f4452 	ldmdbmi	pc, {r1, r4, r6, sl, lr}^	; <UNPREDICTABLE>
    1bac:	3a4f464e 	bcc	13d34ec <start_address+0x13c34ec>
    1bb0:	532c3631 			; <UNDEFINED> instruction: 0x532c3631
    1bb4:	41435359 	cmpmi	r3, r9, asr r3
    1bb8:	505f4c4c 	subspl	r4, pc, ip, asr #24
    1bbc:	454c4946 	strbmi	r4, [ip, #-2374]	; 0x946
    1bc0:	444f4e5f 	strbmi	r4, [pc], #-3679	; 1bc8 <start_address-0xe438>
    1bc4:	37313a45 	ldrcc	r3, [r1, -r5, asr #20]!
    1bc8:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1bcc:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1bd0:	4946505f 	stmdbmi	r6, {r0, r1, r2, r3, r4, r6, ip, lr}^
    1bd4:	475f454c 	ldrbmi	r4, [pc, -ip, asr #10]
    1bd8:	535f5445 	cmppl	pc, #1157627904	; 0x45000000
    1bdc:	3a4b4545 	bcc	12d30f8 <start_address+0x12c30f8>
    1be0:	532c3831 			; <UNDEFINED> instruction: 0x532c3831
    1be4:	41435359 	cmpmi	r3, r9, asr r3
    1be8:	505f4c4c 	subspl	r4, pc, ip, asr #24
    1bec:	454c4946 	strbmi	r4, [ip, #-2374]	; 0x946
    1bf0:	4545535f 	strbmi	r5, [r5, #-863]	; 0x35f
    1bf4:	39313a4b 	ldmdbcc	r1!, {r0, r1, r3, r6, r9, fp, ip, sp}
    1bf8:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1bfc:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1c00:	4946505f 	stmdbmi	r6, {r0, r1, r2, r3, r4, r6, ip, lr}^
    1c04:	4f5f454c 	svcmi	0x005f454c
    1c08:	3a4e4550 	bcc	1393150 <start_address+0x1383150>
    1c0c:	532c3032 			; <UNDEFINED> instruction: 0x532c3032
    1c10:	41435359 	cmpmi	r3, r9, asr r3
    1c14:	505f4c4c 	subspl	r4, pc, ip, asr #24
    1c18:	454c4946 	strbmi	r4, [ip, #-2374]	; 0x946
    1c1c:	4f4c435f 	svcmi	0x004c435f
    1c20:	323a4553 	eorscc	r4, sl, #348127232	; 0x14c00000
    1c24:	59532c31 	ldmdbpl	r3, {r0, r4, r5, sl, fp, sp}^
    1c28:	4c414353 	mcrrmi	3, 5, r4, r1, cr3
    1c2c:	534b5f4c 	movtpl	r5, #48972	; 0xbf4c
    1c30:	5f565245 	svcpl	0x00565245
    1c34:	3a474552 	bcc	11d3184 <start_address+0x11c3184>
    1c38:	532c3232 			; <UNDEFINED> instruction: 0x532c3232
    1c3c:	41435359 	cmpmi	r3, r9, asr r3
    1c40:	4b5f4c4c 	blmi	17d4d78 <start_address+0x17c4d78>
    1c44:	56524553 			; <UNDEFINED> instruction: 0x56524553
    1c48:	5445475f 	strbpl	r4, [r5], #-1887	; 0x75f
    1c4c:	2c33323a 	lfmcs	f3, 4, [r3], #-232	; 0xffffff18
    1c50:	43535953 	cmpmi	r3, #1359872	; 0x14c000
    1c54:	5f4c4c41 	svcpl	0x004c4c41
    1c58:	5f544547 	svcpl	0x00544547
    1c5c:	434f5250 	movtmi	r5, #62032	; 0xf250
    1c60:	34323a53 	ldrtcc	r3, [r2], #-2643	; 0xa53
    1c64:	5359532c 	cmppl	r9, #44, 6	; 0xb0000000
    1c68:	4c4c4143 	stfmie	f4, [ip], {67}	; 0x43
    1c6c:	5445475f 	strbpl	r4, [r5], #-1887	; 0x75f
    1c70:	4457435f 	ldrbmi	r4, [r7], #-863	; 0x35f
    1c74:	2c35323a 	lfmcs	f3, 4, [r5], #-232	; 0xffffff18
    1c78:	43535953 	cmpmi	r3, #1359872	; 0x14c000
    1c7c:	5f4c4c41 	svcpl	0x004c4c41
    1c80:	5f544553 	svcpl	0x00544553
    1c84:	3a445743 	bcc	1117998 <start_address+0x1107998>
    1c88:	532c3632 			; <UNDEFINED> instruction: 0x532c3632
    1c8c:	41435359 	cmpmi	r3, r9, asr r3
    1c90:	535f4c4c 	cmppl	pc, #76, 24	; 0x4c00
    1c94:	555f5445 	ldrbpl	r5, [pc, #-1093]	; 1857 <start_address-0xe7a9>
    1c98:	323a4449 	eorscc	r4, sl, #1224736768	; 0x49000000
    1c9c:	59532c37 	ldmdbpl	r3, {r0, r1, r2, r4, r5, sl, fp, sp}^
    1ca0:	4c414353 	mcrrmi	3, 5, r4, r1, cr3
    1ca4:	45475f4c 	strbmi	r5, [r7, #-3916]	; 0xf4c
    1ca8:	49555f54 	ldmdbmi	r5, {r2, r4, r6, r8, r9, sl, fp, ip, lr}^
    1cac:	38323a44 	ldmdacc	r2!, {r2, r6, r9, fp, ip, sp}
    1cb0:	53003b2c 	movwpl	r3, #2860	; 0xb2c
    1cb4:	61637379 	smcvs	14137	; 0x3739
    1cb8:	6f436c6c 	svcvs	0x00436c6c
    1cbc:	3a546564 	bcc	151b254 <start_address+0x150b254>
    1cc0:	2c312874 	ldccs	8, cr2, [r1], #-464	; 0xfffffe30
    1cc4:	283d2932 	ldmdacs	sp!, {r1, r4, r5, r8, fp, sp}
    1cc8:	29312c31 	ldmdbcs	r1!, {r0, r4, r5, sl, fp, sp}
    1ccc:	636e6900 	cmnvs	lr, #0, 18
    1cd0:	6564756c 	strbvs	r7, [r4, #-1388]!	; 0x56c
    1cd4:	7379732f 	cmnvc	r9, #-1140850688	; 0xbc000000
    1cd8:	2e6d6574 	mcrcs	5, 3, r6, cr13, cr4, {3}
    1cdc:	72500068 	subsvc	r0, r0, #104	; 0x68
    1ce0:	6e49636f 	cdpvs	3, 4, cr6, cr9, cr15, {3}
    1ce4:	3a546f66 	bcc	151da84 <start_address+0x150da84>
    1ce8:	30312874 	eorscc	r2, r1, r4, ror r8
    1cec:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
    1cf0:	2c303128 	ldfcss	f3, [r0], #-160	; 0xffffff60
    1cf4:	733d2932 	teqvc	sp, #819200	; 0xc8000
    1cf8:	70343431 	eorsvc	r3, r4, r1, lsr r4
    1cfc:	283a6469 	ldmdacs	sl!, {r0, r3, r5, r6, sl, sp, lr}
    1d00:	29362c33 	ldmdbcs	r6!, {r0, r1, r4, r5, sl, fp, sp}
    1d04:	332c302c 			; <UNDEFINED> instruction: 0x332c302c
    1d08:	61663b32 	cmnvs	r6, r2, lsr fp
    1d0c:	72656874 	rsbvc	r6, r5, #116, 16	; 0x740000
    1d10:	3a646950 	bcc	191c258 <start_address+0x190c258>
    1d14:	362c3328 	strtcc	r3, [ip], -r8, lsr #6
    1d18:	32332c29 	eorscc	r2, r3, #10496	; 0x2900
    1d1c:	3b32332c 	blcc	c8e9d4 <start_address+0xc7e9d4>
    1d20:	656e776f 	strbvs	r7, [lr, #-1903]!	; 0x76f
    1d24:	33283a72 			; <UNDEFINED> instruction: 0x33283a72
    1d28:	2c29362c 	stccs	6, cr3, [r9], #-176	; 0xffffff50
    1d2c:	332c3436 			; <UNDEFINED> instruction: 0x332c3436
    1d30:	65683b32 	strbvs	r3, [r8, #-2866]!	; 0xb32
    1d34:	69537061 	ldmdbvs	r3, {r0, r5, r6, ip, sp, lr}^
    1d38:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    1d3c:	29372c33 	ldmdbcs	r7!, {r0, r1, r4, r5, sl, fp, sp}
    1d40:	2c36392c 	ldccs	9, cr3, [r6], #-176	; 0xffffff50
    1d44:	633b3233 	teqvs	fp, #805306371	; 0x30000003
    1d48:	283a646d 	ldmdacs	sl!, {r0, r2, r3, r5, r6, sl, sp, lr}
    1d4c:	332c3031 			; <UNDEFINED> instruction: 0x332c3031
    1d50:	72613d29 	rsbvc	r3, r1, #2624	; 0xa40
    1d54:	2c303128 	ldfcss	f3, [r0], #-160	; 0xffffff60
    1d58:	723d2934 	eorsvc	r2, sp, #52, 18	; 0xd0000
    1d5c:	2c303128 	ldfcss	f3, [r0], #-160	; 0xffffff60
    1d60:	303b2934 	eorscc	r2, fp, r4, lsr r9
    1d64:	3733303b 			; <UNDEFINED> instruction: 0x3733303b
    1d68:	37373737 			; <UNDEFINED> instruction: 0x37373737
    1d6c:	37373737 			; <UNDEFINED> instruction: 0x37373737
    1d70:	303b3b37 	eorscc	r3, fp, r7, lsr fp
    1d74:	3732313b 			; <UNDEFINED> instruction: 0x3732313b
    1d78:	2c30283b 	ldccs	8, cr2, [r0], #-236	; 0xffffff14
    1d7c:	312c2932 			; <UNDEFINED> instruction: 0x312c2932
    1d80:	312c3832 			; <UNDEFINED> instruction: 0x312c3832
    1d84:	3b343230 	blcc	d0e64c <start_address+0xcfe64c>
    1d88:	6e45003b 	mcrvs	0, 2, r0, cr5, cr11, {1}
    1d8c:	46797274 			; <UNDEFINED> instruction: 0x46797274
    1d90:	74636e75 	strbtvc	r6, [r3], #-3701	; 0xe75
    1d94:	546e6f69 	strbtpl	r6, [lr], #-3945	; 0xf69
    1d98:	3428743a 	strtcc	r7, [r8], #-1082	; 0x43a
    1d9c:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
    1da0:	322c3428 	eorcc	r3, ip, #40, 8	; 0x28000000
    1da4:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    1da8:	29332c34 	ldmdbcs	r3!, {r2, r4, r5, sl, fp, sp}
    1dac:	3028663d 	eorcc	r6, r8, sp, lsr r6
    1db0:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    1db4:	6f725000 	svcvs	0x00725000
    1db8:	73736563 	cmnvc	r3, #415236096	; 0x18c00000
    1dbc:	74617453 	strbtvc	r7, [r1], #-1107	; 0x453
    1dc0:	28543a65 	ldmdacs	r4, {r0, r2, r5, r6, r9, fp, ip, sp}^
    1dc4:	29342c34 	ldmdbcs	r4!, {r2, r4, r5, sl, fp, sp}
    1dc8:	3873403d 	ldmdacc	r3!, {r0, r2, r3, r4, r5, lr}^
    1dcc:	4e55653b 	mrcmi	5, 2, r6, cr5, cr11, {1}
    1dd0:	44455355 	strbmi	r5, [r5], #-853	; 0x355
    1dd4:	432c303a 			; <UNDEFINED> instruction: 0x432c303a
    1dd8:	54414552 	strbpl	r4, [r1], #-1362	; 0x552
    1ddc:	313a4445 	teqcc	sl, r5, asr #8
    1de0:	454c532c 	strbmi	r5, [ip, #-812]	; 0x32c
    1de4:	4e495045 	cdpmi	0, 4, cr5, cr9, cr5, {2}
    1de8:	2c323a47 	ldccs	10, cr3, [r2], #-284	; 0xfffffee4
    1dec:	44414552 	strbmi	r4, [r1], #-1362	; 0x552
    1df0:	2c333a59 	ldccs	10, cr3, [r3], #-356	; 0xfffffe9c
    1df4:	4e4e5552 	mcrmi	5, 2, r5, cr14, cr2, {2}
    1df8:	3a474e49 	bcc	11d5724 <start_address+0x11c5724>
    1dfc:	45542c34 	ldrbmi	r2, [r4, #-3124]	; 0xc34
    1e00:	4e494d52 	mcrmi	13, 2, r4, cr9, cr2, {2}
    1e04:	44455441 	strbmi	r5, [r5], #-1089	; 0x441
    1e08:	3b2c353a 	blcc	b0f2f8 <start_address+0xaff2f8>
    1e0c:	6e6f4300 	cdpvs	3, 6, cr4, cr15, cr0, {0}
    1e10:	74786574 	ldrbtvc	r6, [r8], #-1396	; 0x574
    1e14:	6d657449 	cfstrdvs	mvd7, [r5, #-292]!	; 0xfffffedc
    1e18:	3428543a 	strtcc	r5, [r8], #-1082	; 0x43a
    1e1c:	3d29352c 	cfstr32cc	mvfx3, [r9, #-176]!	; 0xffffff50
    1e20:	3b387340 	blcc	e1eb28 <start_address+0xe0eb28>
    1e24:	53504365 	cmppl	r0, #-1811939327	; 0x94000001
    1e28:	2c303a52 	ldccs	10, cr3, [r0], #-328	; 0xfffffeb8
    1e2c:	54534552 	ldrbpl	r4, [r3], #-1362	; 0x552
    1e30:	5f545241 	svcpl	0x00545241
    1e34:	52444441 	subpl	r4, r4, #1090519040	; 0x41000000
    1e38:	522c313a 	eorpl	r3, ip, #-2147483634	; 0x8000000e
    1e3c:	2c323a30 	ldccs	10, cr3, [r2], #-192	; 0xffffff40
    1e40:	333a3152 	teqcc	sl, #-2147483628	; 0x80000014
    1e44:	3a32522c 	bcc	c966fc <start_address+0xc866fc>
    1e48:	33522c34 	cmpcc	r2, #52, 24	; 0x3400
    1e4c:	522c353a 	eorpl	r3, ip, #243269632	; 0xe800000
    1e50:	2c363a34 	ldccs	10, cr3, [r6], #-208	; 0xffffff30
    1e54:	373a3552 			; <UNDEFINED> instruction: 0x373a3552
    1e58:	3a36522c 	bcc	d96710 <start_address+0xd86710>
    1e5c:	37522c38 	smmlarcc	r2, r8, ip, r2
    1e60:	522c393a 	eorpl	r3, ip, #950272	; 0xe8000
    1e64:	30313a38 	eorscc	r3, r1, r8, lsr sl
    1e68:	3a39522c 	bcc	e56720 <start_address+0xe46720>
    1e6c:	522c3131 	eorpl	r3, ip, #1073741836	; 0x4000000c
    1e70:	313a3031 	teqcc	sl, r1, lsr r0
    1e74:	31522c32 	cmpcc	r2, r2, lsr ip
    1e78:	33313a31 	teqcc	r1, #200704	; 0x31000
    1e7c:	3231522c 	eorscc	r5, r1, #44, 4	; 0xc0000002
    1e80:	2c34313a 	ldfcss	f3, [r4], #-232	; 0xffffff18
    1e84:	313a5053 	teqcc	sl, r3, asr r0
    1e88:	524c2c35 	subpl	r2, ip, #13568	; 0x3500
    1e8c:	2c36313a 	ldfcss	f3, [r6], #-232	; 0xffffff18
    1e90:	7250003b 	subsvc	r0, r0, #59	; 0x3b
    1e94:	6946636f 	stmdbvs	r6, {r0, r1, r2, r3, r5, r6, r8, r9, sp, lr}^
    1e98:	3a54656c 	bcc	151b450 <start_address+0x150b450>
    1e9c:	2c342874 	ldccs	8, cr2, [r4], #-464	; 0xfffffe30
    1ea0:	283d2936 	ldmdacs	sp!, {r1, r2, r4, r5, r8, fp, sp}
    1ea4:	29372c34 	ldmdbcs	r7!, {r2, r4, r5, sl, fp, sp}
    1ea8:	3231733d 	eorscc	r7, r1, #-201326592	; 0xf4000000
    1eac:	283a666b 	ldmdacs	sl!, {r0, r1, r3, r5, r6, r9, sl, sp, lr}
    1eb0:	29382c34 	ldmdbcs	r8!, {r2, r4, r5, sl, fp, sp}
    1eb4:	39282a3d 	stmdbcc	r8!, {r0, r2, r3, r4, r5, r9, fp, sp}
    1eb8:	2c29332c 	stccs	3, cr3, [r9], #-176	; 0xffffff50
    1ebc:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    1ec0:	616c663b 	cmnvs	ip, fp, lsr r6
    1ec4:	283a7367 	ldmdacs	sl!, {r0, r1, r2, r5, r6, r8, r9, ip, sp, lr}
    1ec8:	29372c33 	ldmdbcs	r7!, {r0, r1, r4, r5, sl, fp, sp}
    1ecc:	2c32332c 	ldccs	3, cr3, [r2], #-176	; 0xffffff50
    1ed0:	733b3233 	teqvc	fp, #805306371	; 0x30000003
    1ed4:	3a6b6565 	bcc	1adb470 <start_address+0x1acb470>
    1ed8:	372c3328 	strcc	r3, [ip, -r8, lsr #6]!
    1edc:	34362c29 	ldrtcc	r2, [r6], #-3113	; 0xc29
    1ee0:	3b32332c 	blcc	c8eb98 <start_address+0xc7eb98>
    1ee4:	7250003b 	subsvc	r0, r0, #59	; 0x3b
    1ee8:	7365636f 	cmnvc	r5, #-1140850687	; 0xbc000001
    1eec:	743a5473 	ldrtvc	r5, [sl], #-1139	; 0x473
    1ef0:	392c3428 	stmdbcc	ip!, {r3, r5, sl, ip, sp}
    1ef4:	34283d29 	strtcc	r3, [r8], #-3369	; 0xd29
    1ef8:	2930312c 	ldmdbcs	r0!, {r2, r3, r5, r8, ip, sp}
    1efc:	3139733d 	teqcc	r9, sp, lsr r3
    1f00:	61747332 	cmnvs	r4, r2, lsr r3
    1f04:	283a6574 	ldmdacs	sl!, {r2, r4, r5, r6, r8, sl, sp, lr}
    1f08:	29342c34 	ldmdbcs	r4!, {r2, r4, r5, sl, fp, sp}
    1f0c:	382c302c 	stmdacc	ip!, {r2, r3, r5, ip, sp}
    1f10:	6469703b 	strbtvs	r7, [r9], #-59	; 0x3b
    1f14:	2c33283a 	ldccs	8, cr2, [r3], #-232	; 0xffffff18
    1f18:	332c2936 			; <UNDEFINED> instruction: 0x332c2936
    1f1c:	32332c32 	eorscc	r2, r3, #12800	; 0x3200
    1f20:	7461663b 	strbtvc	r6, [r1], #-1595	; 0x63b
    1f24:	50726568 	rsbspl	r6, r2, r8, ror #10
    1f28:	283a6469 	ldmdacs	sl!, {r0, r3, r5, r6, sl, sp, lr}
    1f2c:	29362c33 	ldmdbcs	r6!, {r0, r1, r4, r5, sl, fp, sp}
    1f30:	2c34362c 	ldccs	6, cr3, [r4], #-176	; 0xffffff50
    1f34:	6f3b3233 	svcvs	0x003b3233
    1f38:	72656e77 	rsbvc	r6, r5, #1904	; 0x770
    1f3c:	2c33283a 	ldccs	8, cr2, [r3], #-232	; 0xffffff18
    1f40:	392c2936 	stmdbcc	ip!, {r1, r2, r4, r5, r8, fp, sp}
    1f44:	32332c36 	eorscc	r2, r3, #13824	; 0x3600
    1f48:	646d633b 	strbtvs	r6, [sp], #-827	; 0x33b
    1f4c:	3031283a 	eorscc	r2, r1, sl, lsr r8
    1f50:	2c29332c 	stccs	3, cr3, [r9], #-176	; 0xffffff50
    1f54:	2c383231 	lfmcs	f3, 4, [r8], #-196	; 0xffffff3c
    1f58:	34323031 	ldrtcc	r3, [r2], #-49	; 0x31
    1f5c:	6477703b 	ldrbtvs	r7, [r7], #-59	; 0x3b
    1f60:	2c34283a 	ldccs	8, cr2, [r4], #-232	; 0xffffff18
    1f64:	3d293131 	stfccs	f3, [r9, #-196]!	; 0xffffff3c
    1f68:	31287261 			; <UNDEFINED> instruction: 0x31287261
    1f6c:	29342c30 	ldmdbcs	r4!, {r4, r5, sl, fp, sp}
    1f70:	323b303b 	eorscc	r3, fp, #59	; 0x3b
    1f74:	283b3535 	ldmdacs	fp!, {r0, r2, r4, r5, r8, sl, ip, sp}
    1f78:	29322c30 	ldmdbcs	r2!, {r4, r5, sl, fp, sp}
    1f7c:	3531312c 	ldrcc	r3, [r1, #-300]!	; 0x12c
    1f80:	30322c32 	eorscc	r2, r2, r2, lsr ip
    1f84:	653b3834 	ldrvs	r3, [fp, #-2100]!	; 0x834
    1f88:	7972746e 	ldmdbvc	r2!, {r1, r2, r3, r5, r6, sl, ip, sp, lr}^
    1f8c:	2c34283a 	ldccs	8, cr2, [r4], #-232	; 0xffffff18
    1f90:	332c2931 			; <UNDEFINED> instruction: 0x332c2931
    1f94:	2c303032 	ldccs	0, cr3, [r0], #-200	; 0xffffff38
    1f98:	763b3233 			; <UNDEFINED> instruction: 0x763b3233
    1f9c:	34283a6d 	strtcc	r3, [r8], #-2669	; 0xa6d
    1fa0:	2932312c 	ldmdbcs	r2!, {r2, r3, r5, r8, ip, sp}
    1fa4:	35282a3d 	strcc	r2, [r8, #-2621]!	; 0xa3d
    1fa8:	2c29312c 	stfcss	f3, [r9], #-176	; 0xffffff50
    1fac:	32333233 	eorscc	r3, r3, #805306371	; 0x30000003
    1fb0:	3b32332c 	blcc	c8ec68 <start_address+0xc7ec68>
    1fb4:	70616568 	rsbvc	r6, r1, r8, ror #10
    1fb8:	657a6953 	ldrbvs	r6, [sl, #-2387]!	; 0x953
    1fbc:	2c33283a 	ldccs	8, cr2, [r3], #-232	; 0xffffff18
    1fc0:	332c2937 			; <UNDEFINED> instruction: 0x332c2937
    1fc4:	2c343632 	ldccs	6, cr3, [r4], #-200	; 0xffffff38
    1fc8:	753b3233 	ldrvc	r3, [fp, #-563]!	; 0x233
    1fcc:	53726573 	cmnpl	r2, #482344960	; 0x1cc00000
    1fd0:	6b636174 	blvs	18da5a8 <start_address+0x18ca5a8>
    1fd4:	2c36283a 	ldccs	8, cr2, [r6], #-232	; 0xffffff18
    1fd8:	332c2933 			; <UNDEFINED> instruction: 0x332c2933
    1fdc:	2c363932 	ldccs	9, cr3, [r6], #-200	; 0xffffff38
    1fe0:	6b3b3233 	blvs	ece8b4 <start_address+0xebe8b4>
    1fe4:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
    1fe8:	6174536c 	cmnvs	r4, ip, ror #6
    1fec:	283a6b63 	ldmdacs	sl!, {r0, r1, r5, r6, r8, r9, fp, sp, lr}
    1ff0:	29332c36 	ldmdbcs	r3!, {r1, r2, r4, r5, sl, fp, sp}
    1ff4:	3233332c 	eorscc	r3, r3, #44, 6	; 0xb0000000
    1ff8:	32332c38 	eorscc	r2, r3, #56, 24	; 0x3800
    1ffc:	6e6f633b 	mcrvs	3, 3, r6, cr15, cr11, {1}
    2000:	74786574 	ldrbtvc	r6, [r8], #-1396	; 0x574
    2004:	2c34283a 	ldccs	8, cr2, [r4], #-232	; 0xffffff18
    2008:	3d293331 	stccc	3, cr3, [r9, #-196]!	; 0xffffff3c
    200c:	31287261 			; <UNDEFINED> instruction: 0x31287261
    2010:	29342c30 	ldmdbcs	r4!, {r4, r5, sl, fp, sp}
    2014:	313b303b 	teqcc	fp, fp, lsr r0
    2018:	30283b36 	eorcc	r3, r8, r6, lsr fp
    201c:	2c29312c 	stfcss	f3, [r9], #-176	; 0xffffff50
    2020:	30363333 	eorscc	r3, r6, r3, lsr r3
    2024:	3434352c 	ldrtcc	r3, [r4], #-1324	; 0x52c
    2028:	6961773b 	stmdbvs	r1!, {r0, r1, r3, r4, r5, r8, r9, sl, ip, sp, lr}^
    202c:	64695074 	strbtvs	r5, [r9], #-116	; 0x74
    2030:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2034:	332c2931 			; <UNDEFINED> instruction: 0x332c2931
    2038:	2c343039 	ldccs	0, cr3, [r4], #-228	; 0xffffff1c
    203c:	633b3233 	teqvs	fp, #805306371	; 0x30000003
    2040:	646c6968 	strbtvs	r6, [ip], #-2408	; 0x968
    2044:	75746552 	ldrbvc	r6, [r4, #-1362]!	; 0x552
    2048:	61566e72 	cmpvs	r6, r2, ror lr
    204c:	3a65756c 	bcc	195f604 <start_address+0x194f604>
    2050:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    2054:	39332c29 	ldmdbcc	r3!, {r0, r3, r5, sl, fp, sp}
    2058:	332c3633 			; <UNDEFINED> instruction: 0x332c3633
    205c:	616d3b32 	cmnvs	sp, r2, lsr fp
    2060:	636f6c6c 	cmnvs	pc, #108, 24	; 0x6c00
    2064:	3a6e614d 	bcc	1b9a5a0 <start_address+0x1b8a5a0>
    2068:	352c3628 	strcc	r3, [ip, #-1576]!	; 0x628
    206c:	39332c29 	ldmdbcc	r3!, {r0, r3, r5, sl, fp, sp}
    2070:	312c3836 			; <UNDEFINED> instruction: 0x312c3836
    2074:	6d3b3239 	lfmvs	f3, 4, [fp, #-228]!	; 0xffffff1c
    2078:	61737365 	cmnvs	r3, r5, ror #6
    207c:	75516567 	ldrbvc	r6, [r1, #-1383]	; 0x567
    2080:	3a657565 	bcc	195f61c <start_address+0x194f61c>
    2084:	352c3728 	strcc	r3, [ip, #-1832]!	; 0x728
    2088:	31342c29 	teqcc	r4, r9, lsr #24
    208c:	362c3036 			; <UNDEFINED> instruction: 0x362c3036
    2090:	69663b34 	stmdbvs	r6!, {r2, r4, r5, r8, r9, fp, ip, sp}^
    2094:	3a73656c 	bcc	1cdb64c <start_address+0x1ccb64c>
    2098:	312c3428 			; <UNDEFINED> instruction: 0x312c3428
    209c:	613d2934 	teqvs	sp, r4, lsr r9
    20a0:	30312872 	eorscc	r2, r1, r2, ror r8
    20a4:	3b29342c 	blcc	a4f15c <start_address+0xa3f15c>
    20a8:	31333b30 	teqcc	r3, r0, lsr fp
    20ac:	2c34283b 	ldccs	8, cr2, [r4], #-236	; 0xffffff14
    20b0:	342c2936 	strtcc	r2, [ip], #-2358	; 0x936
    20b4:	2c343232 	lfmcs	f3, 4, [r4], #-200	; 0xffffff38
    20b8:	32373033 	eorscc	r3, r7, #51	; 0x33
    20bc:	52003b3b 	andpl	r3, r0, #60416	; 0xec00
    20c0:	69466d61 	stmdbvs	r6, {r0, r5, r6, r8, sl, fp, sp, lr}^
    20c4:	543a656c 	ldrtpl	r6, [sl], #-1388	; 0x56c
    20c8:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    20cc:	733d2931 	teqvc	sp, #802816	; 0xc4000
    20d0:	6e383632 	mrcvs	6, 1, r3, cr8, cr2, {1}
    20d4:	3a747865 	bcc	1d20270 <start_address+0x1d10270>
    20d8:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    20dc:	2a3d2932 	bcs	f4c5ac <start_address+0xf3c5ac>
    20e0:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    20e4:	302c2931 	eorcc	r2, ip, r1, lsr r9
    20e8:	3b32332c 	blcc	c8eda0 <start_address+0xc7eda0>
    20ec:	656d616e 	strbvs	r6, [sp, #-366]!	; 0x16e
    20f0:	2c34283a 	ldccs	8, cr2, [r4], #-232	; 0xffffff18
    20f4:	2c293131 	stfcss	f3, [r9], #-196	; 0xffffff3c
    20f8:	322c3233 	eorcc	r3, ip, #805306371	; 0x30000003
    20fc:	3b383430 	blcc	e0f1c4 <start_address+0xdff1c4>
    2100:	746e6f63 	strbtvc	r6, [lr], #-3939	; 0xf63
    2104:	3a746e65 	bcc	1d1daa0 <start_address+0x1d0daa0>
    2108:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    210c:	2a3d2933 	bcs	f4c5e0 <start_address+0xf3c5e0>
    2110:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    2114:	6b3d2934 	blvs	f4c5ec <start_address+0xf3c5ec>
    2118:	322c3028 	eorcc	r3, ip, #40	; 0x28
    211c:	30322c29 	eorscc	r2, r2, r9, lsr #24
    2120:	332c3038 			; <UNDEFINED> instruction: 0x332c3038
    2124:	69733b32 	ldmdbvs	r3!, {r1, r4, r5, r8, r9, fp, ip, sp}^
    2128:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    212c:	29362c33 	ldmdbcs	r6!, {r0, r1, r4, r5, sl, fp, sp}
    2130:	3131322c 	teqcc	r1, ip, lsr #4
    2134:	32332c32 	eorscc	r2, r3, #12800	; 0x3200
    2138:	52003b3b 	andpl	r3, r0, #60416	; 0xec00
    213c:	69466d61 	stmdbvs	r6, {r0, r5, r6, r8, sl, fp, sp, lr}^
    2140:	3a54656c 	bcc	151b6f8 <start_address+0x150b6f8>
    2144:	32312874 	eorscc	r2, r1, #116, 16	; 0x740000
    2148:	3d29352c 	cfstr32cc	mvfx3, [r9, #-176]!	; 0xffffff50
    214c:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    2150:	52002931 	andpl	r2, r0, #802816	; 0xc4000
    2154:	69446d61 	stmdbvs	r4, {r0, r5, r6, r8, sl, fp, sp, lr}^
    2158:	3a546b73 	bcc	151cf2c <start_address+0x150cf2c>
    215c:	32312874 	eorscc	r2, r1, #116, 16	; 0x740000
    2160:	3d29362c 	stccc	6, cr3, [r9, #-176]!	; 0xffffff50
    2164:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    2168:	733d2937 	teqvc	sp, #901120	; 0xdc000
    216c:	61656838 	cmnvs	r5, r8, lsr r8
    2170:	31283a64 			; <UNDEFINED> instruction: 0x31283a64
    2174:	29382c32 	ldmdbcs	r8!, {r1, r4, r5, sl, fp, sp}
    2178:	31282a3d 			; <UNDEFINED> instruction: 0x31282a3d
    217c:	29352c32 	ldmdbcs	r5!, {r1, r4, r5, sl, fp, sp}
    2180:	332c302c 			; <UNDEFINED> instruction: 0x332c302c
    2184:	61723b32 	cmnvs	r2, r2, lsr fp
    2188:	31283a6d 			; <UNDEFINED> instruction: 0x31283a6d
    218c:	29332c32 	ldmdbcs	r3!, {r1, r4, r5, sl, fp, sp}
    2190:	2c32332c 	ldccs	3, cr3, [r2], #-176	; 0xffffff50
    2194:	3b3b3233 	blcc	ecea68 <start_address+0xebea68>
    2198:	636e6900 	cmnvs	lr, #0, 18
    219c:	6564756c 	strbvs	r7, [r4, #-1388]!	; 0x56c
    21a0:	65736b2f 	ldrbvs	r6, [r3, #-2863]!	; 0xb2f
    21a4:	682e7672 	stmdavs	lr!, {r1, r4, r5, r6, r9, sl, ip, sp, lr}
    21a8:	65534b00 	ldrbvs	r4, [r3, #-2816]	; 0xb00
    21ac:	3a547672 	bcc	151fb7c <start_address+0x150fb7c>
    21b0:	33312874 	teqcc	r1, #116, 16	; 0x740000
    21b4:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
    21b8:	2c333128 	ldfcss	f3, [r3], #-160	; 0xffffff60
    21bc:	733d2932 	teqvc	sp, #819200	; 0xc8000
    21c0:	616e3432 	cmnvs	lr, r2, lsr r4
    21c4:	283a656d 	ldmdacs	sl!, {r0, r2, r3, r5, r6, r8, sl, sp, lr}
    21c8:	332c3331 			; <UNDEFINED> instruction: 0x332c3331
    21cc:	72613d29 	rsbvc	r3, r1, #2624	; 0xa40
    21d0:	2c303128 	ldfcss	f3, [r0], #-160	; 0xffffff60
    21d4:	303b2934 	eorscc	r2, fp, r4, lsr r9
    21d8:	3b36313b 	blcc	d8e6cc <start_address+0xd7e6cc>
    21dc:	322c3028 	eorcc	r3, ip, #40	; 0x28
    21e0:	2c302c29 	ldccs	12, cr2, [r0], #-164	; 0xffffff5c
    21e4:	3b363331 	blcc	d8eeb0 <start_address+0xd7eeb0>
    21e8:	3a646970 	bcc	191c7b0 <start_address+0x190c7b0>
    21ec:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    21f0:	36312c29 	ldrtcc	r2, [r1], -r9, lsr #24
    21f4:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    21f8:	69003b3b 	stmdbvs	r0, {r0, r1, r3, r4, r5, r8, r9, fp, ip, sp}
    21fc:	756c636e 	strbvc	r6, [ip, #-878]!	; 0x36e
    2200:	662f6564 	strtvs	r6, [pc], -r4, ror #10
    2204:	666e6973 			; <UNDEFINED> instruction: 0x666e6973
    2208:	00682e6f 	rsbeq	r2, r8, pc, ror #28
    220c:	6e495346 	cdpvs	3, 4, cr5, cr9, cr6, {2}
    2210:	543a6f66 	ldrtpl	r6, [sl], #-3942	; 0xf66
    2214:	2c343128 	ldfcss	f3, [r4], #-160	; 0xffffff60
    2218:	733d2931 	teqvc	sp, #802816	; 0xc4000
    221c:	73383632 	teqvc	r8, #52428800	; 0x3200000
    2220:	3a657a69 	bcc	1960bcc <start_address+0x1950bcc>
    2224:	372c3328 	strcc	r3, [ip, -r8, lsr #6]!
    2228:	2c302c29 	ldccs	12, cr2, [r0], #-164	; 0xffffff5c
    222c:	743b3233 	ldrtvc	r3, [fp], #-563	; 0x233
    2230:	3a657079 	bcc	195e41c <start_address+0x194e41c>
    2234:	372c3328 	strcc	r3, [ip, -r8, lsr #6]!
    2238:	32332c29 	eorscc	r2, r3, #10496	; 0x2900
    223c:	3b32332c 	blcc	c8eef4 <start_address+0xc7eef4>
    2240:	656e776f 	strbvs	r7, [lr, #-1903]!	; 0x76f
    2244:	33283a72 			; <UNDEFINED> instruction: 0x33283a72
    2248:	2c29372c 	stccs	7, cr3, [r9], #-176	; 0xffffff50
    224c:	332c3436 			; <UNDEFINED> instruction: 0x332c3436
    2250:	616e3b32 	cmnvs	lr, r2, lsr fp
    2254:	283a656d 	ldmdacs	sl!, {r0, r2, r3, r5, r6, r8, sl, sp, lr}
    2258:	31312c34 	teqcc	r1, r4, lsr ip
    225c:	36392c29 	ldrtcc	r2, [r9], -r9, lsr #24
    2260:	3430322c 	ldrtcc	r3, [r0], #-556	; 0x22c
    2264:	003b3b38 	eorseq	r3, fp, r8, lsr fp
    2268:	6e495346 	cdpvs	3, 4, cr5, cr9, cr6, {2}
    226c:	3a546f66 	bcc	151e00c <start_address+0x150e00c>
    2270:	34312874 	ldrtcc	r2, [r1], #-2164	; 0x874
    2274:	3d29322c 	sfmcc	f3, 4, [r9, #-176]!	; 0xffffff50
    2278:	2c343128 	ldfcss	f3, [r4], #-160	; 0xffffff60
    227c:	73002931 	movwvc	r2, #2353	; 0x931
    2280:	61637379 	smcvs	14137	; 0x3739
    2284:	755f6c6c 	ldrbvc	r6, [pc, #-3180]	; 1620 <start_address-0xe9e0>
    2288:	50747261 	rsbspl	r7, r4, r1, ror #4
    228c:	68637475 	stmdavs	r3!, {r0, r2, r4, r5, r6, sl, ip, sp, lr}^
    2290:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2294:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2298:	63737973 	cmnvs	r3, #1884160	; 0x1cc000
    229c:	5f6c6c61 	svcpl	0x006c6c61
    22a0:	74726175 	ldrbtvc	r6, [r2], #-373	; 0x175
    22a4:	63746547 	cmnvs	r4, #297795584	; 0x11c00000
    22a8:	28663a68 	stmdacs	r6!, {r3, r5, r6, r9, fp, ip, sp}^
    22ac:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    22b0:	283a7200 	ldmdacs	sl!, {r9, ip, sp, lr}
    22b4:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    22b8:	73797300 	cmnvc	r9, #0, 6
    22bc:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    22c0:	6578655f 	ldrbvs	r6, [r8, #-1375]!	; 0x55f
    22c4:	666c4563 	strbtvs	r4, [ip], -r3, ror #10
    22c8:	3028663a 	eorcc	r6, r8, sl, lsr r6
    22cc:	0029312c 	eoreq	r3, r9, ip, lsr #2
    22d0:	30677261 	rsbcc	r7, r7, r1, ror #4
    22d4:	3028703a 	eorcc	r7, r8, sl, lsr r0
    22d8:	0029312c 	eoreq	r3, r9, ip, lsr #2
    22dc:	31677261 	cmncc	r7, r1, ror #4
    22e0:	3028703a 	eorcc	r7, r8, sl, lsr r0
    22e4:	0029312c 	eoreq	r3, r9, ip, lsr #2
    22e8:	3a646d63 	bcc	191d87c <start_address+0x190d87c>
    22ec:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    22f0:	70002933 	andvc	r2, r0, r3, lsr r9
    22f4:	3231283a 	eorscc	r2, r1, #3801088	; 0x3a0000
    22f8:	0029332c 	eoreq	r3, r9, ip, lsr #6
    22fc:	63737973 	cmnvs	r3, #1884160	; 0x1cc000
    2300:	5f6c6c61 	svcpl	0x006c6c61
    2304:	6b726f66 	blvs	1c9e0a4 <start_address+0x1c8e0a4>
    2308:	3028663a 	eorcc	r6, r8, sl, lsr r6
    230c:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2310:	63737973 	cmnvs	r3, #1884160	; 0x1cc000
    2314:	5f6c6c61 	svcpl	0x006c6c61
    2318:	70746567 	rsbsvc	r6, r4, r7, ror #10
    231c:	663a6469 	ldrtvs	r6, [sl], -r9, ror #8
    2320:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    2324:	79730029 	ldmdbvc	r3!, {r0, r3, r5}^
    2328:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    232c:	78655f6c 	stmdavc	r5!, {r2, r3, r5, r6, r8, r9, sl, fp, ip, lr}^
    2330:	663a7469 	ldrtvs	r7, [sl], -r9, ror #8
    2334:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    2338:	79730029 	ldmdbvc	r3!, {r0, r3, r5}^
    233c:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    2340:	61775f6c 	cmnvs	r7, ip, ror #30
    2344:	463a7469 	ldrtmi	r7, [sl], -r9, ror #8
    2348:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    234c:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    2350:	283a636f 	ldmdacs	sl!, {r0, r1, r2, r3, r5, r6, r8, r9, sp, lr}
    2354:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    2358:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    235c:	29392c34 	ldmdbcs	r9!, {r2, r4, r5, sl, fp, sp}
    2360:	73797300 	cmnvc	r9, #0, 6
    2364:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    2368:	6569795f 	strbvs	r7, [r9, #-2399]!	; 0x95f
    236c:	663a646c 	ldrtvs	r6, [sl], -ip, ror #8
    2370:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    2374:	79730029 	ldmdbvc	r3!, {r0, r3, r5}^
    2378:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    237c:	6d705f6c 	ldclvs	15, cr5, [r0, #-432]!	; 0xfffffe50
    2380:	6f6c6c61 	svcvs	0x006c6c61
    2384:	28663a63 	stmdacs	r6!, {r0, r1, r5, r6, r9, fp, ip, sp}^
    2388:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    238c:	283a7000 	ldmdacs	sl!, {ip, sp, lr}
    2390:	29332c36 	ldmdbcs	r3!, {r1, r2, r4, r5, sl, fp, sp}
    2394:	73797300 	cmnvc	r9, #0, 6
    2398:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    239c:	7266705f 	rsbvc	r7, r6, #95	; 0x5f
    23a0:	663a6565 	ldrtvs	r6, [sl], -r5, ror #10
    23a4:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    23a8:	79730029 	ldmdbvc	r3!, {r0, r3, r5}^
    23ac:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    23b0:	65735f6c 	ldrbvs	r5, [r3, #-3948]!	; 0xf6c
    23b4:	654d646e 	strbvs	r6, [sp, #-1134]	; 0x46e
    23b8:	67617373 			; <UNDEFINED> instruction: 0x67617373
    23bc:	28663a65 	stmdacs	r6!, {r0, r2, r5, r6, r9, fp, ip, sp}^
    23c0:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    23c4:	67726100 	ldrbvs	r6, [r2, -r0, lsl #2]!
    23c8:	28703a32 	ldmdacs	r0!, {r1, r4, r5, r9, fp, ip, sp}^
    23cc:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    23d0:	283a7000 	ldmdacs	sl!, {ip, sp, lr}
    23d4:	29332c37 	ldmdbcs	r3!, {r0, r1, r2, r4, r5, sl, fp, sp}
    23d8:	73797300 	cmnvc	r9, #0, 6
    23dc:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    23e0:	6165725f 	cmnvs	r5, pc, asr r2
    23e4:	73654d64 	cmnvc	r5, #100, 26	; 0x1900
    23e8:	65676173 	strbvs	r6, [r7, #-371]!	; 0x173
    23ec:	3028663a 	eorcc	r6, r8, sl, lsr r6
    23f0:	0029312c 	eoreq	r3, r9, ip, lsr #2
    23f4:	3a676b70 	bcc	19dd1bc <start_address+0x19cd1bc>
    23f8:	332c3728 			; <UNDEFINED> instruction: 0x332c3728
    23fc:	79730029 	ldmdbvc	r3!, {r0, r3, r5}^
    2400:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    2404:	65725f6c 	ldrbvs	r5, [r2, #-3948]!	; 0xf6c
    2408:	6e496461 	cdpvs	4, 4, cr6, cr9, cr1, {3}
    240c:	44527469 	ldrbmi	r7, [r2], #-1129	; 0x469
    2410:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2414:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2418:	6d616e66 	stclvs	14, cr6, [r1, #-408]!	; 0xfffffe68
    241c:	31283a65 			; <UNDEFINED> instruction: 0x31283a65
    2420:	29332c32 	ldmdbcs	r3!, {r1, r4, r5, sl, fp, sp}
    2424:	69736600 	ldmdbvs	r3!, {r9, sl, sp, lr}^
    2428:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    242c:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    2430:	53647200 	cmnpl	r4, #0, 4
    2434:	3a657a69 	bcc	1960de0 <start_address+0x1950de0>
    2438:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    243c:	65720029 	ldrbvs	r0, [r2, #-41]!	; 0x29
    2440:	69537473 	ldmdbvs	r3, {r0, r1, r4, r5, r6, sl, ip, sp, lr}^
    2444:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    2448:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    244c:	74657200 	strbtvc	r7, [r5], #-512	; 0x200
    2450:	2c36283a 	ldccs	8, cr2, [r6], #-232	; 0xffffff18
    2454:	73002933 	movwvc	r2, #2355	; 0x933
    2458:	61637379 	smcvs	14137	; 0x3739
    245c:	665f6c6c 	ldrbvs	r6, [pc], -ip, ror #24
    2460:	73656c69 	cmnvc	r5, #26880	; 0x6900
    2464:	74696e49 	strbtvc	r6, [r9], #-3657	; 0xe49
    2468:	663a4452 			; <UNDEFINED> instruction: 0x663a4452
    246c:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    2470:	3a660029 	bcc	198251c <start_address+0x197251c>
    2474:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    2478:	6a002938 	bvs	c960 <start_address-0x36a0>
    247c:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2480:	63002931 	movwvs	r2, #2353	; 0x931
    2484:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2488:	73002932 	movwvc	r2, #2354	; 0x932
    248c:	61637379 	smcvs	14137	; 0x3739
    2490:	695f6c6c 	ldmdbvs	pc, {r2, r3, r5, r6, sl, fp, sp, lr}^	; <UNPREDICTABLE>
    2494:	496f666e 	stmdbmi	pc!, {r1, r2, r3, r5, r6, r9, sl, sp, lr}^	; <UNPREDICTABLE>
    2498:	5274696e 	rsbspl	r6, r4, #1802240	; 0x1b8000
    249c:	28663a44 	stmdacs	r6!, {r2, r6, r9, fp, ip, sp}^
    24a0:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    24a4:	666e6900 	strbtvs	r6, [lr], -r0, lsl #18
    24a8:	30283a6f 	eorcc	r3, r8, pc, ror #20
    24ac:	2937342c 	ldmdbcs	r7!, {r2, r3, r5, sl, ip, sp}
    24b0:	31282a3d 			; <UNDEFINED> instruction: 0x31282a3d
    24b4:	29322c34 	ldmdbcs	r2!, {r2, r4, r5, sl, fp, sp}
    24b8:	73797300 	cmnvc	r9, #0, 6
    24bc:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    24c0:	62646b5f 	rsbvs	r6, r4, #97280	; 0x17c00
    24c4:	3028663a 	eorcc	r6, r8, sl, lsr r6
    24c8:	0029312c 	eoreq	r3, r9, ip, lsr #2
    24cc:	63737973 	cmnvs	r3, #1884160	; 0x1cc000
    24d0:	5f6c6c61 	svcpl	0x006c6c61
    24d4:	704f6670 	subvc	r6, pc, r0, ror r6	; <UNPREDICTABLE>
    24d8:	663a6e65 	ldrtvs	r6, [sl], -r5, ror #28
    24dc:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    24e0:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    24e4:	283a636f 	ldmdacs	sl!, {r0, r1, r2, r3, r5, r6, r8, r9, sp, lr}
    24e8:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    24ec:	666b0029 	strbtvs	r0, [fp], -r9, lsr #32
    24f0:	2c34283a 	ldccs	8, cr2, [r4], #-232	; 0xffffff18
    24f4:	73002938 	movwvc	r2, #2360	; 0x938
    24f8:	61637379 	smcvs	14137	; 0x3739
    24fc:	705f6c6c 	subsvc	r6, pc, ip, ror #24
    2500:	6f6c4366 	svcvs	0x006c4366
    2504:	663a6573 			; <UNDEFINED> instruction: 0x663a6573
    2508:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    250c:	64660029 	strbtvs	r0, [r6], #-41	; 0x29
    2510:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2514:	73002931 	movwvc	r2, #2353	; 0x931
    2518:	61637379 	smcvs	14137	; 0x3739
    251c:	705f6c6c 	subsvc	r6, pc, ip, ror #24
    2520:	646f4e66 	strbtvs	r4, [pc], #-3686	; 2528 <start_address-0xdad8>
    2524:	28663a65 	stmdacs	r6!, {r0, r2, r5, r6, r9, fp, ip, sp}^
    2528:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    252c:	73797300 	cmnvc	r9, #0, 6
    2530:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    2534:	5366705f 	cmnpl	r6, #95	; 0x5f
    2538:	3a6b6565 	bcc	1adbad4 <start_address+0x1acbad4>
    253c:	2c302866 	ldccs	8, cr2, [r0], #-408	; 0xfffffe68
    2540:	73002931 	movwvc	r2, #2353	; 0x931
    2544:	61637379 	smcvs	14137	; 0x3739
    2548:	705f6c6c 	subsvc	r6, pc, ip, ror #24
    254c:	74654766 	strbtvc	r4, [r5], #-1894	; 0x766
    2550:	6b656553 	blvs	195baa4 <start_address+0x194baa4>
    2554:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2558:	0029312c 	eoreq	r3, r9, ip, lsr #2
    255c:	63737973 	cmnvs	r3, #1884160	; 0x1cc000
    2560:	5f6c6c61 	svcpl	0x006c6c61
    2564:	7265736b 	rsbvc	r7, r5, #-1409286143	; 0xac000001
    2568:	67655276 			; <UNDEFINED> instruction: 0x67655276
    256c:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2570:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2574:	63737973 	cmnvs	r3, #1884160	; 0x1cc000
    2578:	5f6c6c61 	svcpl	0x006c6c61
    257c:	7265736b 	rsbvc	r7, r5, #-1409286143	; 0xac000001
    2580:	74654776 	strbtvc	r4, [r5], #-1910	; 0x776
    2584:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2588:	0029312c 	eoreq	r3, r9, ip, lsr #2
    258c:	50746567 	rsbspl	r6, r4, r7, ror #10
    2590:	73636f72 	cmnvc	r3, #456	; 0x1c8
    2594:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2598:	0029312c 	eoreq	r3, r9, ip, lsr #2
    259c:	656e776f 	strbvs	r7, [lr, #-1903]!	; 0x76f
    25a0:	28703a72 	ldmdacs	r0!, {r1, r4, r5, r6, r9, fp, ip, sp}^
    25a4:	29312c33 	ldmdbcs	r1!, {r0, r1, r4, r5, sl, fp, sp}
    25a8:	73657200 	cmnvc	r5, #0, 4
    25ac:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    25b0:	73002931 	movwvc	r2, #2353	; 0x931
    25b4:	61637379 	smcvs	14137	; 0x3739
    25b8:	675f6c6c 	ldrbvs	r6, [pc, -ip, ror #24]
    25bc:	72507465 	subsvc	r7, r0, #1694498816	; 0x65000000
    25c0:	3a73636f 	bcc	1cdb384 <start_address+0x1ccb384>
    25c4:	2c302866 	ldccs	8, cr2, [r0], #-408	; 0xfffffe68
    25c8:	6f002931 	svcvs	0x00002931
    25cc:	72656e77 	rsbvc	r6, r5, #1904	; 0x770
    25d0:	2c33283a 	ldccs	8, cr2, [r3], #-232	; 0xffffff18
    25d4:	6e002931 	mcrvs	9, 0, r2, cr0, cr1, {1}
    25d8:	283a6d75 	ldmdacs	sl!, {r0, r2, r4, r5, r6, r8, sl, fp, sp, lr}
    25dc:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    25e0:	6f727000 	svcvs	0x00727000
    25e4:	283a7363 	ldmdacs	sl!, {r0, r1, r5, r6, r8, r9, ip, sp, lr}
    25e8:	38342c30 	ldmdacc	r4!, {r4, r5, sl, fp, sp}
    25ec:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    25f0:	312c3031 			; <UNDEFINED> instruction: 0x312c3031
    25f4:	79730029 	ldmdbvc	r3!, {r0, r3, r5}^
    25f8:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    25fc:	65675f6c 	strbvs	r5, [r7, #-3948]!	; 0xf6c
    2600:	44574374 	ldrbmi	r4, [r7], #-884	; 0x374
    2604:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2608:	0029312c 	eoreq	r3, r9, ip, lsr #2
    260c:	3a647770 	bcc	19203d4 <start_address+0x19103d4>
    2610:	332c3628 			; <UNDEFINED> instruction: 0x332c3628
    2614:	79730029 	ldmdbvc	r3!, {r0, r3, r5}^
    2618:	6c616373 	stclvs	3, cr6, [r1], #-460	; 0xfffffe34
    261c:	65735f6c 	ldrbvs	r5, [r3, #-3948]!	; 0xf6c
    2620:	44574374 	ldrbmi	r4, [r7], #-884	; 0x374
    2624:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2628:	0029312c 	eoreq	r3, r9, ip, lsr #2
    262c:	3a647770 	bcc	19203f4 <start_address+0x19103f4>
    2630:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    2634:	73002933 	movwvc	r2, #2355	; 0x933
    2638:	61637379 	smcvs	14137	; 0x3739
    263c:	675f6c6c 	ldrbvs	r6, [pc, -ip, ror #24]
    2640:	6d437465 	cfstrdvs	mvd7, [r3, #-404]	; 0xfffffe6c
    2644:	28663a64 	stmdacs	r6!, {r2, r5, r6, r9, fp, ip, sp}^
    2648:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    264c:	646d6300 	strbtvs	r6, [sp], #-768	; 0x300
    2650:	2c36283a 	ldccs	8, cr2, [r6], #-232	; 0xffffff18
    2654:	73002933 	movwvc	r2, #2355	; 0x933
    2658:	61637379 	smcvs	14137	; 0x3739
    265c:	735f6c6c 	cmpvc	pc, #108, 24	; 0x6c00
    2660:	49557465 	ldmdbmi	r5, {r0, r2, r5, r6, sl, ip, sp, lr}^
    2664:	28663a44 	stmdacs	r6!, {r2, r6, r9, fp, ip, sp}^
    2668:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    266c:	73797300 	cmnvc	r9, #0, 6
    2670:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    2674:	7465675f 	strbtvc	r6, [r5], #-1887	; 0x75f
    2678:	3a444955 	bcc	1114bd4 <start_address+0x1104bd4>
    267c:	2c302866 	ldccs	8, cr2, [r0], #-408	; 0xfffffe68
    2680:	68002931 	stmdavs	r0, {r0, r4, r5, r8, fp, sp}
    2684:	6c646e61 	stclvs	14, cr6, [r4], #-388	; 0xfffffe7c
    2688:	73795365 	cmnvc	r9, #-1811939327	; 0x94000001
    268c:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    2690:	3028463a 	eorcc	r4, r8, sl, lsr r6
    2694:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2698:	65646f63 	strbvs	r6, [r4, #-3939]!	; 0xf63
    269c:	3028703a 	eorcc	r7, r8, sl, lsr r0
    26a0:	0029312c 	eoreq	r3, r9, ip, lsr #2
    26a4:	5f474b50 	svcpl	0x00474b50
    26a8:	45505954 	ldrbmi	r5, [r0, #-2388]	; 0x954
    26ac:	5252455f 	subspl	r4, r2, #398458880	; 0x17c00000
    26b0:	3028533a 	eorcc	r5, r8, sl, lsr r3
    26b4:	2939342c 	ldmdbcs	r9!, {r2, r3, r5, sl, ip, sp}
    26b8:	33286b3d 			; <UNDEFINED> instruction: 0x33286b3d
    26bc:	0029372c 	eoreq	r3, r9, ip, lsr #14
    26c0:	7379735f 	cmnvc	r9, #2080374785	; 0x7c000001
    26c4:	6c6c6163 	stfvse	f6, [ip], #-396	; 0xfffffe74
    26c8:	646e6148 	strbtvs	r6, [lr], #-328	; 0x148
    26cc:	3a72656c 	bcc	1c9bc84 <start_address+0x1c8bc84>
    26d0:	2c302853 	ldccs	8, cr2, [r0], #-332	; 0xfffffeb4
    26d4:	3d293035 	stccc	0, cr3, [r9, #-212]!	; 0xffffff2c
    26d8:	31287261 			; <UNDEFINED> instruction: 0x31287261
    26dc:	29342c30 	ldmdbcs	r4!, {r4, r5, sl, fp, sp}
    26e0:	323b303b 	eorscc	r3, fp, #59	; 0x3b
    26e4:	30283b38 	eorcc	r3, r8, r8, lsr fp
    26e8:	2931352c 	ldmdbcs	r1!, {r2, r3, r5, r8, sl, ip, sp}
    26ec:	30286b3d 	eorcc	r6, r8, sp, lsr fp
    26f0:	2932352c 	ldmdbcs	r2!, {r2, r3, r5, r8, sl, ip, sp}
    26f4:	30282a3d 	eorcc	r2, r8, sp, lsr sl
    26f8:	2933352c 	ldmdbcs	r3!, {r2, r3, r5, r8, sl, ip, sp}
    26fc:	3028663d 	eorcc	r6, r8, sp, lsr r6
    2700:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2704:	2f637273 	svccs	0x00637273
    2708:	732f6d6d 			; <UNDEFINED> instruction: 0x732f6d6d
    270c:	74726174 	ldrbtvc	r6, [r2], #-372	; 0x174
    2710:	632e7075 			; <UNDEFINED> instruction: 0x632e7075
    2714:	6e695f00 	cdpvs	15, 6, cr5, cr9, cr0, {0}
    2718:	74537469 	ldrbvc	r7, [r3], #-1129	; 0x469
    271c:	3a6b6361 	bcc	1adb4a8 <start_address+0x1acb4a8>
    2720:	2c302847 	ldccs	8, cr2, [r0], #-284	; 0xfffffee4
    2724:	3d293634 	stccc	6, cr3, [r9, #-208]!	; 0xffffff30
    2728:	30287261 	eorcc	r7, r8, r1, ror #4
    272c:	2937342c 	ldmdbcs	r7!, {r2, r3, r5, sl, ip, sp}
    2730:	3028723d 	eorcc	r7, r8, sp, lsr r2
    2734:	2937342c 	ldmdbcs	r7!, {r2, r3, r5, sl, ip, sp}
    2738:	303b303b 	eorscc	r3, fp, fp, lsr r0
    273c:	37373733 			; <UNDEFINED> instruction: 0x37373733
    2740:	37373737 			; <UNDEFINED> instruction: 0x37373737
    2744:	3b373737 	blcc	dd0428 <start_address+0xdc0428>
    2748:	343b303b 	ldrtcc	r3, [fp], #-59	; 0x3b
    274c:	3b353930 	blcc	d50c14 <start_address+0xd40c14>
    2750:	322c3028 	eorcc	r3, ip, #40	; 0x28
    2754:	695f0029 	ldmdbvs	pc, {r0, r3, r5}^	; <UNPREDICTABLE>
    2758:	74537172 	ldrbvc	r7, [r3], #-370	; 0x172
    275c:	3a6b6361 	bcc	1adb4e8 <start_address+0x1acb4e8>
    2760:	2c302847 	ldccs	8, cr2, [r0], #-284	; 0xfffffee4
    2764:	00293634 	eoreq	r3, r9, r4, lsr r6
    2768:	6174735f 	cmnvs	r4, pc, asr r3
    276c:	70757472 	rsbsvc	r7, r5, r2, ror r4
    2770:	65676150 	strbvs	r6, [r7, #-336]!	; 0x150
    2774:	3a726944 	bcc	1c9cc8c <start_address+0x1c8cc8c>
    2778:	2c302847 	ldccs	8, cr2, [r0], #-284	; 0xfffffee4
    277c:	3d293834 	stccc	8, cr3, [r9, #-208]!	; 0xffffff30
    2780:	30287261 	eorcc	r7, r8, r1, ror #4
    2784:	2937342c 	ldmdbcs	r7!, {r2, r3, r5, sl, ip, sp}
    2788:	343b303b 	ldrtcc	r3, [fp], #-59	; 0x3b
    278c:	3b353930 	blcc	d50c54 <start_address+0xd40c54>
    2790:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    2794:	72730029 	rsbsvc	r0, r3, #41	; 0x29
    2798:	6d6d2f63 	stclvs	15, cr2, [sp, #-396]!	; 0xfffffe74
    279c:	756d6d2f 	strbvc	r6, [sp, #-3375]!	; 0xd2f
    27a0:	6d00632e 	stcvs	3, cr6, [r0, #-184]	; 0xffffff48
    27a4:	61507061 	cmpvs	r0, r1, rrx
    27a8:	3a736567 	bcc	1cdbd4c <start_address+0x1ccbd4c>
    27ac:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    27b0:	00293534 	eoreq	r3, r9, r4, lsr r5
    27b4:	703a6d76 	eorsvc	r6, sl, r6, ror sp
    27b8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    27bc:	2a3d2936 	bcs	f4cc9c <start_address+0xf3cc9c>
    27c0:	312c3128 			; <UNDEFINED> instruction: 0x312c3128
    27c4:	61760029 	cmnvs	r6, r9, lsr #32
    27c8:	3a726464 	bcc	1c9b960 <start_address+0x1c8b960>
    27cc:	2c322870 	ldccs	8, cr2, [r2], #-448	; 0xfffffe40
    27d0:	70002937 	andvc	r2, r0, r7, lsr r9
    27d4:	72617473 	rsbvc	r7, r1, #1929379840	; 0x73000000
    27d8:	28703a74 	ldmdacs	r0!, {r2, r4, r5, r6, r9, fp, ip, sp}^
    27dc:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    27e0:	6e657000 	cdpvs	0, 6, cr7, cr5, cr0, {0}
    27e4:	28703a64 	ldmdacs	r0!, {r2, r5, r6, r9, fp, ip, sp}^
    27e8:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    27ec:	72657000 	rsbvc	r7, r5, #0
    27f0:	7373696d 	cmnvc	r3, #1785856	; 0x1b4000
    27f4:	736e6f69 	cmnvc	lr, #420	; 0x1a4
    27f8:	3028703a 	eorcc	r7, r8, sl, lsr r0
    27fc:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2800:	73796870 	cmnvc	r9, #112, 16	; 0x700000
    2804:	6c616369 	stclvs	3, cr6, [r1], #-420	; 0xfffffe5c
    2808:	72727543 	rsbsvc	r7, r2, #281018368	; 0x10c00000
    280c:	3a746e65 	bcc	1d1e1a8 <start_address+0x1d0e1a8>
    2810:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    2814:	69760029 	ldmdbvs	r6!, {r0, r3, r5}^
    2818:	61757472 	cmnvs	r5, r2, ror r4
    281c:	7275436c 	rsbsvc	r4, r5, #108, 6	; 0xb0000001
    2820:	746e6572 	strbtvc	r6, [lr], #-1394	; 0x572
    2824:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    2828:	76002937 			; <UNDEFINED> instruction: 0x76002937
    282c:	75747269 	ldrbvc	r7, [r4, #-617]!	; 0x269
    2830:	74536c61 	ldrbvc	r6, [r3], #-3169	; 0xc61
    2834:	3a747261 	bcc	1d1f1c0 <start_address+0x1d0f1c0>
    2838:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    283c:	68700029 	ldmdavs	r0!, {r0, r3, r5}^
    2840:	63697379 	cmnvs	r9, #-469762047	; 0xe4000001
    2844:	74536c61 	ldrbvc	r6, [r3], #-3169	; 0xc61
    2848:	3a747261 	bcc	1d1f1d4 <start_address+0x1d0f1d4>
    284c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    2850:	68700029 	ldmdavs	r0!, {r0, r3, r5}^
    2854:	63697379 	cmnvs	r9, #-469762047	; 0xe4000001
    2858:	6e456c61 	cdpvs	12, 4, cr6, cr5, cr1, {3}
    285c:	32283a64 	eorcc	r3, r8, #100, 20	; 0x64000
    2860:	0029372c 	eoreq	r3, r9, ip, lsr #14
    2864:	5070616d 	rsbspl	r6, r0, sp, ror #2
    2868:	3a656761 	bcc	195c5f4 <start_address+0x194c5f4>
    286c:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    2870:	00293534 	eoreq	r3, r9, r4, lsr r5
    2874:	703a6d76 	eorsvc	r6, sl, r6, ror sp
    2878:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    287c:	76002936 			; <UNDEFINED> instruction: 0x76002936
    2880:	75747269 	ldrbvc	r7, [r4, #-617]!	; 0x269
    2884:	64416c61 	strbvs	r6, [r1], #-3169	; 0xc61
    2888:	703a7264 	eorsvc	r7, sl, r4, ror #4
    288c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    2890:	68700029 	ldmdavs	r0!, {r0, r3, r5}^
    2894:	63697379 	cmnvs	r9, #-469762047	; 0xe4000001
    2898:	703a6c61 	eorsvc	r6, sl, r1, ror #24
    289c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    28a0:	61700029 	cmnvs	r0, r9, lsr #32
    28a4:	61546567 	cmpvs	r4, r7, ror #10
    28a8:	3a656c62 	bcc	195da38 <start_address+0x194da38>
    28ac:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    28b0:	2a3d2937 	bcs	f4cd94 <start_address+0xf3cd94>
    28b4:	332c3128 			; <UNDEFINED> instruction: 0x332c3128
    28b8:	61700029 	cmnvs	r0, r9, lsr #32
    28bc:	69446567 	stmdbvs	r4, {r0, r1, r2, r5, r6, r8, sl, sp, lr}^
    28c0:	646e4972 	strbtvs	r4, [lr], #-2418	; 0x972
    28c4:	283a7865 	ldmdacs	sl!, {r0, r2, r5, r6, fp, ip, sp, lr}
    28c8:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    28cc:	67617000 	strbvs	r7, [r1, -r0]!
    28d0:	646e4965 	strbtvs	r4, [lr], #-2405	; 0x965
    28d4:	283a7865 	ldmdacs	sl!, {r0, r2, r5, r6, fp, ip, sp, lr}
    28d8:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    28dc:	6d6e7500 	cfstr64vs	mvdx7, [lr, #-0]
    28e0:	61507061 	cmpvs	r0, r1, rrx
    28e4:	463a6567 	ldrtmi	r6, [sl], -r7, ror #10
    28e8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    28ec:	70002935 	andvc	r2, r0, r5, lsr r9
    28f0:	54656761 	strbtpl	r6, [r5], #-1889	; 0x761
    28f4:	656c6261 	strbvs	r6, [ip, #-609]!	; 0x261
    28f8:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    28fc:	00293734 	eoreq	r3, r9, r4, lsr r7
    2900:	6f736572 	svcvs	0x00736572
    2904:	5065766c 	rsbpl	r7, r5, ip, ror #12
    2908:	64417968 	strbvs	r7, [r1], #-2408	; 0x968
    290c:	73657264 	cmnvc	r5, #100, 4	; 0x40000006
    2910:	28463a73 	stmdacs	r6, {r0, r1, r4, r5, r6, r9, fp, ip, sp}^
    2914:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2918:	72697600 	rsbvc	r7, r9, #0, 12
    291c:	6c617574 	cfstr64vs	mvdx7, [r1], #-464	; 0xfffffe30
    2920:	3228703a 	eorcc	r7, r8, #58	; 0x3a
    2924:	0029372c 	eoreq	r3, r9, ip, lsr #14
    2928:	72696470 	rsbvc	r6, r9, #112, 8	; 0x70000000
    292c:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2930:	00293634 	eoreq	r3, r9, r4, lsr r6
    2934:	65676170 	strbvs	r6, [r7, #-368]!	; 0x170
    2938:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    293c:	00293734 	eoreq	r3, r9, r4, lsr r7
    2940:	75736572 	ldrbvc	r6, [r3, #-1394]!	; 0x572
    2944:	283a746c 	ldmdacs	sl!, {r2, r3, r5, r6, sl, ip, sp, lr}
    2948:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    294c:	73616200 	cmnvc	r1, #0, 4
    2950:	64644165 	strbtvs	r4, [r4], #-357	; 0x165
    2954:	73736572 	cmnvc	r3, #478150656	; 0x1c800000
    2958:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    295c:	3d293834 	stccc	8, cr3, [r9, #-208]!	; 0xffffff30
    2960:	2c30282a 	ldccs	8, cr2, [r0], #-168	; 0xffffff58
    2964:	00293534 	eoreq	r3, r9, r4, lsr r5
    2968:	65657266 	strbvs	r7, [r5, #-614]!	; 0x266
    296c:	65676150 	strbvs	r6, [r7, #-336]!	; 0x150
    2970:	6c626154 	stfvse	f6, [r2], #-336	; 0xfffffeb0
    2974:	463a7365 	ldrtmi	r7, [sl], -r5, ror #6
    2978:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    297c:	70002935 	andvc	r2, r0, r5, lsr r9
    2980:	54656761 	strbtpl	r6, [r5], #-1889	; 0x761
    2984:	656c6261 	strbvs	r6, [ip, #-609]!	; 0x261
    2988:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    298c:	00293834 	eoreq	r3, r9, r4, lsr r8
    2990:	2f637273 	svccs	0x00637273
    2994:	6b2f6d6d 	blvs	bddf50 <start_address+0xbcdf50>
    2998:	6f6c6c61 	svcvs	0x006c6c61
    299c:	00632e63 	rsbeq	r2, r3, r3, ror #28
    29a0:	6c6c616b 	stfvse	f6, [ip], #-428	; 0xfffffe54
    29a4:	6e49636f 	cdpvs	3, 4, cr6, cr9, cr15, {3}
    29a8:	463a7469 	ldrtmi	r7, [sl], -r9, ror #8
    29ac:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    29b0:	73002935 	movwvc	r2, #2357	; 0x935
    29b4:	74726174 	ldrbtvc	r6, [r2], #-372	; 0x174
    29b8:	3228703a 	eorcc	r7, r8, #58	; 0x3a
    29bc:	0029372c 	eoreq	r3, r9, ip, lsr #14
    29c0:	3a646e65 	bcc	191e35c <start_address+0x190e35c>
    29c4:	2c322870 	ldccs	8, cr2, [r2], #-448	; 0xfffffe40
    29c8:	73002937 	movwvc	r2, #2359	; 0x937
    29cc:	74726174 	ldrbtvc	r6, [r2], #-372	; 0x174
    29d0:	72646441 	rsbvc	r6, r4, #1090519040	; 0x41000000
    29d4:	3a737365 	bcc	1cdf770 <start_address+0x1ccf770>
    29d8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    29dc:	2a3d2936 	bcs	f4cebc <start_address+0xf3cebc>
    29e0:	322c3028 	eorcc	r3, ip, #40	; 0x28
    29e4:	6e650029 	cdpvs	0, 6, cr0, cr5, cr9, {1}
    29e8:	64644164 	strbtvs	r4, [r4], #-356	; 0x164
    29ec:	73736572 	cmnvc	r3, #478150656	; 0x1c800000
    29f0:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    29f4:	00293634 	eoreq	r3, r9, r4, lsr r6
    29f8:	72727563 	rsbsvc	r7, r2, #415236096	; 0x18c00000
    29fc:	50746e65 	rsbspl	r6, r4, r5, ror #28
    2a00:	3a656761 	bcc	195c78c <start_address+0x194c78c>
    2a04:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    2a08:	6b002936 	blvs	cee8 <start_address-0x3118>
    2a0c:	6f6c6c61 	svcvs	0x006c6c61
    2a10:	28463a63 	stmdacs	r6, {r0, r1, r5, r6, r9, fp, ip, sp}^
    2a14:	37342c30 			; <UNDEFINED> instruction: 0x37342c30
    2a18:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    2a1c:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    2a20:	65720029 	ldrbvs	r0, [r2, #-41]!	; 0x29
    2a24:	746c7573 	strbtvc	r7, [ip], #-1395	; 0x573
    2a28:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2a2c:	00293734 	eoreq	r3, r9, r4, lsr r7
    2a30:	6572666b 	ldrbvs	r6, [r2, #-1643]!	; 0x66b
    2a34:	28463a65 	stmdacs	r6, {r0, r2, r5, r6, r9, fp, ip, sp}^
    2a38:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    2a3c:	61700029 	cmnvs	r0, r9, lsr #32
    2a40:	703a6567 	eorsvc	r6, sl, r7, ror #10
    2a44:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    2a48:	6b002937 	blvs	cf2c <start_address-0x30d4>
    2a4c:	6f6c6c61 	svcvs	0x006c6c61
    2a50:	3a6b3163 	bcc	1acefe4 <start_address+0x1abefe4>
    2a54:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    2a58:	00293734 	eoreq	r3, r9, r4, lsr r7
    2a5c:	65676170 	strbvs	r6, [r7, #-368]!	; 0x170
    2a60:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2a64:	00293634 	eoreq	r3, r9, r4, lsr r6
    2a68:	6572666b 	ldrbvs	r6, [r2, #-1643]!	; 0x66b
    2a6c:	3a6b3165 	bcc	1acf008 <start_address+0x1abf008>
    2a70:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    2a74:	00293534 	eoreq	r3, r9, r4, lsr r5
    2a78:	3a6d656d 	bcc	1b5c034 <start_address+0x1b4c034>
    2a7c:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    2a80:	00293734 	eoreq	r3, r9, r4, lsr r7
    2a84:	46746567 	ldrbtmi	r6, [r4], -r7, ror #10
    2a88:	4d656572 	cfstr64mi	mvdx6, [r5, #-456]!	; 0xfffffe38
    2a8c:	726f6d65 	rsbvc	r6, pc, #6464	; 0x1940
    2a90:	7a695379 	bvc	1a5787c <start_address+0x1a4787c>
    2a94:	28463a65 	stmdacs	r6, {r0, r2, r5, r6, r9, fp, ip, sp}^
    2a98:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2a9c:	72756300 	rsbsvc	r6, r5, #0, 6
    2aa0:	746e6572 	strbtvc	r6, [lr], #-1394	; 0x572
    2aa4:	65676150 	strbvs	r6, [r7, #-336]!	; 0x150
    2aa8:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2aac:	3d293834 	stccc	8, cr3, [r9, #-208]!	; 0xffffff30
    2ab0:	2c31282a 	ldccs	8, cr2, [r1], #-168	; 0xffffff58
    2ab4:	70002933 	andvc	r2, r0, r3, lsr r9
    2ab8:	4c656761 	stclmi	7, cr6, [r5], #-388	; 0xfffffe7c
    2abc:	50747369 	rsbspl	r7, r4, r9, ror #6
    2ac0:	65706572 	ldrbvs	r6, [r0, #-1394]!	; 0x572
    2ac4:	663a646e 	ldrtvs	r6, [sl], -lr, ror #8
    2ac8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    2acc:	70002938 	andvc	r2, r0, r8, lsr r9
    2ad0:	4c656761 	stclmi	7, cr6, [r5], #-388	; 0xfffffe7c
    2ad4:	3a747369 	bcc	1d1f880 <start_address+0x1d0f880>
    2ad8:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    2adc:	00293834 	eoreq	r3, r9, r4, lsr r8
    2ae0:	65676170 	strbvs	r6, [r7, #-368]!	; 0x170
    2ae4:	72646441 	rsbvc	r6, r4, #1090519040	; 0x41000000
    2ae8:	3a737365 	bcc	1cdf884 <start_address+0x1ccf884>
    2aec:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    2af0:	00293634 	eoreq	r3, r9, r4, lsr r6
    2af4:	65676170 	strbvs	r6, [r7, #-368]!	; 0x170
    2af8:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2afc:	00293834 	eoreq	r3, r9, r4, lsr r8
    2b00:	65657266 	strbvs	r7, [r5, #-614]!	; 0x266
    2b04:	7473694c 	ldrbtvc	r6, [r3], #-2380	; 0x94c
    2b08:	533a6b34 	teqpl	sl, #52, 22	; 0xd000
    2b0c:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    2b10:	66002938 			; <UNDEFINED> instruction: 0x66002938
    2b14:	4c656572 	cfstr64mi	mvdx6, [r5], #-456	; 0xfffffe38
    2b18:	31747369 	cmncc	r4, r9, ror #6
    2b1c:	28533a6b 	ldmdacs	r3, {r0, r1, r3, r5, r6, r9, fp, ip, sp}^
    2b20:	38342c30 	ldmdacc	r4!, {r4, r5, sl, fp, sp}
    2b24:	72730029 	rsbsvc	r0, r3, #41	; 0x29
    2b28:	6d6d2f63 	stclvs	15, cr2, [sp, #-396]!	; 0xfffffe74
    2b2c:	7572742f 	ldrbvc	r7, [r2, #-1071]!	; 0x42f
    2b30:	616d6b6e 	cmnvs	sp, lr, ror #22
    2b34:	636f6c6c 	cmnvs	pc, #108, 24	; 0x6c00
    2b38:	6700632e 	strvs	r6, [r0, -lr, lsr #6]
    2b3c:	6c426e65 	mcrrvs	14, 6, r6, r2, cr5
    2b40:	3a6b636f 	bcc	1adb904 <start_address+0x1acb904>
    2b44:	2c312866 	ldccs	8, cr2, [r1], #-408	; 0xfffffe68
    2b48:	00293431 	eoreq	r3, r9, r1, lsr r4
    2b4c:	28703a70 	ldmdacs	r0!, {r4, r5, r6, r9, fp, ip, sp}^
    2b50:	29332c31 	ldmdbcs	r3!, {r0, r4, r5, sl, fp, sp}
    2b54:	7a697300 	bvc	1a5f75c <start_address+0x1a4f75c>
    2b58:	28703a65 	ldmdacs	r0!, {r0, r2, r5, r6, r9, fp, ip, sp}^
    2b5c:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2b60:	6f6c6200 	svcvs	0x006c6200
    2b64:	69536b63 	ldmdbvs	r3, {r0, r1, r5, r6, r8, r9, fp, sp, lr}^
    2b68:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    2b6c:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2b70:	6f6c6200 	svcvs	0x006c6200
    2b74:	283a6b63 	ldmdacs	sl!, {r0, r1, r5, r6, r8, r9, fp, sp, lr}
    2b78:	34312c31 	ldrtcc	r2, [r1], #-3121	; 0xc31
    2b7c:	72740029 	rsbsvc	r0, r4, #41	; 0x29
    2b80:	65724279 	ldrbvs	r4, [r2, #-633]!	; 0x279
    2b84:	663a6b61 	ldrtvs	r6, [sl], -r1, ror #22
    2b88:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    2b8c:	6d002935 	stcvs	9, cr2, [r0, #-212]	; 0xffffff2c
    2b90:	3028703a 	eorcc	r7, r8, sl, lsr r0
    2b94:	2936342c 	ldmdbcs	r6!, {r2, r3, r5, sl, ip, sp}
    2b98:	31282a3d 			; <UNDEFINED> instruction: 0x31282a3d
    2b9c:	0029352c 	eoreq	r3, r9, ip, lsr #10
    2ba0:	636f6c62 	cmnvs	pc, #25088	; 0x6200
    2ba4:	28703a6b 	ldmdacs	r0!, {r0, r1, r3, r5, r6, r9, fp, ip, sp}^
    2ba8:	34312c31 	ldrtcc	r2, [r1], #-3121	; 0xc31
    2bac:	3a700029 	bcc	1c02c58 <start_address+0x1bf2c58>
    2bb0:	332c3128 			; <UNDEFINED> instruction: 0x332c3128
    2bb4:	656e0029 	strbvs	r0, [lr, #-41]!	; 0x29
    2bb8:	6f6c4277 	svcvs	0x006c4277
    2bbc:	283a6b63 	ldmdacs	sl!, {r0, r1, r5, r6, r8, r9, fp, sp, lr}
    2bc0:	34312c31 	ldrtcc	r2, [r1], #-3121	; 0xc31
    2bc4:	72740029 	rsbsvc	r0, r4, #41	; 0x29
    2bc8:	4d6b6e75 	stclmi	14, cr6, [fp, #-468]!	; 0xfffffe2c
    2bcc:	6f6c6c61 	svcvs	0x006c6c61
    2bd0:	28463a63 	stmdacs	r6, {r0, r1, r5, r6, r9, fp, ip, sp}^
    2bd4:	29332c31 	ldmdbcs	r3!, {r0, r4, r5, sl, fp, sp}
    2bd8:	703a6d00 	eorsvc	r6, sl, r0, lsl #26
    2bdc:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    2be0:	65002936 	strvs	r2, [r0, #-2358]	; 0x936
    2be4:	6e617078 	mcrvs	0, 3, r7, cr1, cr8, {3}
    2be8:	7a695364 	bvc	1a57980 <start_address+0x1a47980>
    2bec:	32283a65 	eorcc	r3, r8, #413696	; 0x65000
    2bf0:	0029372c 	eoreq	r3, r9, ip, lsr #14
    2bf4:	65676170 	strbvs	r6, [r7, #-368]!	; 0x170
    2bf8:	32283a73 	eorcc	r3, r8, #471040	; 0x73000
    2bfc:	0029372c 	eoreq	r3, r9, ip, lsr #14
    2c00:	4d797274 	lfmmi	f7, 2, [r9, #-464]!	; 0xfffffe30
    2c04:	65677265 	strbvs	r7, [r7, #-613]!	; 0x265
    2c08:	3028663a 	eorcc	r6, r8, sl, lsr r6
    2c0c:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    2c10:	283a6200 	ldmdacs	sl!, {r9, sp, lr}
    2c14:	34312c31 	ldrtcc	r2, [r1], #-3121	; 0xc31
    2c18:	72740029 	rsbsvc	r0, r4, #41	; 0x29
    2c1c:	72685379 	rsbvc	r5, r8, #-469762047	; 0xe4000001
    2c20:	3a6b6e69 	bcc	1ade5cc <start_address+0x1ace5cc>
    2c24:	2c302866 	ldccs	8, cr2, [r0], #-408	; 0xfffffe68
    2c28:	00293534 	eoreq	r3, r9, r4, lsr r5
    2c2c:	72646461 	rsbvc	r6, r4, #1627389952	; 0x61000000
    2c30:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    2c34:	70002937 	andvc	r2, r0, r7, lsr r9
    2c38:	73656761 	cmnvc	r5, #25427968	; 0x1840000
    2c3c:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    2c40:	74002931 	strvc	r2, [r0], #-2353	; 0x931
    2c44:	6b6e7572 	blvs	1ba0214 <start_address+0x1b90214>
    2c48:	65657246 	strbvs	r7, [r5, #-582]!	; 0x246
    2c4c:	3028463a 	eorcc	r4, r8, sl, lsr r6
    2c50:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    2c54:	63727300 	cmnvs	r2, #0, 6
    2c58:	2f6d6d2f 	svccs	0x006d6d2f
    2c5c:	6c616d6b 	stclvs	13, cr6, [r1], #-428	; 0xfffffe54
    2c60:	2e636f6c 	cdpcs	15, 6, cr6, cr3, cr12, {3}
    2c64:	6e690063 	cdpvs	0, 6, cr0, cr9, cr3, {3}
    2c68:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
    2c6c:	6d6d2f65 	stclvs	15, cr2, [sp, #-404]!	; 0xfffffe6c
    2c70:	616d6b2f 	cmnvs	sp, pc, lsr #22
    2c74:	636f6c6c 	cmnvs	pc, #108, 24	; 0x6c00
    2c78:	6b00682e 	blvs	1cd38 <start_address+0xcd38>
    2c7c:	7268536d 	rsbvc	r5, r8, #-1275068415	; 0xb4000001
    2c80:	3a6b6e69 	bcc	1ade62c <start_address+0x1ace62c>
    2c84:	2c302866 	ldccs	8, cr2, [r0], #-408	; 0xfffffe68
    2c88:	00293534 	eoreq	r3, r9, r4, lsr r5
    2c8c:	3a677261 	bcc	19df618 <start_address+0x19cf618>
    2c90:	2c332870 	ldccs	8, cr2, [r3], #-448	; 0xfffffe40
    2c94:	70002937 	andvc	r2, r0, r7, lsr r9
    2c98:	73656761 	cmnvc	r5, #25427968	; 0x1840000
    2c9c:	3028703a 	eorcc	r7, r8, sl, lsr r0
    2ca0:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2ca4:	78456d6b 	stmdavc	r5, {r0, r1, r3, r5, r6, r8, sl, fp, sp, lr}^
    2ca8:	646e6170 	strbtvs	r6, [lr], #-368	; 0x170
    2cac:	3228663a 	eorcc	r6, r8, #60817408	; 0x3a00000
    2cb0:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2cb4:	283a6f74 	ldmdacs	sl!, {r2, r4, r5, r6, r8, r9, sl, fp, sp, lr}
    2cb8:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2cbc:	476d6b00 	strbmi	r6, [sp, -r0, lsl #22]!
    2cc0:	654d7465 	strbvs	r7, [sp, #-1125]	; 0x465
    2cc4:	6961546d 	stmdbvs	r1!, {r0, r2, r3, r5, r6, sl, ip, lr}^
    2cc8:	28663a6c 	stmdacs	r6!, {r2, r3, r5, r6, r9, fp, ip, sp}^
    2ccc:	29372c33 	ldmdbcs	r7!, {r0, r1, r4, r5, sl, fp, sp}
    2cd0:	496d6b00 	stmdbmi	sp!, {r8, r9, fp, sp, lr}^
    2cd4:	3a74696e 	bcc	1d1d294 <start_address+0x1d0d294>
    2cd8:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    2cdc:	00293534 	eoreq	r3, r9, r4, lsr r5
    2ce0:	6c616d6b 	stclvs	13, cr6, [r1], #-428	; 0xfffffe54
    2ce4:	3a636f6c 	bcc	18dea9c <start_address+0x18cea9c>
    2ce8:	2c332846 	ldccs	8, cr2, [r3], #-280	; 0xfffffee8
    2cec:	6b002937 	blvs	d1d0 <start_address-0x2e30>
    2cf0:	6572666d 	ldrbvs	r6, [r2, #-1645]!	; 0x66d
    2cf4:	28463a65 	stmdacs	r6, {r0, r2, r5, r6, r9, fp, ip, sp}^
    2cf8:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    2cfc:	3a700029 	bcc	1c02da8 <start_address+0x1bf2da8>
    2d00:	2c332870 	ldccs	8, cr2, [r3], #-448	; 0xfffffe40
    2d04:	5f002937 	svcpl	0x00002937
    2d08:	6c614d6b 	stclvs	13, cr4, [r1], #-428	; 0xfffffe54
    2d0c:	3a636f6c 	bcc	18deac4 <start_address+0x18ceac4>
    2d10:	2c332853 	ldccs	8, cr2, [r3], #-332	; 0xfffffeb4
    2d14:	5f002935 	svcpl	0x00002935
    2d18:	6c614d6b 	stclvs	13, cr4, [r1], #-428	; 0xfffffe54
    2d1c:	4d636f6c 	stclmi	15, cr6, [r3, #-432]!	; 0xfffffe50
    2d20:	61546d65 	cmpvs	r4, r5, ror #26
    2d24:	533a6c69 	teqpl	sl, #26880	; 0x6900
    2d28:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    2d2c:	72730029 	rsbsvc	r0, r3, #41	; 0x29
    2d30:	72702f63 	rsbsvc	r2, r0, #396	; 0x18c
    2d34:	632e636f 			; <UNDEFINED> instruction: 0x632e636f
    2d38:	636e6900 	cmnvs	lr, #0, 18
    2d3c:	6564756c 	strbvs	r7, [r4, #-1388]!	; 0x56c
    2d40:	666c652f 	strbtvs	r6, [ip], -pc, lsr #10
    2d44:	4500682e 	strmi	r6, [r0, #-2094]	; 0x82e
    2d48:	7954666c 	ldmdbvc	r4, {r2, r3, r5, r6, r9, sl, sp, lr}^
    2d4c:	543a6570 	ldrtpl	r6, [sl], #-1392	; 0x570
    2d50:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    2d54:	403d2931 	eorsmi	r2, sp, r1, lsr r9
    2d58:	653b3873 	ldrvs	r3, [fp, #-2163]!	; 0x873
    2d5c:	54464c45 	strbpl	r4, [r6], #-3141	; 0xc45
    2d60:	5f455059 	svcpl	0x00455059
    2d64:	454e4f4e 	strbmi	r4, [lr, #-3918]	; 0xf4e
    2d68:	452c303a 	strmi	r3, [ip, #-58]!	; 0x3a
    2d6c:	5954464c 	ldmdbpl	r4, {r2, r3, r6, r9, sl, lr}^
    2d70:	525f4550 	subspl	r4, pc, #80, 10	; 0x14000000
    2d74:	434f4c45 	movtmi	r4, #64581	; 0xfc45
    2d78:	42415441 	submi	r5, r1, #1090519040	; 0x41000000
    2d7c:	313a454c 	teqcc	sl, ip, asr #10
    2d80:	464c452c 	strbmi	r4, [ip], -ip, lsr #10
    2d84:	45505954 	ldrbmi	r5, [r0, #-2388]	; 0x954
    2d88:	4558455f 	ldrbmi	r4, [r8, #-1375]	; 0x55f
    2d8c:	41545543 	cmpmi	r4, r3, asr #10
    2d90:	3a454c42 	bcc	1155ea0 <start_address+0x1145ea0>
    2d94:	003b2c32 	eorseq	r2, fp, r2, lsr ip
    2d98:	48666c45 	stmdami	r6!, {r0, r2, r6, sl, fp, sp, lr}^
    2d9c:	65646165 	strbvs	r6, [r4, #-357]!	; 0x165
    2da0:	28543a72 	ldmdacs	r4, {r1, r4, r5, r6, r9, fp, ip, sp}^
    2da4:	322c3231 	eorcc	r3, ip, #268435459	; 0x10000003
    2da8:	35733d29 	ldrbcc	r3, [r3, #-3369]!	; 0xd29
    2dac:	67616d32 			; <UNDEFINED> instruction: 0x67616d32
    2db0:	283a6369 	ldmdacs	sl!, {r0, r3, r5, r6, r8, r9, sp, lr}
    2db4:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2db8:	332c302c 			; <UNDEFINED> instruction: 0x332c302c
    2dbc:	6c653b32 	stclvs	11, cr3, [r5], #-200	; 0xffffff38
    2dc0:	31283a66 			; <UNDEFINED> instruction: 0x31283a66
    2dc4:	29332c32 	ldmdbcs	r3!, {r1, r4, r5, sl, fp, sp}
    2dc8:	2872613d 	ldmdacs	r2!, {r0, r2, r3, r4, r5, r8, sp, lr}^
    2dcc:	29342c39 	ldmdbcs	r4!, {r0, r3, r4, r5, sl, fp, sp}
    2dd0:	313b303b 	teqcc	fp, fp, lsr r0
    2dd4:	30283b31 	eorcc	r3, r8, r1, lsr fp
    2dd8:	2c29322c 	sfmcs	f3, 4, [r9], #-176	; 0xffffff50
    2ddc:	392c3233 	stmdbcc	ip!, {r0, r1, r4, r5, r9, ip, sp}
    2de0:	79743b36 	ldmdbvc	r4!, {r1, r2, r4, r5, r8, r9, fp, ip, sp}^
    2de4:	283a6570 	ldmdacs	sl!, {r4, r5, r6, r8, sl, sp, lr}
    2de8:	29352c32 	ldmdbcs	r5!, {r1, r4, r5, sl, fp, sp}
    2dec:	3832312c 	ldmdacc	r2!, {r2, r3, r5, r8, ip, sp}
    2df0:	3b36312c 	blcc	d8f2a8 <start_address+0xd7f2a8>
    2df4:	6863616d 	stmdavs	r3!, {r0, r2, r3, r5, r6, r8, sp, lr}^
    2df8:	3a656e69 	bcc	195e7a4 <start_address+0x194e7a4>
    2dfc:	352c3228 	strcc	r3, [ip, #-552]!	; 0x228
    2e00:	34312c29 	ldrtcc	r2, [r1], #-3113	; 0xc29
    2e04:	36312c34 			; <UNDEFINED> instruction: 0x36312c34
    2e08:	7265763b 	rsbvc	r7, r5, #61865984	; 0x3b00000
    2e0c:	6e6f6973 	mcrvs	9, 3, r6, cr15, cr3, {3}
    2e10:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    2e14:	312c2937 			; <UNDEFINED> instruction: 0x312c2937
    2e18:	332c3036 			; <UNDEFINED> instruction: 0x332c3036
    2e1c:	6e653b32 	vmovvs.8	d5[5], r3
    2e20:	3a797274 	bcc	1e5f7f8 <start_address+0x1e4f7f8>
    2e24:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    2e28:	39312c29 	ldmdbcc	r1!, {r0, r3, r5, sl, fp, sp}
    2e2c:	32332c32 	eorscc	r2, r3, #12800	; 0x3200
    2e30:	6f68703b 	svcvs	0x0068703b
    2e34:	283a6666 	ldmdacs	sl!, {r1, r2, r5, r6, r9, sl, sp, lr}
    2e38:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2e3c:	3432322c 	ldrtcc	r3, [r2], #-556	; 0x22c
    2e40:	3b32332c 	blcc	c8faf8 <start_address+0xc7faf8>
    2e44:	666f6873 			; <UNDEFINED> instruction: 0x666f6873
    2e48:	32283a66 	eorcc	r3, r8, #417792	; 0x66000
    2e4c:	2c29372c 	stccs	7, cr3, [r9], #-176	; 0xffffff50
    2e50:	2c363532 	cfldr32cs	mvfx3, [r6], #-200	; 0xffffff38
    2e54:	663b3233 			; <UNDEFINED> instruction: 0x663b3233
    2e58:	7367616c 	cmnvc	r7, #108, 2
    2e5c:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    2e60:	322c2937 	eorcc	r2, ip, #901120	; 0xdc000
    2e64:	332c3838 			; <UNDEFINED> instruction: 0x332c3838
    2e68:	68653b32 	stmdavs	r5!, {r1, r4, r5, r8, r9, fp, ip, sp}^
    2e6c:	657a6973 	ldrbvs	r6, [sl, #-2419]!	; 0x973
    2e70:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    2e74:	332c2935 			; <UNDEFINED> instruction: 0x332c2935
    2e78:	312c3032 			; <UNDEFINED> instruction: 0x312c3032
    2e7c:	68703b36 	ldmdavs	r0!, {r1, r2, r4, r5, r8, r9, fp, ip, sp}^
    2e80:	73746e65 	cmnvc	r4, #1616	; 0x650
    2e84:	3a657a69 	bcc	1961830 <start_address+0x1951830>
    2e88:	352c3228 	strcc	r3, [ip, #-552]!	; 0x228
    2e8c:	33332c29 	teqcc	r3, #10496	; 0x2900
    2e90:	36312c36 			; <UNDEFINED> instruction: 0x36312c36
    2e94:	6e68703b 	mcrvs	0, 3, r7, cr8, cr11, {1}
    2e98:	283a6d75 	ldmdacs	sl!, {r0, r2, r4, r5, r6, r8, sl, fp, sp, lr}
    2e9c:	29352c32 	ldmdbcs	r5!, {r1, r4, r5, sl, fp, sp}
    2ea0:	3235332c 	eorscc	r3, r5, #44, 6	; 0xb0000000
    2ea4:	3b36312c 	blcc	d8f35c <start_address+0xd7f35c>
    2ea8:	6e656873 	mcrvs	8, 3, r6, cr5, cr3, {3}
    2eac:	7a697374 	bvc	1a5fc84 <start_address+0x1a4fc84>
    2eb0:	32283a65 	eorcc	r3, r8, #413696	; 0x65000
    2eb4:	2c29352c 	cfstr32cs	mvfx3, [r9], #-176	; 0xffffff50
    2eb8:	2c383633 	ldccs	6, cr3, [r8], #-204	; 0xffffff34
    2ebc:	733b3631 	teqvc	fp, #51380224	; 0x3100000
    2ec0:	6d756e68 	ldclvs	14, cr6, [r5, #-416]!	; 0xfffffe60
    2ec4:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    2ec8:	332c2935 			; <UNDEFINED> instruction: 0x332c2935
    2ecc:	312c3438 			; <UNDEFINED> instruction: 0x312c3438
    2ed0:	68733b36 	ldmdavs	r3!, {r1, r2, r4, r5, r8, r9, fp, ip, sp}^
    2ed4:	6e727473 	mrcvs	4, 3, r7, cr2, cr3, {3}
    2ed8:	283a7864 	ldmdacs	sl!, {r2, r5, r6, fp, ip, sp, lr}
    2edc:	29352c32 	ldmdbcs	r5!, {r1, r4, r5, sl, fp, sp}
    2ee0:	3030342c 	eorscc	r3, r0, ip, lsr #8
    2ee4:	3b36312c 	blcc	d8f39c <start_address+0xd7f39c>
    2ee8:	6c45003b 	mcrrvs	0, 3, r0, r5, cr11
    2eec:	6f725066 	svcvs	0x00725066
    2ef0:	6d617267 	sfmvs	f7, 2, [r1, #-412]!	; 0xfffffe64
    2ef4:	64616548 	strbtvs	r6, [r1], #-1352	; 0x548
    2ef8:	543a7265 	ldrtpl	r7, [sl], #-613	; 0x265
    2efc:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    2f00:	733d2934 	teqvc	sp, #52, 18	; 0xd0000
    2f04:	79743233 	ldmdbvc	r4!, {r0, r1, r4, r5, r9, ip, sp}^
    2f08:	283a6570 	ldmdacs	sl!, {r4, r5, r6, r8, sl, sp, lr}
    2f0c:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2f10:	332c302c 			; <UNDEFINED> instruction: 0x332c302c
    2f14:	666f3b32 			; <UNDEFINED> instruction: 0x666f3b32
    2f18:	32283a66 	eorcc	r3, r8, #417792	; 0x66000
    2f1c:	2c29372c 	stccs	7, cr3, [r9], #-176	; 0xffffff50
    2f20:	332c3233 			; <UNDEFINED> instruction: 0x332c3233
    2f24:	61763b32 	cmnvs	r6, r2, lsr fp
    2f28:	3a726464 	bcc	1c9c0c0 <start_address+0x1c8c0c0>
    2f2c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    2f30:	34362c29 	ldrtcc	r2, [r6], #-3113	; 0xc29
    2f34:	3b32332c 	blcc	c8fbec <start_address+0xc7fbec>
    2f38:	64646170 	strbtvs	r6, [r4], #-368	; 0x170
    2f3c:	32283a72 	eorcc	r3, r8, #466944	; 0x72000
    2f40:	2c29372c 	stccs	7, cr3, [r9], #-176	; 0xffffff50
    2f44:	332c3639 			; <UNDEFINED> instruction: 0x332c3639
    2f48:	69663b32 	stmdbvs	r6!, {r1, r4, r5, r8, r9, fp, ip, sp}^
    2f4c:	7a73656c 	bvc	1cdc504 <start_address+0x1ccc504>
    2f50:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    2f54:	312c2937 			; <UNDEFINED> instruction: 0x312c2937
    2f58:	332c3832 			; <UNDEFINED> instruction: 0x332c3832
    2f5c:	656d3b32 	strbvs	r3, [sp, #-2866]!	; 0xb32
    2f60:	3a7a736d 	bcc	1e9fd1c <start_address+0x1e8fd1c>
    2f64:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    2f68:	36312c29 	ldrtcc	r2, [r1], -r9, lsr #24
    2f6c:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    2f70:	616c663b 	cmnvs	ip, fp, lsr r6
    2f74:	283a7367 	ldmdacs	sl!, {r0, r1, r2, r5, r6, r8, r9, ip, sp, lr}
    2f78:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    2f7c:	3239312c 	eorscc	r3, r9, #44, 2
    2f80:	3b32332c 	blcc	c8fc38 <start_address+0xc7fc38>
    2f84:	67696c61 	strbvs	r6, [r9, -r1, ror #24]!
    2f88:	32283a6e 	eorcc	r3, r8, #450560	; 0x6e000
    2f8c:	2c29372c 	stccs	7, cr3, [r9], #-176	; 0xffffff50
    2f90:	2c343232 	lfmcs	f3, 4, [r4], #-200	; 0xffffff38
    2f94:	3b3b3233 	blcc	ecf868 <start_address+0xebf868>
    2f98:	6f727000 	svcvs	0x00727000
    2f9c:	696e4963 	stmdbvs	lr!, {r0, r1, r5, r6, r8, fp, lr}^
    2fa0:	28463a74 	stmdacs	r6, {r2, r4, r5, r6, r9, fp, ip, sp}^
    2fa4:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    2fa8:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    2fac:	7845636f 	stmdavc	r5, {r0, r1, r2, r3, r5, r6, r8, r9, sp, lr}^
    2fb0:	646e6170 	strbtvs	r6, [lr], #-368	; 0x170
    2fb4:	6f6d654d 	svcvs	0x006d654d
    2fb8:	463a7972 			; <UNDEFINED> instruction: 0x463a7972
    2fbc:	312c3228 			; <UNDEFINED> instruction: 0x312c3228
    2fc0:	3a700029 	bcc	1c0306c <start_address+0x1bf306c>
    2fc4:	2c352870 	ldccs	8, cr2, [r5], #-448	; 0xfffffe40
    2fc8:	70002937 	andvc	r2, r0, r7, lsr r9
    2fcc:	43656761 	cmnmi	r5, #25427968	; 0x1840000
    2fd0:	746e756f 	strbtvc	r7, [lr], #-1391	; 0x56f
    2fd4:	3028703a 	eorcc	r7, r8, sl, lsr r0
    2fd8:	0029312c 	eoreq	r3, r9, ip, lsr #2
    2fdc:	65676170 	strbvs	r6, [r7, #-368]!	; 0x170
    2fe0:	2c35283a 	ldccs	8, cr2, [r5], #-232	; 0xffffff18
    2fe4:	70002933 	andvc	r2, r0, r3, lsr r9
    2fe8:	53636f72 	cmnpl	r3, #456	; 0x1c8
    2fec:	6e697268 	cdpvs	2, 6, cr7, cr9, cr8, {3}
    2ff0:	6d654d6b 	stclvs	13, cr4, [r5, #-428]!	; 0xfffffe54
    2ff4:	3a79726f 	bcc	1e5f9b8 <start_address+0x1e4f9b8>
    2ff8:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    2ffc:	00293534 	eoreq	r3, r9, r4, lsr r5
    3000:	74726976 	ldrbtvc	r6, [r2], #-2422	; 0x976
    3004:	416c6175 	smcmi	50709	; 0xc615
    3008:	3a726464 	bcc	1c9c1a0 <start_address+0x1c8c1a0>
    300c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    3010:	68700029 	ldmdavs	r0!, {r0, r3, r5}^
    3014:	63697379 	cmnvs	r9, #-469762047	; 0xe4000001
    3018:	64416c61 	strbvs	r6, [r1], #-3169	; 0xc61
    301c:	283a7264 	ldmdacs	sl!, {r2, r5, r6, r9, ip, sp, lr}
    3020:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    3024:	72656b00 	rsbvc	r6, r5, #0, 22
    3028:	416c656e 	cmnmi	ip, lr, ror #10
    302c:	3a726464 	bcc	1c9c1c4 <start_address+0x1c8c1c4>
    3030:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    3034:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    3038:	6547636f 	strbvs	r6, [r7, #-879]	; 0x36f
    303c:	6d654d74 	stclvs	13, cr4, [r5, #-464]!	; 0xfffffe30
    3040:	6c696154 	stfvse	f6, [r9], #-336	; 0xfffffeb0
    3044:	3528663a 	strcc	r6, [r8, #-1594]!	; 0x63a
    3048:	0029372c 	eoreq	r3, r9, ip, lsr #14
    304c:	636f7270 	cmnvs	pc, #112, 4
    3050:	61657243 	cmnvs	r5, r3, asr #4
    3054:	463a6574 			; <UNDEFINED> instruction: 0x463a6574
    3058:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    305c:	70002936 	andvc	r2, r0, r6, lsr r9
    3060:	3a636f72 	bcc	18dee30 <start_address+0x18cee30>
    3064:	2c302856 	ldccs	8, cr2, [r0], #-344	; 0xfffffea8
    3068:	00293634 	eoreq	r3, r9, r4, lsr r6
    306c:	65646e69 	strbvs	r6, [r4, #-3689]!	; 0xe69
    3070:	28563a78 	ldmdacs	r6, {r3, r4, r5, r6, r9, fp, ip, sp}^
    3074:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    3078:	3a6d7600 	bcc	1b60880 <start_address+0x1b50880>
    307c:	312c3428 			; <UNDEFINED> instruction: 0x312c3428
    3080:	6b002932 	blvs	d550 <start_address-0x2ab0>
    3084:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
    3088:	6174536c 	cmnvs	r4, ip, ror #6
    308c:	283a6b63 	ldmdacs	sl!, {r0, r1, r5, r6, r8, r9, fp, sp, lr}
    3090:	29332c35 	ldmdbcs	r3!, {r0, r2, r4, r5, sl, fp, sp}
    3094:	65737500 	ldrbvs	r7, [r3, #-1280]!	; 0x500
    3098:	61745372 	cmnvs	r4, r2, ror r3
    309c:	283a6b63 	ldmdacs	sl!, {r0, r1, r5, r6, r8, r9, fp, sp, lr}
    30a0:	29332c35 	ldmdbcs	r3!, {r0, r2, r4, r5, sl, fp, sp}
    30a4:	74656700 	strbtvc	r6, [r5], #-1792	; 0x700
    30a8:	72727543 	rsbsvc	r7, r2, #281018368	; 0x10c00000
    30ac:	43746e65 	cmnmi	r4, #1616	; 0x650
    30b0:	65746e6f 	ldrbvs	r6, [r4, #-3695]!	; 0xe6f
    30b4:	463a7478 			; <UNDEFINED> instruction: 0x463a7478
    30b8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    30bc:	2a3d2937 	bcs	f4d5a0 <start_address+0xf3d5a0>
    30c0:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    30c4:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    30c8:	7246636f 	subvc	r6, r6, #-1140850687	; 0xbc000001
    30cc:	463a6565 	ldrtmi	r6, [sl], -r5, ror #10
    30d0:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    30d4:	70002935 	andvc	r2, r0, r5, lsr r9
    30d8:	3a636f72 	bcc	18deea8 <start_address+0x18ceea8>
    30dc:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    30e0:	00293634 	eoreq	r3, r9, r4, lsr r6
    30e4:	636f7270 	cmnvs	pc, #112, 4
    30e8:	64616f4c 	strbtvs	r6, [r1], #-3916	; 0xf4c
    30ec:	3228463a 	eorcc	r4, r8, #60817408	; 0x3a00000
    30f0:	0029312c 	eoreq	r3, r9, ip, lsr #2
    30f4:	636f7270 	cmnvs	pc, #112, 4
    30f8:	67616d49 	strbvs	r6, [r1, -r9, asr #26]!
    30fc:	28703a65 	ldmdacs	r0!, {r0, r2, r5, r6, r9, fp, ip, sp}^
    3100:	332c3131 			; <UNDEFINED> instruction: 0x332c3131
    3104:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    3108:	6548676f 	strbvs	r6, [r8, #-1903]	; 0x76f
    310c:	72656461 	rsbvc	r6, r5, #1627389952	; 0x61000000
    3110:	7366664f 	cmnvc	r6, #82837504	; 0x4f00000
    3114:	283a7465 	ldmdacs	sl!, {r0, r2, r5, r6, sl, ip, sp, lr}
    3118:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    311c:	6f727000 	svcvs	0x00727000
    3120:	61654867 	cmnvs	r5, r7, ror #16
    3124:	43726564 	cmnmi	r2, #100, 10	; 0x19000000
    3128:	746e756f 	strbtvc	r7, [lr], #-1391	; 0x56f
    312c:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    3130:	68002931 	stmdavs	r0, {r0, r4, r5, r8, fp, sp}
    3134:	65646165 	strbvs	r6, [r4, #-357]!	; 0x165
    3138:	30283a72 	eorcc	r3, r8, r2, ror sl
    313c:	2938342c 	ldmdbcs	r8!, {r2, r3, r5, sl, ip, sp}
    3140:	31282a3d 			; <UNDEFINED> instruction: 0x31282a3d
    3144:	29322c32 	ldmdbcs	r2!, {r1, r4, r5, sl, fp, sp}
    3148:	283a6a00 	ldmdacs	sl!, {r9, fp, sp, lr}
    314c:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    3150:	61656800 	cmnvs	r5, r0, lsl #16
    3154:	3a726564 	bcc	1c9c6ec <start_address+0x1c8c6ec>
    3158:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    315c:	2a3d2939 	bcs	f4d648 <start_address+0xf3d648>
    3160:	2c323128 	ldfcss	f3, [r2], #-160	; 0xffffff60
    3164:	76002934 			; <UNDEFINED> instruction: 0x76002934
    3168:	72646461 	rsbvc	r6, r4, #1627389952	; 0x61000000
    316c:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    3170:	70002931 	andvc	r2, r0, r1, lsr r9
    3174:	72646461 	rsbvc	r6, r4, #1627389952	; 0x61000000
    3178:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    317c:	70002931 	andvc	r2, r0, r1, lsr r9
    3180:	283a7274 	ldmdacs	sl!, {r2, r4, r5, r6, r9, ip, sp, lr}
    3184:	29332c35 	ldmdbcs	r3!, {r0, r2, r4, r5, sl, fp, sp}
    3188:	616d6900 	cmnvs	sp, r0, lsl #18
    318c:	664f6567 	strbvs	r6, [pc], -r7, ror #10
    3190:	30283a66 	eorcc	r3, r8, r6, ror #20
    3194:	0029312c 	eoreq	r3, r9, ip, lsr #2
    3198:	636f7270 	cmnvs	pc, #112, 4
    319c:	72617453 	rsbvc	r7, r1, #1392508928	; 0x53000000
    31a0:	28463a74 	stmdacs	r6, {r2, r4, r5, r6, r9, fp, ip, sp}^
    31a4:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    31a8:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    31ac:	6547636f 	strbvs	r6, [r7, #-879]	; 0x36f
    31b0:	28463a74 	stmdacs	r6, {r2, r4, r5, r6, r9, fp, ip, sp}^
    31b4:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    31b8:	69700029 	ldmdbvs	r0!, {r0, r3, r5}^
    31bc:	28703a64 	ldmdacs	r0!, {r2, r5, r6, r9, fp, ip, sp}^
    31c0:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    31c4:	6f666b00 	svcvs	0x00666b00
    31c8:	463a6b72 			; <UNDEFINED> instruction: 0x463a6b72
    31cc:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    31d0:	68630029 	stmdavs	r3!, {r0, r3, r5}^
    31d4:	3a646c69 	bcc	191e380 <start_address+0x190e380>
    31d8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    31dc:	70002936 	andvc	r2, r0, r6, lsr r9
    31e0:	6e657261 	cdpvs	2, 6, cr7, cr5, cr1, {3}
    31e4:	30283a74 	eorcc	r3, r8, r4, ror sl
    31e8:	2936342c 	ldmdbcs	r6!, {r2, r3, r5, sl, ip, sp}
    31ec:	69686300 	stmdbvs	r8!, {r8, r9, sp, lr}^
    31f0:	4150646c 	cmpmi	r0, ip, ror #8
    31f4:	3a726464 	bcc	1c9c38c <start_address+0x1c8c38c>
    31f8:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    31fc:	68630029 	stmdavs	r3!, {r0, r3, r5}^
    3200:	50646c69 	rsbpl	r6, r4, r9, ror #24
    3204:	283a7274 	ldmdacs	sl!, {r2, r4, r5, r6, r9, ip, sp, lr}
    3208:	29332c35 	ldmdbcs	r3!, {r0, r2, r4, r5, sl, fp, sp}
    320c:	72617000 	rsbvc	r7, r1, #0
    3210:	50746e65 	rsbspl	r6, r4, r5, ror #28
    3214:	283a7274 	ldmdacs	sl!, {r2, r4, r5, r6, r9, ip, sp, lr}
    3218:	29332c35 	ldmdbcs	r3!, {r0, r2, r4, r5, sl, fp, sp}
    321c:	6f727000 	svcvs	0x00727000
    3220:	69784563 	ldmdbvs	r8!, {r0, r1, r5, r6, r8, sl, lr}^
    3224:	28463a74 	stmdacs	r6, {r2, r4, r5, r6, r9, fp, ip, sp}^
    3228:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    322c:	615f0029 	cmpvs	pc, r9, lsr #32
    3230:	74726f62 	ldrbtvc	r6, [r2], #-3938	; 0xf62
    3234:	72746e45 	rsbsvc	r6, r4, #1104	; 0x450
    3238:	28463a79 	stmdacs	r6, {r0, r3, r4, r5, r6, r9, fp, ip, sp}^
    323c:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    3240:	4b500029 	blmi	14032ec <start_address+0x13f32ec>
    3244:	59545f47 	ldmdbpl	r4, {r0, r1, r2, r6, r8, r9, sl, fp, ip, lr}^
    3248:	455f4550 	ldrbmi	r4, [pc, #-1360]	; 2d00 <start_address-0xd300>
    324c:	533a5252 	teqpl	sl, #536870917	; 0x20000005
    3250:	352c3028 	strcc	r3, [ip, #-40]!	; 0x28
    3254:	6b3d2930 	blvs	f4d71c <start_address+0xf3d71c>
    3258:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    325c:	705f0029 	subsvc	r0, pc, r9, lsr #32
    3260:	65636f72 	strbvs	r6, [r3, #-3954]!	; 0xf72
    3264:	4d567373 	ldclmi	3, cr7, [r6, #-460]	; 0xfffffe34
    3268:	3028533a 	eorcc	r5, r8, sl, lsr r3
    326c:	2931352c 	ldmdbcs	r1!, {r2, r3, r5, r8, sl, ip, sp}
    3270:	2872613d 	ldmdacs	r2!, {r0, r2, r3, r4, r5, r8, sp, lr}^
    3274:	29342c39 	ldmdbcs	r4!, {r0, r3, r4, r5, sl, fp, sp}
    3278:	313b303b 	teqcc	fp, fp, lsr r0
    327c:	283b3732 	ldmdacs	fp!, {r1, r4, r5, r8, r9, sl, ip, sp}
    3280:	32352c30 	eorscc	r2, r5, #48, 24	; 0x3000
    3284:	72613d29 	rsbvc	r3, r1, #2624	; 0xa40
    3288:	342c3928 	strtcc	r3, [ip], #-2344	; 0x928
    328c:	3b303b29 	blcc	c11f38 <start_address+0xc01f38>
    3290:	35393034 	ldrcc	r3, [r9, #-52]!	; 0x34
    3294:	2c31283b 	ldccs	8, cr2, [r1], #-236	; 0xffffff14
    3298:	5f002931 	svcpl	0x00002931
    329c:	72727563 	rsbsvc	r7, r2, #415236096	; 0x18c00000
    32a0:	50746e65 	rsbspl	r6, r4, r5, ror #28
    32a4:	65636f72 	strbvs	r6, [r3, #-3954]!	; 0xf72
    32a8:	473a7373 			; <UNDEFINED> instruction: 0x473a7373
    32ac:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    32b0:	5f002936 	svcpl	0x00002936
    32b4:	636f7270 	cmnvs	pc, #112, 4
    32b8:	54737365 	ldrbtpl	r7, [r3], #-869	; 0x365
    32bc:	656c6261 	strbvs	r6, [ip, #-609]!	; 0x261
    32c0:	3028473a 	eorcc	r4, r8, sl, lsr r7
    32c4:	2933352c 	ldmdbcs	r3!, {r2, r3, r5, r8, sl, ip, sp}
    32c8:	2872613d 	ldmdacs	r2!, {r0, r2, r3, r4, r5, r8, sp, lr}^
    32cc:	29342c39 	ldmdbcs	r4!, {r0, r3, r4, r5, sl, fp, sp}
    32d0:	313b303b 	teqcc	fp, fp, lsr r0
    32d4:	283b3732 	ldmdacs	fp!, {r1, r4, r5, r8, r9, sl, ip, sp}
    32d8:	29392c34 	ldmdbcs	r9!, {r2, r4, r5, sl, fp, sp}
    32dc:	63727300 	cmnvs	r2, #0, 6
    32e0:	656d6b2f 	strbvs	r6, [sp, #-2863]!	; 0xb2f
    32e4:	67617373 			; <UNDEFINED> instruction: 0x67617373
    32e8:	00632e65 	rsbeq	r2, r3, r5, ror #28
    32ec:	50746567 	rsbspl	r6, r4, r7, ror #10
    32f0:	616b6361 	cmnvs	fp, r1, ror #6
    32f4:	69536567 	ldmdbvs	r3, {r0, r1, r2, r5, r6, r8, sl, sp, lr}^
    32f8:	663a657a 			; <UNDEFINED> instruction: 0x663a657a
    32fc:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    3300:	6b700029 	blvs	1c033ac <start_address+0x1bf33ac>
    3304:	28703a67 	ldmdacs	r0!, {r0, r1, r2, r5, r6, r9, fp, ip, sp}^
    3308:	29332c31 	ldmdbcs	r3!, {r0, r4, r5, sl, fp, sp}
    330c:	65736b00 	ldrbvs	r6, [r3, #-2816]!	; 0xb00
    3310:	463a646e 	ldrtmi	r6, [sl], -lr, ror #8
    3314:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    3318:	64690029 	strbtvs	r0, [r9], #-41	; 0x29
    331c:	3028703a 	eorcc	r7, r8, sl, lsr r0
    3320:	0029312c 	eoreq	r3, r9, ip, lsr #2
    3324:	72506f74 	subsvc	r6, r0, #116, 30	; 0x1d0
    3328:	283a636f 	ldmdacs	sl!, {r0, r1, r2, r3, r5, r6, r8, r9, sp, lr}
    332c:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    3330:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    3334:	29392c34 	ldmdbcs	r9!, {r2, r4, r5, sl, fp, sp}
    3338:	67736d00 	ldrbvs	r6, [r3, -r0, lsl #26]!
    333c:	2c31283a 	ldccs	8, cr2, [r1], #-232	; 0xffffff18
    3340:	70002937 	andvc	r2, r0, r7, lsr r9
    3344:	6953676b 	ldmdbvs	r3, {r0, r1, r3, r5, r6, r8, r9, sl, sp, lr}^
    3348:	283a657a 	ldmdacs	sl!, {r1, r3, r4, r5, r6, r8, sl, sp, lr}
    334c:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    3350:	65726b00 	ldrbvs	r6, [r2, #-2816]!	; 0xb00
    3354:	463a7663 	ldrtmi	r7, [sl], -r3, ror #12
    3358:	332c3128 			; <UNDEFINED> instruction: 0x332c3128
    335c:	75710029 	ldrbvc	r0, [r1, #-41]!	; 0x29
    3360:	3a657565 	bcc	19608fc <start_address+0x19508fc>
    3364:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    3368:	2a3d2937 	bcs	f4d84c <start_address+0xf3d84c>
    336c:	352c3128 	strcc	r3, [ip, #-296]!	; 0x128
    3370:	65720029 	ldrbvs	r0, [r2, #-41]!	; 0x29
    3374:	31283a74 			; <UNDEFINED> instruction: 0x31283a74
    3378:	0029332c 	eoreq	r3, r9, ip, lsr #6
    337c:	61656c63 	cmnvs	r5, r3, ror #24
    3380:	73654d72 	cmnvc	r5, #7296	; 0x1c80
    3384:	65676173 	strbvs	r6, [r7, #-371]!	; 0x173
    3388:	75657551 	strbvc	r7, [r5, #-1361]!	; 0x551
    338c:	28463a65 	stmdacs	r6, {r0, r2, r5, r6, r9, fp, ip, sp}^
    3390:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    3394:	75710029 	ldrbvc	r0, [r1, #-41]!	; 0x29
    3398:	3a657565 	bcc	1960934 <start_address+0x1950934>
    339c:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    33a0:	00293734 	eoreq	r3, r9, r4, lsr r7
    33a4:	283a7266 	ldmdacs	sl!, {r1, r2, r5, r6, r9, ip, sp, lr}
    33a8:	29372c31 	ldmdbcs	r7!, {r0, r4, r5, sl, fp, sp}
    33ac:	474b5000 	strbmi	r5, [fp, -r0]
    33b0:	5059545f 	subspl	r5, r9, pc, asr r4
    33b4:	52455f45 	subpl	r5, r5, #276	; 0x114
    33b8:	28533a52 	ldmdacs	r3, {r1, r4, r6, r9, fp, ip, sp}^
    33bc:	38342c30 	ldmdacc	r4!, {r4, r5, sl, fp, sp}
    33c0:	286b3d29 	stmdacs	fp!, {r0, r3, r5, r8, sl, fp, ip, sp}^
    33c4:	29372c32 	ldmdbcs	r7!, {r1, r4, r5, sl, fp, sp}
    33c8:	6b705f00 	blvs	1c1afd0 <start_address+0x1c0afd0>
    33cc:	43444967 	movtmi	r4, #18791	; 0x4967
    33d0:	746e756f 	strbtvc	r7, [lr], #-1391	; 0x56f
    33d4:	3028533a 	eorcc	r5, r8, sl, lsr r3
    33d8:	0029312c 	eoreq	r3, r9, ip, lsr #2
    33dc:	2f637273 	svccs	0x00637273
    33e0:	6c69666b 	stclvs	6, cr6, [r9], #-428	; 0xfffffe54
    33e4:	00632e65 	rsbeq	r2, r3, r5, ror #28
    33e8:	704f666b 	subvc	r6, pc, fp, ror #12
    33ec:	463a6e65 	ldrtmi	r6, [sl], -r5, ror #28
    33f0:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    33f4:	2a3d2936 	bcs	f4d8d4 <start_address+0xf3d8d4>
    33f8:	332c3128 			; <UNDEFINED> instruction: 0x332c3128
    33fc:	73660029 	cmnvc	r6, #41	; 0x29
    3400:	65646f4e 	strbvs	r6, [r4, #-3918]!	; 0xf4e
    3404:	72646441 	rsbvc	r6, r4, #1090519040	; 0x41000000
    3408:	3228703a 	eorcc	r7, r8, #58	; 0x3a
    340c:	0029372c 	eoreq	r3, r9, ip, lsr #14
    3410:	283a666b 	ldmdacs	sl!, {r0, r1, r3, r5, r6, r9, sl, sp, lr}
    3414:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    3418:	666b0029 	strbtvs	r0, [fp], -r9, lsr #32
    341c:	65726e55 	ldrbvs	r6, [r2, #-3669]!	; 0xe55
    3420:	28463a66 	stmdacs	r6, {r1, r2, r5, r6, r9, fp, ip, sp}^
    3424:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    3428:	666b0029 	strbtvs	r0, [fp], -r9, lsr #32
    342c:	3028703a 	eorcc	r7, r8, sl, lsr r0
    3430:	2936342c 	ldmdbcs	r6!, {r2, r3, r5, sl, ip, sp}
    3434:	616c6600 	cmnvs	ip, r0, lsl #12
    3438:	703a7367 	eorsvc	r7, sl, r7, ror #6
    343c:	372c3228 	strcc	r3, [ip, -r8, lsr #4]!
    3440:	666b0029 	strbtvs	r0, [fp], -r9, lsr #32
    3444:	3a666552 	bcc	199c994 <start_address+0x198c994>
    3448:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    344c:	00293534 	eoreq	r3, r9, r4, lsr r5
    3450:	72656b5f 	rsbvc	r6, r5, #97280	; 0x17c00
    3454:	466c656e 	strbtmi	r6, [ip], -lr, ror #10
    3458:	73656c69 	cmnvc	r5, #26880	; 0x6900
    345c:	3028533a 	eorcc	r5, r8, sl, lsr r3
    3460:	2936342c 	ldmdbcs	r6!, {r2, r3, r5, sl, ip, sp}
    3464:	63727300 	cmnvs	r2, #0, 6
    3468:	6172732f 	cmnvs	r2, pc, lsr #6
    346c:	7369646d 	cmnvc	r9, #1828716544	; 0x6d000000
    3470:	00632e6b 	rsbeq	r2, r3, fp, ror #28
    3474:	646d6172 	strbtvs	r6, [sp], #-370	; 0x172
    3478:	436b7369 	cmnmi	fp, #-1543503871	; 0xa4000001
    347c:	65736f6c 	ldrbvs	r6, [r3, #-3948]!	; 0xf6c
    3480:	3028463a 	eorcc	r4, r8, sl, lsr r6
    3484:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    3488:	3a647200 	bcc	191fc90 <start_address+0x190fc90>
    348c:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    3490:	3d293634 	stccc	6, cr3, [r9, #-208]!	; 0xffffff30
    3494:	2c31282a 	ldccs	8, cr2, [r1], #-168	; 0xffffff58
    3498:	66002938 			; <UNDEFINED> instruction: 0x66002938
    349c:	28703a72 	ldmdacs	r0!, {r1, r4, r5, r6, r9, fp, ip, sp}^
    34a0:	37342c30 			; <UNDEFINED> instruction: 0x37342c30
    34a4:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    34a8:	38342c30 	ldmdacc	r4!, {r4, r5, sl, fp, sp}
    34ac:	28663d29 	stmdacs	r6!, {r0, r3, r5, r8, sl, fp, ip, sp}^
    34b0:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    34b4:	66720029 	ldrbtvs	r0, [r2], -r9, lsr #32
    34b8:	2c31283a 	ldccs	8, cr2, [r1], #-232	; 0xffffff18
    34bc:	00293031 	eoreq	r3, r9, r1, lsr r0
    34c0:	646d6172 	strbtvs	r6, [sp], #-370	; 0x172
    34c4:	4f6b7369 	svcmi	0x006b7369
    34c8:	3a6e6570 	bcc	1b9ca90 <start_address+0x1b8ca90>
    34cc:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    34d0:	00293534 	eoreq	r3, r9, r4, lsr r5
    34d4:	3a6d6172 	bcc	1b5baa4 <start_address+0x1b4baa4>
    34d8:	2c312870 	ldccs	8, cr2, [r1], #-448	; 0xfffffe40
    34dc:	72002935 	andvc	r2, r0, #868352	; 0xd4000
    34e0:	28703a64 	ldmdacs	r0!, {r2, r5, r6, r9, fp, ip, sp}^
    34e4:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    34e8:	6c610029 	stclvs	0, cr0, [r1], #-164	; 0xffffff5c
    34ec:	3a636f6c 	bcc	18df2a4 <start_address+0x18cf2a4>
    34f0:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    34f4:	3d293934 	stccc	9, cr3, [r9, #-208]!	; 0xffffff30
    34f8:	2c30282a 	ldccs	8, cr2, [r0], #-168	; 0xffffff58
    34fc:	3d293035 	stccc	0, cr3, [r9, #-212]!	; 0xffffff2c
    3500:	2c302866 	ldccs	8, cr2, [r0], #-408	; 0xfffffe68
    3504:	3d293135 	stfccs	f3, [r9, #-212]!	; 0xffffff2c
    3508:	2c30282a 	ldccs	8, cr2, [r0], #-168	; 0xffffff58
    350c:	00293534 	eoreq	r3, r9, r4, lsr r5
    3510:	656d616e 	strbvs	r6, [sp, #-366]!	; 0x16e
    3514:	3a6e654c 	bcc	1b9ca4c <start_address+0x1b8ca4c>
    3518:	362c3228 	strtcc	r3, [ip], -r8, lsr #4
    351c:	61720029 	cmnvs	r2, r9, lsr #32
    3520:	7369646d 	cmnvc	r9, #1828716544	; 0x6d000000
    3524:	6165526b 	cmnvs	r5, fp, ror #4
    3528:	28463a64 	stmdacs	r6, {r2, r5, r6, r9, fp, ip, sp}^
    352c:	29352c31 	ldmdbcs	r5!, {r0, r4, r5, sl, fp, sp}
    3530:	616e6600 	cmnvs	lr, r0, lsl #12
    3534:	703a656d 	eorsvc	r6, sl, sp, ror #10
    3538:	352c3128 	strcc	r3, [ip, #-296]!	; 0x128
    353c:	69730029 	ldmdbvs	r3!, {r0, r3, r5}^
    3540:	703a657a 	eorsvc	r6, sl, sl, ror r5
    3544:	352c3028 	strcc	r3, [ip, #-40]!	; 0x28
    3548:	2a3d2932 	bcs	f4da18 <start_address+0xf3da18>
    354c:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    3550:	72730029 	rsbsvc	r0, r3, #41	; 0x29
    3554:	736b2f63 	cmnvc	fp, #396	; 0x18c
    3558:	2e767265 	cdpcs	2, 7, cr7, cr6, cr5, {3}
    355c:	534b0063 	movtpl	r0, #45155	; 0xb063
    3560:	54767265 	ldrbtpl	r7, [r6], #-613	; 0x265
    3564:	3128743a 			; <UNDEFINED> instruction: 0x3128743a
    3568:	3d29312c 	stfccs	f3, [r9, #-176]!	; 0xffffff50
    356c:	322c3128 	eorcc	r3, ip, #40, 2
    3570:	32733d29 	rsbscc	r3, r3, #2624	; 0xa40
    3574:	6d616e34 	stclvs	14, cr6, [r1, #-208]!	; 0xffffff30
    3578:	31283a65 			; <UNDEFINED> instruction: 0x31283a65
    357c:	3d29332c 	stccc	3, cr3, [r9, #-176]!	; 0xffffff50
    3580:	31287261 			; <UNDEFINED> instruction: 0x31287261
    3584:	3d29342c 	cfstrscc	mvf3, [r9, #-176]!	; 0xffffff50
    3588:	2c312872 	ldccs	8, cr2, [r1], #-456	; 0xfffffe38
    358c:	303b2934 	eorscc	r2, fp, r4, lsr r9
    3590:	3733303b 			; <UNDEFINED> instruction: 0x3733303b
    3594:	37373737 			; <UNDEFINED> instruction: 0x37373737
    3598:	37373737 			; <UNDEFINED> instruction: 0x37373737
    359c:	303b3b37 	eorscc	r3, fp, r7, lsr fp
    35a0:	3b36313b 	blcc	d8fa94 <start_address+0xd7fa94>
    35a4:	322c3028 	eorcc	r3, ip, #40	; 0x28
    35a8:	2c302c29 	ldccs	12, cr2, [r0], #-164	; 0xffffff5c
    35ac:	3b363331 	blcc	d90278 <start_address+0xd80278>
    35b0:	3a646970 	bcc	191db78 <start_address+0x190db78>
    35b4:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    35b8:	36312c29 	ldrtcc	r2, [r1], -r9, lsr #24
    35bc:	32332c30 	eorscc	r2, r3, #48, 24	; 0x3000
    35c0:	2e003b3b 	vmovcs.16	d0[0], r3
    35c4:	696c2f2e 	stmdbvs	ip!, {r1, r2, r3, r5, r8, r9, sl, fp, sp}^
    35c8:	6e692f62 	cdpvs	15, 6, cr2, cr9, cr2, {3}
    35cc:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
    35d0:	74732f65 	ldrbtvc	r2, [r3], #-3941	; 0xf65
    35d4:	676e6972 			; <UNDEFINED> instruction: 0x676e6972
    35d8:	6b00682e 	blvs	1d698 <start_address+0xd698>
    35dc:	76726573 			; <UNDEFINED> instruction: 0x76726573
    35e0:	3a676552 	bcc	19dcb30 <start_address+0x19ccb30>
    35e4:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    35e8:	6e002931 	mcrvs	9, 0, r2, cr0, cr1, {1}
    35ec:	3a656d61 	bcc	195eb78 <start_address+0x194eb78>
    35f0:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    35f4:	3d293634 	stccc	6, cr3, [r9, #-208]!	; 0xffffff30
    35f8:	2c30282a 	ldccs	8, cr2, [r0], #-168	; 0xffffff58
    35fc:	3d293734 	stccc	7, cr3, [r9, #-208]!	; 0xffffff30
    3600:	2c30286b 	ldccs	8, cr2, [r0], #-428	; 0xfffffe54
    3604:	70002932 	andvc	r2, r0, r2, lsr r9
    3608:	283a6469 	ldmdacs	sl!, {r0, r3, r5, r6, sl, sp, lr}
    360c:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    3610:	65736b00 	ldrbvs	r6, [r3, #-2816]!	; 0xb00
    3614:	65477672 	strbvs	r7, [r7, #-1650]	; 0x672
    3618:	28463a74 	stmdacs	r6, {r2, r4, r5, r6, r9, fp, ip, sp}^
    361c:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    3620:	6d616e00 	stclvs	14, cr6, [r1, #-0]
    3624:	28703a65 	ldmdacs	r0!, {r0, r2, r5, r6, r9, fp, ip, sp}^
    3628:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    362c:	4b500029 	blmi	14036d8 <start_address+0x13f36d8>
    3630:	59545f47 	ldmdbpl	r4, {r0, r1, r2, r6, r8, r9, sl, fp, ip, lr}^
    3634:	455f4550 	ldrbmi	r4, [pc, #-1360]	; 30ec <start_address-0xcf14>
    3638:	533a5252 	teqpl	sl, #536870917	; 0x20000005
    363c:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    3640:	6b3d2938 	blvs	f4db28 <start_address+0xf3db28>
    3644:	372c3328 	strcc	r3, [ip, -r8, lsr #6]!
    3648:	6b5f0029 	blvs	17c36f4 <start_address+0x17b36f4>
    364c:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
    3650:	7265536c 	rsbvc	r5, r5, #108, 6	; 0xb0000001
    3654:	65636976 	strbvs	r6, [r3, #-2422]!	; 0x976
    3658:	28533a73 	ldmdacs	r3, {r0, r1, r4, r5, r6, r9, fp, ip, sp}^
    365c:	39342c30 	ldmdbcc	r4!, {r4, r5, sl, fp, sp}
    3660:	72613d29 	rsbvc	r3, r1, #2624	; 0xa40
    3664:	342c3128 	strtcc	r3, [ip], #-296	; 0x128
    3668:	3b303b29 	blcc	c12314 <start_address+0xc02314>
    366c:	283b3133 	ldmdacs	fp!, {r0, r1, r4, r5, r8, ip, sp}
    3670:	29312c31 	ldmdbcs	r1!, {r0, r4, r5, sl, fp, sp}
    3674:	63727300 	cmnvs	r2, #0, 6
    3678:	6863732f 	stmdavs	r3!, {r0, r1, r2, r3, r5, r8, r9, ip, sp, lr}^
    367c:	6c756465 	cfldrdvs	mvd6, [r5], #-404	; 0xfffffe6c
    3680:	632e7265 			; <UNDEFINED> instruction: 0x632e7265
    3684:	68637300 	stmdavs	r3!, {r8, r9, ip, sp, lr}^
    3688:	6c756465 	cfldrdvs	mvd6, [r5], #-404	; 0xfffffe6c
    368c:	28463a65 	stmdacs	r6, {r0, r2, r5, r6, r9, fp, ip, sp}^
    3690:	35342c30 	ldrcc	r2, [r4, #-3120]!	; 0xc30
    3694:	72700029 	rsbsvc	r0, r0, #41	; 0x29
    3698:	283a636f 	ldmdacs	sl!, {r0, r1, r2, r3, r5, r6, r8, r9, sp, lr}
    369c:	36342c30 			; <UNDEFINED> instruction: 0x36342c30
    36a0:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    36a4:	29392c31 	ldmdbcs	r9!, {r0, r4, r5, sl, fp, sp}
    36a8:	6e616800 	cdpvs	8, 6, cr6, cr1, cr0, {0}
    36ac:	54656c64 	strbtpl	r6, [r5], #-3172	; 0xc64
    36b0:	72656d69 	rsbvc	r6, r5, #6720	; 0x1a40
    36b4:	3028463a 	eorcc	r4, r8, sl, lsr r6
    36b8:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    36bc:	68637300 	stmdavs	r3!, {r8, r9, ip, sp, lr}^
    36c0:	6c756465 	cfldrdvs	mvd6, [r5], #-404	; 0xfffffe6c
    36c4:	6e497265 	cdpvs	2, 4, cr7, cr9, cr5, {3}
    36c8:	463a7469 	ldrtmi	r7, [sl], -r9, ror #8
    36cc:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    36d0:	72002935 	andvc	r2, r0, #868352	; 0xd4000
    36d4:	646e756f 	strbtvs	r7, [lr], #-1391	; 0x56f
    36d8:	69626f52 	stmdbvs	r2!, {r1, r4, r6, r8, r9, sl, fp, sp, lr}^
    36dc:	646e496e 	strbtvs	r4, [lr], #-2414	; 0x96e
    36e0:	533a7865 	teqpl	sl, #6619136	; 0x650000
    36e4:	312c3028 			; <UNDEFINED> instruction: 0x312c3028
    36e8:	2e2e0029 	cdpcs	0, 2, cr0, cr14, cr9, {1}
    36ec:	62696c2f 	rsbvs	r6, r9, #12032	; 0x2f00
    36f0:	6372732f 	cmnvs	r2, #-1140850688	; 0xbc000000
    36f4:	7274732f 	rsbsvc	r7, r4, #-1140850688	; 0xbc000000
    36f8:	2e676e69 	cdpcs	14, 6, cr6, cr7, cr9, {3}
    36fc:	68630063 	stmdavs	r3!, {r0, r1, r5, r6}^
    3700:	743a7261 	ldrtvc	r7, [sl], #-609	; 0x261
    3704:	322c3028 	eorcc	r3, ip, #40	; 0x28
    3708:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
    370c:	29322c30 	ldmdbcs	r2!, {r4, r5, sl, fp, sp}
    3710:	323b303b 	eorscc	r3, fp, #59	; 0x3b
    3714:	003b3535 	eorseq	r3, fp, r5, lsr r5
    3718:	69736e75 	ldmdbvs	r3!, {r0, r2, r4, r5, r6, r9, sl, fp, sp, lr}^
    371c:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
    3720:	746e6920 	strbtvc	r6, [lr], #-2336	; 0x920
    3724:	3028743a 	eorcc	r7, r8, sl, lsr r4
    3728:	3d29342c 	cfstrscc	mvf3, [r9, #-176]!	; 0xffffff50
    372c:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
    3730:	303b2934 	eorscc	r2, fp, r4, lsr r9
    3734:	3932343b 	ldmdbcc	r2!, {r0, r1, r3, r4, r5, sl, ip, sp}
    3738:	37363934 			; <UNDEFINED> instruction: 0x37363934
    373c:	3b353932 	blcc	d51c0c <start_address+0xd41c0c>
    3740:	6e6f6c00 	cdpvs	12, 6, cr6, cr15, cr0, {0}
    3744:	6e752067 	cdpvs	0, 7, cr2, cr5, cr7, {3}
    3748:	6e676973 	mcrvs	9, 3, r6, cr7, cr3, {3}
    374c:	69206465 	stmdbvs	r0!, {r0, r2, r5, r6, sl, sp, lr}
    3750:	743a746e 	ldrtvc	r7, [sl], #-1134	; 0x46e
    3754:	352c3028 	strcc	r3, [ip, #-40]!	; 0x28
    3758:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
    375c:	29352c30 	ldmdbcs	r5!, {r4, r5, sl, fp, sp}
    3760:	343b303b 	ldrtcc	r3, [fp], #-59	; 0x3b
    3764:	39343932 	ldmdbcc	r4!, {r1, r4, r5, r8, fp, ip, sp}
    3768:	39323736 	ldmdbcc	r2!, {r1, r2, r4, r5, r8, r9, sl, ip, sp}
    376c:	6c003b35 	stcvs	11, cr3, [r0], {53}	; 0x35
    3770:	20676e6f 	rsbcs	r6, r7, pc, ror #28
    3774:	676e6f6c 	strbvs	r6, [lr, -ip, ror #30]!
    3778:	746e6920 	strbtvc	r6, [lr], #-2336	; 0x920
    377c:	3028743a 	eorcc	r7, r8, sl, lsr r4
    3780:	3d29362c 	stccc	6, cr3, [r9, #-176]!	; 0xffffff50
    3784:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
    3788:	2d3b2936 	ldccs	9, cr2, [fp, #-216]!	; 0xffffff28
    378c:	32343b30 	eorscc	r3, r4, #48, 22	; 0xc000
    3790:	36393439 			; <UNDEFINED> instruction: 0x36393439
    3794:	35393237 	ldrcc	r3, [r9, #-567]!	; 0x237
    3798:	6f6c003b 	svcvs	0x006c003b
    379c:	6c20676e 	stcvs	7, cr6, [r0], #-440	; 0xfffffe48
    37a0:	20676e6f 	rsbcs	r6, r7, pc, ror #28
    37a4:	69736e75 	ldmdbvs	r3!, {r0, r2, r4, r5, r6, r9, sl, fp, sp, lr}^
    37a8:	64656e67 	strbtvs	r6, [r5], #-3687	; 0xe67
    37ac:	746e6920 	strbtvc	r6, [lr], #-2336	; 0x920
    37b0:	3028743a 	eorcc	r7, r8, sl, lsr r4
    37b4:	3d29372c 	stccc	7, cr3, [r9, #-176]!	; 0xffffff50
    37b8:	2c302872 	ldccs	8, cr2, [r0], #-456	; 0xfffffe38
    37bc:	303b2937 	eorscc	r2, fp, r7, lsr r9
    37c0:	3b312d3b 	blcc	c4ecb4 <start_address+0xc3ecb4>
    37c4:	6f687300 	svcvs	0x00687300
    37c8:	69207472 	stmdbvs	r0!, {r1, r4, r5, r6, sl, ip, sp, lr}
    37cc:	743a746e 	ldrtvc	r7, [sl], #-1134	; 0x46e
    37d0:	382c3028 	stmdacc	ip!, {r3, r5, ip, sp}
    37d4:	28723d29 	ldmdacs	r2!, {r0, r3, r5, r8, sl, fp, ip, sp}^
    37d8:	29382c30 	ldmdbcs	r8!, {r4, r5, sl, fp, sp}
    37dc:	32332d3b 	eorscc	r2, r3, #3776	; 0xec0
    37e0:	3b383637 	blcc	e110c4 <start_address+0xe010c4>
    37e4:	36373233 			; <UNDEFINED> instruction: 0x36373233
    37e8:	73003b37 	movwvc	r3, #2871	; 0xb37
    37ec:	74726f68 	ldrbtvc	r6, [r2], #-3944	; 0xf68
    37f0:	736e7520 	cmnvc	lr, #32, 10	; 0x8000000
    37f4:	656e6769 	strbvs	r6, [lr, #-1897]!	; 0x769
    37f8:	6e692064 	cdpvs	0, 6, cr2, cr9, cr4, {3}
    37fc:	28743a74 	ldmdacs	r4!, {r2, r4, r5, r6, r9, fp, ip, sp}^
    3800:	29392c30 	ldmdbcs	r9!, {r4, r5, sl, fp, sp}
    3804:	3028723d 	eorcc	r7, r8, sp, lsr r2
    3808:	3b29392c 	blcc	a51cc0 <start_address+0xa41cc0>
    380c:	35363b30 	ldrcc	r3, [r6, #-2864]!	; 0xb30
    3810:	3b353335 	blcc	d504ec <start_address+0xd404ec>
    3814:	67697300 	strbvs	r7, [r9, -r0, lsl #6]!
    3818:	2064656e 	rsbcs	r6, r4, lr, ror #10
    381c:	72616863 	rsbvc	r6, r1, #6488064	; 0x630000
    3820:	3028743a 	eorcc	r7, r8, sl, lsr r4
    3824:	2930312c 	ldmdbcs	r0!, {r2, r3, r5, r8, ip, sp}
    3828:	3028723d 	eorcc	r7, r8, sp, lsr r2
    382c:	2930312c 	ldmdbcs	r0!, {r2, r3, r5, r8, ip, sp}
    3830:	32312d3b 	eorscc	r2, r1, #3776	; 0xec0
    3834:	32313b38 	eorscc	r3, r1, #56, 22	; 0xe000
    3838:	75003b37 	strvc	r3, [r0, #-2871]	; 0xb37
    383c:	6769736e 	strbvs	r7, [r9, -lr, ror #6]!
    3840:	2064656e 	rsbcs	r6, r4, lr, ror #10
    3844:	72616863 	rsbvc	r6, r1, #6488064	; 0x630000
    3848:	3028743a 	eorcc	r7, r8, sl, lsr r4
    384c:	2931312c 	ldmdbcs	r1!, {r2, r3, r5, r8, ip, sp}
    3850:	3028723d 	eorcc	r7, r8, sp, lsr r2
    3854:	2931312c 	ldmdbcs	r1!, {r2, r3, r5, r8, ip, sp}
    3858:	323b303b 	eorscc	r3, fp, #59	; 0x3b
    385c:	003b3535 	eorseq	r3, fp, r5, lsr r5
    3860:	6b2f2e2e 	blvs	bcf120 <start_address+0xbbf120>
    3864:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
    3868:	6e692f6c 	cdpvs	15, 6, cr2, cr9, cr12, {3}
    386c:	64756c63 	ldrbtvs	r6, [r5], #-3171	; 0xc63
    3870:	79742f65 	ldmdbvc	r4!, {r0, r2, r5, r6, r8, r9, sl, fp, sp}^
    3874:	2e736570 	mrccs	5, 3, r6, cr3, cr0, {3}
    3878:	656d0068 	strbvs	r0, [sp, #-104]!	; 0x68
    387c:	7970636d 	ldmdbvc	r0!, {r0, r2, r3, r5, r6, r8, r9, sp, lr}^
    3880:	3028463a 	eorcc	r4, r8, sl, lsr r6
    3884:	2936342c 	ldmdbcs	r6!, {r2, r3, r5, sl, ip, sp}
    3888:	30282a3d 	eorcc	r2, r8, sp, lsr sl
    388c:	2935342c 	ldmdbcs	r5!, {r2, r3, r5, sl, ip, sp}
    3890:	72617400 	rsbvc	r7, r1, #0, 8
    3894:	3a746567 	bcc	1d1ce38 <start_address+0x1d0ce38>
    3898:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    389c:	00293634 	eoreq	r3, r9, r4, lsr r6
    38a0:	72756f73 	rsbsvc	r6, r5, #460	; 0x1cc
    38a4:	703a6563 	eorsvc	r6, sl, r3, ror #10
    38a8:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    38ac:	2a3d2937 	bcs	f4dd90 <start_address+0xf3dd90>
    38b0:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    38b4:	6e002935 	mcrvs	9, 0, r2, cr0, cr5, {1}
    38b8:	3228703a 	eorcc	r7, r8, #58	; 0x3a
    38bc:	0029392c 	eoreq	r3, r9, ip, lsr #18
    38c0:	67726174 			; <UNDEFINED> instruction: 0x67726174
    38c4:	625f7465 	subsvs	r7, pc, #1694498816	; 0x65000000
    38c8:	65666675 	strbvs	r6, [r6, #-1653]!	; 0x675
    38cc:	30283a72 	eorcc	r3, r8, r2, ror sl
    38d0:	2938342c 	ldmdbcs	r8!, {r2, r3, r5, sl, ip, sp}
    38d4:	30282a3d 	eorcc	r2, r8, sp, lsr sl
    38d8:	0029322c 	eoreq	r3, r9, ip, lsr #4
    38dc:	72756f73 	rsbsvc	r6, r5, #460	; 0x1cc
    38e0:	625f6563 	subsvs	r6, pc, #415236096	; 0x18c00000
    38e4:	65666675 	strbvs	r6, [r6, #-1653]!	; 0x675
    38e8:	30283a72 	eorcc	r3, r8, r2, ror sl
    38ec:	2938342c 	ldmdbcs	r8!, {r2, r3, r5, sl, ip, sp}
    38f0:	283a6900 	ldmdacs	sl!, {r8, fp, sp, lr}
    38f4:	29392c32 	ldmdbcs	r9!, {r1, r4, r5, sl, fp, sp}
    38f8:	72747300 	rsbsvc	r7, r4, #0, 6
    38fc:	3a797063 	bcc	1e5fa90 <start_address+0x1e4fa90>
    3900:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    3904:	00293834 	eoreq	r3, r9, r4, lsr r8
    3908:	67726174 			; <UNDEFINED> instruction: 0x67726174
    390c:	703a7465 	eorsvc	r7, sl, r5, ror #8
    3910:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    3914:	73002938 	movwvc	r2, #2360	; 0x938
    3918:	6372756f 	cmnvs	r2, #465567744	; 0x1bc00000
    391c:	28703a65 	ldmdacs	r0!, {r0, r2, r5, r6, r9, fp, ip, sp}^
    3920:	39342c30 	ldmdbcc	r4!, {r4, r5, sl, fp, sp}
    3924:	282a3d29 	stmdacs	sl!, {r0, r3, r5, r8, sl, fp, ip, sp}
    3928:	29322c30 	ldmdbcs	r2!, {r4, r5, sl, fp, sp}
    392c:	73657200 	cmnvc	r5, #0, 4
    3930:	3a746c75 	bcc	1d1eb0c <start_address+0x1d0eb0c>
    3934:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    3938:	73002938 	movwvc	r2, #2360	; 0x938
    393c:	636e7274 	cmnvs	lr, #116, 4	; 0x40000007
    3940:	463a7970 			; <UNDEFINED> instruction: 0x463a7970
    3944:	392c3228 	stmdbcc	ip!, {r3, r5, r9, ip, sp}
    3948:	6f730029 	svcvs	0x00730029
    394c:	65637275 	strbvs	r7, [r3, #-629]!	; 0x275
    3950:	3028703a 	eorcc	r7, r8, sl, lsr r0
    3954:	2939342c 	ldmdbcs	r9!, {r2, r3, r5, sl, ip, sp}
    3958:	756f7300 	strbvc	r7, [pc, #-768]!	; 3660 <start_address-0xc9a0>
    395c:	5f656372 	svcpl	0x00656372
    3960:	3a6e656c 	bcc	1b9cf18 <start_address+0x1b8cf18>
    3964:	392c3228 	stmdbcc	ip!, {r3, r5, r9, ip, sp}
    3968:	74730029 	ldrbtvc	r0, [r3], #-41	; 0x29
    396c:	706d6372 	rsbvc	r6, sp, r2, ror r3
    3970:	3028463a 	eorcc	r4, r8, sl, lsr r6
    3974:	0029312c 	eoreq	r3, r9, ip, lsr #2
    3978:	703a3173 	eorsvc	r3, sl, r3, ror r1
    397c:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    3980:	73002939 	movwvc	r2, #2361	; 0x939
    3984:	28703a32 	ldmdacs	r0!, {r1, r4, r5, r9, fp, ip, sp}^
    3988:	39342c30 	ldmdbcc	r4!, {r4, r5, sl, fp, sp}
    398c:	74730029 	ldrbtvc	r0, [r3], #-41	; 0x29
    3990:	6d636e72 	stclvs	14, cr6, [r3, #-456]!	; 0xfffffe38
    3994:	28463a70 	stmdacs	r6, {r4, r5, r6, r9, fp, ip, sp}^
    3998:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    399c:	72747300 	rsbsvc	r7, r4, #0, 6
    39a0:	3a726863 	bcc	1c9db34 <start_address+0x1c8db34>
    39a4:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    39a8:	00293834 	eoreq	r3, r9, r4, lsr r8
    39ac:	3a727473 	bcc	1ca0b80 <start_address+0x1c90b80>
    39b0:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    39b4:	00293934 	eoreq	r3, r9, r4, lsr r9
    39b8:	72616863 	rsbvc	r6, r1, #6488064	; 0x630000
    39bc:	65746361 	ldrbvs	r6, [r4, #-865]!	; 0x361
    39c0:	28703a72 	ldmdacs	r0!, {r1, r4, r5, r6, r9, fp, ip, sp}^
    39c4:	29312c30 	ldmdbcs	r1!, {r4, r5, sl, fp, sp}
    39c8:	72747300 	rsbsvc	r7, r4, #0, 6
    39cc:	3a6b6f74 	bcc	1adf7a4 <start_address+0x1acf7a4>
    39d0:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    39d4:	00293834 	eoreq	r3, r9, r4, lsr r8
    39d8:	3a727473 	bcc	1ca0bac <start_address+0x1c90bac>
    39dc:	2c302870 	ldccs	8, cr2, [r0], #-448	; 0xfffffe40
    39e0:	00293834 	eoreq	r3, r9, r4, lsr r8
    39e4:	696c6564 	stmdbvs	ip!, {r2, r5, r6, r8, sl, sp, lr}^
    39e8:	6574696d 	ldrbvs	r6, [r4, #-2413]!	; 0x96d
    39ec:	703a7372 	eorsvc	r7, sl, r2, ror r3
    39f0:	342c3028 	strtcc	r3, [ip], #-40	; 0x28
    39f4:	6c002939 	stcvs	9, cr2, [r0], {57}	; 0x39
    39f8:	3a747361 	bcc	1d20784 <start_address+0x1d10784>
    39fc:	2c302856 	ldccs	8, cr2, [r0], #-344	; 0xfffffea8
    3a00:	00293834 	eoreq	r3, r9, r4, lsr r8
    3a04:	656b6f74 	strbvs	r6, [fp, #-3956]!	; 0xf74
    3a08:	30283a6e 	eorcc	r3, r8, lr, ror #20
    3a0c:	2938342c 	ldmdbcs	r8!, {r2, r3, r5, sl, ip, sp}
    3a10:	6d656d00 	stclvs	13, cr6, [r5, #-0]
    3a14:	3a746573 	bcc	1d1cfe8 <start_address+0x1d0cfe8>
    3a18:	2c302846 	ldccs	8, cr2, [r0], #-280	; 0xfffffee8
    3a1c:	00293634 	eoreq	r3, r9, r4, lsr r6
    3a20:	3a6e656c 	bcc	1b9cfd8 <start_address+0x1b8cfd8>
    3a24:	2c322870 	ldccs	8, cr2, [r2], #-448	; 0xfffffe40
    3a28:	74002939 	strvc	r2, [r0], #-2361	; 0x939
    3a2c:	65677261 	strbvs	r7, [r7, #-609]!	; 0x261
    3a30:	75625f74 	strbvc	r5, [r2, #-3956]!	; 0xf74
    3a34:	72656666 	rsbvc	r6, r5, #106954752	; 0x6600000
    3a38:	2c30283a 	ldccs	8, cr2, [r0], #-232	; 0xffffff18
    3a3c:	00293834 	eoreq	r3, r9, r4, lsr r8
    3a40:	6c727473 	cfldrdvs	mvd7, [r2], #-460	; 0xfffffe34
    3a44:	463a6e65 	ldrtmi	r6, [sl], -r5, ror #28
    3a48:	392c3228 	stmdbcc	ip!, {r3, r5, r9, ip, sp}
    3a4c:	656c0029 	strbvs	r0, [ip, #-41]!	; 0x29
    3a50:	6874676e 	ldmdavs	r4!, {r1, r2, r3, r5, r6, r8, r9, sl, sp, lr}^
    3a54:	2c32283a 	ldccs	8, cr2, [r2], #-232	; 0xffffff18
    3a58:	Address 0x0000000000003a58 is out of bounds.

