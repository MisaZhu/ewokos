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

#define EV_KEY 0x01 // 按键事件
#define EV_REL 0x02 // 相对坐标事件（如鼠标移动）
#define EV_ABS 0x03 // 绝对坐标事件（如触摸屏）
#define EV_SYN 0x00 // 同步事件

#define MAX_KEY (6)

typedef struct
{
	uint8_t key_count;
	uint8_t shift;
	uint8_t key_code[6];
} key_data_t;

static key_data_t key_data;

void key_press(uint8_t key_code)
{
	for (int i = 0; i < key_data.key_count; i++)
	{
		if (key_data.key_code[i] == key_code)
		{
			return;
		}
	}
	if (key_data.key_count < MAX_KEY)
	{
		key_data.key_code[key_data.key_count++] = key_code;
	}
}

void key_release(uint8_t key_code)
{
	for (int i = 0; i < key_data.key_count; i++)
	{
		if (key_data.key_code[i] == key_code)
		{
			for (int j = i; j < key_data.key_count - 1; j++)
			{
				key_data.key_code[j] = key_data.key_code[j + 1];
			}
			key_data.key_count--;
			return;
		}
	}
}

int get_key_code(char *buf)
{
	int num = 0;
	for (int i = 0; i < key_data.key_count; i++)
	{
		if (key_data.key_code[i] < sizeof(keymapUp))
		{
			if (key_data.shift)
			{
				buf[num] = keymapUp[key_data.key_code[i]];
			}
			else
			{
				buf[num] = keymapDown[key_data.key_code[i]];
			}
			if (buf[num])
				num++;
		}
	}
	return num;
}

static int keybd_read(int fd, int from_pid, fsinfo_t *node,
					  void *buf, int size, int offset, void *p)
{
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;

	int num = get_key_code(buf);
	return num ? num : VFS_ERR_RETRY;
}

struct virtio_input_event
{
	uint16_t type;
	uint16_t code;
	uint32_t value;
} __attribute__((packed));

void keybd_interrupt_handle(virtio_dev_t dev, struct virtio_input_event *event)
{
	(void)dev;
	if (event->type == EV_KEY)
	{
		if (event->value)
		{
			key_press(event->code);
		}
		else
		{
			key_release(event->code);
		}
		if (event->code == 0x2a || event->code == 0x36)
		{
			key_data.shift = event->value;
		}
	}
	else if (event->type == EV_SYN)
	{
		proc_wakeup(RW_BLOCK_EVT);
	}
}

int main(int argc, char **argv)
{
	const char *mnt_point = argc > 1 ? argv[1] : "/dev/keyboard0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "keyboard");
	dev.read = keybd_read;
	// dev.loop_step = keybd_loop;

	_mmio_base = mmio_map();

	virtio_dev_t vio = virtio_input_get("QEMU Virtio Keyboard");
	if (!vio || virtio_init(vio, 0) != 0)
	{
		klog("Virtio-input init failed\n");
		return -1;
	}
	virtio_interrupt_enable(vio, keybd_interrupt_handle);

	dev.extra_data = (void *)vio;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
