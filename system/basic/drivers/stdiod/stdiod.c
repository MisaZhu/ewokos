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
	const char* mnt_point = argc < 2 ?  "/dev/stdin": argv[1];

	if(strstr(mnt_point, "stdout") != 0)
		_io_fd = 1;
	else if(strstr(mnt_point, "stderr") != 0)
		_io_fd = 2;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "stdio");
	dev.read = stdio_read;
	dev.write = stdio_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
