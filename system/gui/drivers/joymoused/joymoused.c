#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>

typedef struct {
	int8_t btn;
	int8_t rx;
	int8_t ry;
	int8_t rz;
} mouse_info_t;

#define MAX_MEVT 64
static mouse_info_t _minfo[MAX_MEVT];
static uint32_t _minfo_num = 0;
static uint32_t _minfo_index = 0;

#define JOY_STEP         3 
static bool _prs_down = false;
static uint32_t _j_speed_up = 0;

static int  _joys_fd = -1;

static void joy_2_mouse(int key, int8_t* mv) {
	uint32_t j_times = 1;
	if(_j_speed_up > 8) {
		/*j_times = _j_speed_up/6;
		if(j_times > 16)
			j_times = 16;
			*/
		j_times = 4;
	}
		
	mv[0] = mv[1] = mv[2] = 0;
	switch(key) {
	case KEY_UP:
		mv[2] -= (JOY_STEP) * j_times;
		return;
	case KEY_DOWN:
		mv[2] += (JOY_STEP) * j_times;
		return;
	case KEY_LEFT:
		mv[1] -= (JOY_STEP) * j_times;
		return;
	case KEY_RIGHT:
		mv[1] += (JOY_STEP) * j_times;
		return;
	case KEY_BUTTON_A:
	case KEY_ENTER:
	case KEY_BUTTON_START:
		//if(!_prs_down) {
			mv[0] = 2;
			_prs_down = true;
		//}
		return;
	}	
}

static void input(char key) {
	int8_t mv[4];
	joy_2_mouse(key, mv);
	/*if(mv[1] != 0 || mv[2] != 0)
		_j_speed_up++;
	else
		*/
		_j_speed_up = 0;

	if(key == 0 && _prs_down) {
		key = 1;
		_prs_down = false;
		mv[0] = 1;
	}
	if(key != 0) {
		//read new event
		mouse_info_t info;
		info.btn = mv[0];
		info.rx = mv[1];
		info.ry = mv[2];
		info.rz = 0;

		if((_minfo_num+1) >= MAX_MEVT) {
			if(info.btn != 0)
				memcpy(&_minfo[MAX_MEVT-1], &info, sizeof(mouse_info_t));
		}
		else {
			memcpy(&_minfo[_minfo_num], &info, sizeof(mouse_info_t));
			_minfo_num++;
		}
	}
}

static int joymouse_read_buffer(uint8_t* buf) {
	buf[0] = 0;
	if(_minfo_index < _minfo_num) {
		buf[0] = 1;
		buf[1] = _minfo[_minfo_index].btn;
		buf[2] = _minfo[_minfo_index].rx;
		buf[3] = _minfo[_minfo_index].ry;

		_minfo_index++;
		if(_minfo_index >= _minfo_num) {
			_minfo_num = _minfo_index = 0;
		}
		return 4;
	}
	return 0;
}

static int joymouse_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;

	if(_joys_fd < 0 || size != 4)
		return -1;

	//read from buffer
	int rd = joymouse_read_buffer((uint8_t*)buf);
	if(rd > 0)
		return rd;

	//read from device
	char v[4] = {0};
 	rd = read(_joys_fd, v, 4 | 0x1000);
	if(rd > 0) {
		for(int i=0;i < rd; i++)
			input(v[i]);
	}
	else
		input(0);
	return joymouse_read_buffer(buf);
}

int main(int argc, char** argv) {
	_prs_down = false;
	_j_speed_up = 0;
	_minfo_num = 0;
	_minfo_index = 0;
	_joys_fd = -1;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/mouse0";
	const char* joys_dev = argc > 2 ? argv[2]: "/dev/vjoystick";

	_joys_fd = open(joys_dev, O_RDONLY | O_NONBLOCK);
	if(_joys_fd < 0) {
		return -1;
	}


	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joymouse");
	dev.read = joymouse_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);

	close(_joys_fd);
	return 0;
}
