#ifndef CPU_CORE_H
#define CPU_CORE_H

#include <stdint.h>

extern void start_slave_cores(void);
extern int32_t slave_cores_ready(void);

#endif
