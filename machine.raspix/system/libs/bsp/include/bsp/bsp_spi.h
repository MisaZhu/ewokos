#ifndef SPI_ARCH_H
#define SPI_ARCH_H

#include <stdint.h>
#include <arch/bcm283x/spi.h>

void     bsp_spi_init(void);
void     bsp_spi_set_div(int32_t clk_divide);
uint8_t  bsp_spi_transfer(uint8_t data);
void     bsp_spi_activate(uint8_t enable);
void     bsp_spi_select(uint32_t which); 
void     bsp_spi_send_recv(const uint8_t* send, uint8_t* recv, uint32_t size);

#endif
