#include <kernel/irq.h>
#include <kernel/svc.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <kprintf.h>
#include <arch_context.h>
#include <stddef.h>

volatile uint32_t _x86_irq_raw = 0;

void dump_ctx(context_t* ctx) {
	printf("\nctx=%08x\n", (uint32_t)(uintptr_t)ctx);
	printf("cr2=%08x trap=%08x err=%08x\n", (uint32_t)ctx->cr2, (uint32_t)ctx->trap_no, (uint32_t)ctx->err_code);
	printf("pc=%08x lr=%08x sp=%08x cs=%08x ss=%08x flags=%08x\n",
		(uint32_t)ctx->pc,
		(uint32_t)ctx->lr,
		(uint32_t)ctx->sp,
		(uint32_t)ctx->cs,
		(uint32_t)ctx->ss,
		(uint32_t)ctx->rflags);
	printf("rdi=%08x rsi=%08x rdx=%08x rcx=%08x\n",
		(uint32_t)ctx->gpr[0],
		(uint32_t)ctx->gpr[1],
		(uint32_t)ctx->gpr[2],
		(uint32_t)ctx->gpr[3]);
	printf("r8 =%08x r9 =%08x rax=%08x rbx=%08x rbp=%08x\n",
		(uint32_t)ctx->gpr[4],
		(uint32_t)ctx->gpr[5],
		(uint32_t)ctx->gpr[6],
		(uint32_t)ctx->gpr[7],
		(uint32_t)ctx->gpr[8]);
	printf("r10=%08x r11=%08x r12=%08x r13=%08x r14=%08x r15=%08x\n",
		(uint32_t)ctx->gpr[9],
		(uint32_t)ctx->gpr[10],
		(uint32_t)ctx->gpr[11],
		(uint32_t)ctx->gpr[12],
		(uint32_t)ctx->gpr[13],
		(uint32_t)ctx->gpr[14]);
}

static void trap_panic(uint64_t vector, context_t* ctx) {
	proc_t* cproc = get_current_proc();
	if (cproc != NULL) {
		printf("pid:%d cmd:%s trap:%d\n", cproc->info.pid, cproc->info.cmd, (int)vector);
	}
	else {
		printf("kernel trap:%d\n", (int)vector);
	}
	dump_ctx(ctx);
	halt();
}

void handle_trap(uint64_t vector, uint64_t err, context_t* ctx) {
	ctx->trap_no = vector;
	ctx->err_code = err;
	ctx->lr = ctx->pc;

	if (vector >= 0x20 && vector < 0x30) {
		_x86_irq_raw = (uint32_t)vector;
		irq_handler(ctx);
		return;
	}

	if (vector == 0x80) {
		svc_handler((int32_t)ctx->gpr[0],
			(ewokos_addr_t)ctx->gpr[1],
			(ewokos_addr_t)ctx->gpr[2],
			(ewokos_addr_t)ctx->gpr[3],
			ctx);
		return;
	}

	if (vector == 14) {
		data_abort_handler(ctx, (ewokos_addr_t)ctx->cr2, (uint32_t)err);
		return;
	}

	if (vector == 13) {
		prefetch_abort_handler(ctx, (uint32_t)err);
		return;
	}

	if (vector == 6 || vector == 3 || vector == 1) {
		undef_abort_handler(ctx, (uint32_t)vector);
		return;
	}

	trap_panic(vector, ctx);
}
