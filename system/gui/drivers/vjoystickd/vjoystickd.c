#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>

static int  _joys_fd = -1;
static bool _mouse_mode = false;

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

	if(_joys_fd < 0 || size < 4)
		return -1;
	static uint64_t home_tic = 0u;
	
	char* v = (char*)buf;
	int rd = -1;
	if((size & 0x1000) != 0) {
		if(_mouse_mode)
			rd = read(_joys_fd, v, size & 0x0fff);
	}
	else if(!_mouse_mode)
		rd = read(_joys_fd, v, size);

	if(rd > 0) {
		for(int i=0; i<rd; i++) {
			if(v[i] == KEY_BUTTON_SELECT) {
				uint64_t now;
				now = kernel_tic_ms(0);
				if((now - home_tic) > 200)
					_mouse_mode = !_mouse_mode;
				rd = -1;
				home_tic = now;
				break;
			}
		}
	}

	return rd;	
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
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_mouse_mode = false;
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

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);

	close(_joys_fd);
	return 0;
}
