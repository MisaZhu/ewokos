#ifndef MMIO_H
#define MMIO_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (val))

uint32_t mmio_map(void);

#ifdef __cplusplus
}
#endif

#endif
