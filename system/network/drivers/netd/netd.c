#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/klog.h>
#include <ewoksys/proto.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ewokos_config.h>

#include "netd.h"
#include "platform.h"
#include "task.h"
#include "stack/util.h"
#include "stack/net.h"
#include "stack/ip.h"
#include "stack/loopback.h"
#include "stack/ether_tap.h"

extern int sock_readable(int sock);

static int network_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
	int cmd, proto_t* in, proto_t* out, void* p) {
    (void)dev;
    (void)p;
	net_task_t *task = (net_task_t*)(ewokos_addr_t)info->data;
    if(task == NULL) {
        return -1;
    }
	task->cmd = cmd;
	int res = task_cntl(task, from_pid, cmd, in, out, p);
	return res;
}

int network_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p){
	(void)dev;
	(void)oflag;
	(void)p;

	net_task_t *task = create_task(fd, from_pid, info->node);
    if(task == NULL) {
        return -1;
    }
	info->data = (ewokos_addr_t)task;
	vfs_update(info, false);
	return 0;
}

static int network_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;
	(void)offset;
	(void)p;

	net_task_t *task = (net_task_t*)(ewokos_addr_t)info->data;
    if(task == NULL) {
        return -1;
    }
	klog("[netd] network_read: fd=%d pid=%d node=%u size=%d task=%p read_task=%p sock=%d\n",
		fd, from_pid, info->node, size, task, task->read_task, task->sock);
	if(task->read_task == NULL){
		task->read_task = create_task(fd, from_pid, info->node);
        if(task->read_task == NULL) {
            return -1;
        }
		task->read_task->sock = task->sock;
		task->read_task->is_read_task = true;
		klog("[netd] network_read create read_task: fd=%d pid=%d node=%u read_task=%p sock=%d\n",
			fd, from_pid, info->node, task->read_task, task->read_task->sock);
	}
	task = task->read_task;
	int ret = task_read(task, from_pid, buf, size, p);
	klog("[netd] network_read result: fd=%d pid=%d node=%u ret=%d read_task=%p state=%d\n",
		fd, from_pid, info->node, ret, task, task->state);
	return ret;
}

static int network_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)offset;

	net_task_t *task = (net_task_t*)(ewokos_addr_t)info->data;
    if(task == NULL) {
        return -1;
    }
	klog("[netd] network_write: fd=%d pid=%d node=%u size=%d task=%p read_task=%p sock=%d\n",
		fd, from_pid, info->node, size, task, task->read_task, task->sock);
	int ret = task_write(task, from_pid, (char *)buf, size, p);
	klog("[netd] network_write result: fd=%d pid=%d node=%u ret=%d task=%p state=%d\n",
		fd, from_pid, info->node, ret, task, task->state);
	//vfs_wakeup(task->node, VFS_EVT_RW);
	return ret;
}

static uint32_t network_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;
	net_task_t *task = (net_task_t*)(ewokos_addr_t)info->data;

	uint32_t events = 0;
	int main_sock = -1;
	int read_sock = -1;
	int read_state = NET_TASK_IDLE;
	int main_state = NET_TASK_IDLE;
	if (task != NULL) {
		pthread_mutex_lock(&task_list_lock);
		main_state = task->state;
		main_sock = task->sock;
		if (main_state == NET_TASK_IDLE) {
			events |= VFS_EVT_WR;
		}
		if (task->read_task != NULL) {
			read_state = task->read_task->state;
			read_sock = task->read_task->sock;
			if (read_state == NET_TASK_FINISH) {
				events |= VFS_EVT_RD;
			}
		}
		pthread_mutex_unlock(&task_list_lock);
		if (!(events & VFS_EVT_RD)) {
			if (read_sock >= 0 && read_state == NET_TASK_IDLE && sock_readable(read_sock)) {
				events |= VFS_EVT_RD;
			}
			else if (read_sock < 0 && main_sock >= 0 && sock_readable(main_sock)) {
				events |= VFS_EVT_RD;
			}
		}
	}

	return events;
}

static int network_dup(vdevice_t* dev, int from_fd, int from_pid, int dup_fd, int dup_pid,
		uint32_t node, fsinfo_t* fsinfo, void* p) {
	(void)dev;
	(void)p;

	net_task_t *task = (net_task_t *)(ewokos_addr_t)fsinfo->data;
	if(task == NULL) {
		klog("[netd] network_dup null: fpid=%d ffd=%d cpid=%d cfd=%d node=%u\n",
			from_pid, from_fd, dup_pid, dup_fd, node);
		return -1;
	}

	klog("[netd] network_dup enter: fpid=%d ffd=%d cpid=%d cfd=%d node=%u task=%p sock=%d refs=%d\n",
		from_pid, from_fd, dup_pid, dup_fd, node, task, task->sock, task->refs);
	klog("[netd] network_dup lock begin: task=%p node=%u\n", task, node);
	pthread_mutex_lock(&task_list_lock);
	klog("[netd] network_dup lock done: task=%p node=%u\n", task, node);
	task->refs++;
	klog("[netd] network_dup: fpid=%d ffd=%d cpid=%d cfd=%d node=%u task=%p sock=%d refs=%d\n",
		from_pid, from_fd, dup_pid, dup_fd, node, task, task->sock, task->refs);
	pthread_mutex_unlock(&task_list_lock);
	return 0;
}

static int network_close(vdevice_t* dev, int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo,void* p) {
	(void)dev;
	(void)from_pid;
	(void)p;
	net_task_t *task = (net_task_t *)(ewokos_addr_t)fsinfo->data;
	if(task) {
		pthread_mutex_lock(&task_list_lock);
		if(task->refs > 1) {
			task->refs--;
			klog("[netd] network_close: keep task=%p fd=%d node=%u sock=%d refs=%d\n",
				task, fd, node, task->sock, task->refs);
			pthread_mutex_unlock(&task_list_lock);
			return 0;
		}
		task->refs = 0;
		klog("[netd] network_close: final task=%p fd=%d node=%u sock=%d read_task=%p\n",
			task, fd, node, task->sock, task->read_task);
		task->running = false;
		if(task->read_task != NULL)
			task->read_task->running = false;
		pthread_mutex_unlock(&task_list_lock);
		fsinfo->data = NULL;
	}
    //vfs_wakeup(dev->mnt_info.node, VFS_EVT_CLOSE);
	return 0;
}

#define LOOPBACK_IP_ADDR "127.0.0.1"
#define LOOPBACK_NETMASK "255.0.0.0"

#define ETHER_TAP_IP_ADDR_DEFAULT "169.254.72.2"
#define ETHER_TAP_NETMASK_DEFAULT "255.255.0.0"
#define DEFAULT_GATEWAY_DEFAULT "169.254.72.1"

static char ETHER_TAP_NAME[16];
static char ETHER_TAP_IP_ADDR[16] = ETHER_TAP_IP_ADDR_DEFAULT;
static char ETHER_TAP_NETMASK[16] = ETHER_TAP_NETMASK_DEFAULT;
static char DEFAULT_GATEWAY[16] = DEFAULT_GATEWAY_DEFAULT;
static bool ETHER_TAP_USE_DHCP = true;

void *net_thread(void* p);

static int setup(void)
{
    struct net_device *dev;
    struct ip_iface *iface;

    if (net_init() == -1) {
        slog("net_init() failure");
        return -1;
    }

    dev = loopback_init();
    if (!dev) {
        slog("loopback_init() failure");
        return -1;
    }

    iface = ip_iface_alloc(LOOPBACK_IP_ADDR, LOOPBACK_NETMASK);
    if (!iface) {
        slog("ip_iface_alloc() failure");
        return -1;
    }
    if (ip_iface_register(dev, iface) == -1) {
        slog("ip_iface_register() failure");
        return -1;
    }

    dev = ether_tap_init(ETHER_TAP_NAME, NULL);
    if (!dev) {
        slog("ether_tap_init() failure");
        return -1;
    }

    iface = ip_iface_alloc(ETHER_TAP_IP_ADDR, ETHER_TAP_NETMASK);
    if (!iface) {
        slog("ip_iface_alloc() failure");
        return -1;
    }

    if (ip_iface_register(dev, iface) == -1) {
        slog("ip_iface_register() failure");
        return -1;
    }

    if (ETHER_TAP_USE_DHCP) {
	    if(dhcp_run(dev) == -1){
            slog("dhcp_run() failure");
            return -1;
	    }
    }
    else if (ip_route_set_default_gateway(iface, DEFAULT_GATEWAY) == -1) {
        slog("ip_route_set_default_gateway() failure");
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

char* network_devcmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
	(void)dev;
	json_var_t* json_var = json_var_new_array();
	if(strcmp(argv[0], "ip") == 0) {
		struct ip_iface *iface =  NULL;
		while(true){
			iface = ip_iface_itor(iface);
			if(iface == NULL)
				break;
			char unicast[16];
			char netmask[16];	
			char broadcast[16];
			char gateway[16];
			ip_addr_ntop(iface->unicast, unicast, sizeof(unicast));
			ip_addr_ntop(iface->netmask, netmask, sizeof(netmask));
			ip_addr_ntop(iface->broadcast, broadcast, sizeof(broadcast));
			ip_addr_ntop(iface->gateway, gateway, sizeof(gateway));

			json_var_t* var_ip = json_var_new_obj(NULL, NULL);
			json_var_add(var_ip, "ip", json_var_new_str(unicast));
			json_var_add(var_ip, "netmask", json_var_new_str(netmask));
			json_var_add(var_ip, "broadcast", json_var_new_str(broadcast));
			json_var_add(var_ip, "gateway", json_var_new_str(gateway));
			json_var_array_add(json_var, var_ip);
		}
	}
	char* ret = json_var_to_cstr(json_var);
	json_var_unref(json_var);
	return ret;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/net0";
	const char* net_dev = argc > 2 ? argv[2]: "/dev/eth0";
	uint8_t mac[6];

    if (argc > 3) {
        if (argc < 6) {
            fprintf(stderr, "usage: %s [mnt_point] [net_dev] [ip netmask gateway]\n", argv[0]);
            return -1;
        }
        strncpy(ETHER_TAP_IP_ADDR, argv[3], sizeof(ETHER_TAP_IP_ADDR)-1);
        strncpy(ETHER_TAP_NETMASK, argv[4], sizeof(ETHER_TAP_NETMASK)-1);
        strncpy(DEFAULT_GATEWAY, argv[5], sizeof(DEFAULT_GATEWAY)-1);
        ETHER_TAP_USE_DHCP = false;
    }

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "networkd");

    pthread_mutex_init(&task_list_lock, NULL);
	strcpy(ETHER_TAP_NAME, net_dev);
	if(setup() != 0) {
        pthread_mutex_destroy(&task_list_lock);
        return -1;
    }
	
	dev.fcntl = network_fcntl;
	dev.open = network_open;
	dev.dup = network_dup;
	dev.read = network_read;
	dev.write = network_write;
	dev.close = network_close;
	dev.cmd = network_devcmd;
	dev.check_poll_events = network_check_poll_events;
	device_run(&dev, mnt_point, FS_TYPE_ANNOUNIMOUS | FS_TYPE_CHAR, 0666);
    pthread_mutex_destroy(&task_list_lock);
	return 0;
}
