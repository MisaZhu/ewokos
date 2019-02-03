
.global __saveContext1
__saveContext1: /*store registers to process content array, except R0*/
	push {r0-r12, r14}

	/* return address of process is passed in r0 */
	mov r14, r0
	
	push {r1-r12, r14}
	bl getCurrentContext /*kernel/src/proc.c, return the address of context array to R0 */
	pop {r1-r12, r14}

	/* if the process content is NULL, return */
	cmpeq r0, #0
	beq saveContextReturn

	/* store cpsr to item 0 of process content array*/
	push {r1}
	mrs r1, SPSR
	str r1, [r0]
	pop {r1}

	/* store return address to item 1 of process content array*/
	add r0, #4
	str r14, [r0]

	/* store the rest, except r0  to rest items(3~16, skip item 2!) of process content array*/
	add r0, #8 /*skip item 2*/
	stmia r0, {r1-r14}^

	b saveContextReturn


.global __saveContext2
__saveContext2: /*store R0 to item 2 of process content array*/
	push {r0-r12, r14}
	
	mov r1, r0

	/* load address of context array for current process into r0 */
	push {r1-r12, r14}
	bl getCurrentContext
	pop {r1-r12, r14}

	/* if the process content is NULL, return */
	cmpeq r0, #0
	beq saveContextReturn

	/* store r0 to the item 2*/
	add r0, #8
	str r1, [r0]

saveContextReturn:
	pop {r0-r12, r14}
	mov pc, lr
	
