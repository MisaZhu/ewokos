#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/charbuf.h>

static int _keyb_fd = -1;
static int _active_pid = -1;
static charbuf_t *_buffer;

static int vkeyb_read(int fd,
		int from_pid,
		fsinfo_t* node,
		void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)node;
	(void)offset;
	(void)p;

	if(_keyb_fd < 0)
		return 0;

	if(_active_pid >= 0 && _active_pid != from_pid)
		return VFS_ERR_RETRY;

	char c;
	int res = charbuf_pop(_buffer, &c);
	if(res != 0)
		return VFS_ERR_RETRY;

	((char*)buf)[0] = c;
	return 1;
}

static int vkeyb_loop(void* p) {
	(void)p;
	
	int32_t c;
	if(read(_keyb_fd, &c, 1) == 1) {
		charbuf_push(_buffer, c, true);
		proc_wakeup(RW_BLOCK_EVT);
	}
	else {
		usleep(30000);
	}
	return 0;
}

static char* vkeyb_cmd(int from_pid, int argc, char** argv, void* p) {
	if(strcmp(argv[0], "active") == 0) {
		if(argc < 2) {
			printf("Usage: active <pid>\n");
			return NULL;
		}
		_active_pid = atoi(argv[1]);
		charbuf_clear(_buffer);
		char* ret = str_detach(str_new("OK"));
		proc_wakeup(RW_BLOCK_EVT);
		return ret;
	}
	return NULL;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/vkeyb";
	const char* keyb_dev = argc > 2 ? argv[2]: "/dev/keyb0";
	_active_pid = -1;

	_keyb_fd = open(keyb_dev, O_RDONLY | O_NONBLOCK);
	if(_keyb_fd < 0) {
		printf("Can not open keyboard device (%s)!\n", keyb_dev);
		return 0;
	}

	_buffer = charbuf_new(0);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "vkeyb");
	dev.read = vkeyb_read;
	dev.cmd = vkeyb_cmd;
	dev.loop_step = vkeyb_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);

	charbuf_free(_buffer);
	close(_keyb_fd);
	return 0;
}
