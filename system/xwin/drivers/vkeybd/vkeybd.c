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

static int ctrl_down(uint8_t* keys, uint8_t num) {
	for(int i=0; i<num; i++) {
		if(_keys[i] == KEY_CTRL)
			return i;
	}
	return -1;
}

static bool do_ctrl(uint8_t* keys, uint8_t num) {
	int i = ctrl_down(keys, num);
	if(i < 0)
		return false;

	for(int j=0; j<num; j++) {
		if(j == i)
			continue;
		uint8_t c = keys[j];
		if(c >= '0' && c <= '9') {
			core_set_active_ux(c - '0');
			return true;
		}
		else if(c == 19) { //left 
			core_prev_ux();
			return true;
		}
		else if(c == 4) { //right
			core_next_ux();
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

static bool do_joys(uint8_t* keys, uint8_t num) {
	int i = sel_down(keys, num);
	if(i < 0)
		return false;

	for(int j=0; j<num; j++) {
		if(j == i)
			continue;
		uint8_t c = keys[j];
		if(c == JOYSTICK_Y) { //left 
			core_prev_ux();
			return true;
		}
		else if(c == JOYSTICK_A) { //right
			core_next_ux();
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
	if(rd <= 0) {
		for(int i=0; i<KEY_NUM; i++) {
			if(_keys[i] != 0) {
				_release = true;
				break;
			}
		}
		_rd = 0;
		memset(_keys, 0, KEY_NUM);
	}
	else {
		memcpy(_keys, keys, rd);
		if(_keyb_type == 'k') {
			if(do_ctrl(_keys, KEY_NUM))
				rd = 0;
		}
		else if(_keyb_type == 'j') {
			if(do_joys(_keys, KEY_NUM))
				rd = 0;
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
