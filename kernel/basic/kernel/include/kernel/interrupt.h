#ifndef KINTR_H
#define KINTR_H

#include <stdint.h>
#include <interrupt.h>
#include <kernel/context.h>
#include <kernel/proc.h>

void interrupt_init(void);

int32_t interrupt_setup(proc_t* proc, uint32_t interrupt, uint32_t entry);

void interrupt_send(context_t* ctx, uint32_t interrupt);

void interrupt_end(context_t* ctx);

#endif
