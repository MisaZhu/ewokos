#ifndef __GPIO_H__
#define __GPIO_H__

void gpio_init(void);
void gpio_pull_cfg(int gp, int en, int updown);
void gpio_direction(int pin, int out, int value);
void gpio_set(int pin, int value);
int gpio_get(int pin);

#endif
