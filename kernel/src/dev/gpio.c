#include <dev/gpio.h>
#include <mm/mmu.h>

#define GPIO_BASE       (MMIO_BASE + 0x00200000)

inline GPIOT* getGPIO(void) {
	return (GPIOT*)GPIO_BASE;
}

void gpioInit() {
	GPIOT* gpio = getGPIO();
	gpio->GPPUD = 0;
	gpio->GPPUDCLK0 = ( 1 << 14 );
	gpio->GPPUDCLK0 = 0;
}

GPIOValueT gpioGet(GPIOPinT pin) {
	GPIOValueT result = IO_UNKNOWN;
	GPIOT* gpio = getGPIO();

	switch(pin / 32) {
		case 0:
			result = gpio->GPLEV0 & (1 << pin);
			break;
		case 1:
			result = gpio->GPLEV1 & (1 << (pin - 32));
			break;
		default:
			break;
	}

	if(result != IO_UNKNOWN) {
		if(result)
			result = IO_HI;
	}
	return result;
}

void gpioToggle(GPIOPinT pin) {
	if(gpioGet(pin))
		gpioLo(pin);
	else
		gpioHi(pin);
}

void gpioHi(GPIOPinT pin) {
	GPIOT* gpio = getGPIO();

	switch(pin / 32) {
		case 0:
			gpio->GPSET0 = (1 << pin);
			break;
		case 1:
			gpio->GPSET1 = (1 << (pin - 32));
			break;
		default:
			break;
	}
}

void gpioLo(GPIOPinT pin) {
	GPIOT* gpio = getGPIO();

	switch(pin / 32) {
		case 0:
			gpio->GPCLR0 = (1 << pin);
			break;
		case 1:
			gpio->GPCLR1 = (1 << (pin - 32));
			break;
		default:
			break;
	}
}

void gpioSet(GPIOPinT pin, GPIOValueT value) {
	if((value == IO_LO) || (value == IO_OFF))
		gpioLo(pin);
	else if((value == IO_HI) || (value == IO_ON))
		gpioHi(pin);
}
