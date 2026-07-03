#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/ipc.h>
#include <ewoksys/klog.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <tinyjson/tinyjson.h>
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
extern int sock_writable(int sock);
extern int sock_get_type(int id);
extern ssize_t sock_send(int id, const void *buf, size_t n);

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
	return 0;
}

static int network_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;
	(void)offset;
	(void)p;

	net_task_t *task = (net_task_t*)(ewokos_addr_t)info->data;
	net_task_t *main_task = task;
	int ret;
	int still_readable = 0;
    if(task == NULL) {
        return -1;
    }
	if(task->read_task == NULL){
		task->read_task = create_task(fd, from_pid, info->node);
        if(task->read_task == NULL) {
            return -1;
        }
		task->read_task->sock = task->sock;
		task->read_task->is_read_task = true;
	}
	task = task->read_task;
	ret = task_read(task, from_pid, buf, size, p);
	if(ret == VFS_ERR_RETRY) {
		/* Read was re-armed (not yet complete) — clear sticky flag.
		 * The async worker will call vfs_wakeup when data is actually available. */
		pthread_mutex_lock(&task_list_lock);
		main_task->pending_main_rd = false;
		pthread_mutex_unlock(&task_list_lock);
	} else {
		if(main_task->sock >= 0) {
			still_readable = sock_readable(main_task->sock);
		}
		pthread_mutex_lock(&task_list_lock);
		main_task->pending_main_rd = still_readable ? true : false;
		pthread_mutex_unlock(&task_list_lock);
	}
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
	int sock_type = -1;
	if(task->sock >= 0)
		sock_type = sock_get_type(task->sock);
	/*
	 * Plain write(2) on a TCP socket should reflect the real tcp_send() result.
	 * tcp_send() is already non-blocking: it either sends immediately or returns
	 * EAGAIN when the stream currently cannot accept more data. Routing write()
	 * through the async net_task layer turns every send into "arm + RETRY",
	 * which exposes an internal async state machine as spurious userspace
	 * EAGAIN and makes telnetd/sshd spin in write()/poll(POLLOUT).
	 */
	if(task->sock >= 0 && sock_type == SOCK_STREAM) {
		int ret = (int)sock_send(task->sock, buf, (size_t)size);
		if(ret < 0 && errno == 0)
			errno = EAGAIN;
		return ret;
	}
	return task_write(task, from_pid, (char *)buf, size, p);
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
	int read_state = NET_TASK_IDLE;
	int main_state = NET_TASK_IDLE;
	int has_read_task = 0;
	int pending_main_rd = 0;
	int can_write = 0;
	if (task != NULL) {
		pthread_mutex_lock(&task_list_lock);
		main_state = task->state;
		main_sock = task->sock;
		pending_main_rd = task->pending_main_rd;
		if (main_state == NET_TASK_IDLE || main_state == NET_TASK_FINISH) {
			can_write = 1;
		}
		if (task->read_task != NULL) {
			has_read_task = 1;
			read_state = task->read_task->state;
			if (read_state == NET_TASK_FINISH) {
				events |= VFS_EVT_RD;
			}
		}
		pthread_mutex_unlock(&task_list_lock);
		/*
		 * WR is only real when the socket can actually accept data. Publishing
		 * it purely from the IDLE/FINISH task state turns the sticky node bit
		 * back into a level-trigger: when the TCP send window is closed,
		 * tcp_send() returns EAGAIN, the worker returns to IDLE, and a client
		 * like sshd would busy-spin write()/poll() forever (appearing hung)
		 * instead of blocking until task_wakeup_tcp_writers() raises WR on the
		 * window-open ACK. Gate on sock_writable(); keep pre-socket states
		 * (main_sock < 0) writable so open/connect can still be armed.
		 */
		if (can_write && (main_sock < 0 || sock_writable(main_sock))) {
			events |= VFS_EVT_WR;
		}
		if (pending_main_rd) {
			if (main_sock >= 0 && sock_readable(main_sock)) {
				events |= VFS_EVT_RD;
			} else {
				/* Flag is stale — clear it */
				pthread_mutex_lock(&task_list_lock);
				task->pending_main_rd = false;
				pthread_mutex_unlock(&task_list_lock);
			}
		}
		if (!(events & VFS_EVT_RD) &&
				main_sock >= 0 &&
				sock_readable(main_sock)) {
			/*
			 * poll() is level-triggered: if the socket is readable right now, a
			 * waiter must observe RD even when a per-fd read_task already exists.
			 *
			 * The explicit vfs_wakeup() edge is still required while the read_task
			 * is START/PROCESS/FINISH, because user space is waiting on that async
			 * operation to complete. But once the read_task falls back to IDLE,
			 * suppressing live readability can strand callers like sshd in
			 * poll(POLLIN) forever if the edge was missed or consumed earlier.
			 */
			if(!has_read_task || read_state == NET_TASK_IDLE) {
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
		return -1;
	}

	pthread_mutex_lock(&task_list_lock);
	task->refs++;
	pthread_mutex_unlock(&task_list_lock);
	return 0;
}

static int network_close(vdevice_t* dev, int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo,void* p) {
	(void)dev;
	(void)from_pid;
	(void)node;
	(void)p;
	net_task_t *task = (net_task_t *)(ewokos_addr_t)fsinfo->data;
	if(task) {
		net_task_t *read_task = NULL;
		uint32_t main_node = task->node;
		uint32_t read_node = 0;
		pthread_mutex_lock(&task_list_lock);
		if(task->refs > 1) {
			task->refs--;
			pthread_mutex_unlock(&task_list_lock);
			return 0;
		}
		task->refs = 0;
		task->running = false;
		read_task = task->read_task;
		if(read_task != NULL) {
			read_node = read_task->node;
			task->read_task->running = false;
		}
		pthread_mutex_unlock(&task_list_lock);
		fsinfo->data = NULL;
		vfs_wakeup(main_node, VFS_EVT_CLOSE);
		if(read_node != 0)
			vfs_wakeup(read_node, VFS_EVT_CLOSE);
	}
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
