#ifndef SPI_bcm283x_H
#define SPI_bcm283x_H

#include <stdint.h>

#define SPI_SELECT_0 0x01
#define SPI_SELECT_1 0x02
#define SPI_SELECT_DEFAULT SPI_SELECT_0

void     bcm283x_spi_init(void);
void     bcm283x_spi_set_div(int32_t clk_divide);
uint8_t  bcm283x_spi_transfer(uint8_t data);
void     bcm283x_spi_activate(uint8_t enable);
void     bcm283x_spi_select(uint32_t which); 
void     bcm283x_spi_send_recv(const uint8_t* send, uint8_t* recv, uint32_t size);

#endif
