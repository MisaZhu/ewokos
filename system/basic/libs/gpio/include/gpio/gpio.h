#ifndef GPIO_H
#define GPIO_H

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
#define GPIO_PULL_MASK 0x03

void     gpio_init(void);
void     gpio_config(int32_t gpio_no, int32_t gpio_sel);
void     gpio_pull(int32_t gpio_no, int32_t pull_dir);
void     gpio_write(int32_t gpio_no, int32_t value);
uint8_t  gpio_read(int32_t gpio_no);

#endif
