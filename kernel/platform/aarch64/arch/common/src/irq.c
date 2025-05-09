#include <kernel/irq.h>
#include <kprintf.h>
#include <kernel/proc.h>

inline void irq_enable_cpsr(context_t* ctx) {
	//ctx->cpsr &= (~0x80);
}

inline void irq_disable_cpsr(context_t* ctx) {
	//ctx->cpsr |= 0x80;
}
//uint64_t gpr[30], fpcr, fpsr, sp_el0, x30, spsr_el1, elr_el1;

extern uint64_t* _kernel_vm;

#define SPLIT(x)	((uint32_t)(((uint64_t)x)>>32)),((uint32_t)(x))

void dump_ctx(context_t* ctx) {
	uint64_t sp, esr, far;
	__asm__ __volatile__("mov %0, sp":"=r" (sp) :  : "memory");
	__asm__ __volatile__("mrs %0, esr_el1":"=r" (esr) :  : "memory");
	__asm__ __volatile__("mrs %0, far_el1":"=r" (far) :  : "memory");
	printf("\nESR:%08x%08x\n", SPLIT(esr));
	printf("FAR:%08x%08x\n", SPLIT(far));
	printf("KSP:%08x%08x\n", SPLIT(sp));
	printf("CTX:%08x%08x\n"
		"pc : %08x%08x\t"
		"spsr:%08x%08x\t"
		"sp : %08x%08x\t"
		"lr : %08x%08x\n",
		SPLIT(ctx),
		SPLIT(ctx->pc),
		SPLIT(ctx->spsr_el1),
		SPLIT(ctx->sp),
		SPLIT(ctx->lr)
	);

	uint32_t i;
	for(i=0; i<30; i++){
		if(i > 0 && i % 4 == 0 )
			printf("\n");
		printf("x%02d: %08x%08x\t", i, (uint32_t)((ctx->gpr[i]>>32)), (uint32_t)ctx->gpr[i]);
	}
	printf("\n");
	//dump_page_tables(_kernel_vm);
}

void sync_exception_handle(uint64_t esr, uint64_t far, context_t* ctx){
	proc_t* cproc = get_current_proc();

	uint32_t EC = (esr >> 26)&0x3F;
	uint32_t DFSC = esr & 0x3F;

	if(EC == 0x24 || EC == 0x25){
		data_abort_handler(ctx, far, 0x5);
	}else{
		printf("%08x %08x\n", EC, DFSC);
		printf("\n--------------------core dump infomation------------------\n");
		if(cproc){
			printf("PID:%d CMD:%s\n", cproc->info.pid,  cproc->info.cmd);
		}
		dump_ctx(ctx);
		printf("\n----------------------------------------------------------\n");
		while(1);
	}
}