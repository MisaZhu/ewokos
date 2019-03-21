#include <dev/initrd.h>
#include <hardware.h>
#include <printk.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <proc.h>
#include <sramdisk.h>
#include <kstring.h>

static char* _initRamDiskBase = 0;
static uint32_t _initRamDiskSize = 0;
static ram_disk_t _initRamDisk;

int32_t load_initrd() {
	/*map initfs memory, */
	printk("Load init ramdisk ... ");
	map_pages(_kernel_vm, get_initrd_base_phy(), get_initrd_base_phy(), get_initrd_base_phy()+get_initrd_size(), AP_RW_D);

	_initRamDiskSize = get_initrd_size();
	_initRamDiskBase = km_alloc(_initRamDiskSize);
	if(_initRamDiskBase == NULL) {
		printk("failed!\n");
		return -1;
	}
	memcpy(_initRamDiskBase, (void*)get_initrd_base_phy(), get_initrd_size());
	ram_disk_open((const char*)_initRamDiskBase, &_initRamDisk, km_alloc);

	printk("ok\n");
	return 0;
}

void close_initrd() {
	if(_initRamDiskBase != NULL) {
		ram_disk_close(&_initRamDisk, km_free);
		km_free(_initRamDiskBase);
		_initRamDiskBase = NULL;
		_initRamDiskSize = 0;
	}
}

void* clone_initrd() {
	void * ret = (void*)pmalloc(_initRamDiskSize);
	memcpy(ret, _initRamDiskBase, _initRamDiskSize);
	return ret;
}

const char* read_initrd(const char* name, int32_t *size) {
	return ram_disk_read(&_initRamDisk, name, size);
}
