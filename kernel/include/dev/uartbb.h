#ifndef UARTBB_H
#define UARTBB_H

#define UARTBB_RX_DEFAULT 3
#define UARTBB_TX_DEFAULT 2

#include <types.h>

void uartbb_init(int rx_gpio,int tx_gpio);
void uartbb_send(uint32_t data);
void uartbb_find_stop(void);
uint32_t uartbb_read(void);
void uartbb_print_hex_byte(unsigned char byte);
void uartbb_print_hex_uint(uint32_t value);
void uartbb_print(char *message);

#endif
