#ifndef __GPIO_H__
#define __GPIO_H__

#define MODE_GPIO	0

void ev3_gpio_config(int32_t pin,int32_t  mode);
void ev3_gpio_init(void);
void ev3_gpio_pull(int32_t pin,int32_t updown);
void ev3_gpio_write(int32_t pin, int32_t  value);
uint8_t  ev3_gpio_read(int32_t pin);

#endif
