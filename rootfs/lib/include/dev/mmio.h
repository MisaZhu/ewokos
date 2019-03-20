#ifndef MMIO_H
#define MMIO_H

#include <types.h>

int32_t mmio_get(uint32_t offset);

int32_t mmio_put(uint32_t offset, int32_t val);

#endif
