#ifndef GPIO_ARCH_BCM283X_H
#define GPIO_ARCH_BCM283X_H

#include <stdint.h>
#include <mm/mmu.h>

#define GPIO_FSEL0         ((volatile uint32_t*)(MMIO_BASE+0x00200000))
#define GPIO_FSEL1         ((volatile uint32_t*)(MMIO_BASE+0x00200004))
#define GPIO_FSEL2         ((volatile uint32_t*)(MMIO_BASE+0x00200008))
#define GPIO_FSEL3         ((volatile uint32_t*)(MMIO_BASE+0x0020000C))
#define GPIO_FSEL4         ((volatile uint32_t*)(MMIO_BASE+0x00200010))
#define GPIO_FSEL5         ((volatile uint32_t*)(MMIO_BASE+0x00200014))
#define GPIO_SET0          ((volatile uint32_t*)(MMIO_BASE+0x0020001C))
#define GPIO_SET1          ((volatile uint32_t*)(MMIO_BASE+0x00200020))
#define GPIO_CLR0          ((volatile uint32_t*)(MMIO_BASE+0x00200028))
#define GPIO_CLR1          ((volatile uint32_t*)(MMIO_BASE+0x0020002C))
#define GPIO_LEV0          ((volatile uint32_t*)(MMIO_BASE+0x00200034))
#define GPIO_LEV1          ((volatile uint32_t*)(MMIO_BASE+0x00200038))
#define GPIO_EDS0          ((volatile uint32_t*)(MMIO_BASE+0x00200040))
#define GPIO_EDS1          ((volatile uint32_t*)(MMIO_BASE+0x00200044))
#define GPIO_HEN0          ((volatile uint32_t*)(MMIO_BASE+0x00200064))
#define GPIO_HEN1          ((volatile uint32_t*)(MMIO_BASE+0x00200068))
#define GPIO_PUD           ((volatile uint32_t*)(MMIO_BASE+0x00200094))
#define GPIO_PUDCLK0       ((volatile uint32_t*)(MMIO_BASE+0x00200098))
#define GPIO_PUDCLK1       ((volatile uint32_t*)(MMIO_BASE+0x0020009C))


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

#define bcm283x_gpio_pull_NONE 0x00
#define bcm283x_gpio_pull_DOWN 0x01
#define bcm283x_gpio_pull_UP   0x02
#define bcm283x_gpio_pull_MASK 0x03

void     bcm283x_gpio_init(void);
void     bcm283x_gpio_config(int32_t gpio_no, int32_t gpio_sel);
void     bcm283x_gpio_pull(int32_t gpio_no, int32_t pull_dir);
void     bcm283x_gpio_write(int32_t gpio_no, int32_t value);
uint32_t bcm283x_gpio_read(int32_t gpio_no);
void     bcm283x_gpio_set(int32_t no);
void     bcm283x_gpio_clr(int32_t no);

#endif
