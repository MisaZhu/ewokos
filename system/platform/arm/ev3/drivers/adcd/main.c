#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/interrupt.h>

#include <arch/ev3/spi.h>
#include <bsp/bsp_gpio.h>

#define BIT(x)  (0x1<<(x))
#define ADC_SPI_CS		(3)
#define ADC_POWEWR_PIN	(101)
#define ADC_BATTERY_EN	(6)
#define ADC_CS_PIN		(130)

#define TI_ADS7950_CR_MANUAL    BIT(12)
#define TI_ADS7950_CR_WRITE BIT(11)
#define TI_ADS7950_CR_CHAN(ch)  ((ch) << 7)
#define TI_ADS7950_CR_RANGE_5V  BIT(6)

#define SWAP(x)	((((x)<<8)&0xFF00)|((((x)>>8)&0xFF)))

#define MAX_CH	(16)

uint16_t _adc_buf[MAX_CH];
uint8_t	 _ch = 0;

static int adcd_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)size;
	(void)offset;
	(void)p;

	if(size > sizeof(_adc_buf))
		size = sizeof(_adc_buf);

	memcpy(buf, _adc_buf, size);
    return size;
}

static uint32_t _fps = 30;
static int adcd_loop(void* p){
	uint64_t tik = kernel_tic_ms(0);
	uint32_t tm = 1000/_fps;

    uint16_t rx,tx;
    tx = SWAP(TI_ADS7950_CR_MANUAL | TI_ADS7950_CR_RANGE_5V | TI_ADS7950_CR_WRITE|TI_ADS7950_CR_CHAN(_ch));
    davinci_spi_read_write(2, &rx, &tx, SPI_XFER_END);
	_adc_buf[(_ch + 14)% MAX_CH] = (SWAP(rx)&0xFFF) * 5000/4096;
	_ch = (_ch + 1) % MAX_CH;

	uint32_t gap = (uint32_t)(kernel_tic_ms(0) - tik);
	if(gap < tm) {
		gap = tm - gap;
		proc_usleep(gap*1000/16);
	}
}

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "f:");
		if(c == -1)
			break;

		switch (c) {
		case 'f':
			_fps = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_fps = 30;
	int argind = doargs(argc, argv);
	const char* mnt_point =  "/dev/adc0";
	if(argind < argc)
		mnt_point = argv[argind];

	bsp_gpio_init();
    bsp_gpio_config(ADC_POWEWR_PIN, GPIO_OUTPUT);
	bsp_gpio_config(ADC_BATTERY_EN, GPIO_OUTPUT);
	bsp_gpio_write(ADC_POWEWR_PIN, 1);
	bsp_gpio_write(ADC_BATTERY_EN, 1);

	davinci_spi_init(0);
	davinci_spi_claim_bus(ADC_SPI_CS);


	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "adcd");
	dev.read = adcd_read;
	dev.loop_step = adcd_loop;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);

	davinci_spi_release_bus();
	return 0;
}
