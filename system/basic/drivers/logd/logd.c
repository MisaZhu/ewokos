#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/charbuf.h>

static int log_read(int fd,
		int from_pid,
		fsinfo_t* node,
		void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;

	uint8_t *data = (uint8_t *)buf;
	charbuf_t* buffer = (charbuf_t*)p;
	int i = 0;
	while(i<size) {
		if(charbuf_pop(buffer, &data[i]) != 0)
			break;
		i++;
	}

	return i==0 ? VFS_ERR_RETRY : i;
}

static int log_write(int fd, 
		int from_pid,
		fsinfo_t* node,
		const void* buf,
		int size,
		int offset,
		void* p) {
		
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)buf;
	(void)offset;
	(void)p;

	charbuf_t* buffer = (charbuf_t*)p;
	uint8_t *data = (uint8_t *)buf;
	int i = 0;
	while(i<size) {
		if(charbuf_push(buffer, data[i], true) != 0)
			break;
		i++;
	}

	if(i > 0)
		proc_wakeup(RW_BLOCK_EVT);
	return i;
}

#define LOG_SIZE_DEFAULT (1024*64)
static uint32_t _log_size = LOG_SIZE_DEFAULT;
static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "b:");
		if(c == -1)
			break;

		switch (c) {
		case 'b':
			_log_size = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	const char* mnt_point = "/dev/log";
	int argind = doargs(argc, argv);
	if(argind < argc)
		mnt_point = argv[argind];

	if(_log_size < LOG_SIZE_DEFAULT)
		_log_size = LOG_SIZE_DEFAULT;

	charbuf_t* _buffer = charbuf_new(_log_size);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "null");
	dev.read = log_read;
	dev.write = log_write;
	dev.extra_data = _buffer;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);

	charbuf_free(_buffer);
	return 0;
}
