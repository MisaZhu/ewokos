#ifndef CPU_CORE_H
#define CPU_CORE_H

#include <stdint.h>

#define CPU_MAX_CORES 32

typedef struct {
	uint32_t actived;
} core_t;

extern core_t _cpu_cores[CPU_MAX_CORES];

extern void start_multi_cores(uint32_t cores);
extern int32_t multi_cores_ready(void);

#endif
