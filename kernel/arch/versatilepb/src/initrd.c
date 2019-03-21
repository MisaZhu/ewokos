#include <dev/initrd.h>
#include <hardware.h>
#include <printk.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <proc.h>
#include <sramdisk.h>
#include <kstring.h>

static char* _init_ram_disk_base = 0;
static uint32_t _init_ram_disk_size = 0;
static ram_disk_t _init_ram_disk;

int32_t load_initrd() {
	/*map initfs memory, */
	printk("Load init ramdisk ... ");
	map_pages(_kernel_vm, 
		get_initrd_base_phy(),
		get_initrd_base_phy(),
		get_initrd_base_phy()+get_initrd_size(),
		AP_RW_D);

	_init_ram_disk_size = get_initrd_size();
	_init_ram_disk_base = km_alloc(_init_ram_disk_size);
	if(_init_ram_disk_base == NULL) {
		printk("failed!\n");
		return -1;
	}
	memcpy(_init_ram_disk_base, (void*)get_initrd_base_phy(), get_initrd_size());
	ram_disk_open((const char*)_init_ram_disk_base, &_init_ram_disk, km_alloc);
	printk("ok\n");
	return 0;
}

void close_initrd() {
	if(_init_ram_disk_base != NULL) {
		ram_disk_close(&_init_ram_disk, km_free);
		km_free(_init_ram_disk_base);
		_init_ram_disk_base = NULL;
		_init_ram_disk_size = 0;
	}
}

void* clone_initrd() {
	void * ret = (void*)pmalloc(_init_ram_disk_size);
	memcpy(ret, _init_ram_disk_base, _init_ram_disk_size);
	return ret;
}

const char* read_initrd(const char* name, int32_t *size) {
	return ram_disk_read(&_init_ram_disk, name, size);
}
