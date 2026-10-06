#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <ewokos_config.h>
#include <arch.h>

typedef struct {
	uint64_t cr2;
	uint64_t gpr[15];
	uint64_t trap_no;
	uint64_t err_code;
	uint64_t pc;
	uint64_t lr;
	uint64_t cs;
	uint64_t rflags;
	uint64_t sp;
	uint64_t ss;
#ifdef ARCH_FPU_STATE_SIZE
	/*
	 * fxsave/fxrstor image (x87 + MMX + SSE xmm0-15) for this context, kept
	 * last so every fixed CTX_* offset in interrupt.S stays valid (mirrors
	 * aarch64, whose context_t carries fpu[] after gpr[]). The kernel is
	 * -mno-sse and never touches xmm, so proc_switch() fills/reads this via
	 * arch_fpu_save/arch_fpu_restore only on a real switch. aligned(16) is
	 * required by fxsave/fxrstor and forces the whole struct (hence proc_t.ctx
	 * and the on-stack trap frame, sized CTX_SIZE in interrupt.S) to be
	 * 16-byte aligned.
	 */
	uint8_t  fpu[ARCH_FPU_STATE_SIZE] __attribute__((aligned(16)));
#endif
} context_t __attribute__((aligned(16)));

#define CONTEXT_INIT(x) do { \
	(x).cr2 = 0; \
	(x).trap_no = 0; \
	(x).err_code = 0; \
	(x).pc = 0; \
	(x).lr = 0; \
	(x).cs = X86_USER_CS; \
	(x).rflags = 0x3202; \
	(x).sp = 0; \
	(x).ss = X86_USER_DS; \
} while(0)

#endif
