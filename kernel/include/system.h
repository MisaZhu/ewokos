#ifndef SYSTEM_H
#define SYSTEM_H

#include <types.h>

void __set_translation_table_base(uint32_t v);
void __switch_to_context(int *context);

void cli(); //disable interrupts
void sti(); //enable interrupts

void loopd(uint32_t times);

#endif

