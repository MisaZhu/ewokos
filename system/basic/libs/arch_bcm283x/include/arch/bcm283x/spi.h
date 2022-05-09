#ifndef SPI_ARCH_H
#define SPI_ARCH_H

#include <stdint.h>
void     bcm283x_spi_init(int32_t clk_divide);
uint8_t  bcm283x_spi_transfer(uint8_t data);
uint16_t bcm283x_spi_transfer16(uint16_t data);
void     bcm283x_spi_write(uint8_t data);
void     bcm283x_spi_activate(uint8_t enable);
void     bcm283x_spi_select(uint32_t which); 
uint16_t bcm283x_spi_transfer16_fast(uint16_t data);
inline void bcm283x_spi_send_recv(const uint8_t* send, uint32_t* recv, uint32_t size);

#endif
