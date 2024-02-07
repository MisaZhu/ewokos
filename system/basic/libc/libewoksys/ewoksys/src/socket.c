#include <stdint.h>
#include <ewoksys/socket.h>

int socket (int domain, int type, int protocol){
    int fd = fopen("/dev/net0", "rw");
    switch(type){
        case SOCK_RAW:
            break;
        case SOCK_DGRAM:
            break;
        case SOCK_STREAM:
            break;
        default:
            break;
    }

    return fd;
}

int socketpair (int domain, int type, int protocol,int fds[2]){
    //TODO:
    return 0;
}

int bind (int fd, const struct sockaddr* addr, uint32_t len){
    //TODO:
    return 0;
}

int getsockname (int fd, struct sockaddr* addr,uint32_t * len){
    //TODO:
    return 0;
}

int connect (int fd, const struct sockaddr* addr, uint32_t len){
    //TODO:
    return 0;
}

int getpeername (int fd, struct sockaddr* addr,uint32_t * len){
    //TODO:
    return 0;
}

int32_t send (int fd, const void *buf, uint32_t n, int flags){
    //TODO:
    return 0;
}

int32_t recv (int fd, void *buf, uint32_t n, int flags){
    //TODO:
    return 0;
}

int32_t sendto (int fd, const void *buf, uint32_t n,
		       int flags, const struct sockaddr* addr,
		       uint32_t addr_len){
    //TODO:
    return 0;
}

int32_t recvfrom (int fd, void * buf, uint32_t n,
			 int flags, struct sockaddr* addr,
			 uint32_t * addr_len){
    //TODO:
    return 0;
}

int32_t sendmsg (int fd, const struct msghdr *message,int flags){
    //TODO:
    return 0;
}

int32_t recvmsg (int fd, struct msghdr *message, int flags){
    //TODO:
    return 0;
}

int getsockopt (int fd, int level, int optname, void * optval, uint32_t * optlen){
    //TODO:
    return 0;
}

int setsockopt (int fd, int level, int optname,
		       const void *optval, uint32_t optlen){
    //TODO:
    return 0;
}

int listen (int fd, int n){
    //TODO:
    return 0;
}

int accept (int fd, struct sockaddr* addr,uint32_t * addr_len){
    //TODO:
    return 0;
}

int shutdown (int fd, int how){
    //TODO:
    return 0;
}

struct hostent *gethostbyname(const char *name){
    //TODO:
    return 0;
}