#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>

static int stdout_read(int fd,
		int from_pid,
		fsinfo_t* info,
		void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)buf;
	(void)size;
	(void)offset;
	(void)p;
	return 0;	
}

static int stdout_write(int fd, 
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
	return write(1, buf, size);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/stdout";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "stdout");
	dev.read = stdout_read;
	dev.write = stdout_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
