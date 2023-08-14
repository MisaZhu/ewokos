#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/mmio.h>
#include <sys/dma.h>
#include <usbd/usbd.h>
#include <device/hid/keyboard.h>
#include <device/hid/touch.h>

void* PlatformAllocateDMA(u32 size){
    if(size > 4096)
        return NULL;

    void* ret =  dma_map(4096);
    printf("DMA: address: %08x\n", ret);
    return ret;
}
void LogPrint(const char* message, uint32_t messageLength) {
  (void)messageLength;
  klog("%s", message);
}

void usb_host_init(void) {
  UsbInitialise();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
}

static u16 _buf[3] = {0};
static int _hasData = 0;

static int usb_step(void* p) {
	(void)p;	
	//klog("detecting...\n");
    if(!TouchPersent()){
       UsbCheckForChange(); 
       usleep(100000);
       return 0;
    }

	struct TouchEvent event;
	int ret = TouchGetEvent(&event);
	if(ret == 0){
		//printf("e:%d x:%d y:%d\n", event.event, event.x, event.y);
        _buf[0] = event.event;
        _buf[1] = event.x;
        _buf[2] = event.y; 
        _hasData = 1;
        proc_wakeup(0);
	}
    usleep(20000);
	return 0;
}

static int touch_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)p;

	if(size < 6)
        return ERR_RETRY;

    if(!_hasData)
        return ERR_RETRY;


    memcpy(buf, _buf, 6);
    _hasData = 0;    
	return 6;	
}

int main(int argc, char** argv) {
	uint32_t _mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;

	usb_host_init();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/touch0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "usb-touch");
	dev.loop_step = usb_step;
    dev.read = touch_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
