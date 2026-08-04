#ifndef MMIO_H
#define MMIO_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (val))

extern ewokos_addr_t _mmio_base;
ewokos_addr_t mmio_map(void);
ewokos_addr_t mmio_map_offset(uint32_t offset, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
