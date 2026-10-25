#include <stdint.h>
#include "sys/socket.h"
#include "netinet/in.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ewoksys/vdevice.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <sys/time.h>

static uint32_t fcntl_wait_event(int cmd) {
    switch (cmd) {
        case SOCK_OPEN:
        case SOCK_BIND:
        case SOCK_LISTEN:
        case SOCK_LINK:
        case SOCK_SETOPT:
        case SOCK_GETOPT:
            /*
             * netd completes these control-path commands on the per-socket
             * worker now. They do not have a semantic readiness edge of their
             * own, so use WR as the generic completion channel: the worker
             * posts VFS_EVT_WR on finish and do_vfs_fcntl() waits on per-fd
             * visibility, so a shared /dev/net0 node does not smear readiness
             * across sibling sockets.
             */
            return VFS_EVT_WR;
        case SOCK_RECV:
        case SOCK_RECVFROM:
        case SOCK_ACCEPT:
            return VFS_EVT_RD;
        case SOCK_CONNECT:
        case SOCK_SEND:
        case SOCK_SENDTO:
            return VFS_EVT_WR;
        default:
            return 0;
    }
}

static int fcntl_retry_backoff_us(uint32_t wait_event) {
    /*
     * WR waits are the backpressure gate for blocking send()/connect().
     * Sleeping after every VFS_EVT_WR wake throttles throughput directly:
     * each send() on /dev/net0 first returns RETRY, blocks for WR, then pays
     * a fixed 3ms delay before retrying the actual fcntl. On raspix that caps
     * sustained WLAN uploads around the low hundreds of KB/s even when netd
     * and wland are otherwise idle. Keep the conservative backoff for the
     * non-WR paths that were tuned around stale RD edges, but retry WR
     * immediately once the wakeup says capacity is available.
     */
    return (wait_event == VFS_EVT_WR) ? 0 : 3000;
}

static int fcntl_wait_uses_fd_visibility(int cmd) {
    switch (cmd) {
        case SOCK_RECV:
        case SOCK_RECVFROM:
        case SOCK_ACCEPT:
        case SOCK_CONNECT:
        case SOCK_SEND:
        case SOCK_SENDTO:
            return 1;
        default:
            return 0;
    }
}


static int do_vfs_fcntl(int fd, int cmd, proto_t* arg_in, proto_t* arg_out){ 
    int ret;
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0)
        return -1;
    uint32_t wait_event = fcntl_wait_event(cmd);
    while(1){
        ret = vfs_fcntl(fd, cmd,  arg_in , arg_out);
        if(ret != VFS_ERR_RETRY)
            break;
        if(wait_event == 0 || info.node == 0) {
            usleep(fcntl_retry_backoff_us(wait_event));
            continue;
        }
        /*
         * Only readiness-driven commands should wait via fd-local visibility.
         * Control-path fcntls such as OPEN/BIND/LISTEN/LINK complete via a
         * synthetic sticky WR edge from netd's worker and do NOT have a
         * corresponding live writable state; forcing them through
         * vfs_get_poll_events(fd) would hide their only completion signal.
         */
        if(fcntl_wait_uses_fd_visibility(cmd)) {
            if(vfs_block_by_fd(fd, wait_event) != 0)
                return -1;
        } else if(vfs_block(info.node, wait_event) != 0) {
            return -1;
        }
        usleep(fcntl_retry_backoff_us(wait_event));
    };

    return ret;
}

int socket (int domain, int type, int protocol){
    int ret;
    proto_t in,out;
    int fd = open("/dev/net0", 0);
    if(fd < 0)
        return -1;
    PF->init(&in)->addi(&in, domain)->addi(&in, type)->addi(&in, protocol);
    PF->init(&out);
    ret = do_vfs_fcntl(fd, SOCK_OPEN, &in , &out);
    if(ret != 0) {
        PF->clear(&in);
        PF->clear(&out);
        close(fd);
        return -1;
    }
    int sock = proto_read_int(&out);
    PF->clear(&in);
    PF->clear(&out);

    if (sock < 0) {
        close(fd);
        return -1;
    }

    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0) {
        close(fd);
        return -1;
    }
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
    ret = do_vfs_fcntl(fd, SOCK_BIND, &in , &out);
    if(ret == 0)
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
    ret = do_vfs_fcntl(fd, SOCK_CONNECT, &in , &out);
    if(ret == 0) {
        int sock_errno;
        ret = proto_read_int(&out);
        sock_errno = proto_read_int(&out);
        if (ret < 0 && sock_errno != 0) {
            errno = sock_errno;
        }
    }
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
    ret = do_vfs_fcntl(fd, SOCK_SEND, &in , &out);
    if(ret == 0) {
        int sock_errno;
        ret = proto_read_int(&out);
        sock_errno = proto_read_int(&out);
        if (ret < 0 && sock_errno != 0) {
            errno = sock_errno;
        }
    }
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
    ret = do_vfs_fcntl(fd, SOCK_RECV, &in , &out);
    if(ret == 0) {
        int sock_errno;
        ret = proto_read_int(&out);
        if(ret > 0){
           proto_read_to(&out, buf, ret);
        }
        sock_errno = proto_read_int(&out);
        if (ret < 0 && sock_errno != 0) {
            errno = sock_errno;
        }
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
    ret = do_vfs_fcntl(fd, SOCK_SENDTO, &in , &out);
    if(ret == 0) {
        int sock_errno;
        ret = proto_read_int(&out);
        sock_errno = proto_read_int(&out);
        if (ret < 0 && sock_errno != 0) {
            errno = sock_errno;
        }
    }
    PF->clear(&in);
    PF->clear(&out);
    return ret;
}

int32_t recvfrom (int fd, void * buf, uint32_t n,
             int flags, struct sockaddr* addr,
             uint32_t * addr_len){
    int ret;
    uint32_t out_addr_len;
    proto_t in,out;
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0)
        return -1;

    PF->init(&in)->addi(&in, n)->addi(&in, *addr_len);
    PF->init(&out);
    ret = do_vfs_fcntl(fd, SOCK_RECVFROM, &in , &out);
    if(ret == 0) {
        int sock_errno;
        ret = proto_read_int(&out);
        if(ret > 0){
            out_addr_len = (uint32_t)proto_read_int(&out);
            proto_read_to(&out, buf, ret);
            if (addr != NULL && addr_len != NULL) {
                uint32_t copy_len = *addr_len < out_addr_len ? *addr_len : out_addr_len;
                proto_read_to(&out, addr, copy_len);
                *addr_len = out_addr_len;
            }
        }
        sock_errno = proto_read_int(&out);
        if (ret < 0 && sock_errno != 0) {
            errno = sock_errno;
        }
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
    ret = do_vfs_fcntl(fd, SOCK_SETOPT, &in , &out);
    if(ret == 0)
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
    ret = do_vfs_fcntl(fd, SOCK_LISTEN, &in , &out);
    if(ret == 0)
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
    if(ret != 0) {
        klog("socket: accept fcntl failed fd=%d ret=%d node=%u\n",
                fd, ret, info.node);
        PF->clear(&out);
        return -1;
    }
    ret = proto_read_int(&out);
    if(ret < 0) {
        int sock_errno = proto_read_int(&out);
        if(sock_errno != 0)
            errno = sock_errno;
        klog("socket: accept result failed fd=%d ret=%d out_size=%u node=%u\n",
                fd, ret, (unsigned int)out.size, info.node);
        PF->clear(&out);
        return -1;
    }
    if(ret >= 0 && addr != NULL && addr_len != NULL) {
        proto_read_to(&out, addr, *addr_len);
    }
    PF->clear(&out);
    int accept_fd = open("/dev/net0", 0);
    if(accept_fd < 0) {
        klog("socket: accept open link fd failed listen_fd=%d sock=%d node=%u err=%d\n",
                fd, ret, info.node, errno);
        return -1; 
    }

    PF->init(&in)->addi(&in, ret);
    PF->init(&out);
    ret = do_vfs_fcntl(accept_fd, SOCK_LINK, &in , &out);
    if(ret != 0) {
        klog("socket: accept link failed listen_fd=%d accept_fd=%d sock=%d ret=%d node=%u\n",
                fd, accept_fd, proto_read_int(&in), ret, info.node);
        PF->clear(&in);
        PF->clear(&out);
        close(accept_fd);
        return -1;
    }
    ret = proto_read_int(&out);
    PF->clear(&in);
    PF->clear(&out);
    if (ret != 0) {
        close(accept_fd);
        return -1;
    }

    return accept_fd;
}

int shutdown (int fd, int how){
    int ret;
    proto_t in, out;
    PF->init(&in)->addi(&in, how);
    PF->init(&out);
    ret = do_vfs_fcntl(fd, SOCK_CLOSE, NULL , &out);
    if(ret == 0)
        ret = proto_read_int(&out);
    PF->clear(&out);
    return ret;
}

// DNS-related structures
struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed));

struct dns_question {
    // The domain name portion has variable length and ends with a zero byte.
    // It is followed by the type and class fields.
    uint16_t type;
    uint16_t class;
} __attribute__((packed));

struct dns_rr {
    // The domain name portion has variable length and ends with a zero byte.
    // It is followed by type, class, ttl, rdlength, and rdata.
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    // The rdata payload has variable length.
} __attribute__((packed));

// Static storage used for gethostbyname() return data.
static struct hostent hostent_result;
static char hostent_name_buf[256];
static char* hostent_name;
static char* hostent_aliases[2];
static struct in_addr hostent_addr;
static char* hostent_addr_list[2];

// Resolve a hostname through the specified DNS server.
static int dns_resolve(const char* domain, struct in_addr* addr, const char* dns_server_ipv4) {
    int sockfd;
    struct sockaddr_in server_addr, from_addr;
    socklen_t from_len;
    uint8_t buf[512];
    int len, i, attempt;
    struct dns_header* header;
    uint8_t* ptr;
    uint16_t qtype, qclass;
    uint16_t ancount, type, class, rdlength;

    // Create a UDP socket.
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = 3; // 3-second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return -1;
    }

    // Set the DNS server address.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    server_addr.sin_addr.s_un.s_addr = inet_addr(dns_server_ipv4);

    // Build the DNS query packet.
    header = (struct dns_header*)buf;
    header->id = htons(12345);
    header->flags = htons(0x0100); // Standard query
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;

    // Encode the queried domain name.
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
    *ptr++ = 0; // Domain name terminator

    // Append the query type and class.
    qtype = htons(1); // A record
    qclass = htons(1); // IN class
    memcpy(ptr, &qtype, 2);
    ptr += 2;
    memcpy(ptr, &qclass, 2);
    ptr += 2;

    len = ptr - buf;

    for (attempt = 0; attempt < 3; ++attempt) {
        // Send the query.
        if (sendto(sockfd, buf, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            close(sockfd);
            return -1;
        }

        // Receive the response.
        from_len = sizeof(from_addr);
        while (1) {
            len = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&from_addr, &from_len);
            if (len >= 0) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == ETIMEDOUT) {
                break;
            }
            close(sockfd);
            return -1;
        }

        if (len >= 0) {
            break;
        }
    }

    if (len < 0) {
        close(sockfd);
        return -1;
    }

    // Parse the response.
    header = (struct dns_header*)buf;
    ancount = ntohs(header->ancount);

    // Skip the question section.
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
    ptr += 1; // Skip the domain name terminator
    if (ptr + 4 > buf + len) {
        close(sockfd);
        return -1;
    }
    // Validate the question type and class.
    qtype = ntohs(*(uint16_t*)ptr);
    qclass = ntohs(*(uint16_t*)(ptr + 2));
    ptr += 4; // Skip type and class

    // Parse the answer section.
    for (i = 0; i < ancount; i++) {
        // Skip the domain name, which may use compressed encoding.
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
                // Compressed name format, skip the 2-byte pointer.
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

        // Read type, class, ttl, and rdlength.
        if (ptr + 10 > buf + len) {
            close(sockfd);
            return -1;
        }
        // Read through properly aligned temporaries.
        uint16_t type_val = *(uint16_t*)ptr;
        type = ntohs(type_val);
        ptr += 2;
        uint16_t class_val = *(uint16_t*)ptr;
        class = ntohs(class_val);
        ptr += 2;
        ptr += 4; // Skip ttl
        uint16_t rdlength_val = *(uint16_t*)ptr;
        rdlength = ntohs(rdlength_val);
        ptr += 2;

        // Extract the IPv4 address from an A record.
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
    int last_errno = 0;
    // Validate input arguments.
    if (name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    // First try to parse the input as an IPv4 literal.
    if (inet_pton(AF_INET, name, &hostent_addr) == 1) {
        // Parsed successfully as an IPv4 address.
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
        last_errno = errno;
    }

    // Resolution failed.
    errno = last_errno != 0 ? last_errno : ENOENT;
    return NULL;
}

static int addrinfo_service(const char *service, int flags,
                            uint16_t *port, int *protocol) {
    char *end;
    unsigned long value;

    *protocol = 0;
    if (service == NULL || *service == '\0') {
        *port = 0;
        return 0;
    }
    if (!(flags & AI_NUMERICSERV)) {
        struct service_ent {
            const char *name;
            uint16_t port;
            int protocol;
        };
        static const struct service_ent services[] = {
            { "domain", 53, IPPROTO_UDP },
            { "ftp", 21, IPPROTO_TCP },
            { "git", 9418, IPPROTO_TCP },
            { "http", 80, IPPROTO_TCP },
            { "https", 443, IPPROTO_TCP },
            { "ssh", 22, IPPROTO_TCP },
            { "telnet", 23, IPPROTO_TCP }
        };
        size_t i;

        for (i = 0; i < sizeof(services) / sizeof(services[0]); i++) {
            if (strcmp(service, services[i].name) == 0) {
                *port = services[i].port;
                *protocol = services[i].protocol;
                return 0;
            }
        }
    }

    errno = 0;
    value = strtoul(service, &end, 10);
    if (errno || end == service || *end != '\0' || value > 65535)
        return EAI_SERVICE;
    *port = (uint16_t)value;
    return 0;
}

static int addrinfo_append(struct addrinfo **list, struct in_addr address,
                           uint16_t port, int socktype, int protocol,
                           const char *canonname) {
    struct addrinfo *entry = calloc(1, sizeof(*entry));
    struct sockaddr_in *in = calloc(1, sizeof(*in));

    if (entry == NULL || in == NULL) {
        free(entry);
        free(in);
        return EAI_MEMORY;
    }
    entry->ai_family = AF_INET;
    entry->ai_socktype = socktype;
    entry->ai_protocol = protocol;
    entry->ai_addrlen = sizeof(*in);
    entry->ai_addr = (struct sockaddr *)in;
    in->sin_len = sizeof(*in);
    in->sin_family = AF_INET;
    in->sin_port = htons(port);
    in->sin_addr = address;
    if (canonname != NULL) {
        size_t len = strlen(canonname) + 1;
        entry->ai_canonname = malloc(len);
        if (entry->ai_canonname == NULL) {
            free(in);
            free(entry);
            return EAI_MEMORY;
        }
        memcpy(entry->ai_canonname, canonname, len);
    }
    entry->ai_next = *list;
    *list = entry;
    return 0;
}

void freeaddrinfo(struct addrinfo *res) {
    while (res != NULL) {
        struct addrinfo *next = res->ai_next;
        free(res->ai_addr);
        free(res->ai_canonname);
        free(res);
        res = next;
    }
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res) {
    struct in_addr address;
    struct hostent *host;
    uint16_t port;
    int flags = hints != NULL ? hints->ai_flags : 0;
    int family = hints != NULL ? hints->ai_family : AF_UNSPEC;
    int socktype = hints != NULL ? hints->ai_socktype : 0;
    int protocol = hints != NULL ? hints->ai_protocol : 0;
    int service_protocol;
    int ret;

    if (res == NULL || (node == NULL && service == NULL))
        return EAI_NONAME;
    *res = NULL;
    if (family != AF_UNSPEC && family != AF_INET)
        return EAI_FAMILY;
    if (socktype != 0 && socktype != SOCK_STREAM && socktype != SOCK_DGRAM)
        return EAI_SOCKTYPE;
    if (protocol != 0 && protocol != IPPROTO_TCP && protocol != IPPROTO_UDP)
        return EAI_SERVICE;
    ret = addrinfo_service(service, flags, &port, &service_protocol);
    if (ret != 0)
        return ret;
    if (protocol != 0 && service_protocol != 0 && protocol != service_protocol)
        return EAI_SERVICE;
    if (protocol == 0)
        protocol = service_protocol;
    if (socktype == SOCK_STREAM && protocol == IPPROTO_UDP)
        return EAI_SOCKTYPE;
    if (socktype == SOCK_DGRAM && protocol == IPPROTO_TCP)
        return EAI_SOCKTYPE;

    if (node == NULL) {
        address.s_un.s_addr = (flags & AI_PASSIVE) ? INADDR_ANY : htonl((in_addr_t)0x7f000001);
    } else if (inet_pton(AF_INET, node, &address) != 1) {
        if (flags & AI_NUMERICHOST)
            return EAI_NONAME;
        host = gethostbyname(node);
        if (host == NULL || host->h_addrtype != AF_INET ||
            host->h_length != sizeof(address) || host->h_addr_list[0] == NULL)
            return EAI_NONAME;
        memcpy(&address, host->h_addr_list[0], sizeof(address));
    }

    if ((socktype == 0 || socktype == SOCK_STREAM) &&
        (protocol == 0 || protocol == IPPROTO_TCP)) {
        ret = addrinfo_append(res, address, port, SOCK_STREAM, IPPROTO_TCP,
                              (flags & AI_CANONNAME) ? node : NULL);
        if (ret != 0) {
            freeaddrinfo(*res);
            *res = NULL;
            return ret;
        }
    }
    if ((socktype == 0 || socktype == SOCK_DGRAM) &&
        (protocol == 0 || protocol == IPPROTO_UDP)) {
        ret = addrinfo_append(res, address, port, SOCK_DGRAM, IPPROTO_UDP,
                              (flags & AI_CANONNAME) ? node : NULL);
        if (ret != 0) {
            freeaddrinfo(*res);
            *res = NULL;
            return ret;
        }
    }
    return *res != NULL ? 0 : EAI_SOCKTYPE;
}

const char *gai_strerror(int errcode) {
    switch (errcode) {
    case 0: return "success";
    case EAI_BADFLAGS: return "invalid flags";
    case EAI_NONAME: return "name or service not known";
    case EAI_AGAIN: return "temporary name resolution failure";
    case EAI_FAIL: return "name resolution failure";
    case EAI_FAMILY: return "address family not supported";
    case EAI_SOCKTYPE: return "socket type not supported";
    case EAI_SERVICE: return "service not supported for socket type";
    case EAI_MEMORY: return "out of memory";
    case EAI_OVERFLOW: return "buffer too small";
    default: return "unknown name resolution error";
    }
}

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, int flags) {
    const struct sockaddr_in *in;

    if (addr == NULL || addrlen < sizeof(struct sockaddr_in) ||
        addr->sa_family != AF_INET)
        return EAI_FAMILY;
    in = (const struct sockaddr_in *)addr;
    if (host != NULL) {
        if (!(flags & NI_NUMERICHOST))
            return EAI_NONAME;
        if (inet_ntop(AF_INET, &in->sin_addr, host, hostlen) == NULL)
            return EAI_OVERFLOW;
    }
    if (serv != NULL) {
        int len;
        if (!(flags & NI_NUMERICSERV))
            return EAI_NONAME;
        len = snprintf(serv, servlen, "%u", (unsigned)ntohs(in->sin_port));
        if (len < 0 || (socklen_t)len >= servlen)
            return EAI_OVERFLOW;
    }
    return 0;
}
