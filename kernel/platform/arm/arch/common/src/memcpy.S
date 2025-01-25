.global __memcpy32
__memcpy32:
	push {r3-r10}
ldmloop:
	ldmia r1!, {r3 - r10}
	stmia r0!, {r3 - r10}
	subs r2, r2, #32
	bgt ldmloop
	pop {r3-r10}
	mov pc, lr
