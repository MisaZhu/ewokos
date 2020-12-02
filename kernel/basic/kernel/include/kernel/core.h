#ifndef CPU_CORE_H
#define CPU_CORE_H

#include <stdint.h>

#ifdef KERNEL_SMP
#define CPU_MAX_CORES 16
#else
#define CPU_MAX_CORES 1
#endif

typedef struct {
	uint32_t actived;
} core_t;

extern core_t _cpu_cores[CPU_MAX_CORES];

extern void start_multi_cores(uint32_t cores);
extern int32_t multi_cores_ready(void);

#endif
