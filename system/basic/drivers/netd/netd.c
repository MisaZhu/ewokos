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
#include "stack/ip.h"
#include "stack/loopback.h"

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

static char ETHER_TAP_NAME[16];
void *net_thread(void* p);

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

    dev = ether_tap_init(ETHER_TAP_NAME, NULL);
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

	if(dhcp_run(dev) == -1){
        klog("dhcp_run() failure");
        return -1;
	}

	pthread_t tid;
    pthread_create(&tid, NULL, net_thread, NULL);

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

char* network_devcmd(int from_pid, int argc, char** argv, void* p) {
	char* buf = malloc(256);
	buf[0] = 0;
	if(strcmp(argv[0], "ip") == 0) {
		struct ip_iface *iface =  NULL;
		char* p = buf;
		while(true){
			iface = ip_iface_itor(iface);
			if(iface == NULL)
				break;
			char unicast[16];
			char netmask[16];	
			char broadcast[16];
			ip_addr_ntop(iface->unicast, unicast, sizeof(unicast));
			ip_addr_ntop(iface->netmask, netmask, sizeof(netmask));
			ip_addr_ntop(iface->broadcast, broadcast, sizeof(broadcast));

			snprintf(p + strlen(p), 256 - strlen(p), "IP Address: %s\n netmask: %s\n broadcast: %s\n", 
					unicast,
					netmask,
					broadcast);
		}
	}
	return buf;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/net0";
	const char* net_dev = argc > 2 ? argv[2]: "/dev/eth0";
	uint8_t mac[6];

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "networkd");

	strcpy(ETHER_TAP_NAME, net_dev);
	setup();
	
	dev.fcntl = network_fcntl;
	dev.open = network_open;
	dev.read = network_read;
	dev.write = network_write;
	dev.close = network_close;
	dev.cmd = network_devcmd;
	device_run(&dev, mnt_point, FS_TYPE_ANNOUNIMOUS | FS_TYPE_CHAR, 0666);
	return 0;
}
