#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/mmio.h>
#include <arch/virt/virtio.h>

#define EV_KEY          0x01  // 按键事件
#define EV_REL          0x02  // 相对坐标事件（如鼠标移动）
#define EV_ABS          0x03  // 绝对坐标事件（如触摸屏）
#define EV_SYN          0x00  // 同步事件

#define ABS_X           0x00  // 绝对X坐标
#define ABS_Y           0x01  // 绝对Y坐标

#define BTN_LEFT        0x110  // 左键
#define BTN_RIGHT       0x111  // 右键

typedef struct {
	uint8_t btn;
	int rx;
	int ry;
} mouse_data_t;

#define CACHE_SIZE (32)
static mouse_data_t mouse_data[CACHE_SIZE];
static uint32_t mouse_data_read = 0;
static uint32_t mouse_data_write = 0;

void mouse_init(void){

}

static int mouse_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;
	
	uint8_t* d = (uint8_t*)buf;
	if(mouse_data_write - mouse_data_read > 0){
		d[0] = 1;
		d[1] = mouse_data[mouse_data_read%CACHE_SIZE].btn;
		d[2] = mouse_data[mouse_data_read%CACHE_SIZE].rx;
		d[3] = mouse_data[mouse_data_read%CACHE_SIZE].ry;
		memset(&mouse_data[mouse_data_read%CACHE_SIZE], 0, sizeof(mouse_data_t));
		mouse_data_read++;
		return 4;
	}

	return VFS_ERR_RETRY;
}

struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} __attribute__((packed));

static int mouse_loop(void* p){
	virtio_dev_t vio = (virtio_dev_t)p;
	struct virtio_input_event events[32];
	int res = virtio_input_read(vio, events, sizeof(events));
	for(int i = 0; i < res/sizeof(struct virtio_input_event); i++){
		if(events[i].type == EV_REL){
			if(events[i].code == ABS_X){
				mouse_data[mouse_data_write%CACHE_SIZE].rx = events[i].value;
			}
			else if(events[i].code == ABS_Y){
				mouse_data[mouse_data_write%CACHE_SIZE].ry = events[i].value;
			}
		}else if(events[i].type == EV_KEY){
			if(events[i].code == BTN_LEFT){
				if(events[i].value)
					mouse_data[mouse_data_write%CACHE_SIZE].btn = 2;
				else
					mouse_data[mouse_data_write%CACHE_SIZE].btn = 1;
			}
			else if(events[i].code == BTN_RIGHT){
				if(events[i].value)
					mouse_data[mouse_data_write%CACHE_SIZE].btn = 3;
				else
					mouse_data[mouse_data_write%CACHE_SIZE].btn = 1;
			}
		}else if(events[i].type == EV_SYN){
			mouse_data_write++;
			proc_wakeup(RW_BLOCK_EVT);
		}
	}
	proc_usleep(10);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/mouse0";

	mouse_init();
	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "mouse");
	dev.read = mouse_read;
	dev.loop_step = mouse_loop;


	_mmio_base = mmio_map();

	virtio_dev_t vio = virtio_get(VIRTIO_ID_INPUT);
	if (!vio || virtio_init(vio, 0) != 0) {
        klog("Virtio-input init failed\n");
        return -1;
    }

	dev.extra_data = (void*)vio;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
