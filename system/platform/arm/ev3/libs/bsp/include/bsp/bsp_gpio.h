#ifndef bsp_gpio_BSP_H
#define bsp_gpio_BSP_H

#include <stdint.h>

#define GPIO_INPUT  0x00
#define GPIO_OUTPUT 0x01
#define GPIO_ALTF5  0x02
#define GPIO_ALTF4  0x03
#define GPIO_ALTF0  0x04
#define GPIO_ALTF1  0x05
#define GPIO_ALTF2  0x06
#define GPIO_ALTF3  0x07

#define GPIO_PULL_NONE 0x00
#define GPIO_PULL_DOWN 0x01
#define GPIO_PULL_UP   0x02

void     bsp_gpio_init(void);
void     bsp_gpio_config(int32_t bsp_gpio_no, int32_t bsp_gpio_sel);
void     bsp_gpio_pull(int32_t bsp_gpio_no, int32_t pull_dir);
void     bsp_gpio_write(int32_t bsp_gpio_no, int32_t value);
uint8_t  bsp_gpio_read(int32_t bsp_gpio_no);

#endif
