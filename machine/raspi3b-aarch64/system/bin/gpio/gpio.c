#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arch/bcm283x/gpio.h>

int main(int argc, char* argv[]) {
	bcm283x_gpio_init();
	int pin = 0;
	int v = 0;
	if(argc < 2) {
		printf("Usage: gpio <pin_no> {1/0/u/d}\n");
		return -1;
	}

	pin = atoi(argv[1]);
	if(argc > 2) {
		if(argv[2][0] == 'u') {
			bcm283x_gpio_pull(pin, GPIO_PULL_UP);
		}
		else if(argv[2][0] == 'd') {
			bcm283x_gpio_pull(pin, GPIO_PULL_DOWN);
		}
		else {
			v = atoi(argv[2]);
			//bcm283x_gpio_pull(pin, GPIO_PULL_DOWN);
			bcm283x_gpio_config(pin, GPIO_OUTPUT);
			bcm283x_gpio_write(pin, v);
		}
	}
	else {
		//bcm283x_gpio_config(pin, GPIO_INPUT);
		v = bcm283x_gpio_read(pin);
		printf("%d\n", v);
	}
	return 0;
}
