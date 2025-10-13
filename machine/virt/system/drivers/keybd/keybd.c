#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/mmio.h>
#include <arch/virt/virtio.h>
#include "keymap.h"

#define EV_KEY          0x01  // 按键事件
#define EV_REL          0x02  // 相对坐标事件（如鼠标移动）
#define EV_ABS          0x03  // 绝对坐标事件（如触摸屏）
#define EV_SYN          0x00  // 同步事件

#define MAX_KEY (7)

typedef struct {
	uint8_t key_count;
	uint8_t key_code[7];
} key_data_t;

static key_data_t key_data;

void key_press(uint8_t key_code){
	int i;
	for(i = 0; i < key_data.key_count; i++){
		if(key_data.key_code[i] == key_code){
			return;
		}
	}
	if(i < MAX_KEY){
		key_data.key_code[key_data.key_count++] = key_code;
	}
}

void key_release(uint8_t key_code){
	for(int i = 0; i < key_data.key_count; i++){
		if(key_data.key_code[i] == key_code){
			for(int j = i; j < key_data.key_count - 1; j++){
				key_data.key_code[j] = key_data.key_code[j+1];
			}
			key_data.key_count--;
			return;
		}
	}
}

int get_key_code(char* buf){
	int shift = 0;
	int num = 0;
	for(int i = 0; i < key_data.key_count; i++){
		if(key_data.key_code[i] < sizeof(keymap)){
			if(keymap[key_data.key_code[i]] == 0x10){
				shift = 1;
			}else if(keymap[key_data.key_code[i]]){
				buf[num++] = keymap[key_data.key_code[i]];
			}
		}
	}
	if(!shift){
		for(int i = 0; i < num; i++){
			if(buf[i] >= 'A' && buf[i] <= 'Z'){
				buf[i] += 0x20;
			}
		}
	}
	return num;
}

static int keybd_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;
	
	int num = get_key_code(buf);
	return num?num:VFS_ERR_RETRY;
}

struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} __attribute__((packed));

static int keybd_loop(void* p){
	virtio_dev_t vio = (virtio_dev_t)p;
	struct virtio_input_event events[32];
	int res = virtio_input_read(vio, events, sizeof(events));
	for(int i = 0; i < res/sizeof(struct virtio_input_event); i++){
		//klog("type: %d, code: %d, value: %d\n", events[i].type, events[i].code, events[i].value);
		if(events[i].type == EV_KEY){
			if(events[i].value){
				key_press(events[i].code);
			}else{
				key_release(events[i].code);
			}	
		}else if(events[i].type == EV_SYN){
			proc_wakeup(RW_BLOCK_EVT);
		}
	}
	proc_usleep(10);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/keyboard0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "keyboard");
	dev.read = keybd_read;
	dev.loop_step = keybd_loop;

	_mmio_base = mmio_map();

	virtio_dev_t vio = virtio_input_get("QEMU Virtio Keyboard");
	if (!vio || virtio_init(vio, 0) != 0) {
        klog("Virtio-input init failed\n");
        return -1;
    }

	dev.extra_data = (void*)vio;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
