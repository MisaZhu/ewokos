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

static int ramfs_read(int fd, int from_pid, uint32_t node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;

	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0)
		return 0;

	if(offset < 0)
		offset = 0;
	if((int)info.size < (size+offset))
		size = info.size-offset;
	if(size < 0)
		return 0;

	char* data = (char*)info.data;
	memcpy(buf, data+offset, size);
	return size;	
}

static int ramfs_write(int fd, int from_pid, uint32_t node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;

	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0)
		return 0;

	if(offset < 0)
		offset = 0;
	if(size <= 0)
		return size;

	char* data = (char*)info.data;
	data = (char*)realloc(data, size + offset);
	memcpy(data+offset, buf, size);
	info.data = (uint32_t)data;
	info.size = size+offset;
	vfs_set(&info);
	return size;
}

static int ramfs_unlink(uint32_t node, const char* fname, void* p) {
	(void)fname;
	(void)p;
	fsinfo_t info;
	if(vfs_get_by_node(node, &info) != 0)
		return -1;
	char* data = (char*)info.data;
	if(data != NULL)
		free(data);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/tmp";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "ramfs");
	dev.read = ramfs_read;
	dev.write = ramfs_write;
	dev.unlink = ramfs_unlink;
	
	device_run(&dev, mnt_point, FS_TYPE_DIR);
	return 0;
}
