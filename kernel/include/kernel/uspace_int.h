#ifndef USPACE_INT_H
#define USPACE_INT_H

#include "kernel/proc.h"
#include "rawdata.h"
#include "usinterrupt.h"

extern void uspace_interrupt_init(void);

extern bool uspace_interrupt(context_t* ctx, int32_t int_id);

extern bool proc_interrupt(context_t* ctx, int32_t pid, int32_t int_id);

extern int32_t uspace_interrupt_register(int32_t int_id);

extern void uspace_interrupt_unregister(int32_t int_id);

extern void uspace_set_interrupt_data(int32_t int_id, void* data, uint32_t size);
extern void uspace_get_interrupt_data(int32_t int_id, rawdata_t* data);

#endif
