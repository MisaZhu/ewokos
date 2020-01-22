#ifndef GIC_H
#define GIC_H

#include <types.h>

extern void pic_set_enabled(uint32_t v);
extern uint32_t pic_get_enabled(void);
extern uint32_t pic_get_status(void);

extern void sic_set_enabled(uint32_t v);
extern uint32_t sic_get_enabled(void);
extern uint32_t sic_get_status(void);

extern uint32_t gic_get_irqs(void);
extern void gic_set_irqs(uint32_t irqs);

#endif
