#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "util.h"
#include "net.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

#include "sock.h"

// Implement timersub if not defined
#ifndef timersub
#define timersub(a, b, result) \
  do { \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
    if ((result)->tv_usec < 0) { \
      --(result)->tv_sec; \
      (result)->tv_usec += 1000000; \
    } \
  } while (0)
#endif

#define SOCKS_MAX 128
static struct sock socks[SOCKS_MAX];

int
sockaddr_pton(const char *p, struct sockaddr *n, size_t size)
{
    struct ip_endpoint ep;

    if (ip_endpoint_pton(p, &ep) == 0) {
        if (size < sizeof(struct sockaddr_in)) {
            return -1;
        }
        ((struct sockaddr_in *)n)->sin_family = AF_INET;
        ((struct sockaddr_in *)n)->sin_port = ep.port;
        ((struct sockaddr_in *)n)->sin_addr = ep.addr;
        return 0;
    }
    return -1;
}

char *
sockaddr_ntop(const struct sockaddr *n, char *p, size_t size)
{
    struct ip_endpoint ep;

    switch (n->sa_family) {
    case AF_INET:
        if (size < IP_ENDPOINT_STR_LEN) {
            return NULL;
        }
        ep.port = ((struct sockaddr_in *)n)->sin_port;
        ep.addr = ((struct sockaddr_in *)n)->sin_addr;
        return ip_endpoint_ntop(&ep, p, size);
    }
    return NULL;
}

static struct sock *
sock_alloc(void)
{
    struct sock *entry;

    for (entry = socks; entry < tailof(socks); entry++) {
        if (!entry->used) {
            entry->used = 1;
            return entry;
        }
    }
    slog("no free socket\n");
    return NULL;
}

static int
sock_free(struct sock *s)
{
    // Free ICMP packet queue for RAW sockets
    if (s->type == SOCK_RAW) {
        struct icmp_packet *packet = s->recv_queue;
        while (packet) {
            struct icmp_packet *next = packet->next;
            free(packet);
            packet = next;
        }
    }
    memset(s, 0, sizeof(*s));
    return 0;
}

static struct sock *
sock_get(int id)
{
    if (id < 0 || id >= (int)countof(socks)) {
        /* out of range */
        return NULL;
    }
    return &socks[id];
}

int
sock_open(int domain, int type, int protocol)
{
    struct sock *s;
    if (domain != AF_INET) {
        return -17;
    }
    if (type != SOCK_STREAM && type != SOCK_DGRAM && type != SOCK_RAW) {
        return -1;
    }
    if (protocol == 0) {
        protocol = (type == SOCK_STREAM) ? IPPROTO_TCP : (type == SOCK_DGRAM) ? IPPROTO_UDP : 0;
    }
    if ((type == SOCK_STREAM && protocol != IPPROTO_TCP) ||
        (type == SOCK_DGRAM && protocol != IPPROTO_UDP)) {
        return -1;
    }
    s = sock_alloc();
    if (!s) {
        return -1;
    }
    s->family = domain;
    s->type = type;
    s->protocol = protocol;
    memset(&s->rcv_timeout, 0, sizeof(struct timeval));
    memset(&s->snd_timeout, 0, sizeof(struct timeval));
    switch (s->type) {
    case SOCK_STREAM:
        s->desc = tcp_open();
        break;
    case SOCK_DGRAM:
        s->desc = udp_open();
        break;
    case SOCK_RAW:
        s->desc = 0; // RAW socket doesn't need a separate descriptor
        break;
    }
    if (s->type != SOCK_RAW && s->desc == -1) {
        return -1;
    }
    return indexof(socks, s);
}

int
sock_close(int id)
{
    struct sock *s;
    int retry = 100; // Max 10 seconds wait (100 * 100ms)
    s = sock_get(id);
    if (!s) {
        return -17;
    }
    switch (s->type) {
    case SOCK_STREAM:
        // Try to close TCP connection gracefully
        while (retry-- > 0) {
            int ret = tcp_close(s->desc);
            if (ret == 0 || ret == -17) {
                // Connection closed successfully or PCB already released (RST)
                break;
            }
            // If connection is still closing, wait a bit
            usleep(10000); // 10ms
        }
        if (retry <= 0) {
            errorf("sock_close: timeout waiting for TCP close");
        }
        break;    
    case SOCK_DGRAM:
        udp_close(s->desc);
        break;
    case SOCK_RAW:
        // RAW socket doesn't need closing
        break;
    default:
        return -1;
    }
    return sock_free(s);
}

ssize_t
sock_recvfrom(int id, void *buf, size_t n, struct sockaddr *addr, int *addrlen)
{
    struct sock *s;
    struct ip_endpoint ep;
    int ret;
    s = sock_get(id);
    if (!s) {
        return -17;
    }
    switch (s->type) {
    case SOCK_DGRAM:
        switch (s->family) {
        case AF_INET:
            ret = udp_recvfrom(s->desc, (uint8_t *)buf, n, &ep);
            if (ret != -1) {
                ((struct sockaddr_in *)addr)->sin_addr = ep.addr;
                ((struct sockaddr_in *)addr)->sin_port = ep.port;
            }
            return ret;
        }
        return -1;
    case SOCK_RAW:
        switch (s->family) {
        case AF_INET:
            // Check if there are packets in the receive queue
            if (s->recv_queue) {
                struct icmp_packet *packet = s->recv_queue;
                
                // Remove packet from queue
                s->recv_queue = packet->next;
                if (!s->recv_queue) {
                    s->recv_queue_tail = NULL;
                }
                
                // Copy packet data to buffer
                size_t copy_len = (n < packet->len) ? n : packet->len;
                memcpy(buf, packet->data, copy_len);
                
                // Set source address
                if (addr && addrlen) {
                    ((struct sockaddr_in *)addr)->sin_family = AF_INET;
                    ((struct sockaddr_in *)addr)->sin_addr = packet->src;
                    ((struct sockaddr_in *)addr)->sin_port = 0; // ICMP doesn't use port
                    *addrlen = sizeof(struct sockaddr_in);
                }
                
                // Free the packet
                free(packet);
                return copy_len;
            }
            
            // If no packets and timeout is set, wait with timeout
            uint32_t rcv_timeout = s->rcv_timeout.tv_sec * 1000000 + s->rcv_timeout.tv_usec;
            if (rcv_timeout > 0) {
                struct timeval start, now, diff;
                gettimeofday(&start, NULL);
                
                while (1) {
                    // Check if packet arrived
                    if (s->recv_queue) {
                        struct icmp_packet *packet = s->recv_queue;
                        
                        // Remove packet from queue
                        s->recv_queue = packet->next;
                        if (!s->recv_queue) {
                            s->recv_queue_tail = NULL;
                        }
                        
                        // Copy packet data to buffer
                        size_t copy_len = (n < packet->len) ? n : packet->len;
                        memcpy(buf, packet->data, copy_len);
                        
                        // Set source address
                        if (addr && addrlen) {
                            ((struct sockaddr_in *)addr)->sin_family = AF_INET;
                            ((struct sockaddr_in *)addr)->sin_addr = packet->src;
                            ((struct sockaddr_in *)addr)->sin_port = 0; // ICMP doesn't use port
                            *addrlen = sizeof(struct sockaddr_in);
                        }
                        
                        // Free the packet
                        free(packet);
                        return copy_len;
                    }
                    
                    // Check if timeout occurred
                    gettimeofday(&now, NULL);
                    timersub(&now, &start, &diff);
                    if ((diff.tv_sec * 1000000 + diff.tv_usec) >= rcv_timeout) {
                        errno = ETIMEDOUT;
                        return -1;
                    }
                    
                    // Sleep for a short time
                    proc_usleep(10000);
                }
            }
            
            return -1; // No packets available and no timeout set
        }
        return -1;
    default:
        return -1;
    }
}

ssize_t
sock_sendto(int id, const void *buf, size_t n, const struct sockaddr *addr, int addrlen)
{
    struct sock *s;
    struct ip_endpoint ep;

    s = sock_get(id);
    if (!s) {
        return -17;
    }
    switch (s->type) {
    case SOCK_DGRAM:
        switch (s->family) {
        case AF_INET:
            ep.addr = ((struct sockaddr_in *)addr)->sin_addr;
            ep.port = ((struct sockaddr_in *)addr)->sin_port;
            return udp_sendto(s->desc, (uint8_t *)buf, n, &ep);
        }
        return -1;
    case SOCK_RAW:
        switch (s->family) {
        case AF_INET:
            ep.addr = ((struct sockaddr_in *)addr)->sin_addr;
            ep.port = 0; // RAW socket doesn't use port
            return ip_output(s->protocol, (uint8_t *)buf, n, 0, ep.addr);
        }
        return -1;
    default:
        return -1;
    }
}

int
sock_bind(int id, const struct sockaddr *addr, int addrlen)
{
    struct sock *s;
    struct ip_endpoint ep;
    s = sock_get(id);
    if (!s) {
        return -17;
    }
    switch (s->type) {
    case SOCK_STREAM:
        switch (s->family) {
        case AF_INET:
            ep.addr = ((struct sockaddr_in *)addr)->sin_addr;
            ep.port = ((struct sockaddr_in *)addr)->sin_port;
            return tcp_bind(s->desc, &ep);
        }
        return -1;
    case SOCK_DGRAM:
        switch (s->family) {
        case AF_INET:
            ep.addr = ((struct sockaddr_in *)addr)->sin_addr;
            ep.port = ((struct sockaddr_in *)addr)->sin_port;
            return udp_bind(s->desc, &ep);
        }
        return -1;
    }
    return -1;
}

int
sock_listen(int id, int backlog)
{
    struct sock *s;

    s = sock_get(id);
    if (!s) {
        return -17;
    }
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    switch (s->family) {
    case AF_INET:
        return tcp_listen(s->desc, backlog);
    }
    return -1;
}

int
sock_accept(int id, struct sockaddr *addr, int *addrlen)
{
    struct sock *s, *new_s;
    struct ip_endpoint ep;
    int ret;

    s = sock_get(id);
    if (!s) {
        return -17;
    }
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    switch (s->family) {
    case AF_INET:
        ret = tcp_accept(s->desc, &ep);
        if (ret == -1) {
            return -1;
        }
        ((struct sockaddr_in *)addr)->sin_family = s->family;
        ((struct sockaddr_in *)addr)->sin_addr = ep.addr;
        ((struct sockaddr_in *)addr)->sin_port = ep.port;
        new_s = sock_alloc();
        slog("sock accept %d %d\n",  indexof(socks, new_s), ret);
        if(new_s){
            new_s->family = s->family;
            new_s->type = s->type;
            new_s->desc = ret;
            ret = indexof(socks, new_s);
            return ret;
        }
    }
    return -1;
}

int
sock_connect(int id, const struct sockaddr *addr, int addrlen)
{
    struct sock *s;
    struct ip_endpoint ep;

    s = sock_get(id);
    if (!s) {
        return -17;
    }
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    switch (s->family) {
    case AF_INET:
        ep.addr = ((struct sockaddr_in *)addr)->sin_addr;
        ep.port = ((struct sockaddr_in *)addr)->sin_port;
        return tcp_connect(s->desc, &ep) < 0 ? -1 : 0;
    }
    return -1;
}

ssize_t
sock_recv(int id, void *buf, size_t n)
{
    struct sock *s;

    s = sock_get(id);
    if (!s) {
        return -17;
    }
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    switch (s->family) {
    case AF_INET:
        int ret = tcp_receive(s->desc, (uint8_t *)buf, n);
        return ret;
    }
    return -1;
}

ssize_t
sock_send(int id, const void *buf, size_t n)
{
    struct sock *s;
    s = sock_get(id);
    if (!s) {
        return -17;
    }
    if (s->type != SOCK_STREAM) {
        return -1;
    }
    switch (s->family) {
    case AF_INET:
        return tcp_send(s->desc, (uint8_t *)buf, n);
    }
    return -1;
}

// Add ICMP packet to RAW socket queue
void
sock_add_icmp_packet(const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst)
{
    // Find all RAW sockets with IPPROTO_ICMP
    for (int i = 0; i < countof(socks); i++) {
        struct sock *s = &socks[i];
        if (s->used && s->type == SOCK_RAW && s->protocol == IPPROTO_ICMP) {
            // Create new ICMP packet
            struct icmp_packet *packet = malloc(sizeof(struct icmp_packet));
            if (packet) {
                memset(packet, 0, sizeof(*packet));
                memcpy(packet->data, data, len);
                packet->len = len;
                packet->src = src;
                packet->dst = dst;
                packet->next = NULL;
                
                // Add to queue
                if (s->recv_queue_tail) {
                    s->recv_queue_tail->next = packet;
                } else {
                    s->recv_queue = packet;
                }
                s->recv_queue_tail = packet;
            }
        }
    }
}

// Set socket options
int
sock_setsockopt(int id, int level, int optname, const void *optval, int optlen)
{
    struct sock *s = sock_get(id);
    if (!s) {
        return -17;
    }
    
    if (level == SOL_SOCKET) {
        switch (optname) {
        case SO_RCVTIMEO:
            if (optlen == sizeof(struct timeval)) {
                struct timeval *timeout = (struct timeval *)optval;
                memcpy(&s->rcv_timeout, timeout, sizeof(struct timeval));
                return 0;
            }
            break;
        case SO_SNDTIMEO:
            if (optlen == sizeof(struct timeval)) {
                struct timeval *timeout = (struct timeval *)optval;
                memcpy(&s->snd_timeout, timeout, sizeof(struct timeval));
                return 0;
            }
            break;
        default:
            break;
        }
    }
    
    return -1;
}

// Get socket timeout for a given descriptor
struct timeval*
sock_get_timeout(int desc, int type, int timeout_type)
{
    for (int i = 0; i < countof(socks); i++) {
        struct sock *s = &socks[i];
        if (s->used && s->type == type && s->desc == desc) {
            if (timeout_type == SO_RCVTIMEO) {
                return &s->rcv_timeout;
            } else if (timeout_type == SO_SNDTIMEO) {
                return &s->snd_timeout;
            }
        }
    }
    return NULL;
}

// Get socket absolute timeout for a given descriptor
struct timeval*
sock_get_timeout_abs(struct timeval* timeout, struct timeval* abs_timeout) {
    if (timeout && abs_timeout && (timeout->tv_sec > 0 || timeout->tv_usec > 0)) {
        struct timeval now;
        uint64_t usec;
        kernel_tic(NULL, &usec);
        now.tv_sec = usec / 1000000;
        now.tv_usec = usec % 1000000;
        abs_timeout->tv_sec = now.tv_sec + timeout->tv_sec;
        abs_timeout->tv_usec = now.tv_usec + timeout->tv_usec;
        // Handle microsecond overflow
        if (abs_timeout->tv_usec >= 1000000) {
            abs_timeout->tv_sec += abs_timeout->tv_usec / 1000000;
            abs_timeout->tv_usec %= 1000000;
        }
        return abs_timeout;
    }
    return NULL;
}

inline uint32_t sock_get_timeout_msec(struct timeval* timeout) {
    if(timeout)
        return timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    return 0;
}