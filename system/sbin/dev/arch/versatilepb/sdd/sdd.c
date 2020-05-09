#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>

#define SECTOR_SIZE 512

static int32_t read_sector(int32_t sector, void* buf) {
	if(syscall1(SYS_SDC_READ, sector) != 0)
		return -1;
	while(1) {
		if(syscall1(SYS_SDC_READ_DONE, (int32_t)buf)  == 0)
			break;
	}
	return 0;
}

static int32_t write_sector(int32_t sector, const void* buf) {
	if(syscall2(SYS_SDC_WRITE, sector, (int32_t)buf) != 0)
		return -1;
	while(1) {
		if(syscall0(SYS_SDC_WRITE_DONE)  == 0)
			break;
	}
	return 0;
}

static int sd_read_block(int from_pid, void* buf, int size, int index, void* p) {
	(void)from_pid;
	(void)buf;
	(void)p;

	if(size < SECTOR_SIZE)
		return 0;
	
	if(read_sector(index, buf) != 0)
		return -1;
	return SECTOR_SIZE;	
}

static int sd_write_block(int from_pid, const void* buf, int size, int index, void* p) {
	(void)from_pid;
	(void)buf;
	(void)p;

	if(size < SECTOR_SIZE)
		return 0;

	if(write_sector(index, buf) != 0)
		return -1;
	return SECTOR_SIZE;	
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "sd");
	dev.read_block = sd_read_block;
	dev.write_block = sd_write_block;

	device_run(&dev, NULL, FS_TYPE_BLOCK);
	return 0;
}
