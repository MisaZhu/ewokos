#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>

static int stdin_read(int fd,
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
	return read(0, buf, size);	
}

static int stdin_write(int fd, 
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
	return write(0, buf, size);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/stdin";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "stdin");
	dev.read = stdin_read;
	dev.write = stdin_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
