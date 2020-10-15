#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arch/arm/bcm283x/gpio.h>

#define DS   6
#define SHCP 19
#define STCP 13

int main(int argc, char* argv[]) {
	bcm283x_gpio_init();
	int v = 0;
	if(argc < 2) {
		printf("Usage: leds <v>\n");
		return -1;
	}

	if(argv[1][0] == '0')
		v = atoi_base(argv[1], 16);
	else
		v = atoi(argv[1]);

	bcm283x_gpio_config(DS, GPIO_OUTPUT);
	bcm283x_gpio_config(SHCP, GPIO_OUTPUT);
	bcm283x_gpio_config(STCP, GPIO_OUTPUT);

	//init
	bcm283x_gpio_write(DS, 0);
	bcm283x_gpio_write(SHCP, 0);
	bcm283x_gpio_write(STCP, 0);

	int i;
	for(i=0; i<8; i++) {
		bcm283x_gpio_write(DS, (v >> i) & 0x1);
		bcm283x_gpio_write(SHCP, 0);
		bcm283x_gpio_write(SHCP, 1);
	}
	bcm283x_gpio_write(STCP, 0);
	bcm283x_gpio_write(STCP, 1);
	return 0;
}
