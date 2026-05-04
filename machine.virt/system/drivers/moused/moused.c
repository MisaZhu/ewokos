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
#include <mouse/mouse.h>

#define EV_KEY 0x01 // Key event
#define EV_REL 0x02 // Relative coordinate event (e.g., mouse movement)
#define EV_ABS 0x03 // Absolute coordinate event (e.g., touchscreen)
#define EV_SYN 0x00 // Synchronization event

#define ABS_X 0x00 // Absolute X coordinate
#define ABS_Y 0x01 // Absolute Y coordinate

#define REL_X 0x00 // Relative X coordinate
#define REL_Y 0x01 // Relative Y coordinate
#define REL_HWHEEL 0x06 // Horizontal wheel
#define REL_WHEEL 0x08 // Vertical wheel

#define BTN_LEFT 0x110	// Left button
#define BTN_RIGHT 0x111 // Right button


#define CACHE_SIZE (32)
static mouse_evt_t mouse_data[CACHE_SIZE];
static uint32_t mouse_data_read = 0;
static uint32_t mouse_data_write = 0;

static int _read(vdevice_t* dev, int fd, int from_pid, fsinfo_t *info,
					  void *buf, int size, int offset, void *p)
{
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)info;

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

static vdevice_t* _dev = NULL;
static bool _wakeup = false;
void mouse_interrupt_handle(struct virtio_device *virt_dev, struct virtio_input_event *event)
{
	(void)virt_dev;
	if(_dev == NULL)
		return;

	if (event->type == EV_REL)
	{
		mouse_data[mouse_data_write % CACHE_SIZE].type = 1;
		if (event->code == REL_X)
		{
			mouse_data[mouse_data_write % CACHE_SIZE].x = event->value;
		}
		else if (event->code == REL_Y)
		{
			mouse_data[mouse_data_write % CACHE_SIZE].y = event->value;
		}
		else if (event->code == REL_WHEEL)
		{
			// Vertical wheel: value > 0 scroll up, value < 0 scroll down
			// Each wheel event is reported independently, without waiting for EV_SYN
			// Clear current slot before writing to avoid stale data
			memset(&mouse_data[mouse_data_write % CACHE_SIZE], 0, sizeof(mouse_evt_t));
			mouse_data[mouse_data_write % CACHE_SIZE].type = 1;
			// Cast to signed int32_t for proper negative value comparison
			int32_t wheel_value = (int32_t)event->value;
			if (wheel_value > 0)
			{
				mouse_data[mouse_data_write % CACHE_SIZE].button = MOUSE_BUTTON_SCROLL_DOWN;
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_MOVE;
			}
			else if (wheel_value < 0)
			{
				mouse_data[mouse_data_write % CACHE_SIZE].button = MOUSE_BUTTON_SCROLL_UP;
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_MOVE;
			}
		}
		else if (event->code == REL_HWHEEL)
		{
			// Horizontal wheel: value > 0 scroll right, value < 0 scroll left
			// Each wheel event is reported independently, without waiting for EV_SYN
			// Clear current slot before writing to avoid stale data
			memset(&mouse_data[mouse_data_write % CACHE_SIZE], 0, sizeof(mouse_evt_t));
			mouse_data[mouse_data_write % CACHE_SIZE].type = 1;
			// Cast to signed int32_t for proper negative value comparison
			int32_t wheel_value = (int32_t)event->value;
			if (wheel_value > 0)
			{
				mouse_data[mouse_data_write % CACHE_SIZE].button = MOUSE_BUTTON_SCROLL_RIGHT;
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_MOVE;
			}
			else if (wheel_value < 0)
			{
				mouse_data[mouse_data_write % CACHE_SIZE].button = MOUSE_BUTTON_SCROLL_LEFT;
				mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_MOVE;
			}
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
		if (mouse_data_write - mouse_data_read >= CACHE_SIZE)
		{
			//klog("moused: buffer overflow, dropping event\n");
			mouse_data_read++;
		}
		mouse_data_write++;
		if(!mouse_data[mouse_data_write % CACHE_SIZE].state)
			 mouse_data[mouse_data_write % CACHE_SIZE].state = MOUSE_STATE_MOVE;
		_wakeup = true;
	}
}

static int mouse_step(struct st_vdevice* dev, void* p) {
	(void)p;
	if(_wakeup) {
		vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
		_wakeup = false;
	}
	usleep(3000);
	return 0;
}

int main(int argc, char **argv)
{
	const char *mnt_point = argc > 1 ? argv[1] : "/dev/mouse0";
	_mmio_base = mmio_map();
	if (_mmio_base == 0) {
		klog("moused: mmio_map failed\n");
		return -1;
	}

	vdevice_t dev;
	_dev = &dev;

	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "mouse");
	dev.read = _read;
	dev.loop_step = mouse_step;

	virtio_dev_t vio = virtio_input_get("QEMU Virtio Tablet");
	if(!vio){
		vio =  virtio_input_get("QEMU Virtio Mouse");
	}
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
