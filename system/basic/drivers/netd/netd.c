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

#include "stack/util.h"
#include "stack/net.h"

typedef struct _net_req{
	int id;
	int fd;
	int from_pid;
	int node;
	int cmd;
	proto_t in;
	proto_t out;
	void *p;
	struct _net_req *next;
}net_req_t;

net_req_t reqs[32] = {0};

static net_req_t *req_list = NULL;
static net_req_t *ack_list = NULL;
static int req_id_seq = 1;

static net_req_t *alloc_request(void){
	for(int i = 0; i < sizeof(reqs)/sizeof(net_req_t); i++ ){
		if(reqs[i].id == 0 ){
			return &reqs[i];
		}
	}
	return NULL;
}

static void free_request(net_req_t* s){
	memset(s, 0, sizeof(net_req_t));
}

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

static int eth_select(char* dev){
	int ret = 0;
	static dev_pid = 0;

	if(dev_pid == 0){
		dev_pid = dev_get_pid(dev);
	}

    proto_t  out;
    PF->init(&out);
    if(dev_cntl_by_pid(dev_pid, 1, NULL, &out) == 0){
       ret =  proto_read_int(&out);
	}
    PF->clear(&out);
    return ret;
}


static int net_queue_push(net_req_t **list, net_req_t *req){

	while(*list!=NULL){
		list = &(*list)->next;				
	}
	*list = req;
	req->next = NULL;
	return 0;
}

static net_req_t* net_queue_pop_head(net_req_t **list){
	if(*list == NULL)
		return NULL;
	
	net_req_t *ret= *list;
	*list = ret->next;

	return ret;
}

static net_req_t* net_queue_pop_id(net_req_t **list, int id){
	while(*list!=NULL){
		net_req_t *ack = *list;
		if(ack->id == id){
			*list = ack->next;
			return ack;
		}
		list = &(*list)->next;				
	}
	return NULL;
}

static int do_network_fcntl(int fd, int from_pid, fsinfo_t* info,
	int cmd, proto_t* in, proto_t* out, void* p){
	int domain, sock,type,protocol;
	char *data;
	char tmp[1500];
	int size, addrlen = sizeof(struct sockaddr);
	struct sockaddr *paddr;
	struct sockaddr addr;
	int ret;

	sock = info->data;

	switch(cmd){
		case SOCK_OPEN:
			domain = proto_read_int(in);
			type = proto_read_int(in);
			protocol = proto_read_int(in);
			sock = sock_open(domain, type, protocol);
			PF->addi(out, sock);
			info->data = sock;
			break;
		case SOCK_BIND:
			paddr = proto_read(in, &addrlen);
			ret = sock_bind(sock, paddr, addrlen);
			PF->addi(out, ret);
			break;
		case SOCK_SENDTO:
			data = proto_read(in, &size);
			paddr = proto_read(in, &addrlen);
			ret = sock_sendto(sock, data, size, paddr, addrlen);
			PF->addi(out, ret);
			break;
		case SOCK_RECVFROM:
			size = proto_read_int(in);
			ret = sock_recvfrom(sock, tmp, size, &addr, &addrlen);
			PF->addi(out, ret);
			if(ret > 0){
				PF->add(out, tmp, size);
				PF->add(out, &addr, addrlen);	
			}
			break;
		case SOCK_SEND:
			data = proto_read(in, &size);
			ret = sock_send(sock, data, size);
			PF->addi(out, ret);
			break;
		case SOCK_RECV:
			size = proto_read_int(in);
			ret = sock_recv(sock, tmp, size);
			PF->addi(out, ret);
			if(ret > 0){
				PF->add(out, tmp, ret);
			}
			break;
		case SOCK_LISTEN:
			size = proto_read_int(in);
			ret = sock_listen(sock, size);
			PF->addi(out, ret);
			break;	
		case SOCK_ACCEPT:
			ret = sock_accept(sock, &addr, &addrlen);
			PF->addi(out, ret);
			if(ret > 0){
				PF->add(out, &addr, addrlen);	
			}
			break;	
		case SOCK_CLOSE:
			ret = sock_close(sock);
			PF->addi(out, ret);
			break;
		case SOCK_LINK:
			sock = proto_read_int(in);	
			info->data = sock;
			vfs_update(info, false);
			PF->addi(out, 0);
			break;
		case SOCK_CONNECT:
			paddr = proto_read(in, &addrlen);
			ret = sock_connect(sock, addr, addrlen);
			PF->addi(out, ret);
			break;
		default:
			break;
	}
    return 0;
}

static int network_split_fcntl(int fd, int from_pid, uint32_t node,
	int cmd, proto_t* in, proto_t *out, void* p) {

	req_id_seq++;
	net_req_t* req = alloc_request();

	if(!req){
		PF->addi(out, VFS_ERR_RETRY);
		return 0;
	}

	req->id = req_id_seq;
	req->fd = fd;
	req->from_pid = from_pid;
	req->node = node;
	req->cmd = proto_read_int(in);	
	req->p = p;
	PF->init(&req->in);
	PF->init(&req->out);
	req->next = NULL;

	proto_read_proto(in, &req->in);
	net_queue_push(&req_list, req);	

	PF->addi(out, req_id_seq);
	return 0;
}

static int network_split_ack(int fd, int from_pid, uint32_t node,
	int cmd, proto_t*in,  proto_t* out, void* p) {

	int id = proto_read_int(in);

	net_req_t *ack = net_queue_pop_id(&ack_list, id);
	if(ack){
		PF->addi(out, 0);
		PF->add(out, ack->out.data, ack->out.size);
		PF->clear(&ack->out);
		PF->clear(&ack->in);
		free_request(ack);
		return 0;
	}
	PF->addi(out, VFS_ERR_RETRY);
	return 0;
}

static int network_fcntl(int fd, int from_pid, fsinfo_t* info,
	int cmd, proto_t* in, proto_t* out, void* p) {
    (void)p;
	if(cmd < SOCK_REQUEST){
		return do_network_fcntl(fd, from_pid, info, cmd, in, out, p);	
	}else if(cmd == SOCK_REQUEST){
		return network_split_fcntl(fd, from_pid, info->node, cmd, in, out, p);	
	}else if(cmd == SOCK_ACK){
		return network_split_ack(fd, from_pid, info->node, cmd, in, out, p);
	}
}

int network_open(int fd, int from_pid, fsinfo_t* node, int oflag, void* p){
	return 0;
}

static int network_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)p;
    return VFS_ERR_RETRY;
}


static int network_write(int fd, int from_pid, fsinfo_t* node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	uint8_t mac[6];	
	return VFS_ERR_RETRY;
}

static int network_close(int fd, int from_pid, uint32_t node, void* p) {
	(void)fd;
	// fsinfo_t test;
	// vfs_get_by_node(node, &test);
	fsinfo_t* info = dev_get_file(fd, from_pid, node);
	// klog("%d %d\n", test.data, info->data);

	int sock = info->data;
	if(sock >= 0){
		sock_close(sock);	
	}
	return 0;
}

static int task_cnt = 0;
static void* network_task(void* p) {
		net_req_t *req = (net_req_t*)p;
		
		fsinfo_t* info = dev_get_file(req->fd, req->from_pid, req->node);
		if(info) {
			do_network_fcntl(req->fd, req->from_pid, info, 
			req->cmd, &req->in, &req->out, req->p);	
		}else{
			PF->addi(&req->out, -1);	
		}

		ipc_disable();
		net_queue_push(&ack_list, req);
		ipc_enable();
        proc_wakeup(RW_BLOCK_EVT);
		task_cnt--;
		return NULL;
}


#define LOOPBACK_IP_ADDR "127.0.0.1"
#define LOOPBACK_NETMASK "255.0.0.0"

#define ETHER_TAP_IP_ADDR "169.254.72.2"
#define ETHER_TAP_NETMASK "255.255.0.0"

#define DEFAULT_GATEWAY "169.254.72.1"

char ETHER_TAP_HW_ADDR[32];
static char ETHER_TAP_NAME[16];

static void* network_loop(void* p) {
	while(1){
    	pthread_t tid;
		ipc_disable();
		net_req_t *req = net_queue_pop_head(&req_list);
		ipc_enable();
		if(!req)
			break;
		task_cnt++;
		pthread_create(&tid, NULL, network_task, req);
	}

	if(eth_select(ETHER_TAP_NAME)){
		raise_softirq(SIGUSR3);
	}
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
	dev.loop_step = network_loop;
	//uint32_t tid = timer_set(1000, net_timer_handler);
	device_run(&dev, mnt_point, FS_TYPE_ANNOUNIMOUS | FS_TYPE_CHAR, 0664);
	//timer_remove(tid);
	return 0;
}
