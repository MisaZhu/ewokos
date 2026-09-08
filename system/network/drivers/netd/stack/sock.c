#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "util.h"
#include "net.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"
#include "../task.h"

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

#ifndef TCP_PCB_SIZE
#define TCP_PCB_SIZE 64
#endif
#ifndef UDP_PCB_SIZE
#define UDP_PCB_SIZE 16
#endif
static int tcp_desc_to_sock[TCP_PCB_SIZE];
static int udp_desc_to_sock[UDP_PCB_SIZE];

/*
 * Protects the socks[] slot allocation state (used flag), the desc->sock
 * reverse maps and the RAW ICMP receive queues against concurrent IPC pool
 * workers. It is a LEAF lock: never call tcp_xxx/udp_xxx stack functions
 * (which take the stack mutex) while holding it.
 */
static pthread_mutex_t socks_lock;

void sock_init_maps(void) {
    memset(tcp_desc_to_sock, -1, sizeof(tcp_desc_to_sock));
    memset(udp_desc_to_sock, -1, sizeof(udp_desc_to_sock));
    pthread_mutex_init(&socks_lock, NULL);
}

/* callers must hold socks_lock */
static void sock_map_register(struct sock *s) {
    int id = indexof(socks, s);
    if (s->type == SOCK_STREAM && s->desc >= 0 && s->desc < TCP_PCB_SIZE)
        tcp_desc_to_sock[s->desc] = id;
    else if (s->type == SOCK_DGRAM && s->desc >= 0 && s->desc < UDP_PCB_SIZE)
        udp_desc_to_sock[s->desc] = id;
}

/* callers must hold socks_lock */
static void sock_map_unregister(struct sock *s) {
    int id = indexof(socks, s);
    if (s->type == SOCK_STREAM && s->desc >= 0 && s->desc < TCP_PCB_SIZE) {
        if (tcp_desc_to_sock[s->desc] == id)
            tcp_desc_to_sock[s->desc] = -1;
    } else if (s->type == SOCK_DGRAM && s->desc >= 0 && s->desc < UDP_PCB_SIZE) {
        if (udp_desc_to_sock[s->desc] == id)
            udp_desc_to_sock[s->desc] = -1;
    }
}

static int
sock_used_count(void)
{
    int used = 0;
    struct sock *entry;

    for (entry = socks; entry < tailof(socks); entry++) {
        if (entry->used) {
            used++;
        }
    }
    return used;
}

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

    pthread_mutex_lock(&socks_lock);
    for (entry = socks; entry < tailof(socks); entry++) {
        if (!entry->used) {
            entry->used = 1;
            pthread_mutex_unlock(&socks_lock);
            return entry;
        }
    }
    pthread_mutex_unlock(&socks_lock);
    errorf("sock_alloc exhausted: used=%d max=%d", sock_used_count(), SOCKS_MAX);
    return NULL;
}

static int
sock_free(struct sock *s)
{
    struct icmp_packet *packet = NULL;

    pthread_mutex_lock(&socks_lock);
    sock_map_unregister(s);
    // Detach the ICMP packet queue of RAW sockets, free it outside the lock
    if (s->type == SOCK_RAW) {
        packet = s->recv_queue;
    }
    memset(s, 0, sizeof(*s));
    pthread_mutex_unlock(&socks_lock);

    while (packet) {
        struct icmp_packet *next = packet->next;
        free(packet);
        packet = next;
    }
    return 0;
}

static struct sock *
sock_get(int id)
{
    if (id < 0 || id >= (int)countof(socks)) {
        /* out of range */
        return NULL;
    }
    if (!socks[id].used) {
        /* slot free or already closed */
        return NULL;
    }
    return &socks[id];
}

int
sock_open(int domain, int type, int protocol)
{
    struct sock *s;
    if (domain != AF_INET) {
        errorf("sock_open invalid domain=%d", domain);
        return -17;
    }
    if (type != SOCK_STREAM && type != SOCK_DGRAM && type != SOCK_RAW) {
        errorf("sock_open invalid type=%d", type);
        return -1;
    }
    if (protocol == 0) {
        protocol = (type == SOCK_STREAM) ? IPPROTO_TCP : (type == SOCK_DGRAM) ? IPPROTO_UDP : 0;
    }
    if ((type == SOCK_STREAM && protocol != IPPROTO_TCP) ||
        (type == SOCK_DGRAM && protocol != IPPROTO_UDP)) {
        errorf("sock_open protocol mismatch: type=%d protocol=%d", type, protocol);
        return -1;
    }
    s = sock_alloc();
    if (!s) {
        errorf("sock_open no slot: type=%d protocol=%d used=%d",
            type, protocol, sock_used_count());
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
        errorf("sock_open backend failure: type=%d protocol=%d used=%d",
            s->type, s->protocol, sock_used_count());
        sock_free(s);
        return -1;
    }
    pthread_mutex_lock(&socks_lock);
    sock_map_register(s);
    pthread_mutex_unlock(&socks_lock);
    return indexof(socks, s);
}

int
sock_close(int id)
{
    struct sock *s;
    s = sock_get(id);
    if (!s) {
        errorf("sock_close invalid id=%d used=%d", id, sock_used_count());
        return -17;
    }
    switch (s->type) {
    case SOCK_STREAM: {
        /*
         * tcp_close() absorbs TX backpressure internally now (the FIN rides
         * the retransmit queue), so it returns 0 or -17 for every known
         * state and never needs a retry. The old 100x3ms retry loop queued
         * a duplicate FIN entry per attempt and, once exhausted, abandoned
         * the pcb without a sock -- leaking the slot forever.
         */
        int ret = tcp_close(s->desc);
        if (ret != 0 && ret != -17) {
            errorf("sock_close: tcp_close desc=%d unexpected ret=%d", s->desc, ret);
        }
        break;
    }
    case SOCK_DGRAM:
        udp_close(s->desc);
        break;
    case SOCK_RAW:
        // RAW socket doesn't need closing
        break;
    default:
        return -1;
    }
    sock_free(s);
    return 0;
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
                ((struct sockaddr_in *)addr)->sin_family = AF_INET;
                ((struct sockaddr_in *)addr)->sin_addr = ep.addr;
                ((struct sockaddr_in *)addr)->sin_port = ep.port;
                if (addrlen) {
                    *addrlen = sizeof(struct sockaddr_in);
                }
            }
            return ret;
        }
        return -1;
    case SOCK_RAW:
        switch (s->family) {
        case AF_INET: {
            struct icmp_packet *packet = NULL;

            // Non-blocking pop from the receive queue; the netd task layer
            // enforces SO_RCVTIMEO by re-issuing the request, so a pool
            // worker must never park in here.
            pthread_mutex_lock(&socks_lock);
            if (s->recv_queue) {
                packet = s->recv_queue;
                s->recv_queue = packet->next;
                if (!s->recv_queue) {
                    s->recv_queue_tail = NULL;
                }
            }
            pthread_mutex_unlock(&socks_lock);

            if (!packet) {
                errno = EAGAIN;
                return -1;
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

            free(packet);
            return copy_len;
        }
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
        errno = EBADF;
        return -17;
    }
    if (s->type != SOCK_STREAM) {
        errno = EINVAL;
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
            pthread_mutex_lock(&socks_lock);
            sock_map_register(new_s);
            pthread_mutex_unlock(&socks_lock);
            ret = indexof(socks, new_s);
            return ret;
        }
        errno = EMFILE;
        tcp_close(ret);
        return -1;
    }
    errno = EAFNOSUPPORT;
    return -1;
}

int
sock_connect(int id, const struct sockaddr *addr, int addrlen)
{
    struct sock *s;
    struct ip_endpoint ep;
    int ret = -1;

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
        ret = tcp_connect(s->desc, &ep) < 0 ? -1 : 0;
        break;
    }
    return ret;
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
        if (ret < 0 && errno == 0) {
            /*
             * Some TCP paths still surface "would block / retry" as -1 without
             * setting errno. Normalize that here so upper VFS layers keep the
             * read blocked instead of propagating a fake hard error.
             */
            errno = EAGAIN;
        }
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
        errno = EBADF;
        return -17;
    }
    if (s->type != SOCK_STREAM) {
        errno = EINVAL;
        return -1;
    }
    switch (s->family) {
    case AF_INET:
        return tcp_send(s->desc, (uint8_t *)buf, n);
    }
    errno = EINVAL;
    return -1;
}

// Add ICMP packet to RAW socket queue
void
sock_add_icmp_packet(const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst)
{
    int wake_ids[8];
    int wake_cnt = 0;

    // Find all RAW sockets with IPPROTO_ICMP
    pthread_mutex_lock(&socks_lock);
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
                if (wake_cnt < (int)countof(wake_ids))
                    wake_ids[wake_cnt++] = i;
            }
        }
    }
    pthread_mutex_unlock(&socks_lock);

    // Wake readers outside socks_lock: the task layer takes its own locks
    for (int i = 0; i < wake_cnt; i++)
        task_wakeup_raw_readers(wake_ids[i]);
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

// Get socket options
int
sock_getsockopt(int id, int level, int optname, void *optval, int *optlen)
{
    struct sock *s = sock_get(id);
    if (!s) {
        return -17;
    }
    
    if (level == SOL_SOCKET) {
        switch (optname) {
        case SO_ERROR: {
            if (*optlen >= sizeof(int)) {
                /*
                 * Report the real pending error for stream sockets instead of a
                 * hardcoded 0. A permanent 0 made non-blocking connect failures
                 * and peer resets invisible to getsockopt(SO_ERROR) consumers
                 * (TLS clients, ffmpeg, wolfSSL).
                 */
                int so_err = 0;
                if (s->type == SOCK_STREAM && s->family == AF_INET)
                    so_err = tcp_socket_error(s->desc);
                *(int *)optval = so_err;
                *optlen = sizeof(int);
                return 0;
            }
            break;
        }
        case SO_RCVTIMEO:
            if (*optlen >= sizeof(struct timeval)) {
                memcpy(optval, &s->rcv_timeout, sizeof(struct timeval));
                *optlen = sizeof(struct timeval);
                return 0;
            }
            break;
        case SO_SNDTIMEO:
            if (*optlen >= sizeof(struct timeval)) {
                memcpy(optval, &s->snd_timeout, sizeof(struct timeval));
                *optlen = sizeof(struct timeval);
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

/*
 * SO_RCVTIMEO of a socket id, for the netd task layer.
 *
 * netd never lets a worker block inside tcp_receive()/udp_recvfrom(), so the
 * timeout those functions implement internally is unreachable; the task state
 * machine has to enforce the deadline itself.
 * Returns 0 and fills *timeout when a non-zero receive timeout is set.
 */
int
sock_get_rcvtimeo(int id, struct timeval *timeout)
{
    struct sock *s;

    if (!timeout) {
        return -1;
    }
    s = sock_get(id);
    if (!s) {
        return -1;
    }
    if (s->rcv_timeout.tv_sec == 0 && s->rcv_timeout.tv_usec == 0) {
        return -1;
    }
    *timeout = s->rcv_timeout;
    return 0;
}

int
sock_readable(int id)
{
    struct sock *s;
    s = sock_get(id);
    if (!s) {
        return 0;
    }
    switch (s->type) {
    case SOCK_STREAM:
        return tcp_readable(s->desc);
    case SOCK_DGRAM:
        return udp_readable(s->desc);
    case SOCK_RAW:
        return s->recv_queue != NULL;
    }
    return 0;
}

int
sock_poll_readable(int id)
{
    struct sock *s;

    s = sock_get(id);
    if (!s) {
        return 0;
    }
    switch (s->type) {
    case SOCK_STREAM:
        return tcp_poll_readable(s->desc);
    case SOCK_DGRAM:
        return udp_poll_readable(s->desc);
    case SOCK_RAW:
        return s->recv_queue != NULL;
    }
    return 0;
}

int
sock_data_readable(int id)
{
    struct sock *s;

    s = sock_get(id);
    if (!s) {
        return 0;
    }
    switch (s->type) {
    case SOCK_STREAM:
        /*
         * Background read-task scanning should only chase real queued payload.
         * EOF/closed transitions already wake poll waiters through the TCP
         * state-change paths, and treating them as perpetually readable here
         * keeps netd pinned in its fast poll loop.
         */
        return tcp_data_readable(s->desc);
    case SOCK_DGRAM:
        return udp_readable(s->desc);
    case SOCK_RAW:
        return s->recv_queue != NULL;
    }
    return 0;
}

int
sock_writable(int id)
{
    struct sock *s;
    s = sock_get(id);
    if (!s) {
        return 0;
    }
    switch (s->type) {
    case SOCK_STREAM:
        return tcp_writable(s->desc);
    }
    /* UDP/RAW have no send window; always accept the write. */
    return 1;
}

int
sock_poll_writable(int id)
{
    struct sock *s;

    s = sock_get(id);
    if (!s) {
        return 0;
    }
    switch (s->type) {
    case SOCK_STREAM:
        return tcp_poll_writable(s->desc);
    }
    /* UDP/RAW have no send window; always accept the write. */
    return 1;
}

/*
 * True while a TCP connect is still inside the handshake
 * (SYN_SENT/SYN_RECEIVED). tcp_poll_writable() reports non-transfer states
 * as writable, so without this check a connect() blocked on VFS_EVT_WR
 * would busy-spin instead of sleeping until the handshake completes.
 */
int
sock_connect_pending(int id)
{
    struct sock *s;
    int state;

    s = sock_get(id);
    if (!s || s->type != SOCK_STREAM) {
        return 0;
    }
    state = tcp_state(s->desc);
    return (state == TCP_STATE_SYN_SENT || state == TCP_STATE_SYN_RECEIVED);
}

int
sock_get_desc(int id)
{
    struct sock *s = sock_get(id);
    if (!s) {
        return -1;
    }
    return s->desc;
}

int
sock_get_type(int id)
{
    struct sock *s = sock_get(id);
    if (!s) {
        return -1;
    }
    return s->type;
}

int
sock_id_from_tcp_desc(int tcp_desc)
{
    int id;

    if (tcp_desc < 0 || tcp_desc >= TCP_PCB_SIZE)
        return -1;
    pthread_mutex_lock(&socks_lock);
    id = tcp_desc_to_sock[tcp_desc];
    pthread_mutex_unlock(&socks_lock);
    return id;
}

int
sock_id_from_udp_desc(int udp_desc)
{
    int id;

    if (udp_desc < 0 || udp_desc >= UDP_PCB_SIZE)
        return -1;
    pthread_mutex_lock(&socks_lock);
    id = udp_desc_to_sock[udp_desc];
    pthread_mutex_unlock(&socks_lock);
    return id;
}
