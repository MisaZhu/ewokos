#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include "timer_arch.h"
#include "hw_arch.h"

void irq_arch_init_pi4(void);
void irq_arch_init_pix(void);
void irq_init_arch(void) {
    if(_pi4)
        irq_arch_init_pi4();
    else
        irq_arch_init_pix();
}

uint32_t irq_get_pi4();
uint32_t irq_get_pix();
void irq_eoi_pi4(uint32_t irq_raw);
void irq_eoi_pix(uint32_t irq_raw);
uint32_t irq_get_unified_pi4(uint32_t irqno);
uint32_t irq_get_unified_pix(uint32_t irqno);

inline uint32_t irq_get_arch(void) {
    if(_pi4)
        return irq_get_pi4();
    return irq_get_pix();
}

inline uint32_t irq_get_unified_arch(uint32_t irqno) {
    if(_pi4)
	    return irq_get_unified_pi4(irqno);
    return irq_get_unified_pix(irqno);
}

inline void irq_eoi_arch(uint32_t irq_raw) {
    if(_pi4)
	    irq_eoi_pi4(irq_raw);
    else
	    irq_eoi_pix(irq_raw);
}

void irq_enable_pi4(uint32_t core, uint32_t irq);
void irq_enable_pix(uint32_t irq);
inline void irq_enable_arch(uint32_t irq) {
    if(_pi4)
        irq_enable_pi4(0, irq);
    else
        irq_enable_pix(irq);
}

inline void irq_enable_core_arch(uint32_t core, uint32_t irq) {
    if(_pi4)
        irq_enable_pi4(core, irq);
    else
        irq_enable_pix(irq);
}

inline void irq_clear_core_arch(uint32_t core, uint32_t irq) {

}

inline void irq_clear_arch(uint32_t irq) {

}

void irq_disable_pi4(uint32_t irq);
void irq_disable_pix(uint32_t irq);
void irq_disable_arch(uint32_t irq) {
    if(_pi4)
        irq_disable_pi4(irq);
    else
        irq_disable_pix(irq);
}
