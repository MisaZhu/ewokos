#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/core.h>
#include <ewoksys/proto.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>
#include <x/xcntl.h>

static int  _keyb_fd = -1;
static uint32_t _fps = 60;

#define KEY_NUM 4
static uint8_t _keys[KEY_NUM];
static int32_t _rd = 0;
static bool _release = false;

static int vkeyb_read(int fd,
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

	if(_keyb_fd < 0)
		return -1;

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
	int ret = _rd;	
	_rd = 0;
	return ret;
}

static int x_show_cursor(bool show) {
	proto_t in;
	PF->init(&in)->addi(&in, (int32_t)show);

	int res = dev_cntl("/dev/x", X_DCNTL_SHOW_CURSOR, &in, NULL);
	PF->clear(&in);
	return res;
}

static int x_next_focus(void) {
	int res = dev_cntl("/dev/x", X_DCNTL_NEXT_FOCUS, NULL, NULL);
	return res;
}

static int x_launcher(void) {
	int res = dev_cntl("/dev/x", X_DCNTL_LAUNCHER, NULL, NULL);
	return res;
}

static int x_close_focus(void) {
	int res = dev_cntl("/dev/x", X_DCNTL_CLOSE_FOCUS, NULL, NULL);
	return res;
}

static int ctrl_down(uint8_t* keys, uint8_t num) {
	for(int i=0; i<num; i++) {
		if(_keys[i] == KEY_CTRL)
			return i;
	}
	return -1;
}

static bool do_keyb_spec(uint8_t* keys, uint8_t num) {
	int i = ctrl_down(keys, num);
	for(int j=0; j<num; j++) {
		uint8_t c = keys[j];
		if(i >= 0) {
			if(c >= '0' && c <= '9') {
				core_set_active_ux(0, c - '0');
				return true;
			}
			else if(c == KEY_LEFT) { //left 
				core_prev_ux(0);
				return true;
			}
			else if(c == KEY_RIGHT) { //right
				core_next_ux(0);
				return true;
			}
			else if(c == KEY_TAB) { //tab for focus
				x_next_focus();
				return true;
			}
			else if(c == 'h') { //h for launcher 
				x_launcher();
				return true;
			}
			else if(c == 'e') { //e for close
				x_close_focus();
				return true;
			}
		}
		else if(c == KEY_HOME) {
			x_launcher();
			return true;
		}
		else if(c == KEY_END) {
			x_close_focus();
			return true;
		}
	}
	return false;
}

static int sel_down(uint8_t* keys, uint8_t num) {
	for(int i=0; i<num; i++) {
		if(_keys[i] == JOYSTICK_SELECT)
			return i;
	}
	return -1;
}

static bool do_joys_spec(uint8_t* keys, uint8_t num, uint8_t* ret_key) {
	int i = sel_down(keys, num);
	for(int j=0; j<num; j++) {
		uint8_t c = keys[j];
		if(i >= 0) {
			if(j == i)
				continue;
			if(c == JOYSTICK_Y) { //Y for prev ux
				core_prev_ux(0);
				return true;
			}
			else if(c == JOYSTICK_A) { //A for next ux
				*ret_key = JOYSTICK_START;
				return true;
			}
			else if(c == JOYSTICK_X) { //X for next focus
				x_next_focus();
				return true;
			}
			else if(c == JOYSTICK_B) { 
				*ret_key = JOYSTICK_R1;
				return true;
			}
			else if(c == KEY_HOME || c == JOYSTICK_START) { 
				x_close_focus();
				return true;
			}
		}
		else if(c == KEY_HOME) {
			x_launcher();
			return true;
		}
		else if(c == KEY_END) {
			x_close_focus();
			return true;
		}
	}
	return false;
}

static uint8_t _keyb_type = 'k';

static uint32_t ct = 0;
static int vkeyb_loop(void* p){
	uint64_t tik = kernel_tic_ms(0);
	uint32_t tm = 1000/_fps;
	uint8_t keys[KEY_NUM] = {0};

	ipc_disable();

	int32_t rd = read(_keyb_fd, keys, KEY_NUM);
	for(int i=0; i<KEY_NUM; i++) {
		uint32_t c = _keys[i];
		if(c == 0)
			continue;

		_release = true;
		for(int j=0; j<rd; j++) {
			if(c == keys[j]) {
				_release = false;
				break;
			}
		}

		if(_release)
			break;
	}

	if(_release) {
		if(_keyb_type == 'k') {
			if(do_keyb_spec(_keys, KEY_NUM)) {
				rd = 0;
			}
		}
		else if(_keyb_type == 'j') {
			uint8_t ret_key = 0;
			if(do_joys_spec(_keys, KEY_NUM, &ret_key)) {
				rd = 0;
				if(ret_key > 0) {
					rd = 1;
					keys[0] = ret_key;
				}
			}
		}
	}

	memset(_keys, 0, KEY_NUM);
	if(rd > 0) {
		memcpy(_keys, keys, rd);
		if(_keyb_type == 'k') {
			if(ctrl_down(_keys, KEY_NUM) >= 0)
				rd = 0;
		}
		else if(_keyb_type == 'j') {
			if(sel_down(_keys, KEY_NUM) >= 0) {
				rd = 0;
			}
		}
	}
	_rd = rd;

	if(_rd > 0 || _release) {
		proc_wakeup(RW_BLOCK_EVT);
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
		c = getopt (argc, argv, "f:t:");
		if(c == -1)
			break;

		switch (c) {
		case 'f':
			_fps = atoi(optarg);
			break;
		case 't':
			_keyb_type = optarg[0];
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_fps = 60;
	_keyb_type = 'k';
	int32_t argind =  doargs(argc, argv);
	const char* mnt_point = "/dev/vkeyb";
	const char* keyb_dev = "/dev/keyb0";
	if(argind < argc) {
		mnt_point = argv[argind];
		argind++;
	}

	if(argind < argc) {
		keyb_dev = argv[argind];
	}

	_keyb_fd = open(keyb_dev, O_RDONLY | O_NONBLOCK);
	if(_keyb_fd < 0) {
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "vkeybtick");
	dev.read = vkeyb_read;
	dev.loop_step = vkeyb_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);

	close(_keyb_fd);
	return 0;
}
