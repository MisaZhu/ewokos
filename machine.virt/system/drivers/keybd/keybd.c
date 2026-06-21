#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <arch/virt/virtio.h>
#include "keymap.h"

#define EV_KEY 0x01 // Key event
#define EV_REL 0x02 // Relative coordinate event, such as mouse movement
#define EV_ABS 0x03 // Absolute coordinate event, such as touchscreen input
#define EV_SYN 0x00 // Synchronization event

#define MAX_KEY (6)

typedef struct
{
	uint8_t key_count;
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
	
	// Process all currently pressed keys.
	for (int i = 0; i < key_data.key_count; i++)
	{
		uint8_t key = key_data.key_code[i];
		// If the key is not mapped in keymap, fall back to the raw key code.
		if (key < sizeof(keymap) && keymap[key] != 0)
		{
			buf[num++] = keymap[key];
		}
		else
		{
			buf[num++] = key;
		}
	}
	return num;
}

static int keybd_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t *info,
					  void *buf, int size, int offset, void *p)
{
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)info;

	int num = get_key_code(buf);
	return num ? num : VFS_ERR_RETRY;
}

static uint32_t keybd_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p)
{
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;

	return key_data.key_count > 0 ? VFS_EVT_RD : 0;
}

struct virtio_input_event
{
	uint16_t type;
	uint16_t code;
	uint32_t value;
} __attribute__((packed));

static vdevice_t* _dev = NULL;
static bool _wakeup = false;
void keybd_interrupt_handle(virtio_dev_t virt_dev, struct virtio_input_event *event)
{
	(void)virt_dev;
	if(_dev == NULL)
		return;

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
	}
	else if (event->type == EV_SYN)
	{
		_wakeup = true;
	}
}

static int keybd_loop(struct st_vdevice* dev, void* p) {
	(void)p;

	if(_wakeup || key_data.key_count > 0) {
		vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
		_wakeup = false;
	}
	usleep(3000);
	return 0;
}

int main(int argc, char **argv)
{
	const char *mnt_point = argc > 1 ? argv[1] : "/dev/keyboard0";

	vdevice_t dev;
	_dev = &dev;

	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "keyboard");
	dev.read = keybd_read;
	dev.check_poll_events = keybd_check_poll_events;
	dev.loop_step = keybd_loop;

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
