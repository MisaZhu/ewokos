#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>

static int _tty_fd = -1;
static int _cons_fd = -1;

static int klog_write(int fd, 
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

	if(_tty_fd > 0)
		write(_tty_fd, buf, size);
	if(_cons_fd > 0)
		write(_cons_fd, buf, size);
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/klog";
	_tty_fd = open("/dev/tty0", O_WRONLY);
	_cons_fd = open("/dev/console0", O_WRONLY);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "klog");
	dev.write = klog_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
