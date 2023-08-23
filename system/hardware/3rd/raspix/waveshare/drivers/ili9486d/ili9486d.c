#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <fb/fb.h>
#include <sys/shm.h>
#include <sys/vdevice.h>
#include <ili9486/ili9486.h>

typedef struct {
	void* data;
	uint32_t size;
} fb_dma_t;

static int lcd_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	ili9486_flush(dma->data, dma->size);
	return 0;
}

static void* lcd_dma(int fd, int from_pid, fsinfo_t* info, int* size, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->data;
}

static int lcd_fcntl(int fd, int from_pid, fsinfo_t* info, 
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)in;
	(void)p;

	if(cmd == FB_CNTL_GET_INFO) { //get lcd size
		PF->addi(out, LCD_WIDTH)->addi(out, LCD_HEIGHT)->addi(out, 16);
	}
	return 0;
}

static int lcd_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)in;
	(void)p;

	if(cmd == FB_DEV_CNTL_GET_INFO) { //get fb size and bpp
		PF->addi(ret, LCD_WIDTH)->addi(ret, LCD_HEIGHT)->addi(ret, 16);
	}	
	return 0;
}

/*LCD_RS	Instruction/Data Register selection
  LCD_CS    LCD chip selection, low active
  LCD_RST   LCD reset
  */
int main(int argc, char** argv) {
	int lcd_rs = 24;
	int lcd_cs = 8;
	int lcd_rst = 25;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/rpi_lcd";
	if(argc > 4) {
		lcd_rs = atoi(argv[2]);
		lcd_cs = atoi(argv[3]);
		lcd_rst = atoi(argv[4]);
	}

	bcm283x_spi_init();
	ili9486_init(lcd_rs, lcd_cs, lcd_rst, 128);

	uint32_t sz = LCD_HEIGHT*LCD_WIDTH*4;
	fb_dma_t dma;
	dma.data = shm_alloc(sz, 1);
	if(dma.data == NULL)
		return -1;
	dma.size = sz;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "rpi_lcd(c)");
	dev.dma   = lcd_dma;
	dev.flush = lcd_flush;
	dev.fcntl = lcd_fcntl;
	dev.dev_cntl = lcd_dev_cntl;

	dev.extra_data = &dma;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);

	shm_unmap(dma.data);
	return 0;
}
