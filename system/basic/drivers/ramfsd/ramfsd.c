#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/klog.h>
#include <stdio.h>

static int ramfs_read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;

	if(offset < 0)
		offset = 0;
	if((int)info->size < (size+offset))
		size = info->size-offset;
	if(size < 0)
		return 0;

	char* data = (char*)info->data;
	memcpy(buf, data+offset, size);
	return size;	
}

static int ramfs_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;

	if(offset < 0)
		offset = 0;
	if(size <= 0)
		return size;

	char* data = (char*)info->data;
	data = (char*)realloc(data, size + offset);
	memcpy(data+offset, buf, size);
	info->data = (uint32_t)data;
	info->size = size+offset;
	vfs_set(info);
	return size;
}

static int ramfs_create(fsinfo_t* info_to, fsinfo_t* info, void* p) {
	(void)info_to;
	(void)info;
	(void)p;
	return 0;
}

static int ramfs_unlink(fsinfo_t* info, const char* fname, void* p) {
	(void)info;
	(void)fname;
	(void)p;
	char* data = (char*)info->data;
	if(data != NULL)
		free(data);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/tmp";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "ramfs");
	dev.create = ramfs_create;
	dev.read = ramfs_read;
	dev.write = ramfs_write;
	dev.unlink = ramfs_unlink;
	
	device_run(&dev, mnt_point, FS_TYPE_DIR);
	return 0;
}
