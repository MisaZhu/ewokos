#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include "timer_arch.h"
#include "hw_arch.h"

void irq_arch_init_pi4(void);
void irq_arch_init_pix(void);
void irq_arch_init(void) {
    if(_pi4)
        irq_arch_init_pi4();
    else
        irq_arch_init_pix();
}

uint32_t irq_get_pi4(void);
uint32_t irq_get_pix(void);
inline uint32_t irq_get(void) {
    if(_pi4)
        return irq_get_pi4();
    return irq_get_pix();
}

void irq_enable_pi4(uint32_t irq);
void irq_enable_pix(uint32_t irq);
inline void irq_enable(uint32_t irq) {
    if(_pi4)
        irq_enable_pi4(irq);
    else
        irq_enable_pix(irq);
}

void irq_disable_pi4(uint32_t irq);
void irq_disable_pix(uint32_t irq);
void irq_disable(uint32_t irq) {
    if(_pi4)
        irq_disable_pi4(irq);
    else
        irq_disable_pix(irq);
}
