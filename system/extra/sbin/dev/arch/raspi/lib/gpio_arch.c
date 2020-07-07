#include "gpio_arch.h"
#include <sys/mmio.h>

static uint32_t _mmio_base = 0;

void gpio_arch_init(void) {
	_mmio_base = mmio_map();
}

void gpio_arch_config(int32_t num, int32_t gpio_arch_sel) {
	uint32_t raddr = (uint32_t)GPIO_FSEL0 + ((num/10)<<2);
	uint32_t shift = (num%10) * GPIO_SEL_BITS;
	uint32_t value = gpio_arch_sel << shift;
	uint32_t mask = GPIO_SEL << shift;
	uint32_t data = get32(raddr);
	data &= ~mask;
	data |= value;
	put32(raddr, data);
}

void gpio_arch_pull(int32_t num, int32_t pull_dir) {
	uint32_t shift = (num % 32);
	uint32_t index = (num/32) + 1;
	*GPIO_PUD = pull_dir & GPIO_PULL_MASK;

	uint32_t n = 150; while(n > 0) n--; //delay 150
	put32((uint32_t)GPIO_PUD+(index<<2), 1<<shift); /* enable ppud clock */
	n = 150; while(n > 0) n--; //delay 150
	*GPIO_PUD = 0;
	put32((uint32_t)GPIO_PUD+(index<<2), 0); /* disable ppud clock */
}

static inline void gpio_arch_set(int32_t num) {
	put32((uint32_t)GPIO_SET0 + ((num/32)<<2), 1<<(num%32));
}

static inline void gpio_arch_clr(int32_t num) {
	put32((uint32_t)GPIO_CLR0+((num/32)<<2), 1<<(num%32));
}

inline void gpio_arch_write(int32_t num, int32_t value) {
	if(value)
		gpio_arch_set(num);
	else 
		gpio_arch_clr(num);
}

inline uint32_t gpio_arch_read(int32_t num) {
	if((get32((uint32_t)GPIO_LEV0+((num/32)<<2)) & (1<<(num%32))) != 0)
		return 1;
	return 0;
}
