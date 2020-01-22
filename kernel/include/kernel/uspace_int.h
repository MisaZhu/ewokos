#ifndef USPACE_INT_H
#define USPACE_INT_H

#include "kernel/proc.h"

#define USPACE_INT_MAX 32

extern void uspace_interrupt_init(void);

extern void uspace_interrupt(context_t* ctx, int32_t int_id);

extern void proc_interrupt(context_t* ctx, int32_t pid, int32_t int_id);

extern void uspace_interrupt_register(int32_t pid, int32_t int_id);

#endif
