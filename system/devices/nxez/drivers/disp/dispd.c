#include <arch/bcm283x/gpio.h>
#include <sys/vdevice.h>
#include <string.h>
#include <unistd.h>

#define DI   25
#define CLK  5

static void delay(void) {
	usleep(0);
}

static void start_bus(void) {
	bcm283x_gpio_write(CLK, 1);
	bcm283x_gpio_write(DI, 1);
	delay();
	bcm283x_gpio_write(DI, 0);
	delay();
	bcm283x_gpio_write(CLK, 0);
	delay();
}

static void stop_bus(void) {
	bcm283x_gpio_write(CLK, 0);
	delay();
	bcm283x_gpio_write(DI, 0);
	delay();
	bcm283x_gpio_write(CLK, 1);
	delay();
	bcm283x_gpio_write(DI, 1);
}

static void set_bit(int bit) {
	bcm283x_gpio_write(CLK, 0);
	delay();
	bcm283x_gpio_write(DI, bit);
	delay();
	bcm283x_gpio_write(CLK, 1);
	delay();
}

static void set_byte(int data) {
	int i;
	for(i=0; i<8; i++) {
		set_bit((data >> i) & 0x1);
	}
	bcm283x_gpio_write(CLK, 0);
	delay();
	bcm283x_gpio_write(DI, 1);
	delay();
	bcm283x_gpio_write(CLK, 1);
	delay();
}

static void set_command(int command) {
	start_bus();
	set_byte(command);
	stop_bus();
}

static void set_data(int address, int data) {
	start_bus();
	set_byte(address);
	set_byte(data);
	stop_bus();
}

static void init(void) {
	bcm283x_gpio_init();
	bcm283x_gpio_config(DI, GPIO_OUTPUT);
	bcm283x_gpio_config(CLK, GPIO_OUTPUT);
}

static int _nums[] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x40 };
static int disp_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;

	if(size < 8)
		return 0;

	set_command(0x44);
	char* cp = (char*)buf;
	for(int i=0; i<8; ) {
		char c0 = cp[i];	
		char c1 = cp[i+1];	
		int v = 0;
		
		if(c0 == '-')
			v = _nums[10];
		else if(c0 >= '0' && c0 <= '9')
			v = _nums[c0 - '0'];

		if(c1 == '.') {
			v |= 0x80;
		}

		set_data((0xc0+i/2), v);
		i+=2;
	}
	set_command(0x8f);
	return 8;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/disp";
	init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "nxez_disp");
	dev.write = disp_write;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
