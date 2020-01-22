#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>
#include <usinterrupt.h>

typedef void(*interrupt_func_t)(int int_id, void* p);

int proc_interrupt_setup(interrupt_func_t func, void* p);

int proc_interrupt_register(uint32_t int_id);
int proc_interrupt_unregister(uint32_t int_id);

void proc_interrupt(int pid, int int_id);

#endif
