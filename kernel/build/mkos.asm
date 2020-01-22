
build/mkos.elf:     file format elf32-littlearm


Disassembly of section .text:

80008000 <__entry>:
80008000:	e329f0d3 	msr	CPSR_fc, #211	; 0xd3
80008004:	e59fd050 	ldr	sp, [pc, #80]	; 8000805c <halt+0x4>
80008008:	e24dd102 	sub	sp, sp, #-2147483648	; 0x80000000
8000800c:	eb001863 	bl	8000e1a0 <_boot_init>
80008010:	eb000065 	bl	800081ac <__enable_paging>
80008014:	eb00006f 	bl	800081d8 <__jump2_high_mem>
80008018:	e329f0d2 	msr	CPSR_fc, #210	; 0xd2
8000801c:	e59fd03c 	ldr	sp, [pc, #60]	; 80008060 <halt+0x8>
80008020:	e329f0d7 	msr	CPSR_fc, #215	; 0xd7
80008024:	e59fd038 	ldr	sp, [pc, #56]	; 80008064 <halt+0xc>
80008028:	e329f0d3 	msr	CPSR_fc, #211	; 0xd3
8000802c:	e59fd028 	ldr	sp, [pc, #40]	; 8000805c <halt+0x4>
80008030:	eb00183c 	bl	8000e128 <_copy_interrupt_table>
80008034:	eb000078 	bl	8000821c <__use_high_interrupts>
80008038:	e24dd044 	sub	sp, sp, #68	; 0x44
8000803c:	e1a0000d 	mov	r0, sp
80008040:	eb00270e 	bl	80011c80 <_kernel_entry_c>
80008044:	e8bd4001 	pop	{r0, lr}
80008048:	e169f000 	msr	SPSR_fc, r0
8000804c:	e8dd7fff 	ldm	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
80008050:	e28dd03c 	add	sp, sp, #60	; 0x3c
80008054:	e1b0f00e 	movs	pc, lr

80008058 <halt>:
80008058:	eafffffe 	b	80008058 <halt>
8000805c:	80269000 	eorhi	r9, r6, r0
80008060:	80249000 	eorhi	r9, r4, r0
80008064:	80259000 	eorhi	r9, r5, r0

80008068 <interrupt_table_start>:
80008068:	e59ff014 	ldr	pc, [pc, #20]	; 80008084 <reset_entry_address>
8000806c:	eafffffe 	b	8000806c <interrupt_table_start+0x4>
80008070:	e59ff010 	ldr	pc, [pc, #16]	; 80008088 <svc_entry_address>
80008074:	e59ff014 	ldr	pc, [pc, #20]	; 80008090 <prefetch_abort_entry_address>
80008078:	e59ff014 	ldr	pc, [pc, #20]	; 80008094 <data_abort_entry_address>
8000807c:	eafffffe 	b	8000807c <interrupt_table_start+0x14>
80008080:	e59ff004 	ldr	pc, [pc, #4]	; 8000808c <irq_entry_address>

80008084 <reset_entry_address>:
80008084:	80008000 	andhi	r8, r0, r0

80008088 <svc_entry_address>:
80008088:	80008098 	mulhi	r0, r8, r0

8000808c <irq_entry_address>:
8000808c:	800080e8 	andhi	r8, r0, r8, ror #1

80008090 <prefetch_abort_entry_address>:
80008090:	8000811c 	andhi	r8, r0, ip, lsl r1

80008094 <data_abort_entry_address>:
80008094:	8000814c 	andhi	r8, r0, ip, asr #2

80008098 <interrupt_table_end>:
80008098:	e24ee000 	sub	lr, lr, #0
8000809c:	e24dd03c 	sub	sp, sp, #60	; 0x3c
800080a0:	e8cd7fff 	stmia	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
800080a4:	e14f5000 	mrs	r5, SPSR
800080a8:	e92d4020 	push	{r5, lr}
800080ac:	e329f0d3 	msr	CPSR_fc, #211	; 0xd3
800080b0:	e28d5044 	add	r5, sp, #68	; 0x44
800080b4:	e8950020 	ldm	r5, {r5}
800080b8:	e205501f 	and	r5, r5, #31
800080bc:	e1a0400d 	mov	r4, sp
800080c0:	e92d0020 	stmfd	sp!, {r5}
800080c4:	e92d0010 	stmfd	sp!, {r4}
800080c8:	eb001985 	bl	8000e6e4 <svc_handler>
800080cc:	e8bd0010 	ldmfd	sp!, {r4}
800080d0:	e8bd0020 	ldmfd	sp!, {r5}
800080d4:	e8bd4002 	pop	{r1, lr}
800080d8:	e169f001 	msr	SPSR_fc, r1
800080dc:	e8dd7fff 	ldm	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
800080e0:	e28dd03c 	add	sp, sp, #60	; 0x3c
800080e4:	e1b0f00e 	movs	pc, lr

800080e8 <irq_entry>:
800080e8:	e24ee004 	sub	lr, lr, #4
800080ec:	e329f0d2 	msr	CPSR_fc, #210	; 0xd2
800080f0:	e24dd03c 	sub	sp, sp, #60	; 0x3c
800080f4:	e8cd7fff 	stmia	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
800080f8:	e14f0000 	mrs	r0, SPSR
800080fc:	e92d4001 	push	{r0, lr}
80008100:	e1a0000d 	mov	r0, sp
80008104:	eb001878 	bl	8000e2ec <irq_handler>
80008108:	e8bd4001 	pop	{r0, lr}
8000810c:	e169f000 	msr	SPSR_fc, r0
80008110:	e8dd7fff 	ldm	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
80008114:	e28dd03c 	add	sp, sp, #60	; 0x3c
80008118:	e1b0f00e 	movs	pc, lr

8000811c <prefetch_abort_entry>:
8000811c:	e24ee004 	sub	lr, lr, #4
80008120:	e329f0d7 	msr	CPSR_fc, #215	; 0xd7
80008124:	e8cd7fff 	stmia	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
80008128:	e14f0000 	mrs	r0, SPSR
8000812c:	e92d4001 	push	{r0, lr}
80008130:	e1a0000d 	mov	r0, sp
80008134:	eb0018ad 	bl	8000e3f0 <prefetch_abort_handler>
80008138:	e8bd4001 	pop	{r0, lr}
8000813c:	e169f000 	msr	SPSR_fc, r0
80008140:	e8dd7fff 	ldm	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
80008144:	e28dd03c 	add	sp, sp, #60	; 0x3c
80008148:	e1b0f00e 	movs	pc, lr

8000814c <data_abort_entry>:
8000814c:	e24ee008 	sub	lr, lr, #8
80008150:	e329f0d7 	msr	CPSR_fc, #215	; 0xd7
80008154:	e8cd7fff 	stmia	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
80008158:	e14f0000 	mrs	r0, SPSR
8000815c:	e92d4001 	push	{r0, lr}
80008160:	e1a0000d 	mov	r0, sp
80008164:	eb0018bb 	bl	8000e458 <data_abort_handler>
80008168:	e8bd4001 	pop	{r0, lr}
8000816c:	e169f000 	msr	SPSR_fc, r0
80008170:	e8dd7fff 	ldm	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}^
80008174:	e28dd03c 	add	sp, sp, #60	; 0x3c
80008178:	e1b0f00e 	movs	pc, lr

8000817c <__read_control_register>:
8000817c:	ee110f10 	mrc	15, 0, r0, cr1, cr0, {0}
80008180:	e1a0f00e 	mov	pc, lr

80008184 <__set_control_register>:
80008184:	ee010f10 	mcr	15, 0, r0, cr1, cr0, {0}
80008188:	e1a0f00e 	mov	pc, lr

8000818c <__set_domain_access_control>:
8000818c:	ee030f10 	mcr	15, 0, r0, cr3, cr0, {0}
80008190:	e1a0f00e 	mov	pc, lr

80008194 <__set_translation_table_base>:
80008194:	ee020f10 	mcr	15, 0, r0, cr2, cr0, {0}
80008198:	ee020f30 	mcr	15, 0, r0, cr2, cr0, {1}
8000819c:	e3a00000 	mov	r0, #0
800081a0:	ee080f17 	mcr	15, 0, r0, cr8, cr7, {0}
800081a4:	ee070f9a 	mcr	15, 0, r0, cr7, cr10, {4}
800081a8:	e1a0f00e 	mov	pc, lr

800081ac <__enable_paging>:
800081ac:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
800081b0:	e3a00001 	mov	r0, #1
800081b4:	ebfffff4 	bl	8000818c <__set_domain_access_control>
800081b8:	e59f0084 	ldr	r0, [pc, #132]	; 80008244 <__dummy+0x4>
800081bc:	e2400102 	sub	r0, r0, #-2147483648	; 0x80000000
800081c0:	ebfffff3 	bl	80008194 <__set_translation_table_base>
800081c4:	ebffffec 	bl	8000817c <__read_control_register>
800081c8:	e3800001 	orr	r0, r0, #1
800081cc:	ebffffec 	bl	80008184 <__set_control_register>
800081d0:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
800081d4:	e1a0f00e 	mov	pc, lr

800081d8 <__jump2_high_mem>:
800081d8:	e28ee102 	add	lr, lr, #-2147483648	; 0x80000000
800081dc:	e1a0f00e 	mov	pc, lr

800081e0 <__irq_enable>:
800081e0:	e10f0000 	mrs	r0, CPSR
800081e4:	e3c00080 	bic	r0, r0, #128	; 0x80
800081e8:	e121f000 	msr	CPSR_c, r0
800081ec:	e1a0f00e 	mov	pc, lr

800081f0 <__irq_disable>:
800081f0:	e10f0000 	mrs	r0, CPSR
800081f4:	e3800080 	orr	r0, r0, #128	; 0x80
800081f8:	e121f000 	msr	CPSR_c, r0
800081fc:	e1a0f00e 	mov	pc, lr

80008200 <__int_off>:
80008200:	e10f0000 	mrs	r0, CPSR
80008204:	e1a01000 	mov	r1, r0
80008208:	e3811080 	orr	r1, r1, #128	; 0x80
8000820c:	e129f001 	msr	CPSR_fc, r1
80008210:	e1a0f00e 	mov	pc, lr

80008214 <__int_on>:
80008214:	e129f000 	msr	CPSR_fc, r0
80008218:	e1a0f00e 	mov	pc, lr

8000821c <__use_high_interrupts>:
8000821c:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
80008220:	ebffffd5 	bl	8000817c <__read_control_register>
80008224:	e3800a02 	orr	r0, r0, #8192	; 0x2000
80008228:	ebffffd5 	bl	80008184 <__set_control_register>
8000822c:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
80008230:	e1a0f00e 	mov	pc, lr

80008234 <__mem_barrier>:
80008234:	e3a00000 	mov	r0, #0
80008238:	ee070fba 	mcr	15, 0, r0, cr7, cr10, {5}
8000823c:	e1a0f00e 	mov	pc, lr

80008240 <__dummy>:
80008240:	e12fff1e 	bx	lr
80008244:	80018000 	andhi	r8, r1, r0

80008248 <__memcpy32>:
80008248:	e92d07f8 	push	{r3, r4, r5, r6, r7, r8, r9, sl}

8000824c <ldmloop>:
8000824c:	e8b107f8 	ldm	r1!, {r3, r4, r5, r6, r7, r8, r9, sl}
80008250:	e8a007f8 	stmia	r0!, {r3, r4, r5, r6, r7, r8, r9, sl}
80008254:	e2522020 	subs	r2, r2, #32
80008258:	cafffffb 	bgt	8000824c <ldmloop>
8000825c:	e8bd07f8 	pop	{r3, r4, r5, r6, r7, r8, r9, sl}
80008260:	e1a0f00e 	mov	pc, lr

80008264 <hw_info_init>:
80008264:	e1a0c00d 	mov	ip, sp
80008268:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000826c:	e59f402c 	ldr	r4, [pc, #44]	; 800082a0 <hw_info_init+0x3c>
80008270:	e59f102c 	ldr	r1, [pc, #44]	; 800082a4 <hw_info_init+0x40>
80008274:	e08f4004 	add	r4, pc, r4
80008278:	e24cb004 	sub	fp, ip, #4
8000827c:	e08f1001 	add	r1, pc, r1
80008280:	e1a00004 	mov	r0, r4
80008284:	eb000aa9 	bl	8000ad30 <strcpy>
80008288:	e3a03201 	mov	r3, #268435456	; 0x10000000
8000828c:	e3a02501 	mov	r2, #4194304	; 0x400000
80008290:	e5842028 	str	r2, [r4, #40]	; 0x28
80008294:	e5843020 	str	r3, [r4, #32]
80008298:	e5843024 	str	r3, [r4, #36]	; 0x24
8000829c:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
800082a0:	00017d84 	andeq	r7, r1, r4, lsl #27
800082a4:	00009d18 	andeq	r9, r0, r8, lsl sp

800082a8 <get_hw_info>:
800082a8:	e59f0004 	ldr	r0, [pc, #4]	; 800082b4 <get_hw_info+0xc>
800082ac:	e08f0000 	add	r0, pc, r0
800082b0:	e12fff1e 	bx	lr
800082b4:	00017d4c 	andeq	r7, r1, ip, asr #26

800082b8 <arch_vm>:
800082b8:	e12fff1e 	bx	lr

800082bc <hw_optimise>:
800082bc:	e12fff1e 	bx	lr

800082c0 <uart_dev_init>:
800082c0:	e59f2020 	ldr	r2, [pc, #32]	; 800082e8 <uart_dev_init+0x28>
800082c4:	e59f3020 	ldr	r3, [pc, #32]	; 800082ec <uart_dev_init+0x2c>
800082c8:	e08f2002 	add	r2, pc, r2
800082cc:	e7923003 	ldr	r3, [r2, r3]
800082d0:	e59f1018 	ldr	r1, [pc, #24]	; 800082f0 <uart_dev_init+0x30>
800082d4:	e5933000 	ldr	r3, [r3]
800082d8:	e3a02010 	mov	r2, #16
800082dc:	e3a00000 	mov	r0, #0
800082e0:	e7832001 	str	r2, [r3, r1]
800082e4:	e12fff1e 	bx	lr
800082e8:	00013d34 	andeq	r3, r1, r4, lsr sp
800082ec:	00000010 	andeq	r0, r0, r0, lsl r0
800082f0:	001f1038 	andseq	r1, pc, r8, lsr r0	; <UNPREDICTABLE>

800082f4 <uart_write>:
800082f4:	e2520000 	subs	r0, r2, #0
800082f8:	e59f2058 	ldr	r2, [pc, #88]	; 80008358 <uart_write+0x64>
800082fc:	e92d4070 	push	{r4, r5, r6, lr}
80008300:	e08f2002 	add	r2, pc, r2
80008304:	da000011 	ble	80008350 <uart_write+0x5c>
80008308:	e59f304c 	ldr	r3, [pc, #76]	; 8000835c <uart_write+0x68>
8000830c:	e0815000 	add	r5, r1, r0
80008310:	e7926003 	ldr	r6, [r2, r3]
80008314:	e596e000 	ldr	lr, [r6]
80008318:	e59fc040 	ldr	ip, [pc, #64]	; 80008360 <uart_write+0x6c>
8000831c:	e4d14001 	ldrb	r4, [r1], #1
80008320:	e08ec00c 	add	ip, lr, ip
80008324:	e59c3000 	ldr	r3, [ip]
80008328:	e3130020 	tst	r3, #32
8000832c:	1afffffc 	bne	80008324 <uart_write+0x30>
80008330:	e354000d 	cmp	r4, #13
80008334:	e28ee81f 	add	lr, lr, #2031616	; 0x1f0000
80008338:	03a0400a 	moveq	r4, #10
8000833c:	e28eea01 	add	lr, lr, #4096	; 0x1000
80008340:	e1510005 	cmp	r1, r5
80008344:	e5ce4000 	strb	r4, [lr]
80008348:	1afffff1 	bne	80008314 <uart_write+0x20>
8000834c:	e8bd8070 	pop	{r4, r5, r6, pc}
80008350:	e3a00000 	mov	r0, #0
80008354:	e8bd8070 	pop	{r4, r5, r6, pc}
80008358:	00013cfc 	strdeq	r3, [r1], -ip
8000835c:	00000010 	andeq	r0, r0, r0, lsl r0
80008360:	001f1060 	andseq	r1, pc, r0, rrx

80008364 <irq_arch_init>:
80008364:	e12fff1e 	bx	lr

80008368 <gic_set_irqs>:
80008368:	e59f206c 	ldr	r2, [pc, #108]	; 800083dc <gic_set_irqs+0x74>
8000836c:	e59f306c 	ldr	r3, [pc, #108]	; 800083e0 <gic_set_irqs+0x78>
80008370:	e08f2002 	add	r2, pc, r2
80008374:	e7923003 	ldr	r3, [r2, r3]
80008378:	e3100001 	tst	r0, #1
8000837c:	e5933000 	ldr	r3, [r3]
80008380:	e2832705 	add	r2, r3, #1310720	; 0x140000
80008384:	e5921010 	ldr	r1, [r2, #16]
80008388:	e2833a03 	add	r3, r3, #12288	; 0x3000
8000838c:	13811142 	orrne	r1, r1, #-2147483632	; 0x80000010
80008390:	03811102 	orreq	r1, r1, #-2147483648	; 0x80000000
80008394:	e3100010 	tst	r0, #16
80008398:	e5821010 	str	r1, [r2, #16]
8000839c:	15921010 	ldrne	r1, [r2, #16]
800083a0:	13811a01 	orrne	r1, r1, #4096	; 0x1000
800083a4:	15821010 	strne	r1, [r2, #16]
800083a8:	e3100c02 	tst	r0, #512	; 0x200
800083ac:	15932008 	ldrne	r2, [r3, #8]
800083b0:	13822008 	orrne	r2, r2, #8
800083b4:	15832008 	strne	r2, [r3, #8]
800083b8:	e3100b01 	tst	r0, #1024	; 0x400
800083bc:	15932008 	ldrne	r2, [r3, #8]
800083c0:	13822010 	orrne	r2, r2, #16
800083c4:	15832008 	strne	r2, [r3, #8]
800083c8:	e3100b02 	tst	r0, #2048	; 0x800
800083cc:	15932008 	ldrne	r2, [r3, #8]
800083d0:	13822501 	orrne	r2, r2, #4194304	; 0x400000
800083d4:	15832008 	strne	r2, [r3, #8]
800083d8:	e12fff1e 	bx	lr
800083dc:	00013c8c 	andeq	r3, r1, ip, lsl #25
800083e0:	00000010 	andeq	r0, r0, r0, lsl r0

800083e4 <gic_get_irqs>:
800083e4:	e59f3050 	ldr	r3, [pc, #80]	; 8000843c <gic_get_irqs+0x58>
800083e8:	e59f2050 	ldr	r2, [pc, #80]	; 80008440 <gic_get_irqs+0x5c>
800083ec:	e08f3003 	add	r3, pc, r3
800083f0:	e7933002 	ldr	r3, [r3, r2]
800083f4:	e5932000 	ldr	r2, [r3]
800083f8:	e2823705 	add	r3, r2, #1310720	; 0x140000
800083fc:	e5933000 	ldr	r3, [r3]
80008400:	e1a00223 	lsr	r0, r3, #4
80008404:	e3130a01 	tst	r3, #4096	; 0x1000
80008408:	e2000001 	and	r0, r0, #1
8000840c:	13800010 	orrne	r0, r0, #16
80008410:	e3530000 	cmp	r3, #0
80008414:	a12fff1e 	bxge	lr
80008418:	e2822a03 	add	r2, r2, #12288	; 0x3000
8000841c:	e5923000 	ldr	r3, [r2]
80008420:	e3130008 	tst	r3, #8
80008424:	13800c02 	orrne	r0, r0, #512	; 0x200
80008428:	e3130010 	tst	r3, #16
8000842c:	13800b01 	orrne	r0, r0, #1024	; 0x400
80008430:	e3130501 	tst	r3, #4194304	; 0x400000
80008434:	13800b02 	orrne	r0, r0, #2048	; 0x800
80008438:	e12fff1e 	bx	lr
8000843c:	00013c10 	andeq	r3, r1, r0, lsl ip
80008440:	00000010 	andeq	r0, r0, r0, lsl r0

80008444 <fb_dev_init>:
80008444:	e1a0c00d 	mov	ip, sp
80008448:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000844c:	e59f4180 	ldr	r4, [pc, #384]	; 800085d4 <fb_dev_init+0x190>
80008450:	e1a06000 	mov	r6, r0
80008454:	e08f4004 	add	r4, pc, r4
80008458:	e1a05001 	mov	r5, r1
8000845c:	e3a02028 	mov	r2, #40	; 0x28
80008460:	e3a01000 	mov	r1, #0
80008464:	e24cb004 	sub	fp, ip, #4
80008468:	e1a00004 	mov	r0, r4
8000846c:	eb0009ff 	bl	8000ac70 <memset>
80008470:	e59f1160 	ldr	r1, [pc, #352]	; 800085d8 <fb_dev_init+0x194>
80008474:	e59f3160 	ldr	r3, [pc, #352]	; 800085dc <fb_dev_init+0x198>
80008478:	e3a02000 	mov	r2, #0
8000847c:	e08f1001 	add	r1, pc, r1
80008480:	e3a00020 	mov	r0, #32
80008484:	e5846000 	str	r6, [r4]
80008488:	e9840060 	stmib	r4, {r5, r6}
8000848c:	e584500c 	str	r5, [r4, #12]
80008490:	e5840014 	str	r0, [r4, #20]
80008494:	e5842010 	str	r2, [r4, #16]
80008498:	e5842018 	str	r2, [r4, #24]
8000849c:	e584201c 	str	r2, [r4, #28]
800084a0:	e7913003 	ldr	r3, [r1, r3]
800084a4:	e3560d0a 	cmp	r6, #640	; 0x280
800084a8:	03550e1e 	cmpeq	r5, #480	; 0x1e0
800084ac:	e5843020 	str	r3, [r4, #32]
800084b0:	e59f3128 	ldr	r3, [pc, #296]	; 800085e0 <fb_dev_init+0x19c>
800084b4:	0a00003c 	beq	800085ac <fb_dev_init+0x168>
800084b8:	e3560e32 	cmp	r6, #800	; 0x320
800084bc:	03550f96 	cmpeq	r5, #600	; 0x258
800084c0:	0a000025 	beq	8000855c <fb_dev_init+0x118>
800084c4:	e3a00b01 	mov	r0, #1024	; 0x400
800084c8:	e3a02c03 	mov	r2, #768	; 0x300
800084cc:	e5840000 	str	r0, [r4]
800084d0:	e5840008 	str	r0, [r4, #8]
800084d4:	e5842004 	str	r2, [r4, #4]
800084d8:	e584200c 	str	r2, [r4, #12]
800084dc:	e7913003 	ldr	r3, [r1, r3]
800084e0:	e3a000fc 	mov	r0, #252	; 0xfc
800084e4:	e5932000 	ldr	r2, [r3]
800084e8:	e59f10f4 	ldr	r1, [pc, #244]	; 800085e4 <fb_dev_init+0x1a0>
800084ec:	e3822812 	orr	r2, r2, #1179648	; 0x120000
800084f0:	e5820000 	str	r0, [r2]
800084f4:	e5932000 	ldr	r2, [r3]
800084f8:	e3822812 	orr	r2, r2, #1179648	; 0x120000
800084fc:	e3822004 	orr	r2, r2, #4
80008500:	e5821000 	str	r1, [r2]
80008504:	e59f20dc 	ldr	r2, [pc, #220]	; 800085e8 <fb_dev_init+0x1a4>
80008508:	e5931000 	ldr	r1, [r3]
8000850c:	e08f2002 	add	r2, pc, r2
80008510:	e5920020 	ldr	r0, [r2, #32]
80008514:	e3811812 	orr	r1, r1, #1179648	; 0x120000
80008518:	e2800102 	add	r0, r0, #-2147483648	; 0x80000000
8000851c:	e3811010 	orr	r1, r1, #16
80008520:	e5810000 	str	r0, [r1]
80008524:	e5933000 	ldr	r3, [r3]
80008528:	e59f10bc 	ldr	r1, [pc, #188]	; 800085ec <fb_dev_init+0x1a8>
8000852c:	e3833812 	orr	r3, r3, #1179648	; 0x120000
80008530:	e3833018 	orr	r3, r3, #24
80008534:	e5831000 	str	r1, [r3]
80008538:	e5920004 	ldr	r0, [r2, #4]
8000853c:	e5921000 	ldr	r1, [r2]
80008540:	e5923014 	ldr	r3, [r2, #20]
80008544:	e0010190 	mul	r1, r0, r1
80008548:	e1a031a3 	lsr	r3, r3, #3
8000854c:	e0030391 	mul	r3, r1, r3
80008550:	e3a00000 	mov	r0, #0
80008554:	e5823024 	str	r3, [r2, #36]	; 0x24
80008558:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000855c:	e7913003 	ldr	r3, [r1, r3]
80008560:	e59fe088 	ldr	lr, [pc, #136]	; 800085f0 <fb_dev_init+0x1ac>
80008564:	e5932000 	ldr	r2, [r3]
80008568:	e59fc084 	ldr	ip, [pc, #132]	; 800085f4 <fb_dev_init+0x1b0>
8000856c:	e382201c 	orr	r2, r2, #28
80008570:	e582e000 	str	lr, [r2]
80008574:	e59f007c 	ldr	r0, [pc, #124]	; 800085f8 <fb_dev_init+0x1b4>
80008578:	e5932000 	ldr	r2, [r3]
8000857c:	e59f1078 	ldr	r1, [pc, #120]	; 800085fc <fb_dev_init+0x1b8>
80008580:	e3822812 	orr	r2, r2, #1179648	; 0x120000
80008584:	e582c000 	str	ip, [r2]
80008588:	e5932000 	ldr	r2, [r3]
8000858c:	e3822812 	orr	r2, r2, #1179648	; 0x120000
80008590:	e3822004 	orr	r2, r2, #4
80008594:	e5820000 	str	r0, [r2]
80008598:	e5932000 	ldr	r2, [r3]
8000859c:	e3822812 	orr	r2, r2, #1179648	; 0x120000
800085a0:	e3822008 	orr	r2, r2, #8
800085a4:	e5821000 	str	r1, [r2]
800085a8:	eaffffd5 	b	80008504 <fb_dev_init+0xc0>
800085ac:	e7913003 	ldr	r3, [r1, r3]
800085b0:	e59fe048 	ldr	lr, [pc, #72]	; 80008600 <fb_dev_init+0x1bc>
800085b4:	e5932000 	ldr	r2, [r3]
800085b8:	e59fc044 	ldr	ip, [pc, #68]	; 80008604 <fb_dev_init+0x1c0>
800085bc:	e382201c 	orr	r2, r2, #28
800085c0:	e582e000 	str	lr, [r2]
800085c4:	e59f003c 	ldr	r0, [pc, #60]	; 80008608 <fb_dev_init+0x1c4>
800085c8:	e5932000 	ldr	r2, [r3]
800085cc:	e59f1038 	ldr	r1, [pc, #56]	; 8000860c <fb_dev_init+0x1c8>
800085d0:	eaffffea 	b	80008580 <fb_dev_init+0x13c>
800085d4:	00017bd4 	ldrdeq	r7, [r1], -r4
800085d8:	00013b80 	andeq	r3, r1, r0, lsl #23
800085dc:	0000002c 	andeq	r0, r0, ip, lsr #32
800085e0:	00000010 	andeq	r0, r0, r0, lsl r0
800085e4:	000002ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>
800085e8:	00017b1c 	andeq	r7, r1, ip, lsl fp
800085ec:	0000082b 	andeq	r0, r0, fp, lsr #16
800085f0:	00002cac 	andeq	r2, r0, ip, lsr #25
800085f4:	1313a4c4 	tstne	r3, #196, 8	; 0xc4000000
800085f8:	0505f6f7 	streq	pc, [r5, #-1783]	; 0x6f7
800085fc:	071f1800 	ldreq	r1, [pc, -r0, lsl #16]
80008600:	00002c77 	andeq	r2, r0, r7, ror ip
80008604:	3f1f3f9c 	svccc	0x001f3f9c
80008608:	090b61df 	stmdbeq	fp, {r0, r1, r2, r3, r4, r6, r7, r8, sp, lr}
8000860c:	067f1800 	ldrbteq	r1, [pc], -r0, lsl #16

80008610 <fb_get_info>:
80008610:	e59f0004 	ldr	r0, [pc, #4]	; 8000861c <fb_get_info+0xc>
80008614:	e08f0000 	add	r0, pc, r0
80008618:	e12fff1e 	bx	lr
8000861c:	00017a14 	andeq	r7, r1, r4, lsl sl

80008620 <fb_dev_write>:
80008620:	e59f0040 	ldr	r0, [pc, #64]	; 80008668 <fb_dev_write+0x48>
80008624:	e1a0c00d 	mov	ip, sp
80008628:	e08f0000 	add	r0, pc, r0
8000862c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80008630:	e5903014 	ldr	r3, [r0, #20]
80008634:	e24cb004 	sub	fp, ip, #4
80008638:	e8905000 	ldm	r0, {ip, lr}
8000863c:	e5900020 	ldr	r0, [r0, #32]
80008640:	e00c0c9e 	mul	ip, lr, ip
80008644:	e1a031a3 	lsr	r3, r3, #3
80008648:	e003039c 	mul	r3, ip, r3
8000864c:	e1520003 	cmp	r2, r3
80008650:	31a04002 	movcc	r4, r2
80008654:	21a04003 	movcs	r4, r3
80008658:	e1a02004 	mov	r2, r4
8000865c:	eb000967 	bl	8000ac00 <memcpy>
80008660:	e1a00004 	mov	r0, r4
80008664:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
80008668:	00017a00 	andeq	r7, r1, r0, lsl #20

8000866c <act_led>:
8000866c:	e12fff1e 	bx	lr

80008670 <sd_init>:
80008670:	e1a0c00d 	mov	ip, sp
80008674:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
80008678:	e59f4140 	ldr	r4, [pc, #320]	; 800087c0 <sd_init+0x150>
8000867c:	e1a05000 	mov	r5, r0
80008680:	e08f4004 	add	r4, pc, r4
80008684:	e24cb004 	sub	fp, ip, #4
80008688:	e3a01000 	mov	r1, #0
8000868c:	e1a00004 	mov	r0, r4
80008690:	e59f212c 	ldr	r2, [pc, #300]	; 800087c4 <sd_init+0x154>
80008694:	eb000975 	bl	8000ac70 <memset>
80008698:	e59f2128 	ldr	r2, [pc, #296]	; 800087c8 <sd_init+0x158>
8000869c:	e59f3128 	ldr	r3, [pc, #296]	; 800087cc <sd_init+0x15c>
800086a0:	e3a0c001 	mov	ip, #1
800086a4:	e08f2002 	add	r2, pc, r2
800086a8:	e3a0ec02 	mov	lr, #512	; 0x200
800086ac:	e584c410 	str	ip, [r4, #1040]	; 0x410
800086b0:	e584c414 	str	ip, [r4, #1044]	; 0x414
800086b4:	e58540ac 	str	r4, [r5, #172]	; 0xac
800086b8:	e585e0b0 	str	lr, [r5, #176]	; 0xb0
800086bc:	e7923003 	ldr	r3, [r2, r3]
800086c0:	e3a000bf 	mov	r0, #191	; 0xbf
800086c4:	e5932000 	ldr	r2, [r3]
800086c8:	e3a010c6 	mov	r1, #198	; 0xc6
800086cc:	e2822a05 	add	r2, r2, #20480	; 0x5000
800086d0:	e5820000 	str	r0, [r2]
800086d4:	e5932000 	ldr	r2, [r3]
800086d8:	e3a00000 	mov	r0, #0
800086dc:	e2822a05 	add	r2, r2, #20480	; 0x5000
800086e0:	e5821004 	str	r1, [r2, #4]
800086e4:	e5932000 	ldr	r2, [r3]
800086e8:	e3a01b01 	mov	r1, #1024	; 0x400
800086ec:	e2822a05 	add	r2, r2, #20480	; 0x5000
800086f0:	e5820008 	str	r0, [r2, #8]
800086f4:	e5932000 	ldr	r2, [r3]
800086f8:	e59f50d0 	ldr	r5, [pc, #208]	; 800087d0 <sd_init+0x160>
800086fc:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008700:	e582100c 	str	r1, [r2, #12]
80008704:	e5932000 	ldr	r2, [r3]
80008708:	e59f40c4 	ldr	r4, [pc, #196]	; 800087d4 <sd_init+0x164>
8000870c:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008710:	e5820008 	str	r0, [r2, #8]
80008714:	e5932000 	ldr	r2, [r3]
80008718:	e59f10b8 	ldr	r1, [pc, #184]	; 800087d8 <sd_init+0x168>
8000871c:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008720:	e582500c 	str	r5, [r2, #12]
80008724:	e5932000 	ldr	r2, [r3]
80008728:	e59f50ac 	ldr	r5, [pc, #172]	; 800087dc <sd_init+0x16c>
8000872c:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008730:	e582c008 	str	ip, [r2, #8]
80008734:	e5932000 	ldr	r2, [r3]
80008738:	e59f60a0 	ldr	r6, [pc, #160]	; 800087e0 <sd_init+0x170>
8000873c:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008740:	e582400c 	str	r4, [r2, #12]
80008744:	e5932000 	ldr	r2, [r3]
80008748:	e28440de 	add	r4, r4, #222	; 0xde
8000874c:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008750:	e5820008 	str	r0, [r2, #8]
80008754:	e5932000 	ldr	r2, [r3]
80008758:	e3a0ce55 	mov	ip, #1360	; 0x550
8000875c:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008760:	e582100c 	str	r1, [r2, #12]
80008764:	e5932000 	ldr	r2, [r3]
80008768:	e3a01806 	mov	r1, #393216	; 0x60000
8000876c:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008770:	e5825008 	str	r5, [r2, #8]
80008774:	e5932000 	ldr	r2, [r3]
80008778:	e2822a05 	add	r2, r2, #20480	; 0x5000
8000877c:	e582600c 	str	r6, [r2, #12]
80008780:	e5932000 	ldr	r2, [r3]
80008784:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008788:	e5825008 	str	r5, [r2, #8]
8000878c:	e5932000 	ldr	r2, [r3]
80008790:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008794:	e582400c 	str	r4, [r2, #12]
80008798:	e5932000 	ldr	r2, [r3]
8000879c:	e2822a05 	add	r2, r2, #20480	; 0x5000
800087a0:	e582e008 	str	lr, [r2, #8]
800087a4:	e5932000 	ldr	r2, [r3]
800087a8:	e2822a05 	add	r2, r2, #20480	; 0x5000
800087ac:	e582c00c 	str	ip, [r2, #12]
800087b0:	e5933000 	ldr	r3, [r3]
800087b4:	e2833a05 	add	r3, r3, #20480	; 0x5000
800087b8:	e583103c 	str	r1, [r3, #60]	; 0x3c
800087bc:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
800087c0:	000179d0 	ldrdeq	r7, [r1], -r0
800087c4:	00000418 	andeq	r0, r0, r8, lsl r4
800087c8:	00013958 	andeq	r3, r1, r8, asr r9
800087cc:	00000010 	andeq	r0, r0, r0, lsl r0
800087d0:	00000577 	andeq	r0, r0, r7, ror r5
800087d4:	00000469 	andeq	r0, r0, r9, ror #8
800087d8:	000005c2 	andeq	r0, r0, r2, asr #11
800087dc:	45670000 	strbmi	r0, [r7, #-0]!
800087e0:	00000543 	andeq	r0, r0, r3, asr #10

800087e4 <sd_dev_handle>:
800087e4:	e59f2168 	ldr	r2, [pc, #360]	; 80008954 <sd_dev_handle+0x170>
800087e8:	e59f3168 	ldr	r3, [pc, #360]	; 80008958 <sd_dev_handle+0x174>
800087ec:	e08f2002 	add	r2, pc, r2
800087f0:	e1a0c00d 	mov	ip, sp
800087f4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
800087f8:	e7924003 	ldr	r4, [r2, r3]
800087fc:	e24cb004 	sub	fp, ip, #4
80008800:	e5943000 	ldr	r3, [r4]
80008804:	e59010ac 	ldr	r1, [r0, #172]	; 0xac
80008808:	e2833a05 	add	r3, r3, #20480	; 0x5000
8000880c:	e5932034 	ldr	r2, [r3, #52]	; 0x34
80008810:	e3120802 	tst	r2, #131072	; 0x20000
80008814:	0a00001b 	beq	80008888 <sd_dev_handle+0xa4>
80008818:	e312002a 	tst	r2, #42	; 0x2a
8000881c:	e591e400 	ldr	lr, [r1, #1024]	; 0x400
80008820:	e5912408 	ldr	r2, [r1, #1032]	; 0x408
80008824:	1a000012 	bne	80008874 <sd_dev_handle+0x90>
80008828:	e3520000 	cmp	r2, #0
8000882c:	0a00003c 	beq	80008924 <sd_dev_handle+0x140>
80008830:	e24e2004 	sub	r2, lr, #4
80008834:	e28ec03c 	add	ip, lr, #60	; 0x3c
80008838:	ea000001 	b	80008844 <sd_dev_handle+0x60>
8000883c:	e5943000 	ldr	r3, [r4]
80008840:	e2833a05 	add	r3, r3, #20480	; 0x5000
80008844:	e5933080 	ldr	r3, [r3, #128]	; 0x80
80008848:	e5a23004 	str	r3, [r2, #4]!
8000884c:	e152000c 	cmp	r2, ip
80008850:	1afffff9 	bne	8000883c <sd_dev_handle+0x58>
80008854:	e5912408 	ldr	r2, [r1, #1032]	; 0x408
80008858:	e5943000 	ldr	r3, [r4]
8000885c:	e2422040 	sub	r2, r2, #64	; 0x40
80008860:	e2833a05 	add	r3, r3, #20480	; 0x5000
80008864:	e28ee040 	add	lr, lr, #64	; 0x40
80008868:	e5812408 	str	r2, [r1, #1032]	; 0x408
8000886c:	e593c034 	ldr	ip, [r3, #52]	; 0x34
80008870:	e581e400 	str	lr, [r1, #1024]	; 0x400
80008874:	e3520000 	cmp	r2, #0
80008878:	0a000029 	beq	80008924 <sd_dev_handle+0x140>
8000887c:	e3e02000 	mvn	r2, #0
80008880:	e5832038 	str	r2, [r3, #56]	; 0x38
80008884:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
80008888:	e3120701 	tst	r2, #262144	; 0x40000
8000888c:	0afffffa 	beq	8000887c <sd_dev_handle+0x98>
80008890:	e312000a 	tst	r2, #10
80008894:	e5915404 	ldr	r5, [r1, #1028]	; 0x404
80008898:	e591240c 	ldr	r2, [r1, #1036]	; 0x40c
8000889c:	1a000012 	bne	800088ec <sd_dev_handle+0x108>
800088a0:	e3520000 	cmp	r2, #0
800088a4:	0a000012 	beq	800088f4 <sd_dev_handle+0x110>
800088a8:	e2452004 	sub	r2, r5, #4
800088ac:	e285e03c 	add	lr, r5, #60	; 0x3c
800088b0:	ea000001 	b	800088bc <sd_dev_handle+0xd8>
800088b4:	e5943000 	ldr	r3, [r4]
800088b8:	e2833a05 	add	r3, r3, #20480	; 0x5000
800088bc:	e5b2c004 	ldr	ip, [r2, #4]!
800088c0:	e152000e 	cmp	r2, lr
800088c4:	e583c080 	str	ip, [r3, #128]	; 0x80
800088c8:	1afffff9 	bne	800088b4 <sd_dev_handle+0xd0>
800088cc:	e591240c 	ldr	r2, [r1, #1036]	; 0x40c
800088d0:	e5943000 	ldr	r3, [r4]
800088d4:	e2422040 	sub	r2, r2, #64	; 0x40
800088d8:	e2833a05 	add	r3, r3, #20480	; 0x5000
800088dc:	e2855040 	add	r5, r5, #64	; 0x40
800088e0:	e581240c 	str	r2, [r1, #1036]	; 0x40c
800088e4:	e593c034 	ldr	ip, [r3, #52]	; 0x34
800088e8:	e5815404 	str	r5, [r1, #1028]	; 0x404
800088ec:	e3520000 	cmp	r2, #0
800088f0:	1affffe1 	bne	8000887c <sd_dev_handle+0x98>
800088f4:	e3a02000 	mov	r2, #0
800088f8:	e5832008 	str	r2, [r3, #8]
800088fc:	e5943000 	ldr	r3, [r4]
80008900:	e59fc054 	ldr	ip, [pc, #84]	; 8000895c <sd_dev_handle+0x178>
80008904:	e2833a05 	add	r3, r3, #20480	; 0x5000
80008908:	e3a02001 	mov	r2, #1
8000890c:	e583c00c 	str	ip, [r3, #12]
80008910:	e5812414 	str	r2, [r1, #1044]	; 0x414
80008914:	eb001e4e 	bl	80010254 <proc_wakeup>
80008918:	e5943000 	ldr	r3, [r4]
8000891c:	e2833a05 	add	r3, r3, #20480	; 0x5000
80008920:	eaffffd5 	b	8000887c <sd_dev_handle+0x98>
80008924:	e3a02000 	mov	r2, #0
80008928:	e5832008 	str	r2, [r3, #8]
8000892c:	e5943000 	ldr	r3, [r4]
80008930:	e59fc024 	ldr	ip, [pc, #36]	; 8000895c <sd_dev_handle+0x178>
80008934:	e2833a05 	add	r3, r3, #20480	; 0x5000
80008938:	e3a02001 	mov	r2, #1
8000893c:	e583c00c 	str	ip, [r3, #12]
80008940:	e5812410 	str	r2, [r1, #1040]	; 0x410
80008944:	eb001e42 	bl	80010254 <proc_wakeup>
80008948:	e5943000 	ldr	r3, [r4]
8000894c:	e2833a05 	add	r3, r3, #20480	; 0x5000
80008950:	eaffffc9 	b	8000887c <sd_dev_handle+0x98>
80008954:	00013810 	andeq	r3, r1, r0, lsl r8
80008958:	00000010 	andeq	r0, r0, r0, lsl r0
8000895c:	0000054c 	andeq	r0, r0, ip, asr #10

80008960 <sd_dev_read>:
80008960:	e59030ac 	ldr	r3, [r0, #172]	; 0xac
80008964:	e92d4010 	push	{r4, lr}
80008968:	e5932410 	ldr	r2, [r3, #1040]	; 0x410
8000896c:	e59fe088 	ldr	lr, [pc, #136]	; 800089fc <sd_dev_read+0x9c>
80008970:	e3520000 	cmp	r2, #0
80008974:	e08fe00e 	add	lr, pc, lr
80008978:	0a00001d 	beq	800089f4 <sd_dev_read+0x94>
8000897c:	e59040b0 	ldr	r4, [r0, #176]	; 0xb0
80008980:	e59fc078 	ldr	ip, [pc, #120]	; 80008a00 <sd_dev_read+0xa0>
80008984:	e3a02000 	mov	r2, #0
80008988:	e5834408 	str	r4, [r3, #1032]	; 0x408
8000898c:	e5832410 	str	r2, [r3, #1040]	; 0x410
80008990:	e5833400 	str	r3, [r3, #1024]	; 0x400
80008994:	e79e300c 	ldr	r3, [lr, ip]
80008998:	e59f4064 	ldr	r4, [pc, #100]	; 80008a04 <sd_dev_read+0xa4>
8000899c:	e593c000 	ldr	ip, [r3]
800089a0:	e59fe060 	ldr	lr, [pc, #96]	; 80008a08 <sd_dev_read+0xa8>
800089a4:	e28cca05 	add	ip, ip, #20480	; 0x5000
800089a8:	e58c4024 	str	r4, [ip, #36]	; 0x24
800089ac:	e593c000 	ldr	ip, [r3]
800089b0:	e59040b0 	ldr	r4, [r0, #176]	; 0xb0
800089b4:	e28cca05 	add	ip, ip, #20480	; 0x5000
800089b8:	e58c4028 	str	r4, [ip, #40]	; 0x28
800089bc:	e590c0b0 	ldr	ip, [r0, #176]	; 0xb0
800089c0:	e5930000 	ldr	r0, [r3]
800089c4:	e001019c 	mul	r1, ip, r1
800089c8:	e2800a05 	add	r0, r0, #20480	; 0x5000
800089cc:	e5801008 	str	r1, [r0, #8]
800089d0:	e5931000 	ldr	r1, [r3]
800089d4:	e3a0c093 	mov	ip, #147	; 0x93
800089d8:	e2811a05 	add	r1, r1, #20480	; 0x5000
800089dc:	e581e00c 	str	lr, [r1, #12]
800089e0:	e5933000 	ldr	r3, [r3]
800089e4:	e1a00002 	mov	r0, r2
800089e8:	e2833a05 	add	r3, r3, #20480	; 0x5000
800089ec:	e583c02c 	str	ip, [r3, #44]	; 0x2c
800089f0:	e8bd8010 	pop	{r4, pc}
800089f4:	e3e00000 	mvn	r0, #0
800089f8:	e8bd8010 	pop	{r4, pc}
800089fc:	00013688 	andeq	r3, r1, r8, lsl #13
80008a00:	00000010 	andeq	r0, r0, r0, lsl r0
80008a04:	ffff0000 			; <UNDEFINED> instruction: 0xffff0000
80008a08:	00000552 	andeq	r0, r0, r2, asr r5

80008a0c <sd_dev_read_done>:
80008a0c:	e1a0c00d 	mov	ip, sp
80008a10:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
80008a14:	e1a04000 	mov	r4, r0
80008a18:	e24cb004 	sub	fp, ip, #4
80008a1c:	e1a05001 	mov	r5, r1
80008a20:	ebffff6f 	bl	800087e4 <sd_dev_handle>
80008a24:	e59410ac 	ldr	r1, [r4, #172]	; 0xac
80008a28:	e5912410 	ldr	r2, [r1, #1040]	; 0x410
80008a2c:	e3520000 	cmp	r2, #0
80008a30:	0a000004 	beq	80008a48 <sd_dev_read_done+0x3c>
80008a34:	e1a00005 	mov	r0, r5
80008a38:	e59420b0 	ldr	r2, [r4, #176]	; 0xb0
80008a3c:	eb00086f 	bl	8000ac00 <memcpy>
80008a40:	e3a00000 	mov	r0, #0
80008a44:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
80008a48:	e3e00000 	mvn	r0, #0
80008a4c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

80008a50 <sd_dev_write>:
80008a50:	e1a0c00d 	mov	ip, sp
80008a54:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
80008a58:	e59050ac 	ldr	r5, [r0, #172]	; 0xac
80008a5c:	e59f70a8 	ldr	r7, [pc, #168]	; 80008b0c <sd_dev_write+0xbc>
80008a60:	e5953414 	ldr	r3, [r5, #1044]	; 0x414
80008a64:	e24cb004 	sub	fp, ip, #4
80008a68:	e3530000 	cmp	r3, #0
80008a6c:	e1a04000 	mov	r4, r0
80008a70:	e1a06001 	mov	r6, r1
80008a74:	e08f7007 	add	r7, pc, r7
80008a78:	0a000021 	beq	80008b04 <sd_dev_write+0xb4>
80008a7c:	e2858c02 	add	r8, r5, #512	; 0x200
80008a80:	e1a01002 	mov	r1, r2
80008a84:	e1a00008 	mov	r0, r8
80008a88:	e59420b0 	ldr	r2, [r4, #176]	; 0xb0
80008a8c:	eb00085b 	bl	8000ac00 <memcpy>
80008a90:	e59420b0 	ldr	r2, [r4, #176]	; 0xb0
80008a94:	e59f3074 	ldr	r3, [pc, #116]	; 80008b10 <sd_dev_write+0xc0>
80008a98:	e3a00000 	mov	r0, #0
80008a9c:	e585240c 	str	r2, [r5, #1036]	; 0x40c
80008aa0:	e5850414 	str	r0, [r5, #1044]	; 0x414
80008aa4:	e5858404 	str	r8, [r5, #1028]	; 0x404
80008aa8:	e7973003 	ldr	r3, [r7, r3]
80008aac:	e59f1060 	ldr	r1, [pc, #96]	; 80008b14 <sd_dev_write+0xc4>
80008ab0:	e5932000 	ldr	r2, [r3]
80008ab4:	e59fc05c 	ldr	ip, [pc, #92]	; 80008b18 <sd_dev_write+0xc8>
80008ab8:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008abc:	e5821024 	str	r1, [r2, #36]	; 0x24
80008ac0:	e5932000 	ldr	r2, [r3]
80008ac4:	e59410b0 	ldr	r1, [r4, #176]	; 0xb0
80008ac8:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008acc:	e5821028 	str	r1, [r2, #40]	; 0x28
80008ad0:	e59410b0 	ldr	r1, [r4, #176]	; 0xb0
80008ad4:	e5932000 	ldr	r2, [r3]
80008ad8:	e0010196 	mul	r1, r6, r1
80008adc:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008ae0:	e5821008 	str	r1, [r2, #8]
80008ae4:	e5932000 	ldr	r2, [r3]
80008ae8:	e3a01091 	mov	r1, #145	; 0x91
80008aec:	e2822a05 	add	r2, r2, #20480	; 0x5000
80008af0:	e582c00c 	str	ip, [r2, #12]
80008af4:	e5933000 	ldr	r3, [r3]
80008af8:	e2833a05 	add	r3, r3, #20480	; 0x5000
80008afc:	e583102c 	str	r1, [r3, #44]	; 0x2c
80008b00:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
80008b04:	e3e00000 	mvn	r0, #0
80008b08:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
80008b0c:	00013588 	andeq	r3, r1, r8, lsl #11
80008b10:	00000010 	andeq	r0, r0, r0, lsl r0
80008b14:	ffff0000 			; <UNDEFINED> instruction: 0xffff0000
80008b18:	00000559 	andeq	r0, r0, r9, asr r5

80008b1c <sd_dev_write_done>:
80008b1c:	e1a0c00d 	mov	ip, sp
80008b20:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80008b24:	e1a04000 	mov	r4, r0
80008b28:	e24cb004 	sub	fp, ip, #4
80008b2c:	ebffff2c 	bl	800087e4 <sd_dev_handle>
80008b30:	e59430ac 	ldr	r3, [r4, #172]	; 0xac
80008b34:	e5930414 	ldr	r0, [r3, #1044]	; 0x414
80008b38:	e3500000 	cmp	r0, #0
80008b3c:	13a00000 	movne	r0, #0
80008b40:	03e00000 	mvneq	r0, #0
80008b44:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}

80008b48 <timer_addr_by_id>:
80008b48:	e59f3070 	ldr	r3, [pc, #112]	; 80008bc0 <timer_addr_by_id+0x78>
80008b4c:	e59f2070 	ldr	r2, [pc, #112]	; 80008bc4 <timer_addr_by_id+0x7c>
80008b50:	e08f3003 	add	r3, pc, r3
80008b54:	e3500003 	cmp	r0, #3
80008b58:	908ff100 	addls	pc, pc, r0, lsl #2
80008b5c:	ea000003 	b	80008b70 <timer_addr_by_id+0x28>
80008b60:	ea000002 	b	80008b70 <timer_addr_by_id+0x28>
80008b64:	ea00000b 	b	80008b98 <timer_addr_by_id+0x50>
80008b68:	ea000005 	b	80008b84 <timer_addr_by_id+0x3c>
80008b6c:	ea00000e 	b	80008bac <timer_addr_by_id+0x64>
80008b70:	e7933002 	ldr	r3, [r3, r2]
80008b74:	e5930000 	ldr	r0, [r3]
80008b78:	e280081e 	add	r0, r0, #1966080	; 0x1e0000
80008b7c:	e2800a02 	add	r0, r0, #8192	; 0x2000
80008b80:	e12fff1e 	bx	lr
80008b84:	e7933002 	ldr	r3, [r3, r2]
80008b88:	e5930000 	ldr	r0, [r3]
80008b8c:	e280081e 	add	r0, r0, #1966080	; 0x1e0000
80008b90:	e2800a03 	add	r0, r0, #12288	; 0x3000
80008b94:	e12fff1e 	bx	lr
80008b98:	e7933002 	ldr	r3, [r3, r2]
80008b9c:	e59f0024 	ldr	r0, [pc, #36]	; 80008bc8 <timer_addr_by_id+0x80>
80008ba0:	e5933000 	ldr	r3, [r3]
80008ba4:	e0830000 	add	r0, r3, r0
80008ba8:	e12fff1e 	bx	lr
80008bac:	e7933002 	ldr	r3, [r3, r2]
80008bb0:	e59f0014 	ldr	r0, [pc, #20]	; 80008bcc <timer_addr_by_id+0x84>
80008bb4:	e5933000 	ldr	r3, [r3]
80008bb8:	e0830000 	add	r0, r3, r0
80008bbc:	e12fff1e 	bx	lr
80008bc0:	000134ac 	andeq	r3, r1, ip, lsr #9
80008bc4:	00000010 	andeq	r0, r0, r0, lsl r0
80008bc8:	001e2020 	andseq	r2, lr, r0, lsr #32
80008bcc:	001e3020 	andseq	r3, lr, r0, lsr #32

80008bd0 <timer_set_interval>:
80008bd0:	e1a0c00d 	mov	ip, sp
80008bd4:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80008bd8:	e24cb004 	sub	fp, ip, #4
80008bdc:	e1a04001 	mov	r4, r1
80008be0:	ebffffd8 	bl	80008b48 <timer_addr_by_id>
80008be4:	e59f3014 	ldr	r3, [pc, #20]	; 80008c00 <timer_set_interval+0x30>
80008be8:	e3e02019 	mvn	r2, #25
80008bec:	e081c493 	umull	ip, r1, r3, r4
80008bf0:	e1a012a1 	lsr	r1, r1, #5
80008bf4:	e5801000 	str	r1, [r0]
80008bf8:	e5c02008 	strb	r2, [r0, #8]
80008bfc:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
80008c00:	51eb851f 	mvnpl	r8, pc, lsl r5

80008c04 <timer_clear_interrupt>:
80008c04:	e1a0c00d 	mov	ip, sp
80008c08:	e92dd800 	push	{fp, ip, lr, pc}
80008c0c:	e24cb004 	sub	fp, ip, #4
80008c10:	ebffffcc 	bl	80008b48 <timer_addr_by_id>
80008c14:	e3e03000 	mvn	r3, #0
80008c18:	e580300c 	str	r3, [r0, #12]
80008c1c:	e89da800 	ldm	sp, {fp, sp, pc}

80008c20 <timer_read_sys_usec>:
80008c20:	e59f3014 	ldr	r3, [pc, #20]	; 80008c3c <timer_read_sys_usec+0x1c>
80008c24:	e3a01000 	mov	r1, #0
80008c28:	e08f3003 	add	r3, pc, r3
80008c2c:	e5930000 	ldr	r0, [r3]
80008c30:	e28000c8 	add	r0, r0, #200	; 0xc8
80008c34:	e5830000 	str	r0, [r3]
80008c38:	e12fff1e 	bx	lr
80008c3c:	00017840 	andeq	r7, r1, r0, asr #16

80008c40 <graph_insect>:
80008c40:	e3500000 	cmp	r0, #0
80008c44:	e92d4070 	push	{r4, r5, r6, lr}
80008c48:	0a000025 	beq	80008ce4 <graph_insect+0xa4>
80008c4c:	e5912000 	ldr	r2, [r1]
80008c50:	e5903004 	ldr	r3, [r0, #4]
80008c54:	e1520003 	cmp	r2, r3
80008c58:	aa000021 	bge	80008ce4 <graph_insect+0xa4>
80008c5c:	e590e008 	ldr	lr, [r0, #8]
80008c60:	e5910004 	ldr	r0, [r1, #4]
80008c64:	e150000e 	cmp	r0, lr
80008c68:	aa00001d 	bge	80008ce4 <graph_insect+0xa4>
80008c6c:	e591c008 	ldr	ip, [r1, #8]
80008c70:	e591400c 	ldr	r4, [r1, #12]
80008c74:	e082600c 	add	r6, r2, ip
80008c78:	e1530006 	cmp	r3, r6
80008c7c:	d08c3003 	addle	r3, ip, r3
80008c80:	d066c003 	rsble	ip, r6, r3
80008c84:	e0805004 	add	r5, r0, r4
80008c88:	d581c008 	strle	ip, [r1, #8]
80008c8c:	e15e0005 	cmp	lr, r5
80008c90:	d084e00e 	addle	lr, r4, lr
80008c94:	d065e00e 	rsble	lr, r5, lr
80008c98:	d581e00c 	strle	lr, [r1, #12]
80008c9c:	e3520000 	cmp	r2, #0
80008ca0:	b08cc002 	addlt	ip, ip, r2
80008ca4:	b3a03000 	movlt	r3, #0
80008ca8:	b5813000 	strlt	r3, [r1]
80008cac:	b581c008 	strlt	ip, [r1, #8]
80008cb0:	e3500000 	cmp	r0, #0
80008cb4:	b591300c 	ldrlt	r3, [r1, #12]
80008cb8:	b3a02000 	movlt	r2, #0
80008cbc:	b0830000 	addlt	r0, r3, r0
80008cc0:	b581000c 	strlt	r0, [r1, #12]
80008cc4:	b5812004 	strlt	r2, [r1, #4]
80008cc8:	e35c0000 	cmp	ip, #0
80008ccc:	da000004 	ble	80008ce4 <graph_insect+0xa4>
80008cd0:	e591000c 	ldr	r0, [r1, #12]
80008cd4:	e3500000 	cmp	r0, #0
80008cd8:	d3a00000 	movle	r0, #0
80008cdc:	c3a00001 	movgt	r0, #1
80008ce0:	e8bd8070 	pop	{r4, r5, r6, pc}
80008ce4:	e3a00000 	mov	r0, #0
80008ce8:	e8bd8070 	pop	{r4, r5, r6, pc}

80008cec <insect>:
80008cec:	e1a0c00d 	mov	ip, sp
80008cf0:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
80008cf4:	e593e000 	ldr	lr, [r3]
80008cf8:	e1a04001 	mov	r4, r1
80008cfc:	e1a05003 	mov	r5, r3
80008d00:	e5911000 	ldr	r1, [r1]
80008d04:	e5943004 	ldr	r3, [r4, #4]
80008d08:	e24cb004 	sub	fp, ip, #4
80008d0c:	e595c004 	ldr	ip, [r5, #4]
80008d10:	e151000e 	cmp	r1, lr
80008d14:	b1a0e001 	movlt	lr, r1
80008d18:	e153000c 	cmp	r3, ip
80008d1c:	b1a0c003 	movlt	ip, r3
80008d20:	e35e0000 	cmp	lr, #0
80008d24:	e1a06002 	mov	r6, r2
80008d28:	ba00002d 	blt	80008de4 <insect+0xf8>
80008d2c:	e35c0000 	cmp	ip, #0
80008d30:	ba00001f 	blt	80008db4 <insect+0xc8>
80008d34:	e1a01004 	mov	r1, r4
80008d38:	ebffffc0 	bl	80008c40 <graph_insect>
80008d3c:	e3500000 	cmp	r0, #0
80008d40:	1a000001 	bne	80008d4c <insect+0x60>
80008d44:	e3a00000 	mov	r0, #0
80008d48:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80008d4c:	e1a00006 	mov	r0, r6
80008d50:	e1a01005 	mov	r1, r5
80008d54:	ebffffb9 	bl	80008c40 <graph_insect>
80008d58:	e3500000 	cmp	r0, #0
80008d5c:	0afffff8 	beq	80008d44 <insect+0x58>
80008d60:	e5943008 	ldr	r3, [r4, #8]
80008d64:	e3530000 	cmp	r3, #0
80008d68:	dafffff5 	ble	80008d44 <insect+0x58>
80008d6c:	e594200c 	ldr	r2, [r4, #12]
80008d70:	e3520000 	cmp	r2, #0
80008d74:	dafffff2 	ble	80008d44 <insect+0x58>
80008d78:	e5951008 	ldr	r1, [r5, #8]
80008d7c:	e3510000 	cmp	r1, #0
80008d80:	daffffef 	ble	80008d44 <insect+0x58>
80008d84:	e595000c 	ldr	r0, [r5, #12]
80008d88:	e3500000 	cmp	r0, #0
80008d8c:	daffffec 	ble	80008d44 <insect+0x58>
80008d90:	e1530001 	cmp	r3, r1
80008d94:	c5841008 	strgt	r1, [r4, #8]
80008d98:	d5853008 	strle	r3, [r5, #8]
80008d9c:	e1520000 	cmp	r2, r0
80008da0:	c584000c 	strgt	r0, [r4, #12]
80008da4:	d585200c 	strle	r2, [r5, #12]
80008da8:	c3a00001 	movgt	r0, #1
80008dac:	d3a00001 	movle	r0, #1
80008db0:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80008db4:	e06c3003 	rsb	r3, ip, r3
80008db8:	e5843004 	str	r3, [r4, #4]
80008dbc:	e594300c 	ldr	r3, [r4, #12]
80008dc0:	e5952004 	ldr	r2, [r5, #4]
80008dc4:	e083300c 	add	r3, r3, ip
80008dc8:	e06c2002 	rsb	r2, ip, r2
80008dcc:	e5852004 	str	r2, [r5, #4]
80008dd0:	e584300c 	str	r3, [r4, #12]
80008dd4:	e595300c 	ldr	r3, [r5, #12]
80008dd8:	e083c00c 	add	ip, r3, ip
80008ddc:	e585c00c 	str	ip, [r5, #12]
80008de0:	eaffffd3 	b	80008d34 <insect+0x48>
80008de4:	e06e1001 	rsb	r1, lr, r1
80008de8:	e5942008 	ldr	r2, [r4, #8]
80008dec:	e5841000 	str	r1, [r4]
80008df0:	e5951000 	ldr	r1, [r5]
80008df4:	e082200e 	add	r2, r2, lr
80008df8:	e06e1001 	rsb	r1, lr, r1
80008dfc:	e5851000 	str	r1, [r5]
80008e00:	e5842008 	str	r2, [r4, #8]
80008e04:	e5952008 	ldr	r2, [r5, #8]
80008e08:	e082e00e 	add	lr, r2, lr
80008e0c:	e585e008 	str	lr, [r5, #8]
80008e10:	eaffffc5 	b	80008d2c <insect+0x40>

80008e14 <argb>:
80008e14:	e1812402 	orr	r2, r1, r2, lsl #8
80008e18:	e1823803 	orr	r3, r2, r3, lsl #16
80008e1c:	e1830c00 	orr	r0, r3, r0, lsl #24
80008e20:	e12fff1e 	bx	lr

80008e24 <has_alpha>:
80008e24:	e1a00c20 	lsr	r0, r0, #24
80008e28:	e25000ff 	subs	r0, r0, #255	; 0xff
80008e2c:	13a00001 	movne	r0, #1
80008e30:	e12fff1e 	bx	lr

80008e34 <argb_int>:
80008e34:	e1a02400 	lsl	r2, r0, #8
80008e38:	e20034ff 	and	r3, r0, #-16777216	; 0xff000000
80008e3c:	e1833c22 	orr	r3, r3, r2, lsr #24
80008e40:	e20020ff 	and	r2, r0, #255	; 0xff
80008e44:	e1833802 	orr	r3, r3, r2, lsl #16
80008e48:	e2000cff 	and	r0, r0, #65280	; 0xff00
80008e4c:	e1830000 	orr	r0, r3, r0
80008e50:	e12fff1e 	bx	lr

80008e54 <graph_new>:
80008e54:	e1a0c00d 	mov	ip, sp
80008e58:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
80008e5c:	e1a05000 	mov	r5, r0
80008e60:	e24cb004 	sub	fp, ip, #4
80008e64:	e3a00010 	mov	r0, #16
80008e68:	e1a07001 	mov	r7, r1
80008e6c:	e1a06002 	mov	r6, r2
80008e70:	eb0012bc 	bl	8000d968 <kmalloc>
80008e74:	e3550000 	cmp	r5, #0
80008e78:	e1a04000 	mov	r4, r0
80008e7c:	e5807004 	str	r7, [r0, #4]
80008e80:	e5806008 	str	r6, [r0, #8]
80008e84:	0a000004 	beq	80008e9c <graph_new+0x48>
80008e88:	e3a03000 	mov	r3, #0
80008e8c:	e5805000 	str	r5, [r0]
80008e90:	e580300c 	str	r3, [r0, #12]
80008e94:	e1a00004 	mov	r0, r4
80008e98:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
80008e9c:	e1a00106 	lsl	r0, r6, #2
80008ea0:	e0000097 	mul	r0, r7, r0
80008ea4:	eb0012af 	bl	8000d968 <kmalloc>
80008ea8:	e3a03001 	mov	r3, #1
80008eac:	e584300c 	str	r3, [r4, #12]
80008eb0:	e5840000 	str	r0, [r4]
80008eb4:	e1a00004 	mov	r0, r4
80008eb8:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}

80008ebc <graph_free>:
80008ebc:	e1a0c00d 	mov	ip, sp
80008ec0:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80008ec4:	e2504000 	subs	r4, r0, #0
80008ec8:	e24cb004 	sub	fp, ip, #4
80008ecc:	089da818 	ldmeq	sp, {r3, r4, fp, sp, pc}
80008ed0:	e5940000 	ldr	r0, [r4]
80008ed4:	e3500000 	cmp	r0, #0
80008ed8:	0a000002 	beq	80008ee8 <graph_free+0x2c>
80008edc:	e594300c 	ldr	r3, [r4, #12]
80008ee0:	e3530001 	cmp	r3, #1
80008ee4:	0a000003 	beq	80008ef8 <graph_free+0x3c>
80008ee8:	e1a00004 	mov	r0, r4
80008eec:	e24bd014 	sub	sp, fp, #20
80008ef0:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
80008ef4:	ea0012ae 	b	8000d9b4 <kfree>
80008ef8:	eb0012ad 	bl	8000d9b4 <kfree>
80008efc:	e1a00004 	mov	r0, r4
80008f00:	e24bd014 	sub	sp, fp, #20
80008f04:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
80008f08:	ea0012a9 	b	8000d9b4 <kfree>

80008f0c <pixel>:
80008f0c:	e8901001 	ldm	r0, {r0, ip}
80008f10:	e022129c 	mla	r2, ip, r2, r1
80008f14:	e7803102 	str	r3, [r0, r2, lsl #2]
80008f18:	e12fff1e 	bx	lr

80008f1c <pixel_safe>:
80008f1c:	e16fcf10 	clz	ip, r0
80008f20:	e1a0c2ac 	lsr	ip, ip, #5
80008f24:	e19ccfa1 	orrs	ip, ip, r1, lsr #31
80008f28:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
80008f2c:	149df004 	popne	{pc}		; (ldrne pc, [sp], #4)
80008f30:	e590c004 	ldr	ip, [r0, #4]
80008f34:	e151000c 	cmp	r1, ip
80008f38:	33a0e000 	movcc	lr, #0
80008f3c:	23a0e001 	movcs	lr, #1
80008f40:	e19eefa2 	orrs	lr, lr, r2, lsr #31
80008f44:	149df004 	popne	{pc}		; (ldrne pc, [sp], #4)
80008f48:	e590e008 	ldr	lr, [r0, #8]
80008f4c:	e152000e 	cmp	r2, lr
80008f50:	3021129c 	mlacc	r1, ip, r2, r1
80008f54:	35902000 	ldrcc	r2, [r0]
80008f58:	37823101 	strcc	r3, [r2, r1, lsl #2]
80008f5c:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)

80008f60 <clear>:
80008f60:	e1a0c00d 	mov	ip, sp
80008f64:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
80008f68:	e2507000 	subs	r7, r0, #0
80008f6c:	e24cb004 	sub	fp, ip, #4
80008f70:	089da9f8 	ldmeq	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
80008f74:	e5976004 	ldr	r6, [r7, #4]
80008f78:	e3560000 	cmp	r6, #0
80008f7c:	089da9f8 	ldmeq	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
80008f80:	e5978000 	ldr	r8, [r7]
80008f84:	e1a06106 	lsl	r6, r6, #2
80008f88:	e2482004 	sub	r2, r8, #4
80008f8c:	e3a03000 	mov	r3, #0
80008f90:	e5a21004 	str	r1, [r2, #4]!
80008f94:	e5970004 	ldr	r0, [r7, #4]
80008f98:	e2833001 	add	r3, r3, #1
80008f9c:	e1500003 	cmp	r0, r3
80008fa0:	8afffffa 	bhi	80008f90 <clear+0x30>
80008fa4:	e5973008 	ldr	r3, [r7, #8]
80008fa8:	e3530001 	cmp	r3, #1
80008fac:	989da9f8 	ldmls	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
80008fb0:	e0885006 	add	r5, r8, r6
80008fb4:	e3a04001 	mov	r4, #1
80008fb8:	e1a00005 	mov	r0, r5
80008fbc:	e1a01008 	mov	r1, r8
80008fc0:	e1a02006 	mov	r2, r6
80008fc4:	eb00070d 	bl	8000ac00 <memcpy>
80008fc8:	e5973008 	ldr	r3, [r7, #8]
80008fcc:	e2844001 	add	r4, r4, #1
80008fd0:	e1530004 	cmp	r3, r4
80008fd4:	e0855006 	add	r5, r5, r6
80008fd8:	8afffff6 	bhi	80008fb8 <clear+0x58>
80008fdc:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}

80008fe0 <reverse>:
80008fe0:	e3500000 	cmp	r0, #0
80008fe4:	e92d4010 	push	{r4, lr}
80008fe8:	08bd8010 	popeq	{r4, pc}
80008fec:	e990000c 	ldmib	r0, {r2, r3}
80008ff0:	e0110392 	muls	r1, r2, r3
80008ff4:	08bd8010 	popeq	{r4, pc}
80008ff8:	e590c000 	ldr	ip, [r0]
80008ffc:	e3a0e000 	mov	lr, #0
80009000:	e24cc004 	sub	ip, ip, #4
80009004:	e5bc3004 	ldr	r3, [ip, #4]!
80009008:	e28ee001 	add	lr, lr, #1
8000900c:	e1e01003 	mvn	r1, r3
80009010:	e20140ff 	and	r4, r1, #255	; 0xff
80009014:	e1e02003 	mvn	r2, r3
80009018:	e20314ff 	and	r1, r3, #-16777216	; 0xff000000
8000901c:	e1841001 	orr	r1, r4, r1
80009020:	e20228ff 	and	r2, r2, #16711680	; 0xff0000
80009024:	e1e03003 	mvn	r3, r3
80009028:	e1812002 	orr	r2, r1, r2
8000902c:	e2033cff 	and	r3, r3, #65280	; 0xff00
80009030:	e1823003 	orr	r3, r2, r3
80009034:	e58c3000 	str	r3, [ip]
80009038:	e5902008 	ldr	r2, [r0, #8]
8000903c:	e5903004 	ldr	r3, [r0, #4]
80009040:	e0030392 	mul	r3, r2, r3
80009044:	e153000e 	cmp	r3, lr
80009048:	8affffed 	bhi	80009004 <reverse+0x24>
8000904c:	e8bd8010 	pop	{r4, pc}

80009050 <line>:
80009050:	e92d47f0 	push	{r4, r5, r6, r7, r8, r9, sl, lr}
80009054:	e2507000 	subs	r7, r0, #0
80009058:	e24dd048 	sub	sp, sp, #72	; 0x48
8000905c:	e1a09001 	mov	r9, r1
80009060:	e1a06002 	mov	r6, r2
80009064:	0a0000be 	beq	80009364 <line+0x314>
80009068:	e59d2068 	ldr	r2, [sp, #104]	; 0x68
8000906c:	e0522006 	subs	r2, r2, r6
80009070:	42622000 	rsbmi	r2, r2, #0
80009074:	43e00000 	mvnmi	r0, #0
80009078:	53a00001 	movpl	r0, #1
8000907c:	e0533001 	subs	r3, r3, r1
80009080:	42633000 	rsbmi	r3, r3, #0
80009084:	43e08000 	mvnmi	r8, #0
80009088:	53a08001 	movpl	r8, #1
8000908c:	e1530002 	cmp	r3, r2
80009090:	a3a01000 	movge	r1, #0
80009094:	a58d1010 	strge	r1, [sp, #16]
80009098:	ba0000b3 	blt	8000936c <line+0x31c>
8000909c:	e59dc06c 	ldr	ip, [sp, #108]	; 0x6c
800090a0:	e1a0a082 	lsl	sl, r2, #1
800090a4:	e1a0cc2c 	lsr	ip, ip, #24
800090a8:	e063200a 	rsb	r2, r3, sl
800090ac:	e35c00ff 	cmp	ip, #255	; 0xff
800090b0:	e58dc00c 	str	ip, [sp, #12]
800090b4:	e58d2000 	str	r2, [sp]
800090b8:	0a0000b1 	beq	80009384 <line+0x334>
800090bc:	e59d106c 	ldr	r1, [sp, #108]	; 0x6c
800090c0:	e59d200c 	ldr	r2, [sp, #12]
800090c4:	e1a0e821 	lsr	lr, r1, #16
800090c8:	e1a04002 	mov	r4, r2
800090cc:	e20ee0ff 	and	lr, lr, #255	; 0xff
800090d0:	e004049e 	mul	r4, lr, r4
800090d4:	e1a0c421 	lsr	ip, r1, #8
800090d8:	e1a0e004 	mov	lr, r4
800090dc:	e20cc0ff 	and	ip, ip, #255	; 0xff
800090e0:	e1a04002 	mov	r4, r2
800090e4:	e004049c 	mul	r4, ip, r4
800090e8:	e20150ff 	and	r5, r1, #255	; 0xff
800090ec:	e1a0c004 	mov	ip, r4
800090f0:	e1a04002 	mov	r4, r2
800090f4:	e0040495 	mul	r4, r5, r4
800090f8:	e59f5328 	ldr	r5, [pc, #808]	; 80009428 <line+0x3d8>
800090fc:	e0c5159e 	smull	r1, r5, lr, r5
80009100:	e1a01005 	mov	r1, r5
80009104:	e59f531c 	ldr	r5, [pc, #796]	; 80009428 <line+0x3d8>
80009108:	e081100e 	add	r1, r1, lr
8000910c:	e0c5259c 	smull	r2, r5, ip, r5
80009110:	e58d1004 	str	r1, [sp, #4]
80009114:	e58d5008 	str	r5, [sp, #8]
80009118:	e59d1008 	ldr	r1, [sp, #8]
8000911c:	e59f5304 	ldr	r5, [pc, #772]	; 80009428 <line+0x3d8>
80009120:	e081100c 	add	r1, r1, ip
80009124:	e0c52594 	smull	r2, r5, r4, r5
80009128:	e1a02001 	mov	r2, r1
8000912c:	e59d1004 	ldr	r1, [sp, #4]
80009130:	e1a0efce 	asr	lr, lr, #31
80009134:	e1a0cfcc 	asr	ip, ip, #31
80009138:	e06ee3c1 	rsb	lr, lr, r1, asr #7
8000913c:	e0855004 	add	r5, r5, r4
80009140:	e06cc3c2 	rsb	ip, ip, r2, asr #7
80009144:	e20e10ff 	and	r1, lr, #255	; 0xff
80009148:	e1a04fc4 	asr	r4, r4, #31
8000914c:	e06443c5 	rsb	r4, r4, r5, asr #7
80009150:	e58d103c 	str	r1, [sp, #60]	; 0x3c
80009154:	e20c10ff 	and	r1, ip, #255	; 0xff
80009158:	e58d1040 	str	r1, [sp, #64]	; 0x40
8000915c:	e20410ff 	and	r1, r4, #255	; 0xff
80009160:	e58d1044 	str	r1, [sp, #68]	; 0x44
80009164:	e59f12bc 	ldr	r1, [pc, #700]	; 80009428 <line+0x3d8>
80009168:	e0635f83 	rsb	r5, r3, r3, lsl #31
8000916c:	e58d1024 	str	r1, [sp, #36]	; 0x24
80009170:	e2831001 	add	r1, r3, #1
80009174:	e59d300c 	ldr	r3, [sp, #12]
80009178:	e58d1004 	str	r1, [sp, #4]
8000917c:	e59d1010 	ldr	r1, [sp, #16]
80009180:	e1a04085 	lsl	r4, r5, #1
80009184:	e26330ff 	rsb	r3, r3, #255	; 0xff
80009188:	e58da008 	str	sl, [sp, #8]
8000918c:	e3a0c000 	mov	ip, #0
80009190:	e1a0a007 	mov	sl, r7
80009194:	e59d2000 	ldr	r2, [sp]
80009198:	e58d3014 	str	r3, [sp, #20]
8000919c:	e58d4000 	str	r4, [sp]
800091a0:	e3590000 	cmp	r9, #0
800091a4:	ba00005c 	blt	8000931c <line+0x2cc>
800091a8:	e59a3004 	ldr	r3, [sl, #4]
800091ac:	e1590003 	cmp	r9, r3
800091b0:	33a0e000 	movcc	lr, #0
800091b4:	23a0e001 	movcs	lr, #1
800091b8:	e19eefa6 	orrs	lr, lr, r6, lsr #31
800091bc:	1a000056 	bne	8000931c <line+0x2cc>
800091c0:	e59ae008 	ldr	lr, [sl, #8]
800091c4:	e156000e 	cmp	r6, lr
800091c8:	2a000053 	bcs	8000931c <line+0x2cc>
800091cc:	e02e9693 	mla	lr, r3, r6, r9
800091d0:	e59a3000 	ldr	r3, [sl]
800091d4:	e59d4014 	ldr	r4, [sp, #20]
800091d8:	e793310e 	ldr	r3, [r3, lr, lsl #2]
800091dc:	e58de02c 	str	lr, [sp, #44]	; 0x2c
800091e0:	e1a05823 	lsr	r5, r3, #16
800091e4:	e20550ff 	and	r5, r5, #255	; 0xff
800091e8:	e1a0e004 	mov	lr, r4
800091ec:	e00e0e95 	mul	lr, r5, lr
800091f0:	e1a05c23 	lsr	r5, r3, #24
800091f4:	e58de018 	str	lr, [sp, #24]
800091f8:	e203e0ff 	and	lr, r3, #255	; 0xff
800091fc:	e004049e 	mul	r4, lr, r4
80009200:	e58d5028 	str	r5, [sp, #40]	; 0x28
80009204:	e26570ff 	rsb	r7, r5, #255	; 0xff
80009208:	e59d500c 	ldr	r5, [sp, #12]
8000920c:	e58d4010 	str	r4, [sp, #16]
80009210:	e0050597 	mul	r5, r7, r5
80009214:	e59d4024 	ldr	r4, [sp, #36]	; 0x24
80009218:	e59d7018 	ldr	r7, [sp, #24]
8000921c:	e1a03423 	lsr	r3, r3, #8
80009220:	e0c7e794 	smull	lr, r7, r4, r7
80009224:	e20330ff 	and	r3, r3, #255	; 0xff
80009228:	e58d301c 	str	r3, [sp, #28]
8000922c:	e59de014 	ldr	lr, [sp, #20]
80009230:	e1a03007 	mov	r3, r7
80009234:	e59d701c 	ldr	r7, [sp, #28]
80009238:	e58d5020 	str	r5, [sp, #32]
8000923c:	e007079e 	mul	r7, lr, r7
80009240:	e1a0e004 	mov	lr, r4
80009244:	e58d701c 	str	r7, [sp, #28]
80009248:	e0c47495 	smull	r7, r4, r5, r4
8000924c:	e59d5018 	ldr	r5, [sp, #24]
80009250:	e58d4030 	str	r4, [sp, #48]	; 0x30
80009254:	e1a0400e 	mov	r4, lr
80009258:	e59de010 	ldr	lr, [sp, #16]
8000925c:	e0833005 	add	r3, r3, r5
80009260:	e1a05fc5 	asr	r5, r5, #31
80009264:	e0ce7e94 	smull	r7, lr, r4, lr
80009268:	e06533c3 	rsb	r3, r5, r3, asr #7
8000926c:	e1a05004 	mov	r5, r4
80009270:	e59d401c 	ldr	r4, [sp, #28]
80009274:	e58de034 	str	lr, [sp, #52]	; 0x34
80009278:	e0c47495 	smull	r7, r4, r5, r4
8000927c:	e59de020 	ldr	lr, [sp, #32]
80009280:	e58d4038 	str	r4, [sp, #56]	; 0x38
80009284:	e59d4030 	ldr	r4, [sp, #48]	; 0x30
80009288:	e59d5044 	ldr	r5, [sp, #68]	; 0x44
8000928c:	e084400e 	add	r4, r4, lr
80009290:	e58d4018 	str	r4, [sp, #24]
80009294:	e0853003 	add	r3, r5, r3
80009298:	e59d4034 	ldr	r4, [sp, #52]	; 0x34
8000929c:	e1a05fce 	asr	r5, lr, #31
800092a0:	e59de010 	ldr	lr, [sp, #16]
800092a4:	e20330ff 	and	r3, r3, #255	; 0xff
800092a8:	e084e00e 	add	lr, r4, lr
800092ac:	e1a0700e 	mov	r7, lr
800092b0:	e59de010 	ldr	lr, [sp, #16]
800092b4:	e59d4018 	ldr	r4, [sp, #24]
800092b8:	e1a0efce 	asr	lr, lr, #31
800092bc:	e06553c4 	rsb	r5, r5, r4, asr #7
800092c0:	e06ee3c7 	rsb	lr, lr, r7, asr #7
800092c4:	e59d4038 	ldr	r4, [sp, #56]	; 0x38
800092c8:	e59d701c 	ldr	r7, [sp, #28]
800092cc:	e1a03803 	lsl	r3, r3, #16
800092d0:	e0844007 	add	r4, r4, r7
800092d4:	e58d4010 	str	r4, [sp, #16]
800092d8:	e59d4028 	ldr	r4, [sp, #40]	; 0x28
800092dc:	e1a07fc7 	asr	r7, r7, #31
800092e0:	e0845005 	add	r5, r4, r5
800092e4:	e59d403c 	ldr	r4, [sp, #60]	; 0x3c
800092e8:	e1833c05 	orr	r3, r3, r5, lsl #24
800092ec:	e084e00e 	add	lr, r4, lr
800092f0:	e59d4010 	ldr	r4, [sp, #16]
800092f4:	e20ee0ff 	and	lr, lr, #255	; 0xff
800092f8:	e06773c4 	rsb	r7, r7, r4, asr #7
800092fc:	e59d4040 	ldr	r4, [sp, #64]	; 0x40
80009300:	e183300e 	orr	r3, r3, lr
80009304:	e0847007 	add	r7, r4, r7
80009308:	e59de02c 	ldr	lr, [sp, #44]	; 0x2c
8000930c:	e59a4000 	ldr	r4, [sl]
80009310:	e20770ff 	and	r7, r7, #255	; 0xff
80009314:	e1833407 	orr	r3, r3, r7, lsl #8
80009318:	e784310e 	str	r3, [r4, lr, lsl #2]
8000931c:	e3520000 	cmp	r2, #0
80009320:	ba000006 	blt	80009340 <line+0x2f0>
80009324:	e59d4000 	ldr	r4, [sp]
80009328:	e3510001 	cmp	r1, #1
8000932c:	00899008 	addeq	r9, r9, r8
80009330:	10866000 	addne	r6, r6, r0
80009334:	e0922004 	adds	r2, r2, r4
80009338:	5afffffa 	bpl	80009328 <line+0x2d8>
8000933c:	e58d4000 	str	r4, [sp]
80009340:	e59d3004 	ldr	r3, [sp, #4]
80009344:	e3510001 	cmp	r1, #1
80009348:	e28cc001 	add	ip, ip, #1
8000934c:	00866000 	addeq	r6, r6, r0
80009350:	10899008 	addne	r9, r9, r8
80009354:	e15c0003 	cmp	ip, r3
80009358:	e59d3008 	ldr	r3, [sp, #8]
8000935c:	e0822003 	add	r2, r2, r3
80009360:	1affff8e 	bne	800091a0 <line+0x150>
80009364:	e28dd048 	add	sp, sp, #72	; 0x48
80009368:	e8bd87f0 	pop	{r4, r5, r6, r7, r8, r9, sl, pc}
8000936c:	e1a0c003 	mov	ip, r3
80009370:	e3a03001 	mov	r3, #1
80009374:	e58d3010 	str	r3, [sp, #16]
80009378:	e1a03002 	mov	r3, r2
8000937c:	e1a0200c 	mov	r2, ip
80009380:	eaffff45 	b	8000909c <line+0x4c>
80009384:	e3530000 	cmp	r3, #0
80009388:	bafffff5 	blt	80009364 <line+0x314>
8000938c:	e59d1010 	ldr	r1, [sp, #16]
80009390:	e063cf83 	rsb	ip, r3, r3, lsl #31
80009394:	e1a0c08c 	lsl	ip, ip, #1
80009398:	e2833001 	add	r3, r3, #1
8000939c:	e3a0e000 	mov	lr, #0
800093a0:	e59d2000 	ldr	r2, [sp]
800093a4:	e58dc000 	str	ip, [sp]
800093a8:	e3590000 	cmp	r9, #0
800093ac:	ba00000b 	blt	800093e0 <line+0x390>
800093b0:	e597c004 	ldr	ip, [r7, #4]
800093b4:	e159000c 	cmp	r9, ip
800093b8:	33a05000 	movcc	r5, #0
800093bc:	23a05001 	movcs	r5, #1
800093c0:	e1954fa6 	orrs	r4, r5, r6, lsr #31
800093c4:	1a000005 	bne	800093e0 <line+0x390>
800093c8:	e5975008 	ldr	r5, [r7, #8]
800093cc:	e1560005 	cmp	r6, r5
800093d0:	30249c96 	mlacc	r4, r6, ip, r9
800093d4:	35975000 	ldrcc	r5, [r7]
800093d8:	359dc06c 	ldrcc	ip, [sp, #108]	; 0x6c
800093dc:	3785c104 	strcc	ip, [r5, r4, lsl #2]
800093e0:	e3520000 	cmp	r2, #0
800093e4:	ba000006 	blt	80009404 <line+0x3b4>
800093e8:	e59dc000 	ldr	ip, [sp]
800093ec:	e3510001 	cmp	r1, #1
800093f0:	00899008 	addeq	r9, r9, r8
800093f4:	10866000 	addne	r6, r6, r0
800093f8:	e092200c 	adds	r2, r2, ip
800093fc:	5afffffa 	bpl	800093ec <line+0x39c>
80009400:	e58dc000 	str	ip, [sp]
80009404:	e3510001 	cmp	r1, #1
80009408:	e28ee001 	add	lr, lr, #1
8000940c:	00866000 	addeq	r6, r6, r0
80009410:	10899008 	addne	r9, r9, r8
80009414:	e15e0003 	cmp	lr, r3
80009418:	e082200a 	add	r2, r2, sl
8000941c:	1affffe1 	bne	800093a8 <line+0x358>
80009420:	e28dd048 	add	sp, sp, #72	; 0x48
80009424:	e8bd87f0 	pop	{r4, r5, r6, r7, r8, r9, sl, pc}
80009428:	80808081 	addhi	r8, r0, r1, lsl #1

8000942c <box>:
8000942c:	e1a0c00d 	mov	ip, sp
80009430:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
80009434:	e24cb004 	sub	fp, ip, #4
80009438:	e24dd008 	sub	sp, sp, #8
8000943c:	e59b4004 	ldr	r4, [fp, #4]
80009440:	e1a09001 	mov	r9, r1
80009444:	e3540000 	cmp	r4, #0
80009448:	c3530000 	cmpgt	r3, #0
8000944c:	d3a0c001 	movle	ip, #1
80009450:	c3a0c000 	movgt	ip, #0
80009454:	e3500000 	cmp	r0, #0
80009458:	038cc001 	orreq	ip, ip, #1
8000945c:	e35c0000 	cmp	ip, #0
80009460:	e1a06002 	mov	r6, r2
80009464:	e59b7008 	ldr	r7, [fp, #8]
80009468:	e1a08000 	mov	r8, r0
8000946c:	1a00001c 	bne	800094e4 <box+0xb8>
80009470:	e0813003 	add	r3, r1, r3
80009474:	e2435001 	sub	r5, r3, #1
80009478:	e0864004 	add	r4, r6, r4
8000947c:	e2444001 	sub	r4, r4, #1
80009480:	e88d00c0 	stm	sp, {r6, r7}
80009484:	e1a03005 	mov	r3, r5
80009488:	e2866001 	add	r6, r6, #1
8000948c:	ebfffeef 	bl	80009050 <line>
80009490:	e1a02006 	mov	r2, r6
80009494:	e1a00008 	mov	r0, r8
80009498:	e1a01009 	mov	r1, r9
8000949c:	e1a03009 	mov	r3, r9
800094a0:	e88d0090 	stm	sp, {r4, r7}
800094a4:	ebfffee9 	bl	80009050 <line>
800094a8:	e2891001 	add	r1, r9, #1
800094ac:	e1a02004 	mov	r2, r4
800094b0:	e1a03005 	mov	r3, r5
800094b4:	e1a00008 	mov	r0, r8
800094b8:	e88d0090 	stm	sp, {r4, r7}
800094bc:	ebfffee3 	bl	80009050 <line>
800094c0:	e1a02006 	mov	r2, r6
800094c4:	e1a00008 	mov	r0, r8
800094c8:	e1a01005 	mov	r1, r5
800094cc:	e1a03005 	mov	r3, r5
800094d0:	e58b4004 	str	r4, [fp, #4]
800094d4:	e58b7008 	str	r7, [fp, #8]
800094d8:	e24bd024 	sub	sp, fp, #36	; 0x24
800094dc:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
800094e0:	eafffeda 	b	80009050 <line>
800094e4:	e24bd024 	sub	sp, fp, #36	; 0x24
800094e8:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}

800094ec <fill>:
800094ec:	e1a0c00d 	mov	ip, sp
800094f0:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
800094f4:	e24cb004 	sub	fp, ip, #4
800094f8:	e24dd03c 	sub	sp, sp, #60	; 0x3c
800094fc:	e59bc004 	ldr	ip, [fp, #4]
80009500:	e1a0e001 	mov	lr, r1
80009504:	e35c0000 	cmp	ip, #0
80009508:	c3530000 	cmpgt	r3, #0
8000950c:	d3a01001 	movle	r1, #1
80009510:	c3a01000 	movgt	r1, #0
80009514:	e3500000 	cmp	r0, #0
80009518:	03811001 	orreq	r1, r1, #1
8000951c:	e3510000 	cmp	r1, #0
80009520:	e59b4008 	ldr	r4, [fp, #8]
80009524:	e1a07000 	mov	r7, r0
80009528:	1a000087 	bne	8000974c <fill+0x260>
8000952c:	e24b103c 	sub	r1, fp, #60	; 0x3c
80009530:	e50be03c 	str	lr, [fp, #-60]	; 0x3c
80009534:	e50b2038 	str	r2, [fp, #-56]	; 0x38
80009538:	e50b3034 	str	r3, [fp, #-52]	; 0x34
8000953c:	e50bc030 	str	ip, [fp, #-48]	; 0x30
80009540:	ebfffdbe 	bl	80008c40 <graph_insect>
80009544:	e3500000 	cmp	r0, #0
80009548:	0a00007f 	beq	8000974c <fill+0x260>
8000954c:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
80009550:	e51b2030 	ldr	r2, [fp, #-48]	; 0x30
80009554:	e1a01003 	mov	r1, r3
80009558:	e50b3044 	str	r3, [fp, #-68]	; 0x44
8000955c:	e51b603c 	ldr	r6, [fp, #-60]	; 0x3c
80009560:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
80009564:	e1a00c24 	lsr	r0, r4, #24
80009568:	e0812002 	add	r2, r1, r2
8000956c:	e0863003 	add	r3, r6, r3
80009570:	e35000ff 	cmp	r0, #255	; 0xff
80009574:	e50b0050 	str	r0, [fp, #-80]	; 0x50
80009578:	e50b2060 	str	r2, [fp, #-96]	; 0x60
8000957c:	e50b3048 	str	r3, [fp, #-72]	; 0x48
80009580:	0a000073 	beq	80009754 <fill+0x268>
80009584:	e51b3044 	ldr	r3, [fp, #-68]	; 0x44
80009588:	e51b2060 	ldr	r2, [fp, #-96]	; 0x60
8000958c:	e1530002 	cmp	r3, r2
80009590:	e1a02824 	lsr	r2, r4, #16
80009594:	e1a03424 	lsr	r3, r4, #8
80009598:	aa00006b 	bge	8000974c <fill+0x260>
8000959c:	e51b8050 	ldr	r8, [fp, #-80]	; 0x50
800095a0:	e20210ff 	and	r1, r2, #255	; 0xff
800095a4:	e1a00008 	mov	r0, r8
800095a8:	e0000091 	mul	r0, r1, r0
800095ac:	e20320ff 	and	r2, r3, #255	; 0xff
800095b0:	e1a0c008 	mov	ip, r8
800095b4:	e20430ff 	and	r3, r4, #255	; 0xff
800095b8:	e00c0c92 	mul	ip, r2, ip
800095bc:	e1a0e008 	mov	lr, r8
800095c0:	e1a01000 	mov	r1, r0
800095c4:	e00e0e93 	mul	lr, r3, lr
800095c8:	e59f01d4 	ldr	r0, [pc, #468]	; 800097a4 <fill+0x2b8>
800095cc:	e1a0200c 	mov	r2, ip
800095d0:	e1a0300e 	mov	r3, lr
800095d4:	e0c4e290 	smull	lr, r4, r0, r2
800095d8:	e0cce190 	smull	lr, ip, r0, r1
800095dc:	e0ce9390 	smull	r9, lr, r0, r3
800095e0:	e0844002 	add	r4, r4, r2
800095e4:	e1a02fc2 	asr	r2, r2, #31
800095e8:	e08ee003 	add	lr, lr, r3
800095ec:	e1a05fc1 	asr	r5, r1, #31
800095f0:	e1a03fc3 	asr	r3, r3, #31
800095f4:	e08c1001 	add	r1, ip, r1
800095f8:	e06223c4 	rsb	r2, r2, r4, asr #7
800095fc:	e06333ce 	rsb	r3, r3, lr, asr #7
80009600:	e20220ff 	and	r2, r2, #255	; 0xff
80009604:	e06513c1 	rsb	r1, r5, r1, asr #7
80009608:	e26890ff 	rsb	r9, r8, #255	; 0xff
8000960c:	e20110ff 	and	r1, r1, #255	; 0xff
80009610:	e50b2058 	str	r2, [fp, #-88]	; 0x58
80009614:	e20330ff 	and	r3, r3, #255	; 0xff
80009618:	e50b704c 	str	r7, [fp, #-76]	; 0x4c
8000961c:	e1a02000 	mov	r2, r0
80009620:	e1a0e008 	mov	lr, r8
80009624:	e1a07009 	mov	r7, r9
80009628:	e50b1054 	str	r1, [fp, #-84]	; 0x54
8000962c:	e50b305c 	str	r3, [fp, #-92]	; 0x5c
80009630:	e51b3048 	ldr	r3, [fp, #-72]	; 0x48
80009634:	e1530006 	cmp	r3, r6
80009638:	da00003c 	ble	80009730 <fill+0x244>
8000963c:	e51b304c 	ldr	r3, [fp, #-76]	; 0x4c
80009640:	e1a09007 	mov	r9, r7
80009644:	e593a000 	ldr	sl, [r3]
80009648:	e50ba040 	str	sl, [fp, #-64]	; 0x40
8000964c:	e51b304c 	ldr	r3, [fp, #-76]	; 0x4c
80009650:	e51b0050 	ldr	r0, [fp, #-80]	; 0x50
80009654:	e593e004 	ldr	lr, [r3, #4]
80009658:	e51b3044 	ldr	r3, [fp, #-68]	; 0x44
8000965c:	e51b1048 	ldr	r1, [fp, #-72]	; 0x48
80009660:	e023639e 	mla	r3, lr, r3, r6
80009664:	e2866001 	add	r6, r6, #1
80009668:	e1a0e003 	mov	lr, r3
8000966c:	e51b3040 	ldr	r3, [fp, #-64]	; 0x40
80009670:	e1560001 	cmp	r6, r1
80009674:	e793310e 	ldr	r3, [r3, lr, lsl #2]
80009678:	e1a08c23 	lsr	r8, r3, #24
8000967c:	e1a04823 	lsr	r4, r3, #16
80009680:	e268c0ff 	rsb	ip, r8, #255	; 0xff
80009684:	e20440ff 	and	r4, r4, #255	; 0xff
80009688:	e0040499 	mul	r4, r9, r4
8000968c:	e000009c 	mul	r0, ip, r0
80009690:	e20310ff 	and	r1, r3, #255	; 0xff
80009694:	e1a0c000 	mov	ip, r0
80009698:	e0c05492 	smull	r5, r0, r2, r4
8000969c:	e1a03423 	lsr	r3, r3, #8
800096a0:	e0010199 	mul	r1, r9, r1
800096a4:	e20330ff 	and	r3, r3, #255	; 0xff
800096a8:	e0030399 	mul	r3, r9, r3
800096ac:	e0c75c92 	smull	r5, r7, r2, ip
800096b0:	e0805004 	add	r5, r0, r4
800096b4:	e1a00fc4 	asr	r0, r4, #31
800096b8:	e06003c5 	rsb	r0, r0, r5, asr #7
800096bc:	e0c54192 	smull	r4, r5, r2, r1
800096c0:	e0ca4392 	smull	r4, sl, r2, r3
800096c4:	e51b405c 	ldr	r4, [fp, #-92]	; 0x5c
800096c8:	e0855001 	add	r5, r5, r1
800096cc:	e1a01fc1 	asr	r1, r1, #31
800096d0:	e0840000 	add	r0, r4, r0
800096d4:	e06113c5 	rsb	r1, r1, r5, asr #7
800096d8:	e08a4003 	add	r4, sl, r3
800096dc:	e51b5054 	ldr	r5, [fp, #-84]	; 0x54
800096e0:	e1a03fc3 	asr	r3, r3, #31
800096e4:	e087700c 	add	r7, r7, ip
800096e8:	e06343c4 	rsb	r4, r3, r4, asr #7
800096ec:	e1a0cfcc 	asr	ip, ip, #31
800096f0:	e51b3058 	ldr	r3, [fp, #-88]	; 0x58
800096f4:	e20000ff 	and	r0, r0, #255	; 0xff
800096f8:	e06cc3c7 	rsb	ip, ip, r7, asr #7
800096fc:	e1a00800 	lsl	r0, r0, #16
80009700:	e088c00c 	add	ip, r8, ip
80009704:	e0851001 	add	r1, r5, r1
80009708:	e0834004 	add	r4, r3, r4
8000970c:	e1800c0c 	orr	r0, r0, ip, lsl #24
80009710:	e20110ff 	and	r1, r1, #255	; 0xff
80009714:	e51b3040 	ldr	r3, [fp, #-64]	; 0x40
80009718:	e1800001 	orr	r0, r0, r1
8000971c:	e20440ff 	and	r4, r4, #255	; 0xff
80009720:	e1800404 	orr	r0, r0, r4, lsl #8
80009724:	e783010e 	str	r0, [r3, lr, lsl #2]
80009728:	1affffc7 	bne	8000964c <fill+0x160>
8000972c:	e1a07009 	mov	r7, r9
80009730:	e51b3044 	ldr	r3, [fp, #-68]	; 0x44
80009734:	e51b1060 	ldr	r1, [fp, #-96]	; 0x60
80009738:	e2833001 	add	r3, r3, #1
8000973c:	e1530001 	cmp	r3, r1
80009740:	e50b3044 	str	r3, [fp, #-68]	; 0x44
80009744:	151b603c 	ldrne	r6, [fp, #-60]	; 0x3c
80009748:	1affffb8 	bne	80009630 <fill+0x144>
8000974c:	e24bd028 	sub	sp, fp, #40	; 0x28
80009750:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
80009754:	e1510002 	cmp	r1, r2
80009758:	e1a03001 	mov	r3, r1
8000975c:	aafffffa 	bge	8000974c <fill+0x260>
80009760:	e51b1048 	ldr	r1, [fp, #-72]	; 0x48
80009764:	e1a0c002 	mov	ip, r2
80009768:	e1a02003 	mov	r2, r3
8000976c:	e1510006 	cmp	r1, r6
80009770:	c5970000 	ldrgt	r0, [r7]
80009774:	da000005 	ble	80009790 <fill+0x2a4>
80009778:	e5973004 	ldr	r3, [r7, #4]
8000977c:	e0236392 	mla	r3, r2, r3, r6
80009780:	e2866001 	add	r6, r6, #1
80009784:	e1560001 	cmp	r6, r1
80009788:	e7804103 	str	r4, [r0, r3, lsl #2]
8000978c:	1afffff9 	bne	80009778 <fill+0x28c>
80009790:	e2822001 	add	r2, r2, #1
80009794:	e152000c 	cmp	r2, ip
80009798:	0affffeb 	beq	8000974c <fill+0x260>
8000979c:	e51b603c 	ldr	r6, [fp, #-60]	; 0x3c
800097a0:	eafffff1 	b	8000976c <fill+0x280>
800097a4:	80808081 	addhi	r8, r0, r1, lsl #1

800097a8 <draw_char>:
800097a8:	e92d47f0 	push	{r4, r5, r6, r7, r8, r9, sl, lr}
800097ac:	e250a000 	subs	sl, r0, #0
800097b0:	e24dd040 	sub	sp, sp, #64	; 0x40
800097b4:	e58d1008 	str	r1, [sp, #8]
800097b8:	e58d200c 	str	r2, [sp, #12]
800097bc:	e1a00003 	mov	r0, r3
800097c0:	e59d8064 	ldr	r8, [sp, #100]	; 0x64
800097c4:	0a000005 	beq	800097e0 <draw_char+0x38>
800097c8:	e59d3060 	ldr	r3, [sp, #96]	; 0x60
800097cc:	e5931004 	ldr	r1, [r3, #4]
800097d0:	e3510008 	cmp	r1, #8
800097d4:	9a00015d 	bls	80009d50 <draw_char+0x5a8>
800097d8:	e3510010 	cmp	r1, #16
800097dc:	9a000001 	bls	800097e8 <draw_char+0x40>
800097e0:	e28dd040 	add	sp, sp, #64	; 0x40
800097e4:	e8bd87f0 	pop	{r4, r5, r6, r7, r8, r9, sl, pc}
800097e8:	e59d3060 	ldr	r3, [sp, #96]	; 0x60
800097ec:	e5931008 	ldr	r1, [r3, #8]
800097f0:	e1a03c28 	lsr	r3, r8, #24
800097f4:	e1a02003 	mov	r2, r3
800097f8:	e58d301c 	str	r3, [sp, #28]
800097fc:	e1a03080 	lsl	r3, r0, #1
80009800:	e35200ff 	cmp	r2, #255	; 0xff
80009804:	e0020391 	mul	r2, r1, r3
80009808:	e59d3060 	ldr	r3, [sp, #96]	; 0x60
8000980c:	e593700c 	ldr	r7, [r3, #12]
80009810:	0a000103 	beq	80009c24 <draw_char+0x47c>
80009814:	e3510000 	cmp	r1, #0
80009818:	e1a03428 	lsr	r3, r8, #8
8000981c:	e1a01828 	lsr	r1, r8, #16
80009820:	0affffee 	beq	800097e0 <draw_char+0x38>
80009824:	e201c0ff 	and	ip, r1, #255	; 0xff
80009828:	e20310ff 	and	r1, r3, #255	; 0xff
8000982c:	e20830ff 	and	r3, r8, #255	; 0xff
80009830:	e59d801c 	ldr	r8, [sp, #28]
80009834:	e0872002 	add	r2, r7, r2
80009838:	e1a00008 	mov	r0, r8
8000983c:	e000009c 	mul	r0, ip, r0
80009840:	e1a0e008 	mov	lr, r8
80009844:	e1a04008 	mov	r4, r8
80009848:	e0040493 	mul	r4, r3, r4
8000984c:	e00e0e91 	mul	lr, r1, lr
80009850:	e1a0c000 	mov	ip, r0
80009854:	e59f0834 	ldr	r0, [pc, #2100]	; 8000a090 <draw_char+0x8e8>
80009858:	e1a03004 	mov	r3, r4
8000985c:	e1a0100e 	mov	r1, lr
80009860:	e0c54190 	smull	r4, r5, r0, r1
80009864:	e0c64c90 	smull	r4, r6, r0, ip
80009868:	e0c49390 	smull	r9, r4, r0, r3
8000986c:	e0855001 	add	r5, r5, r1
80009870:	e0844003 	add	r4, r4, r3
80009874:	e1a03fc3 	asr	r3, r3, #31
80009878:	e06333c4 	rsb	r3, r3, r4, asr #7
8000987c:	e20330ff 	and	r3, r3, #255	; 0xff
80009880:	e58d3028 	str	r3, [sp, #40]	; 0x28
80009884:	e59d3008 	ldr	r3, [sp, #8]
80009888:	e1a01fc1 	asr	r1, r1, #31
8000988c:	e1a0efcc 	asr	lr, ip, #31
80009890:	e06113c5 	rsb	r1, r1, r5, asr #7
80009894:	e086c00c 	add	ip, r6, ip
80009898:	e59d500c 	ldr	r5, [sp, #12]
8000989c:	e2837008 	add	r7, r3, #8
800098a0:	e06ec3cc 	rsb	ip, lr, ip, asr #7
800098a4:	e58d0030 	str	r0, [sp, #48]	; 0x30
800098a8:	e2830010 	add	r0, r3, #16
800098ac:	e26830ff 	rsb	r3, r8, #255	; 0xff
800098b0:	e58d2004 	str	r2, [sp, #4]
800098b4:	e58d302c 	str	r3, [sp, #44]	; 0x2c
800098b8:	e20c20ff 	and	r2, ip, #255	; 0xff
800098bc:	e1a03000 	mov	r3, r0
800098c0:	e58d2020 	str	r2, [sp, #32]
800098c4:	e1a00007 	mov	r0, r7
800098c8:	e20120ff 	and	r2, r1, #255	; 0xff
800098cc:	e1a0700a 	mov	r7, sl
800098d0:	e1a0a005 	mov	sl, r5
800098d4:	e1a05003 	mov	r5, r3
800098d8:	e58d2024 	str	r2, [sp, #36]	; 0x24
800098dc:	e59d3004 	ldr	r3, [sp, #4]
800098e0:	e1a01faa 	lsr	r1, sl, #31
800098e4:	e5d38000 	ldrb	r8, [r3]
800098e8:	e59d3008 	ldr	r3, [sp, #8]
800098ec:	e3a02080 	mov	r2, #128	; 0x80
800098f0:	e58d1010 	str	r1, [sp, #16]
800098f4:	e58d5014 	str	r5, [sp, #20]
800098f8:	e1120008 	tst	r2, r8
800098fc:	03a01001 	moveq	r1, #1
80009900:	13a01000 	movne	r1, #0
80009904:	e1911fa3 	orrs	r1, r1, r3, lsr #31
80009908:	1a000051 	bne	80009a54 <draw_char+0x2ac>
8000990c:	e5971004 	ldr	r1, [r7, #4]
80009910:	e59dc010 	ldr	ip, [sp, #16]
80009914:	e1530001 	cmp	r3, r1
80009918:	238cc001 	orrcs	ip, ip, #1
8000991c:	e35c0000 	cmp	ip, #0
80009920:	1a00004b 	bne	80009a54 <draw_char+0x2ac>
80009924:	e597c008 	ldr	ip, [r7, #8]
80009928:	e15a000c 	cmp	sl, ip
8000992c:	2a000048 	bcs	80009a54 <draw_char+0x2ac>
80009930:	e02c3a91 	mla	ip, r1, sl, r3
80009934:	e5971000 	ldr	r1, [r7]
80009938:	e59de02c 	ldr	lr, [sp, #44]	; 0x2c
8000993c:	e791110c 	ldr	r1, [r1, ip, lsl #2]
80009940:	e59d901c 	ldr	r9, [sp, #28]
80009944:	e1a06821 	lsr	r6, r1, #16
80009948:	e1a04c21 	lsr	r4, r1, #24
8000994c:	e58dc038 	str	ip, [sp, #56]	; 0x38
80009950:	e20660ff 	and	r6, r6, #255	; 0xff
80009954:	e1a0c00e 	mov	ip, lr
80009958:	e00c0c96 	mul	ip, r6, ip
8000995c:	e58d4034 	str	r4, [sp, #52]	; 0x34
80009960:	e26440ff 	rsb	r4, r4, #255	; 0xff
80009964:	e0090994 	mul	r9, r4, r9
80009968:	e1a0600c 	mov	r6, ip
8000996c:	e201c0ff 	and	ip, r1, #255	; 0xff
80009970:	e1a0400e 	mov	r4, lr
80009974:	e1a05009 	mov	r5, r9
80009978:	e00e0e9c 	mul	lr, ip, lr
8000997c:	e59d9030 	ldr	r9, [sp, #48]	; 0x30
80009980:	e1a01421 	lsr	r1, r1, #8
80009984:	e20110ff 	and	r1, r1, #255	; 0xff
80009988:	e58de018 	str	lr, [sp, #24]
8000998c:	e1a0c001 	mov	ip, r1
80009990:	e1a0e009 	mov	lr, r9
80009994:	e0ce1e96 	smull	r1, lr, r6, lr
80009998:	e1a0100c 	mov	r1, ip
8000999c:	e0010194 	mul	r1, r4, r1
800099a0:	e58d503c 	str	r5, [sp, #60]	; 0x3c
800099a4:	e0c54599 	smull	r4, r5, r9, r5
800099a8:	e1a0c001 	mov	ip, r1
800099ac:	e1a04005 	mov	r4, r5
800099b0:	e59d5018 	ldr	r5, [sp, #24]
800099b4:	e1a01009 	mov	r1, r9
800099b8:	e08e9006 	add	r9, lr, r6
800099bc:	e1a0efc6 	asr	lr, r6, #31
800099c0:	e06ee3c9 	rsb	lr, lr, r9, asr #7
800099c4:	e0c59591 	smull	r9, r5, r1, r5
800099c8:	e1a06001 	mov	r6, r1
800099cc:	e1a09005 	mov	r9, r5
800099d0:	e1a0500c 	mov	r5, ip
800099d4:	e0cc1c96 	smull	r1, ip, r6, ip
800099d8:	e59d1028 	ldr	r1, [sp, #40]	; 0x28
800099dc:	e1a0600c 	mov	r6, ip
800099e0:	e59dc018 	ldr	ip, [sp, #24]
800099e4:	e081e00e 	add	lr, r1, lr
800099e8:	e1a01004 	mov	r1, r4
800099ec:	e59d403c 	ldr	r4, [sp, #60]	; 0x3c
800099f0:	e089900c 	add	r9, r9, ip
800099f4:	e1a0cfcc 	asr	ip, ip, #31
800099f8:	e06cc3c9 	rsb	ip, ip, r9, asr #7
800099fc:	e59d9034 	ldr	r9, [sp, #52]	; 0x34
80009a00:	e0811004 	add	r1, r1, r4
80009a04:	e1a04fc4 	asr	r4, r4, #31
80009a08:	e06443c1 	rsb	r4, r4, r1, asr #7
80009a0c:	e0894004 	add	r4, r9, r4
80009a10:	e1a01fc5 	asr	r1, r5, #31
80009a14:	e0866005 	add	r6, r6, r5
80009a18:	e59d9020 	ldr	r9, [sp, #32]
80009a1c:	e06163c6 	rsb	r6, r1, r6, asr #7
80009a20:	e59d1024 	ldr	r1, [sp, #36]	; 0x24
80009a24:	e20ee0ff 	and	lr, lr, #255	; 0xff
80009a28:	e1a0e80e 	lsl	lr, lr, #16
80009a2c:	e089c00c 	add	ip, r9, ip
80009a30:	e20cc0ff 	and	ip, ip, #255	; 0xff
80009a34:	e0816006 	add	r6, r1, r6
80009a38:	e18eec04 	orr	lr, lr, r4, lsl #24
80009a3c:	e18ee00c 	orr	lr, lr, ip
80009a40:	e20660ff 	and	r6, r6, #255	; 0xff
80009a44:	e18e1406 	orr	r1, lr, r6, lsl #8
80009a48:	e59dc038 	ldr	ip, [sp, #56]	; 0x38
80009a4c:	e597e000 	ldr	lr, [r7]
80009a50:	e78e110c 	str	r1, [lr, ip, lsl #2]
80009a54:	e2833001 	add	r3, r3, #1
80009a58:	e1530000 	cmp	r3, r0
80009a5c:	e1a020c2 	asr	r2, r2, #1
80009a60:	1affffa4 	bne	800098f8 <draw_char+0x150>
80009a64:	e59d2004 	ldr	r2, [sp, #4]
80009a68:	e1a01faa 	lsr	r1, sl, #31
80009a6c:	e5d28001 	ldrb	r8, [r2, #1]
80009a70:	e1a03000 	mov	r3, r0
80009a74:	e3a02080 	mov	r2, #128	; 0x80
80009a78:	e59d5014 	ldr	r5, [sp, #20]
80009a7c:	e58d1010 	str	r1, [sp, #16]
80009a80:	e58d0014 	str	r0, [sp, #20]
80009a84:	e1120008 	tst	r2, r8
80009a88:	03a01001 	moveq	r1, #1
80009a8c:	13a01000 	movne	r1, #0
80009a90:	e1911fa3 	orrs	r1, r1, r3, lsr #31
80009a94:	1a000051 	bne	80009be0 <draw_char+0x438>
80009a98:	e5971004 	ldr	r1, [r7, #4]
80009a9c:	e59dc010 	ldr	ip, [sp, #16]
80009aa0:	e1530001 	cmp	r3, r1
80009aa4:	238cc001 	orrcs	ip, ip, #1
80009aa8:	e35c0000 	cmp	ip, #0
80009aac:	1a00004b 	bne	80009be0 <draw_char+0x438>
80009ab0:	e597c008 	ldr	ip, [r7, #8]
80009ab4:	e15a000c 	cmp	sl, ip
80009ab8:	2a000048 	bcs	80009be0 <draw_char+0x438>
80009abc:	e02c3a91 	mla	ip, r1, sl, r3
80009ac0:	e5971000 	ldr	r1, [r7]
80009ac4:	e59de02c 	ldr	lr, [sp, #44]	; 0x2c
80009ac8:	e791110c 	ldr	r1, [r1, ip, lsl #2]
80009acc:	e59d901c 	ldr	r9, [sp, #28]
80009ad0:	e1a06821 	lsr	r6, r1, #16
80009ad4:	e1a04c21 	lsr	r4, r1, #24
80009ad8:	e58dc038 	str	ip, [sp, #56]	; 0x38
80009adc:	e20660ff 	and	r6, r6, #255	; 0xff
80009ae0:	e1a0c00e 	mov	ip, lr
80009ae4:	e00c0c96 	mul	ip, r6, ip
80009ae8:	e58d4034 	str	r4, [sp, #52]	; 0x34
80009aec:	e26440ff 	rsb	r4, r4, #255	; 0xff
80009af0:	e0090994 	mul	r9, r4, r9
80009af4:	e1a0600c 	mov	r6, ip
80009af8:	e201c0ff 	and	ip, r1, #255	; 0xff
80009afc:	e1a0400e 	mov	r4, lr
80009b00:	e1a00009 	mov	r0, r9
80009b04:	e00e0e9c 	mul	lr, ip, lr
80009b08:	e59d9030 	ldr	r9, [sp, #48]	; 0x30
80009b0c:	e1a01421 	lsr	r1, r1, #8
80009b10:	e20110ff 	and	r1, r1, #255	; 0xff
80009b14:	e58de018 	str	lr, [sp, #24]
80009b18:	e1a0c001 	mov	ip, r1
80009b1c:	e1a0e009 	mov	lr, r9
80009b20:	e0ce1e96 	smull	r1, lr, r6, lr
80009b24:	e1a0100c 	mov	r1, ip
80009b28:	e0010194 	mul	r1, r4, r1
80009b2c:	e58d003c 	str	r0, [sp, #60]	; 0x3c
80009b30:	e0c04099 	smull	r4, r0, r9, r0
80009b34:	e1a0c001 	mov	ip, r1
80009b38:	e1a04000 	mov	r4, r0
80009b3c:	e59d0018 	ldr	r0, [sp, #24]
80009b40:	e1a01009 	mov	r1, r9
80009b44:	e08e9006 	add	r9, lr, r6
80009b48:	e1a0efc6 	asr	lr, r6, #31
80009b4c:	e06ee3c9 	rsb	lr, lr, r9, asr #7
80009b50:	e0c09091 	smull	r9, r0, r1, r0
80009b54:	e1a06001 	mov	r6, r1
80009b58:	e1a09000 	mov	r9, r0
80009b5c:	e1a0000c 	mov	r0, ip
80009b60:	e0cc1c96 	smull	r1, ip, r6, ip
80009b64:	e59d1028 	ldr	r1, [sp, #40]	; 0x28
80009b68:	e1a0600c 	mov	r6, ip
80009b6c:	e59dc018 	ldr	ip, [sp, #24]
80009b70:	e081e00e 	add	lr, r1, lr
80009b74:	e1a01004 	mov	r1, r4
80009b78:	e59d403c 	ldr	r4, [sp, #60]	; 0x3c
80009b7c:	e089900c 	add	r9, r9, ip
80009b80:	e1a0cfcc 	asr	ip, ip, #31
80009b84:	e06cc3c9 	rsb	ip, ip, r9, asr #7
80009b88:	e59d9034 	ldr	r9, [sp, #52]	; 0x34
80009b8c:	e0811004 	add	r1, r1, r4
80009b90:	e1a04fc4 	asr	r4, r4, #31
80009b94:	e06443c1 	rsb	r4, r4, r1, asr #7
80009b98:	e0894004 	add	r4, r9, r4
80009b9c:	e1a01fc0 	asr	r1, r0, #31
80009ba0:	e0866000 	add	r6, r6, r0
80009ba4:	e59d9020 	ldr	r9, [sp, #32]
80009ba8:	e06163c6 	rsb	r6, r1, r6, asr #7
80009bac:	e59d1024 	ldr	r1, [sp, #36]	; 0x24
80009bb0:	e20ee0ff 	and	lr, lr, #255	; 0xff
80009bb4:	e1a0e80e 	lsl	lr, lr, #16
80009bb8:	e089c00c 	add	ip, r9, ip
80009bbc:	e20cc0ff 	and	ip, ip, #255	; 0xff
80009bc0:	e0816006 	add	r6, r1, r6
80009bc4:	e18eec04 	orr	lr, lr, r4, lsl #24
80009bc8:	e18ee00c 	orr	lr, lr, ip
80009bcc:	e20660ff 	and	r6, r6, #255	; 0xff
80009bd0:	e18e1406 	orr	r1, lr, r6, lsl #8
80009bd4:	e59dc038 	ldr	ip, [sp, #56]	; 0x38
80009bd8:	e597e000 	ldr	lr, [r7]
80009bdc:	e78e110c 	str	r1, [lr, ip, lsl #2]
80009be0:	e2833001 	add	r3, r3, #1
80009be4:	e1530005 	cmp	r3, r5
80009be8:	e1a020c2 	asr	r2, r2, #1
80009bec:	1affffa4 	bne	80009a84 <draw_char+0x2dc>
80009bf0:	e59d3060 	ldr	r3, [sp, #96]	; 0x60
80009bf4:	e28aa001 	add	sl, sl, #1
80009bf8:	e5932008 	ldr	r2, [r3, #8]
80009bfc:	e59d300c 	ldr	r3, [sp, #12]
80009c00:	e59d0014 	ldr	r0, [sp, #20]
80009c04:	e063300a 	rsb	r3, r3, sl
80009c08:	e1520003 	cmp	r2, r3
80009c0c:	e59d3004 	ldr	r3, [sp, #4]
80009c10:	e2833002 	add	r3, r3, #2
80009c14:	e58d3004 	str	r3, [sp, #4]
80009c18:	8affff2f 	bhi	800098dc <draw_char+0x134>
80009c1c:	e28dd040 	add	sp, sp, #64	; 0x40
80009c20:	e8bd87f0 	pop	{r4, r5, r6, r7, r8, r9, sl, pc}
80009c24:	e3510000 	cmp	r1, #0
80009c28:	0afffeec 	beq	800097e0 <draw_char+0x38>
80009c2c:	e59d3008 	ldr	r3, [sp, #8]
80009c30:	e59d400c 	ldr	r4, [sp, #12]
80009c34:	e59d5060 	ldr	r5, [sp, #96]	; 0x60
80009c38:	e0879002 	add	r9, r7, r2
80009c3c:	e1a0e009 	mov	lr, r9
80009c40:	e3a0c000 	mov	ip, #0
80009c44:	e2837008 	add	r7, r3, #8
80009c48:	e2830010 	add	r0, r3, #16
80009c4c:	e58d9010 	str	r9, [sp, #16]
80009c50:	e58d8064 	str	r8, [sp, #100]	; 0x64
80009c54:	e59d3010 	ldr	r3, [sp, #16]
80009c58:	e3a02080 	mov	r2, #128	; 0x80
80009c5c:	e7d3108c 	ldrb	r1, [r3, ip, lsl #1]
80009c60:	e59d3008 	ldr	r3, [sp, #8]
80009c64:	e1a08fa4 	lsr	r8, r4, #31
80009c68:	e58d0004 	str	r0, [sp, #4]
80009c6c:	e1120001 	tst	r2, r1
80009c70:	03a06001 	moveq	r6, #1
80009c74:	13a06000 	movne	r6, #0
80009c78:	e1966fa3 	orrs	r6, r6, r3, lsr #31
80009c7c:	1a00000b 	bne	80009cb0 <draw_char+0x508>
80009c80:	e59a6004 	ldr	r6, [sl, #4]
80009c84:	e0293496 	mla	r9, r6, r4, r3
80009c88:	e1530006 	cmp	r3, r6
80009c8c:	31a06008 	movcc	r6, r8
80009c90:	23886001 	orrcs	r6, r8, #1
80009c94:	e3560000 	cmp	r6, #0
80009c98:	1a000004 	bne	80009cb0 <draw_char+0x508>
80009c9c:	e59a6008 	ldr	r6, [sl, #8]
80009ca0:	e1540006 	cmp	r4, r6
80009ca4:	359a6000 	ldrcc	r6, [sl]
80009ca8:	359d0064 	ldrcc	r0, [sp, #100]	; 0x64
80009cac:	37860109 	strcc	r0, [r6, r9, lsl #2]
80009cb0:	e2833001 	add	r3, r3, #1
80009cb4:	e1530007 	cmp	r3, r7
80009cb8:	e1a020c2 	asr	r2, r2, #1
80009cbc:	1affffea 	bne	80009c6c <draw_char+0x4c4>
80009cc0:	e5de1001 	ldrb	r1, [lr, #1]
80009cc4:	e1a03007 	mov	r3, r7
80009cc8:	e3a02080 	mov	r2, #128	; 0x80
80009ccc:	e1a08fa4 	lsr	r8, r4, #31
80009cd0:	e59d0004 	ldr	r0, [sp, #4]
80009cd4:	e58dc004 	str	ip, [sp, #4]
80009cd8:	e1120001 	tst	r2, r1
80009cdc:	03a06001 	moveq	r6, #1
80009ce0:	13a06000 	movne	r6, #0
80009ce4:	e1966fa3 	orrs	r6, r6, r3, lsr #31
80009ce8:	1a00000b 	bne	80009d1c <draw_char+0x574>
80009cec:	e59a6004 	ldr	r6, [sl, #4]
80009cf0:	e0293496 	mla	r9, r6, r4, r3
80009cf4:	e1530006 	cmp	r3, r6
80009cf8:	31a06008 	movcc	r6, r8
80009cfc:	23886001 	orrcs	r6, r8, #1
80009d00:	e3560000 	cmp	r6, #0
80009d04:	1a000004 	bne	80009d1c <draw_char+0x574>
80009d08:	e59a6008 	ldr	r6, [sl, #8]
80009d0c:	e1540006 	cmp	r4, r6
80009d10:	359a6000 	ldrcc	r6, [sl]
80009d14:	359dc064 	ldrcc	ip, [sp, #100]	; 0x64
80009d18:	3786c109 	strcc	ip, [r6, r9, lsl #2]
80009d1c:	e2833001 	add	r3, r3, #1
80009d20:	e1530000 	cmp	r3, r0
80009d24:	e1a020c2 	asr	r2, r2, #1
80009d28:	1affffea 	bne	80009cd8 <draw_char+0x530>
80009d2c:	e59dc004 	ldr	ip, [sp, #4]
80009d30:	e5953008 	ldr	r3, [r5, #8]
80009d34:	e28cc001 	add	ip, ip, #1
80009d38:	e15c0003 	cmp	ip, r3
80009d3c:	e2844001 	add	r4, r4, #1
80009d40:	e28ee002 	add	lr, lr, #2
80009d44:	3affffc2 	bcc	80009c54 <draw_char+0x4ac>
80009d48:	e28dd040 	add	sp, sp, #64	; 0x40
80009d4c:	e8bd87f0 	pop	{r4, r5, r6, r7, r8, r9, sl, pc}
80009d50:	e5932008 	ldr	r2, [r3, #8]
80009d54:	e59d1060 	ldr	r1, [sp, #96]	; 0x60
80009d58:	e1a03c28 	lsr	r3, r8, #24
80009d5c:	e35300ff 	cmp	r3, #255	; 0xff
80009d60:	e58d3028 	str	r3, [sp, #40]	; 0x28
80009d64:	e591500c 	ldr	r5, [r1, #12]
80009d68:	e0030092 	mul	r3, r2, r0
80009d6c:	0a00009b 	beq	80009fe0 <draw_char+0x838>
80009d70:	e3520000 	cmp	r2, #0
80009d74:	e1a01828 	lsr	r1, r8, #16
80009d78:	e1a02428 	lsr	r2, r8, #8
80009d7c:	0afffe97 	beq	800097e0 <draw_char+0x38>
80009d80:	e201c0ff 	and	ip, r1, #255	; 0xff
80009d84:	e20810ff 	and	r1, r8, #255	; 0xff
80009d88:	e59d8028 	ldr	r8, [sp, #40]	; 0x28
80009d8c:	e20220ff 	and	r2, r2, #255	; 0xff
80009d90:	e1a00008 	mov	r0, r8
80009d94:	e000009c 	mul	r0, ip, r0
80009d98:	e1a0e008 	mov	lr, r8
80009d9c:	e1a04008 	mov	r4, r8
80009da0:	e1a0c000 	mov	ip, r0
80009da4:	e0040491 	mul	r4, r1, r4
80009da8:	e59f02e0 	ldr	r0, [pc, #736]	; 8000a090 <draw_char+0x8e8>
80009dac:	e00e0e92 	mul	lr, r2, lr
80009db0:	e1a01004 	mov	r1, r4
80009db4:	e1a0200e 	mov	r2, lr
80009db8:	e0c74c90 	smull	r4, r7, r0, ip
80009dbc:	e0c64290 	smull	r4, r6, r0, r2
80009dc0:	e0c49190 	smull	r9, r4, r0, r1
80009dc4:	e1a0efcc 	asr	lr, ip, #31
80009dc8:	e087c00c 	add	ip, r7, ip
80009dcc:	e0866002 	add	r6, r6, r2
80009dd0:	e0853003 	add	r3, r5, r3
80009dd4:	e1a02fc2 	asr	r2, r2, #31
80009dd8:	e06ec3cc 	rsb	ip, lr, ip, asr #7
80009ddc:	e0844001 	add	r4, r4, r1
80009de0:	e06223c6 	rsb	r2, r2, r6, asr #7
80009de4:	e1a01fc1 	asr	r1, r1, #31
80009de8:	e58d3004 	str	r3, [sp, #4]
80009dec:	e20c30ff 	and	r3, ip, #255	; 0xff
80009df0:	e06113c4 	rsb	r1, r1, r4, asr #7
80009df4:	e58d302c 	str	r3, [sp, #44]	; 0x2c
80009df8:	e20230ff 	and	r3, r2, #255	; 0xff
80009dfc:	e58d3030 	str	r3, [sp, #48]	; 0x30
80009e00:	e20130ff 	and	r3, r1, #255	; 0xff
80009e04:	e58d3034 	str	r3, [sp, #52]	; 0x34
80009e08:	e58d003c 	str	r0, [sp, #60]	; 0x3c
80009e0c:	e59d3008 	ldr	r3, [sp, #8]
80009e10:	e59d000c 	ldr	r0, [sp, #12]
80009e14:	e2837008 	add	r7, r3, #8
80009e18:	e1a0600a 	mov	r6, sl
80009e1c:	e26830ff 	rsb	r3, r8, #255	; 0xff
80009e20:	e1a0a000 	mov	sl, r0
80009e24:	e58d3038 	str	r3, [sp, #56]	; 0x38
80009e28:	e59d3004 	ldr	r3, [sp, #4]
80009e2c:	e59d4008 	ldr	r4, [sp, #8]
80009e30:	e4d38001 	ldrb	r8, [r3], #1
80009e34:	e1a01faa 	lsr	r1, sl, #31
80009e38:	e58d3004 	str	r3, [sp, #4]
80009e3c:	e3a03080 	mov	r3, #128	; 0x80
80009e40:	e1130008 	tst	r3, r8
80009e44:	03a02001 	moveq	r2, #1
80009e48:	13a02000 	movne	r2, #0
80009e4c:	e1922fa4 	orrs	r2, r2, r4, lsr #31
80009e50:	1a000056 	bne	80009fb0 <draw_char+0x808>
80009e54:	e5962004 	ldr	r2, [r6, #4]
80009e58:	e1540002 	cmp	r4, r2
80009e5c:	31a00001 	movcc	r0, r1
80009e60:	23810001 	orrcs	r0, r1, #1
80009e64:	e3500000 	cmp	r0, #0
80009e68:	1a000050 	bne	80009fb0 <draw_char+0x808>
80009e6c:	e5960008 	ldr	r0, [r6, #8]
80009e70:	e15a0000 	cmp	sl, r0
80009e74:	2a00004d 	bcs	80009fb0 <draw_char+0x808>
80009e78:	e0204a92 	mla	r0, r2, sl, r4
80009e7c:	e5962000 	ldr	r2, [r6]
80009e80:	e59dc038 	ldr	ip, [sp, #56]	; 0x38
80009e84:	e7922100 	ldr	r2, [r2, r0, lsl #2]
80009e88:	e59d9028 	ldr	r9, [sp, #40]	; 0x28
80009e8c:	e1a05822 	lsr	r5, r2, #16
80009e90:	e1a0ec22 	lsr	lr, r2, #24
80009e94:	e58d001c 	str	r0, [sp, #28]
80009e98:	e20550ff 	and	r5, r5, #255	; 0xff
80009e9c:	e1a0000c 	mov	r0, ip
80009ea0:	e0000095 	mul	r0, r5, r0
80009ea4:	e58de018 	str	lr, [sp, #24]
80009ea8:	e26ee0ff 	rsb	lr, lr, #255	; 0xff
80009eac:	e009099e 	mul	r9, lr, r9
80009eb0:	e1a05000 	mov	r5, r0
80009eb4:	e20200ff 	and	r0, r2, #255	; 0xff
80009eb8:	e1a0e00c 	mov	lr, ip
80009ebc:	e58d9014 	str	r9, [sp, #20]
80009ec0:	e00c0c90 	mul	ip, r0, ip
80009ec4:	e59d903c 	ldr	r9, [sp, #60]	; 0x3c
80009ec8:	e1a02422 	lsr	r2, r2, #8
80009ecc:	e58dc010 	str	ip, [sp, #16]
80009ed0:	e20220ff 	and	r2, r2, #255	; 0xff
80009ed4:	e1a0c009 	mov	ip, r9
80009ed8:	e1a00002 	mov	r0, r2
80009edc:	e0cc2c95 	smull	r2, ip, r5, ip
80009ee0:	e1a02000 	mov	r2, r0
80009ee4:	e58dc020 	str	ip, [sp, #32]
80009ee8:	e59dc014 	ldr	ip, [sp, #20]
80009eec:	e002029e 	mul	r2, lr, r2
80009ef0:	e0ccec99 	smull	lr, ip, r9, ip
80009ef4:	e1a00002 	mov	r0, r2
80009ef8:	e1a0e00c 	mov	lr, ip
80009efc:	e59dc020 	ldr	ip, [sp, #32]
80009f00:	e1a02009 	mov	r2, r9
80009f04:	e08c9005 	add	r9, ip, r5
80009f08:	e1a0cfc5 	asr	ip, r5, #31
80009f0c:	e06cc3c9 	rsb	ip, ip, r9, asr #7
80009f10:	e58dc020 	str	ip, [sp, #32]
80009f14:	e59dc010 	ldr	ip, [sp, #16]
80009f18:	e1a05002 	mov	r5, r2
80009f1c:	e0cc9c92 	smull	r9, ip, r2, ip
80009f20:	e58d0024 	str	r0, [sp, #36]	; 0x24
80009f24:	e0c02095 	smull	r2, r0, r5, r0
80009f28:	e59d2034 	ldr	r2, [sp, #52]	; 0x34
80009f2c:	e1a05000 	mov	r5, r0
80009f30:	e59d0020 	ldr	r0, [sp, #32]
80009f34:	e1a0900c 	mov	r9, ip
80009f38:	e082c000 	add	ip, r2, r0
80009f3c:	e1a0200e 	mov	r2, lr
80009f40:	e59d0010 	ldr	r0, [sp, #16]
80009f44:	e59de014 	ldr	lr, [sp, #20]
80009f48:	e0899000 	add	r9, r9, r0
80009f4c:	e082200e 	add	r2, r2, lr
80009f50:	e1a00fc0 	asr	r0, r0, #31
80009f54:	e1a0efce 	asr	lr, lr, #31
80009f58:	e06ee3c2 	rsb	lr, lr, r2, asr #7
80009f5c:	e06003c9 	rsb	r0, r0, r9, asr #7
80009f60:	e59d2024 	ldr	r2, [sp, #36]	; 0x24
80009f64:	e59d9018 	ldr	r9, [sp, #24]
80009f68:	e0855002 	add	r5, r5, r2
80009f6c:	e089e00e 	add	lr, r9, lr
80009f70:	e1a02fc2 	asr	r2, r2, #31
80009f74:	e59d902c 	ldr	r9, [sp, #44]	; 0x2c
80009f78:	e06253c5 	rsb	r5, r2, r5, asr #7
80009f7c:	e59d2030 	ldr	r2, [sp, #48]	; 0x30
80009f80:	e20cc0ff 	and	ip, ip, #255	; 0xff
80009f84:	e1a0c80c 	lsl	ip, ip, #16
80009f88:	e0890000 	add	r0, r9, r0
80009f8c:	e18ccc0e 	orr	ip, ip, lr, lsl #24
80009f90:	e20000ff 	and	r0, r0, #255	; 0xff
80009f94:	e0822005 	add	r2, r2, r5
80009f98:	e18c0000 	orr	r0, ip, r0
80009f9c:	e20220ff 	and	r2, r2, #255	; 0xff
80009fa0:	e1802402 	orr	r2, r0, r2, lsl #8
80009fa4:	e596c000 	ldr	ip, [r6]
80009fa8:	e59d001c 	ldr	r0, [sp, #28]
80009fac:	e78c2100 	str	r2, [ip, r0, lsl #2]
80009fb0:	e2844001 	add	r4, r4, #1
80009fb4:	e1540007 	cmp	r4, r7
80009fb8:	e1a030c3 	asr	r3, r3, #1
80009fbc:	1affff9f 	bne	80009e40 <draw_char+0x698>
80009fc0:	e59d3060 	ldr	r3, [sp, #96]	; 0x60
80009fc4:	e28aa001 	add	sl, sl, #1
80009fc8:	e5932008 	ldr	r2, [r3, #8]
80009fcc:	e59d300c 	ldr	r3, [sp, #12]
80009fd0:	e063300a 	rsb	r3, r3, sl
80009fd4:	e1520003 	cmp	r2, r3
80009fd8:	8affff92 	bhi	80009e28 <draw_char+0x680>
80009fdc:	eafffdff 	b	800097e0 <draw_char+0x38>
80009fe0:	e3520000 	cmp	r2, #0
80009fe4:	0afffdfd 	beq	800097e0 <draw_char+0x38>
80009fe8:	e59d200c 	ldr	r2, [sp, #12]
80009fec:	e0855003 	add	r5, r5, r3
80009ff0:	e59d3008 	ldr	r3, [sp, #8]
80009ff4:	e59d9060 	ldr	r9, [sp, #96]	; 0x60
80009ff8:	e58d8064 	str	r8, [sp, #100]	; 0x64
80009ffc:	e1a0e002 	mov	lr, r2
8000a000:	e2837008 	add	r7, r3, #8
8000a004:	e1a06003 	mov	r6, r3
8000a008:	e1a08002 	mov	r8, r2
8000a00c:	e4d51001 	ldrb	r1, [r5], #1
8000a010:	e1a03006 	mov	r3, r6
8000a014:	e3a02080 	mov	r2, #128	; 0x80
8000a018:	e1a0cfae 	lsr	ip, lr, #31
8000a01c:	e58d5004 	str	r5, [sp, #4]
8000a020:	e1120001 	tst	r2, r1
8000a024:	03a00001 	moveq	r0, #1
8000a028:	13a00000 	movne	r0, #0
8000a02c:	e1900fa3 	orrs	r0, r0, r3, lsr #31
8000a030:	1a00000b 	bne	8000a064 <draw_char+0x8bc>
8000a034:	e59a0004 	ldr	r0, [sl, #4]
8000a038:	e0243e90 	mla	r4, r0, lr, r3
8000a03c:	e1530000 	cmp	r3, r0
8000a040:	31a0000c 	movcc	r0, ip
8000a044:	238c0001 	orrcs	r0, ip, #1
8000a048:	e3500000 	cmp	r0, #0
8000a04c:	1a000004 	bne	8000a064 <draw_char+0x8bc>
8000a050:	e59a0008 	ldr	r0, [sl, #8]
8000a054:	e15e0000 	cmp	lr, r0
8000a058:	359a0000 	ldrcc	r0, [sl]
8000a05c:	359d5064 	ldrcc	r5, [sp, #100]	; 0x64
8000a060:	37805104 	strcc	r5, [r0, r4, lsl #2]
8000a064:	e2833001 	add	r3, r3, #1
8000a068:	e1530007 	cmp	r3, r7
8000a06c:	e1a020c2 	asr	r2, r2, #1
8000a070:	1affffea 	bne	8000a020 <draw_char+0x878>
8000a074:	e5992008 	ldr	r2, [r9, #8]
8000a078:	e28ee001 	add	lr, lr, #1
8000a07c:	e068300e 	rsb	r3, r8, lr
8000a080:	e1520003 	cmp	r2, r3
8000a084:	e59d5004 	ldr	r5, [sp, #4]
8000a088:	8affffdf 	bhi	8000a00c <draw_char+0x864>
8000a08c:	eafffdd3 	b	800097e0 <draw_char+0x38>
8000a090:	80808081 	addhi	r8, r0, r1, lsl #1

8000a094 <draw_text>:
8000a094:	e1a0c00d 	mov	ip, sp
8000a098:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
8000a09c:	e24cb004 	sub	fp, ip, #4
8000a0a0:	e24dd008 	sub	sp, sp, #8
8000a0a4:	e2507000 	subs	r7, r0, #0
8000a0a8:	e1a04001 	mov	r4, r1
8000a0ac:	e1a08002 	mov	r8, r2
8000a0b0:	e1a05003 	mov	r5, r3
8000a0b4:	e99b0240 	ldmib	fp, {r6, r9}
8000a0b8:	0a00000c 	beq	8000a0f0 <draw_text+0x5c>
8000a0bc:	e5d33000 	ldrb	r3, [r3]
8000a0c0:	e3530000 	cmp	r3, #0
8000a0c4:	0a000009 	beq	8000a0f0 <draw_text+0x5c>
8000a0c8:	e1a01004 	mov	r1, r4
8000a0cc:	e1a02008 	mov	r2, r8
8000a0d0:	e88d0240 	stm	sp, {r6, r9}
8000a0d4:	e1a00007 	mov	r0, r7
8000a0d8:	ebfffdb2 	bl	800097a8 <draw_char>
8000a0dc:	e5f53001 	ldrb	r3, [r5, #1]!
8000a0e0:	e5962004 	ldr	r2, [r6, #4]
8000a0e4:	e3530000 	cmp	r3, #0
8000a0e8:	e0844002 	add	r4, r4, r2
8000a0ec:	1afffff5 	bne	8000a0c8 <draw_text+0x34>
8000a0f0:	e24bd024 	sub	sp, fp, #36	; 0x24
8000a0f4:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}

8000a0f8 <blt>:
8000a0f8:	e1a0c00d 	mov	ip, sp
8000a0fc:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000a100:	e24cb004 	sub	fp, ip, #4
8000a104:	e24dd024 	sub	sp, sp, #36	; 0x24
8000a108:	e59b4008 	ldr	r4, [fp, #8]
8000a10c:	e59b5010 	ldr	r5, [fp, #16]
8000a110:	e59b7004 	ldr	r7, [fp, #4]
8000a114:	e59b600c 	ldr	r6, [fp, #12]
8000a118:	e59be014 	ldr	lr, [fp, #20]
8000a11c:	e59bc018 	ldr	ip, [fp, #24]
8000a120:	e50b104c 	str	r1, [fp, #-76]	; 0x4c
8000a124:	e50b2048 	str	r2, [fp, #-72]	; 0x48
8000a128:	e50b3044 	str	r3, [fp, #-68]	; 0x44
8000a12c:	e1a02004 	mov	r2, r4
8000a130:	e24b104c 	sub	r1, fp, #76	; 0x4c
8000a134:	e24b303c 	sub	r3, fp, #60	; 0x3c
8000a138:	e50b5038 	str	r5, [fp, #-56]	; 0x38
8000a13c:	e50b7040 	str	r7, [fp, #-64]	; 0x40
8000a140:	e50b603c 	str	r6, [fp, #-60]	; 0x3c
8000a144:	e50be034 	str	lr, [fp, #-52]	; 0x34
8000a148:	e50bc030 	str	ip, [fp, #-48]	; 0x30
8000a14c:	e1a05000 	mov	r5, r0
8000a150:	ebfffae5 	bl	80008cec <insect>
8000a154:	e3500000 	cmp	r0, #0
8000a158:	0a00001e 	beq	8000a1d8 <blt+0xe0>
8000a15c:	e51b0048 	ldr	r0, [fp, #-72]	; 0x48
8000a160:	e51b7040 	ldr	r7, [fp, #-64]	; 0x40
8000a164:	e51b304c 	ldr	r3, [fp, #-76]	; 0x4c
8000a168:	e51be044 	ldr	lr, [fp, #-68]	; 0x44
8000a16c:	e0807007 	add	r7, r0, r7
8000a170:	e1500007 	cmp	r0, r7
8000a174:	e083e00e 	add	lr, r3, lr
8000a178:	e51b8038 	ldr	r8, [fp, #-56]	; 0x38
8000a17c:	aa000015 	bge	8000a1d8 <blt+0xe0>
8000a180:	e0608008 	rsb	r8, r0, r8
8000a184:	e15e0003 	cmp	lr, r3
8000a188:	e51bc03c 	ldr	ip, [fp, #-60]	; 0x3c
8000a18c:	da00000d 	ble	8000a1c8 <blt+0xd0>
8000a190:	e5949000 	ldr	r9, [r4]
8000a194:	e5956000 	ldr	r6, [r5]
8000a198:	e063c00c 	rsb	ip, r3, ip
8000a19c:	e088a000 	add	sl, r8, r0
8000a1a0:	e5951004 	ldr	r1, [r5, #4]
8000a1a4:	e5942004 	ldr	r2, [r4, #4]
8000a1a8:	e0213190 	mla	r1, r0, r1, r3
8000a1ac:	e022329a 	mla	r2, sl, r2, r3
8000a1b0:	e7961101 	ldr	r1, [r6, r1, lsl #2]
8000a1b4:	e2833001 	add	r3, r3, #1
8000a1b8:	e082200c 	add	r2, r2, ip
8000a1bc:	e153000e 	cmp	r3, lr
8000a1c0:	e7891102 	str	r1, [r9, r2, lsl #2]
8000a1c4:	1afffff5 	bne	8000a1a0 <blt+0xa8>
8000a1c8:	e2800001 	add	r0, r0, #1
8000a1cc:	e1500007 	cmp	r0, r7
8000a1d0:	151b304c 	ldrne	r3, [fp, #-76]	; 0x4c
8000a1d4:	1affffea 	bne	8000a184 <blt+0x8c>
8000a1d8:	e24bd028 	sub	sp, fp, #40	; 0x28
8000a1dc:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}

8000a1e0 <blt_alpha>:
8000a1e0:	e1a0c00d 	mov	ip, sp
8000a1e4:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000a1e8:	e24cb004 	sub	fp, ip, #4
8000a1ec:	e24dd054 	sub	sp, sp, #84	; 0x54
8000a1f0:	e59bc018 	ldr	ip, [fp, #24]
8000a1f4:	e59b6004 	ldr	r6, [fp, #4]
8000a1f8:	e59b500c 	ldr	r5, [fp, #12]
8000a1fc:	e59b4010 	ldr	r4, [fp, #16]
8000a200:	e59be014 	ldr	lr, [fp, #20]
8000a204:	e50bc030 	str	ip, [fp, #-48]	; 0x30
8000a208:	e5dbc01c 	ldrb	ip, [fp, #28]
8000a20c:	e50b104c 	str	r1, [fp, #-76]	; 0x4c
8000a210:	e50b2048 	str	r2, [fp, #-72]	; 0x48
8000a214:	e50b3044 	str	r3, [fp, #-68]	; 0x44
8000a218:	e59b2008 	ldr	r2, [fp, #8]
8000a21c:	e24b104c 	sub	r1, fp, #76	; 0x4c
8000a220:	e24b303c 	sub	r3, fp, #60	; 0x3c
8000a224:	e50b6040 	str	r6, [fp, #-64]	; 0x40
8000a228:	e50b503c 	str	r5, [fp, #-60]	; 0x3c
8000a22c:	e50b4038 	str	r4, [fp, #-56]	; 0x38
8000a230:	e50be034 	str	lr, [fp, #-52]	; 0x34
8000a234:	e50b006c 	str	r0, [fp, #-108]	; 0x6c
8000a238:	e50bc070 	str	ip, [fp, #-112]	; 0x70
8000a23c:	ebfffaaa 	bl	80008cec <insect>
8000a240:	e3500000 	cmp	r0, #0
8000a244:	0a00007e 	beq	8000a444 <blt_alpha+0x264>
8000a248:	e51b3048 	ldr	r3, [fp, #-72]	; 0x48
8000a24c:	e51b1040 	ldr	r1, [fp, #-64]	; 0x40
8000a250:	e51b004c 	ldr	r0, [fp, #-76]	; 0x4c
8000a254:	e51b2044 	ldr	r2, [fp, #-68]	; 0x44
8000a258:	e0831001 	add	r1, r3, r1
8000a25c:	e0802002 	add	r2, r0, r2
8000a260:	e1530001 	cmp	r3, r1
8000a264:	e50b2060 	str	r2, [fp, #-96]	; 0x60
8000a268:	e50b1074 	str	r1, [fp, #-116]	; 0x74
8000a26c:	e51b2038 	ldr	r2, [fp, #-56]	; 0x38
8000a270:	aa000073 	bge	8000a444 <blt_alpha+0x264>
8000a274:	e59f81d0 	ldr	r8, [pc, #464]	; 8000a44c <blt_alpha+0x26c>
8000a278:	e50b305c 	str	r3, [fp, #-92]	; 0x5c
8000a27c:	e0633002 	rsb	r3, r3, r2
8000a280:	e1a02000 	mov	r2, r0
8000a284:	e50b3078 	str	r3, [fp, #-120]	; 0x78
8000a288:	e51b3060 	ldr	r3, [fp, #-96]	; 0x60
8000a28c:	e1530002 	cmp	r3, r2
8000a290:	e51b303c 	ldr	r3, [fp, #-60]	; 0x3c
8000a294:	da000063 	ble	8000a428 <blt_alpha+0x248>
8000a298:	e0623003 	rsb	r3, r2, r3
8000a29c:	e50b3068 	str	r3, [fp, #-104]	; 0x68
8000a2a0:	e51b105c 	ldr	r1, [fp, #-92]	; 0x5c
8000a2a4:	e51b3078 	ldr	r3, [fp, #-120]	; 0x78
8000a2a8:	e1a05002 	mov	r5, r2
8000a2ac:	e0833001 	add	r3, r3, r1
8000a2b0:	e50b3064 	str	r3, [fp, #-100]	; 0x64
8000a2b4:	e51b206c 	ldr	r2, [fp, #-108]	; 0x6c
8000a2b8:	e51b105c 	ldr	r1, [fp, #-92]	; 0x5c
8000a2bc:	e5923004 	ldr	r3, [r2, #4]
8000a2c0:	e5922000 	ldr	r2, [r2]
8000a2c4:	e0215193 	mla	r1, r3, r1, r5
8000a2c8:	e7920101 	ldr	r0, [r2, r1, lsl #2]
8000a2cc:	ebfffad8 	bl	80008e34 <argb_int>
8000a2d0:	e59b3008 	ldr	r3, [fp, #8]
8000a2d4:	e51b4064 	ldr	r4, [fp, #-100]	; 0x64
8000a2d8:	e5931004 	ldr	r1, [r3, #4]
8000a2dc:	e51b3070 	ldr	r3, [fp, #-112]	; 0x70
8000a2e0:	e1a02c20 	lsr	r2, r0, #24
8000a2e4:	e0030392 	mul	r3, r2, r3
8000a2e8:	e20070ff 	and	r7, r0, #255	; 0xff
8000a2ec:	e1a02003 	mov	r2, r3
8000a2f0:	e51b3068 	ldr	r3, [fp, #-104]	; 0x68
8000a2f4:	e1a0c820 	lsr	ip, r0, #16
8000a2f8:	e0853003 	add	r3, r5, r3
8000a2fc:	e0243491 	mla	r4, r1, r4, r3
8000a300:	e59b3008 	ldr	r3, [fp, #8]
8000a304:	e0821298 	umull	r1, r2, r8, r2
8000a308:	e5933000 	ldr	r3, [r3]
8000a30c:	e1a023a2 	lsr	r2, r2, #7
8000a310:	e50b3058 	str	r3, [fp, #-88]	; 0x58
8000a314:	e7933104 	ldr	r3, [r3, r4, lsl #2]
8000a318:	e20220ff 	and	r2, r2, #255	; 0xff
8000a31c:	e1a06823 	lsr	r6, r3, #16
8000a320:	e26290ff 	rsb	r9, r2, #255	; 0xff
8000a324:	e20660ff 	and	r6, r6, #255	; 0xff
8000a328:	e0060699 	mul	r6, r9, r6
8000a32c:	e1a01c23 	lsr	r1, r3, #24
8000a330:	e0070792 	mul	r7, r2, r7
8000a334:	e1a00420 	lsr	r0, r0, #8
8000a338:	e50b1050 	str	r1, [fp, #-80]	; 0x50
8000a33c:	e261e0ff 	rsb	lr, r1, #255	; 0xff
8000a340:	e20cc0ff 	and	ip, ip, #255	; 0xff
8000a344:	e0ca1698 	smull	r1, sl, r8, r6
8000a348:	e20000ff 	and	r0, r0, #255	; 0xff
8000a34c:	e1a01423 	lsr	r1, r3, #8
8000a350:	e00c0c92 	mul	ip, r2, ip
8000a354:	e00e0e92 	mul	lr, r2, lr
8000a358:	e0000092 	mul	r0, r2, r0
8000a35c:	e20320ff 	and	r2, r3, #255	; 0xff
8000a360:	e20130ff 	and	r3, r1, #255	; 0xff
8000a364:	e0020299 	mul	r2, r9, r2
8000a368:	e0030399 	mul	r3, r9, r3
8000a36c:	e0c91798 	smull	r1, r9, r8, r7
8000a370:	e08aa006 	add	sl, sl, r6
8000a374:	e0899007 	add	r9, r9, r7
8000a378:	e1a06fc6 	asr	r6, r6, #31
8000a37c:	e1a07fc7 	asr	r7, r7, #31
8000a380:	e50b4054 	str	r4, [fp, #-84]	; 0x54
8000a384:	e06663ca 	rsb	r6, r6, sl, asr #7
8000a388:	e0ca1e98 	smull	r1, sl, r8, lr
8000a38c:	e06713c9 	rsb	r1, r7, r9, asr #7
8000a390:	e0c97298 	smull	r7, r9, r8, r2
8000a394:	e0c74c98 	smull	r4, r7, r8, ip
8000a398:	e08aa00e 	add	sl, sl, lr
8000a39c:	e087700c 	add	r7, r7, ip
8000a3a0:	e1a0cfcc 	asr	ip, ip, #31
8000a3a4:	e06c73c7 	rsb	r7, ip, r7, asr #7
8000a3a8:	e51bc050 	ldr	ip, [fp, #-80]	; 0x50
8000a3ac:	e0861001 	add	r1, r6, r1
8000a3b0:	e0899002 	add	r9, r9, r2
8000a3b4:	e1a0efce 	asr	lr, lr, #31
8000a3b8:	e1a02fc2 	asr	r2, r2, #31
8000a3bc:	e0c64398 	smull	r4, r6, r8, r3
8000a3c0:	e06ee3ca 	rsb	lr, lr, sl, asr #7
8000a3c4:	e20110ff 	and	r1, r1, #255	; 0xff
8000a3c8:	e0ca4098 	smull	r4, sl, r8, r0
8000a3cc:	e06223c9 	rsb	r2, r2, r9, asr #7
8000a3d0:	e1a01801 	lsl	r1, r1, #16
8000a3d4:	e08ce00e 	add	lr, ip, lr
8000a3d8:	e0822007 	add	r2, r2, r7
8000a3dc:	e20220ff 	and	r2, r2, #255	; 0xff
8000a3e0:	e1811c0e 	orr	r1, r1, lr, lsl #24
8000a3e4:	e1811002 	orr	r1, r1, r2
8000a3e8:	e51b2060 	ldr	r2, [fp, #-96]	; 0x60
8000a3ec:	e0866003 	add	r6, r6, r3
8000a3f0:	e08aa000 	add	sl, sl, r0
8000a3f4:	e1a03fc3 	asr	r3, r3, #31
8000a3f8:	e1a00fc0 	asr	r0, r0, #31
8000a3fc:	e06363c6 	rsb	r6, r3, r6, asr #7
8000a400:	e06003ca 	rsb	r0, r0, sl, asr #7
8000a404:	e2855001 	add	r5, r5, #1
8000a408:	e0866000 	add	r6, r6, r0
8000a40c:	e1550002 	cmp	r5, r2
8000a410:	e51b3058 	ldr	r3, [fp, #-88]	; 0x58
8000a414:	e51b2054 	ldr	r2, [fp, #-84]	; 0x54
8000a418:	e20660ff 	and	r6, r6, #255	; 0xff
8000a41c:	e1811406 	orr	r1, r1, r6, lsl #8
8000a420:	e7831102 	str	r1, [r3, r2, lsl #2]
8000a424:	1affffa2 	bne	8000a2b4 <blt_alpha+0xd4>
8000a428:	e51b305c 	ldr	r3, [fp, #-92]	; 0x5c
8000a42c:	e51b2074 	ldr	r2, [fp, #-116]	; 0x74
8000a430:	e2833001 	add	r3, r3, #1
8000a434:	e1530002 	cmp	r3, r2
8000a438:	e50b305c 	str	r3, [fp, #-92]	; 0x5c
8000a43c:	151b204c 	ldrne	r2, [fp, #-76]	; 0x4c
8000a440:	1affff90 	bne	8000a288 <blt_alpha+0xa8>
8000a444:	e24bd028 	sub	sp, fp, #40	; 0x28
8000a448:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000a44c:	80808081 	addhi	r8, r0, r1, lsl #1

8000a450 <check_in_rect>:
8000a450:	e5923000 	ldr	r3, [r2]
8000a454:	e1530000 	cmp	r3, r0
8000a458:	ca00000c 	bgt	8000a490 <check_in_rect+0x40>
8000a45c:	e592c008 	ldr	ip, [r2, #8]
8000a460:	e083300c 	add	r3, r3, ip
8000a464:	e1500003 	cmp	r0, r3
8000a468:	aa000008 	bge	8000a490 <check_in_rect+0x40>
8000a46c:	e5923004 	ldr	r3, [r2, #4]
8000a470:	e1530001 	cmp	r3, r1
8000a474:	ca000005 	bgt	8000a490 <check_in_rect+0x40>
8000a478:	e592000c 	ldr	r0, [r2, #12]
8000a47c:	e0830000 	add	r0, r3, r0
8000a480:	e1510000 	cmp	r1, r0
8000a484:	b3a00000 	movlt	r0, #0
8000a488:	a3e00000 	mvnge	r0, #0
8000a48c:	e12fff1e 	bx	lr
8000a490:	e3e00000 	mvn	r0, #0
8000a494:	e12fff1e 	bx	lr

8000a498 <get_text_size>:
8000a498:	e1a0c00d 	mov	ip, sp
8000a49c:	e3520000 	cmp	r2, #0
8000a4a0:	13510000 	cmpne	r1, #0
8000a4a4:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000a4a8:	e24cb004 	sub	fp, ip, #4
8000a4ac:	03a04001 	moveq	r4, #1
8000a4b0:	13a04000 	movne	r4, #0
8000a4b4:	e1a06001 	mov	r6, r1
8000a4b8:	e1a05002 	mov	r5, r2
8000a4bc:	0a000008 	beq	8000a4e4 <get_text_size+0x4c>
8000a4c0:	eb0002ec 	bl	8000b078 <strlen>
8000a4c4:	e596c004 	ldr	ip, [r6, #4]
8000a4c8:	e5962008 	ldr	r2, [r6, #8]
8000a4cc:	e1a03004 	mov	r3, r4
8000a4d0:	e5852004 	str	r2, [r5, #4]
8000a4d4:	e000009c 	mul	r0, ip, r0
8000a4d8:	e5850000 	str	r0, [r5]
8000a4dc:	e1a00003 	mov	r0, r3
8000a4e0:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000a4e4:	e3e03000 	mvn	r3, #0
8000a4e8:	eafffffb 	b	8000a4dc <get_text_size+0x44>

8000a4ec <dup16>:
8000a4ec:	e0030392 	mul	r3, r2, r3
8000a4f0:	e92d4010 	push	{r4, lr}
8000a4f4:	e3530000 	cmp	r3, #0
8000a4f8:	d8bd8010 	pople	{r4, pc}
8000a4fc:	e0803083 	add	r3, r0, r3, lsl #1
8000a500:	e2411004 	sub	r1, r1, #4
8000a504:	e5b1c004 	ldr	ip, [r1, #4]!
8000a508:	e1a0e40c 	lsl	lr, ip, #8
8000a50c:	e1a0440c 	lsl	r4, ip, #8
8000a510:	e20eeb3e 	and	lr, lr, #63488	; 0xf800
8000a514:	e1a0c2ac 	lsr	ip, ip, #5
8000a518:	e18eeda4 	orr	lr, lr, r4, lsr #27
8000a51c:	e20ccd1f 	and	ip, ip, #1984	; 0x7c0
8000a520:	e18ec00c 	orr	ip, lr, ip
8000a524:	e0c0c0b2 	strh	ip, [r0], #2
8000a528:	e1500003 	cmp	r0, r3
8000a52c:	1afffff4 	bne	8000a504 <dup16+0x18>
8000a530:	e8bd8010 	pop	{r4, pc}

8000a534 <font_by_name>:
8000a534:	e1a0c00d 	mov	ip, sp
8000a538:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000a53c:	e59f3068 	ldr	r3, [pc, #104]	; 8000a5ac <font_by_name+0x78>
8000a540:	e1a05000 	mov	r5, r0
8000a544:	e08f3003 	add	r3, pc, r3
8000a548:	e5930000 	ldr	r0, [r3]
8000a54c:	e24cb004 	sub	fp, ip, #4
8000a550:	e5d03000 	ldrb	r3, [r0]
8000a554:	e3530000 	cmp	r3, #0
8000a558:	0a000011 	beq	8000a5a4 <font_by_name+0x70>
8000a55c:	e59f604c 	ldr	r6, [pc, #76]	; 8000a5b0 <font_by_name+0x7c>
8000a560:	e3a04000 	mov	r4, #0
8000a564:	e08f6006 	add	r6, pc, r6
8000a568:	ea000004 	b	8000a580 <font_by_name+0x4c>
8000a56c:	e2844001 	add	r4, r4, #1
8000a570:	e7960184 	ldr	r0, [r6, r4, lsl #3]
8000a574:	e5d03000 	ldrb	r3, [r0]
8000a578:	e3530000 	cmp	r3, #0
8000a57c:	0a000008 	beq	8000a5a4 <font_by_name+0x70>
8000a580:	e1a01005 	mov	r1, r5
8000a584:	eb00021a 	bl	8000adf4 <strcmp>
8000a588:	e3500000 	cmp	r0, #0
8000a58c:	1afffff6 	bne	8000a56c <font_by_name+0x38>
8000a590:	e59f301c 	ldr	r3, [pc, #28]	; 8000a5b4 <font_by_name+0x80>
8000a594:	e08f3003 	add	r3, pc, r3
8000a598:	e0834184 	add	r4, r3, r4, lsl #3
8000a59c:	e5940004 	ldr	r0, [r4, #4]
8000a5a0:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000a5a4:	e3a00000 	mov	r0, #0
8000a5a8:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000a5ac:	00009ab4 			; <UNDEFINED> instruction: 0x00009ab4
8000a5b0:	00009a94 	muleq	r0, r4, sl
8000a5b4:	00009a64 	andeq	r9, r0, r4, ror #20

8000a5b8 <font_by_index>:
8000a5b8:	e3500000 	cmp	r0, #0
8000a5bc:	1a000002 	bne	8000a5cc <font_by_index+0x14>
8000a5c0:	e59f000c 	ldr	r0, [pc, #12]	; 8000a5d4 <font_by_index+0x1c>
8000a5c4:	e08f0000 	add	r0, pc, r0
8000a5c8:	e12fff1e 	bx	lr
8000a5cc:	e3a00000 	mov	r0, #0
8000a5d0:	e12fff1e 	bx	lr
8000a5d4:	00009a34 	andeq	r9, r0, r4, lsr sl

8000a5d8 <console_init>:
8000a5d8:	e1a0c00d 	mov	ip, sp
8000a5dc:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000a5e0:	e1a05000 	mov	r5, r0
8000a5e4:	e3a04000 	mov	r4, #0
8000a5e8:	e3a000ff 	mov	r0, #255	; 0xff
8000a5ec:	e24cb004 	sub	fp, ip, #4
8000a5f0:	e5854000 	str	r4, [r5]
8000a5f4:	e1a01000 	mov	r1, r0
8000a5f8:	e1a02000 	mov	r2, r0
8000a5fc:	e1a03000 	mov	r3, r0
8000a600:	ebfffa03 	bl	80008e14 <argb>
8000a604:	e1a01004 	mov	r1, r4
8000a608:	e1a02004 	mov	r2, r4
8000a60c:	e1a03004 	mov	r3, r4
8000a610:	e5850008 	str	r0, [r5, #8]
8000a614:	e3a000ff 	mov	r0, #255	; 0xff
8000a618:	ebfff9fd 	bl	80008e14 <argb>
8000a61c:	e5850004 	str	r0, [r5, #4]
8000a620:	e59f0020 	ldr	r0, [pc, #32]	; 8000a648 <console_init+0x70>
8000a624:	e08f0000 	add	r0, pc, r0
8000a628:	ebffffc1 	bl	8000a534 <font_by_name>
8000a62c:	e1a01004 	mov	r1, r4
8000a630:	e3a0201c 	mov	r2, #28
8000a634:	e585000c 	str	r0, [r5, #12]
8000a638:	e2850010 	add	r0, r5, #16
8000a63c:	eb00018b 	bl	8000ac70 <memset>
8000a640:	e1a00004 	mov	r0, r4
8000a644:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000a648:	0000797c 	andeq	r7, r0, ip, ror r9

8000a64c <console_close>:
8000a64c:	e1a0c00d 	mov	ip, sp
8000a650:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000a654:	e1a04000 	mov	r4, r0
8000a658:	e24cb004 	sub	fp, ip, #4
8000a65c:	e5900024 	ldr	r0, [r0, #36]	; 0x24
8000a660:	eb000cd3 	bl	8000d9b4 <kfree>
8000a664:	e3a03000 	mov	r3, #0
8000a668:	e5843028 	str	r3, [r4, #40]	; 0x28
8000a66c:	e5843024 	str	r3, [r4, #36]	; 0x24
8000a670:	e5843000 	str	r3, [r4]
8000a674:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}

8000a678 <console_refresh>:
8000a678:	e1a0c00d 	mov	ip, sp
8000a67c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000a680:	e24cb004 	sub	fp, ip, #4
8000a684:	e24dd008 	sub	sp, sp, #8
8000a688:	e1a04000 	mov	r4, r0
8000a68c:	e8900003 	ldm	r0, {r0, r1}
8000a690:	ebfffa32 	bl	80008f60 <clear>
8000a694:	e5940028 	ldr	r0, [r4, #40]	; 0x28
8000a698:	e3500000 	cmp	r0, #0
8000a69c:	0a00001e 	beq	8000a71c <console_refresh+0xa4>
8000a6a0:	e594101c 	ldr	r1, [r4, #28]
8000a6a4:	e3a07000 	mov	r7, #0
8000a6a8:	e1a05007 	mov	r5, r7
8000a6ac:	e1a06007 	mov	r6, r7
8000a6b0:	e5943010 	ldr	r3, [r4, #16]
8000a6b4:	e5942020 	ldr	r2, [r4, #32]
8000a6b8:	e0236391 	mla	r3, r1, r3, r6
8000a6bc:	e2866001 	add	r6, r6, #1
8000a6c0:	e1530002 	cmp	r3, r2
8000a6c4:	20623003 	rsbcs	r3, r2, r3
8000a6c8:	e5942024 	ldr	r2, [r4, #36]	; 0x24
8000a6cc:	e7d22003 	ldrb	r2, [r2, r3]
8000a6d0:	e3520020 	cmp	r2, #32
8000a6d4:	e1a03002 	mov	r3, r2
8000a6d8:	0a000009 	beq	8000a704 <console_refresh+0x8c>
8000a6dc:	e594c00c 	ldr	ip, [r4, #12]
8000a6e0:	e594e008 	ldr	lr, [r4, #8]
8000a6e4:	e99c0006 	ldmib	ip, {r1, r2}
8000a6e8:	e5940000 	ldr	r0, [r4]
8000a6ec:	e0010195 	mul	r1, r5, r1
8000a6f0:	e0020297 	mul	r2, r7, r2
8000a6f4:	e88d5000 	stm	sp, {ip, lr}
8000a6f8:	ebfffc2a 	bl	800097a8 <draw_char>
8000a6fc:	e594101c 	ldr	r1, [r4, #28]
8000a700:	e5940028 	ldr	r0, [r4, #40]	; 0x28
8000a704:	e2855001 	add	r5, r5, #1
8000a708:	e1550001 	cmp	r5, r1
8000a70c:	22877001 	addcs	r7, r7, #1
8000a710:	23a05000 	movcs	r5, #0
8000a714:	e1500006 	cmp	r0, r6
8000a718:	8affffe4 	bhi	8000a6b0 <console_refresh+0x38>
8000a71c:	e24bd01c 	sub	sp, fp, #28
8000a720:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}

8000a724 <console_put_char>:
8000a724:	e1a0c00d 	mov	ip, sp
8000a728:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000a72c:	e351000d 	cmp	r1, #13
8000a730:	e24cb004 	sub	fp, ip, #4
8000a734:	e24dd010 	sub	sp, sp, #16
8000a738:	e1a04000 	mov	r4, r0
8000a73c:	0a000039 	beq	8000a828 <console_put_char+0x104>
8000a740:	e3510008 	cmp	r1, #8
8000a744:	0a00002b 	beq	8000a7f8 <console_put_char+0xd4>
8000a748:	e3510009 	cmp	r1, #9
8000a74c:	0a000031 	beq	8000a818 <console_put_char+0xf4>
8000a750:	e351000a 	cmp	r1, #10
8000a754:	0a000034 	beq	8000a82c <console_put_char+0x108>
8000a758:	e5940014 	ldr	r0, [r4, #20]
8000a75c:	e594301c 	ldr	r3, [r4, #28]
8000a760:	e5942028 	ldr	r2, [r4, #40]	; 0x28
8000a764:	e00e0390 	mul	lr, r0, r3
8000a768:	e282c001 	add	ip, r2, #1
8000a76c:	e06ec00c 	rsb	ip, lr, ip
8000a770:	e594e018 	ldr	lr, [r4, #24]
8000a774:	e153000c 	cmp	r3, ip
8000a778:	02800001 	addeq	r0, r0, #1
8000a77c:	05840014 	streq	r0, [r4, #20]
8000a780:	e15e0000 	cmp	lr, r0
8000a784:	3a000046 	bcc	8000a8a4 <console_put_char+0x180>
8000a788:	e5940010 	ldr	r0, [r4, #16]
8000a78c:	e594c020 	ldr	ip, [r4, #32]
8000a790:	e0222390 	mla	r2, r0, r3, r2
8000a794:	e5940024 	ldr	r0, [r4, #36]	; 0x24
8000a798:	e152000c 	cmp	r2, ip
8000a79c:	206c2002 	rsbcs	r2, ip, r2
8000a7a0:	e7c01002 	strb	r1, [r0, r2]
8000a7a4:	e1a03001 	mov	r3, r1
8000a7a8:	e5942014 	ldr	r2, [r4, #20]
8000a7ac:	e594101c 	ldr	r1, [r4, #28]
8000a7b0:	e594c00c 	ldr	ip, [r4, #12]
8000a7b4:	e594e028 	ldr	lr, [r4, #40]	; 0x28
8000a7b8:	e0010192 	mul	r1, r2, r1
8000a7bc:	e59c7004 	ldr	r7, [ip, #4]
8000a7c0:	e59c6008 	ldr	r6, [ip, #8]
8000a7c4:	e5945008 	ldr	r5, [r4, #8]
8000a7c8:	e061100e 	rsb	r1, r1, lr
8000a7cc:	e5940000 	ldr	r0, [r4]
8000a7d0:	e0010197 	mul	r1, r7, r1
8000a7d4:	e0020296 	mul	r2, r6, r2
8000a7d8:	e58dc000 	str	ip, [sp]
8000a7dc:	e58d5004 	str	r5, [sp, #4]
8000a7e0:	ebfffbf0 	bl	800097a8 <draw_char>
8000a7e4:	e5943028 	ldr	r3, [r4, #40]	; 0x28
8000a7e8:	e2833001 	add	r3, r3, #1
8000a7ec:	e5843028 	str	r3, [r4, #40]	; 0x28
8000a7f0:	e24bd01c 	sub	sp, fp, #28
8000a7f4:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000a7f8:	e5903028 	ldr	r3, [r0, #40]	; 0x28
8000a7fc:	e3530000 	cmp	r3, #0
8000a800:	0afffffa 	beq	8000a7f0 <console_put_char+0xcc>
8000a804:	e2433001 	sub	r3, r3, #1
8000a808:	e5803028 	str	r3, [r0, #40]	; 0x28
8000a80c:	e24bd01c 	sub	sp, fp, #28
8000a810:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
8000a814:	eaffff97 	b	8000a678 <console_refresh>
8000a818:	e3a01020 	mov	r1, #32
8000a81c:	ebffffc0 	bl	8000a724 <console_put_char>
8000a820:	e3a01020 	mov	r1, #32
8000a824:	eaffffcb 	b	8000a758 <console_put_char+0x34>
8000a828:	e3a0100a 	mov	r1, #10
8000a82c:	e594301c 	ldr	r3, [r4, #28]
8000a830:	e594c014 	ldr	ip, [r4, #20]
8000a834:	e594e028 	ldr	lr, [r4, #40]	; 0x28
8000a838:	e000039c 	mul	r0, ip, r3
8000a83c:	e1a0200e 	mov	r2, lr
8000a840:	e060000e 	rsb	r0, r0, lr
8000a844:	e1530000 	cmp	r3, r0
8000a848:	9a000010 	bls	8000a890 <console_put_char+0x16c>
8000a84c:	e3a05020 	mov	r5, #32
8000a850:	e5942010 	ldr	r2, [r4, #16]
8000a854:	e594c020 	ldr	ip, [r4, #32]
8000a858:	e023e392 	mla	r3, r2, r3, lr
8000a85c:	e5942024 	ldr	r2, [r4, #36]	; 0x24
8000a860:	e153000c 	cmp	r3, ip
8000a864:	206c3003 	rsbcs	r3, ip, r3
8000a868:	e7c25003 	strb	r5, [r2, r3]
8000a86c:	e5942028 	ldr	r2, [r4, #40]	; 0x28
8000a870:	e594301c 	ldr	r3, [r4, #28]
8000a874:	e2800001 	add	r0, r0, #1
8000a878:	e2822001 	add	r2, r2, #1
8000a87c:	e1530000 	cmp	r3, r0
8000a880:	e5842028 	str	r2, [r4, #40]	; 0x28
8000a884:	e1a0e002 	mov	lr, r2
8000a888:	8afffff0 	bhi	8000a850 <console_put_char+0x12c>
8000a88c:	e594c014 	ldr	ip, [r4, #20]
8000a890:	e594e018 	ldr	lr, [r4, #24]
8000a894:	e28c0001 	add	r0, ip, #1
8000a898:	e150000e 	cmp	r0, lr
8000a89c:	e5840014 	str	r0, [r4, #20]
8000a8a0:	9affffd2 	bls	8000a7f0 <console_put_char+0xcc>
8000a8a4:	e594c010 	ldr	ip, [r4, #16]
8000a8a8:	e2400001 	sub	r0, r0, #1
8000a8ac:	e28cc001 	add	ip, ip, #1
8000a8b0:	e15c000e 	cmp	ip, lr
8000a8b4:	e5840014 	str	r0, [r4, #20]
8000a8b8:	e0632002 	rsb	r2, r3, r2
8000a8bc:	23a00000 	movcs	r0, #0
8000a8c0:	25840010 	strcs	r0, [r4, #16]
8000a8c4:	3584c010 	strcc	ip, [r4, #16]
8000a8c8:	e5842028 	str	r2, [r4, #40]	; 0x28
8000a8cc:	e1a00004 	mov	r0, r4
8000a8d0:	e50b1020 	str	r1, [fp, #-32]
8000a8d4:	ebffff67 	bl	8000a678 <console_refresh>
8000a8d8:	e51b1020 	ldr	r1, [fp, #-32]
8000a8dc:	e351000a 	cmp	r1, #10
8000a8e0:	0affffc2 	beq	8000a7f0 <console_put_char+0xcc>
8000a8e4:	e5942028 	ldr	r2, [r4, #40]	; 0x28
8000a8e8:	e594301c 	ldr	r3, [r4, #28]
8000a8ec:	eaffffa5 	b	8000a788 <console_put_char+0x64>

8000a8f0 <console_reset>:
8000a8f0:	e1a0c00d 	mov	ip, sp
8000a8f4:	e92ddff8 	push	{r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000a8f8:	e5903000 	ldr	r3, [r0]
8000a8fc:	e24cb004 	sub	fp, ip, #4
8000a900:	e3530000 	cmp	r3, #0
8000a904:	e1a0a000 	mov	sl, r0
8000a908:	0a000054 	beq	8000aa60 <console_reset+0x170>
8000a90c:	e5906020 	ldr	r6, [r0, #32]
8000a910:	e5907028 	ldr	r7, [r0, #40]	; 0x28
8000a914:	e1a00006 	mov	r0, r6
8000a918:	e59a801c 	ldr	r8, [sl, #28]
8000a91c:	e59a4010 	ldr	r4, [sl, #16]
8000a920:	eb000c10 	bl	8000d968 <kmalloc>
8000a924:	e1a02006 	mov	r2, r6
8000a928:	e59a1024 	ldr	r1, [sl, #36]	; 0x24
8000a92c:	e1a09000 	mov	r9, r0
8000a930:	eb0000b2 	bl	8000ac00 <memcpy>
8000a934:	e59a1000 	ldr	r1, [sl]
8000a938:	e59a200c 	ldr	r2, [sl, #12]
8000a93c:	e3a03000 	mov	r3, #0
8000a940:	e5910004 	ldr	r0, [r1, #4]
8000a944:	e5921004 	ldr	r1, [r2, #4]
8000a948:	e58a3028 	str	r3, [sl, #40]	; 0x28
8000a94c:	e58a3010 	str	r3, [sl, #16]
8000a950:	e58a3014 	str	r3, [sl, #20]
8000a954:	eb000626 	bl	8000c1f4 <div_u32>
8000a958:	e59a3000 	ldr	r3, [sl]
8000a95c:	e59a200c 	ldr	r2, [sl, #12]
8000a960:	e5921008 	ldr	r1, [r2, #8]
8000a964:	e2400001 	sub	r0, r0, #1
8000a968:	e58a001c 	str	r0, [sl, #28]
8000a96c:	e5930008 	ldr	r0, [r3, #8]
8000a970:	eb00061f 	bl	8000c1f4 <div_u32>
8000a974:	e59a3000 	ldr	r3, [sl]
8000a978:	e59a200c 	ldr	r2, [sl, #12]
8000a97c:	e5921008 	ldr	r1, [r2, #8]
8000a980:	e2400001 	sub	r0, r0, #1
8000a984:	e58a0018 	str	r0, [sl, #24]
8000a988:	e5930008 	ldr	r0, [r3, #8]
8000a98c:	eb0006a6 	bl	8000c42c <mod_u32>
8000a990:	e59a5018 	ldr	r5, [sl, #24]
8000a994:	e59a301c 	ldr	r3, [sl, #28]
8000a998:	e3500000 	cmp	r0, #0
8000a99c:	12455001 	subne	r5, r5, #1
8000a9a0:	e59a0024 	ldr	r0, [sl, #36]	; 0x24
8000a9a4:	158a5018 	strne	r5, [sl, #24]
8000a9a8:	e0050593 	mul	r5, r3, r5
8000a9ac:	e3500000 	cmp	r0, #0
8000a9b0:	e58a5020 	str	r5, [sl, #32]
8000a9b4:	0a000000 	beq	8000a9bc <console_reset+0xcc>
8000a9b8:	eb000bfd 	bl	8000d9b4 <kfree>
8000a9bc:	e1a00005 	mov	r0, r5
8000a9c0:	eb000be8 	bl	8000d968 <kmalloc>
8000a9c4:	e1a02005 	mov	r2, r5
8000a9c8:	e3a01000 	mov	r1, #0
8000a9cc:	e58a0024 	str	r0, [sl, #36]	; 0x24
8000a9d0:	eb0000a6 	bl	8000ac70 <memset>
8000a9d4:	e89a0003 	ldm	sl, {r0, r1}
8000a9d8:	ebfff960 	bl	80008f60 <clear>
8000a9dc:	e1570008 	cmp	r7, r8
8000a9e0:	c3540000 	cmpgt	r4, #0
8000a9e4:	c0687007 	rsbgt	r7, r8, r7
8000a9e8:	c2844001 	addgt	r4, r4, #1
8000a9ec:	e3570000 	cmp	r7, #0
8000a9f0:	c0040498 	mulgt	r4, r8, r4
8000a9f4:	c3a05000 	movgt	r5, #0
8000a9f8:	ca000002 	bgt	8000aa08 <console_reset+0x118>
8000a9fc:	ea000013 	b	8000aa50 <console_reset+0x160>
8000aa00:	e1550007 	cmp	r5, r7
8000aa04:	0a000011 	beq	8000aa50 <console_reset+0x160>
8000aa08:	e1560004 	cmp	r6, r4
8000aa0c:	e1a03004 	mov	r3, r4
8000aa10:	d0663004 	rsble	r3, r6, r4
8000aa14:	e7d91003 	ldrb	r1, [r9, r3]
8000aa18:	e1a0000a 	mov	r0, sl
8000aa1c:	e2855001 	add	r5, r5, #1
8000aa20:	ebffff3f 	bl	8000a724 <console_put_char>
8000aa24:	e1a00005 	mov	r0, r5
8000aa28:	e1a01008 	mov	r1, r8
8000aa2c:	eb00067e 	bl	8000c42c <mod_u32>
8000aa30:	e2844001 	add	r4, r4, #1
8000aa34:	e3500000 	cmp	r0, #0
8000aa38:	1afffff0 	bne	8000aa00 <console_reset+0x110>
8000aa3c:	e1a0000a 	mov	r0, sl
8000aa40:	e3a0100a 	mov	r1, #10
8000aa44:	ebffff36 	bl	8000a724 <console_put_char>
8000aa48:	e1550007 	cmp	r5, r7
8000aa4c:	1affffed 	bne	8000aa08 <console_reset+0x118>
8000aa50:	e1a00009 	mov	r0, r9
8000aa54:	eb000bd6 	bl	8000d9b4 <kfree>
8000aa58:	e3a00000 	mov	r0, #0
8000aa5c:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000aa60:	e3e00000 	mvn	r0, #0
8000aa64:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}

8000aa68 <console_put_string>:
8000aa68:	e1a0c00d 	mov	ip, sp
8000aa6c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000aa70:	e1a04001 	mov	r4, r1
8000aa74:	e5d11000 	ldrb	r1, [r1]
8000aa78:	e24cb004 	sub	fp, ip, #4
8000aa7c:	e3510000 	cmp	r1, #0
8000aa80:	e1a05000 	mov	r5, r0
8000aa84:	089da830 	ldmeq	sp, {r4, r5, fp, sp, pc}
8000aa88:	e1a00005 	mov	r0, r5
8000aa8c:	ebffff24 	bl	8000a724 <console_put_char>
8000aa90:	e5f41001 	ldrb	r1, [r4, #1]!
8000aa94:	e3510000 	cmp	r1, #0
8000aa98:	1afffffa 	bne	8000aa88 <console_put_string+0x20>
8000aa9c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

8000aaa0 <strstr_bmh>:
8000aaa0:	e92d43f0 	push	{r4, r5, r6, r7, r8, r9, lr}
8000aaa4:	e3510000 	cmp	r1, #0
8000aaa8:	e24ddb01 	sub	sp, sp, #1024	; 0x400
8000aaac:	e24dd004 	sub	sp, sp, #4
8000aab0:	0a000049 	beq	8000abdc <strstr_bmh+0x13c>
8000aab4:	e5d1c000 	ldrb	ip, [r1]
8000aab8:	e35c0000 	cmp	ip, #0
8000aabc:	0a00004a 	beq	8000abec <strstr_bmh+0x14c>
8000aac0:	e2813001 	add	r3, r1, #1
8000aac4:	e0615003 	rsb	r5, r1, r3
8000aac8:	e4d32001 	ldrb	r2, [r3], #1
8000aacc:	e3520000 	cmp	r2, #0
8000aad0:	1afffffb 	bne	8000aac4 <strstr_bmh+0x24>
8000aad4:	e1a02005 	mov	r2, r5
8000aad8:	e1a0e00c 	mov	lr, ip
8000aadc:	e3500000 	cmp	r0, #0
8000aae0:	01a06000 	moveq	r6, r0
8000aae4:	0a000007 	beq	8000ab08 <strstr_bmh+0x68>
8000aae8:	e5d03000 	ldrb	r3, [r0]
8000aaec:	e3530000 	cmp	r3, #0
8000aaf0:	0a000040 	beq	8000abf8 <strstr_bmh+0x158>
8000aaf4:	e2803001 	add	r3, r0, #1
8000aaf8:	e0606003 	rsb	r6, r0, r3
8000aafc:	e4d3c001 	ldrb	ip, [r3], #1
8000ab00:	e35c0000 	cmp	ip, #0
8000ab04:	1afffffb 	bne	8000aaf8 <strstr_bmh+0x58>
8000ab08:	e35e0000 	cmp	lr, #0
8000ab0c:	0a00002f 	beq	8000abd0 <strstr_bmh+0x130>
8000ab10:	e24d3004 	sub	r3, sp, #4
8000ab14:	e28dcfff 	add	ip, sp, #1020	; 0x3fc
8000ab18:	e5a32004 	str	r2, [r3, #4]!
8000ab1c:	e153000c 	cmp	r3, ip
8000ab20:	1afffffc 	bne	8000ab18 <strstr_bmh+0x78>
8000ab24:	e2428001 	sub	r8, r2, #1
8000ab28:	e3580000 	cmp	r8, #0
8000ab2c:	c2424002 	subgt	r4, r2, #2
8000ab30:	c0814004 	addgt	r4, r1, r4
8000ab34:	c241e001 	subgt	lr, r1, #1
8000ab38:	c1a0c008 	movgt	ip, r8
8000ab3c:	da000006 	ble	8000ab5c <strstr_bmh+0xbc>
8000ab40:	e5fe3001 	ldrb	r3, [lr, #1]!
8000ab44:	e28d7b01 	add	r7, sp, #1024	; 0x400
8000ab48:	e0873103 	add	r3, r7, r3, lsl #2
8000ab4c:	e15e0004 	cmp	lr, r4
8000ab50:	e503c400 	str	ip, [r3, #-1024]	; 0x400
8000ab54:	e24cc001 	sub	ip, ip, #1
8000ab58:	1afffff8 	bne	8000ab40 <strstr_bmh+0xa0>
8000ab5c:	e1560002 	cmp	r6, r2
8000ab60:	a7f19008 	ldrbge	r9, [r1, r8]!
8000ab64:	ba000018 	blt	8000abcc <strstr_bmh+0x12c>
8000ab68:	e7d03008 	ldrb	r3, [r0, r8]
8000ab6c:	e1530009 	cmp	r3, r9
8000ab70:	1a00000c 	bne	8000aba8 <strstr_bmh+0x108>
8000ab74:	e3580000 	cmp	r8, #0
8000ab78:	0a000014 	beq	8000abd0 <strstr_bmh+0x130>
8000ab7c:	e1a0e001 	mov	lr, r1
8000ab80:	e080c008 	add	ip, r0, r8
8000ab84:	e1a03008 	mov	r3, r8
8000ab88:	ea000001 	b	8000ab94 <strstr_bmh+0xf4>
8000ab8c:	e3530000 	cmp	r3, #0
8000ab90:	0a00000e 	beq	8000abd0 <strstr_bmh+0x130>
8000ab94:	e57e7001 	ldrb	r7, [lr, #-1]!
8000ab98:	e57c4001 	ldrb	r4, [ip, #-1]!
8000ab9c:	e2433001 	sub	r3, r3, #1
8000aba0:	e1570004 	cmp	r7, r4
8000aba4:	0afffff8 	beq	8000ab8c <strstr_bmh+0xec>
8000aba8:	e0803005 	add	r3, r0, r5
8000abac:	e5533001 	ldrb	r3, [r3, #-1]
8000abb0:	e28dcb01 	add	ip, sp, #1024	; 0x400
8000abb4:	e08c3103 	add	r3, ip, r3, lsl #2
8000abb8:	e5133400 	ldr	r3, [r3, #-1024]	; 0x400
8000abbc:	e0636006 	rsb	r6, r3, r6
8000abc0:	e1560002 	cmp	r6, r2
8000abc4:	e0800003 	add	r0, r0, r3
8000abc8:	aaffffe6 	bge	8000ab68 <strstr_bmh+0xc8>
8000abcc:	e3a00000 	mov	r0, #0
8000abd0:	e28ddb01 	add	sp, sp, #1024	; 0x400
8000abd4:	e28dd004 	add	sp, sp, #4
8000abd8:	e8bd83f0 	pop	{r4, r5, r6, r7, r8, r9, pc}
8000abdc:	e5d1e000 	ldrb	lr, [r1]
8000abe0:	e1a02001 	mov	r2, r1
8000abe4:	e1a05001 	mov	r5, r1
8000abe8:	eaffffbb 	b	8000aadc <strstr_bmh+0x3c>
8000abec:	e1a0200c 	mov	r2, ip
8000abf0:	e1a0500c 	mov	r5, ip
8000abf4:	eaffffb7 	b	8000aad8 <strstr_bmh+0x38>
8000abf8:	e1a06003 	mov	r6, r3
8000abfc:	eaffffc1 	b	8000ab08 <strstr_bmh+0x68>

8000ac00 <memcpy>:
8000ac00:	e1a0c00d 	mov	ip, sp
8000ac04:	e3510000 	cmp	r1, #0
8000ac08:	13500000 	cmpne	r0, #0
8000ac0c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000ac10:	e24cb004 	sub	fp, ip, #4
8000ac14:	e1a06002 	mov	r6, r2
8000ac18:	e1a07000 	mov	r7, r0
8000ac1c:	e1a04001 	mov	r4, r1
8000ac20:	0a000010 	beq	8000ac68 <memcpy+0x68>
8000ac24:	e1b052a2 	lsrs	r5, r2, #5
8000ac28:	1a00000a 	bne	8000ac58 <memcpy+0x58>
8000ac2c:	e1560005 	cmp	r6, r5
8000ac30:	80842006 	addhi	r2, r4, r6
8000ac34:	80841005 	addhi	r1, r4, r5
8000ac38:	80873005 	addhi	r3, r7, r5
8000ac3c:	9a000003 	bls	8000ac50 <memcpy+0x50>
8000ac40:	e4d1c001 	ldrb	ip, [r1], #1
8000ac44:	e1510002 	cmp	r1, r2
8000ac48:	e4c3c001 	strb	ip, [r3], #1
8000ac4c:	1afffffb 	bne	8000ac40 <memcpy+0x40>
8000ac50:	e1a00007 	mov	r0, r7
8000ac54:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000ac58:	e1a05285 	lsl	r5, r5, #5
8000ac5c:	e1a02005 	mov	r2, r5
8000ac60:	ebfff578 	bl	80008248 <__memcpy32>
8000ac64:	eafffff0 	b	8000ac2c <memcpy+0x2c>
8000ac68:	e3a00000 	mov	r0, #0
8000ac6c:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}

8000ac70 <memset>:
8000ac70:	e3500000 	cmp	r0, #0
8000ac74:	e92d4010 	push	{r4, lr}
8000ac78:	08bd8010 	popeq	{r4, pc}
8000ac7c:	e210e003 	ands	lr, r0, #3
8000ac80:	e20110ff 	and	r1, r1, #255	; 0xff
8000ac84:	1a000016 	bne	8000ace4 <memset+0x74>
8000ac88:	e1a03000 	mov	r3, r0
8000ac8c:	e1814401 	orr	r4, r1, r1, lsl #8
8000ac90:	e3520003 	cmp	r2, #3
8000ac94:	e1844804 	orr	r4, r4, r4, lsl #16
8000ac98:	9a00000a 	bls	8000acc8 <memset+0x58>
8000ac9c:	e1a0e003 	mov	lr, r3
8000aca0:	e1a0c002 	mov	ip, r2
8000aca4:	e24cc004 	sub	ip, ip, #4
8000aca8:	e35c0003 	cmp	ip, #3
8000acac:	e48e4004 	str	r4, [lr], #4
8000acb0:	8afffffb 	bhi	8000aca4 <memset+0x34>
8000acb4:	e242c004 	sub	ip, r2, #4
8000acb8:	e3ccc003 	bic	ip, ip, #3
8000acbc:	e28cc004 	add	ip, ip, #4
8000acc0:	e083300c 	add	r3, r3, ip
8000acc4:	e2022003 	and	r2, r2, #3
8000acc8:	e3520000 	cmp	r2, #0
8000accc:	08bd8010 	popeq	{r4, pc}
8000acd0:	e0832002 	add	r2, r3, r2
8000acd4:	e4c31001 	strb	r1, [r3], #1
8000acd8:	e1530002 	cmp	r3, r2
8000acdc:	1afffffc 	bne	8000acd4 <memset+0x64>
8000ace0:	e8bd8010 	pop	{r4, pc}
8000ace4:	e26ee004 	rsb	lr, lr, #4
8000ace8:	e152000e 	cmp	r2, lr
8000acec:	8a000007 	bhi	8000ad10 <memset+0xa0>
8000acf0:	e3520000 	cmp	r2, #0
8000acf4:	10802002 	addne	r2, r0, r2
8000acf8:	11a03000 	movne	r3, r0
8000acfc:	0a00000a 	beq	8000ad2c <memset+0xbc>
8000ad00:	e4c31001 	strb	r1, [r3], #1
8000ad04:	e1530002 	cmp	r3, r2
8000ad08:	1afffffc 	bne	8000ad00 <memset+0x90>
8000ad0c:	e8bd8010 	pop	{r4, pc}
8000ad10:	e080300e 	add	r3, r0, lr
8000ad14:	e1a0c000 	mov	ip, r0
8000ad18:	e4cc1001 	strb	r1, [ip], #1
8000ad1c:	e15c0003 	cmp	ip, r3
8000ad20:	1afffffc 	bne	8000ad18 <memset+0xa8>
8000ad24:	e06e2002 	rsb	r2, lr, r2
8000ad28:	eaffffd7 	b	8000ac8c <memset+0x1c>
8000ad2c:	e8bd8010 	pop	{r4, pc}

8000ad30 <strcpy>:
8000ad30:	e3510000 	cmp	r1, #0
8000ad34:	012fff1e 	bxeq	lr
8000ad38:	e5d13000 	ldrb	r3, [r1]
8000ad3c:	e1a02000 	mov	r2, r0
8000ad40:	e3530000 	cmp	r3, #0
8000ad44:	0a000003 	beq	8000ad58 <strcpy+0x28>
8000ad48:	e4c23001 	strb	r3, [r2], #1
8000ad4c:	e5f13001 	ldrb	r3, [r1, #1]!
8000ad50:	e3530000 	cmp	r3, #0
8000ad54:	1afffffb 	bne	8000ad48 <strcpy+0x18>
8000ad58:	e3a03000 	mov	r3, #0
8000ad5c:	e5c23000 	strb	r3, [r2]
8000ad60:	e12fff1e 	bx	lr

8000ad64 <strncpy>:
8000ad64:	e3510000 	cmp	r1, #0
8000ad68:	e92d4010 	push	{r4, lr}
8000ad6c:	0a00001c 	beq	8000ade4 <strncpy+0x80>
8000ad70:	e3520000 	cmp	r2, #0
8000ad74:	0a00000d 	beq	8000adb0 <strncpy+0x4c>
8000ad78:	e5d1c000 	ldrb	ip, [r1]
8000ad7c:	e35c0000 	cmp	ip, #0
8000ad80:	0a000019 	beq	8000adec <strncpy+0x88>
8000ad84:	e240e001 	sub	lr, r0, #1
8000ad88:	e1a04001 	mov	r4, r1
8000ad8c:	e3a03000 	mov	r3, #0
8000ad90:	ea000002 	b	8000ada0 <strncpy+0x3c>
8000ad94:	e5f4c001 	ldrb	ip, [r4, #1]!
8000ad98:	e35c0000 	cmp	ip, #0
8000ad9c:	0a00000e 	beq	8000addc <strncpy+0x78>
8000ada0:	e2833001 	add	r3, r3, #1
8000ada4:	e1530002 	cmp	r3, r2
8000ada8:	e5eec001 	strb	ip, [lr, #1]!
8000adac:	1afffff8 	bne	8000ad94 <strncpy+0x30>
8000adb0:	e3a03000 	mov	r3, #0
8000adb4:	e7c03002 	strb	r3, [r0, r2]
8000adb8:	e5d10000 	ldrb	r0, [r1]
8000adbc:	e1500003 	cmp	r0, r3
8000adc0:	08bd8010 	popeq	{r4, pc}
8000adc4:	e2813001 	add	r3, r1, #1
8000adc8:	e0610003 	rsb	r0, r1, r3
8000adcc:	e4d32001 	ldrb	r2, [r3], #1
8000add0:	e3520000 	cmp	r2, #0
8000add4:	1afffffb 	bne	8000adc8 <strncpy+0x64>
8000add8:	e8bd8010 	pop	{r4, pc}
8000addc:	e1a02003 	mov	r2, r3
8000ade0:	eafffff2 	b	8000adb0 <strncpy+0x4c>
8000ade4:	e1a00001 	mov	r0, r1
8000ade8:	e8bd8010 	pop	{r4, pc}
8000adec:	e1a0200c 	mov	r2, ip
8000adf0:	eaffffee 	b	8000adb0 <strncpy+0x4c>

8000adf4 <strcmp>:
8000adf4:	e3510000 	cmp	r1, #0
8000adf8:	13500000 	cmpne	r0, #0
8000adfc:	0a000011 	beq	8000ae48 <strcmp+0x54>
8000ae00:	e5d03000 	ldrb	r3, [r0]
8000ae04:	e5d12000 	ldrb	r2, [r1]
8000ae08:	e1530002 	cmp	r3, r2
8000ae0c:	0a000004 	beq	8000ae24 <strcmp+0x30>
8000ae10:	ea000008 	b	8000ae38 <strcmp+0x44>
8000ae14:	e5f03001 	ldrb	r3, [r0, #1]!
8000ae18:	e5f12001 	ldrb	r2, [r1, #1]!
8000ae1c:	e1530002 	cmp	r3, r2
8000ae20:	1a000004 	bne	8000ae38 <strcmp+0x44>
8000ae24:	e3530000 	cmp	r3, #0
8000ae28:	1afffff9 	bne	8000ae14 <strcmp+0x20>
8000ae2c:	e1a00003 	mov	r0, r3
8000ae30:	e0630000 	rsb	r0, r3, r0
8000ae34:	e12fff1e 	bx	lr
8000ae38:	e1a00003 	mov	r0, r3
8000ae3c:	e1a03002 	mov	r3, r2
8000ae40:	e0630000 	rsb	r0, r3, r0
8000ae44:	e12fff1e 	bx	lr
8000ae48:	e3e00000 	mvn	r0, #0
8000ae4c:	e12fff1e 	bx	lr

8000ae50 <strncmp>:
8000ae50:	e3510000 	cmp	r1, #0
8000ae54:	13500000 	cmpne	r0, #0
8000ae58:	0a000019 	beq	8000aec4 <strncmp+0x74>
8000ae5c:	e3520000 	cmp	r2, #0
8000ae60:	0a000013 	beq	8000aeb4 <strncmp+0x64>
8000ae64:	e5d03000 	ldrb	r3, [r0]
8000ae68:	e5d1c000 	ldrb	ip, [r1]
8000ae6c:	e153000c 	cmp	r3, ip
8000ae70:	1a000015 	bne	8000aecc <strncmp+0x7c>
8000ae74:	e3530000 	cmp	r3, #0
8000ae78:	0a000015 	beq	8000aed4 <strncmp+0x84>
8000ae7c:	e2522001 	subs	r2, r2, #1
8000ae80:	0a00000b 	beq	8000aeb4 <strncmp+0x64>
8000ae84:	e080c002 	add	ip, r0, r2
8000ae88:	ea000003 	b	8000ae9c <strncmp+0x4c>
8000ae8c:	e3520000 	cmp	r2, #0
8000ae90:	0a000007 	beq	8000aeb4 <strncmp+0x64>
8000ae94:	e150000c 	cmp	r0, ip
8000ae98:	0a000007 	beq	8000aebc <strncmp+0x6c>
8000ae9c:	e5f02001 	ldrb	r2, [r0, #1]!
8000aea0:	e5f13001 	ldrb	r3, [r1, #1]!
8000aea4:	e1520003 	cmp	r2, r3
8000aea8:	0afffff7 	beq	8000ae8c <strncmp+0x3c>
8000aeac:	e0630002 	rsb	r0, r3, r2
8000aeb0:	e12fff1e 	bx	lr
8000aeb4:	e1a00002 	mov	r0, r2
8000aeb8:	e12fff1e 	bx	lr
8000aebc:	e3a00000 	mov	r0, #0
8000aec0:	e12fff1e 	bx	lr
8000aec4:	e3e00000 	mvn	r0, #0
8000aec8:	e12fff1e 	bx	lr
8000aecc:	e06c0003 	rsb	r0, ip, r3
8000aed0:	e12fff1e 	bx	lr
8000aed4:	e1a00003 	mov	r0, r3
8000aed8:	e12fff1e 	bx	lr

8000aedc <strchr>:
8000aedc:	e3500000 	cmp	r0, #0
8000aee0:	012fff1e 	bxeq	lr
8000aee4:	e5d03000 	ldrb	r3, [r0]
8000aee8:	e3530000 	cmp	r3, #0
8000aeec:	0a000007 	beq	8000af10 <strchr+0x34>
8000aef0:	e1530001 	cmp	r3, r1
8000aef4:	1a000002 	bne	8000af04 <strchr+0x28>
8000aef8:	e12fff1e 	bx	lr
8000aefc:	e1530001 	cmp	r3, r1
8000af00:	0a000005 	beq	8000af1c <strchr+0x40>
8000af04:	e5f03001 	ldrb	r3, [r0, #1]!
8000af08:	e3530000 	cmp	r3, #0
8000af0c:	1afffffa 	bne	8000aefc <strchr+0x20>
8000af10:	e3510000 	cmp	r1, #0
8000af14:	13a00000 	movne	r0, #0
8000af18:	e12fff1e 	bx	lr
8000af1c:	e12fff1e 	bx	lr

8000af20 <memcmp>:
8000af20:	e3520000 	cmp	r2, #0
8000af24:	0a00000b 	beq	8000af58 <memcmp+0x38>
8000af28:	e0800002 	add	r0, r0, r2
8000af2c:	e0811002 	add	r1, r1, r2
8000af30:	ea000001 	b	8000af3c <memcmp+0x1c>
8000af34:	e3520000 	cmp	r2, #0
8000af38:	0a000006 	beq	8000af58 <memcmp+0x38>
8000af3c:	e570c001 	ldrb	ip, [r0, #-1]!
8000af40:	e5713001 	ldrb	r3, [r1, #-1]!
8000af44:	e2422001 	sub	r2, r2, #1
8000af48:	e15c0003 	cmp	ip, r3
8000af4c:	9afffff8 	bls	8000af34 <memcmp+0x14>
8000af50:	e3a00001 	mov	r0, #1
8000af54:	e12fff1e 	bx	lr
8000af58:	e1a00002 	mov	r0, r2
8000af5c:	e12fff1e 	bx	lr

8000af60 <strstr>:
8000af60:	eafffece 	b	8000aaa0 <strstr_bmh>

8000af64 <strtok>:
8000af64:	e92d4030 	push	{r4, r5, lr}
8000af68:	e2504000 	subs	r4, r0, #0
8000af6c:	0a00003b 	beq	8000b060 <strtok+0xfc>
8000af70:	e5d4e000 	ldrb	lr, [r4]
8000af74:	e59f30ec 	ldr	r3, [pc, #236]	; 8000b068 <strtok+0x104>
8000af78:	e35e0000 	cmp	lr, #0
8000af7c:	e08f3003 	add	r3, pc, r3
8000af80:	e5834000 	str	r4, [r3]
8000af84:	0a000028 	beq	8000b02c <strtok+0xc8>
8000af88:	e3510000 	cmp	r1, #0
8000af8c:	e1a0c00e 	mov	ip, lr
8000af90:	0a000030 	beq	8000b058 <strtok+0xf4>
8000af94:	e5d15000 	ldrb	r5, [r1]
8000af98:	e1a00004 	mov	r0, r4
8000af9c:	e3550000 	cmp	r5, #0
8000afa0:	0a000008 	beq	8000afc8 <strtok+0x64>
8000afa4:	e15c0005 	cmp	ip, r5
8000afa8:	0a00001c 	beq	8000b020 <strtok+0xbc>
8000afac:	e1a02001 	mov	r2, r1
8000afb0:	ea000001 	b	8000afbc <strtok+0x58>
8000afb4:	e15c0003 	cmp	ip, r3
8000afb8:	0a000018 	beq	8000b020 <strtok+0xbc>
8000afbc:	e5f23001 	ldrb	r3, [r2, #1]!
8000afc0:	e3530000 	cmp	r3, #0
8000afc4:	1afffffa 	bne	8000afb4 <strtok+0x50>
8000afc8:	e1a0c004 	mov	ip, r4
8000afcc:	e3510000 	cmp	r1, #0
8000afd0:	0a00000b 	beq	8000b004 <strtok+0xa0>
8000afd4:	e5d13000 	ldrb	r3, [r1]
8000afd8:	e3530000 	cmp	r3, #0
8000afdc:	0a000008 	beq	8000b004 <strtok+0xa0>
8000afe0:	e15e0003 	cmp	lr, r3
8000afe4:	0a000015 	beq	8000b040 <strtok+0xdc>
8000afe8:	e1a02001 	mov	r2, r1
8000afec:	ea000001 	b	8000aff8 <strtok+0x94>
8000aff0:	e15e0003 	cmp	lr, r3
8000aff4:	0a000011 	beq	8000b040 <strtok+0xdc>
8000aff8:	e5f23001 	ldrb	r3, [r2, #1]!
8000affc:	e3530000 	cmp	r3, #0
8000b000:	1afffffa 	bne	8000aff0 <strtok+0x8c>
8000b004:	e5fce001 	ldrb	lr, [ip, #1]!
8000b008:	e35e0000 	cmp	lr, #0
8000b00c:	1affffee 	bne	8000afcc <strtok+0x68>
8000b010:	e59f3054 	ldr	r3, [pc, #84]	; 8000b06c <strtok+0x108>
8000b014:	e08f3003 	add	r3, pc, r3
8000b018:	e583c000 	str	ip, [r3]
8000b01c:	e8bd8030 	pop	{r4, r5, pc}
8000b020:	e5f0c001 	ldrb	ip, [r0, #1]!
8000b024:	e35c0000 	cmp	ip, #0
8000b028:	1affffdb 	bne	8000af9c <strtok+0x38>
8000b02c:	e59f303c 	ldr	r3, [pc, #60]	; 8000b070 <strtok+0x10c>
8000b030:	e3a00000 	mov	r0, #0
8000b034:	e08f3003 	add	r3, pc, r3
8000b038:	e5830000 	str	r0, [r3]
8000b03c:	e8bd8030 	pop	{r4, r5, pc}
8000b040:	e59f302c 	ldr	r3, [pc, #44]	; 8000b074 <strtok+0x110>
8000b044:	e3a02000 	mov	r2, #0
8000b048:	e08f3003 	add	r3, pc, r3
8000b04c:	e4cc2001 	strb	r2, [ip], #1
8000b050:	e583c000 	str	ip, [r3]
8000b054:	e8bd8030 	pop	{r4, r5, pc}
8000b058:	e1a00004 	mov	r0, r4
8000b05c:	eaffffd9 	b	8000afc8 <strtok+0x64>
8000b060:	e1a00004 	mov	r0, r4
8000b064:	e8bd8030 	pop	{r4, r5, pc}
8000b068:	000154f4 	strdeq	r5, [r1], -r4
8000b06c:	0001545c 	andeq	r5, r1, ip, asr r4
8000b070:	0001543c 	andeq	r5, r1, ip, lsr r4
8000b074:	00015428 	andeq	r5, r1, r8, lsr #8

8000b078 <strlen>:
8000b078:	e2501000 	subs	r1, r0, #0
8000b07c:	0a000008 	beq	8000b0a4 <strlen+0x2c>
8000b080:	e5d13000 	ldrb	r3, [r1]
8000b084:	e3530000 	cmp	r3, #0
8000b088:	0a000007 	beq	8000b0ac <strlen+0x34>
8000b08c:	e2813001 	add	r3, r1, #1
8000b090:	e0610003 	rsb	r0, r1, r3
8000b094:	e4d32001 	ldrb	r2, [r3], #1
8000b098:	e3520000 	cmp	r2, #0
8000b09c:	1afffffb 	bne	8000b090 <strlen+0x18>
8000b0a0:	e12fff1e 	bx	lr
8000b0a4:	e1a00001 	mov	r0, r1
8000b0a8:	e12fff1e 	bx	lr
8000b0ac:	e1a00003 	mov	r0, r3
8000b0b0:	e12fff1e 	bx	lr

8000b0b4 <buffer_is_empty>:
8000b0b4:	e5903400 	ldr	r3, [r0, #1024]	; 0x400
8000b0b8:	e5900404 	ldr	r0, [r0, #1028]	; 0x404
8000b0bc:	e0600003 	rsb	r0, r0, r3
8000b0c0:	e3500000 	cmp	r0, #0
8000b0c4:	c3a00000 	movgt	r0, #0
8000b0c8:	d3a00001 	movle	r0, #1
8000b0cc:	e12fff1e 	bx	lr

8000b0d0 <buffer_read>:
8000b0d0:	e1a0c00d 	mov	ip, sp
8000b0d4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000b0d8:	e5903400 	ldr	r3, [r0, #1024]	; 0x400
8000b0dc:	e24cb004 	sub	fp, ip, #4
8000b0e0:	e590c404 	ldr	ip, [r0, #1028]	; 0x404
8000b0e4:	e1a05000 	mov	r5, r0
8000b0e8:	e06c3003 	rsb	r3, ip, r3
8000b0ec:	e243e001 	sub	lr, r3, #1
8000b0f0:	e35e0b01 	cmp	lr, #1024	; 0x400
8000b0f4:	3a000001 	bcc	8000b100 <buffer_read+0x30>
8000b0f8:	e3a00000 	mov	r0, #0
8000b0fc:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000b100:	e1520003 	cmp	r2, r3
8000b104:	a1a04003 	movge	r4, r3
8000b108:	b1a04002 	movlt	r4, r2
8000b10c:	e1a00001 	mov	r0, r1
8000b110:	e1a02004 	mov	r2, r4
8000b114:	e085100c 	add	r1, r5, ip
8000b118:	ebfffeb8 	bl	8000ac00 <memcpy>
8000b11c:	e5953404 	ldr	r3, [r5, #1028]	; 0x404
8000b120:	e5952400 	ldr	r2, [r5, #1024]	; 0x400
8000b124:	e0843003 	add	r3, r4, r3
8000b128:	e1530002 	cmp	r3, r2
8000b12c:	e5853404 	str	r3, [r5, #1028]	; 0x404
8000b130:	0a000001 	beq	8000b13c <buffer_read+0x6c>
8000b134:	e1a00004 	mov	r0, r4
8000b138:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000b13c:	e3a03000 	mov	r3, #0
8000b140:	e1a00004 	mov	r0, r4
8000b144:	e5853404 	str	r3, [r5, #1028]	; 0x404
8000b148:	e5853400 	str	r3, [r5, #1024]	; 0x400
8000b14c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

8000b150 <buffer_write>:
8000b150:	e1a0c00d 	mov	ip, sp
8000b154:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000b158:	e5906400 	ldr	r6, [r0, #1024]	; 0x400
8000b15c:	e24cb004 	sub	fp, ip, #4
8000b160:	e3560000 	cmp	r6, #0
8000b164:	e1a05000 	mov	r5, r0
8000b168:	0a000001 	beq	8000b174 <buffer_write+0x24>
8000b16c:	e3a00000 	mov	r0, #0
8000b170:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000b174:	e3520b01 	cmp	r2, #1024	; 0x400
8000b178:	b1a04002 	movlt	r4, r2
8000b17c:	a3a04b01 	movge	r4, #1024	; 0x400
8000b180:	e1a02004 	mov	r2, r4
8000b184:	ebfffe9d 	bl	8000ac00 <memcpy>
8000b188:	e5854400 	str	r4, [r5, #1024]	; 0x400
8000b18c:	e1a00004 	mov	r0, r4
8000b190:	e5856404 	str	r6, [r5, #1028]	; 0x404
8000b194:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}

8000b198 <print_uint_in_base_raw>:
8000b198:	e1a0c00d 	mov	ip, sp
8000b19c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000b1a0:	e3a0e000 	mov	lr, #0
8000b1a4:	e1a05003 	mov	r5, r3
8000b1a8:	e24cb004 	sub	fp, ip, #4
8000b1ac:	e1a06000 	mov	r6, r0
8000b1b0:	e1a0400e 	mov	r4, lr
8000b1b4:	e3a0301f 	mov	r3, #31
8000b1b8:	e3a07001 	mov	r7, #1
8000b1bc:	e1a00317 	lsl	r0, r7, r3
8000b1c0:	e1100001 	tst	r0, r1
8000b1c4:	13a0c001 	movne	ip, #1
8000b1c8:	03a0c000 	moveq	ip, #0
8000b1cc:	e18c4084 	orr	r4, ip, r4, lsl #1
8000b1d0:	e1520004 	cmp	r2, r4
8000b1d4:	e2433001 	sub	r3, r3, #1
8000b1d8:	90624004 	rsbls	r4, r2, r4
8000b1dc:	918ee000 	orrls	lr, lr, r0
8000b1e0:	e3730001 	cmn	r3, #1
8000b1e4:	1afffff4 	bne	8000b1bc <print_uint_in_base_raw+0x24>
8000b1e8:	e35e0000 	cmp	lr, #0
8000b1ec:	1a00000b 	bne	8000b220 <print_uint_in_base_raw+0x88>
8000b1f0:	e3550000 	cmp	r5, #0
8000b1f4:	1a000004 	bne	8000b20c <print_uint_in_base_raw+0x74>
8000b1f8:	e59f3034 	ldr	r3, [pc, #52]	; 8000b234 <print_uint_in_base_raw+0x9c>
8000b1fc:	e08f3003 	add	r3, pc, r3
8000b200:	e7d33004 	ldrb	r3, [r3, r4]
8000b204:	e5c63000 	strb	r3, [r6]
8000b208:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000b20c:	e59f3024 	ldr	r3, [pc, #36]	; 8000b238 <print_uint_in_base_raw+0xa0>
8000b210:	e08f3003 	add	r3, pc, r3
8000b214:	e7d33004 	ldrb	r3, [r3, r4]
8000b218:	e5c63000 	strb	r3, [r6]
8000b21c:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000b220:	e1a0100e 	mov	r1, lr
8000b224:	e2860001 	add	r0, r6, #1
8000b228:	e1a03005 	mov	r3, r5
8000b22c:	ebffffd9 	bl	8000b198 <print_uint_in_base_raw>
8000b230:	eaffffee 	b	8000b1f0 <print_uint_in_base_raw+0x58>
8000b234:	00006dac 	andeq	r6, r0, ip, lsr #27
8000b238:	00006dac 	andeq	r6, r0, ip, lsr #27

8000b23c <outc_sn>:
8000b23c:	e5913004 	ldr	r3, [r1, #4]
8000b240:	e5912008 	ldr	r2, [r1, #8]
8000b244:	e1530002 	cmp	r3, r2
8000b248:	35912000 	ldrcc	r2, [r1]
8000b24c:	37c20003 	strbcc	r0, [r2, r3]
8000b250:	35913004 	ldrcc	r3, [r1, #4]
8000b254:	32833001 	addcc	r3, r3, #1
8000b258:	35813004 	strcc	r3, [r1, #4]
8000b25c:	e12fff1e 	bx	lr

8000b260 <print_uint_in_base>:
8000b260:	e1a0c00d 	mov	ip, sp
8000b264:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000b268:	e24cb004 	sub	fp, ip, #4
8000b26c:	e24dd02c 	sub	sp, sp, #44	; 0x2c
8000b270:	e24b604c 	sub	r6, fp, #76	; 0x4c
8000b274:	e1a07000 	mov	r7, r0
8000b278:	e50b2050 	str	r2, [fp, #-80]	; 0x50
8000b27c:	e1a08001 	mov	r8, r1
8000b280:	e3a02020 	mov	r2, #32
8000b284:	e3a01000 	mov	r1, #0
8000b288:	e1a00006 	mov	r0, r6
8000b28c:	e1a0a003 	mov	sl, r3
8000b290:	e5db4008 	ldrb	r4, [fp, #8]
8000b294:	e5db500c 	ldrb	r5, [fp, #12]
8000b298:	e59b9004 	ldr	r9, [fp, #4]
8000b29c:	ebfffe73 	bl	8000ac70 <memset>
8000b2a0:	e51bc050 	ldr	ip, [fp, #-80]	; 0x50
8000b2a4:	e1a00006 	mov	r0, r6
8000b2a8:	e1a0100c 	mov	r1, ip
8000b2ac:	e1a0200a 	mov	r2, sl
8000b2b0:	e1a03005 	mov	r3, r5
8000b2b4:	ebffffb7 	bl	8000b198 <print_uint_in_base_raw>
8000b2b8:	e1a00006 	mov	r0, r6
8000b2bc:	ebffff6d 	bl	8000b078 <strlen>
8000b2c0:	e3540000 	cmp	r4, #0
8000b2c4:	0a000009 	beq	8000b2f0 <print_uint_in_base+0x90>
8000b2c8:	e0604009 	rsb	r4, r0, r9
8000b2cc:	e3540000 	cmp	r4, #0
8000b2d0:	da00002f 	ble	8000b394 <print_uint_in_base+0x134>
8000b2d4:	e3a05000 	mov	r5, #0
8000b2d8:	e2855001 	add	r5, r5, #1
8000b2dc:	e3a00030 	mov	r0, #48	; 0x30
8000b2e0:	e1a01008 	mov	r1, r8
8000b2e4:	e12fff37 	blx	r7
8000b2e8:	e1550004 	cmp	r5, r4
8000b2ec:	1afffff9 	bne	8000b2d8 <print_uint_in_base+0x78>
8000b2f0:	e1a00006 	mov	r0, r6
8000b2f4:	ebffff5f 	bl	8000b078 <strlen>
8000b2f8:	e2505001 	subs	r5, r0, #1
8000b2fc:	4a00001a 	bmi	8000b36c <print_uint_in_base+0x10c>
8000b300:	e24b302c 	sub	r3, fp, #44	; 0x2c
8000b304:	e0833005 	add	r3, r3, r5
8000b308:	e5530020 	ldrb	r0, [r3, #-32]
8000b30c:	e3500000 	cmp	r0, #0
8000b310:	0a000015 	beq	8000b36c <print_uint_in_base+0x10c>
8000b314:	e3590000 	cmp	r9, #0
8000b318:	d3a0a000 	movle	sl, #0
8000b31c:	c3a0a001 	movgt	sl, #1
8000b320:	e1540009 	cmp	r4, r9
8000b324:	a3590000 	cmpge	r9, #0
8000b328:	ca000017 	bgt	8000b38c <print_uint_in_base+0x12c>
8000b32c:	e0866005 	add	r6, r6, r5
8000b330:	ea000007 	b	8000b354 <print_uint_in_base+0xf4>
8000b334:	e5760001 	ldrb	r0, [r6, #-1]!
8000b338:	e3500000 	cmp	r0, #0
8000b33c:	0a00000a 	beq	8000b36c <print_uint_in_base+0x10c>
8000b340:	e1590004 	cmp	r9, r4
8000b344:	c3a03000 	movgt	r3, #0
8000b348:	d20a3001 	andle	r3, sl, #1
8000b34c:	e3530000 	cmp	r3, #0
8000b350:	1a00000d 	bne	8000b38c <print_uint_in_base+0x12c>
8000b354:	e2455001 	sub	r5, r5, #1
8000b358:	e1a01008 	mov	r1, r8
8000b35c:	e12fff37 	blx	r7
8000b360:	e3750001 	cmn	r5, #1
8000b364:	e2844001 	add	r4, r4, #1
8000b368:	1afffff1 	bne	8000b334 <print_uint_in_base+0xd4>
8000b36c:	e1590004 	cmp	r9, r4
8000b370:	da000005 	ble	8000b38c <print_uint_in_base+0x12c>
8000b374:	e2844001 	add	r4, r4, #1
8000b378:	e3a00020 	mov	r0, #32
8000b37c:	e1a01008 	mov	r1, r8
8000b380:	e12fff37 	blx	r7
8000b384:	e1540009 	cmp	r4, r9
8000b388:	1afffff9 	bne	8000b374 <print_uint_in_base+0x114>
8000b38c:	e24bd028 	sub	sp, fp, #40	; 0x28
8000b390:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000b394:	e3a04000 	mov	r4, #0
8000b398:	eaffffd4 	b	8000b2f0 <print_uint_in_base+0x90>

8000b39c <v_printf>:
8000b39c:	e1a0c00d 	mov	ip, sp
8000b3a0:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000b3a4:	e24cb004 	sub	fp, ip, #4
8000b3a8:	e24dd02c 	sub	sp, sp, #44	; 0x2c
8000b3ac:	e1a07002 	mov	r7, r2
8000b3b0:	e5d22000 	ldrb	r2, [r2]
8000b3b4:	e1a08003 	mov	r8, r3
8000b3b8:	e3520000 	cmp	r2, #0
8000b3bc:	124b3034 	subne	r3, fp, #52	; 0x34
8000b3c0:	e1a06000 	mov	r6, r0
8000b3c4:	e1a05001 	mov	r5, r1
8000b3c8:	13a09000 	movne	r9, #0
8000b3cc:	11a00002 	movne	r0, r2
8000b3d0:	150b3038 	strne	r3, [fp, #-56]	; 0x38
8000b3d4:	0a000076 	beq	8000b5b4 <v_printf+0x218>
8000b3d8:	e3500025 	cmp	r0, #37	; 0x25
8000b3dc:	0a000009 	beq	8000b408 <v_printf+0x6c>
8000b3e0:	e0874009 	add	r4, r7, r9
8000b3e4:	e1a01005 	mov	r1, r5
8000b3e8:	e12fff36 	blx	r6
8000b3ec:	e5f40001 	ldrb	r0, [r4, #1]!
8000b3f0:	e2899001 	add	r9, r9, #1
8000b3f4:	e3500000 	cmp	r0, #0
8000b3f8:	13500025 	cmpne	r0, #37	; 0x25
8000b3fc:	1afffff8 	bne	8000b3e4 <v_printf+0x48>
8000b400:	e3500000 	cmp	r0, #0
8000b404:	0a00006a 	beq	8000b5b4 <v_printf+0x218>
8000b408:	e0873009 	add	r3, r7, r9
8000b40c:	e5d33001 	ldrb	r3, [r3, #1]
8000b410:	e3530000 	cmp	r3, #0
8000b414:	0a000066 	beq	8000b5b4 <v_printf+0x218>
8000b418:	e289e001 	add	lr, r9, #1
8000b41c:	e7d7300e 	ldrb	r3, [r7, lr]
8000b420:	e51b2038 	ldr	r2, [fp, #-56]	; 0x38
8000b424:	e3530030 	cmp	r3, #48	; 0x30
8000b428:	0289e002 	addeq	lr, r9, #2
8000b42c:	07d7300e 	ldrbeq	r3, [r7, lr]
8000b430:	03a09001 	moveq	r9, #1
8000b434:	13a09000 	movne	r9, #0
8000b438:	e353002d 	cmp	r3, #45	; 0x2d
8000b43c:	13a03000 	movne	r3, #0
8000b440:	054b3034 	strbeq	r3, [fp, #-52]	; 0x34
8000b444:	03a03001 	moveq	r3, #1
8000b448:	e08ec003 	add	ip, lr, r3
8000b44c:	e24c0001 	sub	r0, ip, #1
8000b450:	e2431001 	sub	r1, r3, #1
8000b454:	e0870000 	add	r0, r7, r0
8000b458:	e0821001 	add	r1, r2, r1
8000b45c:	ea000000 	b	8000b464 <v_printf+0xc8>
8000b460:	e08ec003 	add	ip, lr, r3
8000b464:	e5f02001 	ldrb	r2, [r0, #1]!
8000b468:	e2424030 	sub	r4, r2, #48	; 0x30
8000b46c:	e3540009 	cmp	r4, #9
8000b470:	8a000098 	bhi	8000b6d8 <v_printf+0x33c>
8000b474:	e2833001 	add	r3, r3, #1
8000b478:	e3530008 	cmp	r3, #8
8000b47c:	e5e12001 	strb	r2, [r1, #1]!
8000b480:	1afffff6 	bne	8000b460 <v_printf+0xc4>
8000b484:	e28ea008 	add	sl, lr, #8
8000b488:	e7d7200a 	ldrb	r2, [r7, sl]
8000b48c:	e24b102c 	sub	r1, fp, #44	; 0x2c
8000b490:	e0813003 	add	r3, r1, r3
8000b494:	e3a01000 	mov	r1, #0
8000b498:	e5431008 	strb	r1, [r3, #-8]
8000b49c:	e55b3034 	ldrb	r3, [fp, #-52]	; 0x34
8000b4a0:	e3a04000 	mov	r4, #0
8000b4a4:	e353002d 	cmp	r3, #45	; 0x2d
8000b4a8:	024b102c 	subeq	r1, fp, #44	; 0x2c
8000b4ac:	05713007 	ldrbeq	r3, [r1, #-7]!
8000b4b0:	03e0e000 	mvneq	lr, #0
8000b4b4:	e2433030 	sub	r3, r3, #48	; 0x30
8000b4b8:	e20300ff 	and	r0, r3, #255	; 0xff
8000b4bc:	13a0e001 	movne	lr, #1
8000b4c0:	151b1038 	ldrne	r1, [fp, #-56]	; 0x38
8000b4c4:	e3500009 	cmp	r0, #9
8000b4c8:	8a000007 	bhi	8000b4ec <v_printf+0x150>
8000b4cc:	e5f10001 	ldrb	r0, [r1, #1]!
8000b4d0:	e0844104 	add	r4, r4, r4, lsl #2
8000b4d4:	e0834084 	add	r4, r3, r4, lsl #1
8000b4d8:	e2403030 	sub	r3, r0, #48	; 0x30
8000b4dc:	e20300ff 	and	r0, r3, #255	; 0xff
8000b4e0:	e3500009 	cmp	r0, #9
8000b4e4:	9afffff8 	bls	8000b4cc <v_printf+0x130>
8000b4e8:	e004049e 	mul	r4, lr, r4
8000b4ec:	e2422058 	sub	r2, r2, #88	; 0x58
8000b4f0:	e3520020 	cmp	r2, #32
8000b4f4:	908ff102 	addls	pc, pc, r2, lsl #2
8000b4f8:	ea000029 	b	8000b5a4 <v_printf+0x208>
8000b4fc:	ea000038 	b	8000b5e4 <v_printf+0x248>
8000b500:	ea000027 	b	8000b5a4 <v_printf+0x208>
8000b504:	ea000026 	b	8000b5a4 <v_printf+0x208>
8000b508:	ea000025 	b	8000b5a4 <v_printf+0x208>
8000b50c:	ea000024 	b	8000b5a4 <v_printf+0x208>
8000b510:	ea000023 	b	8000b5a4 <v_printf+0x208>
8000b514:	ea000022 	b	8000b5a4 <v_printf+0x208>
8000b518:	ea000021 	b	8000b5a4 <v_printf+0x208>
8000b51c:	ea000020 	b	8000b5a4 <v_printf+0x208>
8000b520:	ea00001f 	b	8000b5a4 <v_printf+0x208>
8000b524:	ea00001e 	b	8000b5a4 <v_printf+0x208>
8000b528:	ea000045 	b	8000b644 <v_printf+0x2a8>
8000b52c:	ea000031 	b	8000b5f8 <v_printf+0x25c>
8000b530:	ea00001b 	b	8000b5a4 <v_printf+0x208>
8000b534:	ea00001a 	b	8000b5a4 <v_printf+0x208>
8000b538:	ea000019 	b	8000b5a4 <v_printf+0x208>
8000b53c:	ea000018 	b	8000b5a4 <v_printf+0x208>
8000b540:	ea000017 	b	8000b5a4 <v_printf+0x208>
8000b544:	ea000016 	b	8000b5a4 <v_printf+0x208>
8000b548:	ea000015 	b	8000b5a4 <v_printf+0x208>
8000b54c:	ea000014 	b	8000b5a4 <v_printf+0x208>
8000b550:	ea000013 	b	8000b5a4 <v_printf+0x208>
8000b554:	ea000012 	b	8000b5a4 <v_printf+0x208>
8000b558:	ea000011 	b	8000b5a4 <v_printf+0x208>
8000b55c:	ea000010 	b	8000b5a4 <v_printf+0x208>
8000b560:	ea00000f 	b	8000b5a4 <v_printf+0x208>
8000b564:	ea00000e 	b	8000b5a4 <v_printf+0x208>
8000b568:	ea00003b 	b	8000b65c <v_printf+0x2c0>
8000b56c:	ea00000c 	b	8000b5a4 <v_printf+0x208>
8000b570:	ea000011 	b	8000b5bc <v_printf+0x220>
8000b574:	ea00000a 	b	8000b5a4 <v_printf+0x208>
8000b578:	ea000009 	b	8000b5a4 <v_printf+0x208>
8000b57c:	eaffffff 	b	8000b580 <v_printf+0x1e4>
8000b580:	e5982000 	ldr	r2, [r8]
8000b584:	e3a03000 	mov	r3, #0
8000b588:	e2888004 	add	r8, r8, #4
8000b58c:	e88d0210 	stm	sp, {r4, r9}
8000b590:	e58d3008 	str	r3, [sp, #8]
8000b594:	e1a00006 	mov	r0, r6
8000b598:	e1a01005 	mov	r1, r5
8000b59c:	e3a03010 	mov	r3, #16
8000b5a0:	ebffff2e 	bl	8000b260 <print_uint_in_base>
8000b5a4:	e28a9001 	add	r9, sl, #1
8000b5a8:	e7d70009 	ldrb	r0, [r7, r9]
8000b5ac:	e3500000 	cmp	r0, #0
8000b5b0:	1affff88 	bne	8000b3d8 <v_printf+0x3c>
8000b5b4:	e24bd028 	sub	sp, fp, #40	; 0x28
8000b5b8:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000b5bc:	e5982000 	ldr	r2, [r8]
8000b5c0:	e2888004 	add	r8, r8, #4
8000b5c4:	e3a03000 	mov	r3, #0
8000b5c8:	e58d3008 	str	r3, [sp, #8]
8000b5cc:	e88d0210 	stm	sp, {r4, r9}
8000b5d0:	e1a00006 	mov	r0, r6
8000b5d4:	e1a01005 	mov	r1, r5
8000b5d8:	e3a0300a 	mov	r3, #10
8000b5dc:	ebffff1f 	bl	8000b260 <print_uint_in_base>
8000b5e0:	eaffffef 	b	8000b5a4 <v_printf+0x208>
8000b5e4:	e5982000 	ldr	r2, [r8]
8000b5e8:	e3a03001 	mov	r3, #1
8000b5ec:	e2888004 	add	r8, r8, #4
8000b5f0:	e88d0210 	stm	sp, {r4, r9}
8000b5f4:	eaffffe5 	b	8000b590 <v_printf+0x1f4>
8000b5f8:	e5982000 	ldr	r2, [r8]
8000b5fc:	e2888004 	add	r8, r8, #4
8000b600:	e3520000 	cmp	r2, #0
8000b604:	aaffffee 	bge	8000b5c4 <v_printf+0x228>
8000b608:	e50b203c 	str	r2, [fp, #-60]	; 0x3c
8000b60c:	e1a01005 	mov	r1, r5
8000b610:	e3a0002d 	mov	r0, #45	; 0x2d
8000b614:	e12fff36 	blx	r6
8000b618:	e51b203c 	ldr	r2, [fp, #-60]	; 0x3c
8000b61c:	e2444001 	sub	r4, r4, #1
8000b620:	e3a03000 	mov	r3, #0
8000b624:	e58d3008 	str	r3, [sp, #8]
8000b628:	e88d0210 	stm	sp, {r4, r9}
8000b62c:	e2622000 	rsb	r2, r2, #0
8000b630:	e1a00006 	mov	r0, r6
8000b634:	e1a01005 	mov	r1, r5
8000b638:	e3a0300a 	mov	r3, #10
8000b63c:	ebffff07 	bl	8000b260 <print_uint_in_base>
8000b640:	eaffffd7 	b	8000b5a4 <v_printf+0x208>
8000b644:	e2883004 	add	r3, r8, #4
8000b648:	e5d80000 	ldrb	r0, [r8]
8000b64c:	e1a01005 	mov	r1, r5
8000b650:	e1a08003 	mov	r8, r3
8000b654:	e12fff36 	blx	r6
8000b658:	eaffffd1 	b	8000b5a4 <v_printf+0x208>
8000b65c:	e5983000 	ldr	r3, [r8]
8000b660:	e2882004 	add	r2, r8, #4
8000b664:	e1a00003 	mov	r0, r3
8000b668:	e50b3040 	str	r3, [fp, #-64]	; 0x40
8000b66c:	e50b203c 	str	r2, [fp, #-60]	; 0x3c
8000b670:	ebfffe80 	bl	8000b078 <strlen>
8000b674:	e3540000 	cmp	r4, #0
8000b678:	ba000018 	blt	8000b6e0 <v_printf+0x344>
8000b67c:	e3a08000 	mov	r8, #0
8000b680:	e51b3040 	ldr	r3, [fp, #-64]	; 0x40
8000b684:	e2439001 	sub	r9, r3, #1
8000b688:	ea000004 	b	8000b6a0 <v_printf+0x304>
8000b68c:	e2888001 	add	r8, r8, #1
8000b690:	e12fff36 	blx	r6
8000b694:	e1580004 	cmp	r8, r4
8000b698:	a3540000 	cmpge	r4, #0
8000b69c:	ca000003 	bgt	8000b6b0 <v_printf+0x314>
8000b6a0:	e5f93001 	ldrb	r3, [r9, #1]!
8000b6a4:	e1a01005 	mov	r1, r5
8000b6a8:	e2530000 	subs	r0, r3, #0
8000b6ac:	1afffff6 	bne	8000b68c <v_printf+0x2f0>
8000b6b0:	e1580004 	cmp	r8, r4
8000b6b4:	aa000005 	bge	8000b6d0 <v_printf+0x334>
8000b6b8:	e2888001 	add	r8, r8, #1
8000b6bc:	e3a00020 	mov	r0, #32
8000b6c0:	e1a01005 	mov	r1, r5
8000b6c4:	e12fff36 	blx	r6
8000b6c8:	e1580004 	cmp	r8, r4
8000b6cc:	1afffff9 	bne	8000b6b8 <v_printf+0x31c>
8000b6d0:	e51b803c 	ldr	r8, [fp, #-60]	; 0x3c
8000b6d4:	eaffffb2 	b	8000b5a4 <v_printf+0x208>
8000b6d8:	e1a0a00c 	mov	sl, ip
8000b6dc:	eaffff6a 	b	8000b48c <v_printf+0xf0>
8000b6e0:	e2644000 	rsb	r4, r4, #0
8000b6e4:	e0608004 	rsb	r8, r0, r4
8000b6e8:	e3580000 	cmp	r8, #0
8000b6ec:	daffffe2 	ble	8000b67c <v_printf+0x2e0>
8000b6f0:	e3a09000 	mov	r9, #0
8000b6f4:	e2899001 	add	r9, r9, #1
8000b6f8:	e3a00020 	mov	r0, #32
8000b6fc:	e1a01005 	mov	r1, r5
8000b700:	e12fff36 	blx	r6
8000b704:	e1590008 	cmp	r9, r8
8000b708:	1afffff9 	bne	8000b6f4 <v_printf+0x358>
8000b70c:	eaffffdb 	b	8000b680 <v_printf+0x2e4>

8000b710 <snprintf>:
8000b710:	e1a0c00d 	mov	ip, sp
8000b714:	e92d000c 	push	{r2, r3}
8000b718:	e92dd810 	push	{r4, fp, ip, lr, pc}
8000b71c:	e24cb00c 	sub	fp, ip, #12
8000b720:	e24dd014 	sub	sp, sp, #20
8000b724:	e50b0020 	str	r0, [fp, #-32]
8000b728:	e59f003c 	ldr	r0, [pc, #60]	; 8000b76c <snprintf+0x5c>
8000b72c:	e28bc008 	add	ip, fp, #8
8000b730:	e50b1018 	str	r1, [fp, #-24]
8000b734:	e1a0300c 	mov	r3, ip
8000b738:	e3a04000 	mov	r4, #0
8000b73c:	e59b2004 	ldr	r2, [fp, #4]
8000b740:	e08f0000 	add	r0, pc, r0
8000b744:	e24b1020 	sub	r1, fp, #32
8000b748:	e50bc024 	str	ip, [fp, #-36]	; 0x24
8000b74c:	e50b401c 	str	r4, [fp, #-28]
8000b750:	ebffff11 	bl	8000b39c <v_printf>
8000b754:	e51b2020 	ldr	r2, [fp, #-32]
8000b758:	e51b301c 	ldr	r3, [fp, #-28]
8000b75c:	e7c24003 	strb	r4, [r2, r3]
8000b760:	e51b001c 	ldr	r0, [fp, #-28]
8000b764:	e24bd010 	sub	sp, fp, #16
8000b768:	e89da810 	ldm	sp, {r4, fp, sp, pc}
8000b76c:	fffffaf4 			; <UNDEFINED> instruction: 0xfffffaf4

8000b770 <proto_init>:
8000b770:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
8000b774:	e291e000 	adds	lr, r1, #0
8000b778:	13a0e001 	movne	lr, #1
8000b77c:	e3e0c000 	mvn	ip, #0
8000b780:	e3a03000 	mov	r3, #0
8000b784:	e5801004 	str	r1, [r0, #4]
8000b788:	e580e014 	str	lr, [r0, #20]
8000b78c:	e5802008 	str	r2, [r0, #8]
8000b790:	e580200c 	str	r2, [r0, #12]
8000b794:	e580c000 	str	ip, [r0]
8000b798:	e5803010 	str	r3, [r0, #16]
8000b79c:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)

8000b7a0 <proto_copy>:
8000b7a0:	e1a0c00d 	mov	ip, sp
8000b7a4:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000b7a8:	e5903014 	ldr	r3, [r0, #20]
8000b7ac:	e24cb004 	sub	fp, ip, #4
8000b7b0:	e3530000 	cmp	r3, #0
8000b7b4:	e1a04000 	mov	r4, r0
8000b7b8:	e1a06001 	mov	r6, r1
8000b7bc:	e1a05002 	mov	r5, r2
8000b7c0:	1a000003 	bne	8000b7d4 <proto_copy+0x34>
8000b7c4:	e5900004 	ldr	r0, [r0, #4]
8000b7c8:	e3500000 	cmp	r0, #0
8000b7cc:	0a000000 	beq	8000b7d4 <proto_copy+0x34>
8000b7d0:	eb000877 	bl	8000d9b4 <kfree>
8000b7d4:	e1a00005 	mov	r0, r5
8000b7d8:	eb000862 	bl	8000d968 <kmalloc>
8000b7dc:	e1a01006 	mov	r1, r6
8000b7e0:	e1a02005 	mov	r2, r5
8000b7e4:	e5840004 	str	r0, [r4, #4]
8000b7e8:	ebfffd04 	bl	8000ac00 <memcpy>
8000b7ec:	e3a03000 	mov	r3, #0
8000b7f0:	e5845008 	str	r5, [r4, #8]
8000b7f4:	e584500c 	str	r5, [r4, #12]
8000b7f8:	e5843010 	str	r3, [r4, #16]
8000b7fc:	e5843014 	str	r3, [r4, #20]
8000b800:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}

8000b804 <proto_new>:
8000b804:	e1a0c00d 	mov	ip, sp
8000b808:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000b80c:	e24cb004 	sub	fp, ip, #4
8000b810:	e1a06000 	mov	r6, r0
8000b814:	e3a00018 	mov	r0, #24
8000b818:	e1a05001 	mov	r5, r1
8000b81c:	eb000851 	bl	8000d968 <kmalloc>
8000b820:	e1a01006 	mov	r1, r6
8000b824:	e1a02005 	mov	r2, r5
8000b828:	e1a04000 	mov	r4, r0
8000b82c:	ebffffcf 	bl	8000b770 <proto_init>
8000b830:	e1a00004 	mov	r0, r4
8000b834:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}

8000b838 <proto_add>:
8000b838:	e1a0c00d 	mov	ip, sp
8000b83c:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
8000b840:	e24cb004 	sub	fp, ip, #4
8000b844:	e24dd00c 	sub	sp, sp, #12
8000b848:	e5903014 	ldr	r3, [r0, #20]
8000b84c:	e1a04000 	mov	r4, r0
8000b850:	e3530000 	cmp	r3, #0
8000b854:	e1a05001 	mov	r5, r1
8000b858:	e50b2020 	str	r2, [fp, #-32]
8000b85c:	1a000018 	bne	8000b8c4 <proto_add+0x8c>
8000b860:	e5900008 	ldr	r0, [r0, #8]
8000b864:	e594300c 	ldr	r3, [r4, #12]
8000b868:	e0802002 	add	r2, r0, r2
8000b86c:	e2821004 	add	r1, r2, #4
8000b870:	e1510003 	cmp	r1, r3
8000b874:	e5946004 	ldr	r6, [r4, #4]
8000b878:	2a000013 	bcs	8000b8cc <proto_add+0x94>
8000b87c:	e3a02004 	mov	r2, #4
8000b880:	e0860000 	add	r0, r6, r0
8000b884:	e24b1020 	sub	r1, fp, #32
8000b888:	ebfffcdc 	bl	8000ac00 <memcpy>
8000b88c:	e51b2020 	ldr	r2, [fp, #-32]
8000b890:	e3550000 	cmp	r5, #0
8000b894:	13520000 	cmpne	r2, #0
8000b898:	0a000005 	beq	8000b8b4 <proto_add+0x7c>
8000b89c:	e5940008 	ldr	r0, [r4, #8]
8000b8a0:	e1a01005 	mov	r1, r5
8000b8a4:	e2800004 	add	r0, r0, #4
8000b8a8:	e0860000 	add	r0, r6, r0
8000b8ac:	ebfffcd3 	bl	8000ac00 <memcpy>
8000b8b0:	e51b2020 	ldr	r2, [fp, #-32]
8000b8b4:	e5943008 	ldr	r3, [r4, #8]
8000b8b8:	e2833004 	add	r3, r3, #4
8000b8bc:	e0832002 	add	r2, r3, r2
8000b8c0:	e5842008 	str	r2, [r4, #8]
8000b8c4:	e24bd018 	sub	sp, fp, #24
8000b8c8:	e89da870 	ldm	sp, {r4, r5, r6, fp, sp, pc}
8000b8cc:	e2820084 	add	r0, r2, #132	; 0x84
8000b8d0:	e584000c 	str	r0, [r4, #12]
8000b8d4:	eb000823 	bl	8000d968 <kmalloc>
8000b8d8:	e5941004 	ldr	r1, [r4, #4]
8000b8dc:	e3510000 	cmp	r1, #0
8000b8e0:	e1a06000 	mov	r6, r0
8000b8e4:	0a000003 	beq	8000b8f8 <proto_add+0xc0>
8000b8e8:	e5942008 	ldr	r2, [r4, #8]
8000b8ec:	ebfffcc3 	bl	8000ac00 <memcpy>
8000b8f0:	e5940004 	ldr	r0, [r4, #4]
8000b8f4:	eb00082e 	bl	8000d9b4 <kfree>
8000b8f8:	e5940008 	ldr	r0, [r4, #8]
8000b8fc:	e5846004 	str	r6, [r4, #4]
8000b900:	eaffffdd 	b	8000b87c <proto_add+0x44>

8000b904 <proto_add_int>:
8000b904:	e1a0c00d 	mov	ip, sp
8000b908:	e92dd800 	push	{fp, ip, lr, pc}
8000b90c:	e24cb004 	sub	fp, ip, #4
8000b910:	e24b300c 	sub	r3, fp, #12
8000b914:	e24dd008 	sub	sp, sp, #8
8000b918:	e3a02004 	mov	r2, #4
8000b91c:	e5231004 	str	r1, [r3, #-4]!
8000b920:	e1a01003 	mov	r1, r3
8000b924:	ebffffc3 	bl	8000b838 <proto_add>
8000b928:	e24bd00c 	sub	sp, fp, #12
8000b92c:	e89da800 	ldm	sp, {fp, sp, pc}

8000b930 <proto_add_str>:
8000b930:	e1a0c00d 	mov	ip, sp
8000b934:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000b938:	e1a05000 	mov	r5, r0
8000b93c:	e24cb004 	sub	fp, ip, #4
8000b940:	e1a00001 	mov	r0, r1
8000b944:	e1a04001 	mov	r4, r1
8000b948:	ebfffdca 	bl	8000b078 <strlen>
8000b94c:	e1a01004 	mov	r1, r4
8000b950:	e2802001 	add	r2, r0, #1
8000b954:	e1a00005 	mov	r0, r5
8000b958:	e24bd014 	sub	sp, fp, #20
8000b95c:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
8000b960:	eaffffb4 	b	8000b838 <proto_add>

8000b964 <proto_read>:
8000b964:	e1a0c00d 	mov	ip, sp
8000b968:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
8000b96c:	e2516000 	subs	r6, r1, #0
8000b970:	13a03000 	movne	r3, #0
8000b974:	e24cb004 	sub	fp, ip, #4
8000b978:	e24dd00c 	sub	sp, sp, #12
8000b97c:	15863000 	strne	r3, [r6]
8000b980:	e5903004 	ldr	r3, [r0, #4]
8000b984:	e1a05000 	mov	r5, r0
8000b988:	e3530000 	cmp	r3, #0
8000b98c:	0a000016 	beq	8000b9ec <proto_read+0x88>
8000b990:	e5902008 	ldr	r2, [r0, #8]
8000b994:	e3520000 	cmp	r2, #0
8000b998:	0a000013 	beq	8000b9ec <proto_read+0x88>
8000b99c:	e5904010 	ldr	r4, [r0, #16]
8000b9a0:	e1520004 	cmp	r2, r4
8000b9a4:	9a000010 	bls	8000b9ec <proto_read+0x88>
8000b9a8:	e0834004 	add	r4, r3, r4
8000b9ac:	e3a02004 	mov	r2, #4
8000b9b0:	e1a01004 	mov	r1, r4
8000b9b4:	e24b0020 	sub	r0, fp, #32
8000b9b8:	ebfffc90 	bl	8000ac00 <memcpy>
8000b9bc:	e5953010 	ldr	r3, [r5, #16]
8000b9c0:	e51b2020 	ldr	r2, [fp, #-32]
8000b9c4:	e2833004 	add	r3, r3, #4
8000b9c8:	e3560000 	cmp	r6, #0
8000b9cc:	e0833002 	add	r3, r3, r2
8000b9d0:	e5853010 	str	r3, [r5, #16]
8000b9d4:	15862000 	strne	r2, [r6]
8000b9d8:	e3520000 	cmp	r2, #0
8000b9dc:	0a000002 	beq	8000b9ec <proto_read+0x88>
8000b9e0:	e2840004 	add	r0, r4, #4
8000b9e4:	e24bd018 	sub	sp, fp, #24
8000b9e8:	e89da870 	ldm	sp, {r4, r5, r6, fp, sp, pc}
8000b9ec:	e3a00000 	mov	r0, #0
8000b9f0:	e24bd018 	sub	sp, fp, #24
8000b9f4:	e89da870 	ldm	sp, {r4, r5, r6, fp, sp, pc}

8000b9f8 <proto_read_to>:
8000b9f8:	e1a0c00d 	mov	ip, sp
8000b9fc:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000ba00:	e24cb004 	sub	fp, ip, #4
8000ba04:	e24dd008 	sub	sp, sp, #8
8000ba08:	e1a05001 	mov	r5, r1
8000ba0c:	e24b1018 	sub	r1, fp, #24
8000ba10:	e1a04002 	mov	r4, r2
8000ba14:	ebffffd2 	bl	8000b964 <proto_read>
8000ba18:	e51b3018 	ldr	r3, [fp, #-24]
8000ba1c:	e1530004 	cmp	r3, r4
8000ba20:	c50b4018 	strgt	r4, [fp, #-24]
8000ba24:	e3550000 	cmp	r5, #0
8000ba28:	13500000 	cmpne	r0, #0
8000ba2c:	e1a01000 	mov	r1, r0
8000ba30:	03a02000 	moveq	r2, #0
8000ba34:	0a000002 	beq	8000ba44 <proto_read_to+0x4c>
8000ba38:	e51b2018 	ldr	r2, [fp, #-24]
8000ba3c:	e3520000 	cmp	r2, #0
8000ba40:	1a000002 	bne	8000ba50 <proto_read_to+0x58>
8000ba44:	e1a00002 	mov	r0, r2
8000ba48:	e24bd014 	sub	sp, fp, #20
8000ba4c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000ba50:	e1a00005 	mov	r0, r5
8000ba54:	ebfffc69 	bl	8000ac00 <memcpy>
8000ba58:	e51b2018 	ldr	r2, [fp, #-24]
8000ba5c:	e1a00002 	mov	r0, r2
8000ba60:	e24bd014 	sub	sp, fp, #20
8000ba64:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

8000ba68 <proto_read_int>:
8000ba68:	e1a0c00d 	mov	ip, sp
8000ba6c:	e92dd800 	push	{fp, ip, lr, pc}
8000ba70:	e3a01000 	mov	r1, #0
8000ba74:	e24cb004 	sub	fp, ip, #4
8000ba78:	ebffffb9 	bl	8000b964 <proto_read>
8000ba7c:	e3500000 	cmp	r0, #0
8000ba80:	15900000 	ldrne	r0, [r0]
8000ba84:	e89da800 	ldm	sp, {fp, sp, pc}

8000ba88 <proto_read_str>:
8000ba88:	e3a01000 	mov	r1, #0
8000ba8c:	eaffffb4 	b	8000b964 <proto_read>

8000ba90 <proto_clear>:
8000ba90:	e1a0c00d 	mov	ip, sp
8000ba94:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000ba98:	e5903014 	ldr	r3, [r0, #20]
8000ba9c:	e24cb004 	sub	fp, ip, #4
8000baa0:	e3530000 	cmp	r3, #0
8000baa4:	e1a04000 	mov	r4, r0
8000baa8:	189da818 	ldmne	sp, {r3, r4, fp, sp, pc}
8000baac:	e5900004 	ldr	r0, [r0, #4]
8000bab0:	e5843008 	str	r3, [r4, #8]
8000bab4:	e3500000 	cmp	r0, #0
8000bab8:	e584300c 	str	r3, [r4, #12]
8000babc:	e5843010 	str	r3, [r4, #16]
8000bac0:	0a000000 	beq	8000bac8 <proto_clear+0x38>
8000bac4:	eb0007ba 	bl	8000d9b4 <kfree>
8000bac8:	e3a03000 	mov	r3, #0
8000bacc:	e5843004 	str	r3, [r4, #4]
8000bad0:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}

8000bad4 <proto_free>:
8000bad4:	e1a0c00d 	mov	ip, sp
8000bad8:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000badc:	e24cb004 	sub	fp, ip, #4
8000bae0:	e1a04000 	mov	r4, r0
8000bae4:	ebffffe9 	bl	8000ba90 <proto_clear>
8000bae8:	e1a00004 	mov	r0, r4
8000baec:	e24bd014 	sub	sp, fp, #20
8000baf0:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
8000baf4:	ea0007ae 	b	8000d9b4 <kfree>

8000baf8 <str_reset>:
8000baf8:	e1a0c00d 	mov	ip, sp
8000bafc:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000bb00:	e5903000 	ldr	r3, [r0]
8000bb04:	e24cb004 	sub	fp, ip, #4
8000bb08:	e3530000 	cmp	r3, #0
8000bb0c:	e1a04000 	mov	r4, r0
8000bb10:	0a000003 	beq	8000bb24 <str_reset+0x2c>
8000bb14:	e3a02000 	mov	r2, #0
8000bb18:	e5c32000 	strb	r2, [r3]
8000bb1c:	e1c420b6 	strh	r2, [r4, #6]
8000bb20:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000bb24:	e3a00010 	mov	r0, #16
8000bb28:	eb00078e 	bl	8000d968 <kmalloc>
8000bb2c:	e3a02010 	mov	r2, #16
8000bb30:	e1c420b4 	strh	r2, [r4, #4]
8000bb34:	e1a03000 	mov	r3, r0
8000bb38:	e5840000 	str	r0, [r4]
8000bb3c:	eafffff4 	b	8000bb14 <str_reset+0x1c>

8000bb40 <str_ncpy>:
8000bb40:	e1a0c00d 	mov	ip, sp
8000bb44:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000bb48:	e2515000 	subs	r5, r1, #0
8000bb4c:	e24cb004 	sub	fp, ip, #4
8000bb50:	e1a04000 	mov	r4, r0
8000bb54:	e1a06002 	mov	r6, r2
8000bb58:	0a000003 	beq	8000bb6c <str_ncpy+0x2c>
8000bb5c:	e5d53000 	ldrb	r3, [r5]
8000bb60:	e3520000 	cmp	r2, #0
8000bb64:	13530000 	cmpne	r3, #0
8000bb68:	1a000003 	bne	8000bb7c <str_ncpy+0x3c>
8000bb6c:	e1a00004 	mov	r0, r4
8000bb70:	ebffffe0 	bl	8000baf8 <str_reset>
8000bb74:	e5940000 	ldr	r0, [r4]
8000bb78:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000bb7c:	e1a00005 	mov	r0, r5
8000bb80:	ebfffd3c 	bl	8000b078 <strlen>
8000bb84:	e1d410b4 	ldrh	r1, [r4, #4]
8000bb88:	e1500006 	cmp	r0, r6
8000bb8c:	31a06000 	movcc	r6, r0
8000bb90:	e1510006 	cmp	r1, r6
8000bb94:	9a000009 	bls	8000bbc0 <str_ncpy+0x80>
8000bb98:	e5940000 	ldr	r0, [r4]
8000bb9c:	e1a02006 	mov	r2, r6
8000bba0:	e1a01005 	mov	r1, r5
8000bba4:	ebfffc6e 	bl	8000ad64 <strncpy>
8000bba8:	e5943000 	ldr	r3, [r4]
8000bbac:	e3a02000 	mov	r2, #0
8000bbb0:	e7c32006 	strb	r2, [r3, r6]
8000bbb4:	e1c460b6 	strh	r6, [r4, #6]
8000bbb8:	e5940000 	ldr	r0, [r4]
8000bbbc:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000bbc0:	e2867010 	add	r7, r6, #16
8000bbc4:	e1a02007 	mov	r2, r7
8000bbc8:	e5940000 	ldr	r0, [r4]
8000bbcc:	eb00077f 	bl	8000d9d0 <krealloc_raw>
8000bbd0:	e1c470b4 	strh	r7, [r4, #4]
8000bbd4:	e1a03000 	mov	r3, r0
8000bbd8:	e5843000 	str	r3, [r4]
8000bbdc:	eaffffee 	b	8000bb9c <str_ncpy+0x5c>

8000bbe0 <str_cpy>:
8000bbe0:	e1a0c00d 	mov	ip, sp
8000bbe4:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000bbe8:	e59f2010 	ldr	r2, [pc, #16]	; 8000bc00 <str_cpy+0x20>
8000bbec:	e1a04000 	mov	r4, r0
8000bbf0:	e24cb004 	sub	fp, ip, #4
8000bbf4:	ebffffd1 	bl	8000bb40 <str_ncpy>
8000bbf8:	e5940000 	ldr	r0, [r4]
8000bbfc:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000bc00:	0000ffff 	strdeq	pc, [r0], -pc	; <UNPREDICTABLE>

8000bc04 <str_new>:
8000bc04:	e1a0c00d 	mov	ip, sp
8000bc08:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000bc0c:	e24cb004 	sub	fp, ip, #4
8000bc10:	e1a05000 	mov	r5, r0
8000bc14:	e3a00008 	mov	r0, #8
8000bc18:	eb000752 	bl	8000d968 <kmalloc>
8000bc1c:	e3a03000 	mov	r3, #0
8000bc20:	e1a01005 	mov	r1, r5
8000bc24:	e1a04000 	mov	r4, r0
8000bc28:	e5803000 	str	r3, [r0]
8000bc2c:	e1c030b4 	strh	r3, [r0, #4]
8000bc30:	e1c030b6 	strh	r3, [r0, #6]
8000bc34:	ebffffe9 	bl	8000bbe0 <str_cpy>
8000bc38:	e1a00004 	mov	r0, r4
8000bc3c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

8000bc40 <str_new_by_size>:
8000bc40:	e1a0c00d 	mov	ip, sp
8000bc44:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000bc48:	e1a05000 	mov	r5, r0
8000bc4c:	e24cb004 	sub	fp, ip, #4
8000bc50:	e3a00008 	mov	r0, #8
8000bc54:	eb000743 	bl	8000d968 <kmalloc>
8000bc58:	e1a04000 	mov	r4, r0
8000bc5c:	e1a00005 	mov	r0, r5
8000bc60:	eb000740 	bl	8000d968 <kmalloc>
8000bc64:	e3a03000 	mov	r3, #0
8000bc68:	e1c450b4 	strh	r5, [r4, #4]
8000bc6c:	e1a02000 	mov	r2, r0
8000bc70:	e5840000 	str	r0, [r4]
8000bc74:	e5c23000 	strb	r3, [r2]
8000bc78:	e1a00004 	mov	r0, r4
8000bc7c:	e1c430b6 	strh	r3, [r4, #6]
8000bc80:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

8000bc84 <str_add>:
8000bc84:	e1a0c00d 	mov	ip, sp
8000bc88:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000bc8c:	e2515000 	subs	r5, r1, #0
8000bc90:	e24cb004 	sub	fp, ip, #4
8000bc94:	e1a04000 	mov	r4, r0
8000bc98:	0a000002 	beq	8000bca8 <str_add+0x24>
8000bc9c:	e5d53000 	ldrb	r3, [r5]
8000bca0:	e3530000 	cmp	r3, #0
8000bca4:	1a000001 	bne	8000bcb0 <str_add+0x2c>
8000bca8:	e5940000 	ldr	r0, [r4]
8000bcac:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000bcb0:	e1a00005 	mov	r0, r5
8000bcb4:	ebfffcef 	bl	8000b078 <strlen>
8000bcb8:	e1d410b4 	ldrh	r1, [r4, #4]
8000bcbc:	e1a06000 	mov	r6, r0
8000bcc0:	e1d400b6 	ldrh	r0, [r4, #6]
8000bcc4:	e0862000 	add	r2, r6, r0
8000bcc8:	e1520001 	cmp	r2, r1
8000bccc:	2a00000c 	bcs	8000bd04 <str_add+0x80>
8000bcd0:	e5943000 	ldr	r3, [r4]
8000bcd4:	e1a01005 	mov	r1, r5
8000bcd8:	e0830000 	add	r0, r3, r0
8000bcdc:	ebfffc13 	bl	8000ad30 <strcpy>
8000bce0:	e1d420b6 	ldrh	r2, [r4, #6]
8000bce4:	e5943000 	ldr	r3, [r4]
8000bce8:	e0866002 	add	r6, r6, r2
8000bcec:	e1a02806 	lsl	r2, r6, #16
8000bcf0:	e3a01000 	mov	r1, #0
8000bcf4:	e1c460b6 	strh	r6, [r4, #6]
8000bcf8:	e7c31822 	strb	r1, [r3, r2, lsr #16]
8000bcfc:	e5940000 	ldr	r0, [r4]
8000bd00:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000bd04:	e2827010 	add	r7, r2, #16
8000bd08:	e1a02007 	mov	r2, r7
8000bd0c:	e5940000 	ldr	r0, [r4]
8000bd10:	eb00072e 	bl	8000d9d0 <krealloc_raw>
8000bd14:	e1c470b4 	strh	r7, [r4, #4]
8000bd18:	e1a02000 	mov	r2, r0
8000bd1c:	e1a03002 	mov	r3, r2
8000bd20:	e1d400b6 	ldrh	r0, [r4, #6]
8000bd24:	e5842000 	str	r2, [r4]
8000bd28:	eaffffe9 	b	8000bcd4 <str_add+0x50>

8000bd2c <str_addc>:
8000bd2c:	e1a0c00d 	mov	ip, sp
8000bd30:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000bd34:	e1d030b6 	ldrh	r3, [r0, #6]
8000bd38:	e1d020b4 	ldrh	r2, [r0, #4]
8000bd3c:	e1a04000 	mov	r4, r0
8000bd40:	e2830001 	add	r0, r3, #1
8000bd44:	e1500002 	cmp	r0, r2
8000bd48:	e24cb004 	sub	fp, ip, #4
8000bd4c:	e1a05001 	mov	r5, r1
8000bd50:	2a00000a 	bcs	8000bd80 <str_addc+0x54>
8000bd54:	e5942000 	ldr	r2, [r4]
8000bd58:	e7c25003 	strb	r5, [r2, r3]
8000bd5c:	e1d430b6 	ldrh	r3, [r4, #6]
8000bd60:	e5942000 	ldr	r2, [r4]
8000bd64:	e2833001 	add	r3, r3, #1
8000bd68:	e1a01803 	lsl	r1, r3, #16
8000bd6c:	e3a00000 	mov	r0, #0
8000bd70:	e1c430b6 	strh	r3, [r4, #6]
8000bd74:	e7c20821 	strb	r0, [r2, r1, lsr #16]
8000bd78:	e5940000 	ldr	r0, [r4]
8000bd7c:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000bd80:	e2836010 	add	r6, r3, #16
8000bd84:	e1a01002 	mov	r1, r2
8000bd88:	e5940000 	ldr	r0, [r4]
8000bd8c:	e1a02006 	mov	r2, r6
8000bd90:	eb00070e 	bl	8000d9d0 <krealloc_raw>
8000bd94:	e1d430b6 	ldrh	r3, [r4, #6]
8000bd98:	e1c460b4 	strh	r6, [r4, #4]
8000bd9c:	e1a02000 	mov	r2, r0
8000bda0:	e5840000 	str	r0, [r4]
8000bda4:	eaffffeb 	b	8000bd58 <str_addc+0x2c>

8000bda8 <str_free>:
8000bda8:	e1a0c00d 	mov	ip, sp
8000bdac:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000bdb0:	e2504000 	subs	r4, r0, #0
8000bdb4:	e24cb004 	sub	fp, ip, #4
8000bdb8:	089da818 	ldmeq	sp, {r3, r4, fp, sp, pc}
8000bdbc:	e5940000 	ldr	r0, [r4]
8000bdc0:	e3500000 	cmp	r0, #0
8000bdc4:	0a000000 	beq	8000bdcc <str_free+0x24>
8000bdc8:	eb0006f9 	bl	8000d9b4 <kfree>
8000bdcc:	e1a00004 	mov	r0, r4
8000bdd0:	e24bd014 	sub	sp, fp, #20
8000bdd4:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
8000bdd8:	ea0006f5 	b	8000d9b4 <kfree>

8000bddc <str_from_int>:
8000bddc:	e1a0c00d 	mov	ip, sp
8000bde0:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
8000bde4:	e59f80b4 	ldr	r8, [pc, #180]	; 8000bea0 <str_from_int+0xc4>
8000bde8:	e2413002 	sub	r3, r1, #2
8000bdec:	e59f60b0 	ldr	r6, [pc, #176]	; 8000bea4 <str_from_int+0xc8>
8000bdf0:	e3530022 	cmp	r3, #34	; 0x22
8000bdf4:	e1a04001 	mov	r4, r1
8000bdf8:	83a0500a 	movhi	r5, #10
8000bdfc:	e24cb004 	sub	fp, ip, #4
8000be00:	e1a07000 	mov	r7, r0
8000be04:	81a04005 	movhi	r4, r5
8000be08:	91a05004 	movls	r5, r4
8000be0c:	e08f8008 	add	r8, pc, r8
8000be10:	e08f6006 	add	r6, pc, r6
8000be14:	ea000001 	b	8000be20 <str_from_int+0x44>
8000be18:	e1a08003 	mov	r8, r3
8000be1c:	e1a07000 	mov	r7, r0
8000be20:	e1a00007 	mov	r0, r7
8000be24:	e1a01005 	mov	r1, r5
8000be28:	eb0000f1 	bl	8000c1f4 <div_u32>
8000be2c:	e0030094 	mul	r3, r4, r0
8000be30:	e3500000 	cmp	r0, #0
8000be34:	e0633007 	rsb	r3, r3, r7
8000be38:	e0863003 	add	r3, r6, r3
8000be3c:	e5d32023 	ldrb	r2, [r3, #35]	; 0x23
8000be40:	e1a03008 	mov	r3, r8
8000be44:	e4c32001 	strb	r2, [r3], #1
8000be48:	1afffff2 	bne	8000be18 <str_from_int+0x3c>
8000be4c:	e3570000 	cmp	r7, #0
8000be50:	b3a0202d 	movlt	r2, #45	; 0x2d
8000be54:	b5c82001 	strblt	r2, [r8, #1]
8000be58:	e59f2048 	ldr	r2, [pc, #72]	; 8000bea8 <str_from_int+0xcc>
8000be5c:	b2883002 	addlt	r3, r8, #2
8000be60:	e08f2002 	add	r2, pc, r2
8000be64:	e2431001 	sub	r1, r3, #1
8000be68:	e3a00000 	mov	r0, #0
8000be6c:	e1510002 	cmp	r1, r2
8000be70:	e5c30000 	strb	r0, [r3]
8000be74:	9a000006 	bls	8000be94 <str_from_int+0xb8>
8000be78:	e1a03002 	mov	r3, r2
8000be7c:	e5d12000 	ldrb	r2, [r1]
8000be80:	e5d30000 	ldrb	r0, [r3]
8000be84:	e4410001 	strb	r0, [r1], #-1
8000be88:	e4c32001 	strb	r2, [r3], #1
8000be8c:	e1510003 	cmp	r1, r3
8000be90:	8afffff9 	bhi	8000be7c <str_from_int+0xa0>
8000be94:	e59f0010 	ldr	r0, [pc, #16]	; 8000beac <str_from_int+0xd0>
8000be98:	e08f0000 	add	r0, pc, r0
8000be9c:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000bea0:	00014668 	andeq	r4, r1, r8, ror #12
8000bea4:	000061c0 	andeq	r6, r0, r0, asr #3
8000bea8:	00014614 	andeq	r4, r1, r4, lsl r6
8000beac:	000145dc 	ldrdeq	r4, [r1], -ip

8000beb0 <str_addc_int>:
8000beb0:	e1a0c00d 	mov	ip, sp
8000beb4:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000beb8:	e1a04000 	mov	r4, r0
8000bebc:	e24cb004 	sub	fp, ip, #4
8000bec0:	e1a00001 	mov	r0, r1
8000bec4:	e1a01002 	mov	r1, r2
8000bec8:	ebffffc3 	bl	8000bddc <str_from_int>
8000becc:	e1a01000 	mov	r1, r0
8000bed0:	e1a00004 	mov	r0, r4
8000bed4:	e24bd014 	sub	sp, fp, #20
8000bed8:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
8000bedc:	eaffff68 	b	8000bc84 <str_add>

8000bee0 <str_from_bool>:
8000bee0:	e3500000 	cmp	r0, #0
8000bee4:	0a000002 	beq	8000bef4 <str_from_bool+0x14>
8000bee8:	e59f0010 	ldr	r0, [pc, #16]	; 8000bf00 <str_from_bool+0x20>
8000beec:	e08f0000 	add	r0, pc, r0
8000bef0:	e12fff1e 	bx	lr
8000bef4:	e59f0008 	ldr	r0, [pc, #8]	; 8000bf04 <str_from_bool+0x24>
8000bef8:	e08f0000 	add	r0, pc, r0
8000befc:	e12fff1e 	bx	lr
8000bf00:	00006134 	andeq	r6, r0, r4, lsr r1
8000bf04:	00006120 	andeq	r6, r0, r0, lsr #2

8000bf08 <str_to_int>:
8000bf08:	e59f1130 	ldr	r1, [pc, #304]	; 8000c040 <str_to_int+0x138>
8000bf0c:	e1a0c00d 	mov	ip, sp
8000bf10:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000bf14:	e08f1001 	add	r1, pc, r1
8000bf18:	e24cb004 	sub	fp, ip, #4
8000bf1c:	e1a04000 	mov	r4, r0
8000bf20:	ebfffc0e 	bl	8000af60 <strstr>
8000bf24:	e5d43000 	ldrb	r3, [r4]
8000bf28:	e3500000 	cmp	r0, #0
8000bf2c:	0a00002f 	beq	8000bff0 <str_to_int+0xe8>
8000bf30:	e3530000 	cmp	r3, #0
8000bf34:	0a00002b 	beq	8000bfe8 <str_to_int+0xe0>
8000bf38:	e3a0c000 	mov	ip, #0
8000bf3c:	e1a01004 	mov	r1, r4
8000bf40:	e1a0500c 	mov	r5, ip
8000bf44:	e1a0200c 	mov	r2, ip
8000bf48:	e1a0000c 	mov	r0, ip
8000bf4c:	ea000017 	b	8000bfb0 <str_to_int+0xa8>
8000bf50:	e3530030 	cmp	r3, #48	; 0x30
8000bf54:	0a00001e 	beq	8000bfd4 <str_to_int+0xcc>
8000bf58:	e243e030 	sub	lr, r3, #48	; 0x30
8000bf5c:	e20e40ff 	and	r4, lr, #255	; 0xff
8000bf60:	e3540009 	cmp	r4, #9
8000bf64:	9a000004 	bls	8000bf7c <str_to_int+0x74>
8000bf68:	e3c34020 	bic	r4, r3, #32
8000bf6c:	e2444041 	sub	r4, r4, #65	; 0x41
8000bf70:	e3540005 	cmp	r4, #5
8000bf74:	83a05001 	movhi	r5, #1
8000bf78:	8a000007 	bhi	8000bf9c <str_to_int+0x94>
8000bf7c:	e243c041 	sub	ip, r3, #65	; 0x41
8000bf80:	e35c0005 	cmp	ip, #5
8000bf84:	81a0c00e 	movhi	ip, lr
8000bf88:	e243e061 	sub	lr, r3, #97	; 0x61
8000bf8c:	9243c037 	subls	ip, r3, #55	; 0x37
8000bf90:	e35e0005 	cmp	lr, #5
8000bf94:	9243c057 	subls	ip, r3, #87	; 0x57
8000bf98:	e1a00200 	lsl	r0, r0, #4
8000bf9c:	e5f13001 	ldrb	r3, [r1, #1]!
8000bfa0:	e2822001 	add	r2, r2, #1
8000bfa4:	e3530000 	cmp	r3, #0
8000bfa8:	e080000c 	add	r0, r0, ip
8000bfac:	0a00000a 	beq	8000bfdc <str_to_int+0xd4>
8000bfb0:	e3520000 	cmp	r2, #0
8000bfb4:	0affffe5 	beq	8000bf50 <str_to_int+0x48>
8000bfb8:	e3520001 	cmp	r2, #1
8000bfbc:	1affffe5 	bne	8000bf58 <str_to_int+0x50>
8000bfc0:	e203e0df 	and	lr, r3, #223	; 0xdf
8000bfc4:	e35e0058 	cmp	lr, #88	; 0x58
8000bfc8:	03a0c000 	moveq	ip, #0
8000bfcc:	0afffff2 	beq	8000bf9c <str_to_int+0x94>
8000bfd0:	eaffffe0 	b	8000bf58 <str_to_int+0x50>
8000bfd4:	e1a0c002 	mov	ip, r2
8000bfd8:	eaffffef 	b	8000bf9c <str_to_int+0x94>
8000bfdc:	e3550000 	cmp	r5, #0
8000bfe0:	13a00000 	movne	r0, #0
8000bfe4:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000bfe8:	e1a00003 	mov	r0, r3
8000bfec:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000bff0:	e3530000 	cmp	r3, #0
8000bff4:	11a02000 	movne	r2, r0
8000bff8:	11a01004 	movne	r1, r4
8000bffc:	11a0e002 	movne	lr, r2
8000c000:	11a00002 	movne	r0, r2
8000c004:	0afffff7 	beq	8000bfe8 <str_to_int+0xe0>
8000c008:	e2433030 	sub	r3, r3, #48	; 0x30
8000c00c:	e203c0ff 	and	ip, r3, #255	; 0xff
8000c010:	e35c0009 	cmp	ip, #9
8000c014:	91a02003 	movls	r2, r3
8000c018:	e5f13001 	ldrb	r3, [r1, #1]!
8000c01c:	e080c100 	add	ip, r0, r0, lsl #2
8000c020:	91a0008c 	lslls	r0, ip, #1
8000c024:	83a0e001 	movhi	lr, #1
8000c028:	e3530000 	cmp	r3, #0
8000c02c:	e0800002 	add	r0, r0, r2
8000c030:	1afffff4 	bne	8000c008 <str_to_int+0x100>
8000c034:	e35e0000 	cmp	lr, #0
8000c038:	13a00000 	movne	r0, #0
8000c03c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000c040:	00006114 	andeq	r6, r0, r4, lsl r1

8000c044 <str_to>:
8000c044:	e1a0c00d 	mov	ip, sp
8000c048:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
8000c04c:	e2526000 	subs	r6, r2, #0
8000c050:	e24cb004 	sub	fp, ip, #4
8000c054:	e1a05000 	mov	r5, r0
8000c058:	e1a07001 	mov	r7, r1
8000c05c:	e1a08003 	mov	r8, r3
8000c060:	0a000001 	beq	8000c06c <str_to+0x28>
8000c064:	e1a00006 	mov	r0, r6
8000c068:	ebfffea2 	bl	8000baf8 <str_reset>
8000c06c:	e3a04000 	mov	r4, #0
8000c070:	e4d5c001 	ldrb	ip, [r5], #1
8000c074:	e1a02004 	mov	r2, r4
8000c078:	e35c0000 	cmp	ip, #0
8000c07c:	e1a00006 	mov	r0, r6
8000c080:	e1a0100c 	mov	r1, ip
8000c084:	e2844001 	add	r4, r4, #1
8000c088:	0a00000a 	beq	8000c0b8 <str_to+0x74>
8000c08c:	e3580000 	cmp	r8, #0
8000c090:	0a000002 	beq	8000c0a0 <str_to+0x5c>
8000c094:	e35c0020 	cmp	ip, #32
8000c098:	135c0009 	cmpne	ip, #9
8000c09c:	0afffff3 	beq	8000c070 <str_to+0x2c>
8000c0a0:	e15c0007 	cmp	ip, r7
8000c0a4:	0a000005 	beq	8000c0c0 <str_to+0x7c>
8000c0a8:	e3560000 	cmp	r6, #0
8000c0ac:	0affffef 	beq	8000c070 <str_to+0x2c>
8000c0b0:	ebffff1d 	bl	8000bd2c <str_addc>
8000c0b4:	eaffffed 	b	8000c070 <str_to+0x2c>
8000c0b8:	e3e00000 	mvn	r0, #0
8000c0bc:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000c0c0:	e3560000 	cmp	r6, #0
8000c0c4:	13580000 	cmpne	r8, #0
8000c0c8:	0a00000a 	beq	8000c0f8 <str_to+0xb4>
8000c0cc:	e1d630b6 	ldrh	r3, [r6, #6]
8000c0d0:	e2533001 	subs	r3, r3, #1
8000c0d4:	3a000007 	bcc	8000c0f8 <str_to+0xb4>
8000c0d8:	e3a0c000 	mov	ip, #0
8000c0dc:	e5960000 	ldr	r0, [r6]
8000c0e0:	e7d01003 	ldrb	r1, [r0, r3]
8000c0e4:	e3510020 	cmp	r1, #32
8000c0e8:	13510009 	cmpne	r1, #9
8000c0ec:	07c0c003 	strbeq	ip, [r0, r3]
8000c0f0:	e2533001 	subs	r3, r3, #1
8000c0f4:	2afffff8 	bcs	8000c0dc <str_to+0x98>
8000c0f8:	e1a00002 	mov	r0, r2
8000c0fc:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}

8000c100 <charbuf_push>:
8000c100:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
8000c104:	e250e000 	subs	lr, r0, #0
8000c108:	0a000024 	beq	8000c1a0 <charbuf_push+0xa0>
8000c10c:	e59ec084 	ldr	ip, [lr, #132]	; 0x84
8000c110:	e35c0080 	cmp	ip, #128	; 0x80
8000c114:	0a000010 	beq	8000c15c <charbuf_push+0x5c>
8000c118:	e59e3080 	ldr	r3, [lr, #128]	; 0x80
8000c11c:	e08c3003 	add	r3, ip, r3
8000c120:	e353007f 	cmp	r3, #127	; 0x7f
8000c124:	8a000006 	bhi	8000c144 <charbuf_push+0x44>
8000c128:	e35c007f 	cmp	ip, #127	; 0x7f
8000c12c:	e7ce1003 	strb	r1, [lr, r3]
8000c130:	8a000007 	bhi	8000c154 <charbuf_push+0x54>
8000c134:	e28cc001 	add	ip, ip, #1
8000c138:	e58ec084 	str	ip, [lr, #132]	; 0x84
8000c13c:	e3a00000 	mov	r0, #0
8000c140:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)
8000c144:	e2433080 	sub	r3, r3, #128	; 0x80
8000c148:	e35c007f 	cmp	ip, #127	; 0x7f
8000c14c:	e7ce1003 	strb	r1, [lr, r3]
8000c150:	9afffff7 	bls	8000c134 <charbuf_push+0x34>
8000c154:	e3a00000 	mov	r0, #0
8000c158:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)
8000c15c:	e3520000 	cmp	r2, #0
8000c160:	0a00000e 	beq	8000c1a0 <charbuf_push+0xa0>
8000c164:	e59e3080 	ldr	r3, [lr, #128]	; 0x80
8000c168:	e2832001 	add	r2, r3, #1
8000c16c:	e3520080 	cmp	r2, #128	; 0x80
8000c170:	0a000006 	beq	8000c190 <charbuf_push+0x90>
8000c174:	e2833080 	add	r3, r3, #128	; 0x80
8000c178:	e353007f 	cmp	r3, #127	; 0x7f
8000c17c:	e58e2080 	str	r2, [lr, #128]	; 0x80
8000c180:	8affffef 	bhi	8000c144 <charbuf_push+0x44>
8000c184:	e3a00000 	mov	r0, #0
8000c188:	e7ce1003 	strb	r1, [lr, r3]
8000c18c:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)
8000c190:	e3a03000 	mov	r3, #0
8000c194:	e58e3080 	str	r3, [lr, #128]	; 0x80
8000c198:	e3a0307f 	mov	r3, #127	; 0x7f
8000c19c:	eafffff8 	b	8000c184 <charbuf_push+0x84>
8000c1a0:	e3e00000 	mvn	r0, #0
8000c1a4:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)

8000c1a8 <charbuf_pop>:
8000c1a8:	e5902084 	ldr	r2, [r0, #132]	; 0x84
8000c1ac:	e1a03000 	mov	r3, r0
8000c1b0:	e3520000 	cmp	r2, #0
8000c1b4:	0a00000c 	beq	8000c1ec <charbuf_pop+0x44>
8000c1b8:	e5902080 	ldr	r2, [r0, #128]	; 0x80
8000c1bc:	e7d02002 	ldrb	r2, [r0, r2]
8000c1c0:	e5c12000 	strb	r2, [r1]
8000c1c4:	e5902080 	ldr	r2, [r0, #128]	; 0x80
8000c1c8:	e2822001 	add	r2, r2, #1
8000c1cc:	e3520080 	cmp	r2, #128	; 0x80
8000c1d0:	03a02000 	moveq	r2, #0
8000c1d4:	e5802080 	str	r2, [r0, #128]	; 0x80
8000c1d8:	e5902084 	ldr	r2, [r0, #132]	; 0x84
8000c1dc:	e3a00000 	mov	r0, #0
8000c1e0:	e2422001 	sub	r2, r2, #1
8000c1e4:	e5832084 	str	r2, [r3, #132]	; 0x84
8000c1e8:	e12fff1e 	bx	lr
8000c1ec:	e3e00000 	mvn	r0, #0
8000c1f0:	e12fff1e 	bx	lr

8000c1f4 <div_u32>:
8000c1f4:	e3510000 	cmp	r1, #0
8000c1f8:	0a00001e 	beq	8000c278 <div_u32+0x84>
8000c1fc:	e3510902 	cmp	r1, #32768	; 0x8000
8000c200:	0a000087 	beq	8000c424 <div_u32+0x230>
8000c204:	9a00000d 	bls	8000c240 <div_u32+0x4c>
8000c208:	e3510502 	cmp	r1, #8388608	; 0x800000
8000c20c:	0a000080 	beq	8000c414 <div_u32+0x220>
8000c210:	8a00001a 	bhi	8000c280 <div_u32+0x8c>
8000c214:	e3510702 	cmp	r1, #524288	; 0x80000
8000c218:	0a00006d 	beq	8000c3d4 <div_u32+0x1e0>
8000c21c:	8a000035 	bhi	8000c2f8 <div_u32+0x104>
8000c220:	e3510802 	cmp	r1, #131072	; 0x20000
8000c224:	0a000052 	beq	8000c374 <div_u32+0x180>
8000c228:	e3510701 	cmp	r1, #262144	; 0x40000
8000c22c:	0a00006e 	beq	8000c3ec <div_u32+0x1f8>
8000c230:	e3510801 	cmp	r1, #65536	; 0x10000
8000c234:	1a00003d 	bne	8000c330 <div_u32+0x13c>
8000c238:	e1a00820 	lsr	r0, r0, #16
8000c23c:	e12fff1e 	bx	lr
8000c240:	e3510080 	cmp	r1, #128	; 0x80
8000c244:	0a000074 	beq	8000c41c <div_u32+0x228>
8000c248:	9a000017 	bls	8000c2ac <div_u32+0xb8>
8000c24c:	e3510b02 	cmp	r1, #2048	; 0x800
8000c250:	0a00006d 	beq	8000c40c <div_u32+0x218>
8000c254:	8a00001f 	bhi	8000c2d8 <div_u32+0xe4>
8000c258:	e3510c02 	cmp	r1, #512	; 0x200
8000c25c:	0a000046 	beq	8000c37c <div_u32+0x188>
8000c260:	e3510b01 	cmp	r1, #1024	; 0x400
8000c264:	0a000058 	beq	8000c3cc <div_u32+0x1d8>
8000c268:	e3510c01 	cmp	r1, #256	; 0x100
8000c26c:	1a00002f 	bne	8000c330 <div_u32+0x13c>
8000c270:	e1a00420 	lsr	r0, r0, #8
8000c274:	e12fff1e 	bx	lr
8000c278:	e1a00001 	mov	r0, r1
8000c27c:	e12fff1e 	bx	lr
8000c280:	e3510302 	cmp	r1, #134217728	; 0x8000000
8000c284:	0a000054 	beq	8000c3dc <div_u32+0x1e8>
8000c288:	8a000022 	bhi	8000c318 <div_u32+0x124>
8000c28c:	e3510402 	cmp	r1, #33554432	; 0x2000000
8000c290:	0a00003d 	beq	8000c38c <div_u32+0x198>
8000c294:	e3510301 	cmp	r1, #67108864	; 0x4000000
8000c298:	0a000051 	beq	8000c3e4 <div_u32+0x1f0>
8000c29c:	e3510401 	cmp	r1, #16777216	; 0x1000000
8000c2a0:	1a000022 	bne	8000c330 <div_u32+0x13c>
8000c2a4:	e1a00c20 	lsr	r0, r0, #24
8000c2a8:	e12fff1e 	bx	lr
8000c2ac:	e3510008 	cmp	r1, #8
8000c2b0:	0a00003d 	beq	8000c3ac <div_u32+0x1b8>
8000c2b4:	9a000036 	bls	8000c394 <div_u32+0x1a0>
8000c2b8:	e3510020 	cmp	r1, #32
8000c2bc:	0a000030 	beq	8000c384 <div_u32+0x190>
8000c2c0:	e3510040 	cmp	r1, #64	; 0x40
8000c2c4:	0a00003c 	beq	8000c3bc <div_u32+0x1c8>
8000c2c8:	e3510010 	cmp	r1, #16
8000c2cc:	1a000017 	bne	8000c330 <div_u32+0x13c>
8000c2d0:	e1a00220 	lsr	r0, r0, #4
8000c2d4:	e12fff1e 	bx	lr
8000c2d8:	e3510a02 	cmp	r1, #8192	; 0x2000
8000c2dc:	0a000020 	beq	8000c364 <div_u32+0x170>
8000c2e0:	e3510901 	cmp	r1, #16384	; 0x4000
8000c2e4:	0a000036 	beq	8000c3c4 <div_u32+0x1d0>
8000c2e8:	e3510a01 	cmp	r1, #4096	; 0x1000
8000c2ec:	1a00000f 	bne	8000c330 <div_u32+0x13c>
8000c2f0:	e1a00620 	lsr	r0, r0, #12
8000c2f4:	e12fff1e 	bx	lr
8000c2f8:	e3510602 	cmp	r1, #2097152	; 0x200000
8000c2fc:	0a00001a 	beq	8000c36c <div_u32+0x178>
8000c300:	e3510501 	cmp	r1, #4194304	; 0x400000
8000c304:	0a00003a 	beq	8000c3f4 <div_u32+0x200>
8000c308:	e3510601 	cmp	r1, #1048576	; 0x100000
8000c30c:	1a000007 	bne	8000c330 <div_u32+0x13c>
8000c310:	e1a00a20 	lsr	r0, r0, #20
8000c314:	e12fff1e 	bx	lr
8000c318:	e3510202 	cmp	r1, #536870912	; 0x20000000
8000c31c:	0a00000e 	beq	8000c35c <div_u32+0x168>
8000c320:	e3510101 	cmp	r1, #1073741824	; 0x40000000
8000c324:	0a000036 	beq	8000c404 <div_u32+0x210>
8000c328:	e3510201 	cmp	r1, #268435456	; 0x10000000
8000c32c:	0a000032 	beq	8000c3fc <div_u32+0x208>
8000c330:	e1510000 	cmp	r1, r0
8000c334:	83a02000 	movhi	r2, #0
8000c338:	8a000005 	bhi	8000c354 <div_u32+0x160>
8000c33c:	e1a03001 	mov	r3, r1
8000c340:	e3a02000 	mov	r2, #0
8000c344:	e0833001 	add	r3, r3, r1
8000c348:	e1500003 	cmp	r0, r3
8000c34c:	e2822001 	add	r2, r2, #1
8000c350:	2afffffb 	bcs	8000c344 <div_u32+0x150>
8000c354:	e1a00002 	mov	r0, r2
8000c358:	e12fff1e 	bx	lr
8000c35c:	e1a00ea0 	lsr	r0, r0, #29
8000c360:	e12fff1e 	bx	lr
8000c364:	e1a006a0 	lsr	r0, r0, #13
8000c368:	e12fff1e 	bx	lr
8000c36c:	e1a00aa0 	lsr	r0, r0, #21
8000c370:	e12fff1e 	bx	lr
8000c374:	e1a008a0 	lsr	r0, r0, #17
8000c378:	e12fff1e 	bx	lr
8000c37c:	e1a004a0 	lsr	r0, r0, #9
8000c380:	e12fff1e 	bx	lr
8000c384:	e1a002a0 	lsr	r0, r0, #5
8000c388:	e12fff1e 	bx	lr
8000c38c:	e1a00ca0 	lsr	r0, r0, #25
8000c390:	e12fff1e 	bx	lr
8000c394:	e3510002 	cmp	r1, #2
8000c398:	0a000005 	beq	8000c3b4 <div_u32+0x1c0>
8000c39c:	e3510004 	cmp	r1, #4
8000c3a0:	1affffe2 	bne	8000c330 <div_u32+0x13c>
8000c3a4:	e1a00120 	lsr	r0, r0, #2
8000c3a8:	e12fff1e 	bx	lr
8000c3ac:	e1a001a0 	lsr	r0, r0, #3
8000c3b0:	e12fff1e 	bx	lr
8000c3b4:	e1a000a0 	lsr	r0, r0, #1
8000c3b8:	e12fff1e 	bx	lr
8000c3bc:	e1a00320 	lsr	r0, r0, #6
8000c3c0:	e12fff1e 	bx	lr
8000c3c4:	e1a00720 	lsr	r0, r0, #14
8000c3c8:	e12fff1e 	bx	lr
8000c3cc:	e1a00520 	lsr	r0, r0, #10
8000c3d0:	e12fff1e 	bx	lr
8000c3d4:	e1a009a0 	lsr	r0, r0, #19
8000c3d8:	e12fff1e 	bx	lr
8000c3dc:	e1a00da0 	lsr	r0, r0, #27
8000c3e0:	e12fff1e 	bx	lr
8000c3e4:	e1a00d20 	lsr	r0, r0, #26
8000c3e8:	e12fff1e 	bx	lr
8000c3ec:	e1a00920 	lsr	r0, r0, #18
8000c3f0:	e12fff1e 	bx	lr
8000c3f4:	e1a00b20 	lsr	r0, r0, #22
8000c3f8:	e12fff1e 	bx	lr
8000c3fc:	e1a00e20 	lsr	r0, r0, #28
8000c400:	e12fff1e 	bx	lr
8000c404:	e1a00f20 	lsr	r0, r0, #30
8000c408:	e12fff1e 	bx	lr
8000c40c:	e1a005a0 	lsr	r0, r0, #11
8000c410:	e12fff1e 	bx	lr
8000c414:	e1a00ba0 	lsr	r0, r0, #23
8000c418:	e12fff1e 	bx	lr
8000c41c:	e1a003a0 	lsr	r0, r0, #7
8000c420:	e12fff1e 	bx	lr
8000c424:	e1a007a0 	lsr	r0, r0, #15
8000c428:	e12fff1e 	bx	lr

8000c42c <mod_u32>:
8000c42c:	e3510000 	cmp	r1, #0
8000c430:	0a000012 	beq	8000c480 <mod_u32+0x54>
8000c434:	e3510902 	cmp	r1, #32768	; 0x8000
8000c438:	0a000086 	beq	8000c658 <mod_u32+0x22c>
8000c43c:	9a000011 	bls	8000c488 <mod_u32+0x5c>
8000c440:	e3510502 	cmp	r1, #8388608	; 0x800000
8000c444:	0a00007e 	beq	8000c644 <mod_u32+0x218>
8000c448:	8a00001e 	bhi	8000c4c8 <mod_u32+0x9c>
8000c44c:	e3510702 	cmp	r1, #524288	; 0x80000
8000c450:	0a000075 	beq	8000c62c <mod_u32+0x200>
8000c454:	8a000042 	bhi	8000c564 <mod_u32+0x138>
8000c458:	e3510802 	cmp	r1, #131072	; 0x20000
8000c45c:	0a000065 	beq	8000c5f8 <mod_u32+0x1cc>
8000c460:	e3510701 	cmp	r1, #262144	; 0x40000
8000c464:	01a01920 	lsreq	r1, r0, #18
8000c468:	01a01901 	lsleq	r1, r1, #18
8000c46c:	0a000003 	beq	8000c480 <mod_u32+0x54>
8000c470:	e3510801 	cmp	r1, #65536	; 0x10000
8000c474:	1a00004e 	bne	8000c5b4 <mod_u32+0x188>
8000c478:	e1a01820 	lsr	r1, r0, #16
8000c47c:	e1a01801 	lsl	r1, r1, #16
8000c480:	e0610000 	rsb	r0, r1, r0
8000c484:	e12fff1e 	bx	lr
8000c488:	e3510080 	cmp	r1, #128	; 0x80
8000c48c:	0a00006f 	beq	8000c650 <mod_u32+0x224>
8000c490:	9a00001a 	bls	8000c500 <mod_u32+0xd4>
8000c494:	e3510b02 	cmp	r1, #2048	; 0x800
8000c498:	0a000066 	beq	8000c638 <mod_u32+0x20c>
8000c49c:	8a000025 	bhi	8000c538 <mod_u32+0x10c>
8000c4a0:	e3510c02 	cmp	r1, #512	; 0x200
8000c4a4:	0a000056 	beq	8000c604 <mod_u32+0x1d8>
8000c4a8:	e3510b01 	cmp	r1, #1024	; 0x400
8000c4ac:	01a01520 	lsreq	r1, r0, #10
8000c4b0:	01a01501 	lsleq	r1, r1, #10
8000c4b4:	0afffff1 	beq	8000c480 <mod_u32+0x54>
8000c4b8:	e3510c01 	cmp	r1, #256	; 0x100
8000c4bc:	1a00003c 	bne	8000c5b4 <mod_u32+0x188>
8000c4c0:	e3c010ff 	bic	r1, r0, #255	; 0xff
8000c4c4:	eaffffed 	b	8000c480 <mod_u32+0x54>
8000c4c8:	e3510302 	cmp	r1, #134217728	; 0x8000000
8000c4cc:	0200133e 	andeq	r1, r0, #-134217728	; 0xf8000000
8000c4d0:	0affffea 	beq	8000c480 <mod_u32+0x54>
8000c4d4:	8a00002d 	bhi	8000c590 <mod_u32+0x164>
8000c4d8:	e3510402 	cmp	r1, #33554432	; 0x2000000
8000c4dc:	020014fe 	andeq	r1, r0, #-33554432	; 0xfe000000
8000c4e0:	0affffe6 	beq	8000c480 <mod_u32+0x54>
8000c4e4:	e3510301 	cmp	r1, #67108864	; 0x4000000
8000c4e8:	0200133f 	andeq	r1, r0, #-67108864	; 0xfc000000
8000c4ec:	0affffe3 	beq	8000c480 <mod_u32+0x54>
8000c4f0:	e3510401 	cmp	r1, #16777216	; 0x1000000
8000c4f4:	1a00002e 	bne	8000c5b4 <mod_u32+0x188>
8000c4f8:	e20014ff 	and	r1, r0, #-16777216	; 0xff000000
8000c4fc:	eaffffdf 	b	8000c480 <mod_u32+0x54>
8000c500:	e3510008 	cmp	r1, #8
8000c504:	03c01007 	biceq	r1, r0, #7
8000c508:	0affffdc 	beq	8000c480 <mod_u32+0x54>
8000c50c:	9a00003f 	bls	8000c610 <mod_u32+0x1e4>
8000c510:	e3510020 	cmp	r1, #32
8000c514:	03c0101f 	biceq	r1, r0, #31
8000c518:	0affffd8 	beq	8000c480 <mod_u32+0x54>
8000c51c:	e3510040 	cmp	r1, #64	; 0x40
8000c520:	03c0103f 	biceq	r1, r0, #63	; 0x3f
8000c524:	0affffd5 	beq	8000c480 <mod_u32+0x54>
8000c528:	e3510010 	cmp	r1, #16
8000c52c:	1a000020 	bne	8000c5b4 <mod_u32+0x188>
8000c530:	e3c0100f 	bic	r1, r0, #15
8000c534:	eaffffd1 	b	8000c480 <mod_u32+0x54>
8000c538:	e3510a02 	cmp	r1, #8192	; 0x2000
8000c53c:	0a000027 	beq	8000c5e0 <mod_u32+0x1b4>
8000c540:	e3510901 	cmp	r1, #16384	; 0x4000
8000c544:	01a01720 	lsreq	r1, r0, #14
8000c548:	01a01701 	lsleq	r1, r1, #14
8000c54c:	0affffcb 	beq	8000c480 <mod_u32+0x54>
8000c550:	e3510a01 	cmp	r1, #4096	; 0x1000
8000c554:	1a000016 	bne	8000c5b4 <mod_u32+0x188>
8000c558:	e1a01620 	lsr	r1, r0, #12
8000c55c:	e1a01601 	lsl	r1, r1, #12
8000c560:	eaffffc6 	b	8000c480 <mod_u32+0x54>
8000c564:	e3510602 	cmp	r1, #2097152	; 0x200000
8000c568:	0a00001f 	beq	8000c5ec <mod_u32+0x1c0>
8000c56c:	e3510501 	cmp	r1, #4194304	; 0x400000
8000c570:	01a01b20 	lsreq	r1, r0, #22
8000c574:	01a01b01 	lsleq	r1, r1, #22
8000c578:	0affffc0 	beq	8000c480 <mod_u32+0x54>
8000c57c:	e3510601 	cmp	r1, #1048576	; 0x100000
8000c580:	1a00000b 	bne	8000c5b4 <mod_u32+0x188>
8000c584:	e1a01a20 	lsr	r1, r0, #20
8000c588:	e1a01a01 	lsl	r1, r1, #20
8000c58c:	eaffffbb 	b	8000c480 <mod_u32+0x54>
8000c590:	e3510202 	cmp	r1, #536870912	; 0x20000000
8000c594:	0200120e 	andeq	r1, r0, #-536870912	; 0xe0000000
8000c598:	0affffb8 	beq	8000c480 <mod_u32+0x54>
8000c59c:	e3510101 	cmp	r1, #1073741824	; 0x40000000
8000c5a0:	02001103 	andeq	r1, r0, #-1073741824	; 0xc0000000
8000c5a4:	0affffb5 	beq	8000c480 <mod_u32+0x54>
8000c5a8:	e3510201 	cmp	r1, #268435456	; 0x10000000
8000c5ac:	0200120f 	andeq	r1, r0, #-268435456	; 0xf0000000
8000c5b0:	0affffb2 	beq	8000c480 <mod_u32+0x54>
8000c5b4:	e1500001 	cmp	r0, r1
8000c5b8:	33a01000 	movcc	r1, #0
8000c5bc:	3affffaf 	bcc	8000c480 <mod_u32+0x54>
8000c5c0:	e1a03001 	mov	r3, r1
8000c5c4:	e3a02000 	mov	r2, #0
8000c5c8:	e0833001 	add	r3, r3, r1
8000c5cc:	e1500003 	cmp	r0, r3
8000c5d0:	e2822001 	add	r2, r2, #1
8000c5d4:	2afffffb 	bcs	8000c5c8 <mod_u32+0x19c>
8000c5d8:	e0010192 	mul	r1, r2, r1
8000c5dc:	eaffffa7 	b	8000c480 <mod_u32+0x54>
8000c5e0:	e1a016a0 	lsr	r1, r0, #13
8000c5e4:	e1a01681 	lsl	r1, r1, #13
8000c5e8:	eaffffa4 	b	8000c480 <mod_u32+0x54>
8000c5ec:	e1a01aa0 	lsr	r1, r0, #21
8000c5f0:	e1a01a81 	lsl	r1, r1, #21
8000c5f4:	eaffffa1 	b	8000c480 <mod_u32+0x54>
8000c5f8:	e1a018a0 	lsr	r1, r0, #17
8000c5fc:	e1a01881 	lsl	r1, r1, #17
8000c600:	eaffff9e 	b	8000c480 <mod_u32+0x54>
8000c604:	e1a014a0 	lsr	r1, r0, #9
8000c608:	e1a01481 	lsl	r1, r1, #9
8000c60c:	eaffff9b 	b	8000c480 <mod_u32+0x54>
8000c610:	e3510002 	cmp	r1, #2
8000c614:	03c01001 	biceq	r1, r0, #1
8000c618:	0affff98 	beq	8000c480 <mod_u32+0x54>
8000c61c:	e3510004 	cmp	r1, #4
8000c620:	03c01003 	biceq	r1, r0, #3
8000c624:	0affff95 	beq	8000c480 <mod_u32+0x54>
8000c628:	eaffffe1 	b	8000c5b4 <mod_u32+0x188>
8000c62c:	e1a019a0 	lsr	r1, r0, #19
8000c630:	e1a01981 	lsl	r1, r1, #19
8000c634:	eaffff91 	b	8000c480 <mod_u32+0x54>
8000c638:	e1a015a0 	lsr	r1, r0, #11
8000c63c:	e1a01581 	lsl	r1, r1, #11
8000c640:	eaffff8e 	b	8000c480 <mod_u32+0x54>
8000c644:	e1a01ba0 	lsr	r1, r0, #23
8000c648:	e1a01b81 	lsl	r1, r1, #23
8000c64c:	eaffff8b 	b	8000c480 <mod_u32+0x54>
8000c650:	e3c0107f 	bic	r1, r0, #127	; 0x7f
8000c654:	eaffff89 	b	8000c480 <mod_u32+0x54>
8000c658:	e1a017a0 	lsr	r1, r0, #15
8000c65c:	e1a01781 	lsl	r1, r1, #15
8000c660:	eaffff86 	b	8000c480 <mod_u32+0x54>

8000c664 <sd_read_sector>:
8000c664:	e1a0c00d 	mov	ip, sp
8000c668:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000c66c:	e24cb004 	sub	fp, ip, #4
8000c670:	e1a06000 	mov	r6, r0
8000c674:	e3a00001 	mov	r0, #1
8000c678:	e1a05001 	mov	r5, r1
8000c67c:	eb0014be 	bl	8001197c <get_dev>
8000c680:	e2504000 	subs	r4, r0, #0
8000c684:	0a00000b 	beq	8000c6b8 <sd_read_sector+0x54>
8000c688:	e1a01006 	mov	r1, r6
8000c68c:	eb0014e2 	bl	80011a1c <dev_block_read>
8000c690:	e3500000 	cmp	r0, #0
8000c694:	1a000007 	bne	8000c6b8 <sd_read_sector+0x54>
8000c698:	e1a00004 	mov	r0, r4
8000c69c:	ebfff050 	bl	800087e4 <sd_dev_handle>
8000c6a0:	e1a00004 	mov	r0, r4
8000c6a4:	e1a01005 	mov	r1, r5
8000c6a8:	eb0014e7 	bl	80011a4c <dev_block_read_done>
8000c6ac:	e3500000 	cmp	r0, #0
8000c6b0:	1afffff8 	bne	8000c698 <sd_read_sector+0x34>
8000c6b4:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000c6b8:	e3e00000 	mvn	r0, #0
8000c6bc:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}

8000c6c0 <sd_read>:
8000c6c0:	e1a0c00d 	mov	ip, sp
8000c6c4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000c6c8:	e24cb004 	sub	fp, ip, #4
8000c6cc:	e1a04000 	mov	r4, r0
8000c6d0:	e3a00001 	mov	r0, #1
8000c6d4:	e1a05001 	mov	r5, r1
8000c6d8:	eb0014a7 	bl	8001197c <get_dev>
8000c6dc:	e3500000 	cmp	r0, #0
8000c6e0:	0a00000e 	beq	8000c720 <sd_read+0x60>
8000c6e4:	e59f303c 	ldr	r3, [pc, #60]	; 8000c728 <sd_read+0x68>
8000c6e8:	e1a01005 	mov	r1, r5
8000c6ec:	e08f3003 	add	r3, pc, r3
8000c6f0:	e5933008 	ldr	r3, [r3, #8]
8000c6f4:	e0834084 	add	r4, r3, r4, lsl #1
8000c6f8:	e1a00004 	mov	r0, r4
8000c6fc:	ebffffd8 	bl	8000c664 <sd_read_sector>
8000c700:	e3500000 	cmp	r0, #0
8000c704:	1a000005 	bne	8000c720 <sd_read+0x60>
8000c708:	e2840001 	add	r0, r4, #1
8000c70c:	e2851c02 	add	r1, r5, #512	; 0x200
8000c710:	ebffffd3 	bl	8000c664 <sd_read_sector>
8000c714:	e3500000 	cmp	r0, #0
8000c718:	1a000000 	bne	8000c720 <sd_read+0x60>
8000c71c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000c720:	e3e00000 	mvn	r0, #0
8000c724:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000c728:	00013dcc 	andeq	r3, r1, ip, asr #27

8000c72c <get_node_by_ino>:
8000c72c:	e1a0c00d 	mov	ip, sp
8000c730:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000c734:	e1a06000 	mov	r6, r0
8000c738:	e24cb004 	sub	fp, ip, #4
8000c73c:	e1a04001 	mov	r4, r1
8000c740:	e1a00001 	mov	r0, r1
8000c744:	e596102c 	ldr	r1, [r6, #44]	; 0x2c
8000c748:	e1a05002 	mov	r5, r2
8000c74c:	ebfffea8 	bl	8000c1f4 <div_u32>
8000c750:	e596102c 	ldr	r1, [r6, #44]	; 0x2c
8000c754:	e5963404 	ldr	r3, [r6, #1028]	; 0x404
8000c758:	e0010190 	mul	r1, r0, r1
8000c75c:	e0830280 	add	r0, r3, r0, lsl #5
8000c760:	e5902008 	ldr	r2, [r0, #8]
8000c764:	e0611004 	rsb	r1, r1, r4
8000c768:	e2514001 	subs	r4, r1, #1
8000c76c:	42810006 	addmi	r0, r1, #6
8000c770:	51a00004 	movpl	r0, r4
8000c774:	e5963408 	ldr	r3, [r6, #1032]	; 0x408
8000c778:	e08201c0 	add	r0, r2, r0, asr #3
8000c77c:	e1a01005 	mov	r1, r5
8000c780:	e12fff33 	blx	r3
8000c784:	e1a00fc4 	asr	r0, r4, #31
8000c788:	e1a00ea0 	lsr	r0, r0, #29
8000c78c:	e0844000 	add	r4, r4, r0
8000c790:	e2044007 	and	r4, r4, #7
8000c794:	e0600004 	rsb	r0, r0, r4
8000c798:	e0850380 	add	r0, r5, r0, lsl #7
8000c79c:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}

8000c7a0 <read_partition>:
8000c7a0:	e1a0c00d 	mov	ip, sp
8000c7a4:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000c7a8:	e24cb004 	sub	fp, ip, #4
8000c7ac:	e24ddc02 	sub	sp, sp, #512	; 0x200
8000c7b0:	e24b6f87 	sub	r6, fp, #540	; 0x21c
8000c7b4:	e1a01006 	mov	r1, r6
8000c7b8:	e3a00000 	mov	r0, #0
8000c7bc:	ebffffa8 	bl	8000c664 <sd_read_sector>
8000c7c0:	e2507000 	subs	r7, r0, #0
8000c7c4:	1a000015 	bne	8000c820 <read_partition+0x80>
8000c7c8:	e55b301e 	ldrb	r3, [fp, #-30]
8000c7cc:	e3530055 	cmp	r3, #85	; 0x55
8000c7d0:	1a000012 	bne	8000c820 <read_partition+0x80>
8000c7d4:	e55b301d 	ldrb	r3, [fp, #-29]
8000c7d8:	e35300aa 	cmp	r3, #170	; 0xaa
8000c7dc:	1a00000f 	bne	8000c820 <read_partition+0x80>
8000c7e0:	e59f5048 	ldr	r5, [pc, #72]	; 8000c830 <read_partition+0x90>
8000c7e4:	e1a04007 	mov	r4, r7
8000c7e8:	e08f5005 	add	r5, pc, r5
8000c7ec:	e2855010 	add	r5, r5, #16
8000c7f0:	e2841f6f 	add	r1, r4, #444	; 0x1bc
8000c7f4:	e3a02010 	mov	r2, #16
8000c7f8:	e2811002 	add	r1, r1, #2
8000c7fc:	e0850004 	add	r0, r5, r4
8000c800:	e0861001 	add	r1, r6, r1
8000c804:	e0844002 	add	r4, r4, r2
8000c808:	ebfff8fc 	bl	8000ac00 <memcpy>
8000c80c:	e3540040 	cmp	r4, #64	; 0x40
8000c810:	1afffff6 	bne	8000c7f0 <read_partition+0x50>
8000c814:	e1a00007 	mov	r0, r7
8000c818:	e24bd01c 	sub	sp, fp, #28
8000c81c:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000c820:	e3e07000 	mvn	r7, #0
8000c824:	e1a00007 	mov	r0, r7
8000c828:	e24bd01c 	sub	sp, fp, #28
8000c82c:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000c830:	00013cd0 	ldrdeq	r3, [r1], -r0

8000c834 <partition_get>:
8000c834:	e1a0c00d 	mov	ip, sp
8000c838:	e3500003 	cmp	r0, #3
8000c83c:	e92dd800 	push	{fp, ip, lr, pc}
8000c840:	e1a03000 	mov	r3, r0
8000c844:	e24cb004 	sub	fp, ip, #4
8000c848:	e1a00001 	mov	r0, r1
8000c84c:	8a000007 	bhi	8000c870 <partition_get+0x3c>
8000c850:	e59f1020 	ldr	r1, [pc, #32]	; 8000c878 <partition_get+0x44>
8000c854:	e3a02010 	mov	r2, #16
8000c858:	e08f1001 	add	r1, pc, r1
8000c85c:	e2811010 	add	r1, r1, #16
8000c860:	e0811203 	add	r1, r1, r3, lsl #4
8000c864:	ebfff8e5 	bl	8000ac00 <memcpy>
8000c868:	e3a00000 	mov	r0, #0
8000c86c:	e89da800 	ldm	sp, {fp, sp, pc}
8000c870:	e3e00000 	mvn	r0, #0
8000c874:	e89da800 	ldm	sp, {fp, sp, pc}
8000c878:	00013c60 	andeq	r3, r1, r0, ror #24

8000c87c <sd_read_ext2>:
8000c87c:	e1a0c00d 	mov	ip, sp
8000c880:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000c884:	e24ddecb 	sub	sp, sp, #3248	; 0xcb0
8000c888:	e24cb004 	sub	fp, ip, #4
8000c88c:	e24dd004 	sub	sp, sp, #4
8000c890:	e1a05000 	mov	r5, r0
8000c894:	e50b1cd4 	str	r1, [fp, #-3284]	; 0xcd4
8000c898:	ebffffc0 	bl	8000c7a0 <read_partition>
8000c89c:	e3500000 	cmp	r0, #0
8000c8a0:	1a000005 	bne	8000c8bc <sd_read_ext2+0x40>
8000c8a4:	e59f1564 	ldr	r1, [pc, #1380]	; 8000ce10 <sd_read_ext2+0x594>
8000c8a8:	e3a00001 	mov	r0, #1
8000c8ac:	e08f1001 	add	r1, pc, r1
8000c8b0:	ebffffdf 	bl	8000c834 <partition_get>
8000c8b4:	e3500000 	cmp	r0, #0
8000c8b8:	0a000004 	beq	8000c8d0 <sd_read_ext2+0x54>
8000c8bc:	e59f0550 	ldr	r0, [pc, #1360]	; 8000ce14 <sd_read_ext2+0x598>
8000c8c0:	e3a01000 	mov	r1, #0
8000c8c4:	e08f0000 	add	r0, pc, r0
8000c8c8:	e3a02010 	mov	r2, #16
8000c8cc:	ebfff8e7 	bl	8000ac70 <memset>
8000c8d0:	e59f3540 	ldr	r3, [pc, #1344]	; 8000ce18 <sd_read_ext2+0x59c>
8000c8d4:	e24b2ec3 	sub	r2, fp, #3120	; 0xc30
8000c8d8:	e242200c 	sub	r2, r2, #12
8000c8dc:	e24b6e43 	sub	r6, fp, #1072	; 0x430
8000c8e0:	e08f3003 	add	r3, pc, r3
8000c8e4:	e1a04002 	mov	r4, r2
8000c8e8:	e1a01002 	mov	r1, r2
8000c8ec:	e50b2cc8 	str	r2, [fp, #-3272]	; 0xcc8
8000c8f0:	e246600c 	sub	r6, r6, #12
8000c8f4:	e3a02000 	mov	r2, #0
8000c8f8:	e3a00001 	mov	r0, #1
8000c8fc:	e50b3034 	str	r3, [fp, #-52]	; 0x34
8000c900:	e50b2030 	str	r2, [fp, #-48]	; 0x30
8000c904:	ebffff6d 	bl	8000c6c0 <sd_read>
8000c908:	e3a02b01 	mov	r2, #1024	; 0x400
8000c90c:	e1a01004 	mov	r1, r4
8000c910:	e2860004 	add	r0, r6, #4
8000c914:	ebfff8b9 	bl	8000ac00 <memcpy>
8000c918:	e51b1418 	ldr	r1, [fp, #-1048]	; 0x418
8000c91c:	e51b0434 	ldr	r0, [fp, #-1076]	; 0x434
8000c920:	ebfffe33 	bl	8000c1f4 <div_u32>
8000c924:	e51b1418 	ldr	r1, [fp, #-1048]	; 0x418
8000c928:	e3a09002 	mov	r9, #2
8000c92c:	e1a04000 	mov	r4, r0
8000c930:	e51b0434 	ldr	r0, [fp, #-1076]	; 0x434
8000c934:	ebfffebc 	bl	8000c42c <mod_u32>
8000c938:	e3500000 	cmp	r0, #0
8000c93c:	12844001 	addne	r4, r4, #1
8000c940:	e1a00284 	lsl	r0, r4, #5
8000c944:	e50b443c 	str	r4, [fp, #-1084]	; 0x43c
8000c948:	eb000406 	bl	8000d968 <kmalloc>
8000c94c:	e24b3e83 	sub	r3, fp, #2096	; 0x830
8000c950:	e243300c 	sub	r3, r3, #12
8000c954:	e3a01020 	mov	r1, #32
8000c958:	e1a0a003 	mov	sl, r3
8000c95c:	e50b3cc0 	str	r3, [fp, #-3264]	; 0xcc0
8000c960:	e3a04000 	mov	r4, #0
8000c964:	e50b0038 	str	r0, [fp, #-56]	; 0x38
8000c968:	e3a00b01 	mov	r0, #1024	; 0x400
8000c96c:	ebfffe20 	bl	8000c1f4 <div_u32>
8000c970:	e50b5cc4 	str	r5, [fp, #-3268]	; 0xcc4
8000c974:	e50b6ccc 	str	r6, [fp, #-3276]	; 0xccc
8000c978:	e1a08000 	mov	r8, r0
8000c97c:	e51b0418 	ldr	r0, [fp, #-1048]	; 0x418
8000c980:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
8000c984:	e0890000 	add	r0, r9, r0
8000c988:	e1a0100a 	mov	r1, sl
8000c98c:	e12fff33 	blx	r3
8000c990:	e3580000 	cmp	r8, #0
8000c994:	da00006f 	ble	8000cb58 <sd_read_ext2+0x2dc>
8000c998:	e1a06284 	lsl	r6, r4, #5
8000c99c:	e1a0500a 	mov	r5, sl
8000c9a0:	e0887004 	add	r7, r8, r4
8000c9a4:	ea000003 	b	8000c9b8 <sd_read_ext2+0x13c>
8000c9a8:	e1540007 	cmp	r4, r7
8000c9ac:	e2866020 	add	r6, r6, #32
8000c9b0:	e2855020 	add	r5, r5, #32
8000c9b4:	0a000068 	beq	8000cb5c <sd_read_ext2+0x2e0>
8000c9b8:	e51b0038 	ldr	r0, [fp, #-56]	; 0x38
8000c9bc:	e1a01005 	mov	r1, r5
8000c9c0:	e0800006 	add	r0, r0, r6
8000c9c4:	e3a02020 	mov	r2, #32
8000c9c8:	ebfff88c 	bl	8000ac00 <memcpy>
8000c9cc:	e51b343c 	ldr	r3, [fp, #-1084]	; 0x43c
8000c9d0:	e2844001 	add	r4, r4, #1
8000c9d4:	e1540003 	cmp	r4, r3
8000c9d8:	bafffff2 	blt	8000c9a8 <sd_read_ext2+0x12c>
8000c9dc:	e51b3cd4 	ldr	r3, [fp, #-3284]	; 0xcd4
8000c9e0:	e51b5cc4 	ldr	r5, [fp, #-3268]	; 0xcc4
8000c9e4:	e3530000 	cmp	r3, #0
8000c9e8:	e51b6ccc 	ldr	r6, [fp, #-3276]	; 0xccc
8000c9ec:	151b2cd4 	ldrne	r2, [fp, #-3284]	; 0xcd4
8000c9f0:	13e03000 	mvnne	r3, #0
8000c9f4:	15823000 	strne	r3, [r2]
8000c9f8:	e59f141c 	ldr	r1, [pc, #1052]	; 8000ce1c <sd_read_ext2+0x5a0>
8000c9fc:	e1a00005 	mov	r0, r5
8000ca00:	e08f1001 	add	r1, pc, r1
8000ca04:	ebfff8fa 	bl	8000adf4 <strcmp>
8000ca08:	e3500000 	cmp	r0, #0
8000ca0c:	03a01002 	moveq	r1, #2
8000ca10:	1a000070 	bne	8000cbd8 <sd_read_ext2+0x35c>
8000ca14:	e1a00006 	mov	r0, r6
8000ca18:	e51b2cc0 	ldr	r2, [fp, #-3264]	; 0xcc0
8000ca1c:	ebffff42 	bl	8000c72c <get_node_by_ino>
8000ca20:	e2501000 	subs	r1, r0, #0
8000ca24:	0a0000f7 	beq	8000ce08 <sd_read_ext2+0x58c>
8000ca28:	e24b0ecb 	sub	r0, fp, #3248	; 0xcb0
8000ca2c:	e3a02080 	mov	r2, #128	; 0x80
8000ca30:	e240000c 	sub	r0, r0, #12
8000ca34:	ebfff871 	bl	8000ac00 <memcpy>
8000ca38:	e51b0cb8 	ldr	r0, [fp, #-3256]	; 0xcb8
8000ca3c:	eb0003c9 	bl	8000d968 <kmalloc>
8000ca40:	e2503000 	subs	r3, r0, #0
8000ca44:	e50b3ccc 	str	r3, [fp, #-3276]	; 0xccc
8000ca48:	0a00003d 	beq	8000cb44 <sd_read_ext2+0x2c8>
8000ca4c:	e51b5cb8 	ldr	r5, [fp, #-3256]	; 0xcb8
8000ca50:	e1a08003 	mov	r8, r3
8000ca54:	e3a03000 	mov	r3, #0
8000ca58:	e50b3cc4 	str	r3, [fp, #-3268]	; 0xcc4
8000ca5c:	e51b2cc4 	ldr	r2, [fp, #-3268]	; 0xcc4
8000ca60:	e1a07fc2 	asr	r7, r2, #31
8000ca64:	e1a03b27 	lsr	r3, r7, #22
8000ca68:	e0827003 	add	r7, r2, r3
8000ca6c:	e1a07b07 	lsl	r7, r7, #22
8000ca70:	e0637b27 	rsb	r7, r3, r7, lsr #22
8000ca74:	e59f33a4 	ldr	r3, [pc, #932]	; 8000ce20 <sd_read_ext2+0x5a4>
8000ca78:	e2521000 	subs	r1, r2, #0
8000ca7c:	e2824fff 	add	r4, r2, #1020	; 0x3fc
8000ca80:	b2841003 	addlt	r1, r4, #3
8000ca84:	e2679b01 	rsb	r9, r7, #1024	; 0x400
8000ca88:	e1590003 	cmp	r9, r3
8000ca8c:	e1a04541 	asr	r4, r1, #10
8000ca90:	d1a06009 	movle	r6, r9
8000ca94:	c3a06b01 	movgt	r6, #1024	; 0x400
8000ca98:	e354000b 	cmp	r4, #11
8000ca9c:	e0625005 	rsb	r5, r2, r5
8000caa0:	ca000030 	bgt	8000cb68 <sd_read_ext2+0x2ec>
8000caa4:	e24b302c 	sub	r3, fp, #44	; 0x2c
8000caa8:	e0834104 	add	r4, r3, r4, lsl #2
8000caac:	e5140c68 	ldr	r0, [r4, #-3176]	; 0xc68
8000cab0:	e51b4cc0 	ldr	r4, [fp, #-3264]	; 0xcc0
8000cab4:	e51b2034 	ldr	r2, [fp, #-52]	; 0x34
8000cab8:	e1a01004 	mov	r1, r4
8000cabc:	e12fff32 	blx	r2
8000cac0:	e0847007 	add	r7, r4, r7
8000cac4:	e3a0a000 	mov	sl, #0
8000cac8:	e1560005 	cmp	r6, r5
8000cacc:	b1a04006 	movlt	r4, r6
8000cad0:	a1a04005 	movge	r4, r5
8000cad4:	e1a02004 	mov	r2, r4
8000cad8:	e1a00008 	mov	r0, r8
8000cadc:	e1a01007 	mov	r1, r7
8000cae0:	ebfff846 	bl	8000ac00 <memcpy>
8000cae4:	e0555004 	subs	r5, r5, r4
8000cae8:	03a03001 	moveq	r3, #1
8000caec:	13a03000 	movne	r3, #0
8000caf0:	e0566004 	subs	r6, r6, r4
8000caf4:	e08aa004 	add	sl, sl, r4
8000caf8:	e0649009 	rsb	r9, r4, r9
8000cafc:	03833001 	orreq	r3, r3, #1
8000cb00:	e3530000 	cmp	r3, #0
8000cb04:	1a000001 	bne	8000cb10 <sd_read_ext2+0x294>
8000cb08:	e3590000 	cmp	r9, #0
8000cb0c:	1affffed 	bne	8000cac8 <sd_read_ext2+0x24c>
8000cb10:	e35a0000 	cmp	sl, #0
8000cb14:	da000006 	ble	8000cb34 <sd_read_ext2+0x2b8>
8000cb18:	e51b5cb8 	ldr	r5, [fp, #-3256]	; 0xcb8
8000cb1c:	e51b3cc4 	ldr	r3, [fp, #-3268]	; 0xcc4
8000cb20:	e1530005 	cmp	r3, r5
8000cb24:	3083300a 	addcc	r3, r3, sl
8000cb28:	3088800a 	addcc	r8, r8, sl
8000cb2c:	350b3cc4 	strcc	r3, [fp, #-3268]	; 0xcc4
8000cb30:	3affffc9 	bcc	8000ca5c <sd_read_ext2+0x1e0>
8000cb34:	e51b3cd4 	ldr	r3, [fp, #-3284]	; 0xcd4
8000cb38:	e3530000 	cmp	r3, #0
8000cb3c:	151b2cc4 	ldrne	r2, [fp, #-3268]	; 0xcc4
8000cb40:	15832000 	strne	r2, [r3]
8000cb44:	e51b0038 	ldr	r0, [fp, #-56]	; 0x38
8000cb48:	eb000399 	bl	8000d9b4 <kfree>
8000cb4c:	e51b0ccc 	ldr	r0, [fp, #-3276]	; 0xccc
8000cb50:	e24bd028 	sub	sp, fp, #40	; 0x28
8000cb54:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000cb58:	e1a07004 	mov	r7, r4
8000cb5c:	e2899001 	add	r9, r9, #1
8000cb60:	e1a04007 	mov	r4, r7
8000cb64:	eaffff84 	b	8000c97c <sd_read_ext2+0x100>
8000cb68:	e244a00c 	sub	sl, r4, #12
8000cb6c:	e35a00ff 	cmp	sl, #255	; 0xff
8000cb70:	8a000007 	bhi	8000cb94 <sd_read_ext2+0x318>
8000cb74:	e51bc034 	ldr	ip, [fp, #-52]	; 0x34
8000cb78:	e51b0c64 	ldr	r0, [fp, #-3172]	; 0xc64
8000cb7c:	e51b1cc0 	ldr	r1, [fp, #-3264]	; 0xcc0
8000cb80:	e12fff3c 	blx	ip
8000cb84:	e24b302c 	sub	r3, fp, #44	; 0x2c
8000cb88:	e083210a 	add	r2, r3, sl, lsl #2
8000cb8c:	e5120810 	ldr	r0, [r2, #-2064]	; 0x810
8000cb90:	eaffffc6 	b	8000cab0 <sd_read_ext2+0x234>
8000cb94:	e2444f43 	sub	r4, r4, #268	; 0x10c
8000cb98:	e51b2034 	ldr	r2, [fp, #-52]	; 0x34
8000cb9c:	e51b1cc8 	ldr	r1, [fp, #-3272]	; 0xcc8
8000cba0:	e51b0c60 	ldr	r0, [fp, #-3168]	; 0xc60
8000cba4:	e12fff32 	blx	r2
8000cba8:	e24b302c 	sub	r3, fp, #44	; 0x2c
8000cbac:	e1a02444 	asr	r2, r4, #8
8000cbb0:	e0832102 	add	r2, r3, r2, lsl #2
8000cbb4:	e5120c10 	ldr	r0, [r2, #-3088]	; 0xc10
8000cbb8:	e51b1cc0 	ldr	r1, [fp, #-3264]	; 0xcc0
8000cbbc:	e51b2034 	ldr	r2, [fp, #-52]	; 0x34
8000cbc0:	e12fff32 	blx	r2
8000cbc4:	e20420ff 	and	r2, r4, #255	; 0xff
8000cbc8:	e24b302c 	sub	r3, fp, #44	; 0x2c
8000cbcc:	e0832102 	add	r2, r3, r2, lsl #2
8000cbd0:	e5120810 	ldr	r0, [r2, #-2064]	; 0x810
8000cbd4:	eaffffb5 	b	8000cab0 <sd_read_ext2+0x234>
8000cbd8:	e5d52000 	ldrb	r2, [r5]
8000cbdc:	e24b3ecb 	sub	r3, fp, #3248	; 0xcb0
8000cbe0:	e243300c 	sub	r3, r3, #12
8000cbe4:	e3a07000 	mov	r7, #0
8000cbe8:	e352002f 	cmp	r2, #47	; 0x2f
8000cbec:	e1a04007 	mov	r4, r7
8000cbf0:	e50b3cd8 	str	r3, [fp, #-3288]	; 0xcd8
8000cbf4:	e51b8cc0 	ldr	r8, [fp, #-3264]	; 0xcc0
8000cbf8:	e1a09003 	mov	r9, r3
8000cbfc:	0a00001c 	beq	8000cc74 <sd_read_ext2+0x3f8>
8000cc00:	e2480001 	sub	r0, r8, #1
8000cc04:	e3a03000 	mov	r3, #0
8000cc08:	ea000007 	b	8000cc2c <sd_read_ext2+0x3b0>
8000cc0c:	e5f51001 	ldrb	r1, [r5, #1]!
8000cc10:	e5e02001 	strb	r2, [r0, #1]!
8000cc14:	e3510000 	cmp	r1, #0
8000cc18:	e2833001 	add	r3, r3, #1
8000cc1c:	0a000004 	beq	8000cc34 <sd_read_ext2+0x3b8>
8000cc20:	e353003f 	cmp	r3, #63	; 0x3f
8000cc24:	0a000002 	beq	8000cc34 <sd_read_ext2+0x3b8>
8000cc28:	e1a02001 	mov	r2, r1
8000cc2c:	e352002f 	cmp	r2, #47	; 0x2f
8000cc30:	1afffff5 	bne	8000cc0c <sd_read_ext2+0x390>
8000cc34:	e24b202c 	sub	r2, fp, #44	; 0x2c
8000cc38:	e0823003 	add	r3, r2, r3
8000cc3c:	e1a00008 	mov	r0, r8
8000cc40:	e5434810 	strb	r4, [r3, #-2064]	; 0x810
8000cc44:	ebfffbee 	bl	8000bc04 <str_new>
8000cc48:	e7890107 	str	r0, [r9, r7, lsl #2]
8000cc4c:	e2877001 	add	r7, r7, #1
8000cc50:	e3570020 	cmp	r7, #32
8000cc54:	0a000009 	beq	8000cc80 <sd_read_ext2+0x404>
8000cc58:	e5d53000 	ldrb	r3, [r5]
8000cc5c:	e3530000 	cmp	r3, #0
8000cc60:	0a000006 	beq	8000cc80 <sd_read_ext2+0x404>
8000cc64:	e2855001 	add	r5, r5, #1
8000cc68:	e5d52000 	ldrb	r2, [r5]
8000cc6c:	e352002f 	cmp	r2, #47	; 0x2f
8000cc70:	1affffe2 	bne	8000cc00 <sd_read_ext2+0x384>
8000cc74:	e5d52001 	ldrb	r2, [r5, #1]
8000cc78:	e2855001 	add	r5, r5, #1
8000cc7c:	eaffffdf 	b	8000cc00 <sd_read_ext2+0x384>
8000cc80:	e51b343c 	ldr	r3, [fp, #-1084]	; 0x43c
8000cc84:	e3530000 	cmp	r3, #0
8000cc88:	e2473107 	sub	r3, r7, #-1073741823	; 0xc0000001
8000cc8c:	e50b3cdc 	str	r3, [fp, #-3292]	; 0xcdc
8000cc90:	da00005a 	ble	8000ce00 <sd_read_ext2+0x584>
8000cc94:	e51b1cd8 	ldr	r1, [fp, #-3288]	; 0xcd8
8000cc98:	e3a02000 	mov	r2, #0
8000cc9c:	e0813103 	add	r3, r1, r3, lsl #2
8000cca0:	e1a07002 	mov	r7, r2
8000cca4:	e50b2cd0 	str	r2, [fp, #-3280]	; 0xcd0
8000cca8:	e50b3ccc 	str	r3, [fp, #-3276]	; 0xccc
8000ccac:	e51b1cd0 	ldr	r1, [fp, #-3280]	; 0xcd0
8000ccb0:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
8000ccb4:	e51b2034 	ldr	r2, [fp, #-52]	; 0x34
8000ccb8:	e0833281 	add	r3, r3, r1, lsl #5
8000ccbc:	e5930008 	ldr	r0, [r3, #8]
8000ccc0:	e51b1cc8 	ldr	r1, [fp, #-3272]	; 0xcc8
8000ccc4:	e12fff32 	blx	r2
8000ccc8:	e3500000 	cmp	r0, #0
8000cccc:	1a000045 	bne	8000cde8 <sd_read_ext2+0x56c>
8000ccd0:	e51b3cd8 	ldr	r3, [fp, #-3288]	; 0xcd8
8000ccd4:	e24b0ebb 	sub	r0, fp, #2992	; 0xbb0
8000ccd8:	e2433004 	sub	r3, r3, #4
8000ccdc:	e240000c 	sub	r0, r0, #12
8000cce0:	e50b3cc4 	str	r3, [fp, #-3268]	; 0xcc4
8000cce4:	e51b2cc4 	ldr	r2, [fp, #-3268]	; 0xcc4
8000cce8:	e2808024 	add	r8, r0, #36	; 0x24
8000ccec:	e5b23004 	ldr	r3, [r2, #4]!
8000ccf0:	e2809054 	add	r9, r0, #84	; 0x54
8000ccf4:	e50b2cc4 	str	r2, [fp, #-3268]	; 0xcc4
8000ccf8:	e5935000 	ldr	r5, [r3]
8000ccfc:	ea000001 	b	8000cd08 <sd_read_ext2+0x48c>
8000cd00:	e1580009 	cmp	r8, r9
8000cd04:	0a000037 	beq	8000cde8 <sd_read_ext2+0x56c>
8000cd08:	e5b80004 	ldr	r0, [r8, #4]!
8000cd0c:	e3500000 	cmp	r0, #0
8000cd10:	0afffffa 	beq	8000cd00 <sd_read_ext2+0x484>
8000cd14:	e51b3034 	ldr	r3, [fp, #-52]	; 0x34
8000cd18:	e51b1cc0 	ldr	r1, [fp, #-3264]	; 0xcc0
8000cd1c:	e12fff33 	blx	r3
8000cd20:	e55b2836 	ldrb	r2, [fp, #-2102]	; 0x836
8000cd24:	e3520000 	cmp	r2, #0
8000cd28:	151bacc0 	ldrne	sl, [fp, #-3264]	; 0xcc0
8000cd2c:	1a00000a 	bne	8000cd5c <sd_read_ext2+0x4e0>
8000cd30:	eafffff2 	b	8000cd00 <sd_read_ext2+0x484>
8000cd34:	e5da3006 	ldrb	r3, [sl, #6]
8000cd38:	e1da20b4 	ldrh	r2, [sl, #4]
8000cd3c:	e08a3003 	add	r3, sl, r3
8000cd40:	e08aa002 	add	sl, sl, r2
8000cd44:	e15a0006 	cmp	sl, r6
8000cd48:	e5c34008 	strb	r4, [r3, #8]
8000cd4c:	2affffeb 	bcs	8000cd00 <sd_read_ext2+0x484>
8000cd50:	e5da2006 	ldrb	r2, [sl, #6]
8000cd54:	e3520000 	cmp	r2, #0
8000cd58:	0affffe8 	beq	8000cd00 <sd_read_ext2+0x484>
8000cd5c:	e08a2002 	add	r2, sl, r2
8000cd60:	e5d24008 	ldrb	r4, [r2, #8]
8000cd64:	e28a0008 	add	r0, sl, #8
8000cd68:	e1a01005 	mov	r1, r5
8000cd6c:	e5c27008 	strb	r7, [r2, #8]
8000cd70:	ebfff81f 	bl	8000adf4 <strcmp>
8000cd74:	e3500000 	cmp	r0, #0
8000cd78:	1affffed 	bne	8000cd34 <sd_read_ext2+0x4b8>
8000cd7c:	e59a4000 	ldr	r4, [sl]
8000cd80:	e3540000 	cmp	r4, #0
8000cd84:	ba000017 	blt	8000cde8 <sd_read_ext2+0x56c>
8000cd88:	e1a00006 	mov	r0, r6
8000cd8c:	e1a01004 	mov	r1, r4
8000cd90:	e51b2cc8 	ldr	r2, [fp, #-3272]	; 0xcc8
8000cd94:	ebfffe64 	bl	8000c72c <get_node_by_ino>
8000cd98:	e3500000 	cmp	r0, #0
8000cd9c:	0a000011 	beq	8000cde8 <sd_read_ext2+0x56c>
8000cda0:	e51b3cc4 	ldr	r3, [fp, #-3268]	; 0xcc4
8000cda4:	e51b2ccc 	ldr	r2, [fp, #-3276]	; 0xccc
8000cda8:	e1530002 	cmp	r3, r2
8000cdac:	1affffcc 	bne	8000cce4 <sd_read_ext2+0x468>
8000cdb0:	e51b3cd8 	ldr	r3, [fp, #-3288]	; 0xcd8
8000cdb4:	e51b2cdc 	ldr	r2, [fp, #-3292]	; 0xcdc
8000cdb8:	e2437004 	sub	r7, r3, #4
8000cdbc:	e0835102 	add	r5, r3, r2, lsl #2
8000cdc0:	e5b70004 	ldr	r0, [r7, #4]!
8000cdc4:	ebfffbf7 	bl	8000bda8 <str_free>
8000cdc8:	e1570005 	cmp	r7, r5
8000cdcc:	1afffffb 	bne	8000cdc0 <sd_read_ext2+0x544>
8000cdd0:	e3740001 	cmn	r4, #1
8000cdd4:	03a03000 	moveq	r3, #0
8000cdd8:	050b3ccc 	streq	r3, [fp, #-3276]	; 0xccc
8000cddc:	0affff58 	beq	8000cb44 <sd_read_ext2+0x2c8>
8000cde0:	e1a01004 	mov	r1, r4
8000cde4:	eaffff0a 	b	8000ca14 <sd_read_ext2+0x198>
8000cde8:	e51b2cd0 	ldr	r2, [fp, #-3280]	; 0xcd0
8000cdec:	e51b343c 	ldr	r3, [fp, #-1084]	; 0x43c
8000cdf0:	e2822001 	add	r2, r2, #1
8000cdf4:	e1520003 	cmp	r2, r3
8000cdf8:	e50b2cd0 	str	r2, [fp, #-3280]	; 0xcd0
8000cdfc:	baffffaa 	blt	8000ccac <sd_read_ext2+0x430>
8000ce00:	e3e04000 	mvn	r4, #0
8000ce04:	eaffffe9 	b	8000cdb0 <sd_read_ext2+0x534>
8000ce08:	e50b1ccc 	str	r1, [fp, #-3276]	; 0xccc
8000ce0c:	eaffff4c 	b	8000cb44 <sd_read_ext2+0x2c8>
8000ce10:	00013c0c 	andeq	r3, r1, ip, lsl #24
8000ce14:	00013bf4 	strdeq	r3, [r1], -r4
8000ce18:	fffffdd8 			; <UNDEFINED> instruction: 0xfffffdd8
8000ce1c:	0000562c 	andeq	r5, r0, ip, lsr #12
8000ce20:	000003ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>

8000ce24 <outc>:
8000ce24:	e59f3024 	ldr	r3, [pc, #36]	; 8000ce50 <outc+0x2c>
8000ce28:	e08f3003 	add	r3, pc, r3
8000ce2c:	e5932000 	ldr	r2, [r3]
8000ce30:	e2821001 	add	r1, r2, #1
8000ce34:	e351007f 	cmp	r1, #127	; 0x7f
8000ce38:	90832002 	addls	r2, r3, r2
8000ce3c:	93a0c000 	movls	ip, #0
8000ce40:	95c20004 	strbls	r0, [r2, #4]
8000ce44:	96831001 	strls	r1, [r3], r1
8000ce48:	95c3c004 	strbls	ip, [r3, #4]
8000ce4c:	e12fff1e 	bx	lr
8000ce50:	000136e0 	andeq	r3, r1, r0, ror #13

8000ce54 <init_console>:
8000ce54:	e59f0008 	ldr	r0, [pc, #8]	; 8000ce64 <init_console+0x10>
8000ce58:	e08f0000 	add	r0, pc, r0
8000ce5c:	e2800084 	add	r0, r0, #132	; 0x84
8000ce60:	eafff5dc 	b	8000a5d8 <console_init>
8000ce64:	000136b0 			; <UNDEFINED> instruction: 0x000136b0

8000ce68 <setup_console>:
8000ce68:	e1a0c00d 	mov	ip, sp
8000ce6c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000ce70:	e59f4044 	ldr	r4, [pc, #68]	; 8000cebc <setup_console+0x54>
8000ce74:	e24cb004 	sub	fp, ip, #4
8000ce78:	e08f4004 	add	r4, pc, r4
8000ce7c:	e5945084 	ldr	r5, [r4, #132]	; 0x84
8000ce80:	e3550000 	cmp	r5, #0
8000ce84:	189da830 	ldmne	sp, {r4, r5, fp, sp, pc}
8000ce88:	ebffede0 	bl	80008610 <fb_get_info>
8000ce8c:	e5902020 	ldr	r2, [r0, #32]
8000ce90:	e1a03000 	mov	r3, r0
8000ce94:	e3520000 	cmp	r2, #0
8000ce98:	089da830 	ldmeq	sp, {r4, r5, fp, sp, pc}
8000ce9c:	e1a00005 	mov	r0, r5
8000cea0:	e8930006 	ldm	r3, {r1, r2}
8000cea4:	ebffefea 	bl	80008e54 <graph_new>
8000cea8:	e5a40084 	str	r0, [r4, #132]!	; 0x84
8000ceac:	e1a00004 	mov	r0, r4
8000ceb0:	e24bd014 	sub	sp, fp, #20
8000ceb4:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
8000ceb8:	eafff68c 	b	8000a8f0 <console_reset>
8000cebc:	00013690 	muleq	r1, r0, r6

8000cec0 <uart_out>:
8000cec0:	e1a0c00d 	mov	ip, sp
8000cec4:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000cec8:	e24cb004 	sub	fp, ip, #4
8000cecc:	e1a04000 	mov	r4, r0
8000ced0:	ebfff868 	bl	8000b078 <strlen>
8000ced4:	e1a01004 	mov	r1, r4
8000ced8:	e1a02000 	mov	r2, r0
8000cedc:	e3a00000 	mov	r0, #0
8000cee0:	e24bd014 	sub	sp, fp, #20
8000cee4:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
8000cee8:	eaffed01 	b	800082f4 <uart_write>

8000ceec <flush_actled>:
8000ceec:	e1a0c00d 	mov	ip, sp
8000cef0:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000cef4:	e59f4028 	ldr	r4, [pc, #40]	; 8000cf24 <flush_actled+0x38>
8000cef8:	e24cb004 	sub	fp, ip, #4
8000cefc:	e3a00001 	mov	r0, #1
8000cf00:	ebffedd9 	bl	8000866c <act_led>
8000cf04:	e1a00004 	mov	r0, r4
8000cf08:	eb0012db 	bl	80011a7c <_delay>
8000cf0c:	e3a00000 	mov	r0, #0
8000cf10:	ebffedd5 	bl	8000866c <act_led>
8000cf14:	e1a00004 	mov	r0, r4
8000cf18:	e24bd014 	sub	sp, fp, #20
8000cf1c:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
8000cf20:	ea0012d5 	b	80011a7c <_delay>
8000cf24:	000f4240 	andeq	r4, pc, r0, asr #4

8000cf28 <printf>:
8000cf28:	e1a0c00d 	mov	ip, sp
8000cf2c:	e92d000f 	push	{r0, r1, r2, r3}
8000cf30:	e92dd870 	push	{r4, r5, r6, fp, ip, lr, pc}
8000cf34:	e59f6094 	ldr	r6, [pc, #148]	; 8000cfd0 <printf+0xa8>
8000cf38:	e24cb014 	sub	fp, ip, #20
8000cf3c:	e24dd00c 	sub	sp, sp, #12
8000cf40:	e3a00001 	mov	r0, #1
8000cf44:	ebffedc8 	bl	8000866c <act_led>
8000cf48:	e3a0000a 	mov	r0, #10
8000cf4c:	eb0012e3 	bl	80011ae0 <_delay_msec>
8000cf50:	e59f007c 	ldr	r0, [pc, #124]	; 8000cfd4 <printf+0xac>
8000cf54:	e28bc008 	add	ip, fp, #8
8000cf58:	e3a04000 	mov	r4, #0
8000cf5c:	e08f6006 	add	r6, pc, r6
8000cf60:	e1a0300c 	mov	r3, ip
8000cf64:	e1a01004 	mov	r1, r4
8000cf68:	e59b2004 	ldr	r2, [fp, #4]
8000cf6c:	e1a05006 	mov	r5, r6
8000cf70:	e08f0000 	add	r0, pc, r0
8000cf74:	e50bc020 	str	ip, [fp, #-32]
8000cf78:	e4854004 	str	r4, [r5], #4
8000cf7c:	ebfff906 	bl	8000b39c <v_printf>
8000cf80:	e1a00005 	mov	r0, r5
8000cf84:	ebffffcd 	bl	8000cec0 <uart_out>
8000cf88:	e1a00004 	mov	r0, r4
8000cf8c:	ebffedb6 	bl	8000866c <act_led>
8000cf90:	e5963084 	ldr	r3, [r6, #132]	; 0x84
8000cf94:	e1530004 	cmp	r3, r4
8000cf98:	0a00000a 	beq	8000cfc8 <printf+0xa0>
8000cf9c:	e1a01005 	mov	r1, r5
8000cfa0:	e2860084 	add	r0, r6, #132	; 0x84
8000cfa4:	ebfff6af 	bl	8000aa68 <console_put_string>
8000cfa8:	ebffed98 	bl	80008610 <fb_get_info>
8000cfac:	e5961084 	ldr	r1, [r6, #132]	; 0x84
8000cfb0:	e5911000 	ldr	r1, [r1]
8000cfb4:	e890000c 	ldm	r0, {r2, r3}
8000cfb8:	e1a00004 	mov	r0, r4
8000cfbc:	e0020293 	mul	r2, r3, r2
8000cfc0:	e1a02102 	lsl	r2, r2, #2
8000cfc4:	ebffed95 	bl	80008620 <fb_dev_write>
8000cfc8:	e24bd018 	sub	sp, fp, #24
8000cfcc:	e89da870 	ldm	sp, {r4, r5, r6, fp, sp, pc}
8000cfd0:	000135ac 	andeq	r3, r1, ip, lsr #11
8000cfd4:	fffffeac 			; <UNDEFINED> instruction: 0xfffffeac

8000cfd8 <close_console>:
8000cfd8:	e1a0c00d 	mov	ip, sp
8000cfdc:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000cfe0:	e59f4024 	ldr	r4, [pc, #36]	; 8000d00c <close_console+0x34>
8000cfe4:	e24cb004 	sub	fp, ip, #4
8000cfe8:	e08f4004 	add	r4, pc, r4
8000cfec:	e5940084 	ldr	r0, [r4, #132]	; 0x84
8000cff0:	e3500000 	cmp	r0, #0
8000cff4:	089da818 	ldmeq	sp, {r3, r4, fp, sp, pc}
8000cff8:	ebffefaf 	bl	80008ebc <graph_free>
8000cffc:	e2840084 	add	r0, r4, #132	; 0x84
8000d000:	e24bd014 	sub	sp, fp, #20
8000d004:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
8000d008:	eafff58f 	b	8000a64c <console_close>
8000d00c:	00013520 	andeq	r3, r1, r0, lsr #10

8000d010 <kmake_hole>:
8000d010:	e59fc090 	ldr	ip, [pc, #144]	; 8000d0a8 <kmake_hole+0x98>
8000d014:	e3500000 	cmp	r0, #0
8000d018:	11500001 	cmpne	r0, r1
8000d01c:	e1a02000 	mov	r2, r0
8000d020:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
8000d024:	23a00001 	movcs	r0, #1
8000d028:	33a00000 	movcc	r0, #0
8000d02c:	e08fc00c 	add	ip, pc, ip
8000d030:	249df004 	popcs	{pc}		; (ldrcs pc, [sp], #4)
8000d034:	e59f3070 	ldr	r3, [pc, #112]	; 8000d0ac <kmake_hole+0x9c>
8000d038:	e79c3003 	ldr	r3, [ip, r3]
8000d03c:	e2833dff 	add	r3, r3, #16320	; 0x3fc0
8000d040:	e283303f 	add	r3, r3, #63	; 0x3f
8000d044:	e3c33dff 	bic	r3, r3, #16320	; 0x3fc0
8000d048:	e3c3303f 	bic	r3, r3, #63	; 0x3f
8000d04c:	e2833621 	add	r3, r3, #34603008	; 0x2100000
8000d050:	e2833802 	add	r3, r3, #131072	; 0x20000
8000d054:	e1530002 	cmp	r3, r2
8000d058:	249df004 	popcs	{pc}		; (ldrcs pc, [sp], #4)
8000d05c:	e59f304c 	ldr	r3, [pc, #76]	; 8000d0b0 <kmake_hole+0xa0>
8000d060:	e79cc003 	ldr	ip, [ip, r3]
8000d064:	e79c3180 	ldr	r3, [ip, r0, lsl #3]
8000d068:	e08ce180 	add	lr, ip, r0, lsl #3
8000d06c:	e3530000 	cmp	r3, #0
8000d070:	0a000003 	beq	8000d084 <kmake_hole+0x74>
8000d074:	e2800001 	add	r0, r0, #1
8000d078:	e3500004 	cmp	r0, #4
8000d07c:	1afffff8 	bne	8000d064 <kmake_hole+0x54>
8000d080:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)
8000d084:	e2813eff 	add	r3, r1, #4080	; 0xff0
8000d088:	e283300f 	add	r3, r3, #15
8000d08c:	e3c33eff 	bic	r3, r3, #4080	; 0xff0
8000d090:	e3c22eff 	bic	r2, r2, #4080	; 0xff0
8000d094:	e3c3300f 	bic	r3, r3, #15
8000d098:	e3c2200f 	bic	r2, r2, #15
8000d09c:	e58e3004 	str	r3, [lr, #4]
8000d0a0:	e78c2180 	str	r2, [ip, r0, lsl #3]
8000d0a4:	e49df004 	pop	{pc}		; (ldr pc, [sp], #4)
8000d0a8:	0000efd0 	ldrdeq	lr, [r0], -r0
8000d0ac:	0000001c 	andeq	r0, r0, ip, lsl r0
8000d0b0:	00000004 	andeq	r0, r0, r4

8000d0b4 <kalloc_init>:
8000d0b4:	e59f30f0 	ldr	r3, [pc, #240]	; 8000d1ac <kalloc_init+0xf8>
8000d0b8:	e92d4030 	push	{r4, r5, lr}
8000d0bc:	e08f3003 	add	r3, pc, r3
8000d0c0:	e3a05000 	mov	r5, #0
8000d0c4:	e5835000 	str	r5, [r3]
8000d0c8:	e5835400 	str	r5, [r3, #1024]	; 0x400
8000d0cc:	e2800eff 	add	r0, r0, #4080	; 0xff0
8000d0d0:	e59f30d8 	ldr	r3, [pc, #216]	; 8000d1b0 <kalloc_init+0xfc>
8000d0d4:	e280000f 	add	r0, r0, #15
8000d0d8:	e3c00eff 	bic	r0, r0, #4080	; 0xff0
8000d0dc:	e3c11eff 	bic	r1, r1, #4080	; 0xff0
8000d0e0:	e3520000 	cmp	r2, #0
8000d0e4:	e3c0000f 	bic	r0, r0, #15
8000d0e8:	e3c1100f 	bic	r1, r1, #15
8000d0ec:	e08f3003 	add	r3, pc, r3
8000d0f0:	1a000010 	bne	8000d138 <kalloc_init+0x84>
8000d0f4:	e1500001 	cmp	r0, r1
8000d0f8:	0a00002a 	beq	8000d1a8 <kalloc_init+0xf4>
8000d0fc:	e1a03000 	mov	r3, r0
8000d100:	ea000000 	b	8000d108 <kalloc_init+0x54>
8000d104:	e1a0300c 	mov	r3, ip
8000d108:	e283ca01 	add	ip, r3, #4096	; 0x1000
8000d10c:	e151000c 	cmp	r1, ip
8000d110:	e5832000 	str	r2, [r3]
8000d114:	e1a02003 	mov	r2, r3
8000d118:	1afffff9 	bne	8000d104 <kalloc_init+0x50>
8000d11c:	e59f2090 	ldr	r2, [pc, #144]	; 8000d1b4 <kalloc_init+0x100>
8000d120:	e0603003 	rsb	r3, r0, r3
8000d124:	e1a03623 	lsr	r3, r3, #12
8000d128:	e08f2002 	add	r2, pc, r2
8000d12c:	e0800603 	add	r0, r0, r3, lsl #12
8000d130:	e5820000 	str	r0, [r2]
8000d134:	e8bd8030 	pop	{r4, r5, pc}
8000d138:	e1500001 	cmp	r0, r1
8000d13c:	08bd8030 	popeq	{r4, r5, pc}
8000d140:	e59f2070 	ldr	r2, [pc, #112]	; 8000d1b8 <kalloc_init+0x104>
8000d144:	e793c002 	ldr	ip, [r3, r2]
8000d148:	e3a03000 	mov	r3, #0
8000d14c:	e79c2003 	ldr	r2, [ip, r3]
8000d150:	e08c4003 	add	r4, ip, r3
8000d154:	e292e000 	adds	lr, r2, #0
8000d158:	13a0e001 	movne	lr, #1
8000d15c:	e1500002 	cmp	r0, r2
8000d160:	33a0e000 	movcc	lr, #0
8000d164:	e35e0000 	cmp	lr, #0
8000d168:	e2833008 	add	r3, r3, #8
8000d16c:	0a000002 	beq	8000d17c <kalloc_init+0xc8>
8000d170:	e5942004 	ldr	r2, [r4, #4]
8000d174:	e1500002 	cmp	r0, r2
8000d178:	9a000003 	bls	8000d18c <kalloc_init+0xd8>
8000d17c:	e3530020 	cmp	r3, #32
8000d180:	1afffff1 	bne	8000d14c <kalloc_init+0x98>
8000d184:	e5805000 	str	r5, [r0]
8000d188:	e1a05000 	mov	r5, r0
8000d18c:	e2800a01 	add	r0, r0, #4096	; 0x1000
8000d190:	e1510000 	cmp	r1, r0
8000d194:	1affffeb 	bne	8000d148 <kalloc_init+0x94>
8000d198:	e59f301c 	ldr	r3, [pc, #28]	; 8000d1bc <kalloc_init+0x108>
8000d19c:	e08f3003 	add	r3, pc, r3
8000d1a0:	e5835000 	str	r5, [r3]
8000d1a4:	e8bd8030 	pop	{r4, r5, pc}
8000d1a8:	e8bd8030 	pop	{r4, r5, pc}
8000d1ac:	00016f3c 	andeq	r6, r1, ip, lsr pc
8000d1b0:	0000ef10 	andeq	lr, r0, r0, lsl pc
8000d1b4:	00016ed0 	ldrdeq	r6, [r1], -r0
8000d1b8:	00000004 	andeq	r0, r0, r4
8000d1bc:	00016e5c 	andeq	r6, r1, ip, asr lr

8000d1c0 <kalloc4k>:
8000d1c0:	e59f3014 	ldr	r3, [pc, #20]	; 8000d1dc <kalloc4k+0x1c>
8000d1c4:	e08f3003 	add	r3, pc, r3
8000d1c8:	e5930000 	ldr	r0, [r3]
8000d1cc:	e3500000 	cmp	r0, #0
8000d1d0:	15902000 	ldrne	r2, [r0]
8000d1d4:	15832000 	strne	r2, [r3]
8000d1d8:	e12fff1e 	bx	lr
8000d1dc:	00016e34 	andeq	r6, r1, r4, lsr lr

8000d1e0 <kfree4k>:
8000d1e0:	e59f3010 	ldr	r3, [pc, #16]	; 8000d1f8 <kfree4k+0x18>
8000d1e4:	e08f3003 	add	r3, pc, r3
8000d1e8:	e5932000 	ldr	r2, [r3]
8000d1ec:	e5830000 	str	r0, [r3]
8000d1f0:	e5802000 	str	r2, [r0]
8000d1f4:	e12fff1e 	bx	lr
8000d1f8:	00016e14 	andeq	r6, r1, r4, lsl lr

8000d1fc <kalloc1k>:
8000d1fc:	e59f306c 	ldr	r3, [pc, #108]	; 8000d270 <kalloc1k+0x74>
8000d200:	e92d4010 	push	{r4, lr}
8000d204:	e08f3003 	add	r3, pc, r3
8000d208:	e5930400 	ldr	r0, [r3, #1024]	; 0x400
8000d20c:	e3500000 	cmp	r0, #0
8000d210:	15903000 	ldrne	r3, [r0]
8000d214:	0a000003 	beq	8000d228 <kalloc1k+0x2c>
8000d218:	e59f2054 	ldr	r2, [pc, #84]	; 8000d274 <kalloc1k+0x78>
8000d21c:	e08f2002 	add	r2, pc, r2
8000d220:	e5823400 	str	r3, [r2, #1024]	; 0x400
8000d224:	e8bd8010 	pop	{r4, pc}
8000d228:	e5931000 	ldr	r1, [r3]
8000d22c:	e3510000 	cmp	r1, #0
8000d230:	08bd8010 	popeq	{r4, pc}
8000d234:	e5914000 	ldr	r4, [r1]
8000d238:	e281cb03 	add	ip, r1, #3072	; 0xc00
8000d23c:	e35c0000 	cmp	ip, #0
8000d240:	e2812b02 	add	r2, r1, #2048	; 0x800
8000d244:	e281eb01 	add	lr, r1, #1024	; 0x400
8000d248:	e5811400 	str	r1, [r1, #1024]	; 0x400
8000d24c:	e5834000 	str	r4, [r3]
8000d250:	e5810000 	str	r0, [r1]
8000d254:	e583c400 	str	ip, [r3, #1024]	; 0x400
8000d258:	e5812c00 	str	r2, [r1, #3072]	; 0xc00
8000d25c:	e581e800 	str	lr, [r1, #2048]	; 0x800
8000d260:	08bd8010 	popeq	{r4, pc}
8000d264:	e1a03002 	mov	r3, r2
8000d268:	e1a0000c 	mov	r0, ip
8000d26c:	eaffffe9 	b	8000d218 <kalloc1k+0x1c>
8000d270:	00016df4 	strdeq	r6, [r1], -r4
8000d274:	00016ddc 	ldrdeq	r6, [r1], -ip

8000d278 <kfree1k>:
8000d278:	e59f3010 	ldr	r3, [pc, #16]	; 8000d290 <kfree1k+0x18>
8000d27c:	e08f3003 	add	r3, pc, r3
8000d280:	e5932400 	ldr	r2, [r3, #1024]	; 0x400
8000d284:	e5830400 	str	r0, [r3, #1024]	; 0x400
8000d288:	e5802000 	str	r2, [r0]
8000d28c:	e12fff1e 	bx	lr
8000d290:	00016d7c 	andeq	r6, r1, ip, ror sp

8000d294 <get_free_mem_size>:
8000d294:	e59f3050 	ldr	r3, [pc, #80]	; 8000d2ec <get_free_mem_size+0x58>
8000d298:	e08f3003 	add	r3, pc, r3
8000d29c:	e5933400 	ldr	r3, [r3, #1024]	; 0x400
8000d2a0:	e3530000 	cmp	r3, #0
8000d2a4:	0a00000e 	beq	8000d2e4 <get_free_mem_size+0x50>
8000d2a8:	e3a00000 	mov	r0, #0
8000d2ac:	e5933000 	ldr	r3, [r3]
8000d2b0:	e2800b01 	add	r0, r0, #1024	; 0x400
8000d2b4:	e3530000 	cmp	r3, #0
8000d2b8:	1afffffb 	bne	8000d2ac <get_free_mem_size+0x18>
8000d2bc:	e59f302c 	ldr	r3, [pc, #44]	; 8000d2f0 <get_free_mem_size+0x5c>
8000d2c0:	e08f3003 	add	r3, pc, r3
8000d2c4:	e5933000 	ldr	r3, [r3]
8000d2c8:	e3530000 	cmp	r3, #0
8000d2cc:	012fff1e 	bxeq	lr
8000d2d0:	e5933000 	ldr	r3, [r3]
8000d2d4:	e2800a01 	add	r0, r0, #4096	; 0x1000
8000d2d8:	e3530000 	cmp	r3, #0
8000d2dc:	1afffffb 	bne	8000d2d0 <get_free_mem_size+0x3c>
8000d2e0:	e12fff1e 	bx	lr
8000d2e4:	e1a00003 	mov	r0, r3
8000d2e8:	eafffff3 	b	8000d2bc <get_free_mem_size+0x28>
8000d2ec:	00016d60 	andeq	r6, r1, r0, ror #26
8000d2f0:	00016d38 	andeq	r6, r1, r8, lsr sp

8000d2f4 <map_page>:
8000d2f4:	e1a0c00d 	mov	ip, sp
8000d2f8:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000d2fc:	e24cb004 	sub	fp, ip, #4
8000d300:	e1a08a21 	lsr	r8, r1, #20
8000d304:	e24dd00c 	sub	sp, sp, #12
8000d308:	e1a09000 	mov	r9, r0
8000d30c:	e7d00108 	ldrb	r0, [r0, r8, lsl #2]
8000d310:	e1a05621 	lsr	r5, r1, #12
8000d314:	e210a003 	ands	sl, r0, #3
8000d318:	e1a06002 	mov	r6, r2
8000d31c:	e1a07003 	mov	r7, r3
8000d320:	e20550ff 	and	r5, r5, #255	; 0xff
8000d324:	e1a0c108 	lsl	ip, r8, #2
8000d328:	0a000016 	beq	8000d388 <map_page+0x94>
8000d32c:	e7994108 	ldr	r4, [r9, r8, lsl #2]
8000d330:	e1a04524 	lsr	r4, r4, #10
8000d334:	e1a04504 	lsl	r4, r4, #10
8000d338:	e2844102 	add	r4, r4, #-2147483648	; 0x80000000
8000d33c:	e7d42105 	ldrb	r2, [r4, r5, lsl #2]
8000d340:	e1a01105 	lsl	r1, r5, #2
8000d344:	e20220f0 	and	r2, r2, #240	; 0xf0
8000d348:	e3822002 	orr	r2, r2, #2
8000d34c:	e7c42105 	strb	r2, [r4, r5, lsl #2]
8000d350:	e19420b1 	ldrh	r2, [r4, r1]
8000d354:	e20730ff 	and	r3, r7, #255	; 0xff
8000d358:	e3c22eff 	bic	r2, r2, #4080	; 0xff0
8000d35c:	e1823203 	orr	r3, r2, r3, lsl #4
8000d360:	e18430b1 	strh	r3, [r4, r1]
8000d364:	e7943105 	ldr	r3, [r4, r5, lsl #2]
8000d368:	e1a06626 	lsr	r6, r6, #12
8000d36c:	e1a03a03 	lsl	r3, r3, #20
8000d370:	e1a03a23 	lsr	r3, r3, #20
8000d374:	e1833606 	orr	r3, r3, r6, lsl #12
8000d378:	e3a00000 	mov	r0, #0
8000d37c:	e7843105 	str	r3, [r4, r5, lsl #2]
8000d380:	e24bd028 	sub	sp, fp, #40	; 0x28
8000d384:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000d388:	e50bc030 	str	ip, [fp, #-48]	; 0x30
8000d38c:	ebffff9a 	bl	8000d1fc <kalloc1k>
8000d390:	e2504000 	subs	r4, r0, #0
8000d394:	0a000011 	beq	8000d3e0 <map_page+0xec>
8000d398:	e3a02b01 	mov	r2, #1024	; 0x400
8000d39c:	e1a0100a 	mov	r1, sl
8000d3a0:	ebfff632 	bl	8000ac70 <memset>
8000d3a4:	e7993108 	ldr	r3, [r9, r8, lsl #2]
8000d3a8:	e2842102 	add	r2, r4, #-2147483648	; 0x80000000
8000d3ac:	e1a03b03 	lsl	r3, r3, #22
8000d3b0:	e1a02522 	lsr	r2, r2, #10
8000d3b4:	e1a03b23 	lsr	r3, r3, #22
8000d3b8:	e1833502 	orr	r3, r3, r2, lsl #10
8000d3bc:	e51bc030 	ldr	ip, [fp, #-48]	; 0x30
8000d3c0:	e20320fd 	and	r2, r3, #253	; 0xfd
8000d3c4:	e3822001 	orr	r2, r2, #1
8000d3c8:	e7893108 	str	r3, [r9, r8, lsl #2]
8000d3cc:	e7c92108 	strb	r2, [r9, r8, lsl #2]
8000d3d0:	e19930bc 	ldrh	r3, [r9, ip]
8000d3d4:	e3c33e1e 	bic	r3, r3, #480	; 0x1e0
8000d3d8:	e18930bc 	strh	r3, [r9, ip]
8000d3dc:	eaffffd6 	b	8000d33c <map_page+0x48>
8000d3e0:	e3e00000 	mvn	r0, #0
8000d3e4:	eaffffe5 	b	8000d380 <map_page+0x8c>

8000d3e8 <map_pages>:
8000d3e8:	e1a0c00d 	mov	ip, sp
8000d3ec:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
8000d3f0:	e2833eff 	add	r3, r3, #4080	; 0xff0
8000d3f4:	e283500f 	add	r5, r3, #15
8000d3f8:	e3c55eff 	bic	r5, r5, #4080	; 0xff0
8000d3fc:	e3c22eff 	bic	r2, r2, #4080	; 0xff0
8000d400:	e3c5500f 	bic	r5, r5, #15
8000d404:	e3c2400f 	bic	r4, r2, #15
8000d408:	e24cb004 	sub	fp, ip, #4
8000d40c:	e3c11eff 	bic	r1, r1, #4080	; 0xff0
8000d410:	e1540005 	cmp	r4, r5
8000d414:	e3c1100f 	bic	r1, r1, #15
8000d418:	e1a07000 	mov	r7, r0
8000d41c:	e59b8004 	ldr	r8, [fp, #4]
8000d420:	289da9f8 	ldmcs	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000d424:	e0646001 	rsb	r6, r4, r1
8000d428:	e0861004 	add	r1, r6, r4
8000d42c:	e1a02004 	mov	r2, r4
8000d430:	e1a00007 	mov	r0, r7
8000d434:	e1a03008 	mov	r3, r8
8000d438:	e2844a01 	add	r4, r4, #4096	; 0x1000
8000d43c:	ebffffac 	bl	8000d2f4 <map_page>
8000d440:	e1550004 	cmp	r5, r4
8000d444:	8afffff7 	bhi	8000d428 <map_pages+0x40>
8000d448:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}

8000d44c <unmap_page>:
8000d44c:	e1a03a21 	lsr	r3, r1, #20
8000d450:	e7903103 	ldr	r3, [r0, r3, lsl #2]
8000d454:	e1a01521 	lsr	r1, r1, #10
8000d458:	e1a03523 	lsr	r3, r3, #10
8000d45c:	e2011fff 	and	r1, r1, #1020	; 0x3fc
8000d460:	e0811503 	add	r1, r1, r3, lsl #10
8000d464:	e2811102 	add	r1, r1, #-2147483648	; 0x80000000
8000d468:	e5d13000 	ldrb	r3, [r1]
8000d46c:	e3c33003 	bic	r3, r3, #3
8000d470:	e5c13000 	strb	r3, [r1]
8000d474:	e12fff1e 	bx	lr

8000d478 <unmap_pages>:
8000d478:	e1a0c00d 	mov	ip, sp
8000d47c:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000d480:	e2527000 	subs	r7, r2, #0
8000d484:	e24cb004 	sub	fp, ip, #4
8000d488:	e1a06000 	mov	r6, r0
8000d48c:	089da8f0 	ldmeq	sp, {r4, r5, r6, r7, fp, sp, pc}
8000d490:	e1a04001 	mov	r4, r1
8000d494:	e3a05000 	mov	r5, #0
8000d498:	e1a01004 	mov	r1, r4
8000d49c:	e2855001 	add	r5, r5, #1
8000d4a0:	e1a00006 	mov	r0, r6
8000d4a4:	ebffffe8 	bl	8000d44c <unmap_page>
8000d4a8:	e1550007 	cmp	r5, r7
8000d4ac:	e2844a01 	add	r4, r4, #4096	; 0x1000
8000d4b0:	1afffff8 	bne	8000d498 <unmap_pages+0x20>
8000d4b4:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}

8000d4b8 <resolve_phy_address>:
8000d4b8:	e1a03a21 	lsr	r3, r1, #20
8000d4bc:	e1800103 	orr	r0, r0, r3, lsl #2
8000d4c0:	e5903000 	ldr	r3, [r0]
8000d4c4:	e1a02521 	lsr	r2, r1, #10
8000d4c8:	e1a00523 	lsr	r0, r3, #10
8000d4cc:	e2023fff 	and	r3, r2, #1020	; 0x3fc
8000d4d0:	e1833500 	orr	r3, r3, r0, lsl #10
8000d4d4:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
8000d4d8:	e5933000 	ldr	r3, [r3]
8000d4dc:	e1a01a01 	lsl	r1, r1, #20
8000d4e0:	e1a03623 	lsr	r3, r3, #12
8000d4e4:	e1a00603 	lsl	r0, r3, #12
8000d4e8:	e1800a21 	orr	r0, r0, r1, lsr #20
8000d4ec:	e12fff1e 	bx	lr

8000d4f0 <resolve_kernel_address>:
8000d4f0:	e1a0c00d 	mov	ip, sp
8000d4f4:	e92dd800 	push	{fp, ip, lr, pc}
8000d4f8:	e24cb004 	sub	fp, ip, #4
8000d4fc:	ebffffed 	bl	8000d4b8 <resolve_phy_address>
8000d500:	e2800102 	add	r0, r0, #-2147483648	; 0x80000000
8000d504:	e89da800 	ldm	sp, {fp, sp, pc}

8000d508 <get_page_table_entry>:
8000d508:	e1a03a21 	lsr	r3, r1, #20
8000d50c:	e1800103 	orr	r0, r0, r3, lsl #2
8000d510:	e5900000 	ldr	r0, [r0]
8000d514:	e1a01521 	lsr	r1, r1, #10
8000d518:	e2011fff 	and	r1, r1, #1020	; 0x3fc
8000d51c:	e1a00520 	lsr	r0, r0, #10
8000d520:	e1810500 	orr	r0, r1, r0, lsl #10
8000d524:	e2800102 	add	r0, r0, #-2147483648	; 0x80000000
8000d528:	e12fff1e 	bx	lr

8000d52c <free_page_tables>:
8000d52c:	e1a0c00d 	mov	ip, sp
8000d530:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000d534:	e24cb004 	sub	fp, ip, #4
8000d538:	e1a04000 	mov	r4, r0
8000d53c:	e2805901 	add	r5, r0, #16384	; 0x4000
8000d540:	e5d43000 	ldrb	r3, [r4]
8000d544:	e3130003 	tst	r3, #3
8000d548:	0a000005 	beq	8000d564 <free_page_tables+0x38>
8000d54c:	e5940000 	ldr	r0, [r4]
8000d550:	e1a00520 	lsr	r0, r0, #10
8000d554:	e1a00500 	lsl	r0, r0, #10
8000d558:	e2900102 	adds	r0, r0, #-2147483648	; 0x80000000
8000d55c:	0a000000 	beq	8000d564 <free_page_tables+0x38>
8000d560:	ebffff44 	bl	8000d278 <kfree1k>
8000d564:	e2844004 	add	r4, r4, #4
8000d568:	e1540005 	cmp	r4, r5
8000d56c:	1afffff3 	bne	8000d540 <free_page_tables+0x14>
8000d570:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

8000d574 <try_break.isra.0>:
8000d574:	e5913008 	ldr	r3, [r1, #8]
8000d578:	e282c010 	add	ip, r2, #16
8000d57c:	e3c33102 	bic	r3, r3, #-2147483648	; 0x80000000
8000d580:	e15c00a3 	cmp	ip, r3, lsr #1
8000d584:	e92d4070 	push	{r4, r5, r6, lr}
8000d588:	88bd8070 	pophi	{r4, r5, r6, pc}
8000d58c:	e591400c 	ldr	r4, [r1, #12]
8000d590:	e2822003 	add	r2, r2, #3
8000d594:	e3c25003 	bic	r5, r2, #3
8000d598:	e084c005 	add	ip, r4, r5
8000d59c:	e59ce008 	ldr	lr, [ip, #8]
8000d5a0:	e2433010 	sub	r3, r3, #16
8000d5a4:	e0653003 	rsb	r3, r5, r3
8000d5a8:	e3c33102 	bic	r3, r3, #-2147483648	; 0x80000000
8000d5ac:	e20ee102 	and	lr, lr, #-2147483648	; 0x80000000
8000d5b0:	e183e00e 	orr	lr, r3, lr
8000d5b4:	e1a03c2e 	lsr	r3, lr, #24
8000d5b8:	e3a06000 	mov	r6, #0
8000d5bc:	e3c33080 	bic	r3, r3, #128	; 0x80
8000d5c0:	e58c6004 	str	r6, [ip, #4]
8000d5c4:	e7846005 	str	r6, [r4, r5]
8000d5c8:	e58ce008 	str	lr, [ip, #8]
8000d5cc:	e5cc300b 	strb	r3, [ip, #11]
8000d5d0:	e5913008 	ldr	r3, [r1, #8]
8000d5d4:	e591e000 	ldr	lr, [r1]
8000d5d8:	e3c2210e 	bic	r2, r2, #-2147483645	; 0x80000003
8000d5dc:	e2033102 	and	r3, r3, #-2147483648	; 0x80000000
8000d5e0:	e1823003 	orr	r3, r2, r3
8000d5e4:	e35e0000 	cmp	lr, #0
8000d5e8:	e28c6010 	add	r6, ip, #16
8000d5ec:	e58c600c 	str	r6, [ip, #12]
8000d5f0:	e5813008 	str	r3, [r1, #8]
8000d5f4:	e784e005 	str	lr, [r4, r5]
8000d5f8:	158ec004 	strne	ip, [lr, #4]
8000d5fc:	e58c1004 	str	r1, [ip, #4]
8000d600:	e581c000 	str	ip, [r1]
8000d604:	e5903000 	ldr	r3, [r0]
8000d608:	e1510003 	cmp	r1, r3
8000d60c:	0580c000 	streq	ip, [r0]
8000d610:	e8bd8070 	pop	{r4, r5, r6, pc}

8000d614 <trunk_malloc>:
8000d614:	e1a0c00d 	mov	ip, sp
8000d618:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000d61c:	e5904010 	ldr	r4, [r0, #16]
8000d620:	e24cb004 	sub	fp, ip, #4
8000d624:	e3540000 	cmp	r4, #0
8000d628:	e1a06000 	mov	r6, r0
8000d62c:	e1a05001 	mov	r5, r1
8000d630:	0a000009 	beq	8000d65c <trunk_malloc+0x48>
8000d634:	e5d4300b 	ldrb	r3, [r4, #11]
8000d638:	e3130080 	tst	r3, #128	; 0x80
8000d63c:	1a000003 	bne	8000d650 <trunk_malloc+0x3c>
8000d640:	e5941008 	ldr	r1, [r4, #8]
8000d644:	e3c11102 	bic	r1, r1, #-2147483648	; 0x80000000
8000d648:	e1510005 	cmp	r1, r5
8000d64c:	2a00002b 	bcs	8000d700 <trunk_malloc+0xec>
8000d650:	e5944000 	ldr	r4, [r4]
8000d654:	e3540000 	cmp	r4, #0
8000d658:	1afffff5 	bne	8000d634 <trunk_malloc+0x20>
8000d65c:	e2853010 	add	r3, r5, #16
8000d660:	e1b02a03 	lsls	r2, r3, #20
8000d664:	e1a04623 	lsr	r4, r3, #12
8000d668:	e5960000 	ldr	r0, [r6]
8000d66c:	e596300c 	ldr	r3, [r6, #12]
8000d670:	12844001 	addne	r4, r4, #1
8000d674:	e12fff33 	blx	r3
8000d678:	e1a01004 	mov	r1, r4
8000d67c:	e1a07000 	mov	r7, r0
8000d680:	e8960009 	ldm	r6, {r0, r3}
8000d684:	e12fff33 	blx	r3
8000d688:	e3500000 	cmp	r0, #0
8000d68c:	1a000023 	bne	8000d720 <trunk_malloc+0x10c>
8000d690:	e5972008 	ldr	r2, [r7, #8]
8000d694:	e1a03604 	lsl	r3, r4, #12
8000d698:	e2433010 	sub	r3, r3, #16
8000d69c:	e3c33102 	bic	r3, r3, #-2147483648	; 0x80000000
8000d6a0:	e2022102 	and	r2, r2, #-2147483648	; 0x80000000
8000d6a4:	e1832002 	orr	r2, r3, r2
8000d6a8:	e1a01c22 	lsr	r1, r2, #24
8000d6ac:	e1a03007 	mov	r3, r7
8000d6b0:	e3811080 	orr	r1, r1, #128	; 0x80
8000d6b4:	e5870004 	str	r0, [r7, #4]
8000d6b8:	e4830010 	str	r0, [r3], #16
8000d6bc:	e5872008 	str	r2, [r7, #8]
8000d6c0:	e5c7100b 	strb	r1, [r7, #11]
8000d6c4:	e5962010 	ldr	r2, [r6, #16]
8000d6c8:	e587300c 	str	r3, [r7, #12]
8000d6cc:	e5963014 	ldr	r3, [r6, #20]
8000d6d0:	e3520000 	cmp	r2, #0
8000d6d4:	05867010 	streq	r7, [r6, #16]
8000d6d8:	e3530000 	cmp	r3, #0
8000d6dc:	15837000 	strne	r7, [r3]
8000d6e0:	e2860014 	add	r0, r6, #20
8000d6e4:	15873004 	strne	r3, [r7, #4]
8000d6e8:	e1a02005 	mov	r2, r5
8000d6ec:	e5867014 	str	r7, [r6, #20]
8000d6f0:	e1a01007 	mov	r1, r7
8000d6f4:	ebffff9e 	bl	8000d574 <try_break.isra.0>
8000d6f8:	e597000c 	ldr	r0, [r7, #12]
8000d6fc:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000d700:	e3833080 	orr	r3, r3, #128	; 0x80
8000d704:	e2860014 	add	r0, r6, #20
8000d708:	e5c4300b 	strb	r3, [r4, #11]
8000d70c:	e1a02005 	mov	r2, r5
8000d710:	e1a01004 	mov	r1, r4
8000d714:	ebffff96 	bl	8000d574 <try_break.isra.0>
8000d718:	e594000c 	ldr	r0, [r4, #12]
8000d71c:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000d720:	e3a00000 	mov	r0, #0
8000d724:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}

8000d728 <trunk_free>:
8000d728:	e351000f 	cmp	r1, #15
8000d72c:	e52de004 	push	{lr}		; (str lr, [sp, #-4]!)
8000d730:	949df004 	popls	{pc}		; (ldrls pc, [sp], #4)
8000d734:	e5513005 	ldrb	r3, [r1, #-5]
8000d738:	e5112010 	ldr	r2, [r1, #-16]
8000d73c:	e3c33080 	bic	r3, r3, #128	; 0x80
8000d740:	e3520000 	cmp	r2, #0
8000d744:	e5413005 	strb	r3, [r1, #-5]
8000d748:	0a000002 	beq	8000d758 <trunk_free+0x30>
8000d74c:	e5d2300b 	ldrb	r3, [r2, #11]
8000d750:	e3130080 	tst	r3, #128	; 0x80
8000d754:	0a00002c 	beq	8000d80c <trunk_free+0xe4>
8000d758:	e511300c 	ldr	r3, [r1, #-12]
8000d75c:	e3530000 	cmp	r3, #0
8000d760:	0a000011 	beq	8000d7ac <trunk_free+0x84>
8000d764:	e5d3200b 	ldrb	r2, [r3, #11]
8000d768:	e3120080 	tst	r2, #128	; 0x80
8000d76c:	1a00000e 	bne	8000d7ac <trunk_free+0x84>
8000d770:	e593e008 	ldr	lr, [r3, #8]
8000d774:	e5112008 	ldr	r2, [r1, #-8]
8000d778:	e3cec102 	bic	ip, lr, #-2147483648	; 0x80000000
8000d77c:	e3c22102 	bic	r2, r2, #-2147483648	; 0x80000000
8000d780:	e5111010 	ldr	r1, [r1, #-16]
8000d784:	e08c2002 	add	r2, ip, r2
8000d788:	e2822010 	add	r2, r2, #16
8000d78c:	e20ee102 	and	lr, lr, #-2147483648	; 0x80000000
8000d790:	e3c22102 	bic	r2, r2, #-2147483648	; 0x80000000
8000d794:	e3510000 	cmp	r1, #0
8000d798:	e182200e 	orr	r2, r2, lr
8000d79c:	e5832008 	str	r2, [r3, #8]
8000d7a0:	e5831000 	str	r1, [r3]
8000d7a4:	15813004 	strne	r3, [r1, #4]
8000d7a8:	05803014 	streq	r3, [r0, #20]
8000d7ac:	e5902008 	ldr	r2, [r0, #8]
8000d7b0:	e3520000 	cmp	r2, #0
8000d7b4:	049df004 	popeq	{pc}		; (ldreq pc, [sp], #4)
8000d7b8:	e5903014 	ldr	r3, [r0, #20]
8000d7bc:	e3530000 	cmp	r3, #0
8000d7c0:	049df004 	popeq	{pc}		; (ldreq pc, [sp], #4)
8000d7c4:	e5d3100b 	ldrb	r1, [r3, #11]
8000d7c8:	e3110080 	tst	r1, #128	; 0x80
8000d7cc:	149df004 	popne	{pc}		; (ldrne pc, [sp], #4)
8000d7d0:	e1a01a03 	lsl	r1, r3, #20
8000d7d4:	e1b0ca21 	lsrs	ip, r1, #20
8000d7d8:	149df004 	popne	{pc}		; (ldrne pc, [sp], #4)
8000d7dc:	e5931008 	ldr	r1, [r3, #8]
8000d7e0:	e5933004 	ldr	r3, [r3, #4]
8000d7e4:	e3c11102 	bic	r1, r1, #-2147483648	; 0x80000000
8000d7e8:	e3530000 	cmp	r3, #0
8000d7ec:	e2811010 	add	r1, r1, #16
8000d7f0:	e5803014 	str	r3, [r0, #20]
8000d7f4:	05803010 	streq	r3, [r0, #16]
8000d7f8:	e1a01621 	lsr	r1, r1, #12
8000d7fc:	1583c000 	strne	ip, [r3]
8000d800:	e5900000 	ldr	r0, [r0]
8000d804:	e49de004 	pop	{lr}		; (ldr lr, [sp], #4)
8000d808:	e12fff12 	bx	r2
8000d80c:	e511e008 	ldr	lr, [r1, #-8]
8000d810:	e5923008 	ldr	r3, [r2, #8]
8000d814:	e3cec102 	bic	ip, lr, #-2147483648	; 0x80000000
8000d818:	e3c33102 	bic	r3, r3, #-2147483648	; 0x80000000
8000d81c:	e08c3003 	add	r3, ip, r3
8000d820:	e5922000 	ldr	r2, [r2]
8000d824:	e2833010 	add	r3, r3, #16
8000d828:	e20ee102 	and	lr, lr, #-2147483648	; 0x80000000
8000d82c:	e3c33102 	bic	r3, r3, #-2147483648	; 0x80000000
8000d830:	e183300e 	orr	r3, r3, lr
8000d834:	e5013008 	str	r3, [r1, #-8]
8000d838:	e3520000 	cmp	r2, #0
8000d83c:	e2413010 	sub	r3, r1, #16
8000d840:	e5012010 	str	r2, [r1, #-16]
8000d844:	15823004 	strne	r3, [r2, #4]
8000d848:	05803014 	streq	r3, [r0, #20]
8000d84c:	eaffffc1 	b	8000d758 <trunk_free+0x30>

8000d850 <km_shrink>:
8000d850:	e59f2010 	ldr	r2, [pc, #16]	; 8000d868 <km_shrink+0x18>
8000d854:	e08f2002 	add	r2, pc, r2
8000d858:	e5923000 	ldr	r3, [r2]
8000d85c:	e0431601 	sub	r1, r3, r1, lsl #12
8000d860:	e5821000 	str	r1, [r2]
8000d864:	e12fff1e 	bx	lr
8000d868:	00016ba8 	andeq	r6, r1, r8, lsr #23

8000d86c <km_expand>:
8000d86c:	e59f2044 	ldr	r2, [pc, #68]	; 8000d8b8 <km_expand+0x4c>
8000d870:	e59f3044 	ldr	r3, [pc, #68]	; 8000d8bc <km_expand+0x50>
8000d874:	e08f2002 	add	r2, pc, r2
8000d878:	e7923003 	ldr	r3, [r2, r3]
8000d87c:	e59f003c 	ldr	r0, [pc, #60]	; 8000d8c0 <km_expand+0x54>
8000d880:	e2833dff 	add	r3, r3, #16320	; 0x3fc0
8000d884:	e08f0000 	add	r0, pc, r0
8000d888:	e283303f 	add	r3, r3, #63	; 0x3f
8000d88c:	e5902000 	ldr	r2, [r0]
8000d890:	e3c33dff 	bic	r3, r3, #16320	; 0x3fc0
8000d894:	e3c3303f 	bic	r3, r3, #63	; 0x3f
8000d898:	e2833402 	add	r3, r3, #33554432	; 0x2000000
8000d89c:	e0821601 	add	r1, r2, r1, lsl #12
8000d8a0:	e2833802 	add	r3, r3, #131072	; 0x20000
8000d8a4:	e1530001 	cmp	r3, r1
8000d8a8:	25801000 	strcs	r1, [r0]
8000d8ac:	23a00000 	movcs	r0, #0
8000d8b0:	33e00000 	mvncc	r0, #0
8000d8b4:	e12fff1e 	bx	lr
8000d8b8:	0000e788 	andeq	lr, r0, r8, lsl #15
8000d8bc:	0000001c 	andeq	r0, r0, ip, lsl r0
8000d8c0:	00016b78 	andeq	r6, r1, r8, ror fp

8000d8c4 <km_get_mem_tail>:
8000d8c4:	e59f3004 	ldr	r3, [pc, #4]	; 8000d8d0 <km_get_mem_tail+0xc>
8000d8c8:	e79f0003 	ldr	r0, [pc, r3]
8000d8cc:	e12fff1e 	bx	lr
8000d8d0:	00016b34 	andeq	r6, r1, r4, lsr fp

8000d8d4 <km_init>:
8000d8d4:	e1a0c00d 	mov	ip, sp
8000d8d8:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000d8dc:	e59f406c 	ldr	r4, [pc, #108]	; 8000d950 <km_init+0x7c>
8000d8e0:	e24cb004 	sub	fp, ip, #4
8000d8e4:	e08f4004 	add	r4, pc, r4
8000d8e8:	e3a01000 	mov	r1, #0
8000d8ec:	e3a02018 	mov	r2, #24
8000d8f0:	e2840004 	add	r0, r4, #4
8000d8f4:	ebfff4dd 	bl	8000ac70 <memset>
8000d8f8:	e59f2054 	ldr	r2, [pc, #84]	; 8000d954 <km_init+0x80>
8000d8fc:	e59f3054 	ldr	r3, [pc, #84]	; 8000d958 <km_init+0x84>
8000d900:	e08f2002 	add	r2, pc, r2
8000d904:	e7923003 	ldr	r3, [r2, r3]
8000d908:	e59f004c 	ldr	r0, [pc, #76]	; 8000d95c <km_init+0x88>
8000d90c:	e2833dff 	add	r3, r3, #16320	; 0x3fc0
8000d910:	e59f1048 	ldr	r1, [pc, #72]	; 8000d960 <km_init+0x8c>
8000d914:	e283303f 	add	r3, r3, #63	; 0x3f
8000d918:	e59f2044 	ldr	r2, [pc, #68]	; 8000d964 <km_init+0x90>
8000d91c:	e3c33dff 	bic	r3, r3, #16320	; 0x3fc0
8000d920:	e3c3303f 	bic	r3, r3, #63	; 0x3f
8000d924:	e08f0000 	add	r0, pc, r0
8000d928:	e08f1001 	add	r1, pc, r1
8000d92c:	e08f2002 	add	r2, pc, r2
8000d930:	e2833802 	add	r3, r3, #131072	; 0x20000
8000d934:	e3a0c000 	mov	ip, #0
8000d938:	e5843000 	str	r3, [r4]
8000d93c:	e5840008 	str	r0, [r4, #8]
8000d940:	e584100c 	str	r1, [r4, #12]
8000d944:	e5842010 	str	r2, [r4, #16]
8000d948:	e584c004 	str	ip, [r4, #4]
8000d94c:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000d950:	00016b18 	andeq	r6, r1, r8, lsl fp
8000d954:	0000e6fc 	strdeq	lr, [r0], -ip
8000d958:	0000001c 	andeq	r0, r0, ip, lsl r0
8000d95c:	ffffff40 			; <UNDEFINED> instruction: 0xffffff40
8000d960:	ffffff20 			; <UNDEFINED> instruction: 0xffffff20
8000d964:	ffffff90 			; <UNDEFINED> instruction: 0xffffff90

8000d968 <kmalloc>:
8000d968:	e1a0c00d 	mov	ip, sp
8000d96c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000d970:	e59f3034 	ldr	r3, [pc, #52]	; 8000d9ac <kmalloc+0x44>
8000d974:	e1a01000 	mov	r1, r0
8000d978:	e08f3003 	add	r3, pc, r3
8000d97c:	e24cb004 	sub	fp, ip, #4
8000d980:	e2830004 	add	r0, r3, #4
8000d984:	ebffff22 	bl	8000d614 <trunk_malloc>
8000d988:	e2504000 	subs	r4, r0, #0
8000d98c:	0a000001 	beq	8000d998 <kmalloc+0x30>
8000d990:	e1a00004 	mov	r0, r4
8000d994:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000d998:	e59f0010 	ldr	r0, [pc, #16]	; 8000d9b0 <kmalloc+0x48>
8000d99c:	e08f0000 	add	r0, pc, r0
8000d9a0:	ebfffd60 	bl	8000cf28 <printf>
8000d9a4:	e1a00004 	mov	r0, r4
8000d9a8:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000d9ac:	00016a84 	andeq	r6, r1, r4, lsl #21
8000d9b0:	00004694 	muleq	r0, r4, r6

8000d9b4 <kfree>:
8000d9b4:	e2501000 	subs	r1, r0, #0
8000d9b8:	012fff1e 	bxeq	lr
8000d9bc:	e59f0008 	ldr	r0, [pc, #8]	; 8000d9cc <kfree+0x18>
8000d9c0:	e08f0000 	add	r0, pc, r0
8000d9c4:	e2800004 	add	r0, r0, #4
8000d9c8:	eaffff56 	b	8000d728 <trunk_free>
8000d9cc:	00016a3c 	andeq	r6, r1, ip, lsr sl

8000d9d0 <krealloc_raw>:
8000d9d0:	e1a0c00d 	mov	ip, sp
8000d9d4:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000d9d8:	e24cb004 	sub	fp, ip, #4
8000d9dc:	e1a04000 	mov	r4, r0
8000d9e0:	e1a00002 	mov	r0, r2
8000d9e4:	e1a06001 	mov	r6, r1
8000d9e8:	ebffffde 	bl	8000d968 <kmalloc>
8000d9ec:	e1a01004 	mov	r1, r4
8000d9f0:	e1a02006 	mov	r2, r6
8000d9f4:	e1a05000 	mov	r5, r0
8000d9f8:	ebfff480 	bl	8000ac00 <memcpy>
8000d9fc:	e1a00004 	mov	r0, r4
8000da00:	ebffffeb 	bl	8000d9b4 <kfree>
8000da04:	e1a00005 	mov	r0, r5
8000da08:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}

8000da0c <shm_new>:
8000da0c:	e1a0c00d 	mov	ip, sp
8000da10:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000da14:	e3a00024 	mov	r0, #36	; 0x24
8000da18:	e24cb004 	sub	fp, ip, #4
8000da1c:	ebffffd1 	bl	8000d968 <kmalloc>
8000da20:	e2504000 	subs	r4, r0, #0
8000da24:	0a00000a 	beq	8000da54 <shm_new+0x48>
8000da28:	e3a01000 	mov	r1, #0
8000da2c:	e3a02024 	mov	r2, #36	; 0x24
8000da30:	ebfff48e 	bl	8000ac70 <memset>
8000da34:	e59f3020 	ldr	r3, [pc, #32]	; 8000da5c <shm_new+0x50>
8000da38:	e3e01000 	mvn	r1, #0
8000da3c:	e08f3003 	add	r3, pc, r3
8000da40:	e5932000 	ldr	r2, [r3]
8000da44:	e5841014 	str	r1, [r4, #20]
8000da48:	e2821001 	add	r1, r2, #1
8000da4c:	e5842000 	str	r2, [r4]
8000da50:	e5831000 	str	r1, [r3]
8000da54:	e1a00004 	mov	r0, r4
8000da58:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000da5c:	0000e5bc 			; <UNDEFINED> instruction: 0x0000e5bc

8000da60 <shm_init>:
8000da60:	e59f3028 	ldr	r3, [pc, #40]	; 8000da90 <shm_init+0x30>
8000da64:	e59f2028 	ldr	r2, [pc, #40]	; 8000da94 <shm_init+0x34>
8000da68:	e08f3003 	add	r3, pc, r3
8000da6c:	e3a01000 	mov	r1, #0
8000da70:	e08f2002 	add	r2, pc, r2
8000da74:	e3a0c101 	mov	ip, #1073741824	; 0x40000000
8000da78:	e3a00001 	mov	r0, #1
8000da7c:	e583c000 	str	ip, [r3]
8000da80:	e5831004 	str	r1, [r3, #4]
8000da84:	e5831008 	str	r1, [r3, #8]
8000da88:	e5820000 	str	r0, [r2]
8000da8c:	e12fff1e 	bx	lr
8000da90:	000169b0 			; <UNDEFINED> instruction: 0x000169b0
8000da94:	0000e588 	andeq	lr, r0, r8, lsl #11

8000da98 <shm_alloc>:
8000da98:	e59f3244 	ldr	r3, [pc, #580]	; 8000dce4 <shm_alloc+0x24c>
8000da9c:	e1a0c00d 	mov	ip, sp
8000daa0:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000daa4:	e59f723c 	ldr	r7, [pc, #572]	; 8000dce8 <shm_alloc+0x250>
8000daa8:	e24cb004 	sub	fp, ip, #4
8000daac:	e08f3003 	add	r3, pc, r3
8000dab0:	e24dd00c 	sub	sp, sp, #12
8000dab4:	e5936000 	ldr	r6, [r3]
8000dab8:	e59f322c 	ldr	r3, [pc, #556]	; 8000dcec <shm_alloc+0x254>
8000dabc:	e1b02a00 	lsls	r2, r0, #20
8000dac0:	e08f3003 	add	r3, pc, r3
8000dac4:	e5934004 	ldr	r4, [r3, #4]
8000dac8:	e1a05620 	lsr	r5, r0, #12
8000dacc:	12855001 	addne	r5, r5, #1
8000dad0:	e3540000 	cmp	r4, #0
8000dad4:	e08f7007 	add	r7, pc, r7
8000dad8:	e50b1030 	str	r1, [fp, #-48]	; 0x30
8000dadc:	0a000008 	beq	8000db04 <shm_alloc+0x6c>
8000dae0:	e594300c 	ldr	r3, [r4, #12]
8000dae4:	e3530000 	cmp	r3, #0
8000dae8:	1a000002 	bne	8000daf8 <shm_alloc+0x60>
8000daec:	e5943008 	ldr	r3, [r4, #8]
8000daf0:	e1550003 	cmp	r5, r3
8000daf4:	9a000011 	bls	8000db40 <shm_alloc+0xa8>
8000daf8:	e594401c 	ldr	r4, [r4, #28]
8000dafc:	e3540000 	cmp	r4, #0
8000db00:	1afffff6 	bne	8000dae0 <shm_alloc+0x48>
8000db04:	ebffffc0 	bl	8000da0c <shm_new>
8000db08:	e2509000 	subs	r9, r0, #0
8000db0c:	0a000072 	beq	8000dcdc <shm_alloc+0x244>
8000db10:	e59f21d8 	ldr	r2, [pc, #472]	; 8000dcf0 <shm_alloc+0x258>
8000db14:	e5896004 	str	r6, [r9, #4]
8000db18:	e08f2002 	add	r2, pc, r2
8000db1c:	e5923004 	ldr	r3, [r2, #4]
8000db20:	e3530000 	cmp	r3, #0
8000db24:	0a000068 	beq	8000dccc <shm_alloc+0x234>
8000db28:	e5921008 	ldr	r1, [r2, #8]
8000db2c:	e5993008 	ldr	r3, [r9, #8]
8000db30:	e5829008 	str	r9, [r2, #8]
8000db34:	e581901c 	str	r9, [r1, #28]
8000db38:	e5891020 	str	r1, [r9, #32]
8000db3c:	ea000002 	b	8000db4c <shm_alloc+0xb4>
8000db40:	e1a09004 	mov	r9, r4
8000db44:	e5946004 	ldr	r6, [r4, #4]
8000db48:	3a000043 	bcc	8000dc5c <shm_alloc+0x1c4>
8000db4c:	e3530000 	cmp	r3, #0
8000db50:	0a000011 	beq	8000db9c <shm_alloc+0x104>
8000db54:	e59f3198 	ldr	r3, [pc, #408]	; 8000dcf4 <shm_alloc+0x25c>
8000db58:	e5990000 	ldr	r0, [r9]
8000db5c:	e08f3003 	add	r3, pc, r3
8000db60:	e5932000 	ldr	r2, [r3]
8000db64:	e1560002 	cmp	r6, r2
8000db68:	00865605 	addeq	r5, r6, r5, lsl #12
8000db6c:	05835000 	streq	r5, [r3]
8000db70:	e59f3180 	ldr	r3, [pc, #384]	; 8000dcf8 <shm_alloc+0x260>
8000db74:	e3a02001 	mov	r2, #1
8000db78:	e589200c 	str	r2, [r9, #12]
8000db7c:	e7973003 	ldr	r3, [r7, r3]
8000db80:	e51b2030 	ldr	r2, [fp, #-48]	; 0x30
8000db84:	e5933000 	ldr	r3, [r3]
8000db88:	e5933004 	ldr	r3, [r3, #4]
8000db8c:	e5892010 	str	r2, [r9, #16]
8000db90:	e5893014 	str	r3, [r9, #20]
8000db94:	e24bd028 	sub	sp, fp, #40	; 0x28
8000db98:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000db9c:	e3550000 	cmp	r5, #0
8000dba0:	0a000047 	beq	8000dcc4 <shm_alloc+0x22c>
8000dba4:	e1a04003 	mov	r4, r3
8000dba8:	e1a0a006 	mov	sl, r6
8000dbac:	ea00000b 	b	8000dbe0 <shm_alloc+0x148>
8000dbb0:	ebfff42e 	bl	8000ac70 <memset>
8000dbb4:	e59f3140 	ldr	r3, [pc, #320]	; 8000dcfc <shm_alloc+0x264>
8000dbb8:	e2882102 	add	r2, r8, #-2147483648	; 0x80000000
8000dbbc:	e7978003 	ldr	r8, [r7, r3]
8000dbc0:	e1a0100a 	mov	r1, sl
8000dbc4:	e2844001 	add	r4, r4, #1
8000dbc8:	e5980000 	ldr	r0, [r8]
8000dbcc:	e3a03055 	mov	r3, #85	; 0x55
8000dbd0:	ebfffdc7 	bl	8000d2f4 <map_page>
8000dbd4:	e1540005 	cmp	r4, r5
8000dbd8:	e28aaa01 	add	sl, sl, #4096	; 0x1000
8000dbdc:	0a000038 	beq	8000dcc4 <shm_alloc+0x22c>
8000dbe0:	ebfffd76 	bl	8000d1c0 <kalloc4k>
8000dbe4:	e3a01000 	mov	r1, #0
8000dbe8:	e3a02a01 	mov	r2, #4096	; 0x1000
8000dbec:	e2508000 	subs	r8, r0, #0
8000dbf0:	1affffee 	bne	8000dbb0 <shm_alloc+0x118>
8000dbf4:	e59f0104 	ldr	r0, [pc, #260]	; 8000dd00 <shm_alloc+0x268>
8000dbf8:	e1a01008 	mov	r1, r8
8000dbfc:	e08f0000 	add	r0, pc, r0
8000dc00:	ebfffcc8 	bl	8000cf28 <printf>
8000dc04:	e3540000 	cmp	r4, #0
8000dc08:	0a00000f 	beq	8000dc4c <shm_alloc+0x1b4>
8000dc0c:	e59f30e8 	ldr	r3, [pc, #232]	; 8000dcfc <shm_alloc+0x264>
8000dc10:	e1a05008 	mov	r5, r8
8000dc14:	e7978003 	ldr	r8, [r7, r3]
8000dc18:	e1a01006 	mov	r1, r6
8000dc1c:	e5980000 	ldr	r0, [r8]
8000dc20:	ebfffe24 	bl	8000d4b8 <resolve_phy_address>
8000dc24:	e1a01006 	mov	r1, r6
8000dc28:	e2855001 	add	r5, r5, #1
8000dc2c:	e2866a01 	add	r6, r6, #4096	; 0x1000
8000dc30:	e1a07000 	mov	r7, r0
8000dc34:	e5980000 	ldr	r0, [r8]
8000dc38:	ebfffe03 	bl	8000d44c <unmap_page>
8000dc3c:	e2870102 	add	r0, r7, #-2147483648	; 0x80000000
8000dc40:	ebffff5b 	bl	8000d9b4 <kfree>
8000dc44:	e1550004 	cmp	r5, r4
8000dc48:	1afffff2 	bne	8000dc18 <shm_alloc+0x180>
8000dc4c:	eb000fb4 	bl	80011b24 <_flush_tlb>
8000dc50:	e3e00000 	mvn	r0, #0
8000dc54:	e24bd028 	sub	sp, fp, #40	; 0x28
8000dc58:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000dc5c:	ebffff6a 	bl	8000da0c <shm_new>
8000dc60:	e3500000 	cmp	r0, #0
8000dc64:	0a00001a 	beq	8000dcd4 <shm_alloc+0x23c>
8000dc68:	e5942008 	ldr	r2, [r4, #8]
8000dc6c:	e5943004 	ldr	r3, [r4, #4]
8000dc70:	e594101c 	ldr	r1, [r4, #28]
8000dc74:	e0652002 	rsb	r2, r5, r2
8000dc78:	e0833605 	add	r3, r3, r5, lsl #12
8000dc7c:	e5802008 	str	r2, [r0, #8]
8000dc80:	e5803004 	str	r3, [r0, #4]
8000dc84:	e5845008 	str	r5, [r4, #8]
8000dc88:	e580101c 	str	r1, [r0, #28]
8000dc8c:	e594301c 	ldr	r3, [r4, #28]
8000dc90:	e5804020 	str	r4, [r0, #32]
8000dc94:	e3530000 	cmp	r3, #0
8000dc98:	15830020 	strne	r0, [r3, #32]
8000dc9c:	e59f3060 	ldr	r3, [pc, #96]	; 8000dd04 <shm_alloc+0x26c>
8000dca0:	e584001c 	str	r0, [r4, #28]
8000dca4:	e08f3003 	add	r3, pc, r3
8000dca8:	e5932008 	ldr	r2, [r3, #8]
8000dcac:	e1520004 	cmp	r2, r4
8000dcb0:	05830008 	streq	r0, [r3, #8]
8000dcb4:	01a09004 	moveq	r9, r4
8000dcb8:	01a03005 	moveq	r3, r5
8000dcbc:	11a03005 	movne	r3, r5
8000dcc0:	eaffffa1 	b	8000db4c <shm_alloc+0xb4>
8000dcc4:	e5895008 	str	r5, [r9, #8]
8000dcc8:	eaffffa1 	b	8000db54 <shm_alloc+0xbc>
8000dccc:	e5829008 	str	r9, [r2, #8]
8000dcd0:	e5829004 	str	r9, [r2, #4]
8000dcd4:	e5993008 	ldr	r3, [r9, #8]
8000dcd8:	eaffff9b 	b	8000db4c <shm_alloc+0xb4>
8000dcdc:	e3e00000 	mvn	r0, #0
8000dce0:	eaffffab 	b	8000db94 <shm_alloc+0xfc>
8000dce4:	0001696c 	andeq	r6, r1, ip, ror #18
8000dce8:	0000e528 	andeq	lr, r0, r8, lsr #10
8000dcec:	00016958 	andeq	r6, r1, r8, asr r9
8000dcf0:	00016900 	andeq	r6, r1, r0, lsl #18
8000dcf4:	000168bc 			; <UNDEFINED> instruction: 0x000168bc
8000dcf8:	00000000 	andeq	r0, r0, r0
8000dcfc:	00000014 	andeq	r0, r0, r4, lsl r0
8000dd00:	00004450 	andeq	r4, r0, r0, asr r4
8000dd04:	00016774 	andeq	r6, r1, r4, ror r7

8000dd08 <shm_alloced_size>:
8000dd08:	e59f3038 	ldr	r3, [pc, #56]	; 8000dd48 <shm_alloced_size+0x40>
8000dd0c:	e08f3003 	add	r3, pc, r3
8000dd10:	e5933004 	ldr	r3, [r3, #4]
8000dd14:	e3530000 	cmp	r3, #0
8000dd18:	0a000008 	beq	8000dd40 <shm_alloced_size+0x38>
8000dd1c:	e3a00000 	mov	r0, #0
8000dd20:	e593200c 	ldr	r2, [r3, #12]
8000dd24:	e3520000 	cmp	r2, #0
8000dd28:	15932008 	ldrne	r2, [r3, #8]
8000dd2c:	e593301c 	ldr	r3, [r3, #28]
8000dd30:	10800602 	addne	r0, r0, r2, lsl #12
8000dd34:	e3530000 	cmp	r3, #0
8000dd38:	1afffff8 	bne	8000dd20 <shm_alloced_size+0x18>
8000dd3c:	e12fff1e 	bx	lr
8000dd40:	e1a00003 	mov	r0, r3
8000dd44:	e12fff1e 	bx	lr
8000dd48:	0001670c 	andeq	r6, r1, ip, lsl #14

8000dd4c <shm_raw>:
8000dd4c:	e59f3040 	ldr	r3, [pc, #64]	; 8000dd94 <shm_raw+0x48>
8000dd50:	e08f3003 	add	r3, pc, r3
8000dd54:	e5933004 	ldr	r3, [r3, #4]
8000dd58:	e3530000 	cmp	r3, #0
8000dd5c:	0a000008 	beq	8000dd84 <shm_raw+0x38>
8000dd60:	e593200c 	ldr	r2, [r3, #12]
8000dd64:	e3520000 	cmp	r2, #0
8000dd68:	0a000002 	beq	8000dd78 <shm_raw+0x2c>
8000dd6c:	e5932000 	ldr	r2, [r3]
8000dd70:	e1520000 	cmp	r2, r0
8000dd74:	0a000004 	beq	8000dd8c <shm_raw+0x40>
8000dd78:	e593301c 	ldr	r3, [r3, #28]
8000dd7c:	e3530000 	cmp	r3, #0
8000dd80:	1afffff6 	bne	8000dd60 <shm_raw+0x14>
8000dd84:	e1a00003 	mov	r0, r3
8000dd88:	e12fff1e 	bx	lr
8000dd8c:	e5930004 	ldr	r0, [r3, #4]
8000dd90:	e12fff1e 	bx	lr
8000dd94:	000166c8 	andeq	r6, r1, r8, asr #13

8000dd98 <shm_proc_ref>:
8000dd98:	e1a0c00d 	mov	ip, sp
8000dd9c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000dda0:	e59f3088 	ldr	r3, [pc, #136]	; 8000de30 <shm_proc_ref+0x98>
8000dda4:	e24cb004 	sub	fp, ip, #4
8000dda8:	e08f3003 	add	r3, pc, r3
8000ddac:	e5934004 	ldr	r4, [r3, #4]
8000ddb0:	e3540000 	cmp	r4, #0
8000ddb4:	0a000008 	beq	8000dddc <shm_proc_ref+0x44>
8000ddb8:	e594300c 	ldr	r3, [r4, #12]
8000ddbc:	e3530000 	cmp	r3, #0
8000ddc0:	0a000002 	beq	8000ddd0 <shm_proc_ref+0x38>
8000ddc4:	e5943000 	ldr	r3, [r4]
8000ddc8:	e1530001 	cmp	r3, r1
8000ddcc:	0a000005 	beq	8000dde8 <shm_proc_ref+0x50>
8000ddd0:	e594401c 	ldr	r4, [r4, #28]
8000ddd4:	e3540000 	cmp	r4, #0
8000ddd8:	1afffff6 	bne	8000ddb8 <shm_proc_ref+0x20>
8000dddc:	eb000666 	bl	8000f77c <proc_get>
8000dde0:	e3e00000 	mvn	r0, #0
8000dde4:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000dde8:	eb000663 	bl	8000f77c <proc_get>
8000ddec:	e3500000 	cmp	r0, #0
8000ddf0:	0afffffa 	beq	8000dde0 <shm_proc_ref+0x48>
8000ddf4:	e5943010 	ldr	r3, [r4, #16]
8000ddf8:	e3530000 	cmp	r3, #0
8000ddfc:	0a000001 	beq	8000de08 <shm_proc_ref+0x70>
8000de00:	e5940018 	ldr	r0, [r4, #24]
8000de04:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000de08:	e5902004 	ldr	r2, [r0, #4]
8000de0c:	e5943014 	ldr	r3, [r4, #20]
8000de10:	e1520003 	cmp	r2, r3
8000de14:	0afffff9 	beq	8000de00 <shm_proc_ref+0x68>
8000de18:	e5900008 	ldr	r0, [r0, #8]
8000de1c:	eb000656 	bl	8000f77c <proc_get>
8000de20:	e3500000 	cmp	r0, #0
8000de24:	1afffff7 	bne	8000de08 <shm_proc_ref+0x70>
8000de28:	e3e00000 	mvn	r0, #0
8000de2c:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000de30:	00016670 	andeq	r6, r1, r0, ror r6

8000de34 <shm_proc_map>:
8000de34:	e1a0c00d 	mov	ip, sp
8000de38:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
8000de3c:	e59f312c 	ldr	r3, [pc, #300]	; 8000df70 <shm_proc_map+0x13c>
8000de40:	e59f612c 	ldr	r6, [pc, #300]	; 8000df74 <shm_proc_map+0x140>
8000de44:	e08f3003 	add	r3, pc, r3
8000de48:	e5934004 	ldr	r4, [r3, #4]
8000de4c:	e24cb004 	sub	fp, ip, #4
8000de50:	e3540000 	cmp	r4, #0
8000de54:	e1a07001 	mov	r7, r1
8000de58:	e08f6006 	add	r6, pc, r6
8000de5c:	0a000008 	beq	8000de84 <shm_proc_map+0x50>
8000de60:	e594300c 	ldr	r3, [r4, #12]
8000de64:	e3530000 	cmp	r3, #0
8000de68:	0a000002 	beq	8000de78 <shm_proc_map+0x44>
8000de6c:	e5943000 	ldr	r3, [r4]
8000de70:	e1530007 	cmp	r3, r7
8000de74:	0a000005 	beq	8000de90 <shm_proc_map+0x5c>
8000de78:	e594401c 	ldr	r4, [r4, #28]
8000de7c:	e3540000 	cmp	r4, #0
8000de80:	1afffff6 	bne	8000de60 <shm_proc_map+0x2c>
8000de84:	eb00063c 	bl	8000f77c <proc_get>
8000de88:	e3a00000 	mov	r0, #0
8000de8c:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000de90:	eb000639 	bl	8000f77c <proc_get>
8000de94:	e2505000 	subs	r5, r0, #0
8000de98:	0afffffa 	beq	8000de88 <shm_proc_map+0x54>
8000de9c:	e5943010 	ldr	r3, [r4, #16]
8000dea0:	e3530000 	cmp	r3, #0
8000dea4:	0a000026 	beq	8000df44 <shm_proc_map+0x110>
8000dea8:	e595102c 	ldr	r1, [r5, #44]	; 0x2c
8000deac:	e3a03000 	mov	r3, #0
8000deb0:	e281201c 	add	r2, r1, #28
8000deb4:	ea000002 	b	8000dec4 <shm_proc_map+0x90>
8000deb8:	e2833001 	add	r3, r3, #1
8000debc:	e3530080 	cmp	r3, #128	; 0x80
8000dec0:	0afffff0 	beq	8000de88 <shm_proc_map+0x54>
8000dec4:	e5b20004 	ldr	r0, [r2, #4]!
8000dec8:	e3500000 	cmp	r0, #0
8000decc:	1afffff9 	bne	8000deb8 <shm_proc_map+0x84>
8000ded0:	e5942008 	ldr	r2, [r4, #8]
8000ded4:	e5948004 	ldr	r8, [r4, #4]
8000ded8:	e2833008 	add	r3, r3, #8
8000dedc:	e3520000 	cmp	r2, #0
8000dee0:	e7817103 	str	r7, [r1, r3, lsl #2]
8000dee4:	01a00008 	moveq	r0, r8
8000dee8:	0a000011 	beq	8000df34 <shm_proc_map+0x100>
8000deec:	e59f3084 	ldr	r3, [pc, #132]	; 8000df78 <shm_proc_map+0x144>
8000def0:	e1a07000 	mov	r7, r0
8000def4:	e7966003 	ldr	r6, [r6, r3]
8000def8:	e1a01008 	mov	r1, r8
8000defc:	e5960000 	ldr	r0, [r6]
8000df00:	ebfffd6c 	bl	8000d4b8 <resolve_phy_address>
8000df04:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
8000df08:	e1a01008 	mov	r1, r8
8000df0c:	e2877001 	add	r7, r7, #1
8000df10:	e2888a01 	add	r8, r8, #4096	; 0x1000
8000df14:	e1a02000 	mov	r2, r0
8000df18:	e5930000 	ldr	r0, [r3]
8000df1c:	e3a030ff 	mov	r3, #255	; 0xff
8000df20:	ebfffcf3 	bl	8000d2f4 <map_page>
8000df24:	e5943008 	ldr	r3, [r4, #8]
8000df28:	e1530007 	cmp	r3, r7
8000df2c:	8afffff1 	bhi	8000def8 <shm_proc_map+0xc4>
8000df30:	e5940004 	ldr	r0, [r4, #4]
8000df34:	e5943018 	ldr	r3, [r4, #24]
8000df38:	e2833001 	add	r3, r3, #1
8000df3c:	e5843018 	str	r3, [r4, #24]
8000df40:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000df44:	e1a00005 	mov	r0, r5
8000df48:	e5902004 	ldr	r2, [r0, #4]
8000df4c:	e5943014 	ldr	r3, [r4, #20]
8000df50:	e1520003 	cmp	r2, r3
8000df54:	0affffd3 	beq	8000dea8 <shm_proc_map+0x74>
8000df58:	e5900008 	ldr	r0, [r0, #8]
8000df5c:	eb000606 	bl	8000f77c <proc_get>
8000df60:	e3500000 	cmp	r0, #0
8000df64:	1afffff7 	bne	8000df48 <shm_proc_map+0x114>
8000df68:	e3a00000 	mov	r0, #0
8000df6c:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000df70:	000165d4 	ldrdeq	r6, [r1], -r4
8000df74:	0000e1a4 	andeq	lr, r0, r4, lsr #3
8000df78:	00000014 	andeq	r0, r0, r4, lsl r0

8000df7c <shm_proc_unmap>:
8000df7c:	e1a0c00d 	mov	ip, sp
8000df80:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000df84:	e24cb004 	sub	fp, ip, #4
8000df88:	e1a05001 	mov	r5, r1
8000df8c:	eb0005fa 	bl	8000f77c <proc_get>
8000df90:	e59f3184 	ldr	r3, [pc, #388]	; 8000e11c <shm_proc_unmap+0x1a0>
8000df94:	e08f3003 	add	r3, pc, r3
8000df98:	e5934004 	ldr	r4, [r3, #4]
8000df9c:	e3540000 	cmp	r4, #0
8000dfa0:	e1a06000 	mov	r6, r0
8000dfa4:	0a000008 	beq	8000dfcc <shm_proc_unmap+0x50>
8000dfa8:	e594300c 	ldr	r3, [r4, #12]
8000dfac:	e3530000 	cmp	r3, #0
8000dfb0:	0a000002 	beq	8000dfc0 <shm_proc_unmap+0x44>
8000dfb4:	e5943000 	ldr	r3, [r4]
8000dfb8:	e1530005 	cmp	r3, r5
8000dfbc:	0a000004 	beq	8000dfd4 <shm_proc_unmap+0x58>
8000dfc0:	e594401c 	ldr	r4, [r4, #28]
8000dfc4:	e3540000 	cmp	r4, #0
8000dfc8:	1afffff6 	bne	8000dfa8 <shm_proc_unmap+0x2c>
8000dfcc:	e3e00000 	mvn	r0, #0
8000dfd0:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000dfd4:	e3560000 	cmp	r6, #0
8000dfd8:	0afffffb 	beq	8000dfcc <shm_proc_unmap+0x50>
8000dfdc:	e596102c 	ldr	r1, [r6, #44]	; 0x2c
8000dfe0:	e3a03000 	mov	r3, #0
8000dfe4:	e281201c 	add	r2, r1, #28
8000dfe8:	ea000002 	b	8000dff8 <shm_proc_unmap+0x7c>
8000dfec:	e2833001 	add	r3, r3, #1
8000dff0:	e3530080 	cmp	r3, #128	; 0x80
8000dff4:	0afffff4 	beq	8000dfcc <shm_proc_unmap+0x50>
8000dff8:	e5b20004 	ldr	r0, [r2, #4]!
8000dffc:	e1500005 	cmp	r0, r5
8000e000:	1afffff9 	bne	8000dfec <shm_proc_unmap+0x70>
8000e004:	e5942008 	ldr	r2, [r4, #8]
8000e008:	e3a05000 	mov	r5, #0
8000e00c:	e2833008 	add	r3, r3, #8
8000e010:	e1520005 	cmp	r2, r5
8000e014:	e7815103 	str	r5, [r1, r3, lsl #2]
8000e018:	e5947004 	ldr	r7, [r4, #4]
8000e01c:	1a000001 	bne	8000e028 <shm_proc_unmap+0xac>
8000e020:	ea000008 	b	8000e048 <shm_proc_unmap+0xcc>
8000e024:	e596102c 	ldr	r1, [r6, #44]	; 0x2c
8000e028:	e5910000 	ldr	r0, [r1]
8000e02c:	e1a01007 	mov	r1, r7
8000e030:	ebfffd05 	bl	8000d44c <unmap_page>
8000e034:	e5942008 	ldr	r2, [r4, #8]
8000e038:	e2855001 	add	r5, r5, #1
8000e03c:	e1520005 	cmp	r2, r5
8000e040:	e2877a01 	add	r7, r7, #4096	; 0x1000
8000e044:	8afffff6 	bhi	8000e024 <shm_proc_unmap+0xa8>
8000e048:	e5943018 	ldr	r3, [r4, #24]
8000e04c:	e2433001 	sub	r3, r3, #1
8000e050:	e3530000 	cmp	r3, #0
8000e054:	e5843018 	str	r3, [r4, #24]
8000e058:	da000002 	ble	8000e068 <shm_proc_unmap+0xec>
8000e05c:	eb000eb0 	bl	80011b24 <_flush_tlb>
8000e060:	e3a00000 	mov	r0, #0
8000e064:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000e068:	e594001c 	ldr	r0, [r4, #28]
8000e06c:	e3a03000 	mov	r3, #0
8000e070:	e1500003 	cmp	r0, r3
8000e074:	e584300c 	str	r3, [r4, #12]
8000e078:	0a00000c 	beq	8000e0b0 <shm_proc_unmap+0x134>
8000e07c:	e590300c 	ldr	r3, [r0, #12]
8000e080:	e3530000 	cmp	r3, #0
8000e084:	1a000009 	bne	8000e0b0 <shm_proc_unmap+0x134>
8000e088:	e590301c 	ldr	r3, [r0, #28]
8000e08c:	e584301c 	str	r3, [r4, #28]
8000e090:	e590301c 	ldr	r3, [r0, #28]
8000e094:	e3530000 	cmp	r3, #0
8000e098:	15834020 	strne	r4, [r3, #32]
8000e09c:	0a000016 	beq	8000e0fc <shm_proc_unmap+0x180>
8000e0a0:	e5903008 	ldr	r3, [r0, #8]
8000e0a4:	e0822003 	add	r2, r2, r3
8000e0a8:	e5842008 	str	r2, [r4, #8]
8000e0ac:	ebfffe40 	bl	8000d9b4 <kfree>
8000e0b0:	e5943020 	ldr	r3, [r4, #32]
8000e0b4:	e3530000 	cmp	r3, #0
8000e0b8:	0affffe7 	beq	8000e05c <shm_proc_unmap+0xe0>
8000e0bc:	e593200c 	ldr	r2, [r3, #12]
8000e0c0:	e3520000 	cmp	r2, #0
8000e0c4:	1affffe4 	bne	8000e05c <shm_proc_unmap+0xe0>
8000e0c8:	e594201c 	ldr	r2, [r4, #28]
8000e0cc:	e583201c 	str	r2, [r3, #28]
8000e0d0:	e594201c 	ldr	r2, [r4, #28]
8000e0d4:	e3520000 	cmp	r2, #0
8000e0d8:	15823020 	strne	r3, [r2, #32]
8000e0dc:	0a00000a 	beq	8000e10c <shm_proc_unmap+0x190>
8000e0e0:	e5932008 	ldr	r2, [r3, #8]
8000e0e4:	e5941008 	ldr	r1, [r4, #8]
8000e0e8:	e1a00004 	mov	r0, r4
8000e0ec:	e0822001 	add	r2, r2, r1
8000e0f0:	e5832008 	str	r2, [r3, #8]
8000e0f4:	ebfffe2e 	bl	8000d9b4 <kfree>
8000e0f8:	eaffffd7 	b	8000e05c <shm_proc_unmap+0xe0>
8000e0fc:	e59f301c 	ldr	r3, [pc, #28]	; 8000e120 <shm_proc_unmap+0x1a4>
8000e100:	e08f3003 	add	r3, pc, r3
8000e104:	e5834008 	str	r4, [r3, #8]
8000e108:	eaffffe4 	b	8000e0a0 <shm_proc_unmap+0x124>
8000e10c:	e59f2010 	ldr	r2, [pc, #16]	; 8000e124 <shm_proc_unmap+0x1a8>
8000e110:	e08f2002 	add	r2, pc, r2
8000e114:	e5823008 	str	r3, [r2, #8]
8000e118:	eafffff0 	b	8000e0e0 <shm_proc_unmap+0x164>
8000e11c:	00016484 	andeq	r6, r1, r4, lsl #9
8000e120:	00016318 	andeq	r6, r1, r8, lsl r3
8000e124:	00016308 	andeq	r6, r1, r8, lsl #6

8000e128 <_copy_interrupt_table>:
8000e128:	e1a0c00d 	mov	ip, sp
8000e12c:	e92dd800 	push	{fp, ip, lr, pc}
8000e130:	e24cb004 	sub	fp, ip, #4
8000e134:	e24dd008 	sub	sp, sp, #8
8000e138:	e59f3054 	ldr	r3, [pc, #84]	; 8000e194 <_copy_interrupt_table+0x6c>
8000e13c:	e08f3003 	add	r3, pc, r3
8000e140:	e59f2050 	ldr	r2, [pc, #80]	; 8000e198 <_copy_interrupt_table+0x70>
8000e144:	e7932002 	ldr	r2, [r3, r2]
8000e148:	e50b2010 	str	r2, [fp, #-16]
8000e14c:	e3a02000 	mov	r2, #0
8000e150:	e50b2014 	str	r2, [fp, #-20]
8000e154:	ea000007 	b	8000e178 <_copy_interrupt_table+0x50>
8000e158:	e51b2014 	ldr	r2, [fp, #-20]
8000e15c:	e2821004 	add	r1, r2, #4
8000e160:	e50b1014 	str	r1, [fp, #-20]
8000e164:	e51b1010 	ldr	r1, [fp, #-16]
8000e168:	e2810004 	add	r0, r1, #4
8000e16c:	e50b0010 	str	r0, [fp, #-16]
8000e170:	e5911000 	ldr	r1, [r1]
8000e174:	e5821000 	str	r1, [r2]
8000e178:	e51b2010 	ldr	r2, [fp, #-16]
8000e17c:	e59f1018 	ldr	r1, [pc, #24]	; 8000e19c <_copy_interrupt_table+0x74>
8000e180:	e7931001 	ldr	r1, [r3, r1]
8000e184:	e1520001 	cmp	r2, r1
8000e188:	3afffff2 	bcc	8000e158 <_copy_interrupt_table+0x30>
8000e18c:	e24bd00c 	sub	sp, fp, #12
8000e190:	e89da800 	ldm	sp, {fp, sp, pc}
8000e194:	0000dec0 	andeq	sp, r0, r0, asr #29
8000e198:	00000034 	andeq	r0, r0, r4, lsr r0
8000e19c:	00000008 	andeq	r0, r0, r8

8000e1a0 <_boot_init>:
8000e1a0:	e1a0c00d 	mov	ip, sp
8000e1a4:	e92dd800 	push	{fp, ip, lr, pc}
8000e1a8:	e24cb004 	sub	fp, ip, #4
8000e1ac:	ebffe82c 	bl	80008264 <hw_info_init>
8000e1b0:	ebffe83c 	bl	800082a8 <get_hw_info>
8000e1b4:	e59f301c 	ldr	r3, [pc, #28]	; 8000e1d8 <_boot_init+0x38>
8000e1b8:	e59f201c 	ldr	r2, [pc, #28]	; 8000e1dc <_boot_init+0x3c>
8000e1bc:	e08f3003 	add	r3, pc, r3
8000e1c0:	e7933002 	ldr	r3, [r3, r2]
8000e1c4:	e5902024 	ldr	r2, [r0, #36]	; 0x24
8000e1c8:	e5832000 	str	r2, [r3]
8000e1cc:	e24bd00c 	sub	sp, fp, #12
8000e1d0:	e89d6800 	ldm	sp, {fp, sp, lr}
8000e1d4:	eafffb44 	b	8000ceec <flush_actled>
8000e1d8:	0000de40 	andeq	sp, r0, r0, asr #28
8000e1dc:	00000010 	andeq	r0, r0, r0, lsl r0

8000e1e0 <uspace_interrupt_init>:
8000e1e0:	e59f201c 	ldr	r2, [pc, #28]	; 8000e204 <uspace_interrupt_init+0x24>
8000e1e4:	e3e01000 	mvn	r1, #0
8000e1e8:	e08f2002 	add	r2, pc, r2
8000e1ec:	e2423004 	sub	r3, r2, #4
8000e1f0:	e282207c 	add	r2, r2, #124	; 0x7c
8000e1f4:	e5a31004 	str	r1, [r3, #4]!
8000e1f8:	e1530002 	cmp	r3, r2
8000e1fc:	1afffffc 	bne	8000e1f4 <uspace_interrupt_init+0x14>
8000e200:	e12fff1e 	bx	lr
8000e204:	0001623c 	andeq	r6, r1, ip, lsr r2

8000e208 <proc_interrupt>:
8000e208:	e1a0c00d 	mov	ip, sp
8000e20c:	e3510000 	cmp	r1, #0
8000e210:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
8000e214:	e24cb004 	sub	fp, ip, #4
8000e218:	e1a06000 	mov	r6, r0
8000e21c:	e1a05002 	mov	r5, r2
8000e220:	b89da9f8 	ldmlt	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e224:	e1a00001 	mov	r0, r1
8000e228:	eb000553 	bl	8000f77c <proc_get>
8000e22c:	e2504000 	subs	r4, r0, #0
8000e230:	089da9f8 	ldmeq	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e234:	e59430b0 	ldr	r3, [r4, #176]	; 0xb0
8000e238:	e3530000 	cmp	r3, #0
8000e23c:	089da9f8 	ldmeq	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e240:	e5d430bc 	ldrb	r3, [r4, #188]	; 0xbc
8000e244:	e3530000 	cmp	r3, #0
8000e248:	189da9f8 	ldmne	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e24c:	e3a00002 	mov	r0, #2
8000e250:	e1a01004 	mov	r1, r4
8000e254:	eb0008a9 	bl	80010500 <kfork_raw>
8000e258:	e2507000 	subs	r7, r0, #0
8000e25c:	089da9f8 	ldmeq	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e260:	e28410d0 	add	r1, r4, #208	; 0xd0
8000e264:	e28700d0 	add	r0, r7, #208	; 0xd0
8000e268:	e3a02044 	mov	r2, #68	; 0x44
8000e26c:	e597810c 	ldr	r8, [r7, #268]	; 0x10c
8000e270:	ebfff262 	bl	8000ac00 <memcpy>
8000e274:	e59410b8 	ldr	r1, [r4, #184]	; 0xb8
8000e278:	e59430b0 	ldr	r3, [r4, #176]	; 0xb0
8000e27c:	e594c0b4 	ldr	ip, [r4, #180]	; 0xb4
8000e280:	e3a02001 	mov	r2, #1
8000e284:	e58710e0 	str	r1, [r7, #224]	; 0xe0
8000e288:	e1a00006 	mov	r0, r6
8000e28c:	e1a01007 	mov	r1, r7
8000e290:	e587810c 	str	r8, [r7, #268]	; 0x10c
8000e294:	e5873110 	str	r3, [r7, #272]	; 0x110
8000e298:	e58730d4 	str	r3, [r7, #212]	; 0xd4
8000e29c:	e58750d8 	str	r5, [r7, #216]	; 0xd8
8000e2a0:	e587c0dc 	str	ip, [r7, #220]	; 0xdc
8000e2a4:	e5c420bc 	strb	r2, [r4, #188]	; 0xbc
8000e2a8:	e24bd024 	sub	sp, fp, #36	; 0x24
8000e2ac:	e89d69f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, lr}
8000e2b0:	ea00053b 	b	8000f7a4 <proc_switch>

8000e2b4 <uspace_interrupt>:
8000e2b4:	e351001f 	cmp	r1, #31
8000e2b8:	812fff1e 	bxhi	lr
8000e2bc:	e59f300c 	ldr	r3, [pc, #12]	; 8000e2d0 <uspace_interrupt+0x1c>
8000e2c0:	e1a02001 	mov	r2, r1
8000e2c4:	e08f3003 	add	r3, pc, r3
8000e2c8:	e7931101 	ldr	r1, [r3, r1, lsl #2]
8000e2cc:	eaffffcd 	b	8000e208 <proc_interrupt>
8000e2d0:	00016160 	andeq	r6, r1, r0, ror #2

8000e2d4 <uspace_interrupt_register>:
8000e2d4:	e351001f 	cmp	r1, #31
8000e2d8:	959f3008 	ldrls	r3, [pc, #8]	; 8000e2e8 <uspace_interrupt_register+0x14>
8000e2dc:	908f3003 	addls	r3, pc, r3
8000e2e0:	97830101 	strls	r0, [r3, r1, lsl #2]
8000e2e4:	e12fff1e 	bx	lr
8000e2e8:	00016148 	andeq	r6, r1, r8, asr #2

8000e2ec <irq_handler>:
8000e2ec:	e1a0c00d 	mov	ip, sp
8000e2f0:	e92ddb78 	push	{r3, r4, r5, r6, r8, r9, fp, ip, lr, pc}
8000e2f4:	e59f40e0 	ldr	r4, [pc, #224]	; 8000e3dc <irq_handler+0xf0>
8000e2f8:	e24cb004 	sub	fp, ip, #4
8000e2fc:	e1a06000 	mov	r6, r0
8000e300:	ebffe7ba 	bl	800081f0 <__irq_disable>
8000e304:	ebffe836 	bl	800083e4 <gic_get_irqs>
8000e308:	e08f4004 	add	r4, pc, r4
8000e30c:	e3100001 	tst	r0, #1
8000e310:	089dab78 	ldmeq	sp, {r3, r4, r5, r6, r8, r9, fp, sp, pc}
8000e314:	ebffea41 	bl	80008c20 <timer_read_sys_usec>
8000e318:	e59f30c0 	ldr	r3, [pc, #192]	; 8000e3e0 <irq_handler+0xf4>
8000e31c:	e7945003 	ldr	r5, [r4, r3]
8000e320:	e5953000 	ldr	r3, [r5]
8000e324:	e3530000 	cmp	r3, #0
8000e328:	0a00000d 	beq	8000e364 <irq_handler+0x78>
8000e32c:	e5933018 	ldr	r3, [r3, #24]
8000e330:	e3530000 	cmp	r3, #0
8000e334:	0a00000a 	beq	8000e364 <irq_handler+0x78>
8000e338:	e3a00000 	mov	r0, #0
8000e33c:	ebffea30 	bl	80008c04 <timer_clear_interrupt>
8000e340:	e5952000 	ldr	r2, [r5]
8000e344:	e3520000 	cmp	r2, #0
8000e348:	0a00001f 	beq	8000e3cc <irq_handler+0xe0>
8000e34c:	e5923018 	ldr	r3, [r2, #24]
8000e350:	e3530000 	cmp	r3, #0
8000e354:	da00001c 	ble	8000e3cc <irq_handler+0xe0>
8000e358:	e2433001 	sub	r3, r3, #1
8000e35c:	e5823018 	str	r3, [r2, #24]
8000e360:	e89dab78 	ldm	sp, {r3, r4, r5, r6, r8, r9, fp, sp, pc}
8000e364:	e59f3078 	ldr	r3, [pc, #120]	; 8000e3e4 <irq_handler+0xf8>
8000e368:	e08f3003 	add	r3, pc, r3
8000e36c:	e1c380d0 	ldrd	r8, [r3]
8000e370:	e1982009 	orrs	r2, r8, r9
8000e374:	0a000012 	beq	8000e3c4 <irq_handler+0xd8>
8000e378:	e5932008 	ldr	r2, [r3, #8]
8000e37c:	e59fc064 	ldr	ip, [pc, #100]	; 8000e3e8 <irq_handler+0xfc>
8000e380:	e0508008 	subs	r8, r0, r8
8000e384:	e0822008 	add	r2, r2, r8
8000e388:	e0c19009 	sbc	r9, r1, r9
8000e38c:	e152000c 	cmp	r2, ip
8000e390:	e1c300f0 	strd	r0, [r3]
8000e394:	e5832008 	str	r2, [r3, #8]
8000e398:	9a000006 	bls	8000e3b8 <irq_handler+0xcc>
8000e39c:	e59f2048 	ldr	r2, [pc, #72]	; 8000e3ec <irq_handler+0x100>
8000e3a0:	e3a01000 	mov	r1, #0
8000e3a4:	e7942002 	ldr	r2, [r4, r2]
8000e3a8:	e5831008 	str	r1, [r3, #8]
8000e3ac:	e5923000 	ldr	r3, [r2]
8000e3b0:	e2833001 	add	r3, r3, #1
8000e3b4:	e5823000 	str	r3, [r2]
8000e3b8:	e1a00008 	mov	r0, r8
8000e3bc:	eb000953 	bl	80010910 <renew_sleep_counter>
8000e3c0:	eaffffdc 	b	8000e338 <irq_handler+0x4c>
8000e3c4:	e1c300f0 	strd	r0, [r3]
8000e3c8:	eaffffda 	b	8000e338 <irq_handler+0x4c>
8000e3cc:	e1a00006 	mov	r0, r6
8000e3d0:	e24bd024 	sub	sp, fp, #36	; 0x24
8000e3d4:	e89d6b78 	ldm	sp, {r3, r4, r5, r6, r8, r9, fp, sp, lr}
8000e3d8:	ea000a00 	b	80010be0 <schedule>
8000e3dc:	0000dcf4 	strdeq	sp, [r0], -r4
8000e3e0:	00000000 	andeq	r0, r0, r0
8000e3e4:	00016140 	andeq	r6, r1, r0, asr #2
8000e3e8:	000f423f 	andeq	r4, pc, pc, lsr r2	; <UNPREDICTABLE>
8000e3ec:	00000030 	andeq	r0, r0, r0, lsr r0

8000e3f0 <prefetch_abort_handler>:
8000e3f0:	e59f3050 	ldr	r3, [pc, #80]	; 8000e448 <prefetch_abort_handler+0x58>
8000e3f4:	e59f2050 	ldr	r2, [pc, #80]	; 8000e44c <prefetch_abort_handler+0x5c>
8000e3f8:	e1a0c00d 	mov	ip, sp
8000e3fc:	e08f3003 	add	r3, pc, r3
8000e400:	e92dd800 	push	{fp, ip, lr, pc}
8000e404:	e7933002 	ldr	r3, [r3, r2]
8000e408:	e24cb004 	sub	fp, ip, #4
8000e40c:	e5933000 	ldr	r3, [r3]
8000e410:	e3530000 	cmp	r3, #0
8000e414:	0a000006 	beq	8000e434 <prefetch_abort_handler+0x44>
8000e418:	e59320c8 	ldr	r2, [r3, #200]	; 0xc8
8000e41c:	e59f002c 	ldr	r0, [pc, #44]	; 8000e450 <prefetch_abort_handler+0x60>
8000e420:	e5931004 	ldr	r1, [r3, #4]
8000e424:	e5922000 	ldr	r2, [r2]
8000e428:	e08f0000 	add	r0, pc, r0
8000e42c:	ebfffabd 	bl	8000cf28 <printf>
8000e430:	eafffffe 	b	8000e430 <prefetch_abort_handler+0x40>
8000e434:	e59f0018 	ldr	r0, [pc, #24]	; 8000e454 <prefetch_abort_handler+0x64>
8000e438:	e08f0000 	add	r0, pc, r0
8000e43c:	e24bd00c 	sub	sp, fp, #12
8000e440:	e89d6800 	ldm	sp, {fp, sp, lr}
8000e444:	eafffab7 	b	8000cf28 <printf>
8000e448:	0000dc00 	andeq	sp, r0, r0, lsl #24
8000e44c:	00000000 	andeq	r0, r0, r0
8000e450:	00003c5c 	andeq	r3, r0, ip, asr ip
8000e454:	00003c30 	andeq	r3, r0, r0, lsr ip

8000e458 <data_abort_handler>:
8000e458:	e59f2078 	ldr	r2, [pc, #120]	; 8000e4d8 <data_abort_handler+0x80>
8000e45c:	e59f3078 	ldr	r3, [pc, #120]	; 8000e4dc <data_abort_handler+0x84>
8000e460:	e1a0c00d 	mov	ip, sp
8000e464:	e08f2002 	add	r2, pc, r2
8000e468:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000e46c:	e7924003 	ldr	r4, [r2, r3]
8000e470:	e24cb004 	sub	fp, ip, #4
8000e474:	e5943000 	ldr	r3, [r4]
8000e478:	e1a05000 	mov	r5, r0
8000e47c:	e3530000 	cmp	r3, #0
8000e480:	0a00000f 	beq	8000e4c4 <data_abort_handler+0x6c>
8000e484:	e59320c8 	ldr	r2, [r3, #200]	; 0xc8
8000e488:	e59f0050 	ldr	r0, [pc, #80]	; 8000e4e0 <data_abort_handler+0x88>
8000e48c:	e5931004 	ldr	r1, [r3, #4]
8000e490:	e5922000 	ldr	r2, [r2]
8000e494:	e08f0000 	add	r0, pc, r0
8000e498:	ebfffaa2 	bl	8000cf28 <printf>
8000e49c:	e1a00005 	mov	r0, r5
8000e4a0:	e5941000 	ldr	r1, [r4]
8000e4a4:	e3e02000 	mvn	r2, #0
8000e4a8:	eb000596 	bl	8000fb08 <proc_exit>
8000e4ac:	e3a03000 	mov	r3, #0
8000e4b0:	e1a00005 	mov	r0, r5
8000e4b4:	e5843000 	str	r3, [r4]
8000e4b8:	e24bd014 	sub	sp, fp, #20
8000e4bc:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
8000e4c0:	ea0009c6 	b	80010be0 <schedule>
8000e4c4:	e59f0018 	ldr	r0, [pc, #24]	; 8000e4e4 <data_abort_handler+0x8c>
8000e4c8:	e08f0000 	add	r0, pc, r0
8000e4cc:	e24bd014 	sub	sp, fp, #20
8000e4d0:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
8000e4d4:	eafffa93 	b	8000cf28 <printf>
8000e4d8:	0000db98 	muleq	r0, r8, fp
8000e4dc:	00000000 	andeq	r0, r0, r0
8000e4e0:	00003c28 	andeq	r3, r0, r8, lsr #24
8000e4e4:	00003bdc 	ldrdeq	r3, [r0], -ip

8000e4e8 <irq_init>:
8000e4e8:	e1a0c00d 	mov	ip, sp
8000e4ec:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000e4f0:	e59f4048 	ldr	r4, [pc, #72]	; 8000e540 <irq_init+0x58>
8000e4f4:	e24cb004 	sub	fp, ip, #4
8000e4f8:	ebffe799 	bl	80008364 <irq_arch_init>
8000e4fc:	ebffff37 	bl	8000e1e0 <uspace_interrupt_init>
8000e500:	e3a00001 	mov	r0, #1
8000e504:	ebffe797 	bl	80008368 <gic_set_irqs>
8000e508:	ebffe734 	bl	800081e0 <__irq_enable>
8000e50c:	e59f1030 	ldr	r1, [pc, #48]	; 8000e544 <irq_init+0x5c>
8000e510:	e08f4004 	add	r4, pc, r4
8000e514:	e59f302c 	ldr	r3, [pc, #44]	; 8000e548 <irq_init+0x60>
8000e518:	e794c001 	ldr	ip, [r4, r1]
8000e51c:	e1a02004 	mov	r2, r4
8000e520:	e08f3003 	add	r3, pc, r3
8000e524:	e3a00000 	mov	r0, #0
8000e528:	e3a02000 	mov	r2, #0
8000e52c:	e3a01000 	mov	r1, #0
8000e530:	e58c2000 	str	r2, [ip]
8000e534:	e5832008 	str	r2, [r3, #8]
8000e538:	e1c300f0 	strd	r0, [r3]
8000e53c:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000e540:	0000daec 	andeq	sp, r0, ip, ror #21
8000e544:	00000030 	andeq	r0, r0, r0, lsr r0
8000e548:	00015f88 	andeq	r5, r1, r8, lsl #31

8000e54c <init_global>:
8000e54c:	e1a0c00d 	mov	ip, sp
8000e550:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000e554:	e59f4028 	ldr	r4, [pc, #40]	; 8000e584 <init_global+0x38>
8000e558:	e24cb004 	sub	fp, ip, #4
8000e55c:	e08f4004 	add	r4, pc, r4
8000e560:	e2845c02 	add	r5, r4, #512	; 0x200
8000e564:	e1a00004 	mov	r0, r4
8000e568:	e3a01000 	mov	r1, #0
8000e56c:	e2844008 	add	r4, r4, #8
8000e570:	e3a02008 	mov	r2, #8
8000e574:	ebfff1bd 	bl	8000ac70 <memset>
8000e578:	e1540005 	cmp	r4, r5
8000e57c:	1afffff8 	bne	8000e564 <init_global+0x18>
8000e580:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8000e584:	00015f5c 	andeq	r5, r1, ip, asr pc

8000e588 <set_global>:
8000e588:	e1a0c00d 	mov	ip, sp
8000e58c:	e92dd9f8 	push	{r3, r4, r5, r6, r7, r8, fp, ip, lr, pc}
8000e590:	e59f50b8 	ldr	r5, [pc, #184]	; 8000e650 <set_global+0xc8>
8000e594:	e24cb004 	sub	fp, ip, #4
8000e598:	e08f5005 	add	r5, pc, r5
8000e59c:	e1a06000 	mov	r6, r0
8000e5a0:	e1a08001 	mov	r8, r1
8000e5a4:	e3a04000 	mov	r4, #0
8000e5a8:	ea000006 	b	8000e5c8 <set_global+0x40>
8000e5ac:	e5930000 	ldr	r0, [r3]
8000e5b0:	ebfff20f 	bl	8000adf4 <strcmp>
8000e5b4:	e3500000 	cmp	r0, #0
8000e5b8:	0a00000d 	beq	8000e5f4 <set_global+0x6c>
8000e5bc:	e2844001 	add	r4, r4, #1
8000e5c0:	e3540040 	cmp	r4, #64	; 0x40
8000e5c4:	0a000019 	beq	8000e630 <set_global+0xa8>
8000e5c8:	e7953184 	ldr	r3, [r5, r4, lsl #3]
8000e5cc:	e1a01006 	mov	r1, r6
8000e5d0:	e3530000 	cmp	r3, #0
8000e5d4:	e1a07184 	lsl	r7, r4, #3
8000e5d8:	1afffff3 	bne	8000e5ac <set_global+0x24>
8000e5dc:	e1a00006 	mov	r0, r6
8000e5e0:	ebfff587 	bl	8000bc04 <str_new>
8000e5e4:	e59f3068 	ldr	r3, [pc, #104]	; 8000e654 <set_global+0xcc>
8000e5e8:	e08f3003 	add	r3, pc, r3
8000e5ec:	e7830184 	str	r0, [r3, r4, lsl #3]
8000e5f0:	ea000002 	b	8000e600 <set_global+0x78>
8000e5f4:	e7953184 	ldr	r3, [r5, r4, lsl #3]
8000e5f8:	e3530000 	cmp	r3, #0
8000e5fc:	0afffff6 	beq	8000e5dc <set_global+0x54>
8000e600:	e59f3050 	ldr	r3, [pc, #80]	; 8000e658 <set_global+0xd0>
8000e604:	e08f3003 	add	r3, pc, r3
8000e608:	e0837007 	add	r7, r3, r7
8000e60c:	e5974004 	ldr	r4, [r7, #4]
8000e610:	e3540000 	cmp	r4, #0
8000e614:	0a000008 	beq	8000e63c <set_global+0xb4>
8000e618:	e1a00004 	mov	r0, r4
8000e61c:	e1a01008 	mov	r1, r8
8000e620:	e3a04000 	mov	r4, #0
8000e624:	ebfff56d 	bl	8000bbe0 <str_cpy>
8000e628:	e1a00004 	mov	r0, r4
8000e62c:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e630:	e3e04000 	mvn	r4, #0
8000e634:	e1a00004 	mov	r0, r4
8000e638:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e63c:	e1a00008 	mov	r0, r8
8000e640:	ebfff56f 	bl	8000bc04 <str_new>
8000e644:	e5870004 	str	r0, [r7, #4]
8000e648:	e1a00004 	mov	r0, r4
8000e64c:	e89da9f8 	ldm	sp, {r3, r4, r5, r6, r7, r8, fp, sp, pc}
8000e650:	00015f20 	andeq	r5, r1, r0, lsr #30
8000e654:	00015ed0 	ldrdeq	r5, [r1], -r0
8000e658:	00015eb4 			; <UNDEFINED> instruction: 0x00015eb4

8000e65c <get_global>:
8000e65c:	e1a0c00d 	mov	ip, sp
8000e660:	e92ddff8 	push	{r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000e664:	e59f5074 	ldr	r5, [pc, #116]	; 8000e6e0 <get_global+0x84>
8000e668:	e24cb004 	sub	fp, ip, #4
8000e66c:	e08f5005 	add	r5, pc, r5
8000e670:	e1a06000 	mov	r6, r0
8000e674:	e1a08001 	mov	r8, r1
8000e678:	e1a07002 	mov	r7, r2
8000e67c:	e3a04000 	mov	r4, #0
8000e680:	ea000005 	b	8000e69c <get_global+0x40>
8000e684:	e5930000 	ldr	r0, [r3]
8000e688:	ebfff1d9 	bl	8000adf4 <strcmp>
8000e68c:	e250a000 	subs	sl, r0, #0
8000e690:	0a00000a 	beq	8000e6c0 <get_global+0x64>
8000e694:	e3540040 	cmp	r4, #64	; 0x40
8000e698:	0a000005 	beq	8000e6b4 <get_global+0x58>
8000e69c:	e7953184 	ldr	r3, [r5, r4, lsl #3]
8000e6a0:	e1a09184 	lsl	r9, r4, #3
8000e6a4:	e3530000 	cmp	r3, #0
8000e6a8:	e1a01006 	mov	r1, r6
8000e6ac:	e2844001 	add	r4, r4, #1
8000e6b0:	1afffff3 	bne	8000e684 <get_global+0x28>
8000e6b4:	e3e0a000 	mvn	sl, #0
8000e6b8:	e1a0000a 	mov	r0, sl
8000e6bc:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000e6c0:	e0855009 	add	r5, r5, r9
8000e6c4:	e5953004 	ldr	r3, [r5, #4]
8000e6c8:	e1a00008 	mov	r0, r8
8000e6cc:	e5931000 	ldr	r1, [r3]
8000e6d0:	e1a02007 	mov	r2, r7
8000e6d4:	ebfff1a2 	bl	8000ad64 <strncpy>
8000e6d8:	e1a0000a 	mov	r0, sl
8000e6dc:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000e6e0:	00015e4c 	andeq	r5, r1, ip, asr #28

8000e6e4 <svc_handler>:
8000e6e4:	e1a0c00d 	mov	ip, sp
8000e6e8:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
8000e6ec:	e59f4d70 	ldr	r4, [pc, #3440]	; 8000f464 <svc_handler+0xd80>
8000e6f0:	e24cb004 	sub	fp, ip, #4
8000e6f4:	e24dd008 	sub	sp, sp, #8
8000e6f8:	e1a06000 	mov	r6, r0
8000e6fc:	e1a09003 	mov	r9, r3
8000e700:	e1a05001 	mov	r5, r1
8000e704:	e1a08002 	mov	r8, r2
8000e708:	e59b7004 	ldr	r7, [fp, #4]
8000e70c:	ebffe6b7 	bl	800081f0 <__irq_disable>
8000e710:	e2463001 	sub	r3, r6, #1
8000e714:	e08f4004 	add	r4, pc, r4
8000e718:	e3530045 	cmp	r3, #69	; 0x45
8000e71c:	908ff103 	addls	pc, pc, r3, lsl #2
8000e720:	ea00031b 	b	8000f394 <svc_handler+0xcb0>
8000e724:	ea0001fa 	b	8000ef14 <svc_handler+0x830>
8000e728:	ea0001ef 	b	8000eeec <svc_handler+0x808>
8000e72c:	ea0001e1 	b	8000eeb8 <svc_handler+0x7d4>
8000e730:	ea0001d9 	b	8000ee9c <svc_handler+0x7b8>
8000e734:	ea0001d0 	b	8000ee7c <svc_handler+0x798>
8000e738:	ea0001c6 	b	8000ee58 <svc_handler+0x774>
8000e73c:	ea0001b9 	b	8000ee28 <svc_handler+0x744>
8000e740:	ea0001b5 	b	8000ee1c <svc_handler+0x738>
8000e744:	ea0001ae 	b	8000ee04 <svc_handler+0x720>
8000e748:	ea0001a6 	b	8000ede8 <svc_handler+0x704>
8000e74c:	ea000199 	b	8000edb8 <svc_handler+0x6d4>
8000e750:	ea000194 	b	8000eda8 <svc_handler+0x6c4>
8000e754:	ea00018e 	b	8000ed94 <svc_handler+0x6b0>
8000e758:	ea000188 	b	8000ed80 <svc_handler+0x69c>
8000e75c:	ea00017d 	b	8000ed58 <svc_handler+0x674>
8000e760:	ea00016a 	b	8000ed10 <svc_handler+0x62c>
8000e764:	ea000163 	b	8000ecf8 <svc_handler+0x614>
8000e768:	ea00014c 	b	8000eca0 <svc_handler+0x5bc>
8000e76c:	ea00013b 	b	8000ec60 <svc_handler+0x57c>
8000e770:	ea00012b 	b	8000ec24 <svc_handler+0x540>
8000e774:	ea00011a 	b	8000ebe4 <svc_handler+0x500>
8000e778:	ea000103 	b	8000eb8c <svc_handler+0x4a8>
8000e77c:	ea0000fa 	b	8000eb6c <svc_handler+0x488>
8000e780:	ea0000e9 	b	8000eb2c <svc_handler+0x448>
8000e784:	ea0000e2 	b	8000eb14 <svc_handler+0x430>
8000e788:	ea0000d3 	b	8000eadc <svc_handler+0x3f8>
8000e78c:	ea0000c9 	b	8000eab8 <svc_handler+0x3d4>
8000e790:	ea0000c0 	b	8000ea98 <svc_handler+0x3b4>
8000e794:	ea0000b2 	b	8000ea64 <svc_handler+0x380>
8000e798:	ea0000a9 	b	8000ea44 <svc_handler+0x360>
8000e79c:	ea0000a1 	b	8000ea28 <svc_handler+0x344>
8000e7a0:	ea000093 	b	8000e9f4 <svc_handler+0x310>
8000e7a4:	ea0002ee 	b	8000f364 <svc_handler+0xc80>
8000e7a8:	ea0002e5 	b	8000f344 <svc_handler+0xc60>
8000e7ac:	ea0002dc 	b	8000f324 <svc_handler+0xc40>
8000e7b0:	ea0002c8 	b	8000f2d8 <svc_handler+0xbf4>
8000e7b4:	ea0002bf 	b	8000f2b8 <svc_handler+0xbd4>
8000e7b8:	ea0002b3 	b	8000f28c <svc_handler+0xba8>
8000e7bc:	ea0002a9 	b	8000f268 <svc_handler+0xb84>
8000e7c0:	ea0002a3 	b	8000f254 <svc_handler+0xb70>
8000e7c4:	ea000237 	b	8000f0a8 <svc_handler+0x9c4>
8000e7c8:	ea000227 	b	8000f06c <svc_handler+0x988>
8000e7cc:	ea0002ec 	b	8000f384 <svc_handler+0xca0>
8000e7d0:	ea000220 	b	8000f058 <svc_handler+0x974>
8000e7d4:	ea0001fb 	b	8000efc8 <svc_handler+0x8e4>
8000e7d8:	ea0001ed 	b	8000ef94 <svc_handler+0x8b0>
8000e7dc:	ea0001d9 	b	8000ef48 <svc_handler+0x864>
8000e7e0:	ea0001cf 	b	8000ef24 <svc_handler+0x840>
8000e7e4:	ea000266 	b	8000f184 <svc_handler+0xaa0>
8000e7e8:	ea00025e 	b	8000f168 <svc_handler+0xa84>
8000e7ec:	ea000258 	b	8000f154 <svc_handler+0xa70>
8000e7f0:	ea000251 	b	8000f13c <svc_handler+0xa58>
8000e7f4:	ea000245 	b	8000f110 <svc_handler+0xa2c>
8000e7f8:	ea00023c 	b	8000f0f0 <svc_handler+0xa0c>
8000e7fc:	ea000236 	b	8000f0dc <svc_handler+0x9f8>
8000e800:	ea00022d 	b	8000f0bc <svc_handler+0x9d8>
8000e804:	ea00028a 	b	8000f234 <svc_handler+0xb50>
8000e808:	ea000281 	b	8000f214 <svc_handler+0xb30>
8000e80c:	ea000269 	b	8000f1b8 <svc_handler+0xad4>
8000e810:	ea000264 	b	8000f1a8 <svc_handler+0xac4>
8000e814:	ea000279 	b	8000f200 <svc_handler+0xb1c>
8000e818:	ea00006f 	b	8000e9dc <svc_handler+0x2f8>
8000e81c:	ea00005e 	b	8000e99c <svc_handler+0x2b8>
8000e820:	ea000047 	b	8000e944 <svc_handler+0x260>
8000e824:	ea00002b 	b	8000e8d8 <svc_handler+0x1f4>
8000e828:	ea000022 	b	8000e8b8 <svc_handler+0x1d4>
8000e82c:	ea00001b 	b	8000e8a0 <svc_handler+0x1bc>
8000e830:	ea000011 	b	8000e87c <svc_handler+0x198>
8000e834:	ea000008 	b	8000e85c <svc_handler+0x178>
8000e838:	eaffffff 	b	8000e83c <svc_handler+0x158>
8000e83c:	e59f3c24 	ldr	r3, [pc, #3108]	; 8000f468 <svc_handler+0xd84>
8000e840:	e7943003 	ldr	r3, [r4, r3]
8000e844:	e5933000 	ldr	r3, [r3]
8000e848:	e5933010 	ldr	r3, [r3, #16]
8000e84c:	e3530000 	cmp	r3, #0
8000e850:	0a0002e9 	beq	8000f3fc <svc_handler+0xd18>
8000e854:	e24bd024 	sub	sp, fp, #36	; 0x24
8000e858:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
8000e85c:	e59f3c04 	ldr	r3, [pc, #3076]	; 8000f468 <svc_handler+0xd84>
8000e860:	e1a01005 	mov	r1, r5
8000e864:	e7943003 	ldr	r3, [r4, r3]
8000e868:	e5933000 	ldr	r3, [r3]
8000e86c:	e5930004 	ldr	r0, [r3, #4]
8000e870:	e24bd024 	sub	sp, fp, #36	; 0x24
8000e874:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000e878:	eafffe95 	b	8000e2d4 <uspace_interrupt_register>
8000e87c:	e59f3be4 	ldr	r3, [pc, #3044]	; 8000f468 <svc_handler+0xd84>
8000e880:	e3a02000 	mov	r2, #0
8000e884:	e7943003 	ldr	r3, [r4, r3]
8000e888:	e5933000 	ldr	r3, [r3]
8000e88c:	e58350b0 	str	r5, [r3, #176]	; 0xb0
8000e890:	e58380b4 	str	r8, [r3, #180]	; 0xb4
8000e894:	e58390b8 	str	r9, [r3, #184]	; 0xb8
8000e898:	e5c320bc 	strb	r2, [r3, #188]	; 0xbc
8000e89c:	eaffffec 	b	8000e854 <svc_handler+0x170>
8000e8a0:	e59f3bc0 	ldr	r3, [pc, #3008]	; 8000f468 <svc_handler+0xd84>
8000e8a4:	e3a02000 	mov	r2, #0
8000e8a8:	e7943003 	ldr	r3, [r4, r3]
8000e8ac:	e5933000 	ldr	r3, [r3]
8000e8b0:	e5832018 	str	r2, [r3, #24]
8000e8b4:	eaffffe6 	b	8000e854 <svc_handler+0x170>
8000e8b8:	e59f3ba8 	ldr	r3, [pc, #2984]	; 8000f468 <svc_handler+0xd84>
8000e8bc:	e7943003 	ldr	r3, [r4, r3]
8000e8c0:	e5933000 	ldr	r3, [r3]
8000e8c4:	e5932010 	ldr	r2, [r3, #16]
8000e8c8:	e3520000 	cmp	r2, #0
8000e8cc:	03a02020 	moveq	r2, #32
8000e8d0:	05832018 	streq	r2, [r3, #24]
8000e8d4:	eaffffde 	b	8000e854 <svc_handler+0x170>
8000e8d8:	e59f3b88 	ldr	r3, [pc, #2952]	; 8000f468 <svc_handler+0xd84>
8000e8dc:	e7944003 	ldr	r4, [r4, r3]
8000e8e0:	e5943000 	ldr	r3, [r4]
8000e8e4:	e5933010 	ldr	r3, [r3, #16]
8000e8e8:	e3530000 	cmp	r3, #0
8000e8ec:	1a000011 	bne	8000e938 <svc_handler+0x254>
8000e8f0:	ebffe746 	bl	80008610 <fb_get_info>
8000e8f4:	e3a02028 	mov	r2, #40	; 0x28
8000e8f8:	e1a06000 	mov	r6, r0
8000e8fc:	e1a01006 	mov	r1, r6
8000e900:	e1a00005 	mov	r0, r5
8000e904:	ebfff0bd 	bl	8000ac00 <memcpy>
8000e908:	e5942000 	ldr	r2, [r4]
8000e90c:	e5953020 	ldr	r3, [r5, #32]
8000e910:	e592102c 	ldr	r1, [r2, #44]	; 0x2c
8000e914:	e5952024 	ldr	r2, [r5, #36]	; 0x24
8000e918:	e5910000 	ldr	r0, [r1]
8000e91c:	e5961020 	ldr	r1, [r6, #32]
8000e920:	e0833002 	add	r3, r3, r2
8000e924:	e3a0c0ff 	mov	ip, #255	; 0xff
8000e928:	e2812102 	add	r2, r1, #-2147483648	; 0x80000000
8000e92c:	e2833102 	add	r3, r3, #-2147483648	; 0x80000000
8000e930:	e58dc000 	str	ip, [sp]
8000e934:	ebfffaab 	bl	8000d3e8 <map_pages>
8000e938:	e3a03000 	mov	r3, #0
8000e93c:	e5873008 	str	r3, [r7, #8]
8000e940:	eaffffc3 	b	8000e854 <svc_handler+0x170>
8000e944:	e59f3b1c 	ldr	r3, [pc, #2844]	; 8000f468 <svc_handler+0xd84>
8000e948:	e7944003 	ldr	r4, [r4, r3]
8000e94c:	e5943000 	ldr	r3, [r4]
8000e950:	e5933010 	ldr	r3, [r3, #16]
8000e954:	e3530000 	cmp	r3, #0
8000e958:	13a03000 	movne	r3, #0
8000e95c:	1a00000b 	bne	8000e990 <svc_handler+0x2ac>
8000e960:	ebffe650 	bl	800082a8 <get_hw_info>
8000e964:	e5943000 	ldr	r3, [r4]
8000e968:	e593102c 	ldr	r1, [r3, #44]	; 0x2c
8000e96c:	e5902024 	ldr	r2, [r0, #36]	; 0x24
8000e970:	e5903028 	ldr	r3, [r0, #40]	; 0x28
8000e974:	e5910000 	ldr	r0, [r1]
8000e978:	e3a010ff 	mov	r1, #255	; 0xff
8000e97c:	e0823003 	add	r3, r2, r3
8000e980:	e58d1000 	str	r1, [sp]
8000e984:	e3a01103 	mov	r1, #-1073741824	; 0xc0000000
8000e988:	ebfffa96 	bl	8000d3e8 <map_pages>
8000e98c:	e3a03103 	mov	r3, #-1073741824	; 0xc0000000
8000e990:	e5873008 	str	r3, [r7, #8]
8000e994:	e24bd024 	sub	sp, fp, #36	; 0x24
8000e998:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
8000e99c:	e3a00001 	mov	r0, #1
8000e9a0:	eb000774 	bl	80010778 <kfork>
8000e9a4:	e2504000 	subs	r4, r0, #0
8000e9a8:	0a000023 	beq	8000ea3c <svc_handler+0x358>
8000e9ac:	e594610c 	ldr	r6, [r4, #268]	; 0x10c
8000e9b0:	e28400d0 	add	r0, r4, #208	; 0xd0
8000e9b4:	e1a01007 	mov	r1, r7
8000e9b8:	e3a02044 	mov	r2, #68	; 0x44
8000e9bc:	ebfff08f 	bl	8000ac00 <memcpy>
8000e9c0:	e584610c 	str	r6, [r4, #268]	; 0x10c
8000e9c4:	e58450d4 	str	r5, [r4, #212]	; 0xd4
8000e9c8:	e5845110 	str	r5, [r4, #272]	; 0x110
8000e9cc:	e58480d8 	str	r8, [r4, #216]	; 0xd8
8000e9d0:	e58490dc 	str	r9, [r4, #220]	; 0xdc
8000e9d4:	e5943004 	ldr	r3, [r4, #4]
8000e9d8:	eaffffec 	b	8000e990 <svc_handler+0x2ac>
8000e9dc:	e1a00005 	mov	r0, r5
8000e9e0:	e1a01008 	mov	r1, r8
8000e9e4:	e1a02009 	mov	r2, r9
8000e9e8:	ebffff1b 	bl	8000e65c <get_global>
8000e9ec:	e5870008 	str	r0, [r7, #8]
8000e9f0:	eaffff97 	b	8000e854 <svc_handler+0x170>
8000e9f4:	e5953000 	ldr	r3, [r5]
8000e9f8:	e3530000 	cmp	r3, #0
8000e9fc:	0a00000e 	beq	8000ea3c <svc_handler+0x358>
8000ea00:	e5931004 	ldr	r1, [r3, #4]
8000ea04:	e3510000 	cmp	r1, #0
8000ea08:	13580000 	cmpne	r8, #0
8000ea0c:	0a00000a 	beq	8000ea3c <svc_handler+0x358>
8000ea10:	e1a00008 	mov	r0, r8
8000ea14:	e2811018 	add	r1, r1, #24
8000ea18:	e3a02058 	mov	r2, #88	; 0x58
8000ea1c:	ebfff077 	bl	8000ac00 <memcpy>
8000ea20:	e3a03000 	mov	r3, #0
8000ea24:	eaffffd9 	b	8000e990 <svc_handler+0x2ac>
8000ea28:	eb000a71 	bl	800113f4 <vfs_root>
8000ea2c:	e1a01005 	mov	r1, r5
8000ea30:	eb000a3f 	bl	80011334 <vfs_get>
8000ea34:	e2501000 	subs	r1, r0, #0
8000ea38:	1afffff4 	bne	8000ea10 <svc_handler+0x32c>
8000ea3c:	e3e03000 	mvn	r3, #0
8000ea40:	eaffffd2 	b	8000e990 <svc_handler+0x2ac>
8000ea44:	e3550000 	cmp	r5, #0
8000ea48:	0affff81 	beq	8000e854 <svc_handler+0x170>
8000ea4c:	e5950000 	ldr	r0, [r5]
8000ea50:	e3500000 	cmp	r0, #0
8000ea54:	0affff7e 	beq	8000e854 <svc_handler+0x170>
8000ea58:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ea5c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ea60:	ea000982 	b	80011070 <vfs_umount>
8000ea64:	e3580000 	cmp	r8, #0
8000ea68:	13550000 	cmpne	r5, #0
8000ea6c:	0afffff2 	beq	8000ea3c <svc_handler+0x358>
8000ea70:	e5950000 	ldr	r0, [r5]
8000ea74:	e5981000 	ldr	r1, [r8]
8000ea78:	e3510000 	cmp	r1, #0
8000ea7c:	13500000 	cmpne	r0, #0
8000ea80:	03a04001 	moveq	r4, #1
8000ea84:	13a04000 	movne	r4, #0
8000ea88:	0affffeb 	beq	8000ea3c <svc_handler+0x358>
8000ea8c:	eb000908 	bl	80010eb4 <vfs_mount>
8000ea90:	e1a03004 	mov	r3, r4
8000ea94:	eaffffbd 	b	8000e990 <svc_handler+0x2ac>
8000ea98:	e16f3f18 	clz	r3, r8
8000ea9c:	e1a032a3 	lsr	r3, r3, #5
8000eaa0:	e1933fa5 	orrs	r3, r3, r5, lsr #31
8000eaa4:	0a000244 	beq	8000f3bc <svc_handler+0xcd8>
8000eaa8:	e3e00000 	mvn	r0, #0
8000eaac:	e5870008 	str	r0, [r7, #8]
8000eab0:	e24bd024 	sub	sp, fp, #36	; 0x24
8000eab4:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
8000eab8:	e3580000 	cmp	r8, #0
8000eabc:	13550000 	cmpne	r5, #0
8000eac0:	0afffff8 	beq	8000eaa8 <svc_handler+0x3c4>
8000eac4:	e5950000 	ldr	r0, [r5]
8000eac8:	e3500000 	cmp	r0, #0
8000eacc:	0afffff5 	beq	8000eaa8 <svc_handler+0x3c4>
8000ead0:	e1a01008 	mov	r1, r8
8000ead4:	eb00087d 	bl	80010cd0 <vfs_get_mount>
8000ead8:	eafffff3 	b	8000eaac <svc_handler+0x3c8>
8000eadc:	e3550000 	cmp	r5, #0
8000eae0:	0affffd5 	beq	8000ea3c <svc_handler+0x358>
8000eae4:	eb0009e9 	bl	80011290 <vfs_new_node>
8000eae8:	e3500000 	cmp	r0, #0
8000eaec:	0affffd2 	beq	8000ea3c <svc_handler+0x358>
8000eaf0:	e3e03000 	mvn	r3, #0
8000eaf4:	e5850000 	str	r0, [r5]
8000eaf8:	e5853050 	str	r3, [r5, #80]	; 0x50
8000eafc:	e2800018 	add	r0, r0, #24
8000eb00:	e1a01005 	mov	r1, r5
8000eb04:	e3a02058 	mov	r2, #88	; 0x58
8000eb08:	ebfff03c 	bl	8000ac00 <memcpy>
8000eb0c:	e3a03000 	mov	r3, #0
8000eb10:	eaffff9e 	b	8000e990 <svc_handler+0x2ac>
8000eb14:	e1a00005 	mov	r0, r5
8000eb18:	e1a01008 	mov	r1, r8
8000eb1c:	e1a02009 	mov	r2, r9
8000eb20:	eb0007ea 	bl	80010ad0 <proc_get_msg>
8000eb24:	e5870008 	str	r0, [r7, #8]
8000eb28:	eaffff49 	b	8000e854 <svc_handler+0x170>
8000eb2c:	e1a00005 	mov	r0, r5
8000eb30:	e1a01008 	mov	r1, r8
8000eb34:	e1a02009 	mov	r2, r9
8000eb38:	eb0007e4 	bl	80010ad0 <proc_get_msg>
8000eb3c:	e3500000 	cmp	r0, #0
8000eb40:	aaffffd9 	bge	8000eaac <svc_handler+0x3c8>
8000eb44:	e59f391c 	ldr	r3, [pc, #2332]	; 8000f468 <svc_handler+0xd84>
8000eb48:	e3e02000 	mvn	r2, #0
8000eb4c:	e5872008 	str	r2, [r7, #8]
8000eb50:	e7943003 	ldr	r3, [r4, r3]
8000eb54:	e1a00007 	mov	r0, r7
8000eb58:	e5931000 	ldr	r1, [r3]
8000eb5c:	e2811004 	add	r1, r1, #4
8000eb60:	e24bd024 	sub	sp, fp, #36	; 0x24
8000eb64:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000eb68:	ea000595 	b	800101c4 <proc_block_on>
8000eb6c:	e1a00005 	mov	r0, r5
8000eb70:	e1a01008 	mov	r1, r8
8000eb74:	e1a02009 	mov	r2, r9
8000eb78:	eb00078d 	bl	800109b4 <proc_send_msg>
8000eb7c:	e3500000 	cmp	r0, #0
8000eb80:	0affffad 	beq	8000ea3c <svc_handler+0x358>
8000eb84:	e5903000 	ldr	r3, [r0]
8000eb88:	eaffff80 	b	8000e990 <svc_handler+0x2ac>
8000eb8c:	e3580000 	cmp	r8, #0
8000eb90:	0affffa9 	beq	8000ea3c <svc_handler+0x358>
8000eb94:	e59f38cc 	ldr	r3, [pc, #2252]	; 8000f468 <svc_handler+0xd84>
8000eb98:	e1a01005 	mov	r1, r5
8000eb9c:	e7944003 	ldr	r4, [r4, r3]
8000eba0:	e5943000 	ldr	r3, [r4]
8000eba4:	e59300c8 	ldr	r0, [r3, #200]	; 0xc8
8000eba8:	ebfff40c 	bl	8000bbe0 <str_cpy>
8000ebac:	e1a01008 	mov	r1, r8
8000ebb0:	e1a02009 	mov	r2, r9
8000ebb4:	e5940000 	ldr	r0, [r4]
8000ebb8:	eb000515 	bl	80010014 <proc_load_elf>
8000ebbc:	e3500000 	cmp	r0, #0
8000ebc0:	1affff9d 	bne	8000ea3c <svc_handler+0x358>
8000ebc4:	e5941000 	ldr	r1, [r4]
8000ebc8:	e5870008 	str	r0, [r7, #8]
8000ebcc:	e28110d0 	add	r1, r1, #208	; 0xd0
8000ebd0:	e1a00007 	mov	r0, r7
8000ebd4:	e3a02044 	mov	r2, #68	; 0x44
8000ebd8:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ebdc:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ebe0:	eafff006 	b	8000ac00 <memcpy>
8000ebe4:	e355003f 	cmp	r5, #63	; 0x3f
8000ebe8:	8affff52 	bhi	8000e938 <svc_handler+0x254>
8000ebec:	e59f3874 	ldr	r3, [pc, #2164]	; 8000f468 <svc_handler+0xd84>
8000ebf0:	e2855088 	add	r5, r5, #136	; 0x88
8000ebf4:	e7944003 	ldr	r4, [r4, r3]
8000ebf8:	e5943000 	ldr	r3, [r4]
8000ebfc:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000ec00:	e7930105 	ldr	r0, [r3, r5, lsl #2]
8000ec04:	e3500000 	cmp	r0, #0
8000ec08:	0affff4a 	beq	8000e938 <svc_handler+0x254>
8000ec0c:	ebfffb68 	bl	8000d9b4 <kfree>
8000ec10:	e5943000 	ldr	r3, [r4]
8000ec14:	e3a02000 	mov	r2, #0
8000ec18:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000ec1c:	e7832105 	str	r2, [r3, r5, lsl #2]
8000ec20:	eaffff44 	b	8000e938 <svc_handler+0x254>
8000ec24:	e355003f 	cmp	r5, #63	; 0x3f
8000ec28:	8affff09 	bhi	8000e854 <svc_handler+0x170>
8000ec2c:	e59f3834 	ldr	r3, [pc, #2100]	; 8000f468 <svc_handler+0xd84>
8000ec30:	e2855088 	add	r5, r5, #136	; 0x88
8000ec34:	e7943003 	ldr	r3, [r4, r3]
8000ec38:	e5933000 	ldr	r3, [r3]
8000ec3c:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000ec40:	e7930105 	ldr	r0, [r3, r5, lsl #2]
8000ec44:	e3500000 	cmp	r0, #0
8000ec48:	0affff01 	beq	8000e854 <svc_handler+0x170>
8000ec4c:	e3a03000 	mov	r3, #0
8000ec50:	e5803000 	str	r3, [r0]
8000ec54:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ec58:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ec5c:	ea00057c 	b	80010254 <proc_wakeup>
8000ec60:	e355003f 	cmp	r5, #63	; 0x3f
8000ec64:	8affff33 	bhi	8000e938 <svc_handler+0x254>
8000ec68:	e59f37f8 	ldr	r3, [pc, #2040]	; 8000f468 <svc_handler+0xd84>
8000ec6c:	e2855088 	add	r5, r5, #136	; 0x88
8000ec70:	e7943003 	ldr	r3, [r4, r3]
8000ec74:	e5933000 	ldr	r3, [r3]
8000ec78:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000ec7c:	e7931105 	ldr	r1, [r3, r5, lsl #2]
8000ec80:	e3510000 	cmp	r1, #0
8000ec84:	0affff2b 	beq	8000e938 <svc_handler+0x254>
8000ec88:	e5913000 	ldr	r3, [r1]
8000ec8c:	e3530000 	cmp	r3, #0
8000ec90:	1a0001d3 	bne	8000f3e4 <svc_handler+0xd00>
8000ec94:	e3a02001 	mov	r2, #1
8000ec98:	e5812000 	str	r2, [r1]
8000ec9c:	eaffff3b 	b	8000e990 <svc_handler+0x2ac>
8000eca0:	e59f37c0 	ldr	r3, [pc, #1984]	; 8000f468 <svc_handler+0xd84>
8000eca4:	e3a05000 	mov	r5, #0
8000eca8:	e7946003 	ldr	r6, [r4, r3]
8000ecac:	e5963000 	ldr	r3, [r6]
8000ecb0:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000ecb4:	e2833f87 	add	r3, r3, #540	; 0x21c
8000ecb8:	ea000002 	b	8000ecc8 <svc_handler+0x5e4>
8000ecbc:	e2855001 	add	r5, r5, #1
8000ecc0:	e3550040 	cmp	r5, #64	; 0x40
8000ecc4:	0a0001d2 	beq	8000f414 <svc_handler+0xd30>
8000ecc8:	e5b34004 	ldr	r4, [r3, #4]!
8000eccc:	e3540000 	cmp	r4, #0
8000ecd0:	1afffff9 	bne	8000ecbc <svc_handler+0x5d8>
8000ecd4:	e3a00004 	mov	r0, #4
8000ecd8:	ebfffb22 	bl	8000d968 <kmalloc>
8000ecdc:	e5963000 	ldr	r3, [r6]
8000ece0:	e2852088 	add	r2, r5, #136	; 0x88
8000ece4:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000ece8:	e5804000 	str	r4, [r0]
8000ecec:	e7830102 	str	r0, [r3, r2, lsl #2]
8000ecf0:	e5875008 	str	r5, [r7, #8]
8000ecf4:	eafffed6 	b	8000e854 <svc_handler+0x170>
8000ecf8:	e59f3768 	ldr	r3, [pc, #1896]	; 8000f468 <svc_handler+0xd84>
8000ecfc:	e3a02000 	mov	r2, #0
8000ed00:	e7943003 	ldr	r3, [r4, r3]
8000ed04:	e5933000 	ldr	r3, [r3]
8000ed08:	e5832008 	str	r2, [r3, #8]
8000ed0c:	eafffed0 	b	8000e854 <svc_handler+0x170>
8000ed10:	e1a00005 	mov	r0, r5
8000ed14:	eb000298 	bl	8000f77c <proc_get>
8000ed18:	e2501000 	subs	r1, r0, #0
8000ed1c:	0afffecc 	beq	8000e854 <svc_handler+0x170>
8000ed20:	e59f3740 	ldr	r3, [pc, #1856]	; 8000f468 <svc_handler+0xd84>
8000ed24:	e7943003 	ldr	r3, [r4, r3]
8000ed28:	e5933000 	ldr	r3, [r3]
8000ed2c:	e5933010 	ldr	r3, [r3, #16]
8000ed30:	e3530000 	cmp	r3, #0
8000ed34:	0a000002 	beq	8000ed44 <svc_handler+0x660>
8000ed38:	e5912010 	ldr	r2, [r1, #16]
8000ed3c:	e1530002 	cmp	r3, r2
8000ed40:	1afffec3 	bne	8000e854 <svc_handler+0x170>
8000ed44:	e1a00007 	mov	r0, r7
8000ed48:	e3a02000 	mov	r2, #0
8000ed4c:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ed50:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ed54:	ea00036b 	b	8000fb08 <proc_exit>
8000ed58:	e59f3708 	ldr	r3, [pc, #1800]	; 8000f468 <svc_handler+0xd84>
8000ed5c:	e7943003 	ldr	r3, [r4, r3]
8000ed60:	e5931000 	ldr	r1, [r3]
8000ed64:	e3510000 	cmp	r1, #0
8000ed68:	0afffeb9 	beq	8000e854 <svc_handler+0x170>
8000ed6c:	e1a00007 	mov	r0, r7
8000ed70:	e1a02005 	mov	r2, r5
8000ed74:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ed78:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ed7c:	ea000361 	b	8000fb08 <proc_exit>
8000ed80:	e1a00007 	mov	r0, r7
8000ed84:	e1a01005 	mov	r1, r5
8000ed88:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ed8c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ed90:	ea0004fb 	b	80010184 <proc_usleep>
8000ed94:	e1a00007 	mov	r0, r7
8000ed98:	e1a01005 	mov	r1, r5
8000ed9c:	e24bd024 	sub	sp, fp, #36	; 0x24
8000eda0:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000eda4:	ea000514 	b	800101fc <proc_waitpid>
8000eda8:	e1a00007 	mov	r0, r7
8000edac:	e24bd024 	sub	sp, fp, #36	; 0x24
8000edb0:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000edb4:	ea000789 	b	80010be0 <schedule>
8000edb8:	e3a00000 	mov	r0, #0
8000edbc:	eb00066d 	bl	80010778 <kfork>
8000edc0:	e2504000 	subs	r4, r0, #0
8000edc4:	0affff1c 	beq	8000ea3c <svc_handler+0x358>
8000edc8:	e28400d0 	add	r0, r4, #208	; 0xd0
8000edcc:	e1a01007 	mov	r1, r7
8000edd0:	e3a02044 	mov	r2, #68	; 0x44
8000edd4:	ebffef89 	bl	8000ac00 <memcpy>
8000edd8:	e3a03000 	mov	r3, #0
8000eddc:	e58430d8 	str	r3, [r4, #216]	; 0xd8
8000ede0:	e5943004 	ldr	r3, [r4, #4]
8000ede4:	eafffee9 	b	8000e990 <svc_handler+0x2ac>
8000ede8:	e59f3678 	ldr	r3, [pc, #1656]	; 8000f468 <svc_handler+0xd84>
8000edec:	e7943003 	ldr	r3, [r4, r3]
8000edf0:	e5933000 	ldr	r3, [r3]
8000edf4:	e3530000 	cmp	r3, #0
8000edf8:	0affff0f 	beq	8000ea3c <svc_handler+0x358>
8000edfc:	e5933004 	ldr	r3, [r3, #4]
8000ee00:	eafffee2 	b	8000e990 <svc_handler+0x2ac>
8000ee04:	e3550000 	cmp	r5, #0
8000ee08:	0afffe91 	beq	8000e854 <svc_handler+0x170>
8000ee0c:	e1a00005 	mov	r0, r5
8000ee10:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ee14:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ee18:	ea0003d5 	b	8000fd74 <proc_free>
8000ee1c:	e1a00005 	mov	r0, r5
8000ee20:	eb0003c7 	bl	8000fd44 <proc_malloc>
8000ee24:	eaffff20 	b	8000eaac <svc_handler+0x3c8>
8000ee28:	e1a00005 	mov	r0, r5
8000ee2c:	eb000ad2 	bl	8001197c <get_dev>
8000ee30:	e3500000 	cmp	r0, #0
8000ee34:	0affff00 	beq	8000ea3c <svc_handler+0x358>
8000ee38:	eb000b09 	bl	80011a64 <dev_block_write_done>
8000ee3c:	e3500000 	cmp	r0, #0
8000ee40:	1afffefd 	bne	8000ea3c <svc_handler+0x358>
8000ee44:	e5870008 	str	r0, [r7, #8]
8000ee48:	e1a00005 	mov	r0, r5
8000ee4c:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ee50:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ee54:	ea0004fe 	b	80010254 <proc_wakeup>
8000ee58:	e1a00005 	mov	r0, r5
8000ee5c:	eb000ac6 	bl	8001197c <get_dev>
8000ee60:	e3500000 	cmp	r0, #0
8000ee64:	0afffef4 	beq	8000ea3c <svc_handler+0x358>
8000ee68:	e1a01008 	mov	r1, r8
8000ee6c:	eb000af6 	bl	80011a4c <dev_block_read_done>
8000ee70:	e3500000 	cmp	r0, #0
8000ee74:	1afffef0 	bne	8000ea3c <svc_handler+0x358>
8000ee78:	eafffff1 	b	8000ee44 <svc_handler+0x760>
8000ee7c:	e1a00005 	mov	r0, r5
8000ee80:	eb000abd 	bl	8001197c <get_dev>
8000ee84:	e3500000 	cmp	r0, #0
8000ee88:	0affff06 	beq	8000eaa8 <svc_handler+0x3c4>
8000ee8c:	e1a01008 	mov	r1, r8
8000ee90:	e1a02009 	mov	r2, r9
8000ee94:	eb000ae6 	bl	80011a34 <dev_block_write>
8000ee98:	eaffff03 	b	8000eaac <svc_handler+0x3c8>
8000ee9c:	e1a00005 	mov	r0, r5
8000eea0:	eb000ab5 	bl	8001197c <get_dev>
8000eea4:	e3500000 	cmp	r0, #0
8000eea8:	0afffefe 	beq	8000eaa8 <svc_handler+0x3c4>
8000eeac:	e1a01008 	mov	r1, r8
8000eeb0:	eb000ad9 	bl	80011a1c <dev_block_read>
8000eeb4:	eafffefc 	b	8000eaac <svc_handler+0x3c8>
8000eeb8:	e1a00005 	mov	r0, r5
8000eebc:	eb000aae 	bl	8001197c <get_dev>
8000eec0:	e3500000 	cmp	r0, #0
8000eec4:	0afffedc 	beq	8000ea3c <svc_handler+0x358>
8000eec8:	e1a01008 	mov	r1, r8
8000eecc:	e1a02009 	mov	r2, r9
8000eed0:	eb000acb 	bl	80011a04 <dev_ch_write>
8000eed4:	e2504000 	subs	r4, r0, #0
8000eed8:	0afffeec 	beq	8000ea90 <svc_handler+0x3ac>
8000eedc:	e1a00005 	mov	r0, r5
8000eee0:	eb0004db 	bl	80010254 <proc_wakeup>
8000eee4:	e1a03004 	mov	r3, r4
8000eee8:	eafffea8 	b	8000e990 <svc_handler+0x2ac>
8000eeec:	e1a00005 	mov	r0, r5
8000eef0:	eb000aa1 	bl	8001197c <get_dev>
8000eef4:	e3500000 	cmp	r0, #0
8000eef8:	0afffecf 	beq	8000ea3c <svc_handler+0x358>
8000eefc:	e1a01008 	mov	r1, r8
8000ef00:	e1a02009 	mov	r2, r9
8000ef04:	eb000ab8 	bl	800119ec <dev_ch_read>
8000ef08:	e2504000 	subs	r4, r0, #0
8000ef0c:	0afffedf 	beq	8000ea90 <svc_handler+0x3ac>
8000ef10:	eafffff1 	b	8000eedc <svc_handler+0x7f8>
8000ef14:	e1a00005 	mov	r0, r5
8000ef18:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ef1c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ef20:	eafff800 	b	8000cf28 <printf>
8000ef24:	e1a00005 	mov	r0, r5
8000ef28:	eb000213 	bl	8000f77c <proc_get>
8000ef2c:	e1a02009 	mov	r2, r9
8000ef30:	e59030c8 	ldr	r3, [r0, #200]	; 0xc8
8000ef34:	e1a00008 	mov	r0, r8
8000ef38:	e5931000 	ldr	r1, [r3]
8000ef3c:	e24bd024 	sub	sp, fp, #36	; 0x24
8000ef40:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000ef44:	eaffef86 	b	8000ad64 <strncpy>
8000ef48:	e3550000 	cmp	r5, #0
8000ef4c:	0afffeba 	beq	8000ea3c <svc_handler+0x358>
8000ef50:	e5953044 	ldr	r3, [r5, #68]	; 0x44
8000ef54:	e3530002 	cmp	r3, #2
8000ef58:	1afffeb7 	bne	8000ea3c <svc_handler+0x358>
8000ef5c:	e5954054 	ldr	r4, [r5, #84]	; 0x54
8000ef60:	e3540000 	cmp	r4, #0
8000ef64:	0afffeb4 	beq	8000ea3c <svc_handler+0x358>
8000ef68:	e1a00004 	mov	r0, r4
8000ef6c:	e5981004 	ldr	r1, [r8, #4]
8000ef70:	e5982000 	ldr	r2, [r8]
8000ef74:	ebfff075 	bl	8000b150 <buffer_write>
8000ef78:	e1a06000 	mov	r6, r0
8000ef7c:	e1a00004 	mov	r0, r4
8000ef80:	eb0004b3 	bl	80010254 <proc_wakeup>
8000ef84:	e3560000 	cmp	r6, #0
8000ef88:	da000123 	ble	8000f41c <svc_handler+0xd38>
8000ef8c:	e5876008 	str	r6, [r7, #8]
8000ef90:	eafffe2f 	b	8000e854 <svc_handler+0x170>
8000ef94:	e3550000 	cmp	r5, #0
8000ef98:	0afffea7 	beq	8000ea3c <svc_handler+0x358>
8000ef9c:	e5953044 	ldr	r3, [r5, #68]	; 0x44
8000efa0:	e3530002 	cmp	r3, #2
8000efa4:	1afffea4 	bne	8000ea3c <svc_handler+0x358>
8000efa8:	e5954054 	ldr	r4, [r5, #84]	; 0x54
8000efac:	e3540000 	cmp	r4, #0
8000efb0:	0afffea1 	beq	8000ea3c <svc_handler+0x358>
8000efb4:	e1a00004 	mov	r0, r4
8000efb8:	e5981004 	ldr	r1, [r8, #4]
8000efbc:	e5982000 	ldr	r2, [r8]
8000efc0:	ebfff042 	bl	8000b0d0 <buffer_read>
8000efc4:	eaffffeb 	b	8000ef78 <svc_handler+0x894>
8000efc8:	e3580000 	cmp	r8, #0
8000efcc:	13550000 	cmpne	r5, #0
8000efd0:	03a06001 	moveq	r6, #1
8000efd4:	13a06000 	movne	r6, #0
8000efd8:	0a00011d 	beq	8000f454 <svc_handler+0xd70>
8000efdc:	eb0008ab 	bl	80011290 <vfs_new_node>
8000efe0:	e2509000 	subs	r9, r0, #0
8000efe4:	0a00011a 	beq	8000f454 <svc_handler+0xd70>
8000efe8:	e59f3478 	ldr	r3, [pc, #1144]	; 8000f468 <svc_handler+0xd84>
8000efec:	e3a02002 	mov	r2, #2
8000eff0:	e589205c 	str	r2, [r9, #92]	; 0x5c
8000eff4:	e7944003 	ldr	r4, [r4, r3]
8000eff8:	e1a01009 	mov	r1, r9
8000effc:	e5943000 	ldr	r3, [r4]
8000f000:	e3a02001 	mov	r2, #1
8000f004:	e5930004 	ldr	r0, [r3, #4]
8000f008:	eb0008fe 	bl	80011408 <vfs_open>
8000f00c:	e3500000 	cmp	r0, #0
8000f010:	ba00010d 	blt	8000f44c <svc_handler+0xd68>
8000f014:	e5943000 	ldr	r3, [r4]
8000f018:	e1a01009 	mov	r1, r9
8000f01c:	e5850000 	str	r0, [r5]
8000f020:	e3a02001 	mov	r2, #1
8000f024:	e5930004 	ldr	r0, [r3, #4]
8000f028:	eb0008f6 	bl	80011408 <vfs_open>
8000f02c:	e3500000 	cmp	r0, #0
8000f030:	ba000102 	blt	8000f440 <svc_handler+0xd5c>
8000f034:	e5880000 	str	r0, [r8]
8000f038:	e59f042c 	ldr	r0, [pc, #1068]	; 8000f46c <svc_handler+0xd88>
8000f03c:	ebfffa49 	bl	8000d968 <kmalloc>
8000f040:	e1a01006 	mov	r1, r6
8000f044:	e59f2420 	ldr	r2, [pc, #1056]	; 8000f46c <svc_handler+0xd88>
8000f048:	e1a04000 	mov	r4, r0
8000f04c:	ebffef07 	bl	8000ac70 <memset>
8000f050:	e589406c 	str	r4, [r9, #108]	; 0x6c
8000f054:	eaffffcc 	b	8000ef8c <svc_handler+0x8a8>
8000f058:	e1a00005 	mov	r0, r5
8000f05c:	e1a01008 	mov	r1, r8
8000f060:	eb0009b5 	bl	8001173c <vfs_dup2>
8000f064:	e5870008 	str	r0, [r7, #8]
8000f068:	eafffdf9 	b	8000e854 <svc_handler+0x170>
8000f06c:	e1a04fa5 	lsr	r4, r5, #31
8000f070:	e3580000 	cmp	r8, #0
8000f074:	03844001 	orreq	r4, r4, #1
8000f078:	e3540000 	cmp	r4, #0
8000f07c:	1a0000f6 	bne	8000f45c <svc_handler+0xd78>
8000f080:	e1a00005 	mov	r0, r5
8000f084:	eb000915 	bl	800114e0 <vfs_node_by_fd>
8000f088:	e2501000 	subs	r1, r0, #0
8000f08c:	0a0000f2 	beq	8000f45c <svc_handler+0xd78>
8000f090:	e1a00008 	mov	r0, r8
8000f094:	e2811018 	add	r1, r1, #24
8000f098:	e3a02058 	mov	r2, #88	; 0x58
8000f09c:	ebffeed7 	bl	8000ac00 <memcpy>
8000f0a0:	e5874008 	str	r4, [r7, #8]
8000f0a4:	eafffdea 	b	8000e854 <svc_handler+0x170>
8000f0a8:	e3550000 	cmp	r5, #0
8000f0ac:	bafffe7d 	blt	8000eaa8 <svc_handler+0x3c4>
8000f0b0:	e1a00005 	mov	r0, r5
8000f0b4:	eb0009e4 	bl	8001184c <vfs_tell>
8000f0b8:	eafffe7b 	b	8000eaac <svc_handler+0x3c8>
8000f0bc:	e59f33a4 	ldr	r3, [pc, #932]	; 8000f468 <svc_handler+0xd84>
8000f0c0:	e1a01005 	mov	r1, r5
8000f0c4:	e7943003 	ldr	r3, [r4, r3]
8000f0c8:	e5933000 	ldr	r3, [r3]
8000f0cc:	e5930004 	ldr	r0, [r3, #4]
8000f0d0:	ebfffb57 	bl	8000de34 <shm_proc_map>
8000f0d4:	e5870008 	str	r0, [r7, #8]
8000f0d8:	eafffddd 	b	8000e854 <svc_handler+0x170>
8000f0dc:	e1a00005 	mov	r0, r5
8000f0e0:	e1a01008 	mov	r1, r8
8000f0e4:	ebfffa6b 	bl	8000da98 <shm_alloc>
8000f0e8:	e5870008 	str	r0, [r7, #8]
8000f0ec:	eafffdd8 	b	8000e854 <svc_handler+0x170>
8000f0f0:	e1a00005 	mov	r0, r5
8000f0f4:	eb0004b2 	bl	800103c4 <proc_get_env_value>
8000f0f8:	e2501000 	subs	r1, r0, #0
8000f0fc:	0a000012 	beq	8000f14c <svc_handler+0xa68>
8000f100:	e1a00008 	mov	r0, r8
8000f104:	e1a02009 	mov	r2, r9
8000f108:	ebffef15 	bl	8000ad64 <strncpy>
8000f10c:	eafffe09 	b	8000e938 <svc_handler+0x254>
8000f110:	e1a00005 	mov	r0, r5
8000f114:	eb000492 	bl	80010364 <proc_get_env_name>
8000f118:	e5d03000 	ldrb	r3, [r0]
8000f11c:	e1a01000 	mov	r1, r0
8000f120:	e3530000 	cmp	r3, #0
8000f124:	0afffe44 	beq	8000ea3c <svc_handler+0x358>
8000f128:	e1a00008 	mov	r0, r8
8000f12c:	e1a02009 	mov	r2, r9
8000f130:	ebffef0b 	bl	8000ad64 <strncpy>
8000f134:	e3a03000 	mov	r3, #0
8000f138:	eafffe14 	b	8000e990 <svc_handler+0x2ac>
8000f13c:	e1a00005 	mov	r0, r5
8000f140:	eb000460 	bl	800102c8 <proc_get_env>
8000f144:	e2501000 	subs	r1, r0, #0
8000f148:	1affffec 	bne	8000f100 <svc_handler+0xa1c>
8000f14c:	e5c81000 	strb	r1, [r8]
8000f150:	eafffdf8 	b	8000e938 <svc_handler+0x254>
8000f154:	e1a00005 	mov	r0, r5
8000f158:	e1a01008 	mov	r1, r8
8000f15c:	eb0004b0 	bl	80010424 <proc_set_env>
8000f160:	e5870008 	str	r0, [r7, #8]
8000f164:	eafffdba 	b	8000e854 <svc_handler+0x170>
8000f168:	e59f32f8 	ldr	r3, [pc, #760]	; 8000f468 <svc_handler+0xd84>
8000f16c:	e1a00005 	mov	r0, r5
8000f170:	e7943003 	ldr	r3, [r4, r3]
8000f174:	e1a02008 	mov	r2, r8
8000f178:	e5933000 	ldr	r3, [r3]
8000f17c:	e59330cc 	ldr	r3, [r3, #204]	; 0xcc
8000f180:	eaffff6c 	b	8000ef38 <svc_handler+0x854>
8000f184:	e59f32dc 	ldr	r3, [pc, #732]	; 8000f468 <svc_handler+0xd84>
8000f188:	e1a01005 	mov	r1, r5
8000f18c:	e7943003 	ldr	r3, [r4, r3]
8000f190:	e5933000 	ldr	r3, [r3]
8000f194:	e59300cc 	ldr	r0, [r3, #204]	; 0xcc
8000f198:	ebfff290 	bl	8000bbe0 <str_cpy>
8000f19c:	e3a03000 	mov	r3, #0
8000f1a0:	e5873008 	str	r3, [r7, #8]
8000f1a4:	eafffdaa 	b	8000e854 <svc_handler+0x170>
8000f1a8:	e1a00005 	mov	r0, r5
8000f1ac:	eb000579 	bl	80010798 <get_procs>
8000f1b0:	e5870008 	str	r0, [r7, #8]
8000f1b4:	eafffda6 	b	8000e854 <svc_handler+0x170>
8000f1b8:	e3550000 	cmp	r5, #0
8000f1bc:	0afffda4 	beq	8000e854 <svc_handler+0x170>
8000f1c0:	ebffe438 	bl	800082a8 <get_hw_info>
8000f1c4:	e1a01000 	mov	r1, r0
8000f1c8:	e1a00005 	mov	r0, r5
8000f1cc:	ebffeed7 	bl	8000ad30 <strcpy>
8000f1d0:	ebfff82f 	bl	8000d294 <get_free_mem_size>
8000f1d4:	e5850020 	str	r0, [r5, #32]
8000f1d8:	ebffe432 	bl	800082a8 <get_hw_info>
8000f1dc:	e5903020 	ldr	r3, [r0, #32]
8000f1e0:	e5853028 	str	r3, [r5, #40]	; 0x28
8000f1e4:	ebfffac7 	bl	8000dd08 <shm_alloced_size>
8000f1e8:	e59f3280 	ldr	r3, [pc, #640]	; 8000f470 <svc_handler+0xd8c>
8000f1ec:	e5850024 	str	r0, [r5, #36]	; 0x24
8000f1f0:	e7943003 	ldr	r3, [r4, r3]
8000f1f4:	e5933000 	ldr	r3, [r3]
8000f1f8:	e585302c 	str	r3, [r5, #44]	; 0x2c
8000f1fc:	eafffd94 	b	8000e854 <svc_handler+0x170>
8000f200:	e1a00005 	mov	r0, r5
8000f204:	e1a01008 	mov	r1, r8
8000f208:	ebfffcde 	bl	8000e588 <set_global>
8000f20c:	e5870008 	str	r0, [r7, #8]
8000f210:	eafffd8f 	b	8000e854 <svc_handler+0x170>
8000f214:	e59f324c 	ldr	r3, [pc, #588]	; 8000f468 <svc_handler+0xd84>
8000f218:	e1a01005 	mov	r1, r5
8000f21c:	e7943003 	ldr	r3, [r4, r3]
8000f220:	e5933000 	ldr	r3, [r3]
8000f224:	e5930004 	ldr	r0, [r3, #4]
8000f228:	ebfffada 	bl	8000dd98 <shm_proc_ref>
8000f22c:	e5870008 	str	r0, [r7, #8]
8000f230:	eafffd87 	b	8000e854 <svc_handler+0x170>
8000f234:	e59f322c 	ldr	r3, [pc, #556]	; 8000f468 <svc_handler+0xd84>
8000f238:	e1a01005 	mov	r1, r5
8000f23c:	e7943003 	ldr	r3, [r4, r3]
8000f240:	e5933000 	ldr	r3, [r3]
8000f244:	e5930004 	ldr	r0, [r3, #4]
8000f248:	ebfffb4b 	bl	8000df7c <shm_proc_unmap>
8000f24c:	e5870008 	str	r0, [r7, #8]
8000f250:	eafffd7f 	b	8000e854 <svc_handler+0x170>
8000f254:	e1a00005 	mov	r0, r5
8000f258:	e1a01008 	mov	r1, r8
8000f25c:	eb000966 	bl	800117fc <vfs_seek>
8000f260:	e5870008 	str	r0, [r7, #8]
8000f264:	eafffd7a 	b	8000e854 <svc_handler+0x170>
8000f268:	e3550000 	cmp	r5, #0
8000f26c:	bafffd78 	blt	8000e854 <svc_handler+0x170>
8000f270:	e59f31f0 	ldr	r3, [pc, #496]	; 8000f468 <svc_handler+0xd84>
8000f274:	e1a01005 	mov	r1, r5
8000f278:	e7943003 	ldr	r3, [r4, r3]
8000f27c:	e5930000 	ldr	r0, [r3]
8000f280:	e24bd024 	sub	sp, fp, #36	; 0x24
8000f284:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000f288:	ea0008a0 	b	80011510 <vfs_close>
8000f28c:	e16f3f18 	clz	r3, r8
8000f290:	e1a032a3 	lsr	r3, r3, #5
8000f294:	e1933fa5 	orrs	r3, r3, r5, lsr #31
8000f298:	1afffe02 	bne	8000eaa8 <svc_handler+0x3c4>
8000f29c:	e5981000 	ldr	r1, [r8]
8000f2a0:	e3510000 	cmp	r1, #0
8000f2a4:	0afffdff 	beq	8000eaa8 <svc_handler+0x3c4>
8000f2a8:	e1a00005 	mov	r0, r5
8000f2ac:	e1a02009 	mov	r2, r9
8000f2b0:	eb000854 	bl	80011408 <vfs_open>
8000f2b4:	eafffdfc 	b	8000eaac <svc_handler+0x3c8>
8000f2b8:	e3550000 	cmp	r5, #0
8000f2bc:	0afffdde 	beq	8000ea3c <svc_handler+0x358>
8000f2c0:	e5950000 	ldr	r0, [r5]
8000f2c4:	e3500000 	cmp	r0, #0
8000f2c8:	0afffddb 	beq	8000ea3c <svc_handler+0x358>
8000f2cc:	eb000797 	bl	80011130 <vfs_del>
8000f2d0:	e3a03000 	mov	r3, #0
8000f2d4:	eafffdad 	b	8000e990 <svc_handler+0x2ac>
8000f2d8:	e3580000 	cmp	r8, #0
8000f2dc:	13550000 	cmpne	r5, #0
8000f2e0:	03a04001 	moveq	r4, #1
8000f2e4:	13a04000 	movne	r4, #0
8000f2e8:	0afffdee 	beq	8000eaa8 <svc_handler+0x3c4>
8000f2ec:	e5955000 	ldr	r5, [r5]
8000f2f0:	e3550000 	cmp	r5, #0
8000f2f4:	0afffdeb 	beq	8000eaa8 <svc_handler+0x3c4>
8000f2f8:	e2881004 	add	r1, r8, #4
8000f2fc:	e1a00005 	mov	r0, r5
8000f300:	eb00080b 	bl	80011334 <vfs_get>
8000f304:	e2501000 	subs	r1, r0, #0
8000f308:	0a00002f 	beq	8000f3cc <svc_handler+0xce8>
8000f30c:	e1a00008 	mov	r0, r8
8000f310:	e2811018 	add	r1, r1, #24
8000f314:	e3a02058 	mov	r2, #88	; 0x58
8000f318:	ebffee38 	bl	8000ac00 <memcpy>
8000f31c:	e1a00004 	mov	r0, r4
8000f320:	eafffde1 	b	8000eaac <svc_handler+0x3c8>
8000f324:	e3550000 	cmp	r5, #0
8000f328:	0afffdde 	beq	8000eaa8 <svc_handler+0x3c4>
8000f32c:	e5950000 	ldr	r0, [r5]
8000f330:	e3500000 	cmp	r0, #0
8000f334:	0afffddb 	beq	8000eaa8 <svc_handler+0x3c4>
8000f338:	e1a01005 	mov	r1, r5
8000f33c:	eb0007b4 	bl	80011214 <vfs_set>
8000f340:	eafffdd9 	b	8000eaac <svc_handler+0x3c8>
8000f344:	e5953000 	ldr	r3, [r5]
8000f348:	e3530000 	cmp	r3, #0
8000f34c:	0afffdba 	beq	8000ea3c <svc_handler+0x358>
8000f350:	e5931000 	ldr	r1, [r3]
8000f354:	e3510000 	cmp	r1, #0
8000f358:	13580000 	cmpne	r8, #0
8000f35c:	1afffdab 	bne	8000ea10 <svc_handler+0x32c>
8000f360:	eafffdb5 	b	8000ea3c <svc_handler+0x358>
8000f364:	e5953000 	ldr	r3, [r5]
8000f368:	e3530000 	cmp	r3, #0
8000f36c:	0afffdb2 	beq	8000ea3c <svc_handler+0x358>
8000f370:	e593100c 	ldr	r1, [r3, #12]
8000f374:	e3510000 	cmp	r1, #0
8000f378:	13580000 	cmpne	r8, #0
8000f37c:	1afffda3 	bne	8000ea10 <svc_handler+0x32c>
8000f380:	eafffdad 	b	8000ea3c <svc_handler+0x358>
8000f384:	e1a00005 	mov	r0, r5
8000f388:	eb0008b8 	bl	80011670 <vfs_dup>
8000f38c:	e5870008 	str	r0, [r7, #8]
8000f390:	eafffd2f 	b	8000e854 <svc_handler+0x170>
8000f394:	e59f30cc 	ldr	r3, [pc, #204]	; 8000f468 <svc_handler+0xd84>
8000f398:	e59f00d4 	ldr	r0, [pc, #212]	; 8000f474 <svc_handler+0xd90>
8000f39c:	e7943003 	ldr	r3, [r4, r3]
8000f3a0:	e1a02006 	mov	r2, r6
8000f3a4:	e5933000 	ldr	r3, [r3]
8000f3a8:	e08f0000 	add	r0, pc, r0
8000f3ac:	e5931004 	ldr	r1, [r3, #4]
8000f3b0:	e24bd024 	sub	sp, fp, #36	; 0x24
8000f3b4:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000f3b8:	eafff6da 	b	8000cf28 <printf>
8000f3bc:	e1a00005 	mov	r0, r5
8000f3c0:	e1a01008 	mov	r1, r8
8000f3c4:	eb00062b 	bl	80010c78 <vfs_get_mount_by_id>
8000f3c8:	eafffdb7 	b	8000eaac <svc_handler+0x3c8>
8000f3cc:	e5981000 	ldr	r1, [r8]
8000f3d0:	e3510000 	cmp	r1, #0
8000f3d4:	0afffdb3 	beq	8000eaa8 <svc_handler+0x3c4>
8000f3d8:	e1a00005 	mov	r0, r5
8000f3dc:	eb00065e 	bl	80010d5c <vfs_add>
8000f3e0:	eafffdb1 	b	8000eaac <svc_handler+0x3c8>
8000f3e4:	e3e03000 	mvn	r3, #0
8000f3e8:	e1a00007 	mov	r0, r7
8000f3ec:	e5873008 	str	r3, [r7, #8]
8000f3f0:	e24bd024 	sub	sp, fp, #36	; 0x24
8000f3f4:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000f3f8:	ea000371 	b	800101c4 <proc_block_on>
8000f3fc:	e1a00007 	mov	r0, r7
8000f400:	e1a01005 	mov	r1, r5
8000f404:	e1a02008 	mov	r2, r8
8000f408:	e24bd024 	sub	sp, fp, #36	; 0x24
8000f40c:	e89d6bf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, lr}
8000f410:	eafffb7c 	b	8000e208 <proc_interrupt>
8000f414:	e3e05000 	mvn	r5, #0
8000f418:	eafffe34 	b	8000ecf0 <svc_handler+0x60c>
8000f41c:	e5953000 	ldr	r3, [r5]
8000f420:	e5933070 	ldr	r3, [r3, #112]	; 0x70
8000f424:	e3530001 	cmp	r3, #1
8000f428:	9afffd83 	bls	8000ea3c <svc_handler+0x358>
8000f42c:	e3590000 	cmp	r9, #0
8000f430:	0afffd40 	beq	8000e938 <svc_handler+0x254>
8000f434:	e3a03000 	mov	r3, #0
8000f438:	e1a01004 	mov	r1, r4
8000f43c:	eaffffe9 	b	8000f3e8 <svc_handler+0xd04>
8000f440:	e5940000 	ldr	r0, [r4]
8000f444:	e5951000 	ldr	r1, [r5]
8000f448:	eb000830 	bl	80011510 <vfs_close>
8000f44c:	e1a00009 	mov	r0, r9
8000f450:	ebfff957 	bl	8000d9b4 <kfree>
8000f454:	e3e06000 	mvn	r6, #0
8000f458:	eafffecb 	b	8000ef8c <svc_handler+0x8a8>
8000f45c:	e3e04000 	mvn	r4, #0
8000f460:	eaffff0e 	b	8000f0a0 <svc_handler+0x9bc>
8000f464:	0000d8e8 	andeq	sp, r0, r8, ror #17
8000f468:	00000000 	andeq	r0, r0, r0
8000f46c:	00000408 	andeq	r0, r0, r8, lsl #8
8000f470:	00000030 	andeq	r0, r0, r0, lsr r0
8000f474:	00002d30 	andeq	r2, r0, r0, lsr sp

8000f478 <proc_get_mem_tail>:
8000f478:	e590302c 	ldr	r3, [r0, #44]	; 0x2c
8000f47c:	e593001c 	ldr	r0, [r3, #28]
8000f480:	e12fff1e 	bx	lr

8000f484 <proc_ready>:
8000f484:	e59f2050 	ldr	r2, [pc, #80]	; 8000f4dc <proc_ready+0x58>
8000f488:	e3500000 	cmp	r0, #0
8000f48c:	e08f2002 	add	r2, pc, r2
8000f490:	012fff1e 	bxeq	lr
8000f494:	e590300c 	ldr	r3, [r0, #12]
8000f498:	e3530005 	cmp	r3, #5
8000f49c:	012fff1e 	bxeq	lr
8000f4a0:	e59f3038 	ldr	r3, [pc, #56]	; 8000f4e0 <proc_ready+0x5c>
8000f4a4:	e3a01005 	mov	r1, #5
8000f4a8:	e580100c 	str	r1, [r0, #12]
8000f4ac:	e7922003 	ldr	r2, [r2, r3]
8000f4b0:	e5923000 	ldr	r3, [r2]
8000f4b4:	e3530000 	cmp	r3, #0
8000f4b8:	01a03000 	moveq	r3, r0
8000f4bc:	05820000 	streq	r0, [r2]
8000f4c0:	e5932118 	ldr	r2, [r3, #280]	; 0x118
8000f4c4:	e5803114 	str	r3, [r0, #276]	; 0x114
8000f4c8:	e5802118 	str	r2, [r0, #280]	; 0x118
8000f4cc:	e5932118 	ldr	r2, [r3, #280]	; 0x118
8000f4d0:	e5820114 	str	r0, [r2, #276]	; 0x114
8000f4d4:	e5830118 	str	r0, [r3, #280]	; 0x118
8000f4d8:	e12fff1e 	bx	lr
8000f4dc:	0000cb70 	andeq	ip, r0, r0, ror fp
8000f4e0:	00000028 	andeq	r0, r0, r8, lsr #32

8000f4e4 <proc_wakeup_waiting>:
8000f4e4:	e1a0c00d 	mov	ip, sp
8000f4e8:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000f4ec:	e59f404c 	ldr	r4, [pc, #76]	; 8000f540 <proc_wakeup_waiting+0x5c>
8000f4f0:	e24cb004 	sub	fp, ip, #4
8000f4f4:	e08f4004 	add	r4, pc, r4
8000f4f8:	e1a06000 	mov	r6, r0
8000f4fc:	e2845a09 	add	r5, r4, #36864	; 0x9000
8000f500:	ea000002 	b	8000f510 <proc_wakeup_waiting+0x2c>
8000f504:	e2844e12 	add	r4, r4, #288	; 0x120
8000f508:	e1540005 	cmp	r4, r5
8000f50c:	0a00000a 	beq	8000f53c <proc_wakeup_waiting+0x58>
8000f510:	e594300c 	ldr	r3, [r4, #12]
8000f514:	e3530003 	cmp	r3, #3
8000f518:	1afffff9 	bne	8000f504 <proc_wakeup_waiting+0x20>
8000f51c:	e5943028 	ldr	r3, [r4, #40]	; 0x28
8000f520:	e1530006 	cmp	r3, r6
8000f524:	1afffff6 	bne	8000f504 <proc_wakeup_waiting+0x20>
8000f528:	e1a00004 	mov	r0, r4
8000f52c:	e2844e12 	add	r4, r4, #288	; 0x120
8000f530:	ebffffd3 	bl	8000f484 <proc_ready>
8000f534:	e1540005 	cmp	r4, r5
8000f538:	1afffff4 	bne	8000f510 <proc_wakeup_waiting+0x2c>
8000f53c:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000f540:	00018b04 	andeq	r8, r1, r4, lsl #22

8000f544 <proc_close_files>:
8000f544:	e1a0c00d 	mov	ip, sp
8000f548:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8000f54c:	e24cb004 	sub	fp, ip, #4
8000f550:	e1a05000 	mov	r5, r0
8000f554:	e3a04000 	mov	r4, #0
8000f558:	ea000002 	b	8000f568 <proc_close_files+0x24>
8000f55c:	e2844001 	add	r4, r4, #1
8000f560:	e3540080 	cmp	r4, #128	; 0x80
8000f564:	0a00000b 	beq	8000f598 <proc_close_files+0x54>
8000f568:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
8000f56c:	e0842084 	add	r2, r4, r4, lsl #1
8000f570:	e0833102 	add	r3, r3, r2, lsl #2
8000f574:	e5933320 	ldr	r3, [r3, #800]	; 0x320
8000f578:	e3530000 	cmp	r3, #0
8000f57c:	0afffff6 	beq	8000f55c <proc_close_files+0x18>
8000f580:	e1a01004 	mov	r1, r4
8000f584:	e1a00005 	mov	r0, r5
8000f588:	e2844001 	add	r4, r4, #1
8000f58c:	eb0007df 	bl	80011510 <vfs_close>
8000f590:	e3540080 	cmp	r4, #128	; 0x80
8000f594:	1afffff3 	bne	8000f568 <proc_close_files+0x24>
8000f598:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

8000f59c <proc_unready>:
8000f59c:	e2513000 	subs	r3, r1, #0
8000f5a0:	e1a01000 	mov	r1, r0
8000f5a4:	e59f0094 	ldr	r0, [pc, #148]	; 8000f640 <proc_unready+0xa4>
8000f5a8:	e08f0000 	add	r0, pc, r0
8000f5ac:	012fff1e 	bxeq	lr
8000f5b0:	e5932118 	ldr	r2, [r3, #280]	; 0x118
8000f5b4:	e593c114 	ldr	ip, [r3, #276]	; 0x114
8000f5b8:	e3520000 	cmp	r2, #0
8000f5bc:	0a000015 	beq	8000f618 <proc_unready+0x7c>
8000f5c0:	e582c114 	str	ip, [r2, #276]	; 0x114
8000f5c4:	e593c114 	ldr	ip, [r3, #276]	; 0x114
8000f5c8:	e35c0000 	cmp	ip, #0
8000f5cc:	0a000001 	beq	8000f5d8 <proc_unready+0x3c>
8000f5d0:	e58c2118 	str	r2, [ip, #280]	; 0x118
8000f5d4:	e5932118 	ldr	r2, [r3, #280]	; 0x118
8000f5d8:	e1530002 	cmp	r3, r2
8000f5dc:	0a000010 	beq	8000f624 <proc_unready+0x88>
8000f5e0:	e59fc05c 	ldr	ip, [pc, #92]	; 8000f644 <proc_unready+0xa8>
8000f5e4:	e790c00c 	ldr	ip, [r0, ip]
8000f5e8:	e58c2000 	str	r2, [ip]
8000f5ec:	e59f2054 	ldr	r2, [pc, #84]	; 8000f648 <proc_unready+0xac>
8000f5f0:	e3a0c000 	mov	ip, #0
8000f5f4:	e583c118 	str	ip, [r3, #280]	; 0x118
8000f5f8:	e583c114 	str	ip, [r3, #276]	; 0x114
8000f5fc:	e7902002 	ldr	r2, [r0, r2]
8000f600:	e5922000 	ldr	r2, [r2]
8000f604:	e1520003 	cmp	r2, r3
8000f608:	0a00000a 	beq	8000f638 <proc_unready+0x9c>
8000f60c:	e28300d0 	add	r0, r3, #208	; 0xd0
8000f610:	e3a02044 	mov	r2, #68	; 0x44
8000f614:	eaffed79 	b	8000ac00 <memcpy>
8000f618:	e35c0000 	cmp	ip, #0
8000f61c:	1affffeb 	bne	8000f5d0 <proc_unready+0x34>
8000f620:	eaffffee 	b	8000f5e0 <proc_unready+0x44>
8000f624:	e59f2018 	ldr	r2, [pc, #24]	; 8000f644 <proc_unready+0xa8>
8000f628:	e3a0c000 	mov	ip, #0
8000f62c:	e7902002 	ldr	r2, [r0, r2]
8000f630:	e582c000 	str	ip, [r2]
8000f634:	eaffffec 	b	8000f5ec <proc_unready+0x50>
8000f638:	e1a00001 	mov	r0, r1
8000f63c:	ea000567 	b	80010be0 <schedule>
8000f640:	0000ca54 	andeq	ip, r0, r4, asr sl
8000f644:	00000028 	andeq	r0, r0, r8, lsr #32
8000f648:	00000000 	andeq	r0, r0, r0

8000f64c <proc_get_user_stack_pages.isra.0>:
8000f64c:	e3500000 	cmp	r0, #0
8000f650:	03a00020 	moveq	r0, #32
8000f654:	13a00004 	movne	r0, #4
8000f658:	e12fff1e 	bx	lr

8000f65c <proc_get_user_stack_base.isra.1>:
8000f65c:	e3500000 	cmp	r0, #0
8000f660:	15910000 	ldrne	r0, [r1]
8000f664:	11a00700 	lslne	r0, r0, #14
8000f668:	12600102 	rsbne	r0, r0, #-2147483648	; 0x80000000
8000f66c:	12400a21 	subne	r0, r0, #135168	; 0x21000
8000f670:	059f0000 	ldreq	r0, [pc]	; 8000f678 <proc_get_user_stack_base.isra.1+0x1c>
8000f674:	e12fff1e 	bx	lr
8000f678:	7ffdf000 	svcvc	0x00fdf000

8000f67c <proc_free_locks.isra.4>:
8000f67c:	e1a0c00d 	mov	ip, sp
8000f680:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000f684:	e5903000 	ldr	r3, [r0]
8000f688:	e3a04000 	mov	r4, #0
8000f68c:	e24cb004 	sub	fp, ip, #4
8000f690:	e1a06000 	mov	r6, r0
8000f694:	e1a07004 	mov	r7, r4
8000f698:	ea000001 	b	8000f6a4 <proc_free_locks.isra.4+0x28>
8000f69c:	e3540040 	cmp	r4, #64	; 0x40
8000f6a0:	0a000009 	beq	8000f6cc <proc_free_locks.isra.4+0x50>
8000f6a4:	e2845088 	add	r5, r4, #136	; 0x88
8000f6a8:	e7930105 	ldr	r0, [r3, r5, lsl #2]
8000f6ac:	e2844001 	add	r4, r4, #1
8000f6b0:	e3500000 	cmp	r0, #0
8000f6b4:	0afffff8 	beq	8000f69c <proc_free_locks.isra.4+0x20>
8000f6b8:	ebfff8bd 	bl	8000d9b4 <kfree>
8000f6bc:	e5963000 	ldr	r3, [r6]
8000f6c0:	e3540040 	cmp	r4, #64	; 0x40
8000f6c4:	e7837105 	str	r7, [r3, r5, lsl #2]
8000f6c8:	1afffff5 	bne	8000f6a4 <proc_free_locks.isra.4+0x28>
8000f6cc:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}

8000f6d0 <proc_unmap_shms.isra.5>:
8000f6d0:	e1a0c00d 	mov	ip, sp
8000f6d4:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000f6d8:	e24cb004 	sub	fp, ip, #4
8000f6dc:	e1a06000 	mov	r6, r0
8000f6e0:	e1a05001 	mov	r5, r1
8000f6e4:	e3a04000 	mov	r4, #0
8000f6e8:	e5953000 	ldr	r3, [r5]
8000f6ec:	e2842008 	add	r2, r4, #8
8000f6f0:	e7933102 	ldr	r3, [r3, r2, lsl #2]
8000f6f4:	e2844001 	add	r4, r4, #1
8000f6f8:	e2531000 	subs	r1, r3, #0
8000f6fc:	da000001 	ble	8000f708 <proc_unmap_shms.isra.5+0x38>
8000f700:	e5960000 	ldr	r0, [r6]
8000f704:	ebfffa1c 	bl	8000df7c <shm_proc_unmap>
8000f708:	e3540080 	cmp	r4, #128	; 0x80
8000f70c:	1afffff5 	bne	8000f6e8 <proc_unmap_shms.isra.5+0x18>
8000f710:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}

8000f714 <procs_init>:
8000f714:	e1a0c00d 	mov	ip, sp
8000f718:	e92dd800 	push	{fp, ip, lr, pc}
8000f71c:	e24cb004 	sub	fp, ip, #4
8000f720:	eb00049d 	bl	8001099c <proc_ipc_init>
8000f724:	e59f3040 	ldr	r3, [pc, #64]	; 8000f76c <procs_init+0x58>
8000f728:	e59fc040 	ldr	ip, [pc, #64]	; 8000f770 <procs_init+0x5c>
8000f72c:	e08f3003 	add	r3, pc, r3
8000f730:	e3a00000 	mov	r0, #0
8000f734:	e2831a09 	add	r1, r3, #36864	; 0x9000
8000f738:	e08fc00c 	add	ip, pc, ip
8000f73c:	e583000c 	str	r0, [r3, #12]
8000f740:	e2833e12 	add	r3, r3, #288	; 0x120
8000f744:	e1530001 	cmp	r3, r1
8000f748:	e3a02000 	mov	r2, #0
8000f74c:	1afffffa 	bne	8000f73c <procs_init+0x28>
8000f750:	e59f101c 	ldr	r1, [pc, #28]	; 8000f774 <procs_init+0x60>
8000f754:	e59f301c 	ldr	r3, [pc, #28]	; 8000f778 <procs_init+0x64>
8000f758:	e79c1001 	ldr	r1, [ip, r1]
8000f75c:	e5812000 	str	r2, [r1]
8000f760:	e79c3003 	ldr	r3, [ip, r3]
8000f764:	e5832000 	str	r2, [r3]
8000f768:	e89da800 	ldm	sp, {fp, sp, pc}
8000f76c:	000188cc 	andeq	r8, r1, ip, asr #17
8000f770:	0000c8c4 	andeq	ip, r0, r4, asr #17
8000f774:	00000000 	andeq	r0, r0, r0
8000f778:	00000028 	andeq	r0, r0, r8, lsr #32

8000f77c <proc_get>:
8000f77c:	e350007f 	cmp	r0, #127	; 0x7f
8000f780:	8a000004 	bhi	8000f798 <proc_get+0x1c>
8000f784:	e59f3014 	ldr	r3, [pc, #20]	; 8000f7a0 <proc_get+0x24>
8000f788:	e0800180 	add	r0, r0, r0, lsl #3
8000f78c:	e08f3003 	add	r3, pc, r3
8000f790:	e0830280 	add	r0, r3, r0, lsl #5
8000f794:	e12fff1e 	bx	lr
8000f798:	e3a00000 	mov	r0, #0
8000f79c:	e12fff1e 	bx	lr
8000f7a0:	0001886c 	andeq	r8, r1, ip, ror #16

8000f7a4 <proc_switch>:
8000f7a4:	e1a0c00d 	mov	ip, sp
8000f7a8:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8000f7ac:	e59f3080 	ldr	r3, [pc, #128]	; 8000f834 <proc_switch+0x90>
8000f7b0:	e2514000 	subs	r4, r1, #0
8000f7b4:	e24cb004 	sub	fp, ip, #4
8000f7b8:	e1a06000 	mov	r6, r0
8000f7bc:	e08f3003 	add	r3, pc, r3
8000f7c0:	089da878 	ldmeq	sp, {r3, r4, r5, r6, fp, sp, pc}
8000f7c4:	e59f206c 	ldr	r2, [pc, #108]	; 8000f838 <proc_switch+0x94>
8000f7c8:	e7935002 	ldr	r5, [r3, r2]
8000f7cc:	e5950000 	ldr	r0, [r5]
8000f7d0:	e1540000 	cmp	r4, r0
8000f7d4:	089da878 	ldmeq	sp, {r3, r4, r5, r6, fp, sp, pc}
8000f7d8:	e3500000 	cmp	r0, #0
8000f7dc:	0a000002 	beq	8000f7ec <proc_switch+0x48>
8000f7e0:	e590300c 	ldr	r3, [r0, #12]
8000f7e4:	e3530000 	cmp	r3, #0
8000f7e8:	1a00000c 	bne	8000f820 <proc_switch+0x7c>
8000f7ec:	e1a00006 	mov	r0, r6
8000f7f0:	e28410d0 	add	r1, r4, #208	; 0xd0
8000f7f4:	e3a02044 	mov	r2, #68	; 0x44
8000f7f8:	ebffed00 	bl	8000ac00 <memcpy>
8000f7fc:	e5953000 	ldr	r3, [r5]
8000f800:	e1530004 	cmp	r3, r4
8000f804:	089da878 	ldmeq	sp, {r3, r4, r5, r6, fp, sp, pc}
8000f808:	e594302c 	ldr	r3, [r4, #44]	; 0x2c
8000f80c:	e5930000 	ldr	r0, [r3]
8000f810:	e2800102 	add	r0, r0, #-2147483648	; 0x80000000
8000f814:	ebffe25e 	bl	80008194 <__set_translation_table_base>
8000f818:	e5854000 	str	r4, [r5]
8000f81c:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8000f820:	e1a01006 	mov	r1, r6
8000f824:	e3a02044 	mov	r2, #68	; 0x44
8000f828:	e28000d0 	add	r0, r0, #208	; 0xd0
8000f82c:	ebffecf3 	bl	8000ac00 <memcpy>
8000f830:	eaffffed 	b	8000f7ec <proc_switch+0x48>
8000f834:	0000c840 	andeq	ip, r0, r0, asr #16
8000f838:	00000000 	andeq	r0, r0, r0

8000f83c <proc_shrink_mem>:
8000f83c:	e1a0c00d 	mov	ip, sp
8000f840:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000f844:	e2517000 	subs	r7, r1, #0
8000f848:	e24cb004 	sub	fp, ip, #4
8000f84c:	e1a06000 	mov	r6, r0
8000f850:	d89da8f0 	ldmle	sp, {r4, r5, r6, r7, fp, sp, pc}
8000f854:	e590302c 	ldr	r3, [r0, #44]	; 0x2c
8000f858:	e3a05000 	mov	r5, #0
8000f85c:	e593101c 	ldr	r1, [r3, #28]
8000f860:	e2414a01 	sub	r4, r1, #4096	; 0x1000
8000f864:	e5930000 	ldr	r0, [r3]
8000f868:	e1a01004 	mov	r1, r4
8000f86c:	ebfff71f 	bl	8000d4f0 <resolve_kernel_address>
8000f870:	ebfff65a 	bl	8000d1e0 <kfree4k>
8000f874:	e596302c 	ldr	r3, [r6, #44]	; 0x2c
8000f878:	e1a01004 	mov	r1, r4
8000f87c:	e5930000 	ldr	r0, [r3]
8000f880:	ebfff6f1 	bl	8000d44c <unmap_page>
8000f884:	e596302c 	ldr	r3, [r6, #44]	; 0x2c
8000f888:	e2855001 	add	r5, r5, #1
8000f88c:	e593101c 	ldr	r1, [r3, #28]
8000f890:	e2411a01 	sub	r1, r1, #4096	; 0x1000
8000f894:	e3510000 	cmp	r1, #0
8000f898:	e583101c 	str	r1, [r3, #28]
8000f89c:	0a000001 	beq	8000f8a8 <proc_shrink_mem+0x6c>
8000f8a0:	e1550007 	cmp	r5, r7
8000f8a4:	1affffed 	bne	8000f860 <proc_shrink_mem+0x24>
8000f8a8:	e24bd01c 	sub	sp, fp, #28
8000f8ac:	e89d68f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, lr}
8000f8b0:	ea00089b 	b	80011b24 <_flush_tlb>

8000f8b4 <proc_expand_mem>:
8000f8b4:	e1a0c00d 	mov	ip, sp
8000f8b8:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8000f8bc:	e2516000 	subs	r6, r1, #0
8000f8c0:	e24cb004 	sub	fp, ip, #4
8000f8c4:	e1a05000 	mov	r5, r0
8000f8c8:	c3a04000 	movgt	r4, #0
8000f8cc:	ca00000e 	bgt	8000f90c <proc_expand_mem+0x58>
8000f8d0:	ea00001e 	b	8000f950 <proc_expand_mem+0x9c>
8000f8d4:	ebffece5 	bl	8000ac70 <memset>
8000f8d8:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
8000f8dc:	e2872102 	add	r2, r7, #-2147483648	; 0x80000000
8000f8e0:	e5930000 	ldr	r0, [r3]
8000f8e4:	e593101c 	ldr	r1, [r3, #28]
8000f8e8:	e3a030ff 	mov	r3, #255	; 0xff
8000f8ec:	ebfff680 	bl	8000d2f4 <map_page>
8000f8f0:	e595202c 	ldr	r2, [r5, #44]	; 0x2c
8000f8f4:	e2844001 	add	r4, r4, #1
8000f8f8:	e592301c 	ldr	r3, [r2, #28]
8000f8fc:	e1540006 	cmp	r4, r6
8000f900:	e2833a01 	add	r3, r3, #4096	; 0x1000
8000f904:	e582301c 	str	r3, [r2, #28]
8000f908:	0a000010 	beq	8000f950 <proc_expand_mem+0x9c>
8000f90c:	ebfff62b 	bl	8000d1c0 <kalloc4k>
8000f910:	e3a01000 	mov	r1, #0
8000f914:	e3a02a01 	mov	r2, #4096	; 0x1000
8000f918:	e2507000 	subs	r7, r0, #0
8000f91c:	1affffec 	bne	8000f8d4 <proc_expand_mem+0x20>
8000f920:	ebfff65b 	bl	8000d294 <get_free_mem_size>
8000f924:	e1a03006 	mov	r3, r6
8000f928:	e5952004 	ldr	r2, [r5, #4]
8000f92c:	e1a01000 	mov	r1, r0
8000f930:	e59f0020 	ldr	r0, [pc, #32]	; 8000f958 <proc_expand_mem+0xa4>
8000f934:	e08f0000 	add	r0, pc, r0
8000f938:	ebfff57a 	bl	8000cf28 <printf>
8000f93c:	e1a00005 	mov	r0, r5
8000f940:	e1a01004 	mov	r1, r4
8000f944:	ebffffbc 	bl	8000f83c <proc_shrink_mem>
8000f948:	e3e00000 	mvn	r0, #0
8000f94c:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000f950:	e3a00000 	mov	r0, #0
8000f954:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
8000f958:	000027c0 	andeq	r2, r0, r0, asr #15

8000f95c <proc_expand>:
8000f95c:	eaffffd4 	b	8000f8b4 <proc_expand_mem>

8000f960 <proc_shrink>:
8000f960:	eaffffb5 	b	8000f83c <proc_shrink_mem>

8000f964 <proc_free_space>:
8000f964:	e1a0c00d 	mov	ip, sp
8000f968:	e92dd800 	push	{fp, ip, lr, pc}
8000f96c:	e24cb004 	sub	fp, ip, #4
8000f970:	e24dd010 	sub	sp, sp, #16
8000f974:	e50b0018 	str	r0, [fp, #-24]
8000f978:	e51b3018 	ldr	r3, [fp, #-24]
8000f97c:	e5933000 	ldr	r3, [r3]
8000f980:	e3530000 	cmp	r3, #0
8000f984:	0a000000 	beq	8000f98c <proc_free_space+0x28>
8000f988:	ea00003c 	b	8000fa80 <proc_free_space+0x11c>
8000f98c:	e3a03000 	mov	r3, #0
8000f990:	e50b3010 	str	r3, [fp, #-16]
8000f994:	ea000019 	b	8000fa00 <proc_free_space+0x9c>
8000f998:	e51b3018 	ldr	r3, [fp, #-24]
8000f99c:	e593202c 	ldr	r2, [r3, #44]	; 0x2c
8000f9a0:	e51b3010 	ldr	r3, [fp, #-16]
8000f9a4:	e2833f49 	add	r3, r3, #292	; 0x124
8000f9a8:	e1a03183 	lsl	r3, r3, #3
8000f9ac:	e0823003 	add	r3, r2, r3
8000f9b0:	e50b3014 	str	r3, [fp, #-20]
8000f9b4:	e51b3014 	ldr	r3, [fp, #-20]
8000f9b8:	e5933000 	ldr	r3, [r3]
8000f9bc:	e3530000 	cmp	r3, #0
8000f9c0:	0a000003 	beq	8000f9d4 <proc_free_space+0x70>
8000f9c4:	e51b3014 	ldr	r3, [fp, #-20]
8000f9c8:	e5933000 	ldr	r3, [r3]
8000f9cc:	e1a00003 	mov	r0, r3
8000f9d0:	ebfff0f4 	bl	8000bda8 <str_free>
8000f9d4:	e51b3014 	ldr	r3, [fp, #-20]
8000f9d8:	e5933004 	ldr	r3, [r3, #4]
8000f9dc:	e3530000 	cmp	r3, #0
8000f9e0:	0a000003 	beq	8000f9f4 <proc_free_space+0x90>
8000f9e4:	e51b3014 	ldr	r3, [fp, #-20]
8000f9e8:	e5933004 	ldr	r3, [r3, #4]
8000f9ec:	e1a00003 	mov	r0, r3
8000f9f0:	ebfff0ec 	bl	8000bda8 <str_free>
8000f9f4:	e51b3010 	ldr	r3, [fp, #-16]
8000f9f8:	e2833001 	add	r3, r3, #1
8000f9fc:	e50b3010 	str	r3, [fp, #-16]
8000fa00:	e51b3010 	ldr	r3, [fp, #-16]
8000fa04:	e353001f 	cmp	r3, #31
8000fa08:	daffffe2 	ble	8000f998 <proc_free_space+0x34>
8000fa0c:	e51b3018 	ldr	r3, [fp, #-24]
8000fa10:	e283302c 	add	r3, r3, #44	; 0x2c
8000fa14:	e1a00003 	mov	r0, r3
8000fa18:	ebffff17 	bl	8000f67c <proc_free_locks.isra.4>
8000fa1c:	e51b0018 	ldr	r0, [fp, #-24]
8000fa20:	ebfffec7 	bl	8000f544 <proc_close_files>
8000fa24:	e51b3018 	ldr	r3, [fp, #-24]
8000fa28:	e2832004 	add	r2, r3, #4
8000fa2c:	e51b3018 	ldr	r3, [fp, #-24]
8000fa30:	e283302c 	add	r3, r3, #44	; 0x2c
8000fa34:	e1a00002 	mov	r0, r2
8000fa38:	e1a01003 	mov	r1, r3
8000fa3c:	ebffff23 	bl	8000f6d0 <proc_unmap_shms.isra.5>
8000fa40:	e51b3018 	ldr	r3, [fp, #-24]
8000fa44:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000fa48:	e593301c 	ldr	r3, [r3, #28]
8000fa4c:	e1a03623 	lsr	r3, r3, #12
8000fa50:	e51b0018 	ldr	r0, [fp, #-24]
8000fa54:	e1a01003 	mov	r1, r3
8000fa58:	ebffff77 	bl	8000f83c <proc_shrink_mem>
8000fa5c:	e51b3018 	ldr	r3, [fp, #-24]
8000fa60:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000fa64:	e5933000 	ldr	r3, [r3]
8000fa68:	e1a00003 	mov	r0, r3
8000fa6c:	ebfff6ae 	bl	8000d52c <free_page_tables>
8000fa70:	e51b3018 	ldr	r3, [fp, #-24]
8000fa74:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000fa78:	e1a00003 	mov	r0, r3
8000fa7c:	ebfff7cc 	bl	8000d9b4 <kfree>
8000fa80:	e24bd00c 	sub	sp, fp, #12
8000fa84:	e89da800 	ldm	sp, {fp, sp, pc}

8000fa88 <proc_get_next_ready>:
8000fa88:	e1a0c00d 	mov	ip, sp
8000fa8c:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
8000fa90:	e59f3060 	ldr	r3, [pc, #96]	; 8000faf8 <proc_get_next_ready+0x70>
8000fa94:	e59f2060 	ldr	r2, [pc, #96]	; 8000fafc <proc_get_next_ready+0x74>
8000fa98:	e08f3003 	add	r3, pc, r3
8000fa9c:	e7932002 	ldr	r2, [r3, r2]
8000faa0:	e24cb004 	sub	fp, ip, #4
8000faa4:	e5922000 	ldr	r2, [r2]
8000faa8:	e3520000 	cmp	r2, #0
8000faac:	0a000002 	beq	8000fabc <proc_get_next_ready+0x34>
8000fab0:	e5920118 	ldr	r0, [r2, #280]	; 0x118
8000fab4:	e3500000 	cmp	r0, #0
8000fab8:	189da818 	ldmne	sp, {r3, r4, fp, sp, pc}
8000fabc:	e59f203c 	ldr	r2, [pc, #60]	; 8000fb00 <proc_get_next_ready+0x78>
8000fac0:	e7933002 	ldr	r3, [r3, r2]
8000fac4:	e5930000 	ldr	r0, [r3]
8000fac8:	e3500000 	cmp	r0, #0
8000facc:	189da818 	ldmne	sp, {r3, r4, fp, sp, pc}
8000fad0:	e59f402c 	ldr	r4, [pc, #44]	; 8000fb04 <proc_get_next_ready+0x7c>
8000fad4:	e08f4004 	add	r4, pc, r4
8000fad8:	e594300c 	ldr	r3, [r4, #12]
8000fadc:	e3530007 	cmp	r3, #7
8000fae0:	13530001 	cmpne	r3, #1
8000fae4:	989da818 	ldmls	sp, {r3, r4, fp, sp, pc}
8000fae8:	e1a00004 	mov	r0, r4
8000faec:	ebfffe64 	bl	8000f484 <proc_ready>
8000faf0:	e1a00004 	mov	r0, r4
8000faf4:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}
8000faf8:	0000c564 	andeq	ip, r0, r4, ror #10
8000fafc:	00000000 	andeq	r0, r0, r0
8000fb00:	00000028 	andeq	r0, r0, r8, lsr #32
8000fb04:	00018524 	andeq	r8, r1, r4, lsr #10

8000fb08 <proc_exit>:
8000fb08:	e1a0c00d 	mov	ip, sp
8000fb0c:	e92dd800 	push	{fp, ip, lr, pc}
8000fb10:	e24cb004 	sub	fp, ip, #4
8000fb14:	e24dd020 	sub	sp, sp, #32
8000fb18:	e50b0020 	str	r0, [fp, #-32]
8000fb1c:	e50b1024 	str	r1, [fp, #-36]	; 0x24
8000fb20:	e50b2028 	str	r2, [fp, #-40]	; 0x28
8000fb24:	e51b0020 	ldr	r0, [fp, #-32]
8000fb28:	e51b1024 	ldr	r1, [fp, #-36]	; 0x24
8000fb2c:	eb000048 	bl	8000fc54 <proc_terminate>
8000fb30:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fb34:	e3a02000 	mov	r2, #0
8000fb38:	e583200c 	str	r2, [r3, #12]
8000fb3c:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fb40:	e59330c8 	ldr	r3, [r3, #200]	; 0xc8
8000fb44:	e1a00003 	mov	r0, r3
8000fb48:	ebfff096 	bl	8000bda8 <str_free>
8000fb4c:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fb50:	e59330cc 	ldr	r3, [r3, #204]	; 0xcc
8000fb54:	e1a00003 	mov	r0, r3
8000fb58:	ebfff092 	bl	8000bda8 <str_free>
8000fb5c:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fb60:	e5932000 	ldr	r2, [r3]
8000fb64:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fb68:	e2833004 	add	r3, r3, #4
8000fb6c:	e1a00002 	mov	r0, r2
8000fb70:	e1a01003 	mov	r1, r3
8000fb74:	ebfffeb8 	bl	8000f65c <proc_get_user_stack_base.isra.1>
8000fb78:	e50b0014 	str	r0, [fp, #-20]
8000fb7c:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fb80:	e5933000 	ldr	r3, [r3]
8000fb84:	e1a00003 	mov	r0, r3
8000fb88:	ebfffeaf 	bl	8000f64c <proc_get_user_stack_pages.isra.0>
8000fb8c:	e50b0018 	str	r0, [fp, #-24]
8000fb90:	e3a03000 	mov	r3, #0
8000fb94:	e50b3010 	str	r3, [fp, #-16]
8000fb98:	ea000012 	b	8000fbe8 <proc_exit+0xe0>
8000fb9c:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fba0:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
8000fba4:	e5931000 	ldr	r1, [r3]
8000fba8:	e51b3010 	ldr	r3, [fp, #-16]
8000fbac:	e1a02603 	lsl	r2, r3, #12
8000fbb0:	e51b3014 	ldr	r3, [fp, #-20]
8000fbb4:	e0823003 	add	r3, r2, r3
8000fbb8:	e1a00001 	mov	r0, r1
8000fbbc:	e1a01003 	mov	r1, r3
8000fbc0:	ebfff621 	bl	8000d44c <unmap_page>
8000fbc4:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fbc8:	e51b2010 	ldr	r2, [fp, #-16]
8000fbcc:	e282200c 	add	r2, r2, #12
8000fbd0:	e7933102 	ldr	r3, [r3, r2, lsl #2]
8000fbd4:	e1a00003 	mov	r0, r3
8000fbd8:	ebfff580 	bl	8000d1e0 <kfree4k>
8000fbdc:	e51b3010 	ldr	r3, [fp, #-16]
8000fbe0:	e2833001 	add	r3, r3, #1
8000fbe4:	e50b3010 	str	r3, [fp, #-16]
8000fbe8:	e51b2010 	ldr	r2, [fp, #-16]
8000fbec:	e51b3018 	ldr	r3, [fp, #-24]
8000fbf0:	e1520003 	cmp	r2, r3
8000fbf4:	3affffe8 	bcc	8000fb9c <proc_exit+0x94>
8000fbf8:	e51b0024 	ldr	r0, [fp, #-36]	; 0x24
8000fbfc:	ebffff58 	bl	8000f964 <proc_free_space>
8000fc00:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fc04:	e5933000 	ldr	r3, [r3]
8000fc08:	e3530002 	cmp	r3, #2
8000fc0c:	1a00000a 	bne	8000fc3c <proc_exit+0x134>
8000fc10:	e51b3024 	ldr	r3, [fp, #-36]	; 0x24
8000fc14:	e5933008 	ldr	r3, [r3, #8]
8000fc18:	e1a00003 	mov	r0, r3
8000fc1c:	ebfffed6 	bl	8000f77c <proc_get>
8000fc20:	e50b001c 	str	r0, [fp, #-28]
8000fc24:	e51b301c 	ldr	r3, [fp, #-28]
8000fc28:	e3530000 	cmp	r3, #0
8000fc2c:	0a000002 	beq	8000fc3c <proc_exit+0x134>
8000fc30:	e51b301c 	ldr	r3, [fp, #-28]
8000fc34:	e3a02000 	mov	r2, #0
8000fc38:	e5c320bc 	strb	r2, [r3, #188]	; 0xbc
8000fc3c:	e51b0024 	ldr	r0, [fp, #-36]	; 0x24
8000fc40:	e3a01000 	mov	r1, #0
8000fc44:	e3a02e12 	mov	r2, #288	; 0x120
8000fc48:	ebffec08 	bl	8000ac70 <memset>
8000fc4c:	e24bd00c 	sub	sp, fp, #12
8000fc50:	e89da800 	ldm	sp, {fp, sp, pc}

8000fc54 <proc_terminate>:
8000fc54:	e1a0c00d 	mov	ip, sp
8000fc58:	e92dd800 	push	{fp, ip, lr, pc}
8000fc5c:	e24cb004 	sub	fp, ip, #4
8000fc60:	e24dd010 	sub	sp, sp, #16
8000fc64:	e50b0018 	str	r0, [fp, #-24]
8000fc68:	e50b101c 	str	r1, [fp, #-28]
8000fc6c:	e51b301c 	ldr	r3, [fp, #-28]
8000fc70:	e593300c 	ldr	r3, [r3, #12]
8000fc74:	e3530007 	cmp	r3, #7
8000fc78:	0a000003 	beq	8000fc8c <proc_terminate+0x38>
8000fc7c:	e51b301c 	ldr	r3, [fp, #-28]
8000fc80:	e593300c 	ldr	r3, [r3, #12]
8000fc84:	e3530000 	cmp	r3, #0
8000fc88:	1a000000 	bne	8000fc90 <proc_terminate+0x3c>
8000fc8c:	ea000029 	b	8000fd38 <proc_terminate+0xe4>
8000fc90:	e51b301c 	ldr	r3, [fp, #-28]
8000fc94:	e593300c 	ldr	r3, [r3, #12]
8000fc98:	e3530005 	cmp	r3, #5
8000fc9c:	1a000002 	bne	8000fcac <proc_terminate+0x58>
8000fca0:	e51b0018 	ldr	r0, [fp, #-24]
8000fca4:	e51b101c 	ldr	r1, [fp, #-28]
8000fca8:	ebfffe3b 	bl	8000f59c <proc_unready>
8000fcac:	e51b301c 	ldr	r3, [fp, #-28]
8000fcb0:	e3a02007 	mov	r2, #7
8000fcb4:	e583200c 	str	r2, [r3, #12]
8000fcb8:	e3a03000 	mov	r3, #0
8000fcbc:	e50b3010 	str	r3, [fp, #-16]
8000fcc0:	ea000015 	b	8000fd1c <proc_terminate+0xc8>
8000fcc4:	e51b2010 	ldr	r2, [fp, #-16]
8000fcc8:	e1a03002 	mov	r3, r2
8000fccc:	e1a03183 	lsl	r3, r3, #3
8000fcd0:	e0833002 	add	r3, r3, r2
8000fcd4:	e1a03283 	lsl	r3, r3, #5
8000fcd8:	e59f2060 	ldr	r2, [pc, #96]	; 8000fd40 <proc_terminate+0xec>
8000fcdc:	e08f2002 	add	r2, pc, r2
8000fce0:	e0833002 	add	r3, r3, r2
8000fce4:	e50b3014 	str	r3, [fp, #-20]
8000fce8:	e51b3014 	ldr	r3, [fp, #-20]
8000fcec:	e5932008 	ldr	r2, [r3, #8]
8000fcf0:	e51b301c 	ldr	r3, [fp, #-28]
8000fcf4:	e5933004 	ldr	r3, [r3, #4]
8000fcf8:	e1520003 	cmp	r2, r3
8000fcfc:	1a000003 	bne	8000fd10 <proc_terminate+0xbc>
8000fd00:	e51b0018 	ldr	r0, [fp, #-24]
8000fd04:	e51b1014 	ldr	r1, [fp, #-20]
8000fd08:	e3a02000 	mov	r2, #0
8000fd0c:	ebffff7d 	bl	8000fb08 <proc_exit>
8000fd10:	e51b3010 	ldr	r3, [fp, #-16]
8000fd14:	e2833001 	add	r3, r3, #1
8000fd18:	e50b3010 	str	r3, [fp, #-16]
8000fd1c:	e51b3010 	ldr	r3, [fp, #-16]
8000fd20:	e353007f 	cmp	r3, #127	; 0x7f
8000fd24:	daffffe6 	ble	8000fcc4 <proc_terminate+0x70>
8000fd28:	e51b301c 	ldr	r3, [fp, #-28]
8000fd2c:	e5933004 	ldr	r3, [r3, #4]
8000fd30:	e1a00003 	mov	r0, r3
8000fd34:	ebfffdea 	bl	8000f4e4 <proc_wakeup_waiting>
8000fd38:	e24bd00c 	sub	sp, fp, #12
8000fd3c:	e89da800 	ldm	sp, {fp, sp, pc}
8000fd40:	0001831c 	andeq	r8, r1, ip, lsl r3

8000fd44 <proc_malloc>:
8000fd44:	e59f2020 	ldr	r2, [pc, #32]	; 8000fd6c <proc_malloc+0x28>
8000fd48:	e2501000 	subs	r1, r0, #0
8000fd4c:	e08f2002 	add	r2, pc, r2
8000fd50:	012fff1e 	bxeq	lr
8000fd54:	e59f3014 	ldr	r3, [pc, #20]	; 8000fd70 <proc_malloc+0x2c>
8000fd58:	e7923003 	ldr	r3, [r2, r3]
8000fd5c:	e5933000 	ldr	r3, [r3]
8000fd60:	e593002c 	ldr	r0, [r3, #44]	; 0x2c
8000fd64:	e2800004 	add	r0, r0, #4
8000fd68:	eafff629 	b	8000d614 <trunk_malloc>
8000fd6c:	0000c2b0 			; <UNDEFINED> instruction: 0x0000c2b0
8000fd70:	00000000 	andeq	r0, r0, r0

8000fd74 <proc_free>:
8000fd74:	e59f2020 	ldr	r2, [pc, #32]	; 8000fd9c <proc_free+0x28>
8000fd78:	e2501000 	subs	r1, r0, #0
8000fd7c:	e08f2002 	add	r2, pc, r2
8000fd80:	012fff1e 	bxeq	lr
8000fd84:	e59f3014 	ldr	r3, [pc, #20]	; 8000fda0 <proc_free+0x2c>
8000fd88:	e7923003 	ldr	r3, [r2, r3]
8000fd8c:	e5933000 	ldr	r3, [r3]
8000fd90:	e593002c 	ldr	r0, [r3, #44]	; 0x2c
8000fd94:	e2800004 	add	r0, r0, #4
8000fd98:	eafff662 	b	8000d728 <trunk_free>
8000fd9c:	0000c280 	andeq	ip, r0, r0, lsl #5
8000fda0:	00000000 	andeq	r0, r0, r0

8000fda4 <proc_create>:
8000fda4:	e59f2234 	ldr	r2, [pc, #564]	; 8000ffe0 <proc_create+0x23c>
8000fda8:	e59f3234 	ldr	r3, [pc, #564]	; 8000ffe4 <proc_create+0x240>
8000fdac:	e1a0c00d 	mov	ip, sp
8000fdb0:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8000fdb4:	e08f2002 	add	r2, pc, r2
8000fdb8:	e24cb004 	sub	fp, ip, #4
8000fdbc:	e24dd014 	sub	sp, sp, #20
8000fdc0:	e08f3003 	add	r3, pc, r3
8000fdc4:	e1a06000 	mov	r6, r0
8000fdc8:	e1a0a001 	mov	sl, r1
8000fdcc:	e3a05000 	mov	r5, #0
8000fdd0:	e50b2030 	str	r2, [fp, #-48]	; 0x30
8000fdd4:	ea000002 	b	8000fde4 <proc_create+0x40>
8000fdd8:	e2855001 	add	r5, r5, #1
8000fddc:	e3550080 	cmp	r5, #128	; 0x80
8000fde0:	0a00007b 	beq	8000ffd4 <proc_create+0x230>
8000fde4:	e593100c 	ldr	r1, [r3, #12]
8000fde8:	e2833e12 	add	r3, r3, #288	; 0x120
8000fdec:	e3510000 	cmp	r1, #0
8000fdf0:	1afffff8 	bne	8000fdd8 <proc_create+0x34>
8000fdf4:	e59f81ec 	ldr	r8, [pc, #492]	; 8000ffe8 <proc_create+0x244>
8000fdf8:	e1a09185 	lsl	r9, r5, #3
8000fdfc:	e0894005 	add	r4, r9, r5
8000fe00:	e08f8008 	add	r8, pc, r8
8000fe04:	e1a04284 	lsl	r4, r4, #5
8000fe08:	e0887004 	add	r7, r8, r4
8000fe0c:	e3a02e12 	mov	r2, #288	; 0x120
8000fe10:	e1a00007 	mov	r0, r7
8000fe14:	ebffeb95 	bl	8000ac70 <memset>
8000fe18:	e3e02000 	mvn	r2, #0
8000fe1c:	e3a03001 	mov	r3, #1
8000fe20:	e3560000 	cmp	r6, #0
8000fe24:	e5875004 	str	r5, [r7, #4]
8000fe28:	e7886004 	str	r6, [r8, r4]
8000fe2c:	e5872008 	str	r2, [r7, #8]
8000fe30:	e587300c 	str	r3, [r7, #12]
8000fe34:	0a000037 	beq	8000ff18 <proc_create+0x174>
8000fe38:	e59a30c8 	ldr	r3, [sl, #200]	; 0xc8
8000fe3c:	e59a202c 	ldr	r2, [sl, #44]	; 0x2c
8000fe40:	e5930000 	ldr	r0, [r3]
8000fe44:	e587202c 	str	r2, [r7, #44]	; 0x2c
8000fe48:	ebffef6d 	bl	8000bc04 <str_new>
8000fe4c:	e59a30cc 	ldr	r3, [sl, #204]	; 0xcc
8000fe50:	e28760c8 	add	r6, r7, #200	; 0xc8
8000fe54:	e58700c8 	str	r0, [r7, #200]	; 0xc8
8000fe58:	e5930000 	ldr	r0, [r3]
8000fe5c:	ebffef68 	bl	8000bc04 <str_new>
8000fe60:	e58700cc 	str	r0, [r7, #204]	; 0xcc
8000fe64:	e7983004 	ldr	r3, [r8, r4]
8000fe68:	e59f417c 	ldr	r4, [pc, #380]	; 8000ffec <proc_create+0x248>
8000fe6c:	e3530000 	cmp	r3, #0
8000fe70:	15978004 	ldrne	r8, [r7, #4]
8000fe74:	e0895005 	add	r5, r9, r5
8000fe78:	11a08708 	lslne	r8, r8, #14
8000fe7c:	059fa16c 	ldreq	sl, [pc, #364]	; 8000fff0 <proc_create+0x24c>
8000fe80:	12688102 	rsbne	r8, r8, #-2147483648	; 0x80000000
8000fe84:	e08f4004 	add	r4, pc, r4
8000fe88:	e1a03285 	lsl	r3, r5, #5
8000fe8c:	059f8160 	ldreq	r8, [pc, #352]	; 8000fff4 <proc_create+0x250>
8000fe90:	1248aa21 	subne	sl, r8, #135168	; 0x21000
8000fe94:	e1a09004 	mov	r9, r4
8000fe98:	13a06004 	movne	r6, #4
8000fe9c:	03a06020 	moveq	r6, #32
8000fea0:	e283202c 	add	r2, r3, #44	; 0x2c
8000fea4:	e50b5034 	str	r5, [fp, #-52]	; 0x34
8000fea8:	12488a1d 	subne	r8, r8, #118784	; 0x1d000
8000feac:	e08a6606 	add	r6, sl, r6, lsl #12
8000feb0:	e0844002 	add	r4, r4, r2
8000feb4:	e0895003 	add	r5, r9, r3
8000feb8:	ebfff4c0 	bl	8000d1c0 <kalloc4k>
8000febc:	e1a0100a 	mov	r1, sl
8000fec0:	e28aaa01 	add	sl, sl, #4096	; 0x1000
8000fec4:	e5a40004 	str	r0, [r4, #4]!
8000fec8:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
8000fecc:	e2802102 	add	r2, r0, #-2147483648	; 0x80000000
8000fed0:	e5930000 	ldr	r0, [r3]
8000fed4:	e3a030ff 	mov	r3, #255	; 0xff
8000fed8:	ebfff505 	bl	8000d2f4 <map_page>
8000fedc:	e15a0006 	cmp	sl, r6
8000fee0:	1afffff4 	bne	8000feb8 <proc_create+0x114>
8000fee4:	e3a02050 	mov	r2, #80	; 0x50
8000fee8:	e58520d0 	str	r2, [r5, #208]	; 0xd0
8000feec:	e59f3104 	ldr	r3, [pc, #260]	; 8000fff8 <proc_create+0x254>
8000fef0:	e51b2030 	ldr	r2, [fp, #-48]	; 0x30
8000fef4:	e585810c 	str	r8, [r5, #268]	; 0x10c
8000fef8:	e7923003 	ldr	r3, [r2, r3]
8000fefc:	e51b2034 	ldr	r2, [fp, #-52]	; 0x34
8000ff00:	e5933000 	ldr	r3, [r3]
8000ff04:	e0899282 	add	r9, r9, r2, lsl #5
8000ff08:	e1a00007 	mov	r0, r7
8000ff0c:	e5893014 	str	r3, [r9, #20]
8000ff10:	e24bd028 	sub	sp, fp, #40	; 0x28
8000ff14:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000ff18:	e59f30dc 	ldr	r3, [pc, #220]	; 8000fffc <proc_create+0x258>
8000ff1c:	e287c028 	add	ip, r7, #40	; 0x28
8000ff20:	e08f3003 	add	r3, pc, r3
8000ff24:	e2833030 	add	r3, r3, #48	; 0x30
8000ff28:	e0833705 	add	r3, r3, r5, lsl #14
8000ff2c:	e1a00003 	mov	r0, r3
8000ff30:	e50b3038 	str	r3, [fp, #-56]	; 0x38
8000ff34:	e50bc034 	str	ip, [fp, #-52]	; 0x34
8000ff38:	eb000733 	bl	80011c0c <set_kernel_vm>
8000ff3c:	e3a00ea2 	mov	r0, #2592	; 0xa20
8000ff40:	ebfff688 	bl	8000d968 <kmalloc>
8000ff44:	e51bc034 	ldr	ip, [fp, #-52]	; 0x34
8000ff48:	e1a01006 	mov	r1, r6
8000ff4c:	e3a02ea2 	mov	r2, #2592	; 0xa20
8000ff50:	e287a0c8 	add	sl, r7, #200	; 0xc8
8000ff54:	e58c0004 	str	r0, [ip, #4]
8000ff58:	ebffeb44 	bl	8000ac70 <memset>
8000ff5c:	e51bc034 	ldr	ip, [fp, #-52]	; 0x34
8000ff60:	e59fe098 	ldr	lr, [pc, #152]	; 80010000 <proc_create+0x25c>
8000ff64:	e59c0004 	ldr	r0, [ip, #4]
8000ff68:	e59f2094 	ldr	r2, [pc, #148]	; 80010004 <proc_create+0x260>
8000ff6c:	e59fc094 	ldr	ip, [pc, #148]	; 80010008 <proc_create+0x264>
8000ff70:	e51b3038 	ldr	r3, [fp, #-56]	; 0x38
8000ff74:	e08fe00e 	add	lr, pc, lr
8000ff78:	e08fc00c 	add	ip, pc, ip
8000ff7c:	e08f2002 	add	r2, pc, r2
8000ff80:	e5803000 	str	r3, [r0]
8000ff84:	e580e008 	str	lr, [r0, #8]
8000ff88:	e580c00c 	str	ip, [r0, #12]
8000ff8c:	e1a01006 	mov	r1, r6
8000ff90:	e5802010 	str	r2, [r0, #16]
8000ff94:	e580601c 	str	r6, [r0, #28]
8000ff98:	e3a02c01 	mov	r2, #256	; 0x100
8000ff9c:	e5807004 	str	r7, [r0, #4]
8000ffa0:	e5806014 	str	r6, [r0, #20]
8000ffa4:	e5806018 	str	r6, [r0, #24]
8000ffa8:	e2800e92 	add	r0, r0, #2336	; 0x920
8000ffac:	ebffeb2f 	bl	8000ac70 <memset>
8000ffb0:	e59f0054 	ldr	r0, [pc, #84]	; 8001000c <proc_create+0x268>
8000ffb4:	e08f0000 	add	r0, pc, r0
8000ffb8:	ebffef11 	bl	8000bc04 <str_new>
8000ffbc:	e58700c8 	str	r0, [r7, #200]	; 0xc8
8000ffc0:	e59f0048 	ldr	r0, [pc, #72]	; 80010010 <proc_create+0x26c>
8000ffc4:	e08f0000 	add	r0, pc, r0
8000ffc8:	ebffef0d 	bl	8000bc04 <str_new>
8000ffcc:	e58a0004 	str	r0, [sl, #4]
8000ffd0:	eaffffa3 	b	8000fe64 <proc_create+0xc0>
8000ffd4:	e3a00000 	mov	r0, #0
8000ffd8:	e24bd028 	sub	sp, fp, #40	; 0x28
8000ffdc:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
8000ffe0:	0000c248 	andeq	ip, r0, r8, asr #4
8000ffe4:	00018238 	andeq	r8, r1, r8, lsr r2
8000ffe8:	000181f8 	strdeq	r8, [r1], -r8	; <UNPREDICTABLE>
8000ffec:	00018174 	andeq	r8, r1, r4, ror r1
8000fff0:	7ffdf000 	svcvc	0x00fdf000
8000fff4:	7ffff000 	svcvc	0x00fff000
8000fff8:	00000030 	andeq	r0, r0, r0, lsr r0
8000fffc:	000240a8 	andeq	r4, r2, r8, lsr #1
80010000:	fffff9e0 			; <UNDEFINED> instruction: 0xfffff9e0
80010004:	fffff4f4 			; <UNDEFINED> instruction: 0xfffff4f4
80010008:	fffff9e0 			; <UNDEFINED> instruction: 0xfffff9e0
8001000c:	00002094 	muleq	r0, r4, r0
80010010:	00002068 	andeq	r2, r0, r8, rrx

80010014 <proc_load_elf>:
80010014:	e1a0c00d 	mov	ip, sp
80010018:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
8001001c:	e24cb004 	sub	fp, ip, #4
80010020:	e24dd00c 	sub	sp, sp, #12
80010024:	e1a05000 	mov	r5, r0
80010028:	e1a00002 	mov	r0, r2
8001002c:	e1a04002 	mov	r4, r2
80010030:	e1a06001 	mov	r6, r1
80010034:	ebfff64b 	bl	8000d968 <kmalloc>
80010038:	e1a02004 	mov	r2, r4
8001003c:	e1a01006 	mov	r1, r6
80010040:	e3a07000 	mov	r7, #0
80010044:	e1a08000 	mov	r8, r0
80010048:	ebffeaec 	bl	8000ac00 <memcpy>
8001004c:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
80010050:	e1a00005 	mov	r0, r5
80010054:	e593101c 	ldr	r1, [r3, #28]
80010058:	e1a01621 	lsr	r1, r1, #12
8001005c:	ebfffdf6 	bl	8000f83c <proc_shrink_mem>
80010060:	e1d821b0 	ldrh	r2, [r8, #16]
80010064:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
80010068:	e3520002 	cmp	r2, #2
8001006c:	e5837014 	str	r7, [r3, #20]
80010070:	e5837018 	str	r7, [r3, #24]
80010074:	1a00003a 	bne	80010164 <proc_load_elf+0x150>
80010078:	e1d822bc 	ldrh	r2, [r8, #44]	; 0x2c
8001007c:	e598401c 	ldr	r4, [r8, #28]
80010080:	e1520007 	cmp	r2, r7
80010084:	e50b2030 	str	r2, [fp, #-48]	; 0x30
80010088:	10884004 	addne	r4, r8, r4
8001008c:	1a000006 	bne	800100ac <proc_load_elf+0x98>
80010090:	ea000020 	b	80010118 <proc_load_elf+0x104>
80010094:	e1a00005 	mov	r0, r5
80010098:	e3a01001 	mov	r1, #1
8001009c:	ebfffe04 	bl	8000f8b4 <proc_expand_mem>
800100a0:	e3500000 	cmp	r0, #0
800100a4:	1a000029 	bne	80010150 <proc_load_elf+0x13c>
800100a8:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
800100ac:	e5946008 	ldr	r6, [r4, #8]
800100b0:	e5942014 	ldr	r2, [r4, #20]
800100b4:	e593001c 	ldr	r0, [r3, #28]
800100b8:	e0821006 	add	r1, r2, r6
800100bc:	e1500001 	cmp	r0, r1
800100c0:	3afffff3 	bcc	80010094 <proc_load_elf+0x80>
800100c4:	e3520000 	cmp	r2, #0
800100c8:	e5942004 	ldr	r2, [r4, #4]
800100cc:	13a0a000 	movne	sl, #0
800100d0:	10889002 	addne	r9, r8, r2
800100d4:	1a000001 	bne	800100e0 <proc_load_elf+0xcc>
800100d8:	ea000009 	b	80010104 <proc_load_elf+0xf0>
800100dc:	e595302c 	ldr	r3, [r5, #44]	; 0x2c
800100e0:	e5930000 	ldr	r0, [r3]
800100e4:	e08a1006 	add	r1, sl, r6
800100e8:	ebfff500 	bl	8000d4f0 <resolve_kernel_address>
800100ec:	e4d93001 	ldrb	r3, [r9], #1
800100f0:	e28aa001 	add	sl, sl, #1
800100f4:	e5c03000 	strb	r3, [r0]
800100f8:	e5943014 	ldr	r3, [r4, #20]
800100fc:	e153000a 	cmp	r3, sl
80010100:	8afffff5 	bhi	800100dc <proc_load_elf+0xc8>
80010104:	e51b3030 	ldr	r3, [fp, #-48]	; 0x30
80010108:	e2877001 	add	r7, r7, #1
8001010c:	e1530007 	cmp	r3, r7
80010110:	e2844020 	add	r4, r4, #32
80010114:	8affffe3 	bhi	800100a8 <proc_load_elf+0x94>
80010118:	e5953000 	ldr	r3, [r5]
8001011c:	e3530000 	cmp	r3, #0
80010120:	059f3058 	ldreq	r3, [pc, #88]	; 80010180 <proc_load_elf+0x16c>
80010124:	1a000010 	bne	8001016c <proc_load_elf+0x158>
80010128:	e5982018 	ldr	r2, [r8, #24]
8001012c:	e1a00005 	mov	r0, r5
80010130:	e585310c 	str	r3, [r5, #268]	; 0x10c
80010134:	e58520d4 	str	r2, [r5, #212]	; 0xd4
80010138:	e5852110 	str	r2, [r5, #272]	; 0x110
8001013c:	ebfffcd0 	bl	8000f484 <proc_ready>
80010140:	e1a00008 	mov	r0, r8
80010144:	ebfff61a 	bl	8000d9b4 <kfree>
80010148:	e3a00000 	mov	r0, #0
8001014c:	ea000002 	b	8001015c <proc_load_elf+0x148>
80010150:	e1a00008 	mov	r0, r8
80010154:	ebfff616 	bl	8000d9b4 <kfree>
80010158:	e3e00000 	mvn	r0, #0
8001015c:	e24bd028 	sub	sp, fp, #40	; 0x28
80010160:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
80010164:	e3e00000 	mvn	r0, #0
80010168:	eafffffb 	b	8001015c <proc_load_elf+0x148>
8001016c:	e5953004 	ldr	r3, [r5, #4]
80010170:	e1a03703 	lsl	r3, r3, #14
80010174:	e2633102 	rsb	r3, r3, #-2147483648	; 0x80000000
80010178:	e2433a1d 	sub	r3, r3, #118784	; 0x1d000
8001017c:	eaffffe9 	b	80010128 <proc_load_elf+0x114>
80010180:	7ffff000 	svcvc	0x00fff000

80010184 <proc_usleep>:
80010184:	e59f2030 	ldr	r2, [pc, #48]	; 800101bc <proc_usleep+0x38>
80010188:	e59f3030 	ldr	r3, [pc, #48]	; 800101c0 <proc_usleep+0x3c>
8001018c:	e08f2002 	add	r2, pc, r2
80010190:	e7923003 	ldr	r3, [r2, r3]
80010194:	e5933000 	ldr	r3, [r3]
80010198:	e3530000 	cmp	r3, #0
8001019c:	012fff1e 	bxeq	lr
800101a0:	e3a0c000 	mov	ip, #0
800101a4:	e3a02002 	mov	r2, #2
800101a8:	e5831020 	str	r1, [r3, #32]
800101ac:	e583c024 	str	ip, [r3, #36]	; 0x24
800101b0:	e583200c 	str	r2, [r3, #12]
800101b4:	e1a01003 	mov	r1, r3
800101b8:	eafffcf7 	b	8000f59c <proc_unready>
800101bc:	0000be70 	andeq	fp, r0, r0, ror lr
800101c0:	00000000 	andeq	r0, r0, r0

800101c4 <proc_block_on>:
800101c4:	e59f2028 	ldr	r2, [pc, #40]	; 800101f4 <proc_block_on+0x30>
800101c8:	e59f3028 	ldr	r3, [pc, #40]	; 800101f8 <proc_block_on+0x34>
800101cc:	e08f2002 	add	r2, pc, r2
800101d0:	e7923003 	ldr	r3, [r2, r3]
800101d4:	e5933000 	ldr	r3, [r3]
800101d8:	e3530000 	cmp	r3, #0
800101dc:	012fff1e 	bxeq	lr
800101e0:	e3a02004 	mov	r2, #4
800101e4:	e583101c 	str	r1, [r3, #28]
800101e8:	e583200c 	str	r2, [r3, #12]
800101ec:	e1a01003 	mov	r1, r3
800101f0:	eafffce9 	b	8000f59c <proc_unready>
800101f4:	0000be30 	andeq	fp, r0, r0, lsr lr
800101f8:	00000000 	andeq	r0, r0, r0

800101fc <proc_waitpid>:
800101fc:	e59f2044 	ldr	r2, [pc, #68]	; 80010248 <proc_waitpid+0x4c>
80010200:	e59f3044 	ldr	r3, [pc, #68]	; 8001024c <proc_waitpid+0x50>
80010204:	e08f2002 	add	r2, pc, r2
80010208:	e7923003 	ldr	r3, [r2, r3]
8001020c:	e5932000 	ldr	r2, [r3]
80010210:	e3520000 	cmp	r2, #0
80010214:	012fff1e 	bxeq	lr
80010218:	e59f3030 	ldr	r3, [pc, #48]	; 80010250 <proc_waitpid+0x54>
8001021c:	e081c181 	add	ip, r1, r1, lsl #3
80010220:	e08f3003 	add	r3, pc, r3
80010224:	e083328c 	add	r3, r3, ip, lsl #5
80010228:	e593300c 	ldr	r3, [r3, #12]
8001022c:	e3530000 	cmp	r3, #0
80010230:	012fff1e 	bxeq	lr
80010234:	e3a03003 	mov	r3, #3
80010238:	e5821028 	str	r1, [r2, #40]	; 0x28
8001023c:	e582300c 	str	r3, [r2, #12]
80010240:	e1a01002 	mov	r1, r2
80010244:	eafffcd4 	b	8000f59c <proc_unready>
80010248:	0000bdf8 	strdeq	fp, [r0], -r8
8001024c:	00000000 	andeq	r0, r0, r0
80010250:	00017dd8 	ldrdeq	r7, [r1], -r8

80010254 <proc_wakeup>:
80010254:	e1a0c00d 	mov	ip, sp
80010258:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8001025c:	e59f4060 	ldr	r4, [pc, #96]	; 800102c4 <proc_wakeup+0x70>
80010260:	e24cb004 	sub	fp, ip, #4
80010264:	e08f4004 	add	r4, pc, r4
80010268:	e1a06000 	mov	r6, r0
8001026c:	e2845a09 	add	r5, r4, #36864	; 0x9000
80010270:	e3a07000 	mov	r7, #0
80010274:	ea000002 	b	80010284 <proc_wakeup+0x30>
80010278:	e2844e12 	add	r4, r4, #288	; 0x120
8001027c:	e1540005 	cmp	r4, r5
80010280:	0a00000e 	beq	800102c0 <proc_wakeup+0x6c>
80010284:	e594300c 	ldr	r3, [r4, #12]
80010288:	e3530004 	cmp	r3, #4
8001028c:	1afffff9 	bne	80010278 <proc_wakeup+0x24>
80010290:	e594301c 	ldr	r3, [r4, #28]
80010294:	e1530006 	cmp	r3, r6
80010298:	1afffff6 	bne	80010278 <proc_wakeup+0x24>
8001029c:	e1c422d0 	ldrd	r2, [r4, #32]
800102a0:	e584701c 	str	r7, [r4, #28]
800102a4:	e1923003 	orrs	r3, r2, r3
800102a8:	1afffff2 	bne	80010278 <proc_wakeup+0x24>
800102ac:	e1a00004 	mov	r0, r4
800102b0:	e2844e12 	add	r4, r4, #288	; 0x120
800102b4:	ebfffc72 	bl	8000f484 <proc_ready>
800102b8:	e1540005 	cmp	r4, r5
800102bc:	1afffff0 	bne	80010284 <proc_wakeup+0x30>
800102c0:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
800102c4:	00017d94 	muleq	r1, r4, sp

800102c8 <proc_get_env>:
800102c8:	e59f2084 	ldr	r2, [pc, #132]	; 80010354 <proc_get_env+0x8c>
800102cc:	e59f3084 	ldr	r3, [pc, #132]	; 80010358 <proc_get_env+0x90>
800102d0:	e1a0c00d 	mov	ip, sp
800102d4:	e08f2002 	add	r2, pc, r2
800102d8:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
800102dc:	e7923003 	ldr	r3, [r2, r3]
800102e0:	e24cb004 	sub	fp, ip, #4
800102e4:	e5936000 	ldr	r6, [r3]
800102e8:	e1a07000 	mov	r7, r0
800102ec:	e3a04000 	mov	r4, #0
800102f0:	e596302c 	ldr	r3, [r6, #44]	; 0x2c
800102f4:	e2845f49 	add	r5, r4, #292	; 0x124
800102f8:	e7933185 	ldr	r3, [r3, r5, lsl #3]
800102fc:	e1a01007 	mov	r1, r7
80010300:	e3530000 	cmp	r3, #0
80010304:	e2844001 	add	r4, r4, #1
80010308:	0a000005 	beq	80010324 <proc_get_env+0x5c>
8001030c:	e5930000 	ldr	r0, [r3]
80010310:	ebffeab7 	bl	8000adf4 <strcmp>
80010314:	e3500000 	cmp	r0, #0
80010318:	0a000004 	beq	80010330 <proc_get_env+0x68>
8001031c:	e3540020 	cmp	r4, #32
80010320:	1afffff2 	bne	800102f0 <proc_get_env+0x28>
80010324:	e59f0030 	ldr	r0, [pc, #48]	; 8001035c <proc_get_env+0x94>
80010328:	e08f0000 	add	r0, pc, r0
8001032c:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
80010330:	e596302c 	ldr	r3, [r6, #44]	; 0x2c
80010334:	e0935185 	adds	r5, r3, r5, lsl #3
80010338:	0a000002 	beq	80010348 <proc_get_env+0x80>
8001033c:	e5953004 	ldr	r3, [r5, #4]
80010340:	e5930000 	ldr	r0, [r3]
80010344:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
80010348:	e59f0010 	ldr	r0, [pc, #16]	; 80010360 <proc_get_env+0x98>
8001034c:	e08f0000 	add	r0, pc, r0
80010350:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
80010354:	0000bd28 	andeq	fp, r0, r8, lsr #26
80010358:	00000000 	andeq	r0, r0, r0
8001035c:	00001d20 	andeq	r1, r0, r0, lsr #26
80010360:	00001cfc 	strdeq	r1, [r0], -ip

80010364 <proc_get_env_name>:
80010364:	e59f3048 	ldr	r3, [pc, #72]	; 800103b4 <proc_get_env_name+0x50>
80010368:	e350001f 	cmp	r0, #31
8001036c:	e08f3003 	add	r3, pc, r3
80010370:	8a000009 	bhi	8001039c <proc_get_env_name+0x38>
80010374:	e59f203c 	ldr	r2, [pc, #60]	; 800103b8 <proc_get_env_name+0x54>
80010378:	e2800f49 	add	r0, r0, #292	; 0x124
8001037c:	e7933002 	ldr	r3, [r3, r2]
80010380:	e5933000 	ldr	r3, [r3]
80010384:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
80010388:	e7933180 	ldr	r3, [r3, r0, lsl #3]
8001038c:	e3530000 	cmp	r3, #0
80010390:	0a000004 	beq	800103a8 <proc_get_env_name+0x44>
80010394:	e5930000 	ldr	r0, [r3]
80010398:	e12fff1e 	bx	lr
8001039c:	e59f0018 	ldr	r0, [pc, #24]	; 800103bc <proc_get_env_name+0x58>
800103a0:	e08f0000 	add	r0, pc, r0
800103a4:	e12fff1e 	bx	lr
800103a8:	e59f0010 	ldr	r0, [pc, #16]	; 800103c0 <proc_get_env_name+0x5c>
800103ac:	e08f0000 	add	r0, pc, r0
800103b0:	e12fff1e 	bx	lr
800103b4:	0000bc90 	muleq	r0, r0, ip
800103b8:	00000000 	andeq	r0, r0, r0
800103bc:	00001ca8 	andeq	r1, r0, r8, lsr #25
800103c0:	00001c9c 	muleq	r0, ip, ip

800103c4 <proc_get_env_value>:
800103c4:	e59f3048 	ldr	r3, [pc, #72]	; 80010414 <proc_get_env_value+0x50>
800103c8:	e350001f 	cmp	r0, #31
800103cc:	e08f3003 	add	r3, pc, r3
800103d0:	8a000009 	bhi	800103fc <proc_get_env_value+0x38>
800103d4:	e59f203c 	ldr	r2, [pc, #60]	; 80010418 <proc_get_env_value+0x54>
800103d8:	e7933002 	ldr	r3, [r3, r2]
800103dc:	e5933000 	ldr	r3, [r3]
800103e0:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
800103e4:	e0830180 	add	r0, r3, r0, lsl #3
800103e8:	e5903924 	ldr	r3, [r0, #2340]	; 0x924
800103ec:	e3530000 	cmp	r3, #0
800103f0:	0a000004 	beq	80010408 <proc_get_env_value+0x44>
800103f4:	e5930000 	ldr	r0, [r3]
800103f8:	e12fff1e 	bx	lr
800103fc:	e59f0018 	ldr	r0, [pc, #24]	; 8001041c <proc_get_env_value+0x58>
80010400:	e08f0000 	add	r0, pc, r0
80010404:	e12fff1e 	bx	lr
80010408:	e59f0010 	ldr	r0, [pc, #16]	; 80010420 <proc_get_env_value+0x5c>
8001040c:	e08f0000 	add	r0, pc, r0
80010410:	e12fff1e 	bx	lr
80010414:	0000bc30 	andeq	fp, r0, r0, lsr ip
80010418:	00000000 	andeq	r0, r0, r0
8001041c:	00001c48 	andeq	r1, r0, r8, asr #24
80010420:	00001c3c 	andeq	r1, r0, ip, lsr ip

80010424 <proc_set_env>:
80010424:	e59f20c4 	ldr	r2, [pc, #196]	; 800104f0 <proc_set_env+0xcc>
80010428:	e59f30c4 	ldr	r3, [pc, #196]	; 800104f4 <proc_set_env+0xd0>
8001042c:	e1a0c00d 	mov	ip, sp
80010430:	e08f2002 	add	r2, pc, r2
80010434:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
80010438:	e7929003 	ldr	r9, [r2, r3]
8001043c:	e24cb004 	sub	fp, ip, #4
80010440:	e1a08000 	mov	r8, r0
80010444:	e1a07001 	mov	r7, r1
80010448:	e3a04000 	mov	r4, #0
8001044c:	e5993000 	ldr	r3, [r9]
80010450:	e2846f49 	add	r6, r4, #292	; 0x124
80010454:	e593502c 	ldr	r5, [r3, #44]	; 0x2c
80010458:	e1a01008 	mov	r1, r8
8001045c:	e7953186 	ldr	r3, [r5, r6, lsl #3]
80010460:	e3530000 	cmp	r3, #0
80010464:	0a00001a 	beq	800104d4 <proc_set_env+0xb0>
80010468:	e5930000 	ldr	r0, [r3]
8001046c:	ebffea60 	bl	8000adf4 <strcmp>
80010470:	e3500000 	cmp	r0, #0
80010474:	0a000004 	beq	8001048c <proc_set_env+0x68>
80010478:	e2844001 	add	r4, r4, #1
8001047c:	e3540020 	cmp	r4, #32
80010480:	1afffff1 	bne	8001044c <proc_set_env+0x28>
80010484:	e3e00000 	mvn	r0, #0
80010488:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
8001048c:	e5993000 	ldr	r3, [r9]
80010490:	e593502c 	ldr	r5, [r3, #44]	; 0x2c
80010494:	e7953186 	ldr	r3, [r5, r6, lsl #3]
80010498:	e3530000 	cmp	r3, #0
8001049c:	0a00000c 	beq	800104d4 <proc_set_env+0xb0>
800104a0:	e0854184 	add	r4, r5, r4, lsl #3
800104a4:	e5940924 	ldr	r0, [r4, #2340]	; 0x924
800104a8:	e3500000 	cmp	r0, #0
800104ac:	0a000003 	beq	800104c0 <proc_set_env+0x9c>
800104b0:	e1a01007 	mov	r1, r7
800104b4:	ebffedc9 	bl	8000bbe0 <str_cpy>
800104b8:	e3a00000 	mov	r0, #0
800104bc:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
800104c0:	e59f0030 	ldr	r0, [pc, #48]	; 800104f8 <proc_set_env+0xd4>
800104c4:	e08f0000 	add	r0, pc, r0
800104c8:	ebffedcd 	bl	8000bc04 <str_new>
800104cc:	e5840924 	str	r0, [r4, #2340]	; 0x924
800104d0:	eafffff6 	b	800104b0 <proc_set_env+0x8c>
800104d4:	e59f0020 	ldr	r0, [pc, #32]	; 800104fc <proc_set_env+0xd8>
800104d8:	e08f0000 	add	r0, pc, r0
800104dc:	ebffedc8 	bl	8000bc04 <str_new>
800104e0:	e1a01008 	mov	r1, r8
800104e4:	e7850186 	str	r0, [r5, r6, lsl #3]
800104e8:	ebffedbc 	bl	8000bbe0 <str_cpy>
800104ec:	eaffffeb 	b	800104a0 <proc_set_env+0x7c>
800104f0:	0000bbcc 	andeq	fp, r0, ip, asr #23
800104f4:	00000000 	andeq	r0, r0, r0
800104f8:	00001b84 	andeq	r1, r0, r4, lsl #23
800104fc:	00001b70 	andeq	r1, r0, r0, ror fp

80010500 <kfork_raw>:
80010500:	e1a0c00d 	mov	ip, sp
80010504:	e92ddff8 	push	{r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
80010508:	e24cb004 	sub	fp, ip, #4
8001050c:	e1a04000 	mov	r4, r0
80010510:	e1a07001 	mov	r7, r1
80010514:	ebfffe22 	bl	8000fda4 <proc_create>
80010518:	e2506000 	subs	r6, r0, #0
8001051c:	0a00008a 	beq	8001074c <kfork_raw+0x24c>
80010520:	e5973004 	ldr	r3, [r7, #4]
80010524:	e3540000 	cmp	r4, #0
80010528:	e5863008 	str	r3, [r6, #8]
8001052c:	1a000030 	bne	800105f4 <kfork_raw+0xf4>
80010530:	e597202c 	ldr	r2, [r7, #44]	; 0x2c
80010534:	e592901c 	ldr	r9, [r2, #28]
80010538:	e1b02a09 	lsls	r2, r9, #20
8001053c:	e1a09629 	lsr	r9, r9, #12
80010540:	12899001 	addne	r9, r9, #1
80010544:	e3590000 	cmp	r9, #0
80010548:	0a00002e 	beq	80010608 <kfork_raw+0x108>
8001054c:	e3a04000 	mov	r4, #0
80010550:	ea00000d 	b	8001058c <kfork_raw+0x8c>
80010554:	e596302c 	ldr	r3, [r6, #44]	; 0x2c
80010558:	e5930000 	ldr	r0, [r3]
8001055c:	ebfff3e3 	bl	8000d4f0 <resolve_kernel_address>
80010560:	e597302c 	ldr	r3, [r7, #44]	; 0x2c
80010564:	e1a01005 	mov	r1, r5
80010568:	e1a08000 	mov	r8, r0
8001056c:	e5930000 	ldr	r0, [r3]
80010570:	ebfff3de 	bl	8000d4f0 <resolve_kernel_address>
80010574:	e3a02a01 	mov	r2, #4096	; 0x1000
80010578:	e1a01000 	mov	r1, r0
8001057c:	e1a00008 	mov	r0, r8
80010580:	ebffe99e 	bl	8000ac00 <memcpy>
80010584:	e1540009 	cmp	r4, r9
80010588:	0a00001d 	beq	80010604 <kfork_raw+0x104>
8001058c:	e3a01001 	mov	r1, #1
80010590:	e1a00006 	mov	r0, r6
80010594:	ebfffcc6 	bl	8000f8b4 <proc_expand_mem>
80010598:	e1a05604 	lsl	r5, r4, #12
8001059c:	e1a01005 	mov	r1, r5
800105a0:	e2844001 	add	r4, r4, #1
800105a4:	e3500000 	cmp	r0, #0
800105a8:	0affffe9 	beq	80010554 <kfork_raw+0x54>
800105ac:	e59f01b0 	ldr	r0, [pc, #432]	; 80010764 <kfork_raw+0x264>
800105b0:	e5971004 	ldr	r1, [r7, #4]
800105b4:	e08f0000 	add	r0, pc, r0
800105b8:	ebfff25a 	bl	8000cf28 <printf>
800105bc:	e59f01a4 	ldr	r0, [pc, #420]	; 80010768 <kfork_raw+0x268>
800105c0:	e5971004 	ldr	r1, [r7, #4]
800105c4:	e08f0000 	add	r0, pc, r0
800105c8:	ebfff256 	bl	8000cf28 <printf>
800105cc:	e3a00000 	mov	r0, #0
800105d0:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
800105d4:	e59730c8 	ldr	r3, [r7, #200]	; 0xc8
800105d8:	e59600c8 	ldr	r0, [r6, #200]	; 0xc8
800105dc:	e5931000 	ldr	r1, [r3]
800105e0:	ebffed7e 	bl	8000bbe0 <str_cpy>
800105e4:	e59730cc 	ldr	r3, [r7, #204]	; 0xcc
800105e8:	e59600cc 	ldr	r0, [r6, #204]	; 0xcc
800105ec:	e5931000 	ldr	r1, [r3]
800105f0:	ebffed7a 	bl	8000bbe0 <str_cpy>
800105f4:	e1a00006 	mov	r0, r6
800105f8:	ebfffba1 	bl	8000f484 <proc_ready>
800105fc:	e1a00006 	mov	r0, r6
80010600:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
80010604:	e5973004 	ldr	r3, [r7, #4]
80010608:	e286402c 	add	r4, r6, #44	; 0x2c
8001060c:	e287502c 	add	r5, r7, #44	; 0x2c
80010610:	e28680ac 	add	r8, r6, #172	; 0xac
80010614:	e5863008 	str	r3, [r6, #8]
80010618:	e5b40004 	ldr	r0, [r4, #4]!
8001061c:	e5b51004 	ldr	r1, [r5, #4]!
80010620:	e3a02a01 	mov	r2, #4096	; 0x1000
80010624:	ebffe975 	bl	8000ac00 <memcpy>
80010628:	e1540008 	cmp	r4, r8
8001062c:	1afffff9 	bne	80010618 <kfork_raw+0x118>
80010630:	e597c02c 	ldr	ip, [r7, #44]	; 0x2c
80010634:	e3a05000 	mov	r5, #0
80010638:	e0853085 	add	r3, r5, r5, lsl #1
8001063c:	e1a03103 	lsl	r3, r3, #2
80010640:	e2830e32 	add	r0, r3, #800	; 0x320
80010644:	e08c3003 	add	r3, ip, r3
80010648:	e5938320 	ldr	r8, [r3, #800]	; 0x320
8001064c:	e3a0200c 	mov	r2, #12
80010650:	e3580000 	cmp	r8, #0
80010654:	e08c1000 	add	r1, ip, r0
80010658:	e2834e32 	add	r4, r3, #800	; 0x320
8001065c:	0a00000c 	beq	80010694 <kfork_raw+0x194>
80010660:	e596302c 	ldr	r3, [r6, #44]	; 0x2c
80010664:	e0830000 	add	r0, r3, r0
80010668:	ebffe964 	bl	8000ac00 <memcpy>
8001066c:	e5983070 	ldr	r3, [r8, #112]	; 0x70
80010670:	e2833001 	add	r3, r3, #1
80010674:	e5883070 	str	r3, [r8, #112]	; 0x70
80010678:	e5943004 	ldr	r3, [r4, #4]
8001067c:	e3530000 	cmp	r3, #0
80010680:	15983074 	ldrne	r3, [r8, #116]	; 0x74
80010684:	0597c02c 	ldreq	ip, [r7, #44]	; 0x2c
80010688:	12833001 	addne	r3, r3, #1
8001068c:	1597c02c 	ldrne	ip, [r7, #44]	; 0x2c
80010690:	15883074 	strne	r3, [r8, #116]	; 0x74
80010694:	e2855001 	add	r5, r5, #1
80010698:	e3550080 	cmp	r5, #128	; 0x80
8001069c:	1affffe5 	bne	80010638 <kfork_raw+0x138>
800106a0:	e59f90c4 	ldr	r9, [pc, #196]	; 8001076c <kfork_raw+0x26c>
800106a4:	e59f80c4 	ldr	r8, [pc, #196]	; 80010770 <kfork_raw+0x270>
800106a8:	e08f9009 	add	r9, pc, r9
800106ac:	e08f8008 	add	r8, pc, r8
800106b0:	e3a05000 	mov	r5, #0
800106b4:	ea000010 	b	800106fc <kfork_raw+0x1fc>
800106b8:	e5943924 	ldr	r3, [r4, #2340]	; 0x924
800106bc:	e3530000 	cmp	r3, #0
800106c0:	0a00001d 	beq	8001073c <kfork_raw+0x23c>
800106c4:	e597302c 	ldr	r3, [r7, #44]	; 0x2c
800106c8:	e5940920 	ldr	r0, [r4, #2336]	; 0x920
800106cc:	e793318a 	ldr	r3, [r3, sl, lsl #3]
800106d0:	e5931000 	ldr	r1, [r3]
800106d4:	ebffed41 	bl	8000bbe0 <str_cpy>
800106d8:	e597302c 	ldr	r3, [r7, #44]	; 0x2c
800106dc:	e5940924 	ldr	r0, [r4, #2340]	; 0x924
800106e0:	e083a18a 	add	sl, r3, sl, lsl #3
800106e4:	e59a3004 	ldr	r3, [sl, #4]
800106e8:	e5931000 	ldr	r1, [r3]
800106ec:	ebffed3b 	bl	8000bbe0 <str_cpy>
800106f0:	e3550020 	cmp	r5, #32
800106f4:	0affffb6 	beq	800105d4 <kfork_raw+0xd4>
800106f8:	e597c02c 	ldr	ip, [r7, #44]	; 0x2c
800106fc:	e285af49 	add	sl, r5, #292	; 0x124
80010700:	e79c318a 	ldr	r3, [ip, sl, lsl #3]
80010704:	e3530000 	cmp	r3, #0
80010708:	0affffb1 	beq	800105d4 <kfork_raw+0xd4>
8001070c:	e596402c 	ldr	r4, [r6, #44]	; 0x2c
80010710:	e0844185 	add	r4, r4, r5, lsl #3
80010714:	e5943920 	ldr	r3, [r4, #2336]	; 0x920
80010718:	e2855001 	add	r5, r5, #1
8001071c:	e3530000 	cmp	r3, #0
80010720:	1affffe4 	bne	800106b8 <kfork_raw+0x1b8>
80010724:	e1a00009 	mov	r0, r9
80010728:	ebffed35 	bl	8000bc04 <str_new>
8001072c:	e5943924 	ldr	r3, [r4, #2340]	; 0x924
80010730:	e3530000 	cmp	r3, #0
80010734:	e5840920 	str	r0, [r4, #2336]	; 0x920
80010738:	1affffe1 	bne	800106c4 <kfork_raw+0x1c4>
8001073c:	e1a00008 	mov	r0, r8
80010740:	ebffed2f 	bl	8000bc04 <str_new>
80010744:	e5840924 	str	r0, [r4, #2340]	; 0x924
80010748:	eaffffdd 	b	800106c4 <kfork_raw+0x1c4>
8001074c:	e59f0020 	ldr	r0, [pc, #32]	; 80010774 <kfork_raw+0x274>
80010750:	e5971004 	ldr	r1, [r7, #4]
80010754:	e08f0000 	add	r0, pc, r0
80010758:	ebfff1f2 	bl	8000cf28 <printf>
8001075c:	e1a00006 	mov	r0, r6
80010760:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
80010764:	00001ba8 	andeq	r1, r0, r8, lsr #23
80010768:	00001bc4 	andeq	r1, r0, r4, asr #23
8001076c:	000019a0 	andeq	r1, r0, r0, lsr #19
80010770:	0000199c 	muleq	r0, ip, r9
80010774:	000019e0 	andeq	r1, r0, r0, ror #19

80010778 <kfork>:
80010778:	e59f3010 	ldr	r3, [pc, #16]	; 80010790 <kfork+0x18>
8001077c:	e59f2010 	ldr	r2, [pc, #16]	; 80010794 <kfork+0x1c>
80010780:	e08f3003 	add	r3, pc, r3
80010784:	e7933002 	ldr	r3, [r3, r2]
80010788:	e5931000 	ldr	r1, [r3]
8001078c:	eaffff5b 	b	80010500 <kfork_raw>
80010790:	0000b87c 	andeq	fp, r0, ip, ror r8
80010794:	00000000 	andeq	r0, r0, r0

80010798 <get_procs>:
80010798:	e59f1160 	ldr	r1, [pc, #352]	; 80010900 <get_procs+0x168>
8001079c:	e59f2160 	ldr	r2, [pc, #352]	; 80010904 <get_procs+0x16c>
800107a0:	e1a0c00d 	mov	ip, sp
800107a4:	e92ddff8 	push	{r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
800107a8:	e59f3158 	ldr	r3, [pc, #344]	; 80010908 <get_procs+0x170>
800107ac:	e08f1001 	add	r1, pc, r1
800107b0:	e7916002 	ldr	r6, [r1, r2]
800107b4:	e08f3003 	add	r3, pc, r3
800107b8:	e24cb004 	sub	fp, ip, #4
800107bc:	e1a05000 	mov	r5, r0
800107c0:	e596c000 	ldr	ip, [r6]
800107c4:	e2831a09 	add	r1, r3, #36864	; 0x9000
800107c8:	e3a00000 	mov	r0, #0
800107cc:	ea000005 	b	800107e8 <get_procs+0x50>
800107d0:	e593e010 	ldr	lr, [r3, #16]
800107d4:	e152000e 	cmp	r2, lr
800107d8:	0a000008 	beq	80010800 <get_procs+0x68>
800107dc:	e2833e12 	add	r3, r3, #288	; 0x120
800107e0:	e1530001 	cmp	r3, r1
800107e4:	0a000009 	beq	80010810 <get_procs+0x78>
800107e8:	e593200c 	ldr	r2, [r3, #12]
800107ec:	e3520000 	cmp	r2, #0
800107f0:	0afffff9 	beq	800107dc <get_procs+0x44>
800107f4:	e59c2010 	ldr	r2, [ip, #16]
800107f8:	e3520000 	cmp	r2, #0
800107fc:	1afffff3 	bne	800107d0 <get_procs+0x38>
80010800:	e2833e12 	add	r3, r3, #288	; 0x120
80010804:	e1530001 	cmp	r3, r1
80010808:	e2800001 	add	r0, r0, #1
8001080c:	1afffff5 	bne	800107e8 <get_procs+0x50>
80010810:	e3500000 	cmp	r0, #0
80010814:	e5850000 	str	r0, [r5]
80010818:	0a000036 	beq	800108f8 <get_procs+0x160>
8001081c:	e3a08f47 	mov	r8, #284	; 0x11c
80010820:	e0000098 	mul	r0, r8, r0
80010824:	ebfffd46 	bl	8000fd44 <proc_malloc>
80010828:	e250a000 	subs	sl, r0, #0
8001082c:	0a000031 	beq	800108f8 <get_procs+0x160>
80010830:	e59f40d4 	ldr	r4, [pc, #212]	; 8001090c <get_procs+0x174>
80010834:	e3a07000 	mov	r7, #0
80010838:	e08f4004 	add	r4, pc, r4
8001083c:	e2849a09 	add	r9, r4, #36864	; 0x9000
80010840:	e289900c 	add	r9, r9, #12
80010844:	e284400c 	add	r4, r4, #12
80010848:	ea000004 	b	80010860 <get_procs+0xc8>
8001084c:	e1520001 	cmp	r2, r1
80010850:	0a00000d 	beq	8001088c <get_procs+0xf4>
80010854:	e2844e12 	add	r4, r4, #288	; 0x120
80010858:	e1540009 	cmp	r4, r9
8001085c:	0a000022 	beq	800108ec <get_procs+0x154>
80010860:	e5953000 	ldr	r3, [r5]
80010864:	e1530007 	cmp	r3, r7
80010868:	da00001f 	ble	800108ec <get_procs+0x154>
8001086c:	e5943000 	ldr	r3, [r4]
80010870:	e3530000 	cmp	r3, #0
80010874:	0afffff6 	beq	80010854 <get_procs+0xbc>
80010878:	e5962000 	ldr	r2, [r6]
8001087c:	e5941004 	ldr	r1, [r4, #4]
80010880:	e5922010 	ldr	r2, [r2, #16]
80010884:	e3520000 	cmp	r2, #0
80010888:	1affffef 	bne	8001084c <get_procs+0xb4>
8001088c:	e514c00c 	ldr	ip, [r4, #-12]
80010890:	e0000798 	mul	r0, r8, r7
80010894:	e2844e12 	add	r4, r4, #288	; 0x120
80010898:	e78ac000 	str	ip, [sl, r0]
8001089c:	e08a2000 	add	r2, sl, r0
800108a0:	e5140128 	ldr	r0, [r4, #-296]	; 0x128
800108a4:	e514c100 	ldr	ip, [r4, #-256]	; 0x100
800108a8:	e5820004 	str	r0, [r2, #4]
800108ac:	e5140124 	ldr	r0, [r4, #-292]	; 0x124
800108b0:	e59cc01c 	ldr	ip, [ip, #28]
800108b4:	e5820008 	str	r0, [r2, #8]
800108b8:	e5823010 	str	r3, [r2, #16]
800108bc:	e582100c 	str	r1, [r2, #12]
800108c0:	e5143064 	ldr	r3, [r4, #-100]	; 0x64
800108c4:	e5141118 	ldr	r1, [r4, #-280]	; 0x118
800108c8:	e282001c 	add	r0, r2, #28
800108cc:	e5821014 	str	r1, [r2, #20]
800108d0:	e582c018 	str	ip, [r2, #24]
800108d4:	e5931000 	ldr	r1, [r3]
800108d8:	e3a020ff 	mov	r2, #255	; 0xff
800108dc:	ebffe920 	bl	8000ad64 <strncpy>
800108e0:	e1540009 	cmp	r4, r9
800108e4:	e2877001 	add	r7, r7, #1
800108e8:	1affffdc 	bne	80010860 <get_procs+0xc8>
800108ec:	e1a0000a 	mov	r0, sl
800108f0:	e5857000 	str	r7, [r5]
800108f4:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
800108f8:	e3a00000 	mov	r0, #0
800108fc:	e89daff8 	ldm	sp, {r3, r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
80010900:	0000b850 	andeq	fp, r0, r0, asr r8
80010904:	00000000 	andeq	r0, r0, r0
80010908:	00017844 	andeq	r7, r1, r4, asr #16
8001090c:	000177c0 	andeq	r7, r1, r0, asr #15

80010910 <renew_sleep_counter>:
80010910:	e1a0c00d 	mov	ip, sp
80010914:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
80010918:	e59f4078 	ldr	r4, [pc, #120]	; 80010998 <renew_sleep_counter+0x88>
8001091c:	e24cb004 	sub	fp, ip, #4
80010920:	e08f4004 	add	r4, pc, r4
80010924:	e1a08000 	mov	r8, r0
80010928:	e3a09000 	mov	r9, #0
8001092c:	e2845a09 	add	r5, r4, #36864	; 0x9000
80010930:	e3a06000 	mov	r6, #0
80010934:	e3a07000 	mov	r7, #0
80010938:	ea000002 	b	80010948 <renew_sleep_counter+0x38>
8001093c:	e2844e12 	add	r4, r4, #288	; 0x120
80010940:	e1540005 	cmp	r4, r5
80010944:	0a000012 	beq	80010994 <renew_sleep_counter+0x84>
80010948:	e594300c 	ldr	r3, [r4, #12]
8001094c:	e3530002 	cmp	r3, #2
80010950:	1afffff9 	bne	8001093c <renew_sleep_counter+0x2c>
80010954:	e1c422d0 	ldrd	r2, [r4, #32]
80010958:	e3520001 	cmp	r2, #1
8001095c:	e2d31000 	sbcs	r1, r3, #0
80010960:	bafffff5 	blt	8001093c <renew_sleep_counter+0x2c>
80010964:	e0522008 	subs	r2, r2, r8
80010968:	e0c33009 	sbc	r3, r3, r9
8001096c:	e3520001 	cmp	r2, #1
80010970:	e2d31000 	sbcs	r1, r3, #0
80010974:	e1c422f0 	strd	r2, [r4, #32]
80010978:	aaffffef 	bge	8001093c <renew_sleep_counter+0x2c>
8001097c:	e1c462f0 	strd	r6, [r4, #32]
80010980:	e1a00004 	mov	r0, r4
80010984:	e2844e12 	add	r4, r4, #288	; 0x120
80010988:	ebfffabd 	bl	8000f484 <proc_ready>
8001098c:	e1540005 	cmp	r4, r5
80010990:	1affffec 	bne	80010948 <renew_sleep_counter+0x38>
80010994:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
80010998:	000176d8 	ldrdeq	r7, [r1], -r8

8001099c <proc_ipc_init>:
8001099c:	e59f300c 	ldr	r3, [pc, #12]	; 800109b0 <proc_ipc_init+0x14>
800109a0:	e3a02000 	mov	r2, #0
800109a4:	e08f3003 	add	r3, pc, r3
800109a8:	e5832000 	str	r2, [r3]
800109ac:	e12fff1e 	bx	lr
800109b0:	0022365c 	eoreq	r3, r2, ip, asr r6

800109b4 <proc_send_msg>:
800109b4:	e1a0c00d 	mov	ip, sp
800109b8:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
800109bc:	e59f7100 	ldr	r7, [pc, #256]	; 80010ac4 <proc_send_msg+0x110>
800109c0:	e24cb004 	sub	fp, ip, #4
800109c4:	e1a06001 	mov	r6, r1
800109c8:	e1a09002 	mov	r9, r2
800109cc:	e1a08000 	mov	r8, r0
800109d0:	ebfffb69 	bl	8000f77c <proc_get>
800109d4:	e08f7007 	add	r7, pc, r7
800109d8:	e2505000 	subs	r5, r0, #0
800109dc:	0a000029 	beq	80010a88 <proc_send_msg+0xd4>
800109e0:	e595300c 	ldr	r3, [r5, #12]
800109e4:	e3530000 	cmp	r3, #0
800109e8:	0a000026 	beq	80010a88 <proc_send_msg+0xd4>
800109ec:	e3a00020 	mov	r0, #32
800109f0:	ebfff3dc 	bl	8000d968 <kmalloc>
800109f4:	e2504000 	subs	r4, r0, #0
800109f8:	0a000022 	beq	80010a88 <proc_send_msg+0xd4>
800109fc:	e3a01000 	mov	r1, #0
80010a00:	e3a02020 	mov	r2, #32
80010a04:	ebffe899 	bl	8000ac70 <memset>
80010a08:	e59530c4 	ldr	r3, [r5, #196]	; 0xc4
80010a0c:	e3530000 	cmp	r3, #0
80010a10:	15834018 	strne	r4, [r3, #24]
80010a14:	058540c0 	streq	r4, [r5, #192]	; 0xc0
80010a18:	1584301c 	strne	r3, [r4, #28]
80010a1c:	e3590000 	cmp	r9, #0
80010a20:	a3a03001 	movge	r3, #1
80010a24:	e58540c4 	str	r4, [r5, #196]	; 0xc4
80010a28:	a5849000 	strge	r9, [r4]
80010a2c:	a5843004 	strge	r3, [r4, #4]
80010a30:	ba000016 	blt	80010a90 <proc_send_msg+0xdc>
80010a34:	e59f308c 	ldr	r3, [pc, #140]	; 80010ac8 <proc_send_msg+0x114>
80010a38:	e5962000 	ldr	r2, [r6]
80010a3c:	e7973003 	ldr	r3, [r7, r3]
80010a40:	e5933000 	ldr	r3, [r3]
80010a44:	e5933004 	ldr	r3, [r3, #4]
80010a48:	e5842010 	str	r2, [r4, #16]
80010a4c:	e584800c 	str	r8, [r4, #12]
80010a50:	e5843008 	str	r3, [r4, #8]
80010a54:	e5960000 	ldr	r0, [r6]
80010a58:	ebfff3c2 	bl	8000d968 <kmalloc>
80010a5c:	e3500000 	cmp	r0, #0
80010a60:	e1a07000 	mov	r7, r0
80010a64:	e5840014 	str	r0, [r4, #20]
80010a68:	0a000011 	beq	80010ab4 <proc_send_msg+0x100>
80010a6c:	e5961004 	ldr	r1, [r6, #4]
80010a70:	e5962000 	ldr	r2, [r6]
80010a74:	ebffe861 	bl	8000ac00 <memcpy>
80010a78:	e2850004 	add	r0, r5, #4
80010a7c:	ebfffdf4 	bl	80010254 <proc_wakeup>
80010a80:	e1a00004 	mov	r0, r4
80010a84:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
80010a88:	e3a00000 	mov	r0, #0
80010a8c:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
80010a90:	e59f3034 	ldr	r3, [pc, #52]	; 80010acc <proc_send_msg+0x118>
80010a94:	e3a01000 	mov	r1, #0
80010a98:	e08f3003 	add	r3, pc, r3
80010a9c:	e5932000 	ldr	r2, [r3]
80010aa0:	e5841004 	str	r1, [r4, #4]
80010aa4:	e2821001 	add	r1, r2, #1
80010aa8:	e5842000 	str	r2, [r4]
80010aac:	e5831000 	str	r1, [r3]
80010ab0:	eaffffdf 	b	80010a34 <proc_send_msg+0x80>
80010ab4:	e1a00004 	mov	r0, r4
80010ab8:	ebfff3bd 	bl	8000d9b4 <kfree>
80010abc:	e1a00007 	mov	r0, r7
80010ac0:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
80010ac4:	0000b628 	andeq	fp, r0, r8, lsr #12
80010ac8:	00000000 	andeq	r0, r0, r0
80010acc:	00223568 	eoreq	r3, r2, r8, ror #10

80010ad0 <proc_get_msg>:
80010ad0:	e1a0c00d 	mov	ip, sp
80010ad4:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
80010ad8:	e59f30f8 	ldr	r3, [pc, #248]	; 80010bd8 <proc_get_msg+0x108>
80010adc:	e24cb004 	sub	fp, ip, #4
80010ae0:	e59fc0f4 	ldr	ip, [pc, #244]	; 80010bdc <proc_get_msg+0x10c>
80010ae4:	e3520000 	cmp	r2, #0
80010ae8:	e08fc00c 	add	ip, pc, ip
80010aec:	e79c5003 	ldr	r5, [ip, r3]
80010af0:	e1a06001 	mov	r6, r1
80010af4:	e5953000 	ldr	r3, [r5]
80010af8:	e59340c0 	ldr	r4, [r3, #192]	; 0xc0
80010afc:	aa000004 	bge	80010b14 <proc_get_msg+0x44>
80010b00:	ea000008 	b	80010b28 <proc_get_msg+0x58>
80010b04:	e5943000 	ldr	r3, [r4]
80010b08:	e1520003 	cmp	r2, r3
80010b0c:	0a00000e 	beq	80010b4c <proc_get_msg+0x7c>
80010b10:	e5944018 	ldr	r4, [r4, #24]
80010b14:	e3540000 	cmp	r4, #0
80010b18:	1afffff9 	bne	80010b04 <proc_get_msg+0x34>
80010b1c:	e3e05000 	mvn	r5, #0
80010b20:	e1a00005 	mov	r0, r5
80010b24:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80010b28:	e3540000 	cmp	r4, #0
80010b2c:	1a000003 	bne	80010b40 <proc_get_msg+0x70>
80010b30:	eafffff9 	b	80010b1c <proc_get_msg+0x4c>
80010b34:	e5944018 	ldr	r4, [r4, #24]
80010b38:	e3540000 	cmp	r4, #0
80010b3c:	0afffff6 	beq	80010b1c <proc_get_msg+0x4c>
80010b40:	e5943004 	ldr	r3, [r4, #4]
80010b44:	e3530000 	cmp	r3, #0
80010b48:	1afffff9 	bne	80010b34 <proc_get_msg+0x64>
80010b4c:	e3500000 	cmp	r0, #0
80010b50:	15943008 	ldrne	r3, [r4, #8]
80010b54:	15803000 	strne	r3, [r0]
80010b58:	e5943010 	ldr	r3, [r4, #16]
80010b5c:	e5863000 	str	r3, [r6]
80010b60:	e5940010 	ldr	r0, [r4, #16]
80010b64:	ebfffc76 	bl	8000fd44 <proc_malloc>
80010b68:	e3500000 	cmp	r0, #0
80010b6c:	e5860004 	str	r0, [r6, #4]
80010b70:	0a000002 	beq	80010b80 <proc_get_msg+0xb0>
80010b74:	e5941014 	ldr	r1, [r4, #20]
80010b78:	e5942010 	ldr	r2, [r4, #16]
80010b7c:	ebffe81f 	bl	8000ac00 <memcpy>
80010b80:	e5943018 	ldr	r3, [r4, #24]
80010b84:	e5952000 	ldr	r2, [r5]
80010b88:	e3530000 	cmp	r3, #0
80010b8c:	1594101c 	ldrne	r1, [r4, #28]
80010b90:	e5945000 	ldr	r5, [r4]
80010b94:	1583101c 	strne	r1, [r3, #28]
80010b98:	e594101c 	ldr	r1, [r4, #28]
80010b9c:	e3510000 	cmp	r1, #0
80010ba0:	15813018 	strne	r3, [r1, #24]
80010ba4:	e59230c0 	ldr	r3, [r2, #192]	; 0xc0
80010ba8:	e1540003 	cmp	r4, r3
80010bac:	05943018 	ldreq	r3, [r4, #24]
80010bb0:	058230c0 	streq	r3, [r2, #192]	; 0xc0
80010bb4:	e59230c4 	ldr	r3, [r2, #196]	; 0xc4
80010bb8:	e1540003 	cmp	r4, r3
80010bbc:	058210c4 	streq	r1, [r2, #196]	; 0xc4
80010bc0:	e5940014 	ldr	r0, [r4, #20]
80010bc4:	ebfff37a 	bl	8000d9b4 <kfree>
80010bc8:	e1a00004 	mov	r0, r4
80010bcc:	ebfff378 	bl	8000d9b4 <kfree>
80010bd0:	e1a00005 	mov	r0, r5
80010bd4:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80010bd8:	00000000 	andeq	r0, r0, r0
80010bdc:	0000b514 	andeq	fp, r0, r4, lsl r5

80010be0 <schedule>:
80010be0:	e1a0c00d 	mov	ip, sp
80010be4:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80010be8:	e24cb004 	sub	fp, ip, #4
80010bec:	e1a04000 	mov	r4, r0
80010bf0:	ebfffba4 	bl	8000fa88 <proc_get_next_ready>
80010bf4:	e2501000 	subs	r1, r0, #0
80010bf8:	089da818 	ldmeq	sp, {r3, r4, fp, sp, pc}
80010bfc:	e1a00004 	mov	r0, r4
80010c00:	e24bd014 	sub	sp, fp, #20
80010c04:	e89d6818 	ldm	sp, {r3, r4, fp, sp, lr}
80010c08:	eafffae5 	b	8000f7a4 <proc_switch>

80010c0c <vfs_simple_get>:
80010c0c:	e1a0c00d 	mov	ip, sp
80010c10:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
80010c14:	e2504000 	subs	r4, r0, #0
80010c18:	e24cb004 	sub	fp, ip, #4
80010c1c:	e1a05001 	mov	r5, r1
80010c20:	0a000012 	beq	80010c70 <vfs_simple_get+0x64>
80010c24:	e1a00001 	mov	r0, r1
80010c28:	e3a0102f 	mov	r1, #47	; 0x2f
80010c2c:	ebffe8aa 	bl	8000aedc <strchr>
80010c30:	e3500000 	cmp	r0, #0
80010c34:	1a00000d 	bne	80010c70 <vfs_simple_get+0x64>
80010c38:	e5944004 	ldr	r4, [r4, #4]
80010c3c:	e3540000 	cmp	r4, #0
80010c40:	1a000003 	bne	80010c54 <vfs_simple_get+0x48>
80010c44:	ea000009 	b	80010c70 <vfs_simple_get+0x64>
80010c48:	e594400c 	ldr	r4, [r4, #12]
80010c4c:	e3540000 	cmp	r4, #0
80010c50:	0a000006 	beq	80010c70 <vfs_simple_get+0x64>
80010c54:	e284001c 	add	r0, r4, #28
80010c58:	e1a01005 	mov	r1, r5
80010c5c:	ebffe864 	bl	8000adf4 <strcmp>
80010c60:	e3500000 	cmp	r0, #0
80010c64:	1afffff7 	bne	80010c48 <vfs_simple_get+0x3c>
80010c68:	e1a00004 	mov	r0, r4
80010c6c:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
80010c70:	e3a00000 	mov	r0, #0
80010c74:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

80010c78 <vfs_get_mount_by_id>:
80010c78:	e1a0c00d 	mov	ip, sp
80010c7c:	e2512000 	subs	r2, r1, #0
80010c80:	e92dd800 	push	{fp, ip, lr, pc}
80010c84:	e24cb004 	sub	fp, ip, #4
80010c88:	0a00000d 	beq	80010cc4 <vfs_get_mount_by_id+0x4c>
80010c8c:	e350001f 	cmp	r0, #31
80010c90:	8a00000b 	bhi	80010cc4 <vfs_get_mount_by_id+0x4c>
80010c94:	e59f1030 	ldr	r1, [pc, #48]	; 80010ccc <vfs_get_mount_by_id+0x54>
80010c98:	e0800300 	add	r0, r0, r0, lsl #6
80010c9c:	e08f1001 	add	r1, pc, r1
80010ca0:	e0811180 	add	r1, r1, r0, lsl #3
80010ca4:	e5913004 	ldr	r3, [r1, #4]
80010ca8:	e3530000 	cmp	r3, #0
80010cac:	0a000004 	beq	80010cc4 <vfs_get_mount_by_id+0x4c>
80010cb0:	e1a00002 	mov	r0, r2
80010cb4:	e3a02f82 	mov	r2, #520	; 0x208
80010cb8:	ebffe7d0 	bl	8000ac00 <memcpy>
80010cbc:	e3a00000 	mov	r0, #0
80010cc0:	e89da800 	ldm	sp, {fp, sp, pc}
80010cc4:	e3e00000 	mvn	r0, #0
80010cc8:	e89da800 	ldm	sp, {fp, sp, pc}
80010ccc:	00223368 	eoreq	r3, r2, r8, ror #6

80010cd0 <vfs_get_mount>:
80010cd0:	e1a0c00d 	mov	ip, sp
80010cd4:	e3510000 	cmp	r1, #0
80010cd8:	13500000 	cmpne	r0, #0
80010cdc:	e92dd800 	push	{fp, ip, lr, pc}
80010ce0:	e24cb004 	sub	fp, ip, #4
80010ce4:	0a000005 	beq	80010d00 <vfs_get_mount+0x30>
80010ce8:	e5903068 	ldr	r3, [r0, #104]	; 0x68
80010cec:	e3530000 	cmp	r3, #0
80010cf0:	aa000004 	bge	80010d08 <vfs_get_mount+0x38>
80010cf4:	e5900000 	ldr	r0, [r0]
80010cf8:	e3500000 	cmp	r0, #0
80010cfc:	1afffff9 	bne	80010ce8 <vfs_get_mount+0x18>
80010d00:	e3e00000 	mvn	r0, #0
80010d04:	e89da800 	ldm	sp, {fp, sp, pc}
80010d08:	e59f201c 	ldr	r2, [pc, #28]	; 80010d2c <vfs_get_mount+0x5c>
80010d0c:	e0833303 	add	r3, r3, r3, lsl #6
80010d10:	e08f2002 	add	r2, pc, r2
80010d14:	e1a00001 	mov	r0, r1
80010d18:	e0821183 	add	r1, r2, r3, lsl #3
80010d1c:	e3a02f82 	mov	r2, #520	; 0x208
80010d20:	ebffe7b6 	bl	8000ac00 <memcpy>
80010d24:	e3a00000 	mov	r0, #0
80010d28:	e89da800 	ldm	sp, {fp, sp, pc}
80010d2c:	002232f4 	strdeq	r3, [r2], -r4	; <UNPREDICTABLE>

80010d30 <get_mount_pid>:
80010d30:	e1a0c00d 	mov	ip, sp
80010d34:	e92dd800 	push	{fp, ip, lr, pc}
80010d38:	e24cb004 	sub	fp, ip, #4
80010d3c:	e24ddf82 	sub	sp, sp, #520	; 0x208
80010d40:	e24b1f85 	sub	r1, fp, #532	; 0x214
80010d44:	ebffffe1 	bl	80010cd0 <vfs_get_mount>
80010d48:	e3500000 	cmp	r0, #0
80010d4c:	051b0214 	ldreq	r0, [fp, #-532]	; 0x214
80010d50:	13e00000 	mvnne	r0, #0
80010d54:	e24bd00c 	sub	sp, fp, #12
80010d58:	e89da800 	ldm	sp, {fp, sp, pc}

80010d5c <vfs_add>:
80010d5c:	e1a0c00d 	mov	ip, sp
80010d60:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
80010d64:	e59f3084 	ldr	r3, [pc, #132]	; 80010df0 <vfs_add+0x94>
80010d68:	e3510000 	cmp	r1, #0
80010d6c:	13500000 	cmpne	r0, #0
80010d70:	e08f3003 	add	r3, pc, r3
80010d74:	e24cb004 	sub	fp, ip, #4
80010d78:	e1a05001 	mov	r5, r1
80010d7c:	e1a04000 	mov	r4, r0
80010d80:	0a000016 	beq	80010de0 <vfs_add+0x84>
80010d84:	e59f2068 	ldr	r2, [pc, #104]	; 80010df4 <vfs_add+0x98>
80010d88:	e7936002 	ldr	r6, [r3, r2]
80010d8c:	e5963000 	ldr	r3, [r6]
80010d90:	e5933010 	ldr	r3, [r3, #16]
80010d94:	e3530000 	cmp	r3, #0
80010d98:	da000004 	ble	80010db0 <vfs_add+0x54>
80010d9c:	ebffffe3 	bl	80010d30 <get_mount_pid>
80010da0:	e5963000 	ldr	r3, [r6]
80010da4:	e5933004 	ldr	r3, [r3, #4]
80010da8:	e1500003 	cmp	r0, r3
80010dac:	1a00000b 	bne	80010de0 <vfs_add+0x84>
80010db0:	e5943008 	ldr	r3, [r4, #8]
80010db4:	e5854000 	str	r4, [r5]
80010db8:	e3530000 	cmp	r3, #0
80010dbc:	1583500c 	strne	r5, [r3, #12]
80010dc0:	15853010 	strne	r3, [r5, #16]
80010dc4:	0a000007 	beq	80010de8 <vfs_add+0x8c>
80010dc8:	e5943014 	ldr	r3, [r4, #20]
80010dcc:	e5845008 	str	r5, [r4, #8]
80010dd0:	e2833001 	add	r3, r3, #1
80010dd4:	e5843014 	str	r3, [r4, #20]
80010dd8:	e3a00000 	mov	r0, #0
80010ddc:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80010de0:	e3e00000 	mvn	r0, #0
80010de4:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80010de8:	e5845004 	str	r5, [r4, #4]
80010dec:	eafffff5 	b	80010dc8 <vfs_add+0x6c>
80010df0:	0000b28c 	andeq	fp, r0, ip, lsl #5
80010df4:	00000000 	andeq	r0, r0, r0

80010df8 <vfs_remove>:
80010df8:	e59f30ac 	ldr	r3, [pc, #172]	; 80010eac <vfs_remove+0xb4>
80010dfc:	e1a0c00d 	mov	ip, sp
80010e00:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
80010e04:	e2504000 	subs	r4, r0, #0
80010e08:	e24cb004 	sub	fp, ip, #4
80010e0c:	e08f3003 	add	r3, pc, r3
80010e10:	089da830 	ldmeq	sp, {r4, r5, fp, sp, pc}
80010e14:	e59f2094 	ldr	r2, [pc, #148]	; 80010eb0 <vfs_remove+0xb8>
80010e18:	e7935002 	ldr	r5, [r3, r2]
80010e1c:	e5953000 	ldr	r3, [r5]
80010e20:	e5933010 	ldr	r3, [r3, #16]
80010e24:	e3530000 	cmp	r3, #0
80010e28:	da000004 	ble	80010e40 <vfs_remove+0x48>
80010e2c:	ebffffbf 	bl	80010d30 <get_mount_pid>
80010e30:	e5953000 	ldr	r3, [r5]
80010e34:	e5933004 	ldr	r3, [r3, #4]
80010e38:	e1500003 	cmp	r0, r3
80010e3c:	1a000019 	bne	80010ea8 <vfs_remove+0xb0>
80010e40:	e5943000 	ldr	r3, [r4]
80010e44:	e3530000 	cmp	r3, #0
80010e48:	0a000013 	beq	80010e9c <vfs_remove+0xa4>
80010e4c:	e5932004 	ldr	r2, [r3, #4]
80010e50:	e594100c 	ldr	r1, [r4, #12]
80010e54:	e1540002 	cmp	r4, r2
80010e58:	e5932008 	ldr	r2, [r3, #8]
80010e5c:	e5930014 	ldr	r0, [r3, #20]
80010e60:	05831004 	streq	r1, [r3, #4]
80010e64:	e1540002 	cmp	r4, r2
80010e68:	e5942010 	ldr	r2, [r4, #16]
80010e6c:	e2400001 	sub	r0, r0, #1
80010e70:	05832008 	streq	r2, [r3, #8]
80010e74:	e5830014 	str	r0, [r3, #20]
80010e78:	e3510000 	cmp	r1, #0
80010e7c:	15812010 	strne	r2, [r1, #16]
80010e80:	15942010 	ldrne	r2, [r4, #16]
80010e84:	e3a03000 	mov	r3, #0
80010e88:	e3520000 	cmp	r2, #0
80010e8c:	1582100c 	strne	r1, [r2, #12]
80010e90:	e5843010 	str	r3, [r4, #16]
80010e94:	e584300c 	str	r3, [r4, #12]
80010e98:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
80010e9c:	e594100c 	ldr	r1, [r4, #12]
80010ea0:	e5942010 	ldr	r2, [r4, #16]
80010ea4:	eafffff3 	b	80010e78 <vfs_remove+0x80>
80010ea8:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
80010eac:	0000b1f0 	strdeq	fp, [r0], -r0
80010eb0:	00000000 	andeq	r0, r0, r0

80010eb4 <vfs_mount>:
80010eb4:	e1a0c00d 	mov	ip, sp
80010eb8:	e92ddff0 	push	{r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
80010ebc:	e24cb004 	sub	fp, ip, #4
80010ec0:	e59fc18c 	ldr	ip, [pc, #396]	; 80011054 <vfs_mount+0x1a0>
80010ec4:	e3510000 	cmp	r1, #0
80010ec8:	13500000 	cmpne	r0, #0
80010ecc:	e24dd00c 	sub	sp, sp, #12
80010ed0:	e08fc00c 	add	ip, pc, ip
80010ed4:	03a05001 	moveq	r5, #1
80010ed8:	13a05000 	movne	r5, #0
80010edc:	e1a0a000 	mov	sl, r0
80010ee0:	e1a09001 	mov	r9, r1
80010ee4:	0a000057 	beq	80011048 <vfs_mount+0x194>
80010ee8:	e5913068 	ldr	r3, [r1, #104]	; 0x68
80010eec:	e3530000 	cmp	r3, #0
80010ef0:	ca000054 	bgt	80011048 <vfs_mount+0x194>
80010ef4:	e59f315c 	ldr	r3, [pc, #348]	; 80011058 <vfs_mount+0x1a4>
80010ef8:	e08f3003 	add	r3, pc, r3
80010efc:	ea000002 	b	80010f0c <vfs_mount+0x58>
80010f00:	e2855001 	add	r5, r5, #1
80010f04:	e3550020 	cmp	r5, #32
80010f08:	0a00004e 	beq	80011048 <vfs_mount+0x194>
80010f0c:	e5932004 	ldr	r2, [r3, #4]
80010f10:	e2833f82 	add	r3, r3, #520	; 0x208
80010f14:	e3520000 	cmp	r2, #0
80010f18:	1afffff8 	bne	80010f00 <vfs_mount+0x4c>
80010f1c:	e59f3138 	ldr	r3, [pc, #312]	; 8001105c <vfs_mount+0x1a8>
80010f20:	e59f2138 	ldr	r2, [pc, #312]	; 80011060 <vfs_mount+0x1ac>
80010f24:	e79c1003 	ldr	r1, [ip, r3]
80010f28:	e0853305 	add	r3, r5, r5, lsl #6
80010f2c:	e5911000 	ldr	r1, [r1]
80010f30:	e08f2002 	add	r2, pc, r2
80010f34:	e591c004 	ldr	ip, [r1, #4]
80010f38:	e1a03183 	lsl	r3, r3, #3
80010f3c:	e59f0120 	ldr	r0, [pc, #288]	; 80011064 <vfs_mount+0x1b0>
80010f40:	e0821003 	add	r1, r2, r3
80010f44:	e581a004 	str	sl, [r1, #4]
80010f48:	e2831008 	add	r1, r3, #8
80010f4c:	e08f0000 	add	r0, pc, r0
80010f50:	e782c003 	str	ip, [r2, r3]
80010f54:	e0823001 	add	r3, r2, r1
80010f58:	e50b3030 	str	r3, [fp, #-48]	; 0x30
80010f5c:	e1a08000 	mov	r8, r0
80010f60:	ebffeb27 	bl	8000bc04 <str_new>
80010f64:	e1a0400a 	mov	r4, sl
80010f68:	e1a07000 	mov	r7, r0
80010f6c:	e1a00008 	mov	r0, r8
80010f70:	ebffeb23 	bl	8000bc04 <str_new>
80010f74:	e284101c 	add	r1, r4, #28
80010f78:	e1a06000 	mov	r6, r0
80010f7c:	ebffeb17 	bl	8000bbe0 <str_cpy>
80010f80:	e5970000 	ldr	r0, [r7]
80010f84:	ebffe83b 	bl	8000b078 <strlen>
80010f88:	e3500000 	cmp	r0, #0
80010f8c:	0a000008 	beq	80010fb4 <vfs_mount+0x100>
80010f90:	e5d4301c 	ldrb	r3, [r4, #28]
80010f94:	e3a0102f 	mov	r1, #47	; 0x2f
80010f98:	e1530001 	cmp	r3, r1
80010f9c:	e1a00006 	mov	r0, r6
80010fa0:	0a000000 	beq	80010fa8 <vfs_mount+0xf4>
80010fa4:	ebffeb60 	bl	8000bd2c <str_addc>
80010fa8:	e1a00006 	mov	r0, r6
80010fac:	e5971000 	ldr	r1, [r7]
80010fb0:	ebffeb33 	bl	8000bc84 <str_add>
80010fb4:	e5961000 	ldr	r1, [r6]
80010fb8:	e1a00007 	mov	r0, r7
80010fbc:	ebffeb07 	bl	8000bbe0 <str_cpy>
80010fc0:	e1a00006 	mov	r0, r6
80010fc4:	ebffeb77 	bl	8000bda8 <str_free>
80010fc8:	e5944000 	ldr	r4, [r4]
80010fcc:	e3540000 	cmp	r4, #0
80010fd0:	1affffe5 	bne	80010f6c <vfs_mount+0xb8>
80010fd4:	e59f808c 	ldr	r8, [pc, #140]	; 80011068 <vfs_mount+0x1b4>
80010fd8:	e59f208c 	ldr	r2, [pc, #140]	; 8001106c <vfs_mount+0x1b8>
80010fdc:	e08f8008 	add	r8, pc, r8
80010fe0:	e2886e11 	add	r6, r8, #272	; 0x110
80010fe4:	e5971000 	ldr	r1, [r7]
80010fe8:	e1a00006 	mov	r0, r6
80010fec:	ebffe75c 	bl	8000ad64 <strncpy>
80010ff0:	e1a00007 	mov	r0, r7
80010ff4:	ebffeb6b 	bl	8000bda8 <str_free>
80010ff8:	e1a01006 	mov	r1, r6
80010ffc:	e51b0030 	ldr	r0, [fp, #-48]	; 0x30
80011000:	ebffe74a 	bl	8000ad30 <strcpy>
80011004:	e289001c 	add	r0, r9, #28
80011008:	e28a101c 	add	r1, sl, #28
8001100c:	ebffe747 	bl	8000ad30 <strcpy>
80011010:	e59a6000 	ldr	r6, [sl]
80011014:	e5895068 	str	r5, [r9, #104]	; 0x68
80011018:	e3560000 	cmp	r6, #0
8001101c:	05889310 	streq	r9, [r8, #784]	; 0x310
80011020:	01a00004 	moveq	r0, r4
80011024:	0a000005 	beq	80011040 <vfs_mount+0x18c>
80011028:	e1a0000a 	mov	r0, sl
8001102c:	ebffff71 	bl	80010df8 <vfs_remove>
80011030:	e1a00006 	mov	r0, r6
80011034:	e1a01009 	mov	r1, r9
80011038:	ebffff47 	bl	80010d5c <vfs_add>
8001103c:	e1a00004 	mov	r0, r4
80011040:	e24bd028 	sub	sp, fp, #40	; 0x28
80011044:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
80011048:	e3e00000 	mvn	r0, #0
8001104c:	e24bd028 	sub	sp, fp, #40	; 0x28
80011050:	e89daff0 	ldm	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, pc}
80011054:	0000b12c 	andeq	fp, r0, ip, lsr #2
80011058:	0022310c 	eoreq	r3, r2, ip, lsl #2
8001105c:	00000000 	andeq	r0, r0, r0
80011060:	002230d4 	ldrdeq	r3, [r2], -r4	; <UNPREDICTABLE>
80011064:	000010fc 	strdeq	r1, [r0], -ip
80011068:	00227018 	eoreq	r7, r2, r8, lsl r0
8001106c:	000001ff 	strdeq	r0, [r0], -pc	; <UNPREDICTABLE>

80011070 <vfs_umount>:
80011070:	e59f20a8 	ldr	r2, [pc, #168]	; 80011120 <vfs_umount+0xb0>
80011074:	e1a0c00d 	mov	ip, sp
80011078:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8001107c:	e2504000 	subs	r4, r0, #0
80011080:	e24cb004 	sub	fp, ip, #4
80011084:	e08f2002 	add	r2, pc, r2
80011088:	089da878 	ldmeq	sp, {r3, r4, r5, r6, fp, sp, pc}
8001108c:	e5943068 	ldr	r3, [r4, #104]	; 0x68
80011090:	e3530000 	cmp	r3, #0
80011094:	b89da878 	ldmlt	sp, {r3, r4, r5, r6, fp, sp, pc}
80011098:	e59f1084 	ldr	r1, [pc, #132]	; 80011124 <vfs_umount+0xb4>
8001109c:	e7925001 	ldr	r5, [r2, r1]
800110a0:	e5952000 	ldr	r2, [r5]
800110a4:	e5922010 	ldr	r2, [r2, #16]
800110a8:	e3520000 	cmp	r2, #0
800110ac:	da000005 	ble	800110c8 <vfs_umount+0x58>
800110b0:	ebffff1e 	bl	80010d30 <get_mount_pid>
800110b4:	e5953000 	ldr	r3, [r5]
800110b8:	e5933004 	ldr	r3, [r3, #4]
800110bc:	e1500003 	cmp	r0, r3
800110c0:	189da878 	ldmne	sp, {r3, r4, r5, r6, fp, sp, pc}
800110c4:	e5943068 	ldr	r3, [r4, #104]	; 0x68
800110c8:	e59f2058 	ldr	r2, [pc, #88]	; 80011128 <vfs_umount+0xb8>
800110cc:	e0833303 	add	r3, r3, r3, lsl #6
800110d0:	e08f2002 	add	r2, pc, r2
800110d4:	e0823183 	add	r3, r2, r3, lsl #3
800110d8:	e5935004 	ldr	r5, [r3, #4]
800110dc:	e3550000 	cmp	r5, #0
800110e0:	0a00000d 	beq	8001111c <vfs_umount+0xac>
800110e4:	e5946000 	ldr	r6, [r4]
800110e8:	e3560000 	cmp	r6, #0
800110ec:	0a000006 	beq	8001110c <vfs_umount+0x9c>
800110f0:	e1a00004 	mov	r0, r4
800110f4:	ebffff3f 	bl	80010df8 <vfs_remove>
800110f8:	e1a00006 	mov	r0, r6
800110fc:	e1a01005 	mov	r1, r5
80011100:	e24bd01c 	sub	sp, fp, #28
80011104:	e89d6878 	ldm	sp, {r3, r4, r5, r6, fp, sp, lr}
80011108:	eaffff13 	b	80010d5c <vfs_add>
8001110c:	e59f3018 	ldr	r3, [pc, #24]	; 8001112c <vfs_umount+0xbc>
80011110:	e08f3003 	add	r3, pc, r3
80011114:	e5835310 	str	r5, [r3, #784]	; 0x310
80011118:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8001111c:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80011120:	0000af78 	andeq	sl, r0, r8, ror pc
80011124:	00000000 	andeq	r0, r0, r0
80011128:	00222f34 	eoreq	r2, r2, r4, lsr pc
8001112c:	00226ee4 	eoreq	r6, r2, r4, ror #29

80011130 <vfs_del>:
80011130:	e59f30d4 	ldr	r3, [pc, #212]	; 8001120c <vfs_del+0xdc>
80011134:	e1a0c00d 	mov	ip, sp
80011138:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
8001113c:	e2505000 	subs	r5, r0, #0
80011140:	e24cb004 	sub	fp, ip, #4
80011144:	e08f3003 	add	r3, pc, r3
80011148:	0a00002d 	beq	80011204 <vfs_del+0xd4>
8001114c:	e5952070 	ldr	r2, [r5, #112]	; 0x70
80011150:	e3520000 	cmp	r2, #0
80011154:	1a00002a 	bne	80011204 <vfs_del+0xd4>
80011158:	e59f20b0 	ldr	r2, [pc, #176]	; 80011210 <vfs_del+0xe0>
8001115c:	e7934002 	ldr	r4, [r3, r2]
80011160:	e5943000 	ldr	r3, [r4]
80011164:	e5933010 	ldr	r3, [r3, #16]
80011168:	e3530000 	cmp	r3, #0
8001116c:	da000004 	ble	80011184 <vfs_del+0x54>
80011170:	ebfffeee 	bl	80010d30 <get_mount_pid>
80011174:	e5943000 	ldr	r3, [r4]
80011178:	e5933004 	ldr	r3, [r3, #4]
8001117c:	e1500003 	cmp	r0, r3
80011180:	1a00001f 	bne	80011204 <vfs_del+0xd4>
80011184:	e5950004 	ldr	r0, [r5, #4]
80011188:	e3500000 	cmp	r0, #0
8001118c:	0a000003 	beq	800111a0 <vfs_del+0x70>
80011190:	e590400c 	ldr	r4, [r0, #12]
80011194:	ebffffe5 	bl	80011130 <vfs_del>
80011198:	e2540000 	subs	r0, r4, #0
8001119c:	1afffffb 	bne	80011190 <vfs_del+0x60>
800111a0:	e5953000 	ldr	r3, [r5]
800111a4:	e3530000 	cmp	r3, #0
800111a8:	0a00000a 	beq	800111d8 <vfs_del+0xa8>
800111ac:	e5932004 	ldr	r2, [r3, #4]
800111b0:	e1520005 	cmp	r2, r5
800111b4:	0595200c 	ldreq	r2, [r5, #12]
800111b8:	05832004 	streq	r2, [r3, #4]
800111bc:	e5932008 	ldr	r2, [r3, #8]
800111c0:	e1520005 	cmp	r2, r5
800111c4:	05952010 	ldreq	r2, [r5, #16]
800111c8:	05832008 	streq	r2, [r3, #8]
800111cc:	e5932014 	ldr	r2, [r3, #20]
800111d0:	e2422001 	sub	r2, r2, #1
800111d4:	e5832014 	str	r2, [r3, #20]
800111d8:	e595300c 	ldr	r3, [r5, #12]
800111dc:	e1a00005 	mov	r0, r5
800111e0:	e3530000 	cmp	r3, #0
800111e4:	15952010 	ldrne	r2, [r5, #16]
800111e8:	15832010 	strne	r2, [r3, #16]
800111ec:	e5952010 	ldr	r2, [r5, #16]
800111f0:	e3520000 	cmp	r2, #0
800111f4:	1582300c 	strne	r3, [r2, #12]
800111f8:	ebfff1ed 	bl	8000d9b4 <kfree>
800111fc:	e3a00000 	mov	r0, #0
80011200:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
80011204:	e3e00000 	mvn	r0, #0
80011208:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
8001120c:	0000aeb8 			; <UNDEFINED> instruction: 0x0000aeb8
80011210:	00000000 	andeq	r0, r0, r0

80011214 <vfs_set>:
80011214:	e1a0c00d 	mov	ip, sp
80011218:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
8001121c:	e59f3064 	ldr	r3, [pc, #100]	; 80011288 <vfs_set+0x74>
80011220:	e3510000 	cmp	r1, #0
80011224:	13500000 	cmpne	r0, #0
80011228:	e08f3003 	add	r3, pc, r3
8001122c:	e24cb004 	sub	fp, ip, #4
80011230:	e1a05001 	mov	r5, r1
80011234:	e1a06000 	mov	r6, r0
80011238:	0a000010 	beq	80011280 <vfs_set+0x6c>
8001123c:	e59f2048 	ldr	r2, [pc, #72]	; 8001128c <vfs_set+0x78>
80011240:	e7934002 	ldr	r4, [r3, r2]
80011244:	e5943000 	ldr	r3, [r4]
80011248:	e5933010 	ldr	r3, [r3, #16]
8001124c:	e3530000 	cmp	r3, #0
80011250:	da000004 	ble	80011268 <vfs_set+0x54>
80011254:	ebfffeb5 	bl	80010d30 <get_mount_pid>
80011258:	e5943000 	ldr	r3, [r4]
8001125c:	e5933004 	ldr	r3, [r3, #4]
80011260:	e1500003 	cmp	r0, r3
80011264:	1a000005 	bne	80011280 <vfs_set+0x6c>
80011268:	e2860018 	add	r0, r6, #24
8001126c:	e1a01005 	mov	r1, r5
80011270:	e3a02058 	mov	r2, #88	; 0x58
80011274:	ebffe661 	bl	8000ac00 <memcpy>
80011278:	e3a00000 	mov	r0, #0
8001127c:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80011280:	e3e00000 	mvn	r0, #0
80011284:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80011288:	0000add4 	ldrdeq	sl, [r0], -r4
8001128c:	00000000 	andeq	r0, r0, r0

80011290 <vfs_new_node>:
80011290:	e1a0c00d 	mov	ip, sp
80011294:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80011298:	e3a00078 	mov	r0, #120	; 0x78
8001129c:	e24cb004 	sub	fp, ip, #4
800112a0:	ebfff1b0 	bl	8000d968 <kmalloc>
800112a4:	e3a01000 	mov	r1, #0
800112a8:	e3a02078 	mov	r2, #120	; 0x78
800112ac:	e1a04000 	mov	r4, r0
800112b0:	ebffe66e 	bl	8000ac70 <memset>
800112b4:	e3e03000 	mvn	r3, #0
800112b8:	e5844018 	str	r4, [r4, #24]
800112bc:	e5843068 	str	r3, [r4, #104]	; 0x68
800112c0:	e1a00004 	mov	r0, r4
800112c4:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}

800112c8 <vfs_init>:
800112c8:	e1a0c00d 	mov	ip, sp
800112cc:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
800112d0:	e59f4050 	ldr	r4, [pc, #80]	; 80011328 <vfs_init+0x60>
800112d4:	e24cb004 	sub	fp, ip, #4
800112d8:	e08f4004 	add	r4, pc, r4
800112dc:	e2845c41 	add	r5, r4, #16640	; 0x4100
800112e0:	e1a00004 	mov	r0, r4
800112e4:	e3a01000 	mov	r1, #0
800112e8:	e2844f82 	add	r4, r4, #520	; 0x208
800112ec:	e3a02f82 	mov	r2, #520	; 0x208
800112f0:	ebffe65e 	bl	8000ac70 <memset>
800112f4:	e1540005 	cmp	r4, r5
800112f8:	1afffff8 	bne	800112e0 <vfs_init+0x18>
800112fc:	ebffffe3 	bl	80011290 <vfs_new_node>
80011300:	e59f3024 	ldr	r3, [pc, #36]	; 8001132c <vfs_init+0x64>
80011304:	e59f1024 	ldr	r1, [pc, #36]	; 80011330 <vfs_init+0x68>
80011308:	e08f3003 	add	r3, pc, r3
8001130c:	e08f1001 	add	r1, pc, r1
80011310:	e1a02000 	mov	r2, r0
80011314:	e280001c 	add	r0, r0, #28
80011318:	e5832310 	str	r2, [r3, #784]	; 0x310
8001131c:	e24bd014 	sub	sp, fp, #20
80011320:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
80011324:	eaffe681 	b	8000ad30 <strcpy>
80011328:	00222d2c 	eoreq	r2, r2, ip, lsr #26
8001132c:	00226cec 	eoreq	r6, r2, ip, ror #25
80011330:	00000d20 	andeq	r0, r0, r0, lsr #26

80011334 <vfs_get>:
80011334:	e1a0c00d 	mov	ip, sp
80011338:	e92ddbf0 	push	{r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
8001133c:	e3500000 	cmp	r0, #0
80011340:	e24cb004 	sub	fp, ip, #4
80011344:	e24ddf82 	sub	sp, sp, #520	; 0x208
80011348:	0a000019 	beq	800113b4 <vfs_get+0x80>
8001134c:	e5d13000 	ldrb	r3, [r1]
80011350:	e353002f 	cmp	r3, #47	; 0x2f
80011354:	0a00001a 	beq	800113c4 <vfs_get+0x90>
80011358:	e3a05000 	mov	r5, #0
8001135c:	e24b8f8a 	sub	r8, fp, #552	; 0x228
80011360:	e1a06001 	mov	r6, r1
80011364:	e2484001 	sub	r4, r8, #1
80011368:	e1a01005 	mov	r1, r5
8001136c:	e1a09005 	mov	r9, r5
80011370:	e24b7029 	sub	r7, fp, #41	; 0x29
80011374:	ea000001 	b	80011380 <vfs_get+0x4c>
80011378:	e1540007 	cmp	r4, r7
8001137c:	0a00000c 	beq	800113b4 <vfs_get+0x80>
80011380:	e4d63001 	ldrb	r3, [r6], #1
80011384:	e2855001 	add	r5, r5, #1
80011388:	e3530000 	cmp	r3, #0
8001138c:	e5e43001 	strb	r3, [r4, #1]!
80011390:	0a000013 	beq	800113e4 <vfs_get+0xb0>
80011394:	e353002f 	cmp	r3, #47	; 0x2f
80011398:	1afffff6 	bne	80011378 <vfs_get+0x44>
8001139c:	e0881001 	add	r1, r8, r1
800113a0:	e5c49000 	strb	r9, [r4]
800113a4:	ebfffe18 	bl	80010c0c <vfs_simple_get>
800113a8:	e1a01005 	mov	r1, r5
800113ac:	e3500000 	cmp	r0, #0
800113b0:	1afffff0 	bne	80011378 <vfs_get+0x44>
800113b4:	e3a00000 	mov	r0, #0
800113b8:	e24bd024 	sub	sp, fp, #36	; 0x24
800113bc:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
800113c0:	e1a00003 	mov	r0, r3
800113c4:	e5903000 	ldr	r3, [r0]
800113c8:	e3530000 	cmp	r3, #0
800113cc:	1afffffb 	bne	800113c0 <vfs_get+0x8c>
800113d0:	e5d13001 	ldrb	r3, [r1, #1]
800113d4:	e3530000 	cmp	r3, #0
800113d8:	0a000003 	beq	800113ec <vfs_get+0xb8>
800113dc:	e2811001 	add	r1, r1, #1
800113e0:	eaffffdc 	b	80011358 <vfs_get+0x24>
800113e4:	e0881001 	add	r1, r8, r1
800113e8:	ebfffe07 	bl	80010c0c <vfs_simple_get>
800113ec:	e24bd024 	sub	sp, fp, #36	; 0x24
800113f0:	e89dabf0 	ldm	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}

800113f4 <vfs_root>:
800113f4:	e59f3008 	ldr	r3, [pc, #8]	; 80011404 <vfs_root+0x10>
800113f8:	e08f3003 	add	r3, pc, r3
800113fc:	e5930310 	ldr	r0, [r3, #784]	; 0x310
80011400:	e12fff1e 	bx	lr
80011404:	00226bfc 	strdeq	r6, [r2], -ip	; <UNPREDICTABLE>

80011408 <vfs_open>:
80011408:	e59f30c8 	ldr	r3, [pc, #200]	; 800114d8 <vfs_open+0xd0>
8001140c:	e1a0c00d 	mov	ip, sp
80011410:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
80011414:	e2514000 	subs	r4, r1, #0
80011418:	e24cb004 	sub	fp, ip, #4
8001141c:	e1a07000 	mov	r7, r0
80011420:	e1a05002 	mov	r5, r2
80011424:	e08f3003 	add	r3, pc, r3
80011428:	0a000028 	beq	800114d0 <vfs_open+0xc8>
8001142c:	e59f20a8 	ldr	r2, [pc, #168]	; 800114dc <vfs_open+0xd4>
80011430:	e7936002 	ldr	r6, [r3, r2]
80011434:	e5963000 	ldr	r3, [r6]
80011438:	e5933010 	ldr	r3, [r3, #16]
8001143c:	e3530000 	cmp	r3, #0
80011440:	da000005 	ble	8001145c <vfs_open+0x54>
80011444:	e1a00004 	mov	r0, r4
80011448:	ebfffe38 	bl	80010d30 <get_mount_pid>
8001144c:	e5963000 	ldr	r3, [r6]
80011450:	e5933004 	ldr	r3, [r3, #4]
80011454:	e1500003 	cmp	r0, r3
80011458:	1a00001c 	bne	800114d0 <vfs_open+0xc8>
8001145c:	e1a00007 	mov	r0, r7
80011460:	ebfff8c5 	bl	8000f77c <proc_get>
80011464:	e3500000 	cmp	r0, #0
80011468:	0a000018 	beq	800114d0 <vfs_open+0xc8>
8001146c:	e590c02c 	ldr	ip, [r0, #44]	; 0x2c
80011470:	e3a00003 	mov	r0, #3
80011474:	e1a0300c 	mov	r3, ip
80011478:	ea000002 	b	80011488 <vfs_open+0x80>
8001147c:	e2800001 	add	r0, r0, #1
80011480:	e3500080 	cmp	r0, #128	; 0x80
80011484:	0a000011 	beq	800114d0 <vfs_open+0xc8>
80011488:	e5932344 	ldr	r2, [r3, #836]	; 0x344
8001148c:	e283300c 	add	r3, r3, #12
80011490:	e3520000 	cmp	r2, #0
80011494:	1afffff8 	bne	8001147c <vfs_open+0x74>
80011498:	e5942070 	ldr	r2, [r4, #112]	; 0x70
8001149c:	e0803080 	add	r3, r0, r0, lsl #1
800114a0:	e08c3103 	add	r3, ip, r3, lsl #2
800114a4:	e2822001 	add	r2, r2, #1
800114a8:	e3550000 	cmp	r5, #0
800114ac:	e5a34320 	str	r4, [r3, #800]!	; 0x320
800114b0:	e5835004 	str	r5, [r3, #4]
800114b4:	e5842070 	str	r2, [r4, #112]	; 0x70
800114b8:	0a000003 	beq	800114cc <vfs_open+0xc4>
800114bc:	e5943074 	ldr	r3, [r4, #116]	; 0x74
800114c0:	e2833001 	add	r3, r3, #1
800114c4:	e5843074 	str	r3, [r4, #116]	; 0x74
800114c8:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
800114cc:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
800114d0:	e3e00000 	mvn	r0, #0
800114d4:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
800114d8:	0000abd8 	ldrdeq	sl, [r0], -r8
800114dc:	00000000 	andeq	r0, r0, r0

800114e0 <vfs_node_by_fd>:
800114e0:	e59f2020 	ldr	r2, [pc, #32]	; 80011508 <vfs_node_by_fd+0x28>
800114e4:	e59f3020 	ldr	r3, [pc, #32]	; 8001150c <vfs_node_by_fd+0x2c>
800114e8:	e08f2002 	add	r2, pc, r2
800114ec:	e7923003 	ldr	r3, [r2, r3]
800114f0:	e0800080 	add	r0, r0, r0, lsl #1
800114f4:	e5933000 	ldr	r3, [r3]
800114f8:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
800114fc:	e0830100 	add	r0, r3, r0, lsl #2
80011500:	e5900320 	ldr	r0, [r0, #800]	; 0x320
80011504:	e12fff1e 	bx	lr
80011508:	0000ab14 	andeq	sl, r0, r4, lsl fp
8001150c:	00000000 	andeq	r0, r0, r0

80011510 <vfs_close>:
80011510:	e16f3f10 	clz	r3, r0
80011514:	e1a032a3 	lsr	r3, r3, #5
80011518:	e1a0c00d 	mov	ip, sp
8001151c:	e351007f 	cmp	r1, #127	; 0x7f
80011520:	83833001 	orrhi	r3, r3, #1
80011524:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
80011528:	e3530000 	cmp	r3, #0
8001152c:	e24cb004 	sub	fp, ip, #4
80011530:	e24dd020 	sub	sp, sp, #32
80011534:	e1a04001 	mov	r4, r1
80011538:	e1a06000 	mov	r6, r0
8001153c:	1a00003e 	bne	8001163c <vfs_close+0x12c>
80011540:	e590302c 	ldr	r3, [r0, #44]	; 0x2c
80011544:	e1a02081 	lsl	r2, r1, #1
80011548:	e0820001 	add	r0, r2, r1
8001154c:	e1a00100 	lsl	r0, r0, #2
80011550:	e0831000 	add	r1, r3, r0
80011554:	e5915320 	ldr	r5, [r1, #800]	; 0x320
80011558:	e2800e32 	add	r0, r0, #800	; 0x320
8001155c:	e3550000 	cmp	r5, #0
80011560:	e0830000 	add	r0, r3, r0
80011564:	0a000034 	beq	8001163c <vfs_close+0x12c>
80011568:	e5951070 	ldr	r1, [r5, #112]	; 0x70
8001156c:	e0822004 	add	r2, r2, r4
80011570:	e3510000 	cmp	r1, #0
80011574:	12411001 	subne	r1, r1, #1
80011578:	e0833102 	add	r3, r3, r2, lsl #2
8001157c:	15851070 	strne	r1, [r5, #112]	; 0x70
80011580:	e5933324 	ldr	r3, [r3, #804]	; 0x324
80011584:	e3530000 	cmp	r3, #0
80011588:	0a000003 	beq	8001159c <vfs_close+0x8c>
8001158c:	e5953074 	ldr	r3, [r5, #116]	; 0x74
80011590:	e3530000 	cmp	r3, #0
80011594:	12433001 	subne	r3, r3, #1
80011598:	15853074 	strne	r3, [r5, #116]	; 0x74
8001159c:	e3a01000 	mov	r1, #0
800115a0:	e3a0200c 	mov	r2, #12
800115a4:	ebffe5b1 	bl	8000ac70 <memset>
800115a8:	e595305c 	ldr	r3, [r5, #92]	; 0x5c
800115ac:	e3530002 	cmp	r3, #2
800115b0:	0a000023 	beq	80011644 <vfs_close+0x134>
800115b4:	e24b7034 	sub	r7, fp, #52	; 0x34
800115b8:	e3a01000 	mov	r1, #0
800115bc:	e1a02001 	mov	r2, r1
800115c0:	e1a00007 	mov	r0, r7
800115c4:	ebffe869 	bl	8000b770 <proto_init>
800115c8:	e1a00007 	mov	r0, r7
800115cc:	e3a01005 	mov	r1, #5
800115d0:	ebffe8cb 	bl	8000b904 <proto_add_int>
800115d4:	e1a00007 	mov	r0, r7
800115d8:	e1a01004 	mov	r1, r4
800115dc:	ebffe8c8 	bl	8000b904 <proto_add_int>
800115e0:	e1a00007 	mov	r0, r7
800115e4:	e5961004 	ldr	r1, [r6, #4]
800115e8:	ebffe8c5 	bl	8000b904 <proto_add_int>
800115ec:	e1a00007 	mov	r0, r7
800115f0:	e2851018 	add	r1, r5, #24
800115f4:	e3a02058 	mov	r2, #88	; 0x58
800115f8:	ebffe88e 	bl	8000b838 <proto_add>
800115fc:	e51b2030 	ldr	r2, [fp, #-48]	; 0x30
80011600:	e51b302c 	ldr	r3, [fp, #-44]	; 0x2c
80011604:	e1a00005 	mov	r0, r5
80011608:	e50b2038 	str	r2, [fp, #-56]	; 0x38
8001160c:	e50b303c 	str	r3, [fp, #-60]	; 0x3c
80011610:	ebfffdc6 	bl	80010d30 <get_mount_pid>
80011614:	e3500000 	cmp	r0, #0
80011618:	ba000005 	blt	80011634 <vfs_close+0x124>
8001161c:	e24b103c 	sub	r1, fp, #60	; 0x3c
80011620:	e3e02000 	mvn	r2, #0
80011624:	ebfffce2 	bl	800109b4 <proc_send_msg>
80011628:	e3500000 	cmp	r0, #0
8001162c:	15963004 	ldrne	r3, [r6, #4]
80011630:	15803008 	strne	r3, [r0, #8]
80011634:	e1a00007 	mov	r0, r7
80011638:	ebffe914 	bl	8000ba90 <proto_clear>
8001163c:	e24bd01c 	sub	sp, fp, #28
80011640:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
80011644:	e595706c 	ldr	r7, [r5, #108]	; 0x6c
80011648:	e1a00007 	mov	r0, r7
8001164c:	ebfffb00 	bl	80010254 <proc_wakeup>
80011650:	e5953070 	ldr	r3, [r5, #112]	; 0x70
80011654:	e3530000 	cmp	r3, #0
80011658:	1affffd5 	bne	800115b4 <vfs_close+0xa4>
8001165c:	e1a00007 	mov	r0, r7
80011660:	ebfff0d3 	bl	8000d9b4 <kfree>
80011664:	e1a00005 	mov	r0, r5
80011668:	ebfff0d1 	bl	8000d9b4 <kfree>
8001166c:	eafffff2 	b	8001163c <vfs_close+0x12c>

80011670 <vfs_dup>:
80011670:	e59f20bc 	ldr	r2, [pc, #188]	; 80011734 <vfs_dup+0xc4>
80011674:	e1a0c00d 	mov	ip, sp
80011678:	e3500080 	cmp	r0, #128	; 0x80
8001167c:	e92dd878 	push	{r3, r4, r5, r6, fp, ip, lr, pc}
80011680:	e08f2002 	add	r2, pc, r2
80011684:	e24cb004 	sub	fp, ip, #4
80011688:	8a000027 	bhi	8001172c <vfs_dup+0xbc>
8001168c:	e59f30a4 	ldr	r3, [pc, #164]	; 80011738 <vfs_dup+0xc8>
80011690:	e3a04003 	mov	r4, #3
80011694:	e7923003 	ldr	r3, [r2, r3]
80011698:	e5933000 	ldr	r3, [r3]
8001169c:	e593c02c 	ldr	ip, [r3, #44]	; 0x2c
800116a0:	e1a0300c 	mov	r3, ip
800116a4:	ea000002 	b	800116b4 <vfs_dup+0x44>
800116a8:	e2844001 	add	r4, r4, #1
800116ac:	e3540080 	cmp	r4, #128	; 0x80
800116b0:	0a00001d 	beq	8001172c <vfs_dup+0xbc>
800116b4:	e5932344 	ldr	r2, [r3, #836]	; 0x344
800116b8:	e283300c 	add	r3, r3, #12
800116bc:	e3520000 	cmp	r2, #0
800116c0:	1afffff8 	bne	800116a8 <vfs_dup+0x38>
800116c4:	e0800080 	add	r0, r0, r0, lsl #1
800116c8:	e1a03100 	lsl	r3, r0, #2
800116cc:	e08c6003 	add	r6, ip, r3
800116d0:	e5965320 	ldr	r5, [r6, #800]	; 0x320
800116d4:	e2833e32 	add	r3, r3, #800	; 0x320
800116d8:	e3550000 	cmp	r5, #0
800116dc:	e08c1003 	add	r1, ip, r3
800116e0:	0a00000f 	beq	80011724 <vfs_dup+0xb4>
800116e4:	e0840084 	add	r0, r4, r4, lsl #1
800116e8:	e08c0100 	add	r0, ip, r0, lsl #2
800116ec:	e2800e32 	add	r0, r0, #800	; 0x320
800116f0:	e3a0200c 	mov	r2, #12
800116f4:	ebffe541 	bl	8000ac00 <memcpy>
800116f8:	e5953070 	ldr	r3, [r5, #112]	; 0x70
800116fc:	e2833001 	add	r3, r3, #1
80011700:	e5853070 	str	r3, [r5, #112]	; 0x70
80011704:	e5963324 	ldr	r3, [r6, #804]	; 0x324
80011708:	e3530000 	cmp	r3, #0
8001170c:	0a000004 	beq	80011724 <vfs_dup+0xb4>
80011710:	e5953074 	ldr	r3, [r5, #116]	; 0x74
80011714:	e1a00004 	mov	r0, r4
80011718:	e2833001 	add	r3, r3, #1
8001171c:	e5853074 	str	r3, [r5, #116]	; 0x74
80011720:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80011724:	e1a00004 	mov	r0, r4
80011728:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
8001172c:	e3e00000 	mvn	r0, #0
80011730:	e89da878 	ldm	sp, {r3, r4, r5, r6, fp, sp, pc}
80011734:	0000a97c 	andeq	sl, r0, ip, ror r9
80011738:	00000000 	andeq	r0, r0, r0

8001173c <vfs_dup2>:
8001173c:	e59f30b0 	ldr	r3, [pc, #176]	; 800117f4 <vfs_dup2+0xb8>
80011740:	e1a0c00d 	mov	ip, sp
80011744:	e1500001 	cmp	r0, r1
80011748:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
8001174c:	e08f3003 	add	r3, pc, r3
80011750:	e24cb004 	sub	fp, ip, #4
80011754:	e1a04000 	mov	r4, r0
80011758:	e1a05001 	mov	r5, r1
8001175c:	0a000020 	beq	800117e4 <vfs_dup2+0xa8>
80011760:	e3510080 	cmp	r1, #128	; 0x80
80011764:	93500080 	cmpls	r0, #128	; 0x80
80011768:	8a00001f 	bhi	800117ec <vfs_dup2+0xb0>
8001176c:	e59f2084 	ldr	r2, [pc, #132]	; 800117f8 <vfs_dup2+0xbc>
80011770:	e0844084 	add	r4, r4, r4, lsl #1
80011774:	e7936002 	ldr	r6, [r3, r2]
80011778:	e1a04104 	lsl	r4, r4, #2
8001177c:	e5960000 	ldr	r0, [r6]
80011780:	ebffff62 	bl	80011510 <vfs_close>
80011784:	e5963000 	ldr	r3, [r6]
80011788:	e593102c 	ldr	r1, [r3, #44]	; 0x2c
8001178c:	e0817004 	add	r7, r1, r4
80011790:	e5976320 	ldr	r6, [r7, #800]	; 0x320
80011794:	e3560000 	cmp	r6, #0
80011798:	0a000011 	beq	800117e4 <vfs_dup2+0xa8>
8001179c:	e0850085 	add	r0, r5, r5, lsl #1
800117a0:	e0810100 	add	r0, r1, r0, lsl #2
800117a4:	e2844e32 	add	r4, r4, #800	; 0x320
800117a8:	e0811004 	add	r1, r1, r4
800117ac:	e2800e32 	add	r0, r0, #800	; 0x320
800117b0:	e3a0200c 	mov	r2, #12
800117b4:	ebffe511 	bl	8000ac00 <memcpy>
800117b8:	e5963070 	ldr	r3, [r6, #112]	; 0x70
800117bc:	e2833001 	add	r3, r3, #1
800117c0:	e5863070 	str	r3, [r6, #112]	; 0x70
800117c4:	e5973324 	ldr	r3, [r7, #804]	; 0x324
800117c8:	e3530000 	cmp	r3, #0
800117cc:	0a000004 	beq	800117e4 <vfs_dup2+0xa8>
800117d0:	e5963074 	ldr	r3, [r6, #116]	; 0x74
800117d4:	e1a00005 	mov	r0, r5
800117d8:	e2833001 	add	r3, r3, #1
800117dc:	e5863074 	str	r3, [r6, #116]	; 0x74
800117e0:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
800117e4:	e1a00005 	mov	r0, r5
800117e8:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
800117ec:	e3e00000 	mvn	r0, #0
800117f0:	e89da8f0 	ldm	sp, {r4, r5, r6, r7, fp, sp, pc}
800117f4:	0000a8b0 			; <UNDEFINED> instruction: 0x0000a8b0
800117f8:	00000000 	andeq	r0, r0, r0

800117fc <vfs_seek>:
800117fc:	e59fc040 	ldr	ip, [pc, #64]	; 80011844 <vfs_seek+0x48>
80011800:	e350007f 	cmp	r0, #127	; 0x7f
80011804:	e08fc00c 	add	ip, pc, ip
80011808:	8a00000b 	bhi	8001183c <vfs_seek+0x40>
8001180c:	e59f2034 	ldr	r2, [pc, #52]	; 80011848 <vfs_seek+0x4c>
80011810:	e0803080 	add	r3, r0, r0, lsl #1
80011814:	e79c2002 	ldr	r2, [ip, r2]
80011818:	e5922000 	ldr	r2, [r2]
8001181c:	e592002c 	ldr	r0, [r2, #44]	; 0x2c
80011820:	e0800103 	add	r0, r0, r3, lsl #2
80011824:	e5903320 	ldr	r3, [r0, #800]	; 0x320
80011828:	e3530000 	cmp	r3, #0
8001182c:	0a000002 	beq	8001183c <vfs_seek+0x40>
80011830:	e5801328 	str	r1, [r0, #808]	; 0x328
80011834:	e1a00001 	mov	r0, r1
80011838:	e12fff1e 	bx	lr
8001183c:	e3e00000 	mvn	r0, #0
80011840:	e12fff1e 	bx	lr
80011844:	0000a7f8 	strdeq	sl, [r0], -r8
80011848:	00000000 	andeq	r0, r0, r0

8001184c <vfs_tell>:
8001184c:	e59f203c 	ldr	r2, [pc, #60]	; 80011890 <vfs_tell+0x44>
80011850:	e350007f 	cmp	r0, #127	; 0x7f
80011854:	e08f2002 	add	r2, pc, r2
80011858:	8a00000a 	bhi	80011888 <vfs_tell+0x3c>
8001185c:	e59f3030 	ldr	r3, [pc, #48]	; 80011894 <vfs_tell+0x48>
80011860:	e0800080 	add	r0, r0, r0, lsl #1
80011864:	e7923003 	ldr	r3, [r2, r3]
80011868:	e5933000 	ldr	r3, [r3]
8001186c:	e593302c 	ldr	r3, [r3, #44]	; 0x2c
80011870:	e0830100 	add	r0, r3, r0, lsl #2
80011874:	e5903320 	ldr	r3, [r0, #800]	; 0x320
80011878:	e3530000 	cmp	r3, #0
8001187c:	0a000001 	beq	80011888 <vfs_tell+0x3c>
80011880:	e5900328 	ldr	r0, [r0, #808]	; 0x328
80011884:	e12fff1e 	bx	lr
80011888:	e3e00000 	mvn	r0, #0
8001188c:	e12fff1e 	bx	lr
80011890:	0000a7a8 	andeq	sl, r0, r8, lsr #15
80011894:	00000000 	andeq	r0, r0, r0

80011898 <dev_init>:
80011898:	e1a0c00d 	mov	ip, sp
8001189c:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
800118a0:	e59f40ac 	ldr	r4, [pc, #172]	; 80011954 <dev_init+0xbc>
800118a4:	e59f00ac 	ldr	r0, [pc, #172]	; 80011958 <dev_init+0xc0>
800118a8:	e59f10ac 	ldr	r1, [pc, #172]	; 8001195c <dev_init+0xc4>
800118ac:	e08f4004 	add	r4, pc, r4
800118b0:	e24cb004 	sub	fp, ip, #4
800118b4:	e08f0000 	add	r0, pc, r0
800118b8:	e28450c4 	add	r5, r4, #196	; 0xc4
800118bc:	e08f1001 	add	r1, pc, r1
800118c0:	ebffed98 	bl	8000cf28 <printf>
800118c4:	e3a01000 	mov	r1, #0
800118c8:	e3a020c4 	mov	r2, #196	; 0xc4
800118cc:	e1a00005 	mov	r0, r5
800118d0:	ebffe4e6 	bl	8000ac70 <memset>
800118d4:	e1a00005 	mov	r0, r5
800118d8:	ebffdb64 	bl	80008670 <sd_init>
800118dc:	e59f307c 	ldr	r3, [pc, #124]	; 80011960 <dev_init+0xc8>
800118e0:	e08f3003 	add	r3, pc, r3
800118e4:	e3500000 	cmp	r0, #0
800118e8:	0a000004 	beq	80011900 <dev_init+0x68>
800118ec:	e59f0070 	ldr	r0, [pc, #112]	; 80011964 <dev_init+0xcc>
800118f0:	e08f0000 	add	r0, pc, r0
800118f4:	e24bd014 	sub	sp, fp, #20
800118f8:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
800118fc:	eaffed89 	b	8000cf28 <printf>
80011900:	e59f1060 	ldr	r1, [pc, #96]	; 80011968 <dev_init+0xd0>
80011904:	e3a02001 	mov	r2, #1
80011908:	e58420c4 	str	r2, [r4, #196]	; 0xc4
8001190c:	e59f2058 	ldr	r2, [pc, #88]	; 8001196c <dev_init+0xd4>
80011910:	e7930001 	ldr	r0, [r3, r1]
80011914:	e59f1054 	ldr	r1, [pc, #84]	; 80011970 <dev_init+0xd8>
80011918:	e5840180 	str	r0, [r4, #384]	; 0x180
8001191c:	e7930002 	ldr	r0, [r3, r2]
80011920:	e59f204c 	ldr	r2, [pc, #76]	; 80011974 <dev_init+0xdc>
80011924:	e5840178 	str	r0, [r4, #376]	; 0x178
80011928:	e7930001 	ldr	r0, [r3, r1]
8001192c:	e3a01002 	mov	r1, #2
80011930:	e5840184 	str	r0, [r4, #388]	; 0x184
80011934:	e59f003c 	ldr	r0, [pc, #60]	; 80011978 <dev_init+0xe0>
80011938:	e7933002 	ldr	r3, [r3, r2]
8001193c:	e08f0000 	add	r0, pc, r0
80011940:	e584317c 	str	r3, [r4, #380]	; 0x17c
80011944:	e58410c8 	str	r1, [r4, #200]	; 0xc8
80011948:	e24bd014 	sub	sp, fp, #20
8001194c:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
80011950:	eaffed74 	b	8000cf28 <printf>
80011954:	00226a5c 	eoreq	r6, r2, ip, asr sl
80011958:	000008f8 	strdeq	r0, [r0], -r8
8001195c:	000008fc 	strdeq	r0, [r0], -ip
80011960:	0000a71c 	andeq	sl, r0, ip, lsl r7
80011964:	000008d8 	ldrdeq	r0, [r0], -r8
80011968:	0000000c 	andeq	r0, r0, ip
8001196c:	00000020 	andeq	r0, r0, r0, lsr #32
80011970:	00000018 	andeq	r0, r0, r8, lsl r0
80011974:	00000024 	andeq	r0, r0, r4, lsr #32
80011978:	00000884 	andeq	r0, r0, r4, lsl #17

8001197c <get_dev>:
8001197c:	e3500001 	cmp	r0, #1
80011980:	8a000004 	bhi	80011998 <get_dev+0x1c>
80011984:	e59f2014 	ldr	r2, [pc, #20]	; 800119a0 <get_dev+0x24>
80011988:	e3a030c4 	mov	r3, #196	; 0xc4
8001198c:	e08f2002 	add	r2, pc, r2
80011990:	e0202093 	mla	r0, r3, r0, r2
80011994:	e12fff1e 	bx	lr
80011998:	e3a00000 	mov	r0, #0
8001199c:	e12fff1e 	bx	lr
800119a0:	0022697c 	eoreq	r6, r2, ip, ror r9

800119a4 <dev_op>:
800119a4:	e5903010 	ldr	r3, [r0, #16]
800119a8:	e3530000 	cmp	r3, #0
800119ac:	0a000000 	beq	800119b4 <dev_op+0x10>
800119b0:	e12fff13 	bx	r3
800119b4:	e3e00000 	mvn	r0, #0
800119b8:	e12fff1e 	bx	lr

800119bc <dev_ready_read>:
800119bc:	e5903008 	ldr	r3, [r0, #8]
800119c0:	e3530000 	cmp	r3, #0
800119c4:	0a000000 	beq	800119cc <dev_ready_read+0x10>
800119c8:	e12fff13 	bx	r3
800119cc:	e3e00000 	mvn	r0, #0
800119d0:	e12fff1e 	bx	lr

800119d4 <dev_ready_write>:
800119d4:	e590300c 	ldr	r3, [r0, #12]
800119d8:	e3530000 	cmp	r3, #0
800119dc:	0a000000 	beq	800119e4 <dev_ready_write+0x10>
800119e0:	e12fff13 	bx	r3
800119e4:	e3e00000 	mvn	r0, #0
800119e8:	e12fff1e 	bx	lr

800119ec <dev_ch_read>:
800119ec:	e59030a4 	ldr	r3, [r0, #164]	; 0xa4
800119f0:	e3530000 	cmp	r3, #0
800119f4:	0a000000 	beq	800119fc <dev_ch_read+0x10>
800119f8:	e12fff13 	bx	r3
800119fc:	e3e00000 	mvn	r0, #0
80011a00:	e12fff1e 	bx	lr

80011a04 <dev_ch_write>:
80011a04:	e59030a8 	ldr	r3, [r0, #168]	; 0xa8
80011a08:	e3530000 	cmp	r3, #0
80011a0c:	0a000000 	beq	80011a14 <dev_ch_write+0x10>
80011a10:	e12fff13 	bx	r3
80011a14:	e3e00000 	mvn	r0, #0
80011a18:	e12fff1e 	bx	lr

80011a1c <dev_block_read>:
80011a1c:	e59030bc 	ldr	r3, [r0, #188]	; 0xbc
80011a20:	e3530000 	cmp	r3, #0
80011a24:	0a000000 	beq	80011a2c <dev_block_read+0x10>
80011a28:	e12fff13 	bx	r3
80011a2c:	e3e00000 	mvn	r0, #0
80011a30:	e12fff1e 	bx	lr

80011a34 <dev_block_write>:
80011a34:	e59030c0 	ldr	r3, [r0, #192]	; 0xc0
80011a38:	e3530000 	cmp	r3, #0
80011a3c:	0a000000 	beq	80011a44 <dev_block_write+0x10>
80011a40:	e12fff13 	bx	r3
80011a44:	e3e00000 	mvn	r0, #0
80011a48:	e12fff1e 	bx	lr

80011a4c <dev_block_read_done>:
80011a4c:	e59030b4 	ldr	r3, [r0, #180]	; 0xb4
80011a50:	e3530000 	cmp	r3, #0
80011a54:	0a000000 	beq	80011a5c <dev_block_read_done+0x10>
80011a58:	e12fff13 	bx	r3
80011a5c:	e3e00000 	mvn	r0, #0
80011a60:	e12fff1e 	bx	lr

80011a64 <dev_block_write_done>:
80011a64:	e59030b8 	ldr	r3, [r0, #184]	; 0xb8
80011a68:	e3530000 	cmp	r3, #0
80011a6c:	0a000000 	beq	80011a74 <dev_block_write_done+0x10>
80011a70:	e12fff13 	bx	r3
80011a74:	e3e00000 	mvn	r0, #0
80011a78:	e12fff1e 	bx	lr

80011a7c <_delay>:
80011a7c:	e1a0c00d 	mov	ip, sp
80011a80:	e92dd818 	push	{r3, r4, fp, ip, lr, pc}
80011a84:	e2504000 	subs	r4, r0, #0
80011a88:	e24cb004 	sub	fp, ip, #4
80011a8c:	089da818 	ldmeq	sp, {r3, r4, fp, sp, pc}
80011a90:	ebffd9ea 	bl	80008240 <__dummy>
80011a94:	e2544001 	subs	r4, r4, #1
80011a98:	1afffffc 	bne	80011a90 <_delay+0x14>
80011a9c:	e89da818 	ldm	sp, {r3, r4, fp, sp, pc}

80011aa0 <_delay_usec>:
80011aa0:	e1a0c00d 	mov	ip, sp
80011aa4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
80011aa8:	e24cb004 	sub	fp, ip, #4
80011aac:	e1a04000 	mov	r4, r0
80011ab0:	e1a05001 	mov	r5, r1
80011ab4:	ebffdc59 	bl	80008c20 <timer_read_sys_usec>
80011ab8:	e0944000 	adds	r4, r4, r0
80011abc:	e0a55001 	adc	r5, r5, r1
80011ac0:	e1510005 	cmp	r1, r5
80011ac4:	01500004 	cmpeq	r0, r4
80011ac8:	289da830 	ldmcs	sp, {r4, r5, fp, sp, pc}
80011acc:	ebffdc53 	bl	80008c20 <timer_read_sys_usec>
80011ad0:	e1550001 	cmp	r5, r1
80011ad4:	01540000 	cmpeq	r4, r0
80011ad8:	8afffffb 	bhi	80011acc <_delay_usec+0x2c>
80011adc:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

80011ae0 <_delay_msec>:
80011ae0:	e1a0c00d 	mov	ip, sp
80011ae4:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
80011ae8:	e24cb004 	sub	fp, ip, #4
80011aec:	e1a04000 	mov	r4, r0
80011af0:	ebffdc4a 	bl	80008c20 <timer_read_sys_usec>
80011af4:	e3a03ffa 	mov	r3, #1000	; 0x3e8
80011af8:	e0030394 	mul	r3, r4, r3
80011afc:	e0904003 	adds	r4, r0, r3
80011b00:	e2a15000 	adc	r5, r1, #0
80011b04:	e1510005 	cmp	r1, r5
80011b08:	01500004 	cmpeq	r0, r4
80011b0c:	289da830 	ldmcs	sp, {r4, r5, fp, sp, pc}
80011b10:	ebffdc42 	bl	80008c20 <timer_read_sys_usec>
80011b14:	e1550001 	cmp	r5, r1
80011b18:	01540000 	cmpeq	r4, r0
80011b1c:	8afffffb 	bhi	80011b10 <_delay_msec+0x30>
80011b20:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}

80011b24 <_flush_tlb>:
80011b24:	e1a0c00d 	mov	ip, sp
80011b28:	e92dd800 	push	{fp, ip, lr, pc}
80011b2c:	e24cb004 	sub	fp, ip, #4
80011b30:	e3a06000 	mov	r6, #0
80011b34:	ee086f17 	mcr	15, 0, r6, cr8, cr7, {0}
80011b38:	e89da800 	ldm	sp, {fp, sp, pc}

80011b3c <set_kernel_init_vm>:
80011b3c:	e1a0c00d 	mov	ip, sp
80011b40:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
80011b44:	e24cb004 	sub	fp, ip, #4
80011b48:	e24dd008 	sub	sp, sp, #8
80011b4c:	e3a01000 	mov	r1, #0
80011b50:	e3a02901 	mov	r2, #16384	; 0x4000
80011b54:	e1a04000 	mov	r4, r0
80011b58:	e3a05055 	mov	r5, #85	; 0x55
80011b5c:	ebffe443 	bl	8000ac70 <memset>
80011b60:	e3a01000 	mov	r1, #0
80011b64:	e1a00004 	mov	r0, r4
80011b68:	e1a02001 	mov	r2, r1
80011b6c:	e58d5000 	str	r5, [sp]
80011b70:	e3a03a01 	mov	r3, #4096	; 0x1000
80011b74:	ebffee1b 	bl	8000d3e8 <map_pages>
80011b78:	e1a00004 	mov	r0, r4
80011b7c:	e58d5000 	str	r5, [sp]
80011b80:	e59f1074 	ldr	r1, [pc, #116]	; 80011bfc <set_kernel_init_vm+0xc0>
80011b84:	e3a02000 	mov	r2, #0
80011b88:	e3a03a01 	mov	r3, #4096	; 0x1000
80011b8c:	ebffee15 	bl	8000d3e8 <map_pages>
80011b90:	e59f2068 	ldr	r2, [pc, #104]	; 80011c00 <set_kernel_init_vm+0xc4>
80011b94:	e59f3068 	ldr	r3, [pc, #104]	; 80011c04 <set_kernel_init_vm+0xc8>
80011b98:	e08f2002 	add	r2, pc, r2
80011b9c:	e7923003 	ldr	r3, [r2, r3]
80011ba0:	e1a00004 	mov	r0, r4
80011ba4:	e2833dff 	add	r3, r3, #16320	; 0x3fc0
80011ba8:	e283303f 	add	r3, r3, #63	; 0x3f
80011bac:	e3c33dff 	bic	r3, r3, #16320	; 0x3fc0
80011bb0:	e3c3303f 	bic	r3, r3, #63	; 0x3f
80011bb4:	e2833482 	add	r3, r3, #-2113929216	; 0x82000000
80011bb8:	e59f1048 	ldr	r1, [pc, #72]	; 80011c08 <set_kernel_init_vm+0xcc>
80011bbc:	e2833812 	add	r3, r3, #1179648	; 0x120000
80011bc0:	e3a02a01 	mov	r2, #4096	; 0x1000
80011bc4:	e58d5000 	str	r5, [sp]
80011bc8:	ebffee06 	bl	8000d3e8 <map_pages>
80011bcc:	ebffd9b5 	bl	800082a8 <get_hw_info>
80011bd0:	e3a01103 	mov	r1, #-1073741824	; 0xc0000000
80011bd4:	e5902024 	ldr	r2, [r0, #36]	; 0x24
80011bd8:	e5903028 	ldr	r3, [r0, #40]	; 0x28
80011bdc:	e1a00004 	mov	r0, r4
80011be0:	e58d5000 	str	r5, [sp]
80011be4:	e0823003 	add	r3, r2, r3
80011be8:	ebffedfe 	bl	8000d3e8 <map_pages>
80011bec:	e1a00004 	mov	r0, r4
80011bf0:	e24bd014 	sub	sp, fp, #20
80011bf4:	e89d6830 	ldm	sp, {r4, r5, fp, sp, lr}
80011bf8:	eaffd9ae 	b	800082b8 <arch_vm>
80011bfc:	ffff0000 			; <UNDEFINED> instruction: 0xffff0000
80011c00:	0000a464 	andeq	sl, r0, r4, ror #8
80011c04:	0000001c 	andeq	r0, r0, ip, lsl r0
80011c08:	80001000 	andhi	r1, r0, r0

80011c0c <set_kernel_vm>:
80011c0c:	e1a0c00d 	mov	ip, sp
80011c10:	e92dd830 	push	{r4, r5, fp, ip, lr, pc}
80011c14:	e59f405c 	ldr	r4, [pc, #92]	; 80011c78 <set_kernel_vm+0x6c>
80011c18:	e24cb004 	sub	fp, ip, #4
80011c1c:	e24dd008 	sub	sp, sp, #8
80011c20:	e1a05000 	mov	r5, r0
80011c24:	ebffffc4 	bl	80011b3c <set_kernel_init_vm>
80011c28:	e59f204c 	ldr	r2, [pc, #76]	; 80011c7c <set_kernel_vm+0x70>
80011c2c:	e08f4004 	add	r4, pc, r4
80011c30:	e1a03004 	mov	r3, r4
80011c34:	e7944002 	ldr	r4, [r4, r2]
80011c38:	ebffd99a 	bl	800082a8 <get_hw_info>
80011c3c:	e3a0c055 	mov	ip, #85	; 0x55
80011c40:	e2844dff 	add	r4, r4, #16320	; 0x3fc0
80011c44:	e284403f 	add	r4, r4, #63	; 0x3f
80011c48:	e3c44dff 	bic	r4, r4, #16320	; 0x3fc0
80011c4c:	e3c4403f 	bic	r4, r4, #63	; 0x3f
80011c50:	e2844802 	add	r4, r4, #131072	; 0x20000
80011c54:	e2842482 	add	r2, r4, #-2113929216	; 0x82000000
80011c58:	e2822601 	add	r2, r2, #1048576	; 0x100000
80011c5c:	e2841621 	add	r1, r4, #34603008	; 0x2100000
80011c60:	e5903020 	ldr	r3, [r0, #32]
80011c64:	e1a00005 	mov	r0, r5
80011c68:	e58dc000 	str	ip, [sp]
80011c6c:	ebffeddd 	bl	8000d3e8 <map_pages>
80011c70:	e24bd014 	sub	sp, fp, #20
80011c74:	e89da830 	ldm	sp, {r4, r5, fp, sp, pc}
80011c78:	0000a3d0 	ldrdeq	sl, [r0], -r0
80011c7c:	0000001c 	andeq	r0, r0, ip, lsl r0

80011c80 <_kernel_entry_c>:
80011c80:	e1a0c00d 	mov	ip, sp
80011c84:	e92dd8f0 	push	{r4, r5, r6, r7, fp, ip, lr, pc}
80011c88:	e59f52b4 	ldr	r5, [pc, #692]	; 80011f44 <_kernel_entry_c+0x2c4>
80011c8c:	e24cb004 	sub	fp, ip, #4
80011c90:	e24dd010 	sub	sp, sp, #16
80011c94:	ebffd972 	bl	80008264 <hw_info_init>
80011c98:	e59f32a8 	ldr	r3, [pc, #680]	; 80011f48 <_kernel_entry_c+0x2c8>
80011c9c:	e08f5005 	add	r5, pc, r5
80011ca0:	e7953003 	ldr	r3, [r5, r3]
80011ca4:	e59f12a0 	ldr	r1, [pc, #672]	; 80011f4c <_kernel_entry_c+0x2cc>
80011ca8:	e3a02000 	mov	r2, #0
80011cac:	e5832004 	str	r2, [r3, #4]
80011cb0:	e5832000 	str	r2, [r3]
80011cb4:	e583200c 	str	r2, [r3, #12]
80011cb8:	e5832008 	str	r2, [r3, #8]
80011cbc:	e5832014 	str	r2, [r3, #20]
80011cc0:	e5832010 	str	r2, [r3, #16]
80011cc4:	e583201c 	str	r2, [r3, #28]
80011cc8:	e5832018 	str	r2, [r3, #24]
80011ccc:	e7954001 	ldr	r4, [r5, r1]
80011cd0:	e59f3278 	ldr	r3, [pc, #632]	; 80011f50 <_kernel_entry_c+0x2d0>
80011cd4:	e2844dff 	add	r4, r4, #16320	; 0x3fc0
80011cd8:	e284403f 	add	r4, r4, #63	; 0x3f
80011cdc:	e7956003 	ldr	r6, [r5, r3]
80011ce0:	e3c44dff 	bic	r4, r4, #16320	; 0x3fc0
80011ce4:	e3c4403f 	bic	r4, r4, #63	; 0x3f
80011ce8:	e2841802 	add	r1, r4, #131072	; 0x20000
80011cec:	e2840901 	add	r0, r4, #16384	; 0x4000
80011cf0:	e5864000 	str	r4, [r6]
80011cf4:	ebffecee 	bl	8000d0b4 <kalloc_init>
80011cf8:	e5960000 	ldr	r0, [r6]
80011cfc:	ebffff8e 	bl	80011b3c <set_kernel_init_vm>
80011d00:	e5960000 	ldr	r0, [r6]
80011d04:	e2800102 	add	r0, r0, #-2147483648	; 0x80000000
80011d08:	ebffd921 	bl	80008194 <__set_translation_table_base>
80011d0c:	e59f3240 	ldr	r3, [pc, #576]	; 80011f54 <_kernel_entry_c+0x2d4>
80011d10:	e3a02103 	mov	r2, #-1073741824	; 0xc0000000
80011d14:	e7953003 	ldr	r3, [r5, r3]
80011d18:	e5832000 	str	r2, [r3]
80011d1c:	ebffeeec 	bl	8000d8d4 <km_init>
80011d20:	ebffd966 	bl	800082c0 <uart_dev_init>
80011d24:	ebffec4a 	bl	8000ce54 <init_console>
80011d28:	e59f0228 	ldr	r0, [pc, #552]	; 80011f58 <_kernel_entry_c+0x2d8>
80011d2c:	e08f0000 	add	r0, pc, r0
80011d30:	ebffec7c 	bl	8000cf28 <printf>
80011d34:	e3a01601 	mov	r1, #1048576	; 0x100000
80011d38:	e3a00402 	mov	r0, #33554432	; 0x2000000
80011d3c:	ebffe92c 	bl	8000c1f4 <div_u32>
80011d40:	e1a01000 	mov	r1, r0
80011d44:	e59f0210 	ldr	r0, [pc, #528]	; 80011f5c <_kernel_entry_c+0x2dc>
80011d48:	e08f0000 	add	r0, pc, r0
80011d4c:	ebffec75 	bl	8000cf28 <printf>
80011d50:	e59f0208 	ldr	r0, [pc, #520]	; 80011f60 <_kernel_entry_c+0x2e0>
80011d54:	e08f0000 	add	r0, pc, r0
80011d58:	ebffec72 	bl	8000cf28 <printf>
80011d5c:	e3a00c05 	mov	r0, #1280	; 0x500
80011d60:	e3a01e2d 	mov	r1, #720	; 0x2d0
80011d64:	e3a02010 	mov	r2, #16
80011d68:	ebffd9b5 	bl	80008444 <fb_dev_init>
80011d6c:	e2507000 	subs	r7, r0, #0
80011d70:	1a00006b 	bne	80011f24 <_kernel_entry_c+0x2a4>
80011d74:	ebffda25 	bl	80008610 <fb_get_info>
80011d78:	e1a05000 	mov	r5, r0
80011d7c:	e5900020 	ldr	r0, [r0, #32]
80011d80:	e8950006 	ldm	r5, {r1, r2}
80011d84:	e5953014 	ldr	r3, [r5, #20]
80011d88:	e58d0000 	str	r0, [sp]
80011d8c:	e5950024 	ldr	r0, [r5, #36]	; 0x24
80011d90:	e58d0004 	str	r0, [sp, #4]
80011d94:	e59f01c8 	ldr	r0, [pc, #456]	; 80011f64 <_kernel_entry_c+0x2e4>
80011d98:	e08f0000 	add	r0, pc, r0
80011d9c:	ebffec61 	bl	8000cf28 <printf>
80011da0:	e1a01007 	mov	r1, r7
80011da4:	e5950020 	ldr	r0, [r5, #32]
80011da8:	e5952024 	ldr	r2, [r5, #36]	; 0x24
80011dac:	ebffe3af 	bl	8000ac70 <memset>
80011db0:	e2844802 	add	r4, r4, #131072	; 0x20000
80011db4:	e2845621 	add	r5, r4, #34603008	; 0x2100000
80011db8:	ebffec2a 	bl	8000ce68 <setup_console>
80011dbc:	e1a01005 	mov	r1, r5
80011dc0:	e2840402 	add	r0, r4, #33554432	; 0x2000000
80011dc4:	e3a02000 	mov	r2, #0
80011dc8:	ebffecb9 	bl	8000d0b4 <kalloc_init>
80011dcc:	e5967000 	ldr	r7, [r6]
80011dd0:	ebffd934 	bl	800082a8 <get_hw_info>
80011dd4:	e3a0c055 	mov	ip, #85	; 0x55
80011dd8:	e2842482 	add	r2, r4, #-2113929216	; 0x82000000
80011ddc:	e2822601 	add	r2, r2, #1048576	; 0x100000
80011de0:	e1a01005 	mov	r1, r5
80011de4:	e59f617c 	ldr	r6, [pc, #380]	; 80011f68 <_kernel_entry_c+0x2e8>
80011de8:	e08f6006 	add	r6, pc, r6
80011dec:	e5903020 	ldr	r3, [r0, #32]
80011df0:	e58dc000 	str	ip, [sp]
80011df4:	e1a00007 	mov	r0, r7
80011df8:	ebffed7a 	bl	8000d3e8 <map_pages>
80011dfc:	ebffd929 	bl	800082a8 <get_hw_info>
80011e00:	e3a02001 	mov	r2, #1
80011e04:	e5901020 	ldr	r1, [r0, #32]
80011e08:	e1a00005 	mov	r0, r5
80011e0c:	e281147e 	add	r1, r1, #2113929216	; 0x7e000000
80011e10:	ebffeca7 	bl	8000d0b4 <kalloc_init>
80011e14:	ebffed1e 	bl	8000d294 <get_free_mem_size>
80011e18:	e3a01601 	mov	r1, #1048576	; 0x100000
80011e1c:	ebffe8f4 	bl	8000c1f4 <div_u32>
80011e20:	e1a01000 	mov	r1, r0
80011e24:	e59f0140 	ldr	r0, [pc, #320]	; 80011f6c <_kernel_entry_c+0x2ec>
80011e28:	e08f0000 	add	r0, pc, r0
80011e2c:	ebffec3d 	bl	8000cf28 <printf>
80011e30:	e59f0138 	ldr	r0, [pc, #312]	; 80011f70 <_kernel_entry_c+0x2f0>
80011e34:	e08f0000 	add	r0, pc, r0
80011e38:	ebffec3a 	bl	8000cf28 <printf>
80011e3c:	ebfffe95 	bl	80011898 <dev_init>
80011e40:	ebffd91d 	bl	800082bc <hw_optimise>
80011e44:	ebfff1c0 	bl	8000e54c <init_global>
80011e48:	e59f0124 	ldr	r0, [pc, #292]	; 80011f74 <_kernel_entry_c+0x2f4>
80011e4c:	e08f0000 	add	r0, pc, r0
80011e50:	ebffec34 	bl	8000cf28 <printf>
80011e54:	ebffef01 	bl	8000da60 <shm_init>
80011e58:	e59f0118 	ldr	r0, [pc, #280]	; 80011f78 <_kernel_entry_c+0x2f8>
80011e5c:	e08f0000 	add	r0, pc, r0
80011e60:	ebffec30 	bl	8000cf28 <printf>
80011e64:	ebfff62a 	bl	8000f714 <procs_init>
80011e68:	e59f010c 	ldr	r0, [pc, #268]	; 80011f7c <_kernel_entry_c+0x2fc>
80011e6c:	e08f0000 	add	r0, pc, r0
80011e70:	ebffec2c 	bl	8000cf28 <printf>
80011e74:	ebfffd13 	bl	800112c8 <vfs_init>
80011e78:	e59f0100 	ldr	r0, [pc, #256]	; 80011f80 <_kernel_entry_c+0x300>
80011e7c:	e08f0000 	add	r0, pc, r0
80011e80:	ebffec28 	bl	8000cf28 <printf>
80011e84:	ebfff197 	bl	8000e4e8 <irq_init>
80011e88:	e59f00f4 	ldr	r0, [pc, #244]	; 80011f84 <_kernel_entry_c+0x304>
80011e8c:	e08f0000 	add	r0, pc, r0
80011e90:	ebffec24 	bl	8000cf28 <printf>
80011e94:	e59f00ec 	ldr	r0, [pc, #236]	; 80011f88 <_kernel_entry_c+0x308>
80011e98:	e08f0000 	add	r0, pc, r0
80011e9c:	ebffec21 	bl	8000cf28 <printf>
80011ea0:	e1a00006 	mov	r0, r6
80011ea4:	e24b1020 	sub	r1, fp, #32
80011ea8:	ebffea73 	bl	8000c87c <sd_read_ext2>
80011eac:	e2504000 	subs	r4, r0, #0
80011eb0:	0a00000f 	beq	80011ef4 <_kernel_entry_c+0x274>
80011eb4:	e3a00000 	mov	r0, #0
80011eb8:	e1a01000 	mov	r1, r0
80011ebc:	ebfff7b8 	bl	8000fda4 <proc_create>
80011ec0:	e1a01006 	mov	r1, r6
80011ec4:	e1a05000 	mov	r5, r0
80011ec8:	e59000c8 	ldr	r0, [r0, #200]	; 0xc8
80011ecc:	ebffe743 	bl	8000bbe0 <str_cpy>
80011ed0:	e1a00005 	mov	r0, r5
80011ed4:	e1a01004 	mov	r1, r4
80011ed8:	e51b2020 	ldr	r2, [fp, #-32]
80011edc:	ebfff84c 	bl	80010014 <proc_load_elf>
80011ee0:	e1a05000 	mov	r5, r0
80011ee4:	e1a00004 	mov	r0, r4
80011ee8:	ebffeeb1 	bl	8000d9b4 <kfree>
80011eec:	e3550000 	cmp	r5, #0
80011ef0:	0a00000f 	beq	80011f34 <_kernel_entry_c+0x2b4>
80011ef4:	e59f0090 	ldr	r0, [pc, #144]	; 80011f8c <_kernel_entry_c+0x30c>
80011ef8:	e08f0000 	add	r0, pc, r0
80011efc:	ebffec09 	bl	8000cf28 <printf>
80011f00:	e59f0088 	ldr	r0, [pc, #136]	; 80011f90 <_kernel_entry_c+0x310>
80011f04:	e08f0000 	add	r0, pc, r0
80011f08:	ebffec06 	bl	8000cf28 <printf>
80011f0c:	e3a00000 	mov	r0, #0
80011f10:	e3a01c02 	mov	r1, #512	; 0x200
80011f14:	ebffdb2d 	bl	80008bd0 <timer_set_interval>
80011f18:	e3a00000 	mov	r0, #0
80011f1c:	ee070f90 	mcr	15, 0, r0, cr7, cr0, {4}
80011f20:	eafffffc 	b	80011f18 <_kernel_entry_c+0x298>
80011f24:	e59f0068 	ldr	r0, [pc, #104]	; 80011f94 <_kernel_entry_c+0x314>
80011f28:	e08f0000 	add	r0, pc, r0
80011f2c:	ebffebfd 	bl	8000cf28 <printf>
80011f30:	eaffff9e 	b	80011db0 <_kernel_entry_c+0x130>
80011f34:	e59f005c 	ldr	r0, [pc, #92]	; 80011f98 <_kernel_entry_c+0x318>
80011f38:	e08f0000 	add	r0, pc, r0
80011f3c:	ebffebf9 	bl	8000cf28 <printf>
80011f40:	eaffffee 	b	80011f00 <_kernel_entry_c+0x280>
80011f44:	0000a360 	andeq	sl, r0, r0, ror #6
80011f48:	00000004 	andeq	r0, r0, r4
80011f4c:	0000001c 	andeq	r0, r0, ip, lsl r0
80011f50:	00000014 	andeq	r0, r0, r4, lsl r0
80011f54:	00000010 	andeq	r0, r0, r0, lsl r0
80011f58:	000004a8 	andeq	r0, r0, r8, lsr #9
80011f5c:	000004d0 	ldrdeq	r0, [r0], -r0	; <UNPREDICTABLE>
80011f60:	000004ec 	andeq	r0, r0, ip, ror #9
80011f64:	000004c8 	andeq	r0, r0, r8, asr #9
80011f68:	00000590 	muleq	r0, r0, r5
80011f6c:	00000474 	andeq	r0, r0, r4, ror r4
80011f70:	00000490 	muleq	r0, r0, r4
80011f74:	00000494 	muleq	r0, r4, r4
80011f78:	000004a0 	andeq	r0, r0, r0, lsr #9
80011f7c:	000004b0 			; <UNDEFINED> instruction: 0x000004b0
80011f80:	000004bc 			; <UNDEFINED> instruction: 0x000004bc
80011f84:	000004c0 	andeq	r0, r0, r0, asr #9
80011f88:	000004c8 	andeq	r0, r0, r8, asr #9
80011f8c:	0000048c 	andeq	r0, r0, ip, lsl #9
80011f90:	00000494 	muleq	r0, r4, r4
80011f94:	00000364 	andeq	r0, r0, r4, ror #6
80011f98:	00000458 	andeq	r0, r0, r8, asr r4

Disassembly of section .rodata.str1.4:

80011f9c <.rodata.str1.4>:
80011f9c:	73726576 	cmnvc	r2, #494927872	; 0x1d800000
80011fa0:	6c697461 	cfstrdvs	mvd7, [r9], #-388	; 0xfffffe7c
80011fa4:	00627065 	rsbeq	r7, r2, r5, rrx
80011fa8:	36317838 			; <UNDEFINED> instruction: 0x36317838
80011fac:	00000000 	andeq	r0, r0, r0
80011fb0:	33323130 	teqcc	r2, #48, 2
80011fb4:	37363534 			; <UNDEFINED> instruction: 0x37363534
80011fb8:	62613938 	rsbvs	r3, r1, #56, 18	; 0xe0000
80011fbc:	66656463 	strbtvs	r6, [r5], -r3, ror #8
80011fc0:	00000000 	andeq	r0, r0, r0
80011fc4:	33323130 	teqcc	r2, #48, 2
80011fc8:	37363534 			; <UNDEFINED> instruction: 0x37363534
80011fcc:	42413938 	submi	r3, r1, #56, 18	; 0xe0000
80011fd0:	46454443 	strbmi	r4, [r5], -r3, asr #8
80011fd4:	00000000 	andeq	r0, r0, r0
80011fd8:	7778797a 			; <UNDEFINED> instruction: 0x7778797a
80011fdc:	73747576 	cmnvc	r4, #494927872	; 0x1d800000
80011fe0:	6f707172 	svcvs	0x00707172
80011fe4:	6b6c6d6e 	blvs	81b2d5a4 <_kernel_end+0x14c45a4>
80011fe8:	6768696a 	strbvs	r6, [r8, -sl, ror #18]!
80011fec:	63646566 	cmnvs	r4, #427819008	; 0x19800000
80011ff0:	38396162 	ldmdacc	r9!, {r1, r5, r6, r8, sp, lr}
80011ff4:	34353637 	ldrtcc	r3, [r5], #-1591	; 0x637
80011ff8:	30313233 	eorscc	r3, r1, r3, lsr r2
80011ffc:	34333231 	ldrtcc	r3, [r3], #-561	; 0x231
80012000:	38373635 	ldmdacc	r7!, {r0, r2, r4, r5, r9, sl, ip, sp}
80012004:	63626139 	cmnvs	r2, #1073741838	; 0x4000000e
80012008:	67666564 	strbvs	r6, [r6, -r4, ror #10]!
8001200c:	6b6a6968 	blvs	81aac5b4 <_kernel_end+0x14435b4>
80012010:	6f6e6d6c 	svcvs	0x006e6d6c
80012014:	73727170 	cmnvc	r2, #112, 2
80012018:	77767574 			; <UNDEFINED> instruction: 0x77767574
8001201c:	007a7978 	rsbseq	r7, sl, r8, ror r9
80012020:	736c6166 	cmnvc	ip, #-2147483623	; 0x80000019
80012024:	00000065 	andeq	r0, r0, r5, rrx
80012028:	65757274 	ldrbvs	r7, [r5, #-628]!	; 0x274
8001202c:	00000000 	andeq	r0, r0, r0
80012030:	00007830 	andeq	r7, r0, r0, lsr r8
80012034:	0000002f 	andeq	r0, r0, pc, lsr #32
80012038:	696e6150 	stmdbvs	lr!, {r4, r6, r8, sp, lr}^
8001203c:	6b203a63 	blvs	808209d0 <_kernel_end+0x1b79d0>
80012040:	6c615f6d 	stclvs	15, cr5, [r1], #-436	; 0xfffffe4c
80012044:	20636f6c 	rsbcs	r6, r3, ip, ror #30
80012048:	6c696166 	stfvse	f6, [r9], #-408	; 0xfffffe68
8001204c:	0a216465 	beq	8086b1e8 <_kernel_end+0x2021e8>
80012050:	00000000 	andeq	r0, r0, r0
80012054:	5f6d6873 	svcpl	0x006d6873
80012058:	3a70616d 	bcc	81c2a614 <_kernel_end+0x15c1614>
8001205c:	6c616b20 	stclvs	11, cr6, [r1], #-128	; 0xffffff80
80012060:	20636f6c 	rsbcs	r6, r3, ip, ror #30
80012064:	6c696166 	stfvse	f6, [r9], #-408	; 0xfffffe68
80012068:	0a216465 	beq	8086b204 <_kernel_end+0x202204>
8001206c:	00000000 	andeq	r0, r0, r0
80012070:	72656b5f 	rsbvc	r6, r5, #97280	; 0x17c00
80012074:	2c6c656e 	cfstr64cs	mvdx6, [ip], #-440	; 0xfffffe48
80012078:	65727020 	ldrbvs	r7, [r2, #-32]!
8001207c:	63746566 	cmnvs	r4, #427819008	; 0x19800000
80012080:	62612068 	rsbvs	r2, r1, #104	; 0x68
80012084:	2174726f 	cmncs	r4, pc, ror #4
80012088:	00000a21 	andeq	r0, r0, r1, lsr #20
8001208c:	3a646970 	bcc	8192c654 <_kernel_end+0x12c3654>
80012090:	28642520 	stmdacs	r4!, {r5, r8, sl, sp}^
80012094:	2c297325 	stccs	3, cr7, [r9], #-148	; 0xffffff6c
80012098:	65727020 	ldrbvs	r7, [r2, #-32]!
8001209c:	63746566 	cmnvs	r4, #427819008	; 0x19800000
800120a0:	62612068 	rsbvs	r2, r1, #104	; 0x68
800120a4:	2174726f 	cmncs	r4, pc, ror #4
800120a8:	00000a21 	andeq	r0, r0, r1, lsr #20
800120ac:	72656b5f 	rsbvc	r6, r5, #97280	; 0x17c00
800120b0:	2c6c656e 	cfstr64cs	mvdx6, [ip], #-440	; 0xfffffe48
800120b4:	74616420 	strbtvc	r6, [r1], #-1056	; 0x420
800120b8:	62612061 	rsbvs	r2, r1, #97	; 0x61
800120bc:	2174726f 	cmncs	r4, pc, ror #4
800120c0:	00000a21 	andeq	r0, r0, r1, lsr #20
800120c4:	3a646970 	bcc	8192c68c <_kernel_end+0x12c368c>
800120c8:	28642520 	stmdacs	r4!, {r5, r8, sl, sp}^
800120cc:	2c297325 	stccs	3, cr7, [r9], #-148	; 0xffffff6c
800120d0:	74616420 	strbtvc	r6, [r1], #-1056	; 0x420
800120d4:	62612061 	rsbvs	r2, r1, #97	; 0x61
800120d8:	2174726f 	cmncs	r4, pc, ror #4
800120dc:	00000a21 	andeq	r0, r0, r1, lsr #20
800120e0:	3a646970 	bcc	8192c6a8 <_kernel_end+0x12c36a8>
800120e4:	202c6425 	eorcs	r6, ip, r5, lsr #8
800120e8:	65646f63 	strbvs	r6, [r4, #-3939]!	; 0xf63
800120ec:	29642528 	stmdbcs	r4!, {r3, r5, r8, sl, sp}^
800120f0:	72726520 	rsbsvc	r6, r2, #32, 10	; 0x8000000
800120f4:	0a21726f 	beq	8086eab8 <_kernel_end+0x205ab8>
800120f8:	00000000 	andeq	r0, r0, r0
800120fc:	636f7270 	cmnvs	pc, #112, 4
80012100:	70786520 	rsbsvc	r6, r8, r0, lsr #10
80012104:	20646e61 	rsbcs	r6, r4, r1, ror #28
80012108:	6c696166 	stfvse	f6, [r9], #-408	; 0xfffffe68
8001210c:	21216465 			; <UNDEFINED> instruction: 0x21216465
80012110:	65726620 	ldrbvs	r6, [r2, #-1568]!	; 0x620
80012114:	656d2065 	strbvs	r2, [sp, #-101]!	; 0x65
80012118:	6973206d 	ldmdbvs	r3!, {r0, r2, r3, r5, r6, sp}^
8001211c:	203a657a 	eorscs	r6, sl, sl, ror r5
80012120:	29782528 	ldmdbcs	r8!, {r3, r5, r8, sl, sp}^
80012124:	6970202c 	ldmdbvs	r0!, {r2, r3, r5, sp}^
80012128:	64253a64 	strtvs	r3, [r5], #-2660	; 0xa64
8001212c:	6170202c 	cmnvs	r0, ip, lsr #32
80012130:	20736567 	rsbscs	r6, r3, r7, ror #10
80012134:	3a6b7361 	bcc	81aeeec0 <_kernel_end+0x1485ec0>
80012138:	000a6425 	andeq	r6, sl, r5, lsr #8
8001213c:	696e6170 	stmdbvs	lr!, {r4, r5, r6, r8, sp, lr}^
80012140:	6b203a63 	blvs	80820ad4 <_kernel_end+0x1b7ad4>
80012144:	6b726f66 	blvs	81cadee4 <_kernel_end+0x1644ee4>
80012148:	65726320 	ldrbvs	r6, [r2, #-800]!	; 0x320
8001214c:	20657461 	rsbcs	r7, r5, r1, ror #8
80012150:	636f7270 	cmnvs	pc, #112, 4
80012154:	69616620 	stmdbvs	r1!, {r5, r9, sl, sp, lr}^
80012158:	2164656c 	cmncs	r4, ip, ror #10
8001215c:	64252821 	strtvs	r2, [r5], #-2081	; 0x821
80012160:	00000a29 	andeq	r0, r0, r9, lsr #20
80012164:	696e6150 	stmdbvs	lr!, {r4, r6, r8, sp, lr}^
80012168:	6b203a63 	blvs	80820afc <_kernel_end+0x1b7afc>
8001216c:	6b726f66 	blvs	81cadf0c <_kernel_end+0x1644f0c>
80012170:	70786520 	rsbsvc	r6, r8, r0, lsr #10
80012174:	20646e61 	rsbcs	r6, r4, r1, ror #28
80012178:	6f6d656d 	svcvs	0x006d656d
8001217c:	66207972 			; <UNDEFINED> instruction: 0x66207972
80012180:	656c6961 	strbvs	r6, [ip, #-2401]!	; 0x961
80012184:	28212164 	stmdacs	r1!, {r2, r5, r6, r8, sp}
80012188:	0a296425 	beq	80a6b224 <_kernel_end+0x402224>
8001218c:	00000000 	andeq	r0, r0, r0
80012190:	696e6170 	stmdbvs	lr!, {r4, r5, r6, r8, sp, lr}^
80012194:	6b203a63 	blvs	80820b28 <_kernel_end+0x1b7b28>
80012198:	6b726f66 	blvs	81cadf38 <_kernel_end+0x1644f38>
8001219c:	6f6c6320 	svcvs	0x006c6320
800121a0:	6620656e 	strtvs	r6, [r0], -lr, ror #10
800121a4:	656c6961 	strbvs	r6, [ip, #-2401]!	; 0x961
800121a8:	28212164 	stmdacs	r1!, {r2, r5, r6, r8, sp}
800121ac:	0a296425 	beq	80a6b248 <_kernel_end+0x402248>
800121b0:	00000000 	andeq	r0, r0, r0
800121b4:	20202020 	eorcs	r2, r0, r0, lsr #32
800121b8:	73363125 	teqvc	r6, #1073741833	; 0x40000009
800121bc:	00000020 	andeq	r0, r0, r0, lsr #32
800121c0:	5f636d6d 	svcpl	0x00636d6d
800121c4:	00006473 	andeq	r6, r0, r3, ror r4
800121c8:	5d4b4f5b 	stclpl	15, cr4, [fp, #-364]	; 0xfffffe94
800121cc:	00000a0a 	andeq	r0, r0, sl, lsl #20
800121d0:	6961465b 	stmdbvs	r1!, {r0, r1, r3, r4, r6, r9, sl, lr}^
800121d4:	2164656c 	cmncs	r4, ip, ror #10
800121d8:	00000a5d 	andeq	r0, r0, sp, asr sl
800121dc:	3d3d0a0a 	vldmdbcc	sp!, {s0-s9}
800121e0:	6f77453d 	svcvs	0x0077453d
800121e4:	696d206b 	stmdbvs	sp!, {r0, r1, r3, r5, r6, sp}^
800121e8:	2d6f7263 	sfmcs	f7, 2, [pc, #-396]!	; 80012064 <_kernel_entry_c+0x3e4>
800121ec:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
800121f0:	3d3d6c65 	ldccc	12, cr6, [sp, #-404]!	; 0xfffffe6c
800121f4:	6b0a0a3d 	blvs	80294af0 <_framebuffer_base_raw+0x2baf0>
800121f8:	656e7265 	strbvs	r7, [lr, #-613]!	; 0x265
800121fc:	6d203a6c 	vstmdbvs	r0!, {s6-s113}
80012200:	6920756d 	stmdbvs	r0!, {r0, r2, r3, r5, r6, r8, sl, ip, sp, lr}
80012204:	6574696e 	ldrbvs	r6, [r4, #-2414]!	; 0x96e
80012208:	656b0a64 	strbvs	r0, [fp, #-2660]!	; 0xa64
8001220c:	6c656e72 	stclvs	14, cr6, [r5], #-456	; 0xfffffe38
80012210:	6175203a 	cmnvs	r5, sl, lsr r0
80012214:	69207472 	stmdbvs	r0!, {r1, r4, r5, r6, sl, ip, sp, lr}
80012218:	6574696e 	ldrbvs	r6, [r4, #-2414]!	; 0x96e
8001221c:	00000a64 	andeq	r0, r0, r4, ror #20
80012220:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
80012224:	203a6c65 	eorscs	r6, sl, r5, ror #24
80012228:	6c616d6b 	stclvs	13, cr6, [r1], #-428	; 0xfffffe54
8001222c:	20636f6c 	rsbcs	r6, r3, ip, ror #30
80012230:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
80012234:	20676e69 	rsbcs	r6, r7, r9, ror #28
80012238:	6b6f5b20 	blvs	81be8ec0 <_kernel_end+0x157fec0>
8001223c:	203a205d 	eorscs	r2, sl, sp, asr r0
80012240:	424d6425 	submi	r6, sp, #620756992	; 0x25000000
80012244:	0000000a 	andeq	r0, r0, sl
80012248:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
8001224c:	203a6c65 	eorscs	r6, sl, r5, ror #24
80012250:	6d617266 	sfmvs	f7, 2, [r1, #-408]!	; 0xfffffe68
80012254:	66756265 	ldrbtvs	r6, [r5], -r5, ror #4
80012258:	20726566 	rsbscs	r6, r2, r6, ror #10
8001225c:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
80012260:	0a676e69 	beq	819edc0c <_kernel_end+0x1384c0c>
80012264:	00000000 	andeq	r0, r0, r0
80012268:	5d4b4f5b 	stclpl	15, cr4, [fp, #-364]	; 0xfffffe94
8001226c:	25203a20 	strcs	r3, [r0, #-2592]!	; 0xa20
80012270:	64257864 	strtvs	r7, [r5], #-2148	; 0x864
80012274:	62642520 	rsbvs	r2, r4, #32, 10	; 0x8000000
80012278:	2c737469 	cfldrdcs	mvd7, [r3], #-420	; 0xfffffe5c
8001227c:	64646120 	strbtvs	r6, [r4], #-288	; 0x120
80012280:	30203a72 	eorcc	r3, r0, r2, ror sl
80012284:	2c582578 	cfldr64cs	mvdx2, [r8], {120}	; 0x78
80012288:	7a697320 	bvc	81a6ef10 <_kernel_end+0x1405f10>
8001228c:	64253a65 	strtvs	r3, [r5], #-2661	; 0xa65
80012290:	0000000a 	andeq	r0, r0, sl
80012294:	465b2020 	ldrbmi	r2, [fp], -r0, lsr #32
80012298:	656c6961 	strbvs	r6, [ip, #-2401]!	; 0x961
8001229c:	0a5d2164 	beq	8175a834 <_kernel_end+0x10f1834>
800122a0:	00000000 	andeq	r0, r0, r0
800122a4:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
800122a8:	203a6c65 	eorscs	r6, sl, r5, ror #24
800122ac:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
800122b0:	6c6c6120 	stfvse	f6, [ip], #-128	; 0xffffff80
800122b4:	6261636f 	rsbvs	r6, r1, #-1140850687	; 0xbc000001
800122b8:	6d20656c 	cfstr32vs	mvfx6, [r0, #-432]!	; 0xfffffe50
800122bc:	726f6d65 	rsbvc	r6, pc, #6464	; 0x1940
800122c0:	25203a79 	strcs	r3, [r0, #-2681]!	; 0xa79
800122c4:	0a424d64 	beq	810a585c <_kernel_end+0xa3c85c>
800122c8:	00000000 	andeq	r0, r0, r0
800122cc:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
800122d0:	203a6c65 	eorscs	r6, sl, r5, ror #24
800122d4:	69766564 	ldmdbvs	r6!, {r2, r5, r6, r8, sl, sp, lr}^
800122d8:	20736563 	rsbscs	r6, r3, r3, ror #10
800122dc:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
800122e0:	2e676e69 	cdpcs	14, 6, cr6, cr7, cr9, {3}
800122e4:	0000000a 	andeq	r0, r0, sl
800122e8:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
800122ec:	203a6c65 	eorscs	r6, sl, r5, ror #24
800122f0:	626f6c67 	rsbvs	r6, pc, #26368	; 0x6700
800122f4:	65206c61 	strvs	r6, [r0, #-3169]!	; 0xc61
800122f8:	6920766e 	stmdbvs	r0!, {r1, r2, r3, r5, r6, r9, sl, ip, sp, lr}
800122fc:	6574696e 	ldrbvs	r6, [r4, #-2414]!	; 0x96e
80012300:	000a2e64 	andeq	r2, sl, r4, ror #28
80012304:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
80012308:	203a6c65 	eorscs	r6, sl, r5, ror #24
8001230c:	72616873 	rsbvc	r6, r1, #7536640	; 0x730000
80012310:	656d2065 	strbvs	r2, [sp, #-101]!	; 0x65
80012314:	79726f6d 	ldmdbvc	r2!, {r0, r2, r3, r5, r6, r8, r9, sl, fp, sp, lr}^
80012318:	696e6920 	stmdbvs	lr!, {r5, r8, fp, sp, lr}^
8001231c:	2e646574 	mcrcs	5, 3, r6, cr4, cr4, {3}
80012320:	0000000a 	andeq	r0, r0, sl
80012324:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
80012328:	203a6c65 	eorscs	r6, sl, r5, ror #24
8001232c:	636f7270 	cmnvs	pc, #112, 4
80012330:	65737365 	ldrbvs	r7, [r3, #-869]!	; 0x365
80012334:	6e692073 	mcrvs	0, 3, r2, cr9, cr3, {3}
80012338:	64657469 	strbtvs	r7, [r5], #-1129	; 0x469
8001233c:	00000a2e 	andeq	r0, r0, lr, lsr #20
80012340:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
80012344:	203a6c65 	eorscs	r6, sl, r5, ror #24
80012348:	20736676 	rsbscs	r6, r3, r6, ror r6
8001234c:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
80012350:	000a6465 	andeq	r6, sl, r5, ror #8
80012354:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
80012358:	203a6c65 	eorscs	r6, sl, r5, ror #24
8001235c:	20717269 	rsbscs	r7, r1, r9, ror #4
80012360:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
80012364:	000a6465 	andeq	r6, sl, r5, ror #8
80012368:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
8001236c:	203a6c65 	eorscs	r6, sl, r5, ror #24
80012370:	64616f6c 	strbtvs	r6, [r1], #-3948	; 0xf6c
80012374:	20676e69 	rsbcs	r6, r7, r9, ror #28
80012378:	74696e69 	strbtvc	r6, [r9], #-3689	; 0xe69
8001237c:	00000000 	andeq	r0, r0, r0
80012380:	6962732f 	stmdbvs	r2!, {r0, r1, r2, r3, r5, r8, r9, ip, sp, lr}^
80012384:	6e692f6e 	cdpvs	15, 6, cr2, cr9, cr14, {3}
80012388:	00007469 	andeq	r7, r0, r9, ror #8
8001238c:	61665b20 	cmnvs	r6, r0, lsr #22
80012390:	64656c69 	strbtvs	r6, [r5], #-3177	; 0xc69
80012394:	000a5d21 	andeq	r5, sl, r1, lsr #26
80012398:	6b6f5b20 	blvs	81be9020 <_kernel_end+0x1580020>
8001239c:	00000a5d 	andeq	r0, r0, sp, asr sl
800123a0:	6e72656b 	cdpvs	5, 7, cr6, cr2, cr11, {3}
800123a4:	203a6c65 	eorscs	r6, sl, r5, ror #24
800123a8:	72617473 	rsbvc	r7, r1, #1929379840	; 0x73000000
800123ac:	69742074 	ldmdbvs	r4!, {r2, r4, r5, r6, sp}^
800123b0:	2e72656d 	cdpcs	5, 7, cr6, cr2, cr13, {3}
800123b4:	0000000a 	andeq	r0, r0, sl

Disassembly of section .rodata:

800123b8 <fontdata_8x16>:
	...
800123c8:	817e0000 	cmnhi	lr, r0
800123cc:	bd8181a5 	stfltd	f0, [r1, #660]	; 0x294
800123d0:	7e818199 	mcrvc	1, 4, r8, cr1, cr9, {4}
800123d4:	00000000 	andeq	r0, r0, r0
800123d8:	ff7e0000 			; <UNDEFINED> instruction: 0xff7e0000
800123dc:	c3ffffdb 	mvnsgt	pc, #876	; 0x36c
800123e0:	7effffe7 	cdpvc	15, 15, cr15, cr15, cr7, {7}
	...
800123ec:	fefefe6c 	cdp2	14, 15, cr15, cr14, cr12, {3}
800123f0:	10387cfe 	ldrshtne	r7, [r8], -lr
	...
800123fc:	fe7c3810 	mrc2	8, 3, r3, cr12, cr0, {0}
80012400:	0010387c 	andseq	r3, r0, ip, ror r8
80012404:	00000000 	andeq	r0, r0, r0
80012408:	18000000 	stmdane	r0, {}	; <UNPREDICTABLE>
8001240c:	e7e73c3c 			; <UNDEFINED> instruction: 0xe7e73c3c
80012410:	3c1818e7 	ldccc	8, cr1, [r8], {231}	; 0xe7
80012414:	00000000 	andeq	r0, r0, r0
80012418:	18000000 	stmdane	r0, {}	; <UNPREDICTABLE>
8001241c:	ffff7e3c 			; <UNDEFINED> instruction: 0xffff7e3c
80012420:	3c18187e 	ldccc	8, cr1, [r8], {126}	; 0x7e
	...
8001242c:	3c180000 	ldccc	0, cr0, [r8], {-0}
80012430:	0000183c 	andeq	r1, r0, ip, lsr r8
80012434:	00000000 	andeq	r0, r0, r0
80012438:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
8001243c:	c3e7ffff 	mvngt	pc, #1020	; 0x3fc
80012440:	ffffe7c3 			; <UNDEFINED> instruction: 0xffffe7c3
80012444:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
80012448:	00000000 	andeq	r0, r0, r0
8001244c:	42663c00 	rsbmi	r3, r6, #0, 24
80012450:	003c6642 	eorseq	r6, ip, r2, asr #12
80012454:	00000000 	andeq	r0, r0, r0
80012458:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
8001245c:	bd99c3ff 	ldclt	3, cr12, [r9, #1020]	; 0x3fc
80012460:	ffc399bd 			; <UNDEFINED> instruction: 0xffc399bd
80012464:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
80012468:	0e1e0000 	cdpeq	0, 1, cr0, cr14, cr0, {0}
8001246c:	cc78321a 	lfmgt	f3, 2, [r8], #-104	; 0xffffff98
80012470:	78cccccc 	stmiavc	ip, {r2, r3, r6, r7, sl, fp, lr, pc}^
80012474:	00000000 	andeq	r0, r0, r0
80012478:	663c0000 	ldrtvs	r0, [ip], -r0
8001247c:	3c666666 	stclcc	6, cr6, [r6], #-408	; 0xfffffe68
80012480:	18187e18 	ldmdane	r8, {r3, r4, r9, sl, fp, ip, sp, lr}
80012484:	00000000 	andeq	r0, r0, r0
80012488:	333f0000 	teqcc	pc, #0
8001248c:	3030303f 	eorscc	r3, r0, pc, lsr r0
80012490:	e0f07030 	rscs	r7, r0, r0, lsr r0
80012494:	00000000 	andeq	r0, r0, r0
80012498:	637f0000 	cmnvs	pc, #0
8001249c:	6363637f 	cmnvs	r3, #-67108863	; 0xfc000001
800124a0:	e6e76763 	strbt	r6, [r7], r3, ror #14
800124a4:	000000c0 	andeq	r0, r0, r0, asr #1
800124a8:	18000000 	stmdane	r0, {}	; <UNPREDICTABLE>
800124ac:	e73cdb18 			; <UNDEFINED> instruction: 0xe73cdb18
800124b0:	1818db3c 	ldmdane	r8, {r2, r3, r4, r5, r8, r9, fp, ip, lr, pc}
800124b4:	00000000 	andeq	r0, r0, r0
800124b8:	e0c08000 	sbc	r8, r0, r0
800124bc:	f8fef8f0 			; <UNDEFINED> instruction: 0xf8fef8f0
800124c0:	80c0e0f0 	strdhi	lr, [r0], #0
800124c4:	00000000 	andeq	r0, r0, r0
800124c8:	0e060200 	cdpeq	2, 0, cr0, cr6, cr0, {0}
800124cc:	3efe3e1e 	mrccc	14, 7, r3, cr14, cr14, {0}
800124d0:	02060e1e 	andeq	r0, r6, #480	; 0x1e0
800124d4:	00000000 	andeq	r0, r0, r0
800124d8:	3c180000 	ldccc	0, cr0, [r8], {-0}
800124dc:	1818187e 	ldmdane	r8, {r1, r2, r3, r4, r5, r6, fp, ip}
800124e0:	00183c7e 	andseq	r3, r8, lr, ror ip
800124e4:	00000000 	andeq	r0, r0, r0
800124e8:	66660000 	strbtvs	r0, [r6], -r0
800124ec:	66666666 	strbtvs	r6, [r6], -r6, ror #12
800124f0:	66660066 	strbtvs	r0, [r6], -r6, rrx
800124f4:	00000000 	andeq	r0, r0, r0
800124f8:	db7f0000 	blle	81fd2500 <_kernel_end+0x1969500>
800124fc:	1b7bdbdb 	blne	81f09470 <_kernel_end+0x18a0470>
80012500:	1b1b1b1b 	blne	806d9174 <_kernel_end+0x70174>
80012504:	00000000 	andeq	r0, r0, r0
80012508:	60c67c00 	sbcvs	r7, r6, r0, lsl #24
8001250c:	c6c66c38 			; <UNDEFINED> instruction: 0xc6c66c38
80012510:	c60c386c 	strgt	r3, [ip], -ip, ror #16
80012514:	0000007c 	andeq	r0, r0, ip, ror r0
	...
80012520:	fefefefe 	mrc2	14, 7, pc, cr14, cr14, {7}
80012524:	00000000 	andeq	r0, r0, r0
80012528:	3c180000 	ldccc	0, cr0, [r8], {-0}
8001252c:	1818187e 	ldmdane	r8, {r1, r2, r3, r4, r5, r6, fp, ip}
80012530:	7e183c7e 	mrcvc	12, 0, r3, cr8, cr14, {3}
80012534:	00000000 	andeq	r0, r0, r0
80012538:	3c180000 	ldccc	0, cr0, [r8], {-0}
8001253c:	1818187e 	ldmdane	r8, {r1, r2, r3, r4, r5, r6, fp, ip}
80012540:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012544:	00000000 	andeq	r0, r0, r0
80012548:	18180000 	ldmdane	r8, {}	; <UNPREDICTABLE>
8001254c:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012550:	183c7e18 	ldmdane	ip!, {r3, r4, r9, sl, fp, ip, sp, lr}
	...
8001255c:	fe0c1800 	cdp2	8, 0, cr1, cr12, cr0, {0}
80012560:	0000180c 	andeq	r1, r0, ip, lsl #16
	...
8001256c:	fe603000 	cdp2	0, 6, cr3, cr0, cr0, {0}
80012570:	00003060 	andeq	r3, r0, r0, rrx
	...
8001257c:	c0c00000 	sbcgt	r0, r0, r0
80012580:	0000fec0 	andeq	pc, r0, r0, asr #29
	...
8001258c:	fe6c2800 	cdp2	8, 6, cr2, cr12, cr0, {0}
80012590:	0000286c 	andeq	r2, r0, ip, ror #16
	...
8001259c:	7c383810 	ldcvc	8, cr3, [r8], #-64	; 0xffffffc0
800125a0:	00fefe7c 	rscseq	pc, lr, ip, ror lr	; <UNPREDICTABLE>
	...
800125ac:	7c7cfefe 	ldclvc	14, cr15, [ip], #-1016	; 0xfffffc08
800125b0:	00103838 	andseq	r3, r0, r8, lsr r8
	...
800125c8:	3c180000 	ldccc	0, cr0, [r8], {-0}
800125cc:	18183c3c 	ldmdane	r8, {r2, r3, r4, r5, sl, fp, ip, sp}
800125d0:	18180018 	ldmdane	r8, {r3, r4}
800125d4:	00000000 	andeq	r0, r0, r0
800125d8:	66666600 	strbtvs	r6, [r6], -r0, lsl #12
800125dc:	00000024 	andeq	r0, r0, r4, lsr #32
	...
800125e8:	6c000000 	stcvs	0, cr0, [r0], {-0}
800125ec:	6c6cfe6c 	stclvs	14, cr15, [ip], #-432	; 0xfffffe50
800125f0:	6c6cfe6c 	stclvs	14, cr15, [ip], #-432	; 0xfffffe50
800125f4:	00000000 	andeq	r0, r0, r0
800125f8:	c67c1818 			; <UNDEFINED> instruction: 0xc67c1818
800125fc:	067cc0c2 	ldrbteq	ip, [ip], -r2, asr #1
80012600:	7cc68606 	stclvc	6, cr8, [r6], {6}
80012604:	00001818 	andeq	r1, r0, r8, lsl r8
80012608:	00000000 	andeq	r0, r0, r0
8001260c:	180cc6c2 	stmdane	ip, {r1, r6, r7, r9, sl, lr, pc}
80012610:	86c66030 			; <UNDEFINED> instruction: 0x86c66030
80012614:	00000000 	andeq	r0, r0, r0
80012618:	6c380000 	ldcvs	0, cr0, [r8], #-0
8001261c:	dc76386c 	ldclle	8, cr3, [r6], #-432	; 0xfffffe50
80012620:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012624:	00000000 	andeq	r0, r0, r0
80012628:	30303000 	eorscc	r3, r0, r0
8001262c:	00000060 	andeq	r0, r0, r0, rrx
	...
80012638:	180c0000 	stmdane	ip, {}	; <UNPREDICTABLE>
8001263c:	30303030 	eorscc	r3, r0, r0, lsr r0
80012640:	0c183030 	ldceq	0, cr3, [r8], {48}	; 0x30
80012644:	00000000 	andeq	r0, r0, r0
80012648:	18300000 	ldmdane	r0!, {}	; <UNPREDICTABLE>
8001264c:	0c0c0c0c 	stceq	12, cr0, [ip], {12}
80012650:	30180c0c 	andscc	r0, r8, ip, lsl #24
	...
8001265c:	ff3c6600 			; <UNDEFINED> instruction: 0xff3c6600
80012660:	0000663c 	andeq	r6, r0, ip, lsr r6
	...
8001266c:	7e181800 	cdpvc	8, 1, cr1, cr8, cr0, {0}
80012670:	00001818 	andeq	r1, r0, r8, lsl r8
	...
80012680:	18181800 	ldmdane	r8, {fp, ip}
80012684:	00000030 	andeq	r0, r0, r0, lsr r0
80012688:	00000000 	andeq	r0, r0, r0
8001268c:	fe000000 	cdp2	0, 0, cr0, cr0, cr0, {0}
	...
800126a0:	18180000 	ldmdane	r8, {}	; <UNPREDICTABLE>
	...
800126ac:	180c0602 	stmdane	ip, {r1, r9, sl}
800126b0:	80c06030 	sbchi	r6, r0, r0, lsr r0
800126b4:	00000000 	andeq	r0, r0, r0
800126b8:	6c380000 	ldcvs	0, cr0, [r8], #-0
800126bc:	d6d6c6c6 	ldrble	ip, [r6], r6, asr #13
800126c0:	386cc6c6 	stmdacc	ip!, {r1, r2, r6, r7, r9, sl, lr, pc}^
800126c4:	00000000 	andeq	r0, r0, r0
800126c8:	38180000 	ldmdacc	r8, {}	; <UNPREDICTABLE>
800126cc:	18181878 	ldmdane	r8, {r3, r4, r5, r6, fp, ip}
800126d0:	7e181818 	mrcvc	8, 0, r1, cr8, cr8, {0}
800126d4:	00000000 	andeq	r0, r0, r0
800126d8:	c67c0000 	ldrbtgt	r0, [ip], -r0
800126dc:	30180c06 	andscc	r0, r8, r6, lsl #24
800126e0:	fec6c060 	cdp2	0, 12, cr12, cr6, cr0, {3}
800126e4:	00000000 	andeq	r0, r0, r0
800126e8:	c67c0000 	ldrbtgt	r0, [ip], -r0
800126ec:	063c0606 	ldrteq	r0, [ip], -r6, lsl #12
800126f0:	7cc60606 	stclvc	6, cr0, [r6], {6}
800126f4:	00000000 	andeq	r0, r0, r0
800126f8:	1c0c0000 	stcne	0, cr0, [ip], {-0}
800126fc:	fecc6c3c 	mcr2	12, 6, r6, cr12, cr12, {1}
80012700:	1e0c0c0c 	cdpne	12, 0, cr0, cr12, cr12, {0}
80012704:	00000000 	andeq	r0, r0, r0
80012708:	c0fe0000 	rscsgt	r0, lr, r0
8001270c:	06fcc0c0 	ldrbteq	ip, [ip], r0, asr #1
80012710:	7cc60606 	stclvc	6, cr0, [r6], {6}
80012714:	00000000 	andeq	r0, r0, r0
80012718:	60380000 	eorsvs	r0, r8, r0
8001271c:	c6fcc0c0 	ldrbtgt	ip, [ip], r0, asr #1
80012720:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012724:	00000000 	andeq	r0, r0, r0
80012728:	c6fe0000 	ldrbtgt	r0, [lr], r0
8001272c:	180c0606 	stmdane	ip, {r1, r2, r9, sl}
80012730:	30303030 	eorscc	r3, r0, r0, lsr r0
80012734:	00000000 	andeq	r0, r0, r0
80012738:	c67c0000 	ldrbtgt	r0, [ip], -r0
8001273c:	c67cc6c6 	ldrbtgt	ip, [ip], -r6, asr #13
80012740:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012744:	00000000 	andeq	r0, r0, r0
80012748:	c67c0000 	ldrbtgt	r0, [ip], -r0
8001274c:	067ec6c6 	ldrbteq	ip, [lr], -r6, asr #13
80012750:	780c0606 	stmdavc	ip, {r1, r2, r9, sl}
	...
8001275c:	00001818 	andeq	r1, r0, r8, lsl r8
80012760:	00181800 	andseq	r1, r8, r0, lsl #16
	...
8001276c:	00001818 	andeq	r1, r0, r8, lsl r8
80012770:	30181800 	andscc	r1, r8, r0, lsl #16
80012774:	00000000 	andeq	r0, r0, r0
80012778:	06000000 	streq	r0, [r0], -r0
8001277c:	6030180c 	eorsvs	r1, r0, ip, lsl #16
80012780:	060c1830 			; <UNDEFINED> instruction: 0x060c1830
	...
8001278c:	00007e00 	andeq	r7, r0, r0, lsl #28
80012790:	0000007e 	andeq	r0, r0, lr, ror r0
80012794:	00000000 	andeq	r0, r0, r0
80012798:	60000000 	andvs	r0, r0, r0
8001279c:	060c1830 			; <UNDEFINED> instruction: 0x060c1830
800127a0:	6030180c 	eorsvs	r1, r0, ip, lsl #16
800127a4:	00000000 	andeq	r0, r0, r0
800127a8:	c67c0000 	ldrbtgt	r0, [ip], -r0
800127ac:	18180cc6 	ldmdane	r8, {r1, r2, r6, r7, sl, fp}
800127b0:	18180018 	ldmdane	r8, {r3, r4}
800127b4:	00000000 	andeq	r0, r0, r0
800127b8:	7c000000 	stcvc	0, cr0, [r0], {-0}
800127bc:	dedec6c6 	cdple	6, 13, cr12, cr14, cr6, {6}
800127c0:	7cc0dcde 	stclvc	12, cr13, [r0], {222}	; 0xde
800127c4:	00000000 	andeq	r0, r0, r0
800127c8:	38100000 	ldmdacc	r0, {}	; <UNPREDICTABLE>
800127cc:	fec6c66c 	cdp2	6, 12, cr12, cr6, cr12, {3}
800127d0:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
800127d4:	00000000 	andeq	r0, r0, r0
800127d8:	66fc0000 	ldrbtvs	r0, [ip], r0
800127dc:	667c6666 	ldrbtvs	r6, [ip], -r6, ror #12
800127e0:	fc666666 	stc2l	6, cr6, [r6], #-408	; 0xfffffe68
800127e4:	00000000 	andeq	r0, r0, r0
800127e8:	663c0000 	ldrtvs	r0, [ip], -r0
800127ec:	c0c0c0c2 	sbcgt	ip, r0, r2, asr #1
800127f0:	3c66c2c0 	sfmcc	f4, 3, [r6], #-768	; 0xfffffd00
800127f4:	00000000 	andeq	r0, r0, r0
800127f8:	6cf80000 	ldclvs	0, cr0, [r8]
800127fc:	66666666 	strbtvs	r6, [r6], -r6, ror #12
80012800:	f86c6666 			; <UNDEFINED> instruction: 0xf86c6666
80012804:	00000000 	andeq	r0, r0, r0
80012808:	66fe0000 	ldrbtvs	r0, [lr], r0
8001280c:	68786862 	ldmdavs	r8!, {r1, r5, r6, fp, sp, lr}^
80012810:	fe666260 	cdp2	2, 6, cr6, cr6, cr0, {3}
80012814:	00000000 	andeq	r0, r0, r0
80012818:	66fe0000 	ldrbtvs	r0, [lr], r0
8001281c:	68786862 	ldmdavs	r8!, {r1, r5, r6, fp, sp, lr}^
80012820:	f0606060 			; <UNDEFINED> instruction: 0xf0606060
80012824:	00000000 	andeq	r0, r0, r0
80012828:	663c0000 	ldrtvs	r0, [ip], -r0
8001282c:	dec0c0c2 	cdple	0, 12, cr12, cr0, cr2, {6}
80012830:	3a66c6c6 	bcc	819c4350 <_kernel_end+0x135b350>
80012834:	00000000 	andeq	r0, r0, r0
80012838:	c6c60000 	strbgt	r0, [r6], r0
8001283c:	c6fec6c6 	ldrbtgt	ip, [lr], r6, asr #13
80012840:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
80012844:	00000000 	andeq	r0, r0, r0
80012848:	183c0000 	ldmdane	ip!, {}	; <UNPREDICTABLE>
8001284c:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012850:	3c181818 	ldccc	8, cr1, [r8], {24}
80012854:	00000000 	andeq	r0, r0, r0
80012858:	0c1e0000 	ldceq	0, cr0, [lr], {-0}
8001285c:	0c0c0c0c 	stceq	12, cr0, [ip], {12}
80012860:	78cccccc 	stmiavc	ip, {r2, r3, r6, r7, sl, fp, lr, pc}^
80012864:	00000000 	andeq	r0, r0, r0
80012868:	66e60000 	strbtvs	r0, [r6], r0
8001286c:	78786c66 	ldmdavc	r8!, {r1, r2, r5, r6, sl, fp, sp, lr}^
80012870:	e666666c 	strbt	r6, [r6], -ip, ror #12
80012874:	00000000 	andeq	r0, r0, r0
80012878:	60f00000 	rscsvs	r0, r0, r0
8001287c:	60606060 	rsbvs	r6, r0, r0, rrx
80012880:	fe666260 	cdp2	2, 6, cr6, cr6, cr0, {3}
80012884:	00000000 	andeq	r0, r0, r0
80012888:	eec60000 	cdp	0, 12, cr0, cr6, cr0, {0}
8001288c:	c6d6fefe 			; <UNDEFINED> instruction: 0xc6d6fefe
80012890:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
80012894:	00000000 	andeq	r0, r0, r0
80012898:	e6c60000 	strb	r0, [r6], r0
8001289c:	cedefef6 	mrcgt	14, 6, APSR_nzcv, cr14, cr6, {7}
800128a0:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
800128a4:	00000000 	andeq	r0, r0, r0
800128a8:	c67c0000 	ldrbtgt	r0, [ip], -r0
800128ac:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
800128b0:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
800128b4:	00000000 	andeq	r0, r0, r0
800128b8:	66fc0000 	ldrbtvs	r0, [ip], r0
800128bc:	607c6666 	rsbsvs	r6, ip, r6, ror #12
800128c0:	f0606060 			; <UNDEFINED> instruction: 0xf0606060
800128c4:	00000000 	andeq	r0, r0, r0
800128c8:	c67c0000 	ldrbtgt	r0, [ip], -r0
800128cc:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
800128d0:	7cded6c6 	ldclvc	6, cr13, [lr], {198}	; 0xc6
800128d4:	00000e0c 	andeq	r0, r0, ip, lsl #28
800128d8:	66fc0000 	ldrbtvs	r0, [ip], r0
800128dc:	6c7c6666 	ldclvs	6, cr6, [ip], #-408	; 0xfffffe68
800128e0:	e6666666 	strbt	r6, [r6], -r6, ror #12
800128e4:	00000000 	andeq	r0, r0, r0
800128e8:	c67c0000 	ldrbtgt	r0, [ip], -r0
800128ec:	0c3860c6 	ldceq	0, cr6, [r8], #-792	; 0xfffffce8
800128f0:	7cc6c606 	stclvc	6, cr12, [r6], {6}
800128f4:	00000000 	andeq	r0, r0, r0
800128f8:	7e7e0000 	cdpvc	0, 7, cr0, cr14, cr0, {0}
800128fc:	1818185a 	ldmdane	r8, {r1, r3, r4, r6, fp, ip}
80012900:	3c181818 	ldccc	8, cr1, [r8], {24}
80012904:	00000000 	andeq	r0, r0, r0
80012908:	c6c60000 	strbgt	r0, [r6], r0
8001290c:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
80012910:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012914:	00000000 	andeq	r0, r0, r0
80012918:	c6c60000 	strbgt	r0, [r6], r0
8001291c:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
80012920:	10386cc6 	eorsne	r6, r8, r6, asr #25
80012924:	00000000 	andeq	r0, r0, r0
80012928:	c6c60000 	strbgt	r0, [r6], r0
8001292c:	d6d6c6c6 	ldrble	ip, [r6], r6, asr #13
80012930:	6ceefed6 	stclvs	14, cr15, [lr], #856	; 0x358
80012934:	00000000 	andeq	r0, r0, r0
80012938:	c6c60000 	strbgt	r0, [r6], r0
8001293c:	38387c6c 	ldmdacc	r8!, {r2, r3, r5, r6, sl, fp, ip, sp, lr}
80012940:	c6c66c7c 	uxtab16gt	r6, r6, ip, ROR #24
80012944:	00000000 	andeq	r0, r0, r0
80012948:	66660000 	strbtvs	r0, [r6], -r0
8001294c:	183c6666 	ldmdane	ip!, {r1, r2, r5, r6, r9, sl, sp, lr}
80012950:	3c181818 	ldccc	8, cr1, [r8], {24}
80012954:	00000000 	andeq	r0, r0, r0
80012958:	c6fe0000 	ldrbtgt	r0, [lr], r0
8001295c:	30180c86 	andscc	r0, r8, r6, lsl #25
80012960:	fec6c260 	cdp2	2, 12, cr12, cr6, cr0, {3}
80012964:	00000000 	andeq	r0, r0, r0
80012968:	303c0000 	eorscc	r0, ip, r0
8001296c:	30303030 	eorscc	r3, r0, r0, lsr r0
80012970:	3c303030 	ldccc	0, cr3, [r0], #-192	; 0xffffff40
80012974:	00000000 	andeq	r0, r0, r0
80012978:	80000000 	andhi	r0, r0, r0
8001297c:	3870e0c0 	ldmdacc	r0!, {r6, r7, sp, lr, pc}^
80012980:	02060e1c 	andeq	r0, r6, #28, 28	; 0x1c0
80012984:	00000000 	andeq	r0, r0, r0
80012988:	0c3c0000 	ldceq	0, cr0, [ip], #-0
8001298c:	0c0c0c0c 	stceq	12, cr0, [ip], {12}
80012990:	3c0c0c0c 	stccc	12, cr0, [ip], {12}
80012994:	00000000 	andeq	r0, r0, r0
80012998:	c66c3810 			; <UNDEFINED> instruction: 0xc66c3810
	...
800129b4:	0000ff00 	andeq	pc, r0, r0, lsl #30
800129b8:	0c183000 	ldceq	0, cr3, [r8], {-0}
	...
800129cc:	7c0c7800 	stcvc	8, cr7, [ip], {-0}
800129d0:	76cccccc 	strbvc	ip, [ip], ip, asr #25
800129d4:	00000000 	andeq	r0, r0, r0
800129d8:	60e00000 	rscvs	r0, r0, r0
800129dc:	666c7860 	strbtvs	r7, [ip], -r0, ror #16
800129e0:	7c666666 	stclvc	6, cr6, [r6], #-408	; 0xfffffe68
	...
800129ec:	c0c67c00 	sbcgt	r7, r6, r0, lsl #24
800129f0:	7cc6c0c0 	stclvc	0, cr12, [r6], {192}	; 0xc0
800129f4:	00000000 	andeq	r0, r0, r0
800129f8:	0c1c0000 	ldceq	0, cr0, [ip], {-0}
800129fc:	cc6c3c0c 	stclgt	12, cr3, [ip], #-48	; 0xffffffd0
80012a00:	76cccccc 	strbvc	ip, [ip], ip, asr #25
	...
80012a0c:	fec67c00 	cdp2	12, 12, cr7, cr6, cr0, {0}
80012a10:	7cc6c0c0 	stclvc	0, cr12, [r6], {192}	; 0xc0
80012a14:	00000000 	andeq	r0, r0, r0
80012a18:	361c0000 	ldrcc	r0, [ip], -r0
80012a1c:	30783032 	rsbscc	r3, r8, r2, lsr r0
80012a20:	78303030 	ldmdavc	r0!, {r4, r5, ip, sp}
	...
80012a2c:	cccc7600 	stclgt	6, cr7, [ip], {0}
80012a30:	7ccccccc 	stclvc	12, cr12, [ip], {204}	; 0xcc
80012a34:	0078cc0c 	rsbseq	ip, r8, ip, lsl #24
80012a38:	60e00000 	rscvs	r0, r0, r0
80012a3c:	66766c60 	ldrbtvs	r6, [r6], -r0, ror #24
80012a40:	e6666666 	strbt	r6, [r6], -r6, ror #12
80012a44:	00000000 	andeq	r0, r0, r0
80012a48:	18180000 	ldmdane	r8, {}	; <UNPREDICTABLE>
80012a4c:	18183800 	ldmdane	r8, {fp, ip, sp}
80012a50:	3c181818 	ldccc	8, cr1, [r8], {24}
80012a54:	00000000 	andeq	r0, r0, r0
80012a58:	06060000 	streq	r0, [r6], -r0
80012a5c:	06060e00 	streq	r0, [r6], -r0, lsl #28
80012a60:	06060606 	streq	r0, [r6], -r6, lsl #12
80012a64:	003c6666 	eorseq	r6, ip, r6, ror #12
80012a68:	60e00000 	rscvs	r0, r0, r0
80012a6c:	786c6660 	stmdavc	ip!, {r5, r6, r9, sl, sp, lr}^
80012a70:	e6666c78 			; <UNDEFINED> instruction: 0xe6666c78
80012a74:	00000000 	andeq	r0, r0, r0
80012a78:	18380000 	ldmdane	r8!, {}	; <UNPREDICTABLE>
80012a7c:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012a80:	3c181818 	ldccc	8, cr1, [r8], {24}
	...
80012a8c:	d6feec00 	ldrbtle	lr, [lr], r0, lsl #24
80012a90:	c6d6d6d6 			; <UNDEFINED> instruction: 0xc6d6d6d6
	...
80012a9c:	6666dc00 	strbtvs	sp, [r6], -r0, lsl #24
80012aa0:	66666666 	strbtvs	r6, [r6], -r6, ror #12
	...
80012aac:	c6c67c00 	strbgt	r7, [r6], r0, lsl #24
80012ab0:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
	...
80012abc:	6666dc00 	strbtvs	sp, [r6], -r0, lsl #24
80012ac0:	7c666666 	stclvc	6, cr6, [r6], #-408	; 0xfffffe68
80012ac4:	00f06060 	rscseq	r6, r0, r0, rrx
80012ac8:	00000000 	andeq	r0, r0, r0
80012acc:	cccc7600 	stclgt	6, cr7, [ip], {0}
80012ad0:	7ccccccc 	stclvc	12, cr12, [ip], {204}	; 0xcc
80012ad4:	001e0c0c 	andseq	r0, lr, ip, lsl #24
80012ad8:	00000000 	andeq	r0, r0, r0
80012adc:	6676dc00 	ldrbtvs	sp, [r6], -r0, lsl #24
80012ae0:	f0606060 			; <UNDEFINED> instruction: 0xf0606060
	...
80012aec:	60c67c00 	sbcvs	r7, r6, r0, lsl #24
80012af0:	7cc60c38 	stclvc	12, cr0, [r6], {56}	; 0x38
80012af4:	00000000 	andeq	r0, r0, r0
80012af8:	30100000 	andscc	r0, r0, r0
80012afc:	3030fc30 	eorscc	pc, r0, r0, lsr ip	; <UNPREDICTABLE>
80012b00:	1c363030 	ldcne	0, cr3, [r6], #-192	; 0xffffff40
	...
80012b0c:	cccccc00 	stclgt	12, cr12, [ip], {0}
80012b10:	76cccccc 	strbvc	ip, [ip], ip, asr #25
	...
80012b1c:	c6c6c600 	strbgt	ip, [r6], r0, lsl #12
80012b20:	386cc6c6 	stmdacc	ip!, {r1, r2, r6, r7, r9, sl, lr, pc}^
	...
80012b2c:	d6c6c600 	strble	ip, [r6], r0, lsl #12
80012b30:	6cfed6d6 	ldclvs	6, cr13, [lr], #856	; 0x358
	...
80012b3c:	386cc600 	stmdacc	ip!, {r9, sl, lr, pc}^
80012b40:	c66c3838 			; <UNDEFINED> instruction: 0xc66c3838
	...
80012b4c:	c6c6c600 	strbgt	ip, [r6], r0, lsl #12
80012b50:	7ec6c6c6 	cdpvc	6, 12, cr12, cr6, cr6, {6}
80012b54:	00f80c06 	rscseq	r0, r8, r6, lsl #24
80012b58:	00000000 	andeq	r0, r0, r0
80012b5c:	18ccfe00 	stmiane	ip, {r9, sl, fp, ip, sp, lr, pc}^
80012b60:	fec66030 	mcr2	0, 6, r6, cr6, cr0, {1}
80012b64:	00000000 	andeq	r0, r0, r0
80012b68:	180e0000 	stmdane	lr, {}	; <UNPREDICTABLE>
80012b6c:	18701818 	ldmdane	r0!, {r3, r4, fp, ip}^
80012b70:	0e181818 	mrceq	8, 0, r1, cr8, cr8, {0}
80012b74:	00000000 	andeq	r0, r0, r0
80012b78:	18180000 	ldmdane	r8, {}	; <UNPREDICTABLE>
80012b7c:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012b80:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012b84:	00000000 	andeq	r0, r0, r0
80012b88:	18700000 	ldmdane	r0!, {}^	; <UNPREDICTABLE>
80012b8c:	180e1818 	stmdane	lr, {r3, r4, fp, ip}
80012b90:	70181818 	andsvc	r1, r8, r8, lsl r8
80012b94:	00000000 	andeq	r0, r0, r0
80012b98:	00dc7600 	sbcseq	r7, ip, r0, lsl #12
	...
80012bac:	c66c3810 			; <UNDEFINED> instruction: 0xc66c3810
80012bb0:	00fec6c6 	rscseq	ip, lr, r6, asr #13
80012bb4:	00000000 	andeq	r0, r0, r0
80012bb8:	663c0000 	ldrtvs	r0, [ip], -r0
80012bbc:	c0c0c0c2 	sbcgt	ip, r0, r2, asr #1
80012bc0:	3c66c2c0 	sfmcc	f4, 3, [r6], #-768	; 0xfffffd00
80012bc4:	00007018 	andeq	r7, r0, r8, lsl r0
80012bc8:	00cc0000 	sbceq	r0, ip, r0
80012bcc:	cccccc00 	stclgt	12, cr12, [ip], {0}
80012bd0:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012bd4:	00000000 	andeq	r0, r0, r0
80012bd8:	30180c00 	andscc	r0, r8, r0, lsl #24
80012bdc:	fec67c00 	cdp2	12, 12, cr7, cr6, cr0, {0}
80012be0:	7cc6c0c0 	stclvc	0, cr12, [r6], {192}	; 0xc0
80012be4:	00000000 	andeq	r0, r0, r0
80012be8:	6c381000 	ldcvs	0, cr1, [r8], #-0
80012bec:	7c0c7800 	stcvc	8, cr7, [ip], {-0}
80012bf0:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012bf4:	00000000 	andeq	r0, r0, r0
80012bf8:	00cc0000 	sbceq	r0, ip, r0
80012bfc:	7c0c7800 	stcvc	8, cr7, [ip], {-0}
80012c00:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012c04:	00000000 	andeq	r0, r0, r0
80012c08:	18306000 	ldmdane	r0!, {sp, lr}
80012c0c:	7c0c7800 	stcvc	8, cr7, [ip], {-0}
80012c10:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012c14:	00000000 	andeq	r0, r0, r0
80012c18:	386c3800 	stmdacc	ip!, {fp, ip, sp}^
80012c1c:	7c0c7800 	stcvc	8, cr7, [ip], {-0}
80012c20:	76cccccc 	strbvc	ip, [ip], ip, asr #25
	...
80012c2c:	c0c67c00 	sbcgt	r7, r6, r0, lsl #24
80012c30:	7cc6c0c0 	stclvc	0, cr12, [r6], {192}	; 0xc0
80012c34:	00007018 	andeq	r7, r0, r8, lsl r0
80012c38:	6c381000 	ldcvs	0, cr1, [r8], #-0
80012c3c:	fec67c00 	cdp2	12, 12, cr7, cr6, cr0, {0}
80012c40:	7cc6c0c0 	stclvc	0, cr12, [r6], {192}	; 0xc0
80012c44:	00000000 	andeq	r0, r0, r0
80012c48:	00c60000 	sbceq	r0, r6, r0
80012c4c:	fec67c00 	cdp2	12, 12, cr7, cr6, cr0, {0}
80012c50:	7cc6c0c0 	stclvc	0, cr12, [r6], {192}	; 0xc0
80012c54:	00000000 	andeq	r0, r0, r0
80012c58:	18306000 	ldmdane	r0!, {sp, lr}
80012c5c:	fec67c00 	cdp2	12, 12, cr7, cr6, cr0, {0}
80012c60:	7cc6c0c0 	stclvc	0, cr12, [r6], {192}	; 0xc0
80012c64:	00000000 	andeq	r0, r0, r0
80012c68:	00660000 	rsbeq	r0, r6, r0
80012c6c:	18183800 	ldmdane	r8, {fp, ip, sp}
80012c70:	3c181818 	ldccc	8, cr1, [r8], {24}
80012c74:	00000000 	andeq	r0, r0, r0
80012c78:	663c1800 	ldrtvs	r1, [ip], -r0, lsl #16
80012c7c:	18183800 	ldmdane	r8, {fp, ip, sp}
80012c80:	3c181818 	ldccc	8, cr1, [r8], {24}
80012c84:	00000000 	andeq	r0, r0, r0
80012c88:	18306000 	ldmdane	r0!, {sp, lr}
80012c8c:	18183800 	ldmdane	r8, {fp, ip, sp}
80012c90:	3c181818 	ldccc	8, cr1, [r8], {24}
80012c94:	00000000 	andeq	r0, r0, r0
80012c98:	1000c600 	andne	ip, r0, r0, lsl #12
80012c9c:	c6c66c38 			; <UNDEFINED> instruction: 0xc6c66c38
80012ca0:	c6c6c6fe 			; <UNDEFINED> instruction: 0xc6c6c6fe
80012ca4:	00000000 	andeq	r0, r0, r0
80012ca8:	10386c38 	eorsne	r6, r8, r8, lsr ip
80012cac:	fec66c38 	mcr2	12, 6, r6, cr6, cr8, {1}
80012cb0:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
80012cb4:	00000000 	andeq	r0, r0, r0
80012cb8:	fe00180c 	cdp2	8, 0, cr1, cr0, cr12, {0}
80012cbc:	78686266 	stmdavc	r8!, {r1, r2, r5, r6, r9, sp, lr}^
80012cc0:	fe666268 	cdp2	2, 6, cr6, cr6, cr8, {3}
	...
80012ccc:	3636ec00 	ldrtcc	lr, [r6], -r0, lsl #24
80012cd0:	6ed8d87e 	mrcvs	8, 6, sp, cr8, cr14, {3}
80012cd4:	00000000 	andeq	r0, r0, r0
80012cd8:	6c3e0000 	ldcvs	0, cr0, [lr], #-0
80012cdc:	ccfecccc 	ldclgt	12, cr12, [lr], #816	; 0x330
80012ce0:	cecccccc 	cdpgt	12, 12, cr12, cr12, cr12, {6}
80012ce4:	00000000 	andeq	r0, r0, r0
80012ce8:	6c381000 	ldcvs	0, cr1, [r8], #-0
80012cec:	c6c67c00 	strbgt	r7, [r6], r0, lsl #24
80012cf0:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012cf4:	00000000 	andeq	r0, r0, r0
80012cf8:	00c60000 	sbceq	r0, r6, r0
80012cfc:	c6c67c00 	strbgt	r7, [r6], r0, lsl #24
80012d00:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012d04:	00000000 	andeq	r0, r0, r0
80012d08:	18306000 	ldmdane	r0!, {sp, lr}
80012d0c:	c6c67c00 	strbgt	r7, [r6], r0, lsl #24
80012d10:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012d14:	00000000 	andeq	r0, r0, r0
80012d18:	cc783000 	ldclgt	0, cr3, [r8], #-0
80012d1c:	cccccc00 	stclgt	12, cr12, [ip], {0}
80012d20:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012d24:	00000000 	andeq	r0, r0, r0
80012d28:	18306000 	ldmdane	r0!, {sp, lr}
80012d2c:	cccccc00 	stclgt	12, cr12, [ip], {0}
80012d30:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012d34:	00000000 	andeq	r0, r0, r0
80012d38:	00c60000 	sbceq	r0, r6, r0
80012d3c:	c6c6c600 	strbgt	ip, [r6], r0, lsl #12
80012d40:	7ec6c6c6 	cdpvc	6, 12, cr12, cr6, cr6, {6}
80012d44:	00780c06 	rsbseq	r0, r8, r6, lsl #24
80012d48:	7c00c600 	stcvc	6, cr12, [r0], {-0}
80012d4c:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
80012d50:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012d54:	00000000 	andeq	r0, r0, r0
80012d58:	c600c600 	strgt	ip, [r0], -r0, lsl #12
80012d5c:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
80012d60:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012d64:	00000000 	andeq	r0, r0, r0
80012d68:	7c181800 	ldcvc	8, cr1, [r8], {-0}
80012d6c:	c0c0c0c6 	sbcgt	ip, r0, r6, asr #1
80012d70:	18187cc6 	ldmdane	r8, {r1, r2, r6, r7, sl, fp, ip, sp, lr}
80012d74:	00000000 	andeq	r0, r0, r0
80012d78:	646c3800 	strbtvs	r3, [ip], #-2048	; 0x800
80012d7c:	6060f060 	rsbvs	pc, r0, r0, rrx
80012d80:	fce66060 	stc2l	0, cr6, [r6], #384	; 0x180
80012d84:	00000000 	andeq	r0, r0, r0
80012d88:	66660000 	strbtvs	r0, [r6], -r0
80012d8c:	187e183c 	ldmdane	lr!, {r2, r3, r4, r5, fp, ip}^
80012d90:	1818187e 	ldmdane	r8, {r1, r2, r3, r4, r5, r6, fp, ip}
80012d94:	00000000 	andeq	r0, r0, r0
80012d98:	ccccf800 	stclgt	8, cr15, [ip], {0}
80012d9c:	deccc4f8 	mcrle	4, 6, ip, cr12, cr8, {7}
80012da0:	c6cccccc 	strbgt	ip, [ip], ip, asr #25
80012da4:	00000000 	andeq	r0, r0, r0
80012da8:	181b0e00 	ldmdane	fp, {r9, sl, fp}
80012dac:	187e1818 	ldmdane	lr!, {r3, r4, fp, ip}^
80012db0:	70d81818 	sbcsvc	r1, r8, r8, lsl r8
80012db4:	00000000 	andeq	r0, r0, r0
80012db8:	60301800 	eorsvs	r1, r0, r0, lsl #16
80012dbc:	7c0c7800 	stcvc	8, cr7, [ip], {-0}
80012dc0:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012dc4:	00000000 	andeq	r0, r0, r0
80012dc8:	30180c00 	andscc	r0, r8, r0, lsl #24
80012dcc:	18183800 	ldmdane	r8, {fp, ip, sp}
80012dd0:	3c181818 	ldccc	8, cr1, [r8], {24}
80012dd4:	00000000 	andeq	r0, r0, r0
80012dd8:	60301800 	eorsvs	r1, r0, r0, lsl #16
80012ddc:	c6c67c00 	strbgt	r7, [r6], r0, lsl #24
80012de0:	7cc6c6c6 	stclvc	6, cr12, [r6], {198}	; 0xc6
80012de4:	00000000 	andeq	r0, r0, r0
80012de8:	60301800 	eorsvs	r1, r0, r0, lsl #16
80012dec:	cccccc00 	stclgt	12, cr12, [ip], {0}
80012df0:	76cccccc 	strbvc	ip, [ip], ip, asr #25
80012df4:	00000000 	andeq	r0, r0, r0
80012df8:	dc760000 	ldclle	0, cr0, [r6], #-0
80012dfc:	6666dc00 	strbtvs	sp, [r6], -r0, lsl #24
80012e00:	66666666 	strbtvs	r6, [r6], -r6, ror #12
80012e04:	00000000 	andeq	r0, r0, r0
80012e08:	c600dc76 			; <UNDEFINED> instruction: 0xc600dc76
80012e0c:	defef6e6 	cdple	6, 15, cr15, cr14, cr6, {7}
80012e10:	c6c6c6ce 	strbgt	ip, [r6], lr, asr #13
80012e14:	00000000 	andeq	r0, r0, r0
80012e18:	6c3c0000 	ldcvs	0, cr0, [ip], #-0
80012e1c:	7e003e6c 	cdpvc	14, 0, cr3, cr0, cr12, {3}
	...
80012e28:	6c380000 	ldcvs	0, cr0, [r8], #-0
80012e2c:	7c00386c 	stcvc	8, cr3, [r0], {108}	; 0x6c
	...
80012e38:	30300000 	eorscc	r0, r0, r0
80012e3c:	60303000 	eorsvs	r3, r0, r0
80012e40:	7cc6c6c0 	stclvc	6, cr12, [r6], {192}	; 0xc0
	...
80012e4c:	c0fe0000 	rscsgt	r0, lr, r0
80012e50:	00c0c0c0 	sbceq	ip, r0, r0, asr #1
	...
80012e5c:	06fe0000 	ldrbteq	r0, [lr], r0
80012e60:	00060606 	andeq	r0, r6, r6, lsl #12
80012e64:	00000000 	andeq	r0, r0, r0
80012e68:	62e06000 	rscvs	r6, r0, #0
80012e6c:	30186c66 	andscc	r6, r8, r6, ror #24
80012e70:	0c86dc60 	stceq	12, cr13, [r6], {96}	; 0x60
80012e74:	00003e18 	andeq	r3, r0, r8, lsl lr
80012e78:	62e06000 	rscvs	r6, r0, #0
80012e7c:	30186c66 	andscc	r6, r8, r6, ror #24
80012e80:	3f9ace66 	svccc	0x009ace66
80012e84:	00000606 	andeq	r0, r0, r6, lsl #12
80012e88:	18180000 	ldmdane	r8, {}	; <UNPREDICTABLE>
80012e8c:	18181800 	ldmdane	r8, {fp, ip}
80012e90:	183c3c3c 	ldmdane	ip!, {r2, r3, r4, r5, sl, fp, ip, sp}
	...
80012e9c:	d86c3600 	stmdale	ip!, {r9, sl, ip, sp}^
80012ea0:	0000366c 	andeq	r3, r0, ip, ror #12
	...
80012eac:	366cd800 	strbtcc	sp, [ip], -r0, lsl #16
80012eb0:	0000d86c 	andeq	sp, r0, ip, ror #16
80012eb4:	00000000 	andeq	r0, r0, r0
80012eb8:	44114411 	ldrmi	r4, [r1], #-1041	; 0x411
80012ebc:	44114411 	ldrmi	r4, [r1], #-1041	; 0x411
80012ec0:	44114411 	ldrmi	r4, [r1], #-1041	; 0x411
80012ec4:	44114411 	ldrmi	r4, [r1], #-1041	; 0x411
80012ec8:	aa55aa55 	bge	8157d824 <_kernel_end+0xf14824>
80012ecc:	aa55aa55 	bge	8157d828 <_kernel_end+0xf14828>
80012ed0:	aa55aa55 	bge	8157d82c <_kernel_end+0xf1482c>
80012ed4:	aa55aa55 	bge	8157d830 <_kernel_end+0xf14830>
80012ed8:	77dd77dd 			; <UNDEFINED> instruction: 0x77dd77dd
80012edc:	77dd77dd 			; <UNDEFINED> instruction: 0x77dd77dd
80012ee0:	77dd77dd 			; <UNDEFINED> instruction: 0x77dd77dd
80012ee4:	77dd77dd 			; <UNDEFINED> instruction: 0x77dd77dd
80012ee8:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012eec:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012ef0:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012ef4:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012ef8:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012efc:	f8181818 			; <UNDEFINED> instruction: 0xf8181818
80012f00:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f04:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f08:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f0c:	f818f818 			; <UNDEFINED> instruction: 0xf818f818
80012f10:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f14:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f18:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f1c:	f6363636 			; <UNDEFINED> instruction: 0xf6363636
80012f20:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f24:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f28:	00000000 	andeq	r0, r0, r0
80012f2c:	fe000000 	cdp2	0, 0, cr0, cr0, cr0, {0}
80012f30:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f34:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f38:	00000000 	andeq	r0, r0, r0
80012f3c:	f818f800 			; <UNDEFINED> instruction: 0xf818f800
80012f40:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f44:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f48:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f4c:	f606f636 			; <UNDEFINED> instruction: 0xf606f636
80012f50:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f54:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f58:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f5c:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f60:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f64:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f68:	00000000 	andeq	r0, r0, r0
80012f6c:	f606fe00 			; <UNDEFINED> instruction: 0xf606fe00
80012f70:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f74:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f78:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f7c:	fe06f636 	mcr2	6, 0, pc, cr6, cr6, {1}	; <UNPREDICTABLE>
	...
80012f88:	36363636 			; <UNDEFINED> instruction: 0x36363636
80012f8c:	fe363636 	mrc2	6, 1, r3, cr6, cr6, {1}
	...
80012f98:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012f9c:	f818f818 			; <UNDEFINED> instruction: 0xf818f818
	...
80012fac:	f8000000 			; <UNDEFINED> instruction: 0xf8000000
80012fb0:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012fb4:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012fb8:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012fbc:	1f181818 	svcne	0x00181818
	...
80012fc8:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012fcc:	ff181818 			; <UNDEFINED> instruction: 0xff181818
	...
80012fdc:	ff000000 			; <UNDEFINED> instruction: 0xff000000
80012fe0:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012fe4:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012fe8:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012fec:	1f181818 	svcne	0x00181818
80012ff0:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012ff4:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80012ff8:	00000000 	andeq	r0, r0, r0
80012ffc:	ff000000 			; <UNDEFINED> instruction: 0xff000000
	...
80013008:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
8001300c:	ff181818 			; <UNDEFINED> instruction: 0xff181818
80013010:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013014:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013018:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
8001301c:	1f181f18 	svcne	0x00181f18
80013020:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013024:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013028:	36363636 			; <UNDEFINED> instruction: 0x36363636
8001302c:	37363636 			; <UNDEFINED> instruction: 0x37363636
80013030:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013034:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013038:	36363636 			; <UNDEFINED> instruction: 0x36363636
8001303c:	3f303736 	svccc	0x00303736
	...
8001304c:	37303f00 	ldrcc	r3, [r0, -r0, lsl #30]!
80013050:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013054:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013058:	36363636 			; <UNDEFINED> instruction: 0x36363636
8001305c:	ff00f736 			; <UNDEFINED> instruction: 0xff00f736
	...
8001306c:	f700ff00 			; <UNDEFINED> instruction: 0xf700ff00
80013070:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013074:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013078:	36363636 			; <UNDEFINED> instruction: 0x36363636
8001307c:	37303736 			; <UNDEFINED> instruction: 0x37303736
80013080:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013084:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013088:	00000000 	andeq	r0, r0, r0
8001308c:	ff00ff00 			; <UNDEFINED> instruction: 0xff00ff00
	...
80013098:	36363636 			; <UNDEFINED> instruction: 0x36363636
8001309c:	f700f736 			; <UNDEFINED> instruction: 0xf700f736
800130a0:	36363636 			; <UNDEFINED> instruction: 0x36363636
800130a4:	36363636 			; <UNDEFINED> instruction: 0x36363636
800130a8:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
800130ac:	ff00ff18 			; <UNDEFINED> instruction: 0xff00ff18
	...
800130b8:	36363636 			; <UNDEFINED> instruction: 0x36363636
800130bc:	ff363636 			; <UNDEFINED> instruction: 0xff363636
	...
800130cc:	ff00ff00 			; <UNDEFINED> instruction: 0xff00ff00
800130d0:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
800130d4:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
800130d8:	00000000 	andeq	r0, r0, r0
800130dc:	ff000000 			; <UNDEFINED> instruction: 0xff000000
800130e0:	36363636 			; <UNDEFINED> instruction: 0x36363636
800130e4:	36363636 			; <UNDEFINED> instruction: 0x36363636
800130e8:	36363636 			; <UNDEFINED> instruction: 0x36363636
800130ec:	3f363636 	svccc	0x00363636
	...
800130f8:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
800130fc:	1f181f18 	svcne	0x00181f18
	...
8001310c:	1f181f00 	svcne	0x00181f00
80013110:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013114:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013118:	00000000 	andeq	r0, r0, r0
8001311c:	3f000000 	svccc	0x00000000
80013120:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013124:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013128:	36363636 			; <UNDEFINED> instruction: 0x36363636
8001312c:	ff363636 			; <UNDEFINED> instruction: 0xff363636
80013130:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013134:	36363636 			; <UNDEFINED> instruction: 0x36363636
80013138:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
8001313c:	ff18ff18 			; <UNDEFINED> instruction: 0xff18ff18
80013140:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013144:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013148:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
8001314c:	f8181818 			; <UNDEFINED> instruction: 0xf8181818
	...
8001315c:	1f000000 	svcne	0x00000000
80013160:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013164:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013168:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
8001316c:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
80013170:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
80013174:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
80013178:	00000000 	andeq	r0, r0, r0
8001317c:	ff000000 			; <UNDEFINED> instruction: 0xff000000
80013180:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
80013184:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
80013188:	f0f0f0f0 			; <UNDEFINED> instruction: 0xf0f0f0f0
8001318c:	f0f0f0f0 			; <UNDEFINED> instruction: 0xf0f0f0f0
80013190:	f0f0f0f0 			; <UNDEFINED> instruction: 0xf0f0f0f0
80013194:	f0f0f0f0 			; <UNDEFINED> instruction: 0xf0f0f0f0
80013198:	0f0f0f0f 	svceq	0x000f0f0f
8001319c:	0f0f0f0f 	svceq	0x000f0f0f
800131a0:	0f0f0f0f 	svceq	0x000f0f0f
800131a4:	0f0f0f0f 	svceq	0x000f0f0f
800131a8:	ffffffff 			; <UNDEFINED> instruction: 0xffffffff
800131ac:	00ffffff 	ldrshteq	pc, [pc], #255	; <UNPREDICTABLE>
	...
800131bc:	d8dc7600 	ldmle	ip, {r9, sl, ip, sp, lr}^
800131c0:	76dcd8d8 			; <UNDEFINED> instruction: 0x76dcd8d8
800131c4:	00000000 	andeq	r0, r0, r0
800131c8:	cc780000 	ldclgt	0, cr0, [r8], #-0
800131cc:	ccd8cccc 	ldclgt	12, cr12, [r8], {204}	; 0xcc
800131d0:	ccc6c6c6 	stclgt	6, cr12, [r6], {198}	; 0xc6
800131d4:	00000000 	andeq	r0, r0, r0
800131d8:	c6fe0000 	ldrbtgt	r0, [lr], r0
800131dc:	c0c0c0c6 	sbcgt	ip, r0, r6, asr #1
800131e0:	c0c0c0c0 	sbcgt	ip, r0, r0, asr #1
	...
800131ec:	6c6cfe00 	stclvs	14, cr15, [ip], #-0
800131f0:	6c6c6c6c 	stclvs	12, cr6, [ip], #-432	; 0xfffffe50
800131f4:	00000000 	andeq	r0, r0, r0
800131f8:	c6fe0000 	ldrbtgt	r0, [lr], r0
800131fc:	18183060 	ldmdane	r8, {r5, r6, ip, sp}
80013200:	fec66030 	mcr2	0, 6, r6, cr6, cr0, {1}
	...
8001320c:	d8d87e00 	ldmle	r8, {r9, sl, fp, ip, sp, lr}^
80013210:	70d8d8d8 	ldrsbvc	sp, [r8], #136	; 0x88
	...
8001321c:	66666600 	strbtvs	r6, [r6], -r0, lsl #12
80013220:	7c666666 	stclvc	6, cr6, [r6], #-408	; 0xfffffe68
80013224:	00c06060 	sbceq	r6, r0, r0, rrx
80013228:	00000000 	andeq	r0, r0, r0
8001322c:	1818dc76 	ldmdane	r8, {r1, r2, r4, r5, r6, sl, fp, ip, lr, pc}
80013230:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013234:	00000000 	andeq	r0, r0, r0
80013238:	187e0000 	ldmdane	lr!, {}^	; <UNPREDICTABLE>
8001323c:	6666663c 			; <UNDEFINED> instruction: 0x6666663c
80013240:	7e183c66 	cdpvc	12, 1, cr3, cr8, cr6, {3}
80013244:	00000000 	andeq	r0, r0, r0
80013248:	6c380000 	ldcvs	0, cr0, [r8], #-0
8001324c:	c6fec6c6 	ldrbtgt	ip, [lr], r6, asr #13
80013250:	386cc6c6 	stmdacc	ip!, {r1, r2, r6, r7, r9, sl, lr, pc}^
80013254:	00000000 	andeq	r0, r0, r0
80013258:	6c380000 	ldcvs	0, cr0, [r8], #-0
8001325c:	6cc6c6c6 	stclvs	6, cr12, [r6], {198}	; 0xc6
80013260:	ee6c6c6c 	cdp	12, 6, cr6, cr12, cr12, {3}
80013264:	00000000 	andeq	r0, r0, r0
80013268:	301e0000 	andscc	r0, lr, r0
8001326c:	663e0c18 			; <UNDEFINED> instruction: 0x663e0c18
80013270:	3c666666 	stclcc	6, cr6, [r6], #-408	; 0xfffffe68
	...
8001327c:	dbdb7e00 	blle	7f6f2a84 <framebuffer_size+0x7f2f2a84>
80013280:	00007edb 	ldrdeq	r7, [r0], -fp
80013284:	00000000 	andeq	r0, r0, r0
80013288:	03000000 	movweq	r0, #0
8001328c:	dbdb7e06 	blle	7f6f2aac <framebuffer_size+0x7f2f2aac>
80013290:	c0607ef3 	strdgt	r7, [r0], #-227	; 0xffffff1d	; <UNPREDICTABLE>
80013294:	00000000 	andeq	r0, r0, r0
80013298:	301c0000 	andscc	r0, ip, r0
8001329c:	607c6060 	rsbsvs	r6, ip, r0, rrx
800132a0:	1c306060 	ldcne	0, cr6, [r0], #-384	; 0xfffffe80
800132a4:	00000000 	andeq	r0, r0, r0
800132a8:	7c000000 	stcvc	0, cr0, [r0], {-0}
800132ac:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
800132b0:	c6c6c6c6 	strbgt	ip, [r6], r6, asr #13
	...
800132bc:	fe0000fe 	mcr2	0, 0, r0, cr0, cr14, {7}
800132c0:	00fe0000 	rscseq	r0, lr, r0
	...
800132cc:	187e1818 	ldmdane	lr!, {r3, r4, fp, ip}^
800132d0:	7e000018 	mcrvc	0, 0, r0, cr0, cr8, {0}
800132d4:	00000000 	andeq	r0, r0, r0
800132d8:	30000000 	andcc	r0, r0, r0
800132dc:	0c060c18 	stceq	12, cr0, [r6], {24}
800132e0:	7e003018 	mcrvc	0, 0, r3, cr0, cr8, {0}
800132e4:	00000000 	andeq	r0, r0, r0
800132e8:	0c000000 	stceq	0, cr0, [r0], {-0}
800132ec:	30603018 	rsbcc	r3, r0, r8, lsl r0
800132f0:	7e000c18 	mcrvc	12, 0, r0, cr0, cr8, {0}
800132f4:	00000000 	andeq	r0, r0, r0
800132f8:	1b0e0000 	blne	80393300 <_framebuffer_base_raw+0x12a300>
800132fc:	1818181b 	ldmdane	r8, {r0, r1, r3, r4, fp, ip}
80013300:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013304:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013308:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
8001330c:	18181818 	ldmdane	r8, {r3, r4, fp, ip}
80013310:	d8d8d818 	ldmle	r8, {r3, r4, fp, ip, lr, pc}^
80013314:	00000070 	andeq	r0, r0, r0, ror r0
80013318:	00000000 	andeq	r0, r0, r0
8001331c:	7e001800 	cdpvc	8, 0, cr1, cr0, cr0, {0}
80013320:	00001800 	andeq	r1, r0, r0, lsl #16
	...
8001332c:	00dc7600 	sbcseq	r7, ip, r0, lsl #12
80013330:	0000dc76 	andeq	sp, r0, r6, ror ip
80013334:	00000000 	andeq	r0, r0, r0
80013338:	6c6c3800 	stclvs	8, cr3, [ip], #-0
8001333c:	00000038 	andeq	r0, r0, r8, lsr r0
	...
8001334c:	18000000 	stmdane	r0, {}	; <UNPREDICTABLE>
80013350:	00000018 	andeq	r0, r0, r8, lsl r0
	...
8001335c:	18000000 	stmdane	r0, {}	; <UNPREDICTABLE>
	...
80013368:	0c0c0f00 	stceq	15, cr0, [ip], {-0}
8001336c:	ec0c0c0c 	stc	12, cr0, [ip], {12}
80013370:	1c3c6c6c 	ldcne	12, cr6, [ip], #-432	; 0xfffffe50
80013374:	00000000 	andeq	r0, r0, r0
80013378:	36366c00 	ldrtcc	r6, [r6], -r0, lsl #24
8001337c:	00363636 	eorseq	r3, r6, r6, lsr r6
	...
80013388:	0c663c00 	stcleq	12, cr3, [r6], #-0
8001338c:	007e3218 	rsbseq	r3, lr, r8, lsl r2
	...
8001339c:	7e7e7e7e 	mrcvc	14, 3, r7, cr14, cr14, {3}
800133a0:	007e7e7e 	rsbseq	r7, lr, lr, ror lr
	...

Disassembly of section .data:

80014000 <_fonts>:
80014000:	80011fa8 	andhi	r1, r1, r8, lsr #31
80014004:	80014010 	andhi	r4, r1, r0, lsl r0
80014008:	80012050 	andhi	r2, r1, r0, asr r0
8001400c:	00000000 	andeq	r0, r0, r0

80014010 <font_8x16>:
80014010:	00000000 	andeq	r0, r0, r0
80014014:	00000008 	andeq	r0, r0, r8
80014018:	00000010 	andeq	r0, r0, r0, lsl r0
8001401c:	800123b8 			; <UNDEFINED> instruction: 0x800123b8
	...

80018000 <_startup_page_dir>:
80018000:	00000802 	andeq	r0, r0, r2, lsl #16
80018004:	00100802 	andseq	r0, r0, r2, lsl #16
80018008:	00200802 	eoreq	r0, r0, r2, lsl #16
8001800c:	00300802 	eorseq	r0, r0, r2, lsl #16
80018010:	00400802 	subeq	r0, r0, r2, lsl #16
80018014:	00500802 	subseq	r0, r0, r2, lsl #16
80018018:	00600802 	rsbeq	r0, r0, r2, lsl #16
8001801c:	00700802 	rsbseq	r0, r0, r2, lsl #16
80018020:	00800802 	addeq	r0, r0, r2, lsl #16
80018024:	00900802 	addseq	r0, r0, r2, lsl #16
80018028:	00a00802 	adceq	r0, r0, r2, lsl #16
8001802c:	00b00802 	adcseq	r0, r0, r2, lsl #16
80018030:	00c00802 	sbceq	r0, r0, r2, lsl #16
80018034:	00d00802 	sbcseq	r0, r0, r2, lsl #16
80018038:	00e00802 	rsceq	r0, r0, r2, lsl #16
8001803c:	00f00802 	rscseq	r0, r0, r2, lsl #16
80018040:	01000802 	tsteq	r0, r2, lsl #16
80018044:	01100802 	tsteq	r0, r2, lsl #16
80018048:	01200802 			; <UNDEFINED> instruction: 0x01200802
8001804c:	01300802 	teqeq	r0, r2, lsl #16
80018050:	01400802 	cmpeq	r0, r2, lsl #16
80018054:	01500802 	cmpeq	r0, r2, lsl #16
80018058:	01600802 	cmneq	r0, r2, lsl #16
8001805c:	01700802 	cmneq	r0, r2, lsl #16
80018060:	01800802 	orreq	r0, r0, r2, lsl #16
80018064:	01900802 	orrseq	r0, r0, r2, lsl #16
80018068:	01a00802 	lsleq	r0, r2, #16
8001806c:	01b00802 	lslseq	r0, r2, #16
80018070:	01c00802 	biceq	r0, r0, r2, lsl #16
80018074:	01d00802 	bicseq	r0, r0, r2, lsl #16
80018078:	01e00802 	mvneq	r0, r2, lsl #16
8001807c:	01f00802 	mvnseq	r0, r2, lsl #16
	...
8001a000:	00000802 	andeq	r0, r0, r2, lsl #16
8001a004:	00100802 	andseq	r0, r0, r2, lsl #16
8001a008:	00200802 	eoreq	r0, r0, r2, lsl #16
8001a00c:	00300802 	eorseq	r0, r0, r2, lsl #16
8001a010:	00400802 	subeq	r0, r0, r2, lsl #16
8001a014:	00500802 	subseq	r0, r0, r2, lsl #16
8001a018:	00600802 	rsbeq	r0, r0, r2, lsl #16
8001a01c:	00700802 	rsbseq	r0, r0, r2, lsl #16
8001a020:	00800802 	addeq	r0, r0, r2, lsl #16
8001a024:	00900802 	addseq	r0, r0, r2, lsl #16
8001a028:	00a00802 	adceq	r0, r0, r2, lsl #16
8001a02c:	00b00802 	adcseq	r0, r0, r2, lsl #16
8001a030:	00c00802 	sbceq	r0, r0, r2, lsl #16
8001a034:	00d00802 	sbcseq	r0, r0, r2, lsl #16
8001a038:	00e00802 	rsceq	r0, r0, r2, lsl #16
8001a03c:	00f00802 	rscseq	r0, r0, r2, lsl #16
8001a040:	01000802 	tsteq	r0, r2, lsl #16
8001a044:	01100802 	tsteq	r0, r2, lsl #16
8001a048:	01200802 			; <UNDEFINED> instruction: 0x01200802
8001a04c:	01300802 	teqeq	r0, r2, lsl #16
8001a050:	01400802 	cmpeq	r0, r2, lsl #16
8001a054:	01500802 	cmpeq	r0, r2, lsl #16
8001a058:	01600802 	cmneq	r0, r2, lsl #16
8001a05c:	01700802 	cmneq	r0, r2, lsl #16
8001a060:	01800802 	orreq	r0, r0, r2, lsl #16
8001a064:	01900802 	orrseq	r0, r0, r2, lsl #16
8001a068:	01a00802 	lsleq	r0, r2, #16
8001a06c:	01b00802 	lslseq	r0, r2, #16
8001a070:	01c00802 	biceq	r0, r0, r2, lsl #16
8001a074:	01d00802 	bicseq	r0, r0, r2, lsl #16
8001a078:	01e00802 	mvneq	r0, r2, lsl #16
8001a07c:	01f00802 	mvnseq	r0, r2, lsl #16
	...

8001c000 <shmem_count>:
8001c000:	00000001 	andeq	r0, r0, r1

Disassembly of section .got:

8001c004 <.got>:
8001c004:	80234004 	eorhi	r4, r3, r4
8001c008:	802384a0 	eorhi	r8, r3, r0, lsr #9
8001c00c:	80008098 	mulhi	r0, r8, r0
8001c010:	80008960 	andhi	r8, r0, r0, ror #18
8001c014:	80238498 	mlahi	r3, r8, r4, r8
8001c018:	8023849c 	mlahi	r3, ip, r4, r8
8001c01c:	80008a50 	andhi	r8, r0, r0, asr sl
8001c020:	80669000 	rsbhi	r9, r6, r0
8001c024:	80008a0c 	andhi	r8, r0, ip, lsl #20
8001c028:	80008b1c 	andhi	r8, r0, ip, lsl fp
8001c02c:	80234000 	eorhi	r4, r3, r0
8001c030:	80269000 	eorhi	r9, r6, r0
8001c034:	800244bc 			; <UNDEFINED> instruction: 0x800244bc
8001c038:	80008068 	andhi	r8, r0, r8, rrx

Disassembly of section .got.plt:

8001c03c <_GLOBAL_OFFSET_TABLE_>:
	...

Disassembly of section .bss:

80020000 <_hw_info>:
	...

80020030 <_fbinfo>:
	...

80020058 <_sdc>:
	...

80020470 <_sys_usec_tic>:
80020470:	00000000 	andeq	r0, r0, r0

80020474 <_mailbox>:
80020474:	00000000 	andeq	r0, r0, r0

80020478 <last.4133>:
80020478:	00000000 	andeq	r0, r0, r0

8002047c <_str_result>:
	...

800204c0 <_partition>:
	...

800204d0 <_partitions>:
	...

80020510 <_len>:
80020510:	00000000 	andeq	r0, r0, r0

80020514 <_buf>:
	...

80020594 <_console>:
	...

80024000 <_free_list4k>:
	...

80024400 <_free_list1k>:
80024400:	00000000 	andeq	r0, r0, r0

80024404 <_kmalloc_mem_tail>:
80024404:	00000000 	andeq	r0, r0, r0

80024408 <_kmalloc>:
	...

80024420 <shmem_tail>:
80024420:	00000000 	andeq	r0, r0, r0

80024424 <_shm_head>:
80024424:	00000000 	andeq	r0, r0, r0

80024428 <_shm_tail>:
80024428:	00000000 	andeq	r0, r0, r0

8002442c <_uspace_int_table>:
	...

800244b0 <_timer_usec>:
	...

800244b8 <_timer_tic>:
800244b8:	00000000 	andeq	r0, r0, r0

800244bc <_kernel_tic>:
800244bc:	00000000 	andeq	r0, r0, r0

800244c0 <_global_envs>:
	...

80028000 <_proc_table>:
	...

80034000 <_proc_vm>:
	...

80234000 <_ready_proc>:
80234000:	00000000 	andeq	r0, r0, r0

80234004 <_current_proc>:
80234004:	00000000 	andeq	r0, r0, r0

80234008 <_msg_counter>:
80234008:	00000000 	andeq	r0, r0, r0

8023400c <_vfs_mounts>:
	...

8023810c <ret.4542>:
	...

8023830c <_vfs_root>:
8023830c:	00000000 	andeq	r0, r0, r0

80238310 <_devs>:
	...

80238498 <_mmio_base>:
80238498:	00000000 	andeq	r0, r0, r0

8023849c <_kernel_vm>:
8023849c:	00000000 	andeq	r0, r0, r0

802384a0 <_ram_holes>:
	...

Disassembly of section .ARM.attributes:

00000000 <.ARM.attributes>:
   0:	00002d41 	andeq	r2, r0, r1, asr #26
   4:	61656100 	cmnvs	r5, r0, lsl #2
   8:	01006962 	tsteq	r0, r2, ror #18
   c:	00000023 	andeq	r0, r0, r3, lsr #32
  10:	4d524105 	ldfmie	f4, [r2, #-20]	; 0xffffffec
  14:	45363239 	ldrmi	r3, [r6, #-569]!	; 0x239
  18:	00532d4a 	subseq	r2, r3, sl, asr #26
  1c:	01080506 	tsteq	r8, r6, lsl #10
  20:	04120109 	ldreq	r0, [r2], #-265	; 0x109
  24:	01150114 	tsteq	r5, r4, lsl r1
  28:	01180317 	tsteq	r8, r7, lsl r3
  2c:	Address 0x000000000000002c is out of bounds.


Disassembly of section .comment:

00000000 <.comment>:
   0:	3a434347 	bcc	10d0d24 <framebuffer_size+0xcd0d24>
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
  48:	652f4d52 	strvs	r4, [pc, #-3410]!	; fffff2fe <_kernel_end+0x7f9962fe>
  4c:	6465626d 	strbtvs	r6, [r5], #-621	; 0x26d
  50:	2d646564 	cfstr64cs	mvdx6, [r4, #-400]!	; 0xfffffe70
  54:	2d395f34 	ldccs	15, cr5, [r9, #-208]!	; 0xffffff30
  58:	6e617262 	cdpvs	2, 6, cr7, cr1, cr2, {3}
  5c:	72206863 	eorvc	r6, r0, #6488064	; 0x630000
  60:	73697665 	cmnvc	r9, #105906176	; 0x6500000
  64:	206e6f69 	rsbcs	r6, lr, r9, ror #30
  68:	39373232 	ldmdbcc	r7!, {r1, r4, r5, r9, ip, sp}
  6c:	005d3737 	subseq	r3, sp, r7, lsr r7
