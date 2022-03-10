#ifndef KINTR_H
#define KINTR_H

#include <stdint.h>
#include <interrupt.h>
#include <kernel/context.h>

void interrupt_init(void);

void interrupt_setup(int32_t pid, uint32_t interrupt, uint32_t entry);

void interrupt_send(context_t* ctx, uint32_t interrupt);

void interrupt_end(context_t* ctx);

#endif
