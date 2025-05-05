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

typedef struct {
	int8_t btn;
	int8_t rx;
	int8_t ry;
	int8_t rz;
} mouse_info_t;

#define KEY_NUM 4
static uint8_t _keys[KEY_NUM];
static int32_t _rd = 0;
static bool _release = false;

#define MAX_MEVT 64
static mouse_info_t _minfo[MAX_MEVT];
static uint32_t _minfo_num = 0;
static uint32_t _minfo_index = 0;

#define JOY_STEP         3 
static bool _prs_down = false;
static bool _move = false;
static uint32_t _j_speed_up = 0;

static void joy_2_mouse(int key, int8_t* mv) {
	uint32_t j_times = 1;
	if(_j_speed_up > 8) {
		j_times = _j_speed_up/6;
		if(j_times > 16)
			j_times = 16;
		//j_times = 4;
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
	case JOYSTICK_A:
	case KEY_ENTER:
	case JOYSTICK_START:
		//if(!_prs_down) {
			mv[0] = 2;
			_prs_down = true;
		//}
		return;
	}	
}

static uint32_t mouse_input(char key) {
	int8_t mv[4];
	joy_2_mouse(key, mv);
	if(mv[1] != 0 || mv[2] != 0)
		_move = true;

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
		return 4;
	}
	return 0;
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
	if(size == 4) { //4 bytes for reading mouse data , 6 bytes for keyb
		mouse = true;
	}

	if(_joys_fd < 0 || size < KEY_NUM)
		return -1;

	if(mouse) {
		if(joymouse_read_buffer((uint8_t*)buf) == 0)
			//return 0;
			return VFS_ERR_RETRY;
		return 4;
	}

	//key mode
	if(_rd > 0)
		memcpy(buf, _keys, _rd);
	else {
		if(!_release)
			//return 0;
			return VFS_ERR_RETRY;
		else
			_release = false;
	}
	return _rd;	
}

static int vjoy_loop(void* p){
	uint64_t tik = kernel_tic_ms(0);
	uint32_t tm = 1000/_fps;
	uint8_t keys[KEY_NUM] = {0};

	ipc_disable();

	int32_t rd = read(_joys_fd, keys, KEY_NUM);
	if(rd <= 0) {
		for(int i=0; i<KEY_NUM; i++) {
			if(_keys[i] != 0) {
				_release = true;
				if(_keys[i] == JOYSTICK_SELECT) {
					_mouse_mode = !_mouse_mode;
					_release = false;
					break;
				}
			}
		}
		memset(_keys, 0, KEY_NUM);
	}
	else {
		memcpy(_keys, keys, rd);
	}

	if(_mouse_mode) {
		_move = false;
		if(rd <= 0)
			rd = mouse_input(0);
		else {
			for(int i=0; i < rd; i++)
				mouse_input(keys[i]);
			rd = 4;
		}

		if(_move)
			_j_speed_up++;
		else
			_j_speed_up = 0;

		if(rd > 0)
			proc_wakeup(RW_BLOCK_EVT);
		_rd = 0;
	}
	else {
		_rd = rd;
		if(_rd > 0 || _release) {
			proc_wakeup(RW_BLOCK_EVT);
		}
	}
	ipc_enable();

	uint32_t gap = (uint32_t)(kernel_tic_ms(0) - tik);
	if(gap < tm) {
		gap = tm - gap;
		proc_usleep(gap*1000);
	}
	return 0;
}

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "mkf:");
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
