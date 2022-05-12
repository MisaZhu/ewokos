#ifndef KINTR_H
#define KINTR_H

#include <stdint.h>
#include <interrupt.h>
#include <kernel/context.h>
#include <kernel/proc.h>

void interrupt_init(void);

int32_t interrupt_setup(proc_t* proc, uint32_t interrupt, uint32_t entry, uint32_t data);

int32_t interrupt_send(context_t* ctx, uint32_t interrupt);

int32_t interrupt_soft_send(context_t* ctx, int32_t to_pid, uint32_t entry, uint32_t data);

void interrupt_end(context_t* ctx);

#endif
