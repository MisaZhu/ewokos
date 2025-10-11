#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vfs.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>
#include <usbd/usbd.h>
#include <device/hid/keyboard.h>
#include <device/hid/touch.h>

void* PlatformAllocateDMA(u32 size){
    if(size > 4096)
        return NULL;

    //void* ret =  dma_phy_addr(0, dma_alloc(4096));
    void* ret =  (dma_alloc(0, 4096));
    printf("DMA: address: %08x\n", ret);
    return ret;
}

void* PlatformDMAVir2Phy(void* v){
	return (void*)dma_phy_addr(0, (uint32_t)v);
}

void LogPrint(const char* message, uint32_t messageLength) {
  (void)messageLength;
  klog("%s", message);
}

static void usb_host_init(uint32_t v_mmio_base) {
  UsbInitialise(v_mmio_base);
  proc_usleep(100000);
  UsbCheckForChange();
  /*proc_usleep(100000);
  UsbCheckForChange();
  proc_usleep(100000);
  UsbCheckForChange();
  proc_usleep(100000);
  UsbCheckForChange();
  proc_usleep(100000);
  UsbCheckForChange();
  */
}

static u16 _buf[3] = {0};
static int _hasData = 0;
static int _release_count = 0;
static int _last_x = 0;
static int _last_y = 0;

static int usb_step(void* p) {
	(void)p;	
	//klog("detecting...\n");
    if(!TouchPersent()){
       UsbCheckForChange(); 
       proc_usleep(100000);
       return 0;
    }

	struct TouchEvent event;
	int ret = TouchGetEvent(&event);
	if(ret == 0 && event.event == 1){
		//fprintf(stderr, "e:%d x:%d y:%d\n", event.event, event.x, event.y);
        _buf[0] = event.event;
        _buf[1] = event.x;
        _buf[2] = event.y; 
        _hasData = 1;
        _release_count = 2;
        _last_x = event.x;
        _last_y = event.y;
        proc_wakeup(RW_BLOCK_EVT);
	}
    else if(_release_count > 0) {
        _release_count--;
        if(_release_count == 0) {
            _buf[0] = 0;
            _buf[1] = _last_x;
            _buf[2] = _last_y;
            //fprintf(stderr, "e:%d x:%d y:%d\n", _buf[0], _buf[1], _buf[2]);
            _hasData = 1;
            proc_wakeup(RW_BLOCK_EVT);
        }
    }
        
    /*
    uint8_t buf[64] = {0};

    if( !unifiedPersent()){
        //klog("detecting...\n");
        UsbCheckForChange(); 
        proc_usleep(100000);
        return 0;
    }

    if(unifiedPersent()){
        int ret = unifiedGetEvent(buf);
        if(ret == 0){
            klog("hid: %02x %02x %02x %02x %02x %02x %02x\n", 
            buf[0], buf[1], buf[2],  buf[3], buf[4], buf[5], buf[6]);
            //dispatch_data(buf[0], buf + 1, 7);
            proc_wakeup(RW_BLOCK_EVT);
        }
    }
    */

    proc_usleep(10000);
	return 0;
}

static int touch_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;

	if(size < 6)
        return 0;

    if(!_hasData)
        return 0;


    memcpy(buf, _buf, 6);
    _hasData = 0;    
	return 6;	
}

int main(int argc, char** argv) {
	uint32_t _mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;

	usb_host_init(_mmio_base);

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/touch0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "usb-touch");
	dev.loop_step = usb_step;
    dev.read = touch_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
