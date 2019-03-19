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
static RamDiskT _initRamDisk;

int32_t loadInitRD() {
	/*map initfs memory, */
	mapPages(_kernelVM, getInitRDBasePhy(), getInitRDBasePhy(), getInitRDBasePhy()+getInitRDSize(), AP_RW_D);

	_initRamDiskSize = getInitRDSize();
	_initRamDiskBase = kmalloc(_initRamDiskSize);
	if(_initRamDiskBase == NULL) {
		printk("panic: initramdisk decode failed!\n");
		return -1;
	}
	memcpy(_initRamDiskBase, (void*)getInitRDBasePhy(), getInitRDSize());
	ramdiskOpen((const char*)_initRamDiskBase, &_initRamDisk, kmalloc);
	return 0;
}

void closeInitRD() {
	if(_initRamDiskBase != NULL) {
		ramdiskClose(&_initRamDisk, kmfree);
		kmfree(_initRamDiskBase);
		_initRamDiskBase = NULL;
		_initRamDiskSize = 0;
	}
}

void* cloneInitRD() {
	void * ret = (void*)pmalloc(_initRamDiskSize);
	memcpy(ret, _initRamDiskBase, _initRamDiskSize);
	return ret;
}

const char* readInitRD(const char* name, int32_t *size) {
	return ramdiskRead(&_initRamDisk, name, size);
}
