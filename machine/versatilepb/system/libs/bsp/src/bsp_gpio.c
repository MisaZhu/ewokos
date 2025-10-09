#include "gpio/gpio.h"
#include "bsp/bsp_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void bsp_gpio_init(void) {
}

void bsp_gpio_config(int32_t bsp_gpio_no, int32_t bsp_gpio_sel) {
}

void bsp_gpio_pull(int32_t bsp_gpio_no, int32_t pull_dir) {
}

void bsp_gpio_write(int32_t bsp_gpio_no, int32_t value) {
}

uint8_t  bsp_gpio_read(int32_t bsp_gpio_no) {
    return 0;
}

#ifdef __cplusplus
}
#endif