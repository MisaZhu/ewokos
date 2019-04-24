#ifndef SYSTEM_H
#define SYSTEM_H

#include <types.h>
#include <scheduler.h>

void __set_translation_table_base(uint32_t v);
void __switch_to_context(int32_t *context);

extern uint32_t __cli(); //disable interrupts
extern void __sti(uint32_t cpsr); //enable interrupts

void loopd(uint32_t times);

int32_t system_cmd(int32_t cmd, int32_t arg0, int32_t arg1);

#define CRIT_IN(P) (P) = __cli();
#define CRIT_OUT(P) __sti(P);

#endif

