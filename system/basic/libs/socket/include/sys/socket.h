#ifndef	_SYS_SOCKET_H
#define	_SYS_SOCKET_H	

#include <stdint.h>
#include <netinet/in.h>

struct sockaddr
{
  unsigned short int sa_family;
  unsigned char sa_data[14];
};

#define h_addr h_addr_list[0] 

struct hostent {
     char  *h_name;         /* official name of host */
     char **h_aliases;         /* alias list */
     int    h_addrtype;       /* host address type */
     int    h_length;         /* length of address */
     char **h_addr_list;        /* list of addresses */
 };

  struct addrinfo {
          int              ai_flags;
          int              ai_family;
          int              ai_socktype;
          int              ai_protocol;
          uint32_t         ai_addrlen;
          struct sockaddr  *ai_addr;
          char            *ai_canonname;
          struct addrinfo   *ai_next;
 };



struct iphdr {
         uint8_t   tos;
         uint16_t  tot_len;
         uint16_t  id;
         uint16_t  frag_off;
         uint8_t   ttl;
         uint8_t   protocol;
         uint16_t  check;
         uint32_t  saddr;
         uint32_t  daddr;
};

struct ip {
    uint8_t  ip_hl:4,     
            ip_v:4;       
    uint8_t  ip_tos;      
    int16_t   ip_len;     
    uint16_t ip_id;       
    int16_t   ip_off;     
    uint8_t  ip_ttl;      
    uint8_t  ip_p;         
    uint16_t ip_sum;       
    struct   in_addr ip_src,ip_dst; 
};


struct sockaddr_storage {
  	uint8_t  ss_len;         /* address length */
    uint8_t  ss_family;     // address family

    // all this is padding, implementation specific, ignore it:
    char      __ss_pad1[6];
    int64_t   __ss_align;
    char      __ss_pad2[6];
};

struct msghdr {
	void            *msg_name;     
	uint32_t        msg_namelen;   
	struct          iovec *msg_iov;
	int             msg_iovlen;    
	void            *msg_control;  
	uint32_t        msg_controllen;
	int             msg_flags;     
};


#define SHUT_RD		SHUT_RD
#define SHUT_WR		SHUT_WR
#define SHUT_RDWR	SHUT_RDWR

#define SOCK_STREAM   1
#define SOCK_DGRAM    2
#define SOCK_RAW      3

#define AF_UNSPEC 0
#define AF_INET   2


#define NETD_SET_PROTOL    0
#define NETD_SET_ADDRESS   1
#define NETD_GET_ADDRESS   2

#define SOCK_OPEN       1
#define SOCK_SENDTO     2
#define SOCK_RECVFROM   3
#define SOCK_BIND       4
#define SOCK_LISTEN     5
#define SOCK_ACCEPT     6
#define SOCK_SEND       7
#define SOCK_RECV       8 
#define SOCK_CLOSE      9 
#define SOCK_LINK       10
#define SOCK_CONNECT    11
#define SOCK_PAIR       12

#define SOCK_REQUEST    100
#define SOCK_ACK        101



#define NETD_SENDTO     3
#define NETD_RECVFROM   4


int socket (int domain, int type, int protocol);

int socketpair (int domain, int type, int protocol,
		       int fds[2]);
int bind (int fd, const struct sockaddr* addr, uint32_t len);
int getsockname (int fd, struct sockaddr* addr,
			uint32_t * len);
int connect (int fd, const struct sockaddr* addr, uint32_t len);
int getpeername (int fd, struct sockaddr* addr,
			uint32_t * len);
int32_t send (int fd, const void *buf, uint32_t n, int flags);
int32_t recv (int fd, void *buf, uint32_t n, int flags);
int32_t sendto (int fd, const void *buf, uint32_t n,
		       int flags, const struct sockaddr* addr,
		       uint32_t addr_len);
int32_t recvfrom (int fd, void * buf, uint32_t n,
			 int flags, struct sockaddr* addr,
			 uint32_t * addr_len);

int32_t sendmsg (int fd, const struct msghdr *message,int flags);

int32_t recvmsg (int fd, struct msghdr *message, int flags);
int getsockopt (int fd, int level, int optname,
		       void * optval,
		       uint32_t * optlen);
int setsockopt (int fd, int level, int optname,
		       const void *optval, uint32_t optlen);
int listen (int fd, int n);
int accept (int fd, struct sockaddr* addr,
		   uint32_t * addr_len);

int shutdown (int fd, int how);

struct hostent *gethostbyname(const char *name);

int getaddrinfo( const char *node, const char *service, 
                const struct addrinfo *hints, struct addrinfo **res);
int freeaddrinfo(struct addrinfo **res);

#endif /* sys/socket.h */
