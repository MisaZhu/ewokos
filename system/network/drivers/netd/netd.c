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
#include "stack/ether.h"
#include "stack/ip.h"
#include "stack/loopback.h"
#include "stack/ether_tap.h"

static int network_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
    int cmd, proto_t* in, proto_t* out, void* p) {
    (void)dev;
    (void)fd;
    (void)p;
    return task_cntl(info->node, from_pid, cmd, in, out);
}

int network_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p){
    (void)dev;
    (void)oflag;
    (void)p;

    /*
     * Tasks are resolved through the live task_list keyed by the unique
     * anonymous node id, never through fsinfo.data (a duplicated/stale
     * FS_CMD_CLOSE can re-seed that blob with a freed pointer).
     */
    net_task_t *task = create_task(fd, from_pid, info->node);
    if(task == NULL) {
        if(errno == 0)
            errno = EAGAIN;
        klog("netd: network_open failed fd=%d from_pid=%d node=%u err=%d\n",
                fd, from_pid, info->node, errno);
        return -1;
    }
    return 0;
}

static int network_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)offset;
    (void)p;
    return task_read(info->node, buf, size);
}

static int network_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        const void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)offset;
    (void)p;
    return task_write(info->node, (const char*)buf, size);
}

static uint32_t network_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)p;
    return task_poll_events(info->node);
}

static int network_dup(vdevice_t* dev, int from_fd, int from_pid, int dup_fd, int dup_pid,
        uint32_t node, fsinfo_t* fsinfo, void* p) {
    (void)dev;
    (void)from_fd;
    (void)from_pid;
    (void)dup_fd;
    (void)dup_pid;
    (void)p;
    (void)fsinfo;
    /*
     * /dev/net0 fds all share the device mount, but every open gets its
     * own anonymous VFS node and each socket task has its own lifetime.
     * Cross-process dup/fork clones fsinfo.data, so keep an explicit
     * per-task refcount here; otherwise the parent's close after fork can
     * leak or prematurely reap the accepted socket. Resolve the task via
     * the live task_list (keyed by the unique node id), never through the
     * caller-supplied fsinfo.data blob, which can be stale.
     */
    net_task_t *task = task_find_live_by_node(node);
    if(task != NULL) {
        task->refs++; /* task->lock already held by the lookup */
        pthread_mutex_unlock(&task->lock);
    }
    return 0;
}

static int network_close(vdevice_t* dev, int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo,void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)p;
    /*
     * FS_CMD_CLOSE can legitimately arrive TWICE for one socket: the client's
     * close() delivers it directly, and vfsd's process-exit cleanup
     * (clear_zombie) delivers it again when the VFS slot removal lost the
     * race with KEV_PROC_EXIT at client shutdown. Resolve the task through
     * the live task_list instead of fsinfo.data: on the second close the
     * per-fd cache entry is already gone, the framework re-seeds fsinfo from
     * the caller's stale blob, and data still points at the freed task
     * (dereferencing it was the netd data-abort). A node that no longer
     * resolves to a live task is a stale/duplicate close -- drop it.
     */
    net_task_t *task = task_find_live_by_node(node);
    if(task == NULL) {
        return 0;
    }
    /* task->lock held from the lookup. */
    bool mine = false;
    if(task->refs > 0) {
        task->refs--;
        mine = (task->refs == 0);
    }
    if(mine) {
        /*
         * Reject new operations before the lock drops: release_task()
         * drains the handlers still inflight and then frees the task.
         */
        task->closing = true;
    }
    pthread_mutex_unlock(&task->lock);

    if(!mine)
        return 0;

    if(fsinfo != NULL)
        fsinfo->data = 0;
    /*
     * Release ONLY on the close whose own decrement reached zero. A close
     * that merely observes refs==0 (already released by someone else) must
     * not re-enter release_task() on a task that may be mid-teardown.
     */
    release_task(task);
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

    /* Open all devices; the protocol engine runs on the main thread via
     * device_run()'s loop_step (network_loop_step), not a private thread. */
    if (net_run() == -1) {
        slog("net_run() failure");
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

char* network_devcmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    (void)dev;
    (void)from_pid;
    (void)argc;
    (void)p;
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
            char mac[ETHER_ADDR_STR_LEN];
            ip_addr_ntop(iface->unicast, unicast, sizeof(unicast));
            ip_addr_ntop(iface->netmask, netmask, sizeof(netmask));
            ip_addr_ntop(iface->broadcast, broadcast, sizeof(broadcast));
            ip_addr_ntop(iface->gateway, gateway, sizeof(gateway));
            ether_addr_ntop(iface->iface.dev->addr, mac, sizeof(mac));

            json_var_t* var_ip = json_var_new_obj(NULL, NULL);
            json_var_add(var_ip, "ip", json_var_new_str(unicast));
            json_var_add(var_ip, "netmask", json_var_new_str(netmask));
            json_var_add(var_ip, "broadcast", json_var_new_str(broadcast));
            json_var_add(var_ip, "gateway", json_var_new_str(gateway));
            json_var_add(var_ip, "mac", json_var_new_str(mac));
            json_var_array_add(json_var, var_ip);
        }
    }
    char* ret = json_var_to_cstr(json_var);
    json_var_unref(json_var);
    return ret;
}

/*
 * One protocol-engine round (packet RX, timers, deferred VFS wakeup flush
 * plus the adaptive sleep), driven by device_run() on the main thread.
 */
static int network_loop_step(vdevice_t* dev, void* p) {
    (void)dev;
    (void)p;
    intr_step();
    return 0;
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
    strcpy(dev.desc, "networkd");

    pthread_mutex_init(&task_list_lock, NULL);
    start_task();
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
    dev.loop_step = network_loop_step;
    dev.loop_step_threaded = false;
    /*
     * IPC_MULTI_TASK: handlers run concurrently on the kernel worker pool
     * and never touch the main context, which is left free to drive the
     * protocol engine through loop_step -- netd spawns no thread of its own.
     * All shared state is guarded (task_list_lock/task->lock here,
     * socks_lock and the stack mutex inside the stack, _files_lock in the
     * vdevice framework); blocking socket ops never park a pool worker --
     * they return VFS_ERR_RETRY and are re-armed by stack wakeups.
     */
    device_run(&dev, mnt_point, FS_TYPE_ANNOUNIMOUS | FS_TYPE_CHAR, 0666, true);
    pthread_mutex_destroy(&task_list_lock);
    return 0;
}
