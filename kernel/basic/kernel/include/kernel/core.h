#ifndef CPU_CORE_H
#define CPU_CORE_H

#include <stdint.h>

extern void mcore_lock(int32_t* v);
extern void mcore_unlock(int32_t* v);

extern uint32_t get_core_id(void);
extern uint32_t get_cpu_cores(void);

#ifdef KERNEL_SMP
#define CPU_MAX_CORES 16
#else
#define CPU_MAX_CORES 1
#endif

typedef struct {
	uint32_t actived;
	uint32_t halt_stack[64];
} core_t;

extern core_t _cpu_cores[CPU_MAX_CORES];

extern void start_multi_cores(uint32_t cores);
extern int32_t multi_cores_ready(void);

#endif
