#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  unsigned long pc;
  unsigned long ra;
  unsigned long sp;
  unsigned long gp;
  unsigned long tp;
  unsigned long t0;
  unsigned long t1;
  unsigned long t2;
  unsigned long s0;
  unsigned long s1;
  unsigned  long gpr[8];
  unsigned long s2;
  unsigned long s3;
  unsigned long s4;
  unsigned long s5;
  unsigned long s6;
  unsigned long s7;
  unsigned long s8;
  unsigned long s9;
  unsigned long s10;
  unsigned long s11;
  unsigned long t3;
  unsigned long t4;
  unsigned long t5;
  unsigned long t6;
  /* Supervisor CSRs */
  unsigned long sstatus;
  unsigned long sbadaddr;
  unsigned long scause;
  /*saved a0*/
  unsigned long a0;

  unsigned long user_sp;
  unsigned long kernel_sp;
  uint64_t lr;
} context_t;

#define CONTEXT_INIT(x) do{x.sstatus = 0x46020; \
                           x.sp-=sizeof(context_t); \
                        }while(0)
#endif
