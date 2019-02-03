#ifndef IRQ_H
#define IRQ_H

#include <types.h>

/* number of interrupt lines */
#define IRQ_COUNT 32

typedef void (*InterruptHandlerT)(void);

void registerInterruptHandler(int line, InterruptHandlerT handler);
void handleIRQ(void);

/* architecture specific functions */
void enableIRQ(int line);
void getPendingIRQs(bool *result);

#endif
