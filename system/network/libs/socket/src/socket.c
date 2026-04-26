#include <stdint.h>
#include "sys/socket.h"
#include "netinet/in.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <ewoksys/vdevice.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <sys/time.h>


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

    if (sock < 0) {
        close(fd);
        return -1;
    }

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
    return write(fd, buf, n);
    /*
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
    */
}

int32_t recv (int fd, void *buf, uint32_t n, int flags){
    return read(fd, buf, n);
    /*
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
    */
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

int getsockopt (int fd, int level, int optname, void * optval, uint32_t * optlen)
{
    int ret;
    proto_t in, out;
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0)
        return -1;

    PF->init(&in)->addi(&in, level)->addi(&in, optname)->addi(&in, *optlen);
    PF->init(&out);
    ret = do_vfs_fcntl(fd, SOCK_GETOPT, &in, &out);
    if(ret == 0) {
        ret = proto_read_int(&out);
        if(ret == 0) {
            int returned_len = proto_read_int(&out);
            if(returned_len > 0) {
                proto_read_to(&out, optval, returned_len);
                *optlen = returned_len;
            }
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return ret;
}

int setsockopt (int fd, int level, int optname,
		       const void *optval, uint32_t optlen){
    int ret;
	proto_t in,out;
    fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

    PF->init(&in)->addi(&in, level)->addi(&in, optname)->add(&in, optval, optlen);
    PF->init(&out);
	do_vfs_fcntl(fd, SOCK_SETOPT, &in , &out);
    ret = proto_read_int(&out);
    PF->clear(&in);
	PF->clear(&out);

    return ret;
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

// DNS相关结构体
struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed));

struct dns_question {
    // 域名部分是可变长度的，以0结尾
    // 后面跟着type和class
    uint16_t type;
    uint16_t class;
} __attribute__((packed));

struct dns_rr {
    // 域名部分是可变长度的，以0结尾
    // 后面跟着type、class、ttl、rdlength和rdata
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    // rdata部分是可变长度的
} __attribute__((packed));

// 静态存储区，用于存储gethostbyname的返回结果
static struct hostent hostent_result;
static char hostent_name_buf[256];
static char* hostent_name;
static char* hostent_aliases[2];
static struct in_addr hostent_addr;
static char* hostent_addr_list[2];

// DNS解析函数
static int dns_resolve(const char* domain, struct in_addr* addr, const char* dns_server_ipv4) {
    int sockfd;
    struct sockaddr_in server_addr, from_addr;
    socklen_t from_len;
    uint8_t buf[512];
    int len, i;
    struct dns_header* header;
    uint8_t* ptr;
    uint16_t qtype, qclass;
    uint16_t ancount, type, class, rdlength;

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = 3; // 3秒超时
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return -1;
    }

    // 设置DNS服务器地址（使用Google DNS）
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    server_addr.sin_addr.s_un.s_addr = inet_addr(dns_server_ipv4);

    // 构造DNS查询数据包
    header = (struct dns_header*)buf;
    header->id = htons(12345);
    header->flags = htons(0x0100); // 标准查询
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    // 构造查询域名
    ptr = buf + sizeof(struct dns_header);
    const char* domain_ptr = domain;
    while (*domain_ptr) {
        const char* label_end = strchr(domain_ptr, '.');
        if (label_end) {
            *ptr++ = label_end - domain_ptr;
            memcpy(ptr, domain_ptr, label_end - domain_ptr);
            ptr += label_end - domain_ptr;
            domain_ptr = label_end + 1;
        } else {
            *ptr++ = strlen(domain_ptr);
            memcpy(ptr, domain_ptr, strlen(domain_ptr));
            ptr += strlen(domain_ptr);
            break;
        }
    }
    *ptr++ = 0; // 域名结束符

    // 添加查询类型和类
    qtype = htons(1); // A记录
    qclass = htons(1); // IN类
    memcpy(ptr, &qtype, 2);
    ptr += 2;
    memcpy(ptr, &qclass, 2);
    ptr += 2;

    len = ptr - buf;

    // 发送查询
    if (sendto(sockfd, buf, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }

    // 接收响应
    from_len = sizeof(from_addr);
    len = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&from_addr, &from_len);
    if (len < 0) {
        close(sockfd);
        return -1;
    }

    // 解析响应
    header = (struct dns_header*)buf;
    ancount = ntohs(header->ancount);

    // 跳过查询部分
    ptr = buf + sizeof(struct dns_header);
    while (ptr < buf + len && *ptr) {
        uint8_t label_len = *ptr;
        if (label_len == 0 || ptr + label_len + 1 > buf + len) {
            close(sockfd);
            return -1;
        }
        ptr += label_len + 1;
    }
    if (ptr >= buf + len) {
        close(sockfd);
        return -1;
    }
    ptr += 1; // 跳过域名结束符
    if (ptr + 4 > buf + len) {
        close(sockfd);
        return -1;
    }
    // 检查查询部分的type和class
    qtype = ntohs(*(uint16_t*)ptr);
    qclass = ntohs(*(uint16_t*)(ptr + 2));
    ptr += 4; // 跳过type和class

    // 解析回答部分
    for (i = 0; i < ancount; i++) {
        // 跳过域名（可能是压缩格式）
        while (ptr < buf + len) {
            if (ptr >= buf + len) {
                close(sockfd);
                return -1;
            }
            if (*ptr == 0) {
                ptr += 1;
                break;
            }
            if ((*ptr & 0xc0) == 0xc0) {
                // 压缩格式，跳过2字节
                if (ptr + 2 > buf + len) {
                    close(sockfd);
                    return -1;
                }
                ptr += 2;
                break;
            }
            uint8_t label_len = *ptr;
            if (label_len == 0 || ptr + label_len + 1 > buf + len) {
                close(sockfd);
                return -1;
            }
            ptr += label_len + 1;
        }

        // 读取type、class、ttl、rdlength
        if (ptr + 10 > buf + len) {
            close(sockfd);
            return -1;
        }
        // 确保指针对齐
        uint16_t type_val = *(uint16_t*)ptr;
        type = ntohs(type_val);
        ptr += 2;
        uint16_t class_val = *(uint16_t*)ptr;
        class = ntohs(class_val);
        ptr += 2;
        ptr += 4; // 跳过ttl
        uint16_t rdlength_val = *(uint16_t*)ptr;
        rdlength = ntohs(rdlength_val);
        ptr += 2;

        // 如果是A记录，提取IP地址
        if (type == 1 && class == 1 && rdlength == 4) {
            if (ptr + 4 > buf + len) {
                close(sockfd);
                return -1;
            }
            memcpy(&addr->s_un.s_addr, ptr, 4);
            close(sockfd);
            return 0;
        }

        if (ptr + rdlength > buf + len) {
            close(sockfd);
            return -1;
        }
        ptr += rdlength;
    }

    close(sockfd);
    return -1;
}

in_addr_t inet_addr(const char *cp) {
    struct in_addr addr;
    if (inet_pton(AF_INET, cp, &addr) == 1) {
        return addr.s_un.s_addr;
    }
    return INADDR_NONE;
}

struct hostent *gethostbyname(const char *name){
    // 检查输入参数
    if (name == NULL) {
        //errno = EINVAL;
        return NULL;
    }

    // 尝试将name解析为IPv4地址
    if (inet_pton(AF_INET, name, &hostent_addr) == 1) {
        // 成功解析为IP地址
        strncpy(hostent_name_buf, name, sizeof(hostent_name_buf));
        hostent_name_buf[sizeof(hostent_name_buf) - 1] = '\0';
        hostent_name = hostent_name_buf;
        hostent_aliases[0] = NULL;
        hostent_aliases[1] = NULL;
        hostent_addr_list[0] = (char*)&hostent_addr;
        hostent_addr_list[1] = NULL;

        hostent_result.h_name = hostent_name;
        hostent_result.h_aliases = hostent_aliases;
        hostent_result.h_addrtype = AF_INET;
        hostent_result.h_length = sizeof(struct in_addr);
        hostent_result.h_addr_list = hostent_addr_list;
        return &hostent_result;
    }

    int num = 2;
    const char* dns_servers[] = {
            "223.5.5.5", //ali-cloud
            "8.8.8.8" //google
        };

    for(int i=0; i<num; i++) {
        if (dns_resolve(name, &hostent_addr, dns_servers[i]) == 0) {
            strncpy(hostent_name_buf, name, sizeof(hostent_name_buf));
            hostent_name_buf[sizeof(hostent_name_buf) - 1] = '\0';
            hostent_name = hostent_name_buf;
            hostent_aliases[0] = NULL;
            hostent_aliases[1] = NULL;
            hostent_addr_list[0] = (char*)&hostent_addr;
            hostent_addr_list[1] = NULL;

            hostent_result.h_name = hostent_name;
            hostent_result.h_aliases = hostent_aliases;
            hostent_result.h_addrtype = AF_INET;
            hostent_result.h_length = sizeof(struct in_addr);
            hostent_result.h_addr_list = hostent_addr_list;
            return &hostent_result;
        }
    }

    // 解析失败
    errno = ENOENT;
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
    if (res == NULL || *res == NULL) {
        return 0;
    }

    struct addrinfo *current = *res;
    while (current) {
        struct addrinfo *next = current->ai_next;
        
        // 释放ai_addr指向的内存
        if (current->ai_addr) {
            free(current->ai_addr);
        }
        
        // 释放ai_canonname指向的内存
        if (current->ai_canonname) {
            free(current->ai_canonname);
        }
        
        // 释放当前addrinfo结构体
        free(current);
        
        current = next;
    }
    
    *res = NULL;
    return 0;
}
