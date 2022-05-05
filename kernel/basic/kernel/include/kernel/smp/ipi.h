#ifndef IPI_H
#define IPI_H

#include <stdint.h>

extern void ipi_enable(uint32_t core_id);
extern void ipi_clear(uint32_t core_id);
extern void ipi_send(uint32_t core_id);

#endif
