#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/proto.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>

static int  _joys_fd = -1;
static bool _mouse_mode = false;
static uint32_t _fps = 60;

#define KEY_NUM 4
static uint8_t _keys[KEY_NUM];
static int _rd = 0;

static int vjoystick_read(int fd,
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

	bool mouse = false;
	if((size & 0x1000) != 0) {
		mouse = true;
		size = size & 0x0fff;
	}

	if(_joys_fd < 0 || size < KEY_NUM)
		return -1;
	
	char* v = (char*)buf;
	memcpy(v, _keys, KEY_NUM);

	if(mouse) {
		if(!_mouse_mode)
			return VFS_ERR_RETRY;
	}
	else if(_mouse_mode)
		return VFS_ERR_RETRY;

	return _rd;	
}

static int vjoy_loop(void* p){
	uint64_t tik = kernel_tic_ms(0);
	uint32_t tm = 1000/_fps;

	_rd = read(_joys_fd, _keys, KEY_NUM);
	if(_rd > 0) {
		for(int i=0; i<_rd; i++) {
			if(_keys[i] == KEY_BUTTON_SELECT) {
				_mouse_mode = !_mouse_mode;
				proc_wakeup(RW_BLOCK_EVT);
				break;
			}
		}
	}

	uint32_t gap = (uint32_t)(kernel_tic_ms(0) - tik);
	if(gap < tm) {
		gap = tm - gap;
		proc_usleep(gap*1000);
	}
}

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "mk");
		if(c == -1)
			break;

		switch (c) {
		case 'm':
			_mouse_mode = true;
			break;
		case 'k':
			_mouse_mode = false;
			break;
		case 'f':
			_fps = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_mouse_mode = false;
	_fps = 60;
	int32_t argind =  doargs(argc, argv);
	const char* mnt_point = "/dev/vjoystick";
	const char* joys_dev = "/dev/joystick";
	if(argind < argc) {
		mnt_point = argv[argind];
		argind++;
	}

	if(argind < argc) {
		joys_dev = argv[argind];
	}


	_joys_fd = open(joys_dev, O_RDONLY | O_NONBLOCK);
	if(_joys_fd < 0) {
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "vjoystick");
	dev.read = vjoystick_read;
	dev.loop_step = vjoy_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);

	close(_joys_fd);
	return 0;
}
