#ifndef GIC_H
#define GIC_H

#include <stdint.h>

extern void pic_set_enabled(uint32_t v);
extern uint32_t pic_get_enabled(void);
extern uint32_t pic_get_status(void);

extern void sic_set_enabled(uint32_t v);
extern uint32_t sic_get_enabled(void);
extern uint32_t sic_get_status(void);

extern uint32_t gic_get_irqs(void);
extern void gic_set_irqs(uint32_t irqs);

#ifdef KERNEL_SMP
extern void ipi_enable(uint32_t core_id);
extern void ipi_clear(uint32_t core_id);
extern void ipi_send(uint32_t core_id);
#endif

#endif
