#ifndef MMIO_H
#define MMIO_H

#include <types.h>

int32_t mmioGet(uint32_t offset);

int32_t mmioPut(uint32_t offset, int32_t val);

#endif
