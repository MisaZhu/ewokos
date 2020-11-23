#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/shm.h>
#include <sys/vdevice.h>
#include <sys/vfs.h>

static uint32_t LCD_HEIGHT = 240;
static uint32_t LCD_WIDTH = 240;

typedef struct {
	void* data;
	uint32_t size;
	int32_t shm_id;
} fb_dma_t;

int  do_flush(const void* buf, uint32_t size);
static int lcd_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;

	return do_flush(dma->data, dma->size);
}

static int lcd_dma(int fd, int from_pid, fsinfo_t* info, int* size, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->shm_id;
}

static int lcd_fcntl(int fd, int from_pid, fsinfo_t* info, 
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
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

void lcd_init(uint32_t w, uint32_t h, uint32_t rot);
int main(int argc, char** argv) {
	uint32_t w=240, h=240, rot=0;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/lcd";

	if(argc >=5) {
		w = atoi(argv[2]);
		h = atoi(argv[3]);
		rot = atoi(argv[4]);
	}
	LCD_HEIGHT = h;
	LCD_WIDTH = w;
	lcd_init(w, h, rot);

	uint32_t sz = LCD_HEIGHT*LCD_WIDTH*4;
	fb_dma_t dma;
	dma.shm_id = shm_alloc(sz, 1);
	if(dma.shm_id <= 0)
		return -1;
	dma.size = sz;
	dma.data = shm_map(dma.shm_id);
	if(dma.data == NULL)
		return -1;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "lcd");
	dev.dma   = lcd_dma;
	dev.flush = lcd_flush;
	dev.fcntl = lcd_fcntl;
	dev.dev_cntl = lcd_dev_cntl;

	dev.extra_data = &dma;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);

	shm_unmap(dma.shm_id);
	return 0;
}
