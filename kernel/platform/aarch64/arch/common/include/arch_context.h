#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

typedef struct {
  uint64_t pc;
  uint64_t spsr_el1;
  uint64_t sp;
  uint64_t lr;
  uint64_t fpcr; 
  uint64_t fpsr; 
  uint64_t gpr[30]; 
  uint64_t fpu[64]; 
} context_t;


#define CONTEXT_INIT(x) (x.spsr_el1 = 0x0)

#endif
