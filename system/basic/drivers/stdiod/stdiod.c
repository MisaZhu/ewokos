#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>

static int _io_fd = 0;

static int stdio_read(int fd,
		int from_pid,
		fsinfo_t* info,
		void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)p;
	return read(_io_fd, buf, size);	
}

static int stdio_write(int fd, 
		int from_pid,
		fsinfo_t* info,
		const void* buf,
		int size,
		int offset,
		void* p) {
		
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)p;
	return write(_io_fd, buf, size);
}

int main(int argc, char** argv) {
	_io_fd = 0;
	char mnt_point[FS_FULL_NAME_MAX];
	const char* dev_name = argc < 2 ?  "stdin": argv[1];
	snprintf(mnt_point, FS_FULL_NAME_MAX, "/dev/%s", dev_name);

	if(strcmp(dev_name, "stdout") == 0)
		_io_fd = 1;
	else if(strcmp(dev_name, "stderr") == 0)
		_io_fd = 2;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, dev_name);
	dev.read = stdio_read;
	dev.write = stdio_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
