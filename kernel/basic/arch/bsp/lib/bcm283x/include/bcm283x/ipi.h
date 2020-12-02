#ifndef BCM283x_IPI_H
#define BCM283x_IPI_H

#include <stdint.h>

void bcm283x_ipi_enable(uint32_t core_id);
void bcm283x_ipi_send(uint32_t core_id);
void bcm283x_ipi_clear(uint32_t core_id);

#endif
