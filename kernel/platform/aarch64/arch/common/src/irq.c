#include <kernel/irq.h>
#include <kprintf.h>
#include <kernel/proc.h>

#define SPLIT(x)	((uint32_t)(((uint64_t)x)>>32)),((uint32_t)(x))

static const char* esr_ec_name(uint32_t ec) {
	switch(ec) {
	case 0x00: return "unknown";
	case 0x01: return "wfi/wfe";
	case 0x03: return "mcr/mrc cp15";
	case 0x04: return "mcrr/mrrc cp15";
	case 0x05: return "mcr/mrc cp14";
	case 0x06: return "ldc/stc cp14";
	case 0x07: return "sve/simd/fp trap";
	case 0x0E: return "illegal execution";
	case 0x11: return "svc aarch32";
	case 0x15: return "svc aarch64";
	case 0x18: return "msr/mrs/sys";
	case 0x20: return "instr abort lower el";
	case 0x21: return "instr abort same el";
	case 0x22: return "pc alignment";
	case 0x24: return "data abort lower el";
	case 0x25: return "data abort same el";
	case 0x26: return "sp alignment";
	case 0x2C: return "fp exception aarch64";
	case 0x2F: return "serror";
	case 0x30: return "breakpoint lower el";
	case 0x31: return "breakpoint same el";
	case 0x32: return "software step lower el";
	case 0x33: return "software step same el";
	case 0x34: return "watchpoint lower el";
	case 0x35: return "watchpoint same el";
	case 0x3C: return "brk aarch64";
	default:   return "reserved";
	}
}

static const char* abort_fsc_name(uint32_t fsc) {
	switch(fsc & 0x3F) {
	case 0x00: return "addr size fault level0";
	case 0x01: return "addr size fault level1";
	case 0x02: return "addr size fault level2";
	case 0x03: return "addr size fault level3";
	case 0x04: return "translation fault level0";
	case 0x05: return "translation fault level1";
	case 0x06: return "translation fault level2";
	case 0x07: return "translation fault level3";
	case 0x09: return "access flag fault level1";
	case 0x0A: return "access flag fault level2";
	case 0x0B: return "access flag fault level3";
	case 0x0D: return "permission fault level1";
	case 0x0E: return "permission fault level2";
	case 0x0F: return "permission fault level3";
	case 0x10: return "sync external abort";
	case 0x11: return "sync tag check fault";
	case 0x14: return "sync external abort level0";
	case 0x15: return "sync external abort level1";
	case 0x16: return "sync external abort level2";
	case 0x17: return "sync external abort level3";
	case 0x18: return "sync parity/ecc";
	case 0x1C: return "sync parity/ecc level0";
	case 0x1D: return "sync parity/ecc level1";
	case 0x1E: return "sync parity/ecc level2";
	case 0x1F: return "sync parity/ecc level3";
	case 0x21: return "alignment fault";
	case 0x22: return "debug event";
	default:   return "other/impl defined";
	}
}

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
	//dump_page_tables(_kernel_info.kernel_vm);
}

void sync_exception_handle(uint64_t esr, uint64_t far, context_t* ctx){
	proc_t* cproc = get_current_proc();

	uint32_t EC = (esr >> 26)&0x3F;
	uint32_t DFSC = esr & 0x3F;
	uint32_t IL = (esr >> 25) & 0x1;
	uint32_t ISS = esr & 0x1FFFFFF;

	if(EC == 0x24 || EC == 0x25){
		data_abort_handler(ctx, far, DFSC);
		return;
	}

	if(EC == 0x20 || EC == 0x21) {
		prefetch_abort_handler(ctx, DFSC);
		return;
	}

	printf("\n--------------------core dump infomation------------------\n");
	if(cproc){
		printf("PID:%d CMD:%s\n", cproc->info.pid,  cproc->info.cmd);
	}
	printf("EC :%08x (%s)\n", EC, esr_ec_name(EC));
	printf("IL :%08x\n", IL);
	printf("ISS:%08x\n", ISS);
	printf("FSC:%08x (%s)\n", DFSC, abort_fsc_name(DFSC));
	dump_ctx(ctx);
	if(cproc != 0) {
		dump_user_fault_words(cproc, ctx);
	}
	printf("\n----------------------------------------------------------\n");
	if(cproc != 0) {
		proc_exit(ctx, proc_get_proc(cproc), -1);
		return;
	}
	while(1);
}
