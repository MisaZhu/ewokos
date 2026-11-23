#include <kernel/irq.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <kprintf.h>
#include "arch.h"

/* GICv3 (see bsp/gic_v3.c): KVM on GICv3 hosts cannot emulate GICv2. */
extern void gic_v3_init(void);
extern void gic_v3_core_init(uint32_t core);
extern void gic_v3_irq_enable(int core, int irq);
extern void gic_v3_irq_disable(int core, int irq);
extern uint32_t gic_v3_get_irq(void);
extern void gic_v3_eoi(uint32_t irq);

extern uint64_t interrupt_table_start[];
void irq_init_arch(void) {
    /* Install real vectors BEFORE touching the GIC so the first exception
     * lands in the syndrome-printing handler instead of the VBAR=0 loop. */
    set_vector_table(&interrupt_table_start);
    gic_v3_init();
    kout_str("gicv3: dist init ok\n");
    /* Start from a clean slate for SPIs; SGI0/PPI27 are set up by the driver. */
    for (int i = 32; i < 1020; i++)
        gic_v3_irq_disable(0, i);
    gic_v3_core_init(0);
    kout_str("gicv3: core0 init ok\n");
}

void irq_enable_arch(uint32_t irq) {
    if(irq == IRQ_TIMER0)
        irq = 27;
    gic_v3_irq_enable(0, irq);
}

void irq_enable_core_arch(uint32_t core, uint32_t irq) {
    if(irq == IRQ_TIMER0)
        irq = 27;
    gic_v3_irq_enable(core, irq);
}

inline void irq_clear_core_arch(uint32_t core, uint32_t irq) {

}

inline void irq_clear_arch(uint32_t irq) {

}

void irq_disable_arch(uint32_t irq) {
    if(irq == IRQ_TIMER0)
        irq = 27;
    gic_v3_irq_disable(0, irq);
}

inline uint32_t irq_get_arch(void) {
    uint32_t irqno = gic_v3_get_irq() & 0x3FF;
    return irqno;
}

inline uint32_t irq_get_unified_arch(uint32_t irqno) {
    if(irqno == 27){
        irqno = IRQ_TIMER0;
    }else if(irqno == 0){
        irqno = IRQ_IPI;
    }
    return irqno;
}

inline void irq_eoi_arch(uint32_t irq_raw) {
    gic_v3_eoi(irq_raw);
}