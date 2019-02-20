#include "dev/initrd.h"
#include "vfs/fstree.h"
#include "sramdisk.h"
#include "kernel.h"
#include "kstring.h"

static bool rdMount(int32_t index, TreeNodeT* node) {
	(void)index;
	RamFileT* f = _initRamDisk.head;
	TreeNodeT* n;
	while(f != NULL) {
		n = fsTreeSimpleAdd(node, f->name);
		if(n != NULL) {
			FSN(n)->type = FS_TYPE_FILE;
			FSN(n)->mount = FSN(node)->mount;
		}
		f = f->next;
	}
	FSN(node)->type = FS_TYPE_DIR;
	return true;
}

static bool rdOpen(int32_t index, TreeNodeT* node, int flags) {
	(void)index;
	(void)flags;
	return ramdiskHas(&_initRamDisk, FSN(node)->name);
}

static bool rdClose(int32_t index, TreeNodeT* node) {
	(void)index;
	(void)node;
	return true;
}

static int rdRead(int32_t index, TreeNodeT* node, char* buf, uint32_t size, int seek) {
	(void)index;
	int32_t fsize;
	const char*p = ramdiskRead(&_initRamDisk, FSN(node)->name, &fsize);
	if(p == NULL || fsize == 0)
		return -1;

	int32_t restSize = fsize - seek;
	if(restSize <= 0)
		return 0;

	if((int32_t)size > restSize)
		size = restSize;

	memcpy(buf, p+seek, size);
	return size;
}

static bool rdInfo(int32_t index, TreeNodeT* node, FSInfoT* info) {
	(void)index;
	int32_t fsize = 0;
	const char*p = ramdiskRead(&_initRamDisk, FSN(node)->name, &fsize);
	if(p == NULL)
		return false;
	
	info->type = FS_TYPE_FILE;
	info->size = fsize;
	return true;
}

static DeviceT _initrdDev = {
	"",
	rdMount, //bool (*mount)(int32_t index, TreeNodeT* node);
	rdOpen, //bool (*open)(int32_t index, TreeNodeT* node, int32_t flag);
	rdClose, //bool (*close)(int32_t index, TreeNodeT* node);
	rdRead, //int32_t read(int32_t index, TreeNodeT* node, char* buf, uint32_t size, int seek);
	NULL, //int32_t write(int32_t index, TreeNodeT* node, char* buf, uint32_t size, int seek);
	rdInfo, //bool (*info)(int32_t index, TreeNodeT* node, FSInfoT* info);
	NULL, //bool (*ioctl)(int32_t index, TreeNodeT* node, int32_t cmd, int32_t value);
	NULL, //bool (*add)(int32_t index, TreeNodeT* node);
	NULL  //bool (*del)(int32_t index, TreeNodeT* node);
};

DeviceT* devInitrd() {
	return &_initrdDev;
}
