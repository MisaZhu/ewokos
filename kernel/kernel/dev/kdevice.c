#include <dev/device.h>
#include <dev/kdevice.h>
#include <dev/framebuffer.h>
#include <dev/sd.h>
#include <kstring.h>
#include <kprintf.h>

static dev_t _devs[DEV_NUM];

/*
static int32_t char_dev_read(dev_t* dev, void* data, uint32_t size) {
	if(dev == NULL)
		return -1;

	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c;
		if(charbuf_pop(&dev->io.ch.buffer, &c) != 0)
			break;
		((char*)data)[i] = c;
	}
	return i;
}

static int32_t char_dev_ready_read(dev_t* dev) {
	if(dev->io.ch.buffer.size > 0)
		return 0;
	return -1;
}
*/

void dev_init(void) {
	dev_t* dev;

	printf("    %16s ", "mmc_sd");
	dev = &_devs[DEV_SD];
	memset(dev, 0, sizeof(dev_t));
	if(sd_init(dev) == 0) {
		dev->type = DEV_TYPE_BLOCK;
		dev->io.block.read = sd_dev_read;
		dev->io.block.read_done = sd_dev_read_done;
		dev->io.block.write = sd_dev_write;
		dev->io.block.write_done = sd_dev_write_done;
		dev->state = DEV_STATE_INITED;
		printf("[OK]\n\n");
	}
	else
		printf("[Failed!]\n");
}

dev_t* get_dev(uint32_t type) {
	if(type >= DEV_NUM)
		return NULL;
	return &_devs[type];
}

int32_t dev_op(dev_t* dev, int32_t opcode, int32_t arg) {
	if(dev->op == NULL)
		return -1;
	return dev->op(dev, opcode, arg);
}

int32_t dev_ready_read(dev_t* dev) {
	if(dev->ready_read == NULL)
		return -1;
	return dev->ready_read(dev);
}

int32_t dev_ready_write(dev_t* dev) {
	if(dev->ready_write == NULL)
		return -1;
	return dev->ready_write(dev);
}

/*return : -1 for error/closed, 0 for retry*/
int32_t dev_ch_read(dev_t* dev, void* data, uint32_t size) {
	if(dev->io.ch.read == NULL)
		return -1;
	return dev->io.ch.read(dev, data, size);
}

/*return : -1 for error/closed, 0 for retry*/
int32_t dev_ch_write(dev_t* dev, void* data, uint32_t size) {
	if(dev->io.ch.write == NULL)
		return -1;
	return dev->io.ch.write(dev, data, size);
}

/*return : -1 for error/closed, 0 for retry*/
int32_t dev_block_read(dev_t* dev, int32_t bid) {
	if(dev->io.block.read == NULL)
		return -1;
	return dev->io.block.read(dev, bid);
}

/*return : -1 for error/closed, 0 for retry*/
int32_t dev_block_write(dev_t* dev, int32_t bid, const char* buf) {
	if(dev->io.block.write == NULL)
		return -1;
	return dev->io.block.write(dev, bid, buf);
}

/*return : -1 for not, 0 for done*/
int32_t dev_block_read_done(dev_t* dev, char* buf) {
	if(dev->io.block.read_done == NULL)
		return -1;
	return dev->io.block.read_done(dev, buf);
}

/*return : -1 for not, 0 for done*/
int32_t dev_block_write_done(dev_t* dev) {
	if(dev->io.block.write_done == NULL)
		return -1;
	return dev->io.block.write_done(dev);
}

