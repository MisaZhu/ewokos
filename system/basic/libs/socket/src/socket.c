#include <stdint.h>
#include "sys/socket.h"
#include "netinet/in.h"
#include <fcntl.h>
#include <ewoksys/vdevice.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/vfs.h>


static int do_vfs_fcntl(int fd, int cmd, proto_t* arg_in, proto_t* arg_out){ 
    int ret;
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0)
        return -1;
    while(1){
        ret = vfs_fcntl(fd, cmd,  arg_in , arg_out);
        if(ret != VFS_ERR_RETRY)
            break;
        proc_block_by(info.mount_pid, RW_BLOCK_EVT);
    };

    return ret;
}

int socket (int domain, int type, int protocol){
    int ret;
	proto_t in,out;
    int fd = open("/dev/net0", 0);
    if(fd <= 0)
        return -1;
    PF->init(&in)->addi(&in, domain)->addi(&in, type)->addi(&in, protocol);
    PF->init(&out);
    do_vfs_fcntl(fd, SOCK_OPEN, &in , &out);
    int sock = proto_read_int(&out);
    PF->clear(&in);
    PF->clear(&out);

    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
    return fd;
}

int socketpair (int domain, int type, int protocol,int fds[2]){
    //TODO:
    return -1;
}

int bind (int fd, const struct sockaddr* addr, uint32_t len){
    int ret;
	proto_t in,out;
    
    PF->init(&in)->add(&in, addr, len);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_BIND, &in , &out);
    ret = proto_read_int(&out);
    PF->clear(&in);
	PF->clear(&out);

    return ret;
}

int getsockname (int fd, struct sockaddr* addr,uint32_t * len){
    //TODO:
    return -1;
}

int connect (int fd, const struct sockaddr* addr, uint32_t len){
    int ret;
	proto_t in,out;
    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

    PF->init(&in)->add(&in, addr, len);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_CONNECT, &in , &out);
    ret = proto_read_int(&out);
    PF->clear(&in);
	PF->clear(&out);

    return ret;
}

int getpeername (int fd, struct sockaddr* addr,uint32_t * len){
    //TODO:
    return -1;
}

int32_t send (int fd, const void *buf, uint32_t n, int flags){
    int ret;
	proto_t in,out;
    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

    PF->init(&in)->add(&in, buf, n);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_SEND, &in , &out);
    ret = proto_read_int(&out);
    PF->clear(&in);
	PF->clear(&out);

    return ret;
}

int32_t recv (int fd, void *buf, uint32_t n, int flags){
    int ret;
	proto_t in,out;
    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

    PF->init(&in)->addi(&in, n);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_RECV, &in , &out);
    ret = proto_read_int(&out);
    if(ret > 0){
       proto_read_to(&out, buf, ret);
    }
    PF->clear(&in);
	PF->clear(&out);
       
    return ret;
}

int32_t sendto (int fd, const void *buf, uint32_t n,
		       int flags, const struct sockaddr* addr,
		       uint32_t addr_len){
    int ret;
	proto_t in,out;
    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;
    
    PF->init(&in)->add(&in, buf, n)->add(&in, addr, addr_len);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_SENDTO, &in , &out);
    ret = proto_read_int(&out);
    PF->clear(&in);
	PF->clear(&out);
    return ret;
}

int32_t recvfrom (int fd, void * buf, uint32_t n,
			 int flags, struct sockaddr* addr,
			 uint32_t * addr_len){
    int ret;
	proto_t in,out;
    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

    PF->init(&in)->addi(&in, n)->addi(&in, *addr_len);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_RECVFROM, &in , &out);
    ret = proto_read_int(&out);
    if(ret > 0){
        proto_read_to(&out, buf, n);
        proto_read_to(&out, addr, *addr_len);
    }
    PF->clear(&in);
	PF->clear(&out);

    return ret;
}

int32_t sendmsg (int fd, const struct msghdr *message,int flags){
    //TODO:
    return -1;
}

int32_t recvmsg (int fd, struct msghdr *message, int flags){
    //TODO:
    return -1;
}

int getsockopt (int fd, int level, int optname, void * optval, uint32_t * optlen){
    //TODO:
    return -1;
}

int setsockopt (int fd, int level, int optname,
		       const void *optval, uint32_t optlen){
    //TODO:
    return -1;
}

int listen (int fd, int n){
    int ret;
	proto_t in,out;
    PF->init(&in)->addi(&in, n);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_LISTEN, &in , &out);
    ret = proto_read_int(&out);
    PF->clear(&in);
	PF->clear(&out);

    return ret;
}

int accept (int fd, struct sockaddr* addr,uint32_t * addr_len){
    int ret;
	proto_t in,out;
    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

    PF->init(&out);
	ret = do_vfs_fcntl(fd, SOCK_ACCEPT, NULL , &out);
    ret = proto_read_int(&out);
    if(ret > 0){
        proto_read_to(&out, addr, *addr_len);
    }
	PF->clear(&out);
    int accept_fd = open("/dev/net0", 0);
    if(accept_fd <= 0)
        return -1; 

    PF->init(&in)->addi(&in, ret);
    PF->init(&out);
	do_vfs_fcntl(accept_fd, SOCK_LINK, &in , &out);
    ret = proto_read_int(&out);
    PF->clear(&in);
	PF->clear(&out);

    return accept_fd;
}

int shutdown (int fd, int how){
    int ret;
	proto_t in, out;
    PF->init(&in)->addi(&in, how);
    PF->init(&out);
    do_vfs_fcntl(fd, SOCK_CLOSE, NULL , &out);
    ret = proto_read_int(&out);
    PF->clear(&out);
    return ret;
}

struct hostent *gethostbyname(const char *name){
    return NULL;
}

int getaddrinfo( const char *node, const char *service, 
                const struct addrinfo *hints, struct addrinfo **res){
    struct  addrinfo *ret = (struct  addrinfo *)malloc(sizeof(struct addrinfo));
	struct sockaddr_in *in = (struct  sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    memset(ret, 0, sizeof(struct addrinfo));
    memset(in, 0, sizeof(IN_ADDR));
    ret->ai_next = NULL;
    ret->ai_socktype = hints->ai_socktype;
    ret->ai_family = hints->ai_family;
    
    in->sin_port = 4950;
    in->sin_family = AF_INET;
	in->sin_addr.s_un.s_un_b.s_b1 = 0xc0;
    in->sin_addr.s_un.s_un_b.s_b2 = 0xa8;
    in->sin_addr.s_un.s_un_b.s_b3 = 0x40;
    in->sin_addr.s_un.s_un_b.s_b4 = 0x01;
    ret->ai_addr = (struct sockaddr*)in;
    ret->ai_addrlen = sizeof(struct sockaddr_in); 
    ret->ai_protocol = 0x11;
    ret->ai_socktype = SOCK_DGRAM;
    *res = ret;
    return 0;
}

int freeaddrinfo(struct addrinfo **res){

}