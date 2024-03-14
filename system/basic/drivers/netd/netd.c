#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <ewoksys/proto.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "netd.h"
#include "platform.h"
#include "task.h"

#include "stack/util.h"
#include "stack/net.h"

static int get_mac_address(char* dev, uint8_t* buf){
	int ret = -1;
    proto_t  out;
    PF->init(&out);
    ret = dev_cntl(dev, 0, NULL, &out);
	if(ret == 0){
        proto_read_to(&out, buf, 6);
	}
    PF->clear(&out);
    return ret;
}



static int network_fcntl(int fd, int from_pid, fsinfo_t* info,
	int cmd, proto_t* in, proto_t* out, void* p) {
    (void)p;
	net_task_t *task = (net_task_t*)info->data;
	return task_cntl(task, from_pid, cmd, in, out, p);
}

int network_open(int fd, int from_pid, fsinfo_t* info, int oflag, void* p){

	net_task_t *task = create_task(fd, from_pid, info->node);
	info->data = task;
	vfs_update(info, false);
	return 0;
}

static int network_read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)p;

	net_task_t *task = (net_task_t*)info->data;
	int ret = task_read(task, from_pid, buf + offset, size, p);
    return ret > 0? ret : VFS_ERR_RETRY;
}


static int network_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;

	net_task_t *task = (net_task_t*)info->data;
	int ret = task_write(task, from_pid, buf + offset, size, p);
    return ret > 0? ret : VFS_ERR_RETRY;
}

static int network_close(int fd, int from_pid, uint32_t node, bool delnode,void* p) {
	(void)fd;
	fsinfo_t* info = dev_get_file(fd, from_pid, node);
	// fsinfo_t info;
	// vfs_get_by_node(node, &info);

	if(delnode){
		net_task_t *task = (net_task_t *)info->data;
		if(task)
			release_task(task);
	}

	return 0;
}

#define LOOPBACK_IP_ADDR "127.0.0.1"
#define LOOPBACK_NETMASK "255.0.0.0"

#define ETHER_TAP_IP_ADDR "169.254.72.2"
#define ETHER_TAP_NETMASK "255.255.0.0"

#define DEFAULT_GATEWAY "169.254.72.1"

char ETHER_TAP_HW_ADDR[32];
static char ETHER_TAP_NAME[16];

static void* network_loop(void* p) {
	// if(eth_select(ETHER_TAP_NAME)){
	// 	raise_softirq(SIGIRQ);
	// }
	raise_softirq(SIGALRM);
	proc_usleep(1000);
}

static int setup(void)
{
    struct net_device *dev;
    struct ip_iface *iface;

    if (net_init() == -1) {
        klog("net_init() failure");
        return -1;
    }

    dev = loopback_init();
    if (!dev) {
        klog("loopback_init() failure");
        return -1;
    }

    iface = ip_iface_alloc(LOOPBACK_IP_ADDR, LOOPBACK_NETMASK);
    if (!iface) {
        klog("ip_iface_alloc() failure");
        return -1;
    }
    if (ip_iface_register(dev, iface) == -1) {
        klog("ip_iface_register() failure");
        return -1;
    }

    dev = ether_tap_init(ETHER_TAP_NAME, ETHER_TAP_HW_ADDR);
    if (!dev) {
        klog("ether_tap_init() failure");
        return -1;
    }

    iface = ip_iface_alloc(ETHER_TAP_IP_ADDR, ETHER_TAP_NETMASK);
    if (!iface) {
        klog("ip_iface_alloc() failure");
        return -1;
    }

    if (ip_iface_register(dev, iface) == -1) {
        klog("ip_iface_register() failure");
        return -1;
    }

    // if (ip_route_set_default_gateway(iface, DEFAULT_GATEWAY) == -1) {
    //     klog("ip_route_set_default_gateway() failure");
    //     return -1;
    // }

	if(dhcp_run(dev) == -1){
        klog("dhcp_run() failure");
        return -1;
	}

    if (net_run() == -1) {
        klog("net_run() failure");
        return -1;
    }
    return 0;
}

int gettimeofday_plat(struct timeval *tp, void *tzp){
	uint64_t usec;
	kernel_tic(NULL, &usec);

	tp->tv_sec = usec / 1000000;
	tp->tv_usec = usec % 1000000;
	return 0;
}

void mac2str(uint8_t *mac,  char* str){
	for(int i = 0; i < 6; i++){
		uint8_t val = mac[i];
		uint8_t hval = (val>>4)&0xf;
		uint8_t lval = val&0xf;

		if(hval <= 9)
			*str++ = hval + '0';
		else
			*str++ = hval - 10 + 'A';
		if(lval <= 9)
			*str++ = lval + '0';
		else
			*str++ = lval - 10 + 'A';
		*str++ = ':';
	}
	*(str - 1) = '\0';
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/net0";
	const char* net_dev = argc > 2 ? argv[2]: "/dev/eth0";
	uint8_t mac[6];

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "networkd");

	get_mac_address(net_dev,  mac);
	mac2str(mac, ETHER_TAP_HW_ADDR);
	strcpy(ETHER_TAP_NAME, net_dev);
	setup();
	
	//dev.dev_cntl = network_dcntl; 
	dev.fcntl = network_fcntl;
	dev.open = network_open;
	dev.read = network_read;
	dev.write = network_write;
	dev.close = network_close;
	//dev.loop_step = network_loop;
	//uint32_t tid = timer_set(1000, net_timer_handler);
	device_run(&dev, mnt_point, FS_TYPE_ANNOUNIMOUS | FS_TYPE_CHAR, 0666);
	//timer_remove(tid);
	return 0;
}
