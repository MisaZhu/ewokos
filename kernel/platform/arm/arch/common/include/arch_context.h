#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

typedef struct {
#ifdef FPU_ENABLED
  ewokos_addr_t fpu[ 64 ], fpscr, cpsr, pc, gpr[ 13 ], sp, lr;
#else
  ewokos_addr_t cpsr, pc, gpr[ 13 ], sp, lr;
#endif
} context_t;


#define CONTEXT_INIT(x) (x.cpsr = 0x50)

#endif
