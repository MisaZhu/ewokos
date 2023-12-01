#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/mmio.h>
#include <sys/dma.h>
#include <sys/interrupt.h>
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/i2c.h>

static int usb_step(void* p) {
	(void)p;	
    int irq = bcm283x_gpio_read(2);
	if(!irq){
		uint8_t reg = i2c_getb(0x34, 0x32);	
		i2c_putb(0x34, 0x32, reg | 0x80);
		for(int i = 0; i < 60 ; i++){
			bcm283x_gpio_clr(i);
			bcm283x_gpio_config(i, GPIO_OUTPUT);
		}
	}
    usleep(100000); 
	return 0;
}


int main(int argc, char** argv) {

	uint32_t _mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;

    bcm283x_gpio_init();
    bcm283x_gpio_config(2, GPIO_INPUT);

	i2c_init(0,1);
	i2c_putb(0x34, 0x42, 0x3);
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/pmu0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "pmu");
	dev.loop_step = usb_step;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
