#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <dev/devserv.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <syscall.h>
#include <sramdisk.h>

static ram_disk_t _init_ram_disk;

static int32_t initfs_mount(uint32_t node, int32_t index) {
	(void)index;

	ram_file_t* f = _init_ram_disk.head;	
	while(f != NULL) {
		vfs_add(node, f->name, f->size, NULL);
		f = f->next;
	}
	return 0;
}

static int32_t readS_ram_disk(uint32_t node, int seek, char* buf, uint32_t size) {
	fs_info_t info;
	if(vfs_node_info(node, &info) != 0)
		return -1;
	
	const char* fname = info.name;
	int32_t fsize = 0;

	const char*p = ram_disk_read(&_init_ram_disk, fname, &fsize);
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

int32_t initfs_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	if(size == 0) {
		return 0;
	}
	return readS_ram_disk(node, seek, buf, size);
}

int main() {
	void *ramdisk_base = (void*)syscall0(SYSCALL_INITRD_CLONE);	
	if(ramdisk_base == NULL) {
		return -1;
	}
	ram_disk_open((const char*)ramdisk_base, &_init_ram_disk, malloc);


	device_t dev = {0};
	dev.mount = initfs_mount;
	dev.read = initfs_read;
	dev_run(&dev, "dev.initfs", 0, "/initfs", false);

	ram_disk_close(&_init_ram_disk, free);
	free(ramdisk_base);
	return 0;
}
