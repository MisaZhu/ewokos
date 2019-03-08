#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <dev/devserv.h>
#include <vfs/vfs.h>
#include <syscall.h>
#include <sramdisk.h>

static RamDiskT _initRamDisk;

static int32_t initfsMount(uint32_t node, int32_t index) {
	(void)index;

	RamFileT* f = _initRamDisk.head;	
	while(f != NULL) {
		vfsAdd(node, f->name, f->size);
		f = f->next;
	}
	return 0;
}

static int32_t readSRamDisk(uint32_t node, int seek, char* buf, uint32_t size) {
	FSInfoT info;
	if(vfsNodeInfo(node, &info) != 0)
		return -1;
	
	const char* fname = info.name;
	int32_t fsize = 0;

	const char*p = ramdiskRead(&_initRamDisk, fname, &fsize);
	if(p == NULL || fsize == 0)
		return 0;

	int32_t restSize = fsize - seek; /*seek*/
	if(restSize <= 0) {
		return 0;
	}

	if(size > (uint32_t)restSize)
		size = restSize;

	memcpy(buf, p+seek, size);
	return size;
}

int32_t initfsRead(uint32_t node, void* buf, uint32_t size, uint32_t seek) {
	if(size == 0) {
		return 0;
	}
	return readSRamDisk(node, seek, buf, size);
}

void _start() {
	void *ramdiskBase = (void*)syscall0(SYSCALL_INITRD_CLONE);	
	if(ramdiskBase == NULL) {
		exit(0);
	}
	ramdiskOpen((const char*)ramdiskBase, &_initRamDisk, malloc);


	DeviceT dev = {0};
	dev.mount = initfsMount;
	dev.read = initfsRead;
	devRun(&dev, "dev.initfs", 0, "/initfs", false);

	ramdiskClose(&_initRamDisk, free);
	free(ramdiskBase);
	exit(0);
}
