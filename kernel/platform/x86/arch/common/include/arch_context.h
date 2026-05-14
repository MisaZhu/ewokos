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
} context_t;

#define CONTEXT_INIT(x) do { \
	(x).cr2 = 0; \
	(x).trap_no = 0; \
	(x).err_code = 0; \
	(x).pc = 0; \
	(x).lr = 0; \
	(x).cs = X86_USER_CS; \
	(x).rflags = 0x202; \
	(x).sp = 0; \
	(x).ss = X86_USER_DS; \
} while(0)

#endif
