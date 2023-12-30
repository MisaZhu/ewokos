#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/mmio.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/timer.h>
#include <fcntl.h>
#include <ewoksys/keydef.h>

#define KEY_MOD_LCTRL  0x01
#define KEY_MOD_LSHIFT 0x02
#define KEY_MOD_LALT   0x04
#define KEY_MOD_LMETA  0x08
#define KEY_MOD_RCTRL  0x10
#define KEY_MOD_RSHIFT 0x20
#define KEY_MOD_RALT   0x40
#define KEY_MOD_RMETA  0x80

static uint8_t _held[128] = {0};
static bool _idle = true;

static int  hid;
static char key[3];

static int keyb_read(int fd, int from_pid, uint32_t node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)size;
	(void)node;

	memcpy(buf, key, 3);
	return 3;
}

const char downMap[] = {  
        ' ',' ',' ',' ','a','b','c','d',    'e','f','g','h','i','j','k','l',
        'm','n','o','p','q','r','s','t',    'u','v','w','x','y','z','1','2',
        '3','4','5','6','7','8','9','0',    '\r','\x1b','\b','\t','\x20', '-', '=', '[', 
        ']', '\\', '$', ';', '\'', '`',',','.',     '/',
    };

const char upMap[] = {
        ' ',' ',' ',' ','A','B','C','D',    'E','F','G','H','I','J','K','L',
        'M','N','O','P','Q','R','S','T',    'U','V','W','X','Y','Z','!','@',
        '#','$','%','^','&','*','(',')',    '\r','\0x1b','\b','\t','\x20', '_', '+', '{', 
        '}', '|', '$', ':', '\"', '~','<','>',      '?',
};


uint8_t getKeyChar(uint8_t alt, uint8_t keycode){
    if(keycode > 0 && keycode < sizeof(upMap)){
        if((alt & KEY_MOD_LSHIFT) ||(alt & KEY_MOD_RSHIFT)){
            return upMap[keycode];
        }else{
        	return downMap[keycode];
        }
    }else if(keycode == 0x4f){
		return KEY_RIGHT;
	}else if(keycode == 0x50){
		return KEY_LEFT;
	}else if(keycode == 0x51){
		return KEY_DOWN;
	}else if(keycode == 0x52){
		return KEY_UP;
	}
    return 0;
}

static int loop(void* p) {
	(void)p;
	int8_t buf[8];
	if(read(hid, buf, 7) == 7){
		//klog("kb: %02x %02x %02x %02x %02x %02x %02x %02x\n", 
		//buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
		key[0] = getKeyChar(buf[0], buf[2]);
		key[1] = getKeyChar(buf[1], buf[3]);
		key[2] = getKeyChar(buf[2], buf[4]);
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
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/keyb0";
	const char* dev_point = argc > 2 ? argv[2]: "/dev/hid0";
	hid = open(dev_point, O_RDONLY);
	set_report_id(hid, 2);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "keyb");
	dev.read = keyb_read;
	dev.loop_step = loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
