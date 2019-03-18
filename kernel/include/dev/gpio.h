#ifndef GPIO_H
#define GPIO_H

#define GPIO_COUNT 54

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

#define GPIO_EVENT_EDGER 0x01
#define GPIO_EVENT_EDGEF 0x02
#define GPIO_EVENT_LVLHI 0x04
#define GPIO_EVENT_LVLLO 0x08
#define GPIO_EVENT_AEDGR 0x10
#define GPIO_EVENT_AEDGF 0x20

void gpio_init(void);
void gpio_config(int32_t gpio_num, int32_t gpio_sel);
void gpio_set(int32_t gpio_num);
void gpio_clr(int32_t gpio_num);
void gpio_write(int32_t gpio_num, int32_t value);
uint32_t gpio_read(int32_t gpio_num);
void gpio_toggle(int32_t gpio_num);
void gpio_pull(int32_t gpio_num, int32_t pull_dir);

/** GPIO_DATA => 20-27 : LSB-MSB */
void gpio_init_data(int32_t gpio_sel); /** select GPIO_INPUT or GPIO_OUTPUT */
void gpio_put_data(uint32_t data);
uint32_t gpio_get_data(void);

void gpio_setevent(int32_t gpio_num,int32_t events); /** enable gpio events detection */
void gpio_rstevent(int32_t gpio_num); /** clears event status */
uint32_t gpio_chkevent(int32_t gpio_num); /** check event status */

#endif
