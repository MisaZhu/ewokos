#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <ewoksys/keydef.h>

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
	
	char* v = (char*)buf;
	int rd = -1;
	if(size == 4 && _mouse_mode) //read mouse 
		rd = read(_joys_fd, v, 4);
	else if(size == 6 && !_mouse_mode) // read joystick 
		rd = read(_joys_fd, v, 6);


	if(rd > 0) {
		for(int i=0; i<rd; i++) {
			if(v[i] == KEY_HOME) {
				_mouse_mode = !_mouse_mode;
				rd = -1;
				break;
			}
		}
	}

	return rd;	
}

int main(int argc, char** argv) {
	_mouse_mode = false;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/vjoystick";
	const char* joys_dev = argc > 2 ? argv[2]: "/dev/joystick";

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
