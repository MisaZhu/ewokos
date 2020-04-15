#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>
#include <usinterrupt.h>

int get_proc_interrupt_pid(uint32_t int_id);
int proc_interrupt_register(uint32_t int_id);
int proc_interrupt_unregister(uint32_t int_id);

#endif
