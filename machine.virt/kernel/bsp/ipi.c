#include <kernel/smp/ipi.h>
#include <kernel/hw_info.h>

extern void gic_v3_irq_enable(int core, int irq);
extern void gic_v3_send_sgi(int core, int irq);

void ipi_enable(uint32_t core_id) {
     gic_v3_irq_enable(core_id, 0);
}

void ipi_send(uint32_t core_id) {
    gic_v3_send_sgi(core_id, 0);
}

void ipi_clear(uint32_t core_id) {
    (void)core_id;
}