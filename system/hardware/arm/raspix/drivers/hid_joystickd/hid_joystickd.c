#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/mmio.h>
#include <fcntl.h>
#include <ewoksys/keydef.h>

static int hid;
static uint8_t key_state;
static uint8_t last_state;

static int joystick_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;

	if(last_state != key_state){
		((uint8_t*)buf)[0] = key_state;
		last_state = key_state;
		return 1;
	}
	return VFS_ERR_RETRY;
}

typedef struct {
   uint32_t r : 4;
   uint32_t x : 10;
   uint32_t y : 10;
   uint32_t n : 8;
}axis_t;

typedef struct {
	uint8_t button[2];
	uint8_t resv[2];
	axis_t axis;
}joystick_t;

static int loop(void* p) {
	(void)p;

	uint8_t buf[8];
	if(read(hid, buf, 7) == 7){
		//klog("joy: %02x %02x %02x %02x %02x %02x %02x %02x\n", 
		//buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
		joystick_t *joy = (joystick_t*)&buf;
		key_state = 0;
		//klog("%d %d", joy->axis.x, joy->axis.y);
		if(joy->axis.x > 768)
			key_state|=JOYSTICK_RIGHT;
		else if(joy->axis.x < 256)
			key_state|=JOYSTICK_LEFT;	
		
		if(joy->axis.y > 768)
			key_state|=JOYSTICK_DOWN;
		else if(joy->axis.y < 256)
			key_state|=JOYSTICK_UP;	
		
		if(joy->button[0] & 0x2)
			key_state |= JOYSTICK_BTN_A;
		if(joy->button[0] & 0x4)
			key_state |= JOYSTICK_BTN_B;
		if(joy->button[1] & 0x1)
			key_state |= JOYSTICK_BTN_SELECT;
		if(joy->button[1] & 0x2)
			key_state |= JOYSTICK_BTN_START;

		proc_wakeup(RW_BLOCK_EVT);
	}
	else {
		usleep(5000);
	}
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
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joystick0";
	const char* dev_point = argc > 2 ? argv[2]: "/dev/hid0";

	hid = open(dev_point, O_RDONLY | O_NONBLOCK);
	set_report_id(hid, 0x14);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joystick");
	dev.loop_step = loop;
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
