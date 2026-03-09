#ifndef SOCK_H
#define SOCK_H

#include <stddef.h>
#include <stdint.h>

#include "ip.h"

#define PF_UNSPEC   0
#define PF_LOCAL    1
#define PF_INET     2
#define PF_INET6   10

#define AF_UNSPEC   PF_UNSPEC
#define AF_LOCAL    PF_LOCAL
#define AF_INET     PF_INET
#define AF_INET6    PF_INET6

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_ICMP 1

#define INADDR_ANY ((ip_addr_t)0)

#define SOCKADDR_STR_LEN IP_ENDPOINT_STR_LEN

// ICMP packet queue for RAW sockets
struct icmp_packet {
    uint8_t data[IP_PAYLOAD_SIZE_MAX];
    size_t len;
    ip_addr_t src;
    ip_addr_t dst;
    struct icmp_packet *next;
};

struct sock {
    int used;
    int family;
    int type;
    int protocol;
    int desc;
    // For RAW sockets
    struct icmp_packet *recv_queue;
    struct icmp_packet *recv_queue_tail;
};

struct sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

struct sockaddr_in {
    unsigned short sin_family;
    uint16_t sin_port;
    ip_addr_t sin_addr;
};

#define IFNAMSIZ 16

extern int
sockaddr_pton(const char *p, struct sockaddr *n, size_t size);
extern char *
sockaddr_ntop(const struct sockaddr *n, char *p, size_t size);

extern int
sock_open(int domain, int type, int protocol);
extern int
sock_close(int id);
extern ssize_t
sock_recvfrom(int id, void *buf, size_t n, struct sockaddr *addr, int *addrlen);
extern ssize_t
sock_sendto(int id, const void *buf, size_t n, const struct sockaddr *addr, int addrlen);
extern int
sock_bind(int id, const struct sockaddr *addr, int addrlen);
extern int
sock_listen(int id, int backlog);
extern int
sock_accept(int id, struct sockaddr *addr, int *addrlen);
extern int
sock_connect(int id, const struct sockaddr *addr, int addrlen);
extern ssize_t
sock_recv(int id, void *buf, size_t n);
extern ssize_t
sock_send(int id, const void *buf, size_t n);

// Add ICMP packet to RAW socket queue
extern void
sock_add_icmp_packet(const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst);

#endif
