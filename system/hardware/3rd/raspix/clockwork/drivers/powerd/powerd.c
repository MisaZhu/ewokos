#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>
#include <ewoksys/interrupt.h>
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/i2c.h>

static uint8_t  _hasBattery = 0;
static uint8_t  _charging = 0;
static uint8_t 	_capacity = 0;

// 　　100%----4.20V
// 　　90%-----4.06V
// 　　80%-----3.98V
// 　　70%-----3.92V
// 　　60%-----3.87V
// 　　50%-----3.82V
// 　　40%-----3.79V
// 　　30%-----3.77V
// 　　20%-----3.74V
// 　　10%-----3.68V
// 　　5%------3.45V
// 　　0%------3.00V

static uint8_t adc2level(uint8_t adc){
	int vol = adc*72072/4095;
	if(vol > 4200)
		return 100;
	if(vol > 4060)
		return 90;
	if(vol > 3980)
		return 80;
	if(vol > 3920)
		return 70;
	if(vol > 3870)
		return 60;
	if(vol > 3820)
		return 50;
	if(vol > 3790)
		return 40;
	if(vol > 3770)
		return 30;
	if(vol > 3740)
		return 20;
	if(vol > 3680)
		return 10;
	if(vol > 3450)
		return 5;
	return 0;
}
static void power_off(){
		uint8_t reg = i2c_getb(0x34, 0x32);	
		i2c_putb(0x34, 0x32, reg | 0x80);
		for(int i = 0; i < 60 ; i++){
			bcm283x_gpio_clr(i);
			bcm283x_gpio_config(i, GPIO_OUTPUT);
		}
		while(1);
}

static int power_step(void* p) {
	(void)p;	
    int irq = bcm283x_gpio_read(2);
	if(!irq){
		power_off();
	}
	uint8_t state = i2c_getb(0x34, 0x01);	
	_hasBattery = !!(state & (0x1 << 5));
	_charging = !!(state & (0x1 << 6));
	_capacity = adc2level(i2c_getb(0x34, 0x78)); 
	//if(_capacity < 10)
		//power_off();
    proc_usleep(300000); 
	return 0;
}

static int power_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;

	uint8_t* data = (uint8_t *)buf;
	data[0] = _hasBattery;
	data[1] = _charging;
	data[2] = _capacity;
    return 3;
 }


int main(int argc, char** argv) {

	uint32_t _mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;

    bcm283x_gpio_init();
    bcm283x_gpio_config(2, GPIO_INPUT);

	i2c_init(0,1);
	i2c_putb(0x34, 0x42, 0x3);
	i2c_putb(0x34, 0x82, 0x80);
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/power0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "powerd");
	dev.loop_step = power_step;
	dev.read = power_read;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
