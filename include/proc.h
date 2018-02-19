#ifndef PROC_H
#define PROC_H

#ifdef __ASSEMBLER__

#define SAVE_CONTEXT \
	push {r0, r14};\
	mov r0, lr;\
	bl __saveContext1;\
	pop {r0, r14};\
	push {r14};\
	bl __saveContext2;\
	pop {r14};

#else

#define CONTEXT_SIZE 17
typedef struct 
{
	int context[CONTEXT_SIZE];
} ProcessT;

extern ProcessT *_currentPocess;

#endif

#endif
