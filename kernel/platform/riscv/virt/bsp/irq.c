#include <kernel/irq.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <csr.h>
#include <arch_context.h>

void irq_arch_init(void) {
}

void irq_enable(uint32_t irq) {
       switch(irq){
              case IRQ_TIMER0:
                    csr_set((CSR_SIE), 0x20);
                    break;
              default:
                     break; 
       }
}

void irq_disable(uint32_t irq) {

}

inline uint32_t irq_get(void) {
	return IRQ_TIMER0;
}
