#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>

#define DFT_LOG_DEV "/dev/tty0"
#define MAX_LOG_DEV 4
static int _fds[MAX_LOG_DEV]; 
static int _fds_num = 0;

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

	int i;
	for(i=0; i<_fds_num; i++) {
		if(_fds[i] > 0)
			write(_fds[i], buf, size);
	}
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/klog";
	if(argv > 2) {
		int i;
		for(i=0; i<MAX_LOG_DEV && i<(argv-2); i++) {
			_fds[i] = open(argv[i+2], O_WRONLY);
		}
		_fds_num = i;
	}
	else {
		_fds[0] = open(DFT_LOG_DEV, O_WRONLY);
		_fds_num = 1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "klog");
	dev.write = klog_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
