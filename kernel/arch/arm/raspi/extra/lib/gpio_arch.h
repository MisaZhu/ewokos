#ifndef GPIO_ARCH_H
#define GPIO_ARCH_H

#include <types.h>

#define GPIO_FSEL0         ((volatile uint32_t*)(_mmio_base+0x00200000))
#define GPIO_SET0          ((volatile uint32_t*)(_mmio_base+0x0020001C))
#define GPIO_CLR0          ((volatile uint32_t*)(_mmio_base+0x00200028))
#define GPIO_LEV0          ((volatile uint32_t*)(_mmio_base+0x00200034))
#define GPIO_EDS0          ((volatile uint32_t*)(_mmio_base+0x00200040))
#define GPIO_HEN0          ((volatile uint32_t*)(_mmio_base+0x00200064))
#define GPIO_PUD           ((volatile uint32_t*)(_mmio_base+0x00200094))
#define GPIO_PUDCLK0       ((volatile uint32_t*)(_mmio_base+0x00200098))
#define GPIO_PUDCLK1       ((volatile uint32_t*)(_mmio_base+0x0020009C))

#define GPIO_SEL_BITS 3
#define GPIO_SEL      0x07

#define GPIO_INPUT  0x00
#define GPIO_OUTPUT 0x01
#define GPIO_ALTF5  0x02
#define GPIO_ALTF4  0x03
#define GPIO_ALTF0  0x04
#define GPIO_ALTF1  0x05
#define GPIO_ALTF2  0x06
#define GPIO_ALTF3  0x07

#define GPIO_PULL_MASK 0x03

void gpio_arch_config(int32_t gpio_num, int32_t gpio_sel);

void gpio_arch_pull(int32_t gpio_num, int32_t pull_dir);

void gpio_arch_write(int32_t gpio_num, int32_t value);

uint32_t gpio_arch_read(int32_t gpio_num);

#endif
