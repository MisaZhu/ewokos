#include <kernel/irq.h>
#include <kprintf.h>

void dump_ctx(context_t* ctx) {
	printf("ctx dump:\n"
		"  cpsr=0x%x\n"
		"  pc=0x%x\n"
		"  sp=0x%x\n"
		"  lr=0x%x\n",
		ctx->cpsr,
		ctx->pc,
		ctx->sp,
		ctx->lr);

	uint32_t i;
	for(i=0; i<13; i++) 
		printf("  r%d: 0x%x\n", i, ctx->gpr[i]);
}