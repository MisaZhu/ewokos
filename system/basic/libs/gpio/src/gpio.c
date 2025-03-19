#include "gpio/gpio.h"
#include "bsp/bsp_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void gpio_init(void) {
    bsp_gpio_init();
}

void gpio_config(int32_t gpio_no, int32_t gpio_sel) {
    bsp_gpio_config(gpio_no, gpio_sel);
}

void gpio_pull(int32_t gpio_no, int32_t pull_dir) {
    bsp_gpio_pull(gpio_no, pull_dir);
}

void gpio_write(int32_t gpio_no, int32_t value) {
    bsp_gpio_write(gpio_no, value);
}

uint8_t  gpio_read(int32_t gpio_no) {
    return bsp_gpio_read(gpio_no);
}

#ifdef __cplusplus
}
#endif