#ifndef TIMER_H
#define TIMER_H

#include <types.h>

void timerSetInterval(uint32_t intervalMicrosecond);
void timerClearInterrupt(void);
void timerInit();
uint32_t timerRead();
void timerWait(uint32_t delay);

#endif
