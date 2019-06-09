#ifndef SYSTEM_H
#define SYSTEM_H

#include <types.h>
#include <scheduler.h>

void __set_translation_table_base(uint32_t v);
void __switch_to_context(uint32_t *context);

extern uint32_t __cli(void); //disable interrupts
extern void __sti(uint32_t cpsr); //enable interrupts

#ifdef CPU_NUM
void __enable_scu(void);
int32_t __get_cpu_id(void);
void send_sgi(int32_t int_id, int32_t target_cpu, int32_t filter);
void __s_lock(int32_t* spin);
void __s_unlock(int32_t* spin);
#endif

#define CRIT_IN(P) (P) = __cli();
#define CRIT_OUT(P) __sti(P);


void loopd(uint32_t times);

int32_t system_cmd(int32_t cmd, int32_t arg0, int32_t arg1);
#endif

