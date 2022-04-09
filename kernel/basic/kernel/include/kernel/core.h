#ifndef CPU_CORE_H
#define CPU_CORE_H

#include <stdint.h>

#ifdef KERNEL_SMP

#define CPU_MAX_CORES 16
extern void mcore_lock(int32_t* v);
extern void mcore_unlock(int32_t* v);
extern void start_multi_cores(uint32_t cores);
extern int32_t multi_cores_ready(void);
extern uint32_t get_core_id(void);
extern uint32_t get_cpu_cores(void);

#else

#define CPU_MAX_CORES 1
#define get_core_id() 0
#define get_cpu_cores() 1

#endif

typedef struct {
	uint32_t actived;
	int32_t halt_pid;
} core_t;

extern core_t _cpu_cores[CPU_MAX_CORES];

#endif
