#include <bcm283x/sd.h>
#include <dev/sd.h>
#include <bcm283x/gpio.h>

void sd_init_gpio(void){
	int64_t r;
	// GPIO_IO_CD
	r = *GPIO_FSEL4;
	r &= ~(7<<(7*3));
	*GPIO_FSEL4 = r;
	*GPIO_PUD=2;
	_delay(150);
	
	*GPIO_PUDCLK1 = (1<<15);
	_delay(150);
	
	*GPIO_PUD = 0;
	*GPIO_PUDCLK1 = 0;
	r = *GPIO_HEN1;
	r |= 1<<15;
	*GPIO_HEN1 = r;

	// GPIO_IO_CLK, GPIO_IO_CMD
	r = *GPIO_FSEL4;
	r |= (7<<(8*3)) | (7<<(9*3));
	*GPIO_FSEL4 = r;
	*GPIO_PUD=2;
	_delay(150);
	*GPIO_PUDCLK1 = (1<<16)|(1<<17); 
	_delay(150);
	*GPIO_PUD = 0; 
	*GPIO_PUDCLK1 = 0;

	// GPIO_IO_DAT0, GPIO_IO_DAT1, GPIO_IO_DAT2, GPIO_IO_DAT3
	r = *GPIO_FSEL5;
	r |= (7<<(0*3)) | (7<<(1*3)) | (7<<(2*3)) | (7<<(3*3));
	*GPIO_FSEL5 = r;
	*GPIO_PUD = 2; 
	_delay(150);
	*GPIO_PUDCLK1 = (1<<18) | (1<<19) | (1<<20) | (1<<21);
	_delay(150);
	*GPIO_PUD = 0;
	*GPIO_PUDCLK1 = 0;
}

int32_t sd_init(void) {
	return bcm283x_sd_init();
}

int32_t sd_dev_read(int32_t sector) {
	return bcm283x_sd_read(sector);
}

int32_t sd_dev_read_done(void* buf) {
	return bcm283x_sd_read_done(buf);
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return bcm283x_sd_write(sector, buf);
}

int32_t sd_dev_write_done(void) {
	return bcm283x_sd_write_done();
}
