#ifndef SYSTEM_H
#define SYSTEM_H

#include <types.h>
#include <scheduler.h>

void __set_translation_table_base(uint32_t v);
void __switch_to_context(int *context);

void cli(); //disable interrupts
void sti(); //enable interrupts

void loopd(uint32_t times);

int32_t system_cmd(int32_t cmd, int32_t arg);

#define CRIT_IN disable_schedule();
#define CRIT_OUT enable_schedule();

#endif

