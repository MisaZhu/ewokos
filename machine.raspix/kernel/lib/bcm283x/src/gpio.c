#include <bcm283x/gpio.h>

void bcm283x_gpio_init(void) {

}

void bcm283x_gpio_config(int32_t no, int32_t gpio_sel) {
	uint32_t raddr = (uint32_t)GPIO_FSEL0 + ((no/10)<<2);
	uint32_t shift = (no%10) * GPIO_SEL_BITS;
	uint32_t value = gpio_sel << shift;
	uint32_t mask = GPIO_SEL << shift;
	uint32_t data = get32(raddr);
	data &= ~mask;
	data |= value;
	put32(raddr, data);
}

void bcm283x_gpio_pull(int32_t no, int32_t pull_dir) {
	uint32_t shift = (no % 32);
	uint32_t index = (no/32) + 1;
	*GPIO_PUD = pull_dir & bcm283x_gpio_pull_MASK;

	uint32_t n = 150; while(n > 0) n--; //delay 150
	put32((uint32_t)GPIO_PUD+(index<<2), 1<<shift); /* enable ppud clock */
	n = 150; while(n > 0) n--; //delay 150
	*GPIO_PUD = 0;
	put32((uint32_t)GPIO_PUD+(index<<2), 0); /* disable ppud clock */
}

inline void bcm283x_gpio_set(int32_t no) {
	put32((uint32_t)GPIO_SET0 + ((no/32)<<2), 1<<(no%32));
}

inline void bcm283x_gpio_clr(int32_t no) {
	put32((uint32_t)GPIO_CLR0+((no/32)<<2), 1<<(no%32));
}

inline void bcm283x_gpio_write(int32_t no, int32_t value) {
	if(value)
		bcm283x_gpio_set(no);
	else 
		bcm283x_gpio_clr(no);
}

inline uint32_t bcm283x_gpio_read(int32_t no) {
	if((get32((uint32_t)GPIO_LEV0+((no/32)<<2)) & (1<<(no%32))) != 0)
		return 1;
	return 0;
}
