//driver for waveshare.RPiLCD3.5(C)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <ewoksys/vdevice.h>
#include <arch/bcm283x/spi.h>
#include <ili9486/ili9486.h>
#include <xpt2046/xpt2046.h>

typedef struct {
	int32_t shm_id;
	void* shm;
	uint32_t size;
} fb_dma_t;

static int lcd_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;

	uint8_t* data = (uint8_t*)dma->shm;
	data[dma->size] = 1;
	ili9486_flush(data, dma->size);
	data[dma->size] = 0;

	return 0;
}

static int32_t lcd_dma(int fd, int from_pid, uint32_t node, int* size, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->shm_id;
}

static int lcd_fcntl(int fd, int from_pid, fsinfo_t* node, 
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)in;
	(void)p;

	if(cmd == 0) { //get lcd size
		PF->addi(out, LCD_WIDTH)->addi(out, LCD_HEIGHT)->addi(out, 16);
	}
	return 0;
}

static int lcd_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)in;
	(void)p;

	if(cmd != 0) { //get fb size and bpp
		PF->addi(ret, LCD_WIDTH)->addi(ret, LCD_HEIGHT)->addi(ret, 16);
	}	
	return 0;
}

static int tp_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;

	if(size >= 6) {
		uint16_t* d = (uint16_t*)buf;
		xpt2046_read(&d[0], &d[1], &d[2]);
	}
	return 6;	
}

int main(int argc, char** argv) {
	const int lcd_dc = 24;
	const int lcd_cs = 8;
  const int lcd_rst = 25;
  const int tp_cs = 7;
  const int tp_irq = 17;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/rpi_lcd";

	bcm283x_spi_init();
	xpt2046_init(tp_cs, tp_irq, 64);
	ili9486_init(lcd_dc, lcd_cs, lcd_rst, 2);

	uint32_t sz = LCD_HEIGHT*LCD_WIDTH*4;
	fb_dma_t dma;
	key_t key = (((int32_t)&dma) << 16) | getpid(); 
	dma.shm_id = shmget(key, sz+1, 0660|IPC_CREAT|IPC_EXCL);
	if(dma.shm_id == -1)
		return -1;
	dma.shm = shmat(dma.shm_id, 0, 0);
	if(dma.shm == NULL)
		return -1;
	dma.size = sz;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "rpi_lcd(c)");
	dev.dma   = lcd_dma;
	dev.flush = lcd_flush;
	dev.fcntl = lcd_fcntl;
	dev.dev_cntl = lcd_dev_cntl;
	dev.read = tp_read;

	dev.extra_data = &dma;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);

	shmdt(dma.shm);
	return 0;
}
