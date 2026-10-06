#ifndef EWOKOS_CONFIG_H
#define EWOKOS_CONFIG_H

#include <stdint.h>

typedef uint64_t ewokos_addr_t;

#define EWOK_STACK_ALIGN      8U
#define EWOK_STACK_INIT_BIAS  8U

/*
 * Size of the fxsave/fxrstor image (x87 + MMX + SSE xmm0-15) carried per proc.
 * The common proc.c embeds an aligned(16) buffer of this size in proc_t and
 * saves/restores it across a context switch; leaving it undefined keeps other
 * arches' proc_t unchanged (aarch64 carries fpu[] inside its context_t).
 */
#define ARCH_FPU_STATE_SIZE   512U

#endif
