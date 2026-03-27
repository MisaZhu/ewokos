#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/mmio.h>
#include <mouse/mouse.h>
#include <fcntl.h>

static int hid;

static uint8_t btn;
static uint8_t last_btn;
static uint8_t x;
static uint8_t y;
static uint8_t has_data = 0;

static int _read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;

	if(size < 4)
		return -1;
	
	uint8_t* d = (uint8_t*)buf;
	if(!has_data){
		return VFS_ERR_RETRY;
	}

	mouse_evt_t *evt = (mouse_evt_t *)buf;
	evt->type = MOUSE_TYPE_REL;
	if(btn){
		evt->state = MOUSE_STATE_DOWN;
		evt->button = btn;
	}else if(last_btn){
		evt->state = MOUSE_STATE_UP;
		evt->button = last_btn;
	}else{
		evt->button = MOUSE_BUTTON_NONE;
		evt->state = MOUSE_STATE_MOVE;
	}

	evt->x = x;
	evt->y = y;
	has_data = 0;
	return sizeof(mouse_evt_t);
}

static int _loop(void* p) {
	(void)p;

	ipc_disable();

	bool wakeup = false;
	while(true) {
		int8_t buf[8] = {0};
		int res = read(hid, buf, 7);
		if(res == 7) {
			btn = buf[0];
			x = buf[1];
			y = buf[2];
			has_data = 1;
			wakeup = true;
			usleep(0);
		}
		else {
			break;
		}
	}

	ipc_enable();
	if(wakeup)
		proc_wakeup(RW_BLOCK_EVT);
	else
		usleep(3000);
	return 0;
}

static int set_report_id(int fd, int id) {

	proto_t in;
	PF->init(&in)->addi(&in, id);
	int ret = vfs_fcntl(fd, 0, &in , NULL);
	PF->clear(&in);
	return ret;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/mouse0";
	const char* dev_point = argc > 2 ? argv[2]: "/dev/hid0";
	hid = open(dev_point, O_RDONLY | O_NONBLOCK);
	set_report_id(hid, 1);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "mouse");
	dev.loop_step = _loop;
	dev.read = _read;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
