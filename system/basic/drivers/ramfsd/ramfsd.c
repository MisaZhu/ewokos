#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/wait.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <stdio.h>

static int ramfs_read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;

	if(offset < 0)
		offset = 0;
	if((int)info->stat.size < (size+offset))
		size = info->stat.size-offset;
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
	info->stat.size = size+offset;
	vfs_set(info);
	return size;
}

static int ramfs_unlink(fsinfo_t* info, const char* fname, void* p) {
	(void)fname;
	(void)p;

	char* data = (char*)info->data;
	if(data != NULL)
		free(data);
	return vfs_del_node(info->node);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/tmp";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "ramfs");
	dev.read = ramfs_read;
	dev.write = ramfs_write;
	dev.unlink = ramfs_unlink;
	
	device_run(&dev, mnt_point, FS_TYPE_DIR, 0777);
	return 0;
}
