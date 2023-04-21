#ifndef CPU_CORE_H
#define CPU_CORE_H

#include <stdint.h>
#include <kernel/proc.h>

#ifdef KERNEL_SMP

#define CPU_MAX_CORES 16
extern void     mcore_lock(int32_t* v);
extern void     mcore_unlock(int32_t* v);
extern uint32_t get_core_id(void);
extern uint32_t get_cpu_cores(void);
extern void     start_core(uint32_t core_id);
extern void     cpu_core_ready(uint32_t core_id);
#else

#define CPU_MAX_CORES 1
#define get_core_id() 0
#define get_cpu_cores() 1

#endif

typedef struct {
	uint32_t actived;
	proc_t* halt_proc;
} core_t;

extern core_t _cpu_cores[CPU_MAX_CORES];

#endif
