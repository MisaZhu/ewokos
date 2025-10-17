#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/mmio.h>
#include <arch/virt/virtio.h>
#include <mouse/mouse.h>

#define EV_KEY 0x01 // 按键事件
#define EV_REL 0x02 // 相对坐标事件（如鼠标移动）
#define EV_ABS 0x03 // 绝对坐标事件（如触摸屏）
#define EV_SYN 0x00 // 同步事件

#define ABS_X 0x00 // 绝对X坐标
#define ABS_Y 0x01 // 绝对Y坐标

#define BTN_LEFT 0x110	// 左键
#define BTN_RIGHT 0x111 // 右键


#define CACHE_SIZE (32)
static mouse_evt_t mouse_data[CACHE_SIZE];
static uint32_t mouse_data_read = 0;
static uint32_t mouse_data_write = 0;

static int _read(int fd, int from_pid, fsinfo_t *node,
					  void *buf, int size, int offset, void *p)
{
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;

	uint8_t *d = (uint8_t *)buf;
	if (mouse_data_write - mouse_data_read > 0)
	{
		memcpy(buf, &mouse_data[mouse_data_read % CACHE_SIZE], sizeof(mouse_evt_t));
		memset(&mouse_data[mouse_data_read % CACHE_SIZE], 0, sizeof(mouse_evt_t));
		mouse_data_read++;
		return sizeof(mouse_evt_t);
	}

	return VFS_ERR_RETRY;
}

struct virtio_input_event
{
	uint16_t type;
	uint16_t code;
	uint32_t value;
} __attribute__((packed));

void mouse_interrupt_handle(struct virtio_device *dev, struct virtio_input_event *event)
{
	(void)dev;
	if (event->type == EV_REL)
	{
		mouse_data[mouse_data_write % CACHE_SIZE].type = 1;
		if (event->code == ABS_X)
		{
			mouse_data[mouse_data_write % CACHE_SIZE].x = event->value;
		}
		else if (event->code == ABS_Y)
		{
			mouse_data[mouse_data_write % CACHE_SIZE].y = event->value;
		}
	}else if(event->type == EV_ABS){
		mouse_data[mouse_data_write % CACHE_SIZE].type = 2;
		if(event->code == ABS_X){
			mouse_data[mouse_data_write % CACHE_SIZE].x = event->value;
		}else if(event->code == ABS_Y){
			mouse_data[mouse_data_write % CACHE_SIZE].y = event->value;
		}
	}
	else if (event->type == EV_KEY)
	{
		if (event->code == BTN_LEFT)
		{
				mouse_data[mouse_data_write % CACHE_SIZE].button = MOUSE_BUTTON_LEFT;
			if (event->value){
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_DOWN;
			}else{
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_UP;
			}
		}
		else if (event->code == BTN_RIGHT)
		{
			mouse_data[mouse_data_write % CACHE_SIZE].button = MOUSE_BUTTON_RIGHT;
			if (event->value){
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_DOWN;
			}else{
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_UP;
			}
		}
	}
	else if (event->type == EV_SYN)
	{
		mouse_data_write++;
		proc_wakeup(RW_BLOCK_EVT);
	}
}

int main(int argc, char **argv)
{
	const char *mnt_point = argc > 1 ? argv[1] : "/dev/mouse0";
	_mmio_base = mmio_map();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "mouse");
	dev.read = _read;

	virtio_dev_t vio = virtio_input_get("QEMU Virtio Tablet");
	if (!vio || virtio_init(vio, 0) != 0)
	{
		klog("Virtio-input init failed\n");
		return -1;
	}
	virtio_interrupt_enable(vio, mouse_interrupt_handle);

	dev.extra_data = (void *)vio;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
