#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vfs.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>
#include <ewoksys/interrupt.h>
#include <usbd/usbd.h>
#include <device/hid/keyboard.h>
#include <device/hid/touch.h>

#include "hid_keyboard.h"
typedef struct _fd_info_t{
	int fd;
	int from_pid;
    struct _usb_info *next;
    uint8_t reportId;
    uint8_t buf[8];
    uint8_t size;
}fd_info_t;



static fd_info_t *open_list= 0;

static fd_info_t* get_fd_info(int fd, int from_pid){
    fd_info_t *ptr = open_list;

    while(ptr != NULL){
        if(ptr->fd == fd && ptr->from_pid == from_pid){
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}

static void add_fd_info(fd_info_t* dev){
    fd_info_t **ptr = &open_list;
    while(*ptr != NULL){
        ptr = &((*ptr)->next);
    }
    *ptr = dev;
    dev->next = NULL;
}

static void del_fd_info(fd_info_t* dev){
    if(dev == NULL)
        return;

    fd_info_t **ptr = &open_list;
    while(*ptr != NULL){
        if(*ptr == dev){
            *ptr = dev->next;
            free(dev);
            return;
        }
        ptr = &(*ptr)->next;
    }
}

static void dispatch_data(uint8_t id, uint8_t *buf,  int size){
    fd_info_t *ptr = open_list;
    while(ptr != NULL){
        if(ptr->reportId == id){
            memcpy(ptr->buf, buf, size);
            ptr->size = size;
        }
        ptr = ptr->next;
    }
}

static int usb_step(void* p) {
	(void)p;	
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
            //klog("hid: %02x %02x %02x %02x %02x %02x %02x\n", 
            //buf[0], buf[1], buf[2],  buf[3], buf[4], buf[5], buf[6]);
            dispatch_data(buf[0], buf + 1, 7);
            proc_wakeup(RW_BLOCK_EVT);
        }
    }

    proc_usleep(10000); 
	return 0;
}

static int usb_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;

    fd_info_t *info = get_fd_info(fd, from_pid);
    if(info != NULL){
        int sz = info->size;
        if(sz > 0){
            memcpy(buf, info->buf, info->size);
            info->size = 0;
            return sz;
        }
    }

    return VFS_ERR_RETRY;
 }


static int usb_open(int fd, int from_pid, fsinfo_t* node, int oflag, void* p) {
	(void)oflag;
	(void)node;
	if(fd < 0)
		return -1;
	fd_info_t* info= (fd_info_t*)malloc(sizeof(fd_info_t));
	info= (fd_info_t*)malloc(sizeof(fd_info_t));
	if( info == NULL)
		return -1;
	memset(info, 0, sizeof(fd_info_t));
	info->fd = fd;
	info->from_pid = from_pid;

    add_fd_info(info);
	return 0;
}

static int usb_close(int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo, void* p) {
	(void)node;
	(void)fd;
    del_fd_info(get_fd_info(fd, from_pid));

	return 0;
}


static int usb_fcntl(int fd, int from_pid, fsinfo_t* info,
    	int cmd, proto_t* in, proto_t* out, void* p) {
    fd_info_t *ptr = get_fd_info(fd, from_pid);
    if(!ptr)
        return -1;
        
    int ret = -1;
    switch(cmd){
        case 0:
            ptr->reportId = proto_read_int(in);     
            ret = 0;
            break;
        defalut:
            break;
    }
    return ret;
}

void* PlatformAllocateDMA(u32 size){
    if(size > 4096)
        return NULL;

    void* ret =  dma_alloc(0, 4096);
    klog("DMA: address: %08x\n", ret);
    return ret;
}

void* PlatformDMAVir2Phy(void*v){
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
  /*
  proc_usleep(100000);
  UsbCheckForChange();
  proc_usleep(100000);
  UsbCheckForChange();
  proc_usleep(100000);
  UsbCheckForChange();
  proc_usleep(100000);
  UsbCheckForChange();
  */
}


int main(int argc, char** argv) {

	uint32_t _mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;

	usb_host_init(_mmio_base);

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/hid0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "usb-hid");
	dev.loop_step = usb_step;
    dev.fcntl = usb_fcntl;
    dev.open = usb_open;
    dev.close = usb_close;
    dev.read = usb_read;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
